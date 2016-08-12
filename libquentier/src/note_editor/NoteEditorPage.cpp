/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteEditorPage.h"
#include "JavaScriptInOrderExecutor.h"
#include "NoteEditor_p.h"
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/logging/QuentierLogger.h>
#include <QMessageBox>
#include <QApplication>

namespace quentier {

NoteEditorPage::NoteEditorPage(NoteEditorPrivate & parent) :
    WebPage(&parent),
    m_parent(&parent),
    m_pJavaScriptInOrderExecutor(new JavaScriptInOrderExecutor(parent, this)),
    m_javaScriptAutoExecution(true)
{
    QUENTIER_CHECK_PTR(m_parent);
    QObject::connect(this, QNSIGNAL(NoteEditorPage,noteLoadCancelled),
                     &parent, QNSLOT(NoteEditorPrivate,onNoteLoadCancelled));
    QObject::connect(m_pJavaScriptInOrderExecutor, QNSIGNAL(JavaScriptInOrderExecutor,finished),
                     this, QNSLOT(NoteEditorPage,onJavaScriptQueueEmpty));
}

NoteEditorPage::~NoteEditorPage()
{
    QNDEBUG("NoteEditorPage::~NoteEditorPage");
}

bool NoteEditorPage::javaScriptQueueEmpty() const
{
    QNDEBUG("NoteEditorPage::javaScriptQueueEmpty: "
            << (m_pJavaScriptInOrderExecutor->empty() ? "true" : "false"));
    return m_pJavaScriptInOrderExecutor->empty();
}

void NoteEditorPage::setInactive()
{
    QNDEBUG("NoteEditorPage::setInactive");

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    NoteEditorPluginFactory * pPluginFactory = qobject_cast<NoteEditorPluginFactory*>(pluginFactory());
    if (Q_LIKELY(pPluginFactory)) {
        pPluginFactory->setInactive();
    }
#endif
}

void NoteEditorPage::setActive()
{
    QNDEBUG("NoteEditorPage::setActive");

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    NoteEditorPluginFactory * pPluginFactory = qobject_cast<NoteEditorPluginFactory*>(pluginFactory());
    if (Q_LIKELY(pPluginFactory)) {
        pPluginFactory->setActive();
    }
#endif
}

void NoteEditorPage::stopJavaScriptAutoExecution()
{
    QNDEBUG("NoteEditorPage::stopJavaScriptAutoExecution");
    m_javaScriptAutoExecution = false;
}

void NoteEditorPage::startJavaScriptAutoExecution()
{
    QNDEBUG("NoteEditorPage::startJavaScriptAutoExecution");
    m_javaScriptAutoExecution = true;
    if (!m_pJavaScriptInOrderExecutor->inProgress()) {
        m_pJavaScriptInOrderExecutor->start();
    }
}

bool NoteEditorPage::shouldInterruptJavaScript()
{
    QNDEBUG("NoteEditorPage::shouldInterruptJavaScript");

    QString title = tr("Note editor hanged");
    QString question = tr("Note editor seems hanged when loading or editing the note. "
                          "Would you like to cancel loading the note?");
    QMessageBox::StandardButton reply = QMessageBox::question(m_parent, title, question,
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QNINFO("Note load was cancelled due to too long javascript evaluation");
        emit noteLoadCancelled();
        return true;
    }
    else {
        QNINFO("Note load seems to take a lot of time but user wished to wait more");
        return false;
    }
}

void NoteEditorPage::executeJavaScript(const QString & script, Callback callback, const bool clearPreviousQueue)
{
    if (Q_UNLIKELY(clearPreviousQueue)) {
        m_pJavaScriptInOrderExecutor->clear();
    }

    m_pJavaScriptInOrderExecutor->append(script, callback);

    if (m_javaScriptAutoExecution && !m_pJavaScriptInOrderExecutor->inProgress()) {
        m_pJavaScriptInOrderExecutor->start();
    }
}

void NoteEditorPage::onJavaScriptQueueEmpty()
{
    QNDEBUG("NoteEditorPage::onJavaScriptQueueEmpty");
    emit javaScriptLoaded();
}

#ifndef QUENTIER_USE_QT_WEB_ENGINE
void NoteEditorPage::javaScriptAlert(QWebFrame * pFrame, const QString & message)
{
    QNDEBUG("NoteEditorPage::javaScriptAlert, message: " << message);
    QWebPage::javaScriptAlert(pFrame, message);
}

bool NoteEditorPage::javaScriptConfirm(QWebFrame * pFrame, const QString & message)
{
    QNDEBUG("NoteEditorPage::javaScriptConfirm, message: " << message);
    return QWebPage::javaScriptConfirm(pFrame, message);
}

void NoteEditorPage::javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID)
{
    QNDEBUG("NoteEditorPage::javaScriptConsoleMessage, message: " << message << ", line number: " << lineNumber
            << ", sourceID = " << sourceID);
    QWebPage::javaScriptConsoleMessage(message, lineNumber, sourceID);
}
#else
void NoteEditorPage::javaScriptAlert(const QUrl & securityOrigin, const QString & msg)
{
    QNDEBUG("NoteEditorPage::javaScriptAlert, message: " << msg);
    QWebEnginePage::javaScriptAlert(securityOrigin, msg);
}

bool NoteEditorPage::javaScriptConfirm(const QUrl & securityOrigin, const QString & msg)
{
    QNDEBUG("NoteEditorPage::javaScriptConfirm, message: " << msg);
    return QWebEnginePage::javaScriptConfirm(securityOrigin, msg);
}

void NoteEditorPage::javaScriptConsoleMessage(QWebEnginePage::JavaScriptConsoleMessageLevel level,
                                              const QString & message, int lineNumber, const QString & sourceID)
{
    QNDEBUG("NoteEditorPage::javaScriptConsoleMessage, message: " << message << ", level = " << level
            << ", line number: " << lineNumber << ", sourceID = " << sourceID);
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
}
#endif

} // namespace quentier
