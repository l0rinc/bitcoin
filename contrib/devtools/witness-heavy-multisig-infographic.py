#!/usr/bin/env python3
"""Generate an explanatory SVG for the P2WSH multisig block demo."""

from argparse import ArgumentParser
from pathlib import Path
from textwrap import wrap
import sys
from xml.sax.saxutils import escape


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "test" / "functional"))

from test_framework.key import ECKey  # noqa: E402
from test_framework.messages import MAX_BLOCK_WEIGHT  # noqa: E402
from test_framework.script_util import keys_to_multisig_script, script_to_p2wsh_script  # noqa: E402


RUNS = [
    {
        "name": "small 2-of-2",
        "k": 2,
        "n": 2,
        "spends": 6_698,
        "serialized": 2_110_106,
        "stripped": 629_839,
        "weight": 3_999_623,
    },
    {
        "name": "small 3-of-5",
        "k": 3,
        "n": 5,
        "spends": 5_183,
        "serialized": 2_537_320,
        "stripped": 487_429,
        "weight": 3_999_607,
    },
    {
        "name": "custody 20-of-20",
        "k": 20,
        "n": 20,
        "spends": 1_588,
        "serialized": 3_550_994,
        "stripped": 149_499,
        "weight": 3_999_491,
    },
]


P2WSH_CURVE = [
    {"n": 1, "script": 37, "spends": 8_161, "serialized": 1_697_751, "stripped": 767_361, "weight": 3_999_834},
    {"n": 2, "script": 71, "spends": 6_698, "serialized": 2_110_133, "stripped": 629_839, "weight": 3_999_650},
    {"n": 3, "script": 105, "spends": 5_696, "serialized": 2_392_583, "stripped": 535_651, "weight": 3_999_536},
    {"n": 4, "script": 139, "spends": 4_931, "serialized": 2_608_762, "stripped": 463_741, "weight": 3_999_985},
    {"n": 5, "script": 173, "spends": 4_365, "serialized": 2_767_673, "stripped": 410_537, "weight": 3_999_284},
    {"n": 6, "script": 207, "spends": 3_909, "serialized": 2_896_832, "stripped": 367_673, "weight": 3_999_851},
    {"n": 7, "script": 241, "spends": 3_548, "serialized": 2_998_323, "stripped": 333_739, "weight": 3_999_540},
    {"n": 8, "script": 275, "spends": 3_232, "serialized": 3_086_823, "stripped": 304_035, "weight": 3_998_928},
    {"n": 9, "script": 309, "spends": 2_975, "serialized": 3_159_713, "stripped": 279_877, "weight": 3_999_344},
    {"n": 10, "script": 343, "spends": 2_752, "serialized": 3_222_855, "stripped": 258_915, "weight": 3_999_600},
    {"n": 11, "script": 377, "spends": 2_565, "serialized": 3_275_768, "stripped": 241_337, "weight": 3_999_779},
    {"n": 12, "script": 411, "spends": 2_403, "serialized": 3_321_209, "stripped": 226_109, "weight": 3_999_536},
    {"n": 13, "script": 445, "spends": 2_260, "serialized": 3_360_883, "stripped": 212_667, "weight": 3_998_884},
    {"n": 14, "script": 479, "spends": 2_127, "serialized": 3_399_209, "stripped": 200_165, "weight": 3_999_704},
    {"n": 15, "script": 513, "spends": 2_014, "serialized": 3_430_105, "stripped": 189_543, "weight": 3_998_734},
    {"n": 16, "script": 547, "spends": 1_910, "serialized": 3_459_273, "stripped": 179_767, "weight": 3_998_574},
    {"n": 17, "script": 583, "spends": 1_822, "serialized": 3_483_927, "stripped": 171_495, "weight": 3_998_412},
    {"n": 18, "script": 617, "spends": 1_734, "serialized": 3_508_145, "stripped": 163_223, "weight": 3_997_814},
    {"n": 19, "script": 651, "spends": 1_655, "serialized": 3_530_378, "stripped": 155_797, "weight": 3_997_769},
    {"n": 20, "script": 685, "spends": 1_587, "serialized": 3_550_382, "stripped": 149_405, "weight": 3_998_597},
]


COLORS = {
    "ink": "#17222c",
    "muted": "#5f6b77",
    "line": "#d9e0e7",
    "panel": "#f7f9fb",
    "base": "#2f6f7e",
    "base_light": "#d8eef2",
    "witness": "#db7c26",
    "witness_light": "#f9dfc8",
    "accent": "#6c4ab6",
    "ok": "#23845f",
}


