#include "NoteEditorPage.h"
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QWebView>
#include <QMessageBox>
#include <QApplication>

namespace qute_note {

NoteEditorPage::NoteEditorPage(NoteEditor & parent) :
    QWebPage(&parent),
    m_parent(&parent)
{
    QUTE_NOTE_CHECK_PTR(m_parent);

    QObject::connect(this, SIGNAL(noteLoadCancelled()), &parent, SLOT(onNoteLoadCancelled()));
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
}

} // namespace qute_note
