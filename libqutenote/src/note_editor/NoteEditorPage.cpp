#include "NoteEditorPage.h"
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <qute_note/logging/QuteNoteLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebView>
#else
#include <QWebEngineView>
#endif

#include <QMessageBox>
#include <QApplication>

namespace qute_note {

NoteEditorPage::NoteEditorPage(NoteEditor & parent) :
#ifndef USE_QT_WEB_ENGINE
    QWebPage(&parent),
#else
    QWebEnginePage(&parent),
#endif
    m_parent(&parent)
{
    QUTE_NOTE_CHECK_PTR(m_parent);

    QObject::connect(this, QNSIGNAL(NoteEditorPage,noteLoadCancelled), &parent, QNSLOT(NoteEditor,onNoteLoadCancelled));
}

bool NoteEditorPage::shouldInterruptJavaScript()
{
    QNDEBUG("NoteEditorPage::shouldInterruptJavaScript");

    QString title = QObject::tr("Note editor hanged");
    QString question = QObject::tr("Note editor seems hanged when loading or editing the note. "
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

#ifndef USE_QT_WEB_ENGINE
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

} // namespace qute_note