def make_keys(key_count):
    keys = []
    for private_key_number in range(1, key_count + 1):
        key = ECKey()
        key.set(secret=private_key_number.to_bytes(length=32, byteorder="big"), compressed=True)
        keys.append(key)
    return keys


def build_case(run):
    keys = make_keys(run["n"])
    pubkeys = [key.get_pubkey().get_bytes() for key in keys]
    script = keys_to_multisig_script(pubkeys, k=run["k"])
    return pubkeys, script, script_to_p2wsh_script(script)


def multiline_text(x, y, value, size=20, color=None, width=80, line_height=None, weight="400"):
    color = color or COLORS["ink"]
    line_height = line_height or int(size * 1.35)
    lines = []
    for paragraph in value.split("\n"):
        lines.extend(wrap(paragraph, width=width) or [""])
    out = []
    for index, line in enumerate(lines):
        out.append(
            f'<text x="{x}" y="{y + index * line_height}" font-size="{size}" '
            f'font-weight="{weight}" fill="{color}">{escape(line)}</text>'
        )
    return "\n".join(out), y + len(lines) * line_height


def rect(x, y, w, h, fill, stroke=None, rx=8, opacity=None):
    stroke_attr = f' stroke="{stroke}"' if stroke else ""
    opacity_attr = f' opacity="{opacity}"' if opacity is not None else ""
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}"{stroke_attr}{opacity_attr}/>'


def text(x, y, value, size=20, fill=None, weight="400", anchor="start"):
    fill = fill or COLORS["ink"]
    return (
        f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{weight}" '
        f'text-anchor="{anchor}" fill="{fill}">{escape(str(value))}</text>'
    )


def line(x1, y1, x2, y2, stroke=None, width=2):
    stroke = stroke or COLORS["line"]
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" stroke-width="{width}"/>'


def bytes_label(n):
    return f"{n:,} B"


def make_serialized_chart(x, y, w):
    out = [text(x, y, "Mined block bytes: stripped/base vs witness", 28, weight="700")]
    out.append(text(x, y + 34, "All rows are accepted blocks built from standard mempool P2WSH multisig spends.", 17, COLORS["muted"]))
    bar_h = 45
    scale = max(run["serialized"] for run in RUNS)
    y += 76
    for run in RUNS:
        witness_bytes = run["serialized"] - run["stripped"]
        total_w = int(w * run["serialized"] / scale)
        base_w = max(6, int(w * run["stripped"] / scale))
        witness_w = max(0, total_w - base_w)
        ratio = run["serialized"] / run["stripped"]
        out.append(text(x, y - 12, f"{run['name']}: {run['spends']:,} spends, {run['serialized']:,} serialized bytes", 18, weight="700"))
        out.append(rect(x, y, w, bar_h, "#ecf0f4", rx=7))
        out.append(rect(x, y, base_w, bar_h, COLORS["base"], rx=7))
        out.append(rect(x + base_w, y, witness_w, bar_h, COLORS["witness"], rx=0))
        out.append(text(x + min(total_w + 15, w - 8), y + 29, f"{ratio:.2f}x", 19, COLORS["accent"], "800", anchor="end" if total_w > w - 110 else "start"))
        out.append(text(x, y + 69, f"stripped/base: {bytes_label(run['stripped'])}", 16, COLORS["base"], "700"))
        out.append(text(x + 250, y + 69, f"witness: {bytes_label(witness_bytes)}", 16, COLORS["witness"], "700"))
        out.append(text(x + 505, y + 69, f"weight: {run['weight']:,}/{MAX_BLOCK_WEIGHT:,}", 16, COLORS["muted"], "700"))
        y += 118
    return "\n".join(out), y


def make_transaction_panel(x, y, w):
    out = [text(x, y, "One example spend transaction", 28, weight="700")]
    y += 45
    out.append(rect(x, y, w, 112, COLORS["base_light"], stroke="#a7cbd3", rx=7))
    out.append(text(x + 18, y + 30, "Stripped/base serialization", 20, COLORS["base"], "800"))
    base_text = "version, input outpoint, sequence, one standard recipient output, locktime"
    block, _ = multiline_text(x + 18, y + 64, base_text, size=18, color=COLORS["ink"], width=54)
    out.append(block)
    y += 134
    out.append(rect(x, y, w, 145, COLORS["witness_light"], stroke="#edbd8e", rx=7))
    out.append(text(x + 18, y + 30, "Witness serialization", 20, COLORS["witness"], "800"))
    witness_text = "empty CHECKMULTISIG dummy item, k ECDSA signatures, and the revealed witnessScript: k <n compressed pubkeys> n OP_CHECKMULTISIG"
    block, _ = multiline_text(x + 18, y + 64, witness_text, size=18, color=COLORS["ink"], width=54)
    out.append(block)
    y += 170
    commit_text = "Previous P2WSH deposit: 34-byte OP_0 HASH256(witnessScript). Spend reveals signatures plus script in witness."
    block, y = multiline_text(x, y - 10, commit_text, size=15, color=COLORS["muted"], width=74, line_height=20)
    out.append(block)
    return "\n".join(out), y


