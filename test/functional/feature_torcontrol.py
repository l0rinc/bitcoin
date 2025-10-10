#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test torcontrol functionality with a mock Tor control server."""
from contextlib import contextmanager
import os
from pathlib import Path
import socket
import sys
import threading
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    ensure_for,
    p2p_port,
)


class MockTorControlServer:
    def __init__(self, port, manual_mode=False):
        self.port = port
        self.sock = None
        self.conn = None
        self.running = False
        self.thread = None
        self.received_commands = []
        self.manual_mode = manual_mode
        self.conn_ready = threading.Event()

    def start(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.settimeout(1.0)
        self.sock.bind(('127.0.0.1', self.port))
        self.sock.listen(1)
        self.running = True
        self.thread = threading.Thread(target=self._serve)
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        self.running = False
        if self.conn:
            self.conn.close()
        if self.sock:
            self.sock.close()
        if self.thread:
            self.thread.join(timeout=5)

    def _serve(self):
        while self.running:
            try:
                self.conn, _ = self.sock.accept()
                self.conn.settimeout(1.0)
                self.conn_ready.set()
                self._handle_connection(self.conn)
            except socket.timeout:
                continue
            except OSError:
                break

    def _handle_connection(self, conn):
        try:
            buf = b""
            while self.running:
                try:
                    data = conn.recv(1024)
                    if not data:
                        break
                    buf += data
                    while b"\r\n" in buf:
                        line, buf = buf.split(b"\r\n", 1)
                        command = line.decode('utf-8').strip()
                        if command:
                            self.received_commands.append(command)
                            if not self.manual_mode:
                                response = self._get_response(command)
                                conn.sendall(response.encode('utf-8'))
                except socket.timeout:
                    continue
        finally:
            conn.close()

    def send_raw(self, data):
        if self.conn:
            self.conn.sendall(data.encode('utf-8'))

    def _get_response(self, command):
        if command == "PROTOCOLINFO 1":
            return (
                "250-PROTOCOLINFO 1\r\n"
                "250-AUTH METHODS=NULL\r\n"
                "250-VERSION Tor=\"0.1.2.3\"\r\n"
                "250 OK\r\n"
            )
        elif command == "AUTHENTICATE":
            return "250 OK\r\n"
        elif command.startswith("ADD_ONION"):
            return (
                "250-ServiceID=testserviceid1234567890123456789012345678901234567890123456\r\n"
                "250 OK\r\n"
            )
        elif command.startswith("GETINFO"):
            return "250-net/listeners/socks=\"127.0.0.1:9050\"\r\n250 OK\r\n"
        else:
            return "510 Unrecognized command\r\n"


class TorControlTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def next_port(self):
        self._port_counter = getattr(self, '_port_counter', 0) + 1
        return p2p_port(self.num_nodes + self._port_counter)

    def restart_with_mock(self, mock_tor, extra_args=None):
        mock_tor.start()
        self.restart_node(0, extra_args=[
            f"-torcontrol=127.0.0.1:{mock_tor.port}",
            "-listenonion=1",
            "-debug=tor",
        ] + (extra_args or []))

        # Wait for connection and PROTOCOLINFO command
        mock_tor.conn_ready.wait(timeout=10)
        self.wait_until(lambda: len(mock_tor.received_commands) >= 1, timeout=10)
        assert_equal(mock_tor.received_commands[0], "PROTOCOLINFO 1")

    @contextmanager
    def expect_disconnect(self, expect, mock_tor):
        initial_len = len(mock_tor.received_commands)
        yield

        if expect:
            # Expect to receive a PROTOCOLINFO 1 on reconnect, bumping the received
            # commands length.
            self.wait_until(lambda: len(mock_tor.received_commands) == initial_len + 1)
            assert_equal(mock_tor.received_commands[initial_len], "PROTOCOLINFO 1")
        else:
            # No disconnect, so no reconnect message
            ensure_for(duration=2, f=lambda: len(mock_tor.received_commands) == initial_len)

    def test_basic(self):
        self.log.info("Test Tor control basic functionality")

        mock_tor = MockTorControlServer(self.next_port())
        self.restart_with_mock(mock_tor)

        # Waiting for Tor control commands
        self.wait_until(lambda: len(mock_tor.received_commands) >= 4, timeout=10)

        # Verify expected protocol sequence
        assert_equal(mock_tor.received_commands[0], "PROTOCOLINFO 1")
        assert_equal(mock_tor.received_commands[1], "AUTHENTICATE")
        assert_equal(mock_tor.received_commands[2], "GETINFO net/listeners/socks")
        assert mock_tor.received_commands[3].startswith("ADD_ONION ")
        assert "PoWDefensesEnabled=1" in mock_tor.received_commands[3]

        # Clean up
        mock_tor.stop()

    def test_bind_any_onion_target_uses_loopback(self):
        self.log.info("Test Tor onion service maps bind-any target to loopback")

        mock_tor = MockTorControlServer(self.next_port())
        onion_bind_port = self.next_port()
        self.restart_with_mock(mock_tor, extra_args=[f"-bind=0.0.0.0:{onion_bind_port}=onion"])

        self.wait_until(lambda: any(command.startswith("ADD_ONION ") for command in mock_tor.received_commands), timeout=10)
        add_onion_command = next(command for command in mock_tor.received_commands if command.startswith("ADD_ONION "))
        assert f",127.0.0.1:{onion_bind_port}" in add_onion_command
        assert f",0.0.0.0:{onion_bind_port}" not in add_onion_command

        mock_tor.stop()

    def test_partial_data(self):
        self.log.info("Test that partial Tor control responses are buffered until complete")

        mock_tor = MockTorControlServer(self.next_port(), manual_mode=True)
        self.restart_with_mock(mock_tor)

        # Send partial response (no \r\n on last line)
        mock_tor.send_raw(
            "250-PROTOCOLINFO 1\r\n"
            "250-AUTH METHODS=NULL\r\n"
            "250 OK"
        )

        # Verify AUTHENTICATE is not sent
        ensure_for(duration=2, f=lambda: len(mock_tor.received_commands) == 1)

        # Complete the response
        mock_tor.send_raw("\r\n")

        # Should now process the complete response and send AUTHENTICATE
        self.wait_until(lambda: len(mock_tor.received_commands) >= 2, timeout=5)
        assert_equal(mock_tor.received_commands[1], "AUTHENTICATE")

        # Clean up
        mock_tor.stop()

    def test_pow_fallback(self):
        self.log.info("Test that ADD_ONION retries without PoW on 512 error")

        class NoPowServer(MockTorControlServer):
            def _get_response(self, command):
                if command.startswith("ADD_ONION"):
                    if "PoWDefensesEnabled=1" in command:
                        return "512 Unrecognized option\r\n"
                    else:
                        return (
                            "250-ServiceID=testserviceid1234567890123456789012345678901234567890123456\r\n"
                            "250 OK\r\n"
                        )
                return super()._get_response(command)

        mock_tor = NoPowServer(self.next_port())
        self.restart_with_mock(mock_tor)

        # Expect: PROTOCOLINFO, AUTHENTICATE, GETINFO, ADD_ONION (with PoW), ADD_ONION (without PoW)
        self.wait_until(lambda: len(mock_tor.received_commands) >= 5, timeout=10)

        # First ADD_ONION should have PoW enabled
        assert mock_tor.received_commands[3].startswith("ADD_ONION ")
        assert "PoWDefensesEnabled=1" in mock_tor.received_commands[3]

        # Retry should be ADD_ONION without PoW
        assert mock_tor.received_commands[4].startswith("ADD_ONION ")
        assert "PoWDefensesEnabled=1" not in mock_tor.received_commands[4]

        # Clean up
        mock_tor.stop()

    def test_oversized_line(self):
        mock_tor = MockTorControlServer(self.next_port(), manual_mode=True)
        self.restart_with_mock(mock_tor)

        MAX_LINE_LENGTH = 100000

        self.log.info("Test that Tor control does not disconnect with a MAX_LINE_LENGTH line.")
        with self.expect_disconnect(False, mock_tor):
            msg = "250-" + ("A" * (MAX_LINE_LENGTH - 5)) + "\r"
            assert_equal(len(msg), MAX_LINE_LENGTH)
            # The \n is not counted in line length.
            mock_tor.send_raw(msg + "\n")

        self.log.info("Test that Tor control disconnects with a MAX_LINE_LENGTH + 1 line")
        with self.expect_disconnect(True, mock_tor):
            msg = "250-" + ("A" * (MAX_LINE_LENGTH - 4)) + "\r"
            assert_equal(len(msg), MAX_LINE_LENGTH + 1)
            mock_tor.send_raw(msg + "\n")

        mock_tor.stop()

    def test_overmany_lines(self):
        mock_tor = MockTorControlServer(self.next_port(), manual_mode=True)
        self.restart_with_mock(mock_tor)

        MAX_LINE_COUNT = 1000

        self.log.info("Test that Tor control does not disconnect on receiving MAX_LINE_COUNT lines.")
        with self.expect_disconnect(False, mock_tor):
            for _ in range(MAX_LINE_COUNT - 1):
                mock_tor.send_raw("250-Continuing\r\n")
            mock_tor.send_raw("250 OK\r\n")

        self.log.info("Test that Tor control disconnects on receiving MAX_LINE_COUNT + 1 lines.")
        with self.expect_disconnect(True, mock_tor):
            for _ in range(MAX_LINE_COUNT + 1):
                mock_tor.send_raw("250-Continuing\r\n")

        mock_tor.stop()

    def test_tor_subprocess(self):
        self.log.info("Test launching a private Tor subprocess")

        fake_tor_script = Path(self.options.tmpdir) / "fake_tor.py"
        fake_tor_log = Path(self.options.tmpdir) / "fake_tor_commands.log"
        fake_tor_script.write_text("""#!/usr/bin/env python3
from pathlib import Path
import socket
import sys

def read_controlport_path(torrc_path):
    for line in Path(torrc_path).read_text().splitlines():
        if line.startswith("ControlPortWriteToFile "):
            return Path(line.split(" ", 1)[1])
    raise SystemExit("missing ControlPortWriteToFile")

if len(sys.argv) != 4 or sys.argv[1] != "-f":
    raise SystemExit("usage: fake_tor.py -f TORRC LOG")

controlport_file = read_controlport_path(sys.argv[2])
log_path = Path(sys.argv[3])
log_path.write_text("")

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("127.0.0.1", 0))
sock.listen(1)
controlport_file.parent.mkdir(parents=True, exist_ok=True)
controlport_file.write_text(f"PORT=127.0.0.1:{sock.getsockname()[1]}\\n")

owned = False
conn, _ = sock.accept()
with conn:
    buf = b""
    while True:
        data = conn.recv(1024)
        if not data:
            break
        buf += data
        while b"\\r\\n" in buf:
            line, buf = buf.split(b"\\r\\n", 1)
            command = line.decode("utf-8").strip()
            if not command:
                continue
            with log_path.open("a") as log:
                log.write(command + "\\n")
            if command == "PROTOCOLINFO 1":
                response = (
                    "250-PROTOCOLINFO 1\\r\\n"
                    "250-AUTH METHODS=NULL\\r\\n"
                    "250-VERSION Tor=\\\"fake\\\"\\r\\n"
                    "250 OK\\r\\n"
                )
            elif command in ("AUTHENTICATE", "TAKEOWNERSHIP"):
                owned = owned or command == "TAKEOWNERSHIP"
                response = "250 OK\\r\\n"
            elif command.startswith("ADD_ONION"):
                response = (
                    "250-ServiceID=testserviceid1234567890123456789012345678901234567890123456\\r\\n"
                    "250 OK\\r\\n"
                )
            elif command == "SIGNAL SHUTDOWN":
                conn.sendall(b"250 OK\\r\\n")
                raise SystemExit(0)
            else:
                response = "510 Unrecognized command\\r\\n"
            conn.sendall(response.encode("utf-8"))

if owned:
    raise SystemExit(0)
""")
        os.chmod(fake_tor_script, 0o755)

        self.restart_node(0, extra_args=[
            f"-torcontrol=127.0.0.1:{self.next_port()}",
            "-listenonion=1",
            "-debug=tor",
            f"-torexecute={sys.executable} {fake_tor_script} {fake_tor_log}",
        ])

        self.wait_until(lambda: fake_tor_log.exists() and "ADD_ONION" in fake_tor_log.read_text(), timeout=15)
        commands = fake_tor_log.read_text().splitlines()
        assert_equal(commands[0], "PROTOCOLINFO 1")
        assert "TAKEOWNERSHIP" in commands
        assert not any(command.startswith("GETINFO") for command in commands)

        self.stop_node(0)

    def run_test(self):
        self.test_basic()
        self.test_bind_any_onion_target_uses_loopback()
        self.test_partial_data()
        self.test_pow_fallback()
        self.test_oversized_line()
        self.test_overmany_lines()
        self.test_tor_subprocess()


if __name__ == '__main__':
    TorControlTest(__file__).main()
