#include "NoteEditorPage.h"
#include "NoteEditorController.h"
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <QWebView>
#include <QMessageBox>
#include <QApplication>

NoteEditorPage::NoteEditorPage(NoteEditorController * pNoteEditorController,
                               QWebView * parentView, QObject * parent) :
    QWebPage(parent),
    m_parentView(parentView)
{
    QUTE_NOTE_CHECK_PTR(m_parentView);

    QObject::connect(this, SIGNAL(noteLoadCancelled()), pNoteEditorController, SLOT(onNoteLoadCancelled()));
}

bool NoteEditorPage::shouldInterruptJavaScript()
{
    QNDEBUG("NoteEditorPage::shouldInterruptJavaScript");

    QString title = QObject::tr("Note editor hanged");
    QString question = QObject::tr("Note editor seems hanged when loading or editing the note. "
                                   "Would you like to cancel loading the note?");
    QMessageBox::StandardButton reply = QMessageBox::question(m_parentView, title, question,
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