def make_case_table(x, y, w):
    out = [text(x, y, "What changes as multisig gets larger", 28, weight="700")]
    y += 48
    headers = ["case", "script", "stack", "result"]
    xs = [x, x + 190, x + 360, x + 540]
    for col_x, header in zip(xs, headers):
        out.append(text(col_x, y, header, 16, COLORS["muted"], "800"))
    y += 22
    out.append(line(x, y, x + w, y))
    y += 34
    for run in RUNS:
        _, script, _ = build_case(run)
        ratio = run["serialized"] / run["stripped"]
        out.append(text(xs[0], y, run["name"], 17, weight="700"))
        out.append(text(xs[1], y, f"{len(script):,} B", 17))
        out.append(text(xs[2], y, f"{run['k']} sigs + script", 17))
        out.append(text(xs[3], y, f"{run['serialized'] / 1_000_000:.2f} MB, {ratio:.2f}x", 17))
        y += 39
    y += 14
    block, y = multiline_text(
        x,
        y,
        "The small cases are the realistic baseline. The 20-of-20 case is the upper end of standard P2WSH multisig: still one normal CHECKMULTISIG spend per transaction, but with much more authorization data in witness.",
        size=18,
        color=COLORS["muted"],
        width=78,
    )
    out.append(block)
    return "\n".join(out), y


def make_validity_panel(x, y, w):
    out = [text(x, y, "Why the block is valid", 28, weight="700")]
    y += 46
    points = [
        "Each spend was accepted by testmempoolaccept/sendrawtransaction before mining.",
        "generateblock submit=False found the exact boundary: N spends fit, N + 1 fails block weight.",
        "The final block was submitted and became the active tip.",
        "The scripts are ordinary P2WSH multisig: signatures plus the revealed witnessScript.",
    ]
    for point in points:
        out.append(rect(x, y - 21, 26, 26, COLORS["ok"], rx=13))
        out.append(text(x + 13, y, "ok", 11, "#ffffff", "700", anchor="middle"))
        block, y = multiline_text(x + 42, y, point, size=18, width=66)
        out.append(block)
        y += 18
    return "\n".join(out), y


def polyline(points, stroke, width=4):
    return (
        f'<polyline points="{" ".join(f"{x},{y}" for x, y in points)}" '
        f'fill="none" stroke="{stroke}" stroke-width="{width}" stroke-linejoin="round" stroke-linecap="round"/>'
    )


def circle(x, y, r, fill, stroke=None):
    stroke_attr = f' stroke="{stroke}" stroke-width="2"' if stroke else ""
    return f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}"{stroke_attr}/>'


