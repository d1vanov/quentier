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
    QNDEBUG(QStringLiteral("NoteEditorPage::~NoteEditorPage"));
}

bool NoteEditorPage::javaScriptQueueEmpty() const
{
    QNDEBUG(QStringLiteral("NoteEditorPage::javaScriptQueueEmpty: ")
            << (m_pJavaScriptInOrderExecutor->empty() ? QStringLiteral("true") : QStringLiteral("false")));
    return m_pJavaScriptInOrderExecutor->empty();
}

void NoteEditorPage::setInactive()
{
    QNDEBUG(QStringLiteral("NoteEditorPage::setInactive"));

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    NoteEditorPluginFactory * pPluginFactory = qobject_cast<NoteEditorPluginFactory*>(pluginFactory());
    if (Q_LIKELY(pPluginFactory)) {
        pPluginFactory->setInactive();
    }
#endif
}

void NoteEditorPage::setActive()
{
    QNDEBUG(QStringLiteral("NoteEditorPage::setActive"));

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    NoteEditorPluginFactory * pPluginFactory = qobject_cast<NoteEditorPluginFactory*>(pluginFactory());
    if (Q_LIKELY(pPluginFactory)) {
        pPluginFactory->setActive();
    }
#endif
}

void NoteEditorPage::stopJavaScriptAutoExecution()
{
    QNDEBUG(QStringLiteral("NoteEditorPage::stopJavaScriptAutoExecution"));
    m_javaScriptAutoExecution = false;
}

void NoteEditorPage::startJavaScriptAutoExecution()
{
    QNDEBUG(QStringLiteral("NoteEditorPage::startJavaScriptAutoExecution"));
    m_javaScriptAutoExecution = true;
    if (!m_pJavaScriptInOrderExecutor->inProgress()) {
        m_pJavaScriptInOrderExecutor->start();
    }
}

bool NoteEditorPage::shouldInterruptJavaScript()
{
    QNDEBUG(QStringLiteral("NoteEditorPage::shouldInterruptJavaScript"));

    QString title = tr("Note editor hanged");
    QString question = tr("Note editor seems hanged when loading or editing the note. "
                          "Would you like to cancel loading the note?");
    QMessageBox::StandardButton reply = QMessageBox::question(m_parent, title, question,
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QNINFO(QStringLiteral("Note load was cancelled due to too long javascript evaluation"));
        emit noteLoadCancelled();
        return true;
    }
    else {
        QNINFO(QStringLiteral("Note load seems to take a lot of time but user wished to wait more"));
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
    QNDEBUG(QStringLiteral("NoteEditorPage::onJavaScriptQueueEmpty"));
    emit javaScriptLoaded();
}

#ifndef QUENTIER_USE_QT_WEB_ENGINE
void NoteEditorPage::javaScriptAlert(QWebFrame * pFrame, const QString & message)
{
    QNDEBUG(QStringLiteral("NoteEditorPage::javaScriptAlert, message: ") << message);
    QWebPage::javaScriptAlert(pFrame, message);
}

bool NoteEditorPage::javaScriptConfirm(QWebFrame * pFrame, const QString & message)
{
    QNDEBUG(QStringLiteral("NoteEditorPage::javaScriptConfirm, message: ") << message);
    return QWebPage::javaScriptConfirm(pFrame, message);
}

void NoteEditorPage::javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID)
{
    QNDEBUG(QStringLiteral("NoteEditorPage::javaScriptConsoleMessage, message: ") << message << QStringLiteral(", line number: ")
            << lineNumber << QStringLiteral(", sourceID = ") << sourceID);
    QWebPage::javaScriptConsoleMessage(message, lineNumber, sourceID);
}
#else
void NoteEditorPage::javaScriptAlert(const QUrl & securityOrigin, const QString & msg)
{
    QNDEBUG(QStringLiteral("NoteEditorPage::javaScriptAlert, message: ") << msg);
    QWebEnginePage::javaScriptAlert(securityOrigin, msg);
}

bool NoteEditorPage::javaScriptConfirm(const QUrl & securityOrigin, const QString & msg)
{
    QNDEBUG(QStringLiteral("NoteEditorPage::javaScriptConfirm, message: ") << msg);
    return QWebEnginePage::javaScriptConfirm(securityOrigin, msg);
}

void NoteEditorPage::javaScriptConsoleMessage(QWebEnginePage::JavaScriptConsoleMessageLevel level,
                                              const QString & message, int lineNumber, const QString & sourceID)
{
    QNDEBUG(QStringLiteral("NoteEditorPage::javaScriptConsoleMessage, message: ") << message << QStringLiteral(", level = ")
            << level << QStringLiteral(", line number: ") << lineNumber << QStringLiteral(", sourceID = ") << sourceID);
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
}
#endif // QUENTIER_USE_QT_WEB_ENGINE

} // namespace quentier
