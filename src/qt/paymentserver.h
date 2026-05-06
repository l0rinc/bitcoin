// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PAYMENTSERVER_H
#define BITCOIN_QT_PAYMENTSERVER_H

// This class handles payment requests from bitcoin: URIs
// and related file-open events.
//
// This is somewhat tricky, because we have to deal with
// the situation where the user clicks on a link during
// startup/initialization, when the splash-screen is up
// but the main window (and the Send Coins tab) is not.
//
// So, the strategy is:
//
// Create the server, and register the event handler,
// when the application is created. Save any URI or file
// inputs received at or during startup in a set.
//
// When startup is finished and the main window is
// shown, a signal is sent to slot uiReady(), which
// processes any queued startup inputs.
//
// After startup, incoming URI and file inputs are handled immediately.
//
// This class has one more feature: a static
// method that finds URI or file inputs passed on the
// command line and, if another process is already
// running, sends them there.
//

#include <qt/sendcoinsrecipient.h>

#include <QObject>
#include <QString>

class OptionsModel;

namespace interfaces {
class Node;
} // namespace interfaces

QT_BEGIN_NAMESPACE
class QApplication;
class QByteArray;
class QLocalServer;
class QUrl;
QT_END_NAMESPACE

extern const QString BITCOIN_IPC_PREFIX;

class PaymentServer : public QObject
{
    Q_OBJECT

public:
    // Parse and queue URI or file inputs from the command line.
    static void ipcParseCommandLine(int argc, char *argv[]);

    // Return true if queued URI or file inputs were successfully sent to an
    // already-running process.
    static bool ipcSendCommandLine();

    // parent should be QApplication object
    explicit PaymentServer(QObject* parent, bool startLocalServer = true);
    ~PaymentServer();

    // Store the GUI options model.
    void setOptionsModel(OptionsModel *optionsModel);

Q_SIGNALS:
    // Fired when a valid payment request is received
    void receivedPaymentRequest(SendCoinsRecipient);

    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

public Q_SLOTS:
    // Signal this when the main window's UI is ready
    // to display payment requests to the user
    void uiReady();

    // Handle an incoming URI or local file path.
    void handleURIOrFile(const QString& s);

private Q_SLOTS:
    void handleURIConnection();

protected:
    // Constructor registers this on the parent QApplication to
    // receive QEvent::FileOpen events.
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    bool saveURIs{true}; // true during startup
    QLocalServer* uriServer{nullptr};
    OptionsModel* optionsModel{nullptr};
};

#endif // BITCOIN_QT_PAYMENTSERVER_H