def make_curve_panel(x, y, w, h):
    out = [text(x, y, "P2WSH-only full-block curve", 28, weight="700")]
    out.append(text(x, y + 34, "Single normal k-of-k CHECKMULTISIG spend shape, SegWit v0/P2WSH only, no padding, no repeated checks.", 17, COLORS["muted"]))
    plot_x = x + 70
    plot_y = y + 88
    plot_w = w - 150
    plot_h = h - 230
    min_size = 1_500_000
    max_size = 3_700_000
    x_for = lambda n: plot_x + (n - 1) * plot_w / 19
    y_for = lambda size: plot_y + plot_h - (size - min_size) * plot_h / (max_size - min_size)
    out.append(line(plot_x, plot_y + plot_h, plot_x + plot_w, plot_y + plot_h, COLORS["line"], 2))
    out.append(line(plot_x, plot_y, plot_x, plot_y + plot_h, COLORS["line"], 2))
    for mb in [2_000_000, 3_000_000, 3_550_000]:
        gy = y_for(mb)
        out.append(line(plot_x, gy, plot_x + plot_w, gy, "#e8edf2", 1))
        out.append(text(plot_x - 12, gy + 5, f"{mb / 1_000_000:.2f} MB", 13, COLORS["muted"], "700", anchor="end"))
    for n in [1, 5, 10, 15, 20]:
        gx = x_for(n)
        out.append(line(gx, plot_y + plot_h, gx, plot_y + plot_h + 7, COLORS["line"], 2))
        out.append(text(gx, plot_y + plot_h + 27, str(n), 14, COLORS["muted"], "700", anchor="middle"))
    points = [(round(x_for(row["n"]), 1), round(y_for(row["serialized"]), 1)) for row in P2WSH_CURVE]
    out.append(polyline(points, COLORS["accent"], 5))
    for row in P2WSH_CURVE:
        out.append(circle(x_for(row["n"]), y_for(row["serialized"]), 4, "#ffffff", COLORS["accent"]))
    for row in [P2WSH_CURVE[0], P2WSH_CURVE[1], P2WSH_CURVE[4], P2WSH_CURVE[9], P2WSH_CURVE[-1]]:
        px = x_for(row["n"])
        py = y_for(row["serialized"])
        anchor = "end" if row["n"] > 15 else "start"
        lx = px - 10 if anchor == "end" else px + 10
        label_y = py + 24 if row["n"] == 1 else py - 9
        out.append(text(lx, label_y, f"{row['n']}-of-{row['n']}: {row['serialized'] / 1_000_000:.2f} MB", 14, COLORS["ink"], "800", anchor=anchor))
    out.append(text(plot_x + plot_w / 2, plot_y + plot_h + 56, "number of pubkeys/signatures in one P2WSH CHECKMULTISIG spend", 16, COLORS["muted"], "700", anchor="middle"))
    out.append(text(plot_x - 55, plot_y - 16, "serialized block size at full weight", 16, COLORS["muted"], "700"))
    summary = "The curve tops out at 20 because P2WSH OP_CHECKMULTISIG is consensus-capped at 20 pubkeys. At that cap, standard SegWit-only spends fill the 4M-weight block and serialize to about 3.55 MB."
    block, _ = multiline_text(x, plot_y + plot_h + 92, summary, size=18, color=COLORS["ink"], width=126)
    out.append(block)
    return "\n".join(out)


def make_infographic(svg_path, script_dump_path):
    width = 1600
    height = 2200
    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        "<style>text{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif}</style>",
        rect(0, 0, width, height, "#ffffff", rx=0),
        text(70, 92, "Standard multisig spends can make witness-heavy blocks", 42, weight="800"),
        text(70, 130, "The demo fills blocks with independent monetary P2WSH spends that first pass mempool policy.", 22, COLORS["muted"]),
        rect(70, 165, 1460, 470, COLORS["panel"], stroke=COLORS["line"], rx=14),
        rect(70, 665, 710, 430, COLORS["panel"], stroke=COLORS["line"], rx=14),
        rect(820, 665, 710, 430, COLORS["panel"], stroke=COLORS["line"], rx=14),
        rect(70, 1125, 710, 360, COLORS["panel"], stroke=COLORS["line"], rx=14),
        rect(820, 1125, 710, 360, COLORS["panel"], stroke=COLORS["line"], rx=14),
        rect(70, 1515, 1460, 430, COLORS["panel"], stroke=COLORS["line"], rx=14),
        rect(70, 1980, 1460, 120, "#fff8f1", stroke="#f1cda9", rx=14),
    ]

    chart, _ = make_serialized_chart(105, 220, 1070)
    out.append(chart)
    out.append(line(1210, 210, 1210, 600))
    out.append(text(1240, 250, "Boundary proof", 27, weight="800"))
    out.append(text(1240, 296, "Each case mines N txs", 21, COLORS["ink"], "700"))
    out.append(text(1240, 336, "N + 1 rejected", 21, COLORS["ink"], "700"))
    out.append(text(1240, 376, "reason: bad-blk-weight", 21, COLORS["accent"], "800"))
    out.append(text(1240, 430, "The block is full because", 20, COLORS["muted"], "700"))
    out.append(text(1240, 462, "consensus validation says", 20, COLORS["muted"], "700"))
    out.append(text(1240, 494, "the next monetary spend", 20, COLORS["muted"], "700"))
    out.append(text(1240, 526, "would exceed 4M weight.", 20, COLORS["muted"], "700"))

    transaction_panel, _ = make_transaction_panel(105, 725, 630)
    out.append(transaction_panel)
    table, _ = make_case_table(855, 725, 625)
    out.append(table)
    validity, _ = make_validity_panel(105, 1190, 630)
    out.append(validity)
    explanation = (
        "This does not require inscriptions or arbitrary data. It only requires a future where many users or custodians spend "
        "P2WSH multisig UTXOs at the same time. Small multisig fills weight with a moderate serialized size; larger standard "
        "multisig pushes much more of the block into witness."
    )
    out.append(text(855, 1190, "What this demonstrates", 28, weight="700"))
    block, _ = multiline_text(855, 1235, explanation, size=21, color=COLORS["ink"], width=58, line_height=31)
    out.append(block)
    out.append(text(855, 1440, "Exact script hex is written to:", 22, COLORS["muted"], "700"))
    out.append(text(855, 1473, script_dump_path.name, 23, COLORS["accent"], "800"))

    out.append(make_curve_panel(105, 1570, 1390, 330))
    bottom, _ = multiline_text(105, 2030, "Generated from the passing functional test run: mined cases used standard mempool P2WSH spends; the curve is the same P2WSH spend-size simulation printed by the test.", 19, COLORS["ink"], 140, line_height=27, weight="700")
    out.append(bottom)
    out.append("</svg>")
    svg_path.write_text("\n".join(out), encoding="utf8")
    write_script_dump(script_dump_path)


