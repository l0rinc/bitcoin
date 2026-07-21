// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <noui.h>

#include <node/interface_ui.h>
#include <util/btcsignals.h>
#include <util/log.h>
#include <util/translation.h>

#include <string>

/** Store connections so we can disconnect them when suppressing output */
btcsignals::connection noui_ThreadSafeMessageBoxConn;
btcsignals::connection noui_ThreadSafeQuestionConn;
btcsignals::connection noui_InitMessageConn;

void noui_ThreadSafeMessageBox(const bilingual_str& message, unsigned int style)
{
    bool fSecure = style & CClientUIInterface::SECURE;
    style &= ~CClientUIInterface::SECURE;

    std::string strCaption;
    const unsigned int icon{style & CClientUIInterface::ICON_MASK};
    if (icon == CClientUIInterface::ICON_ERROR) {
        strCaption = "Error: ";
        if (!fSecure) LogError("%s\n", message.original);
    } else if (icon == CClientUIInterface::ICON_WARNING) {
        strCaption = "Warning: ";
        if (!fSecure) LogWarning("%s\n", message.original);
    } else if (style == CClientUIInterface::MSG_INFORMATION) {
        strCaption = "Information: ";
        if (!fSecure) LogInfo("%s\n", message.original);
    } else {
        if (!fSecure) LogInfo("%s%s\n", strCaption, message.original);
    }

    tfm::format(std::cerr, "%s%s\n", strCaption, message.original);
}

bool noui_ThreadSafeQuestion(const bilingual_str& /* ignored interactive message */, const std::string& message, unsigned int style)
{
    noui_ThreadSafeMessageBox(Untranslated(message), style);
    return false; // Answer the question with false in the noui context
}

void noui_InitMessage(const std::string& message)
{
    LogInfo("init message: %s", message);
}

void noui_connect()
{
    noui_ThreadSafeMessageBoxConn = uiInterface.ThreadSafeMessageBox.connect(noui_ThreadSafeMessageBox);
    noui_ThreadSafeQuestionConn = uiInterface.ThreadSafeQuestion.connect(noui_ThreadSafeQuestion);
    noui_InitMessageConn = uiInterface.InitMessage.connect(noui_InitMessage);
}

void noui_ThreadSafeMessageBoxRedirect(const bilingual_str& message, unsigned int style)
{
    LogInfo("%s", message.original);
}

bool noui_ThreadSafeQuestionRedirect(const bilingual_str& /* ignored interactive message */, const std::string& message, unsigned int style)
{
    LogInfo("%s", message);
    return false;
}

void noui_InitMessageRedirect(const std::string& message)
{
    LogInfo("init message: %s", message);
}

void noui_test_redirect()
{
    noui_ThreadSafeMessageBoxConn.disconnect();
    noui_ThreadSafeQuestionConn.disconnect();
    noui_InitMessageConn.disconnect();
    noui_ThreadSafeMessageBoxConn = uiInterface.ThreadSafeMessageBox.connect(noui_ThreadSafeMessageBoxRedirect);
    noui_ThreadSafeQuestionConn = uiInterface.ThreadSafeQuestion.connect(noui_ThreadSafeQuestionRedirect);
    noui_InitMessageConn = uiInterface.InitMessage.connect(noui_InitMessageRedirect);
}

void noui_reconnect()
{
    noui_ThreadSafeMessageBoxConn.disconnect();
    noui_ThreadSafeQuestionConn.disconnect();
    noui_InitMessageConn.disconnect();
    noui_connect();
}