def script_asm(run, pubkeys):
    lines = [f"{run['k']}"]
    lines.extend(f"PUSHDATA33 pubkey_{i:02d} {pubkey.hex()}" for i, pubkey in enumerate(pubkeys, start=1))
    lines.append(str(run["n"]))
    lines.append("OP_CHECKMULTISIG")
    return "\n".join(lines)


def wrap_hex(hex_string, width=96):
    return "\n".join(wrap(hex_string, width=width))


def write_script_dump(path):
    lines = [
        "Mempool-standard P2WSH multisig script dump",
        "",
        "Generated from contrib/devtools/witness-heavy-multisig-infographic.py.",
        "The functional test is test_witness_heavy_multisig_blocks() in test/functional/mining_basic.py.",
        "",
        "P2WSH CHECKMULTISIG full-block curve from the passing test run:",
        "  columns: multisig, witnessScript bytes, spends that fit, serialized bytes, stripped bytes, ratio, weight",
    ]
    for row in P2WSH_CURVE:
        ratio = row["serialized"] / row["stripped"]
        lines.append(
            f"  {row['n']:02d}-of-{row['n']:02d}: script {row['script']:>4,} B, "
            f"{row['spends']:>5,} spends, serialized {row['serialized']:>9,} B, "
            f"stripped {row['stripped']:>8,} B, {ratio:>5.2f}x, weight {row['weight']:>9,}"
        )
    lines.extend(
        [
            "",
            "P2WSH OP_CHECKMULTISIG is capped at 20 public keys by MAX_PUBKEYS_PER_MULTISIG.",
            "This file is intentionally SegWit v0/P2WSH-only.",
            "",
        ]
    )
    lines.extend(
        [
        "Every measured spend has this shape:",
        "  stripped/base tx data: one input spending a previous P2WSH deposit, one standard recipient output",
        "  witness data: empty CHECKMULTISIG dummy item, k ECDSA signatures, witnessScript",
        "  previous deposit scriptPubKey: OP_0 HASH256(witnessScript), serialized as 34 bytes",
        "",
        ]
    )
    for run in RUNS:
        pubkeys, script, script_pubkey = build_case(run)
        ratio = run["serialized"] / run["stripped"]
        lines.extend(
            [
                f"Case: {run['name']}",
                f"  script: {run['k']}-of-{run['n']} P2WSH multisig",
                f"  witnessScript size: {len(script):,} bytes",
                f"  P2WSH scriptPubKey hex: {script_pubkey.hex()}",
                f"  mempool block spends: {run['spends']:,}",
                f"  accepted serialized size: {run['serialized']:,} bytes",
                f"  stripped/base size: {run['stripped']:,} bytes",
                f"  block weight: {run['weight']:,} / {MAX_BLOCK_WEIGHT:,}",
                f"  serialized/stripped ratio: {ratio:.2f}x",
                "  boundary check: adding one more standard spend failed block validation with bad-blk-weight",
                "",
                "  compressed public keys:",
            ]
        )
        lines.extend(f"    pubkey_{i:02d}: {pubkey.hex()}" for i, pubkey in enumerate(pubkeys, start=1))
        lines.extend(
            [
                "",
                "  witnessScript ASM:",
                "\n".join(f"    {line}" for line in script_asm(run, pubkeys).splitlines()),
                "",
                "  witnessScript hex:",
                "\n".join(f"    {line}" for line in wrap_hex(script.hex()).splitlines()),
                "",
            ]
        )
    path.write_text("\n".join(lines), encoding="utf8")


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--svg", type=Path, default=ROOT / "witness_heavy_multisig_infographic.svg")
    parser.add_argument("--script-dump", type=Path, default=ROOT / "witness_heavy_multisig_script.txt")
    args = parser.parse_args()
    make_infographic(args.svg, args.script_dump)
    print(f"Wrote {args.svg}")
    print(f"Wrote {args.script_dump}")


if __name__ == "__main__":
    main()
