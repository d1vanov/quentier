#include "NoteEditorController.h"
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <client/types/Note.h>
#include <QWebView>
#include <QWebFrame>
#include <QMimeData>
#include <QFont>
#include <QColor>

NoteEditorController::NoteEditorController(QWebView * webView, QObject * parent) :
    QObject(parent),
    m_pWebView(webView),
    m_modified(false)
{
    QUTE_NOTE_CHECK_PTR(webView)
}

NoteEditorController::~NoteEditorController()
{}

bool NoteEditorController::setNote(const Note & note, QString & errorDescription)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    return true;
}

bool NoteEditorController::getNote(Note & note, QString & errorDescription) const
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(errorDescription)
    return true;
}

bool NoteEditorController::modified() const
{
    return m_modified;
}

void NoteEditorController::undo()
{
    m_pWebView->page()->triggerAction(QWebPage::Undo);
}

void NoteEditorController::redo()
{
    m_pWebView->page()->triggerAction(QWebPage::Redo);
}

void NoteEditorController::cut()
{
    m_pWebView->page()->triggerAction(QWebPage::Cut);
}

void NoteEditorController::copy()
{
    m_pWebView->page()->triggerAction(QWebPage::Copy);
}

void NoteEditorController::paste()
{
    m_pWebView->page()->triggerAction(QWebPage::Paste);
}

void NoteEditorController::textBold()
{
    m_pWebView->page()->triggerAction(QWebPage::ToggleBold);
}

void NoteEditorController::textItalic()
{
    m_pWebView->page()->triggerAction(QWebPage::ToggleItalic);
}

void NoteEditorController::textUnderline()
{
    m_pWebView->page()->triggerAction(QWebPage::ToggleUnderline);
}

void NoteEditorController::textStrikethrough()
{
    m_pWebView->page()->triggerAction(QWebPage::ToggleStrikethrough);
}

void NoteEditorController::alignLeft()
{
    m_pWebView->page()->triggerAction(QWebPage::AlignLeft);
}

void NoteEditorController::alignCenter()
{
    m_pWebView->page()->triggerAction(QWebPage::AlignCenter);
}

void NoteEditorController::alignRight()
{
    m_pWebView->page()->triggerAction(QWebPage::AlignRight);
}

void NoteEditorController::insertToDoCheckbox()
{
    QString html("<input type=\"checkbox\" onmouseover=\"style.cursor=\"arrow\"\" "
                 "onclick=\"if(checked) setAttribute(\"checked\", \"checked\") "
                 "else removeAttribute(\"checked\")\" />");
    execJavascriptCommand("insertHtml", html);
}

void NoteEditorController::setSpellcheck(const bool enabled)
{
    // TODO: implement
    Q_UNUSED(enabled)
}

void NoteEditorController::setFont(const QFont & font)
{
    QString fontName = font.family();
    execJavascriptCommand("fontName", fontName);
}

void NoteEditorController::setFontHeight(const int height)
{
    if (height > 0) {
        execJavascriptCommand("fontSize", QString::number(height));
    }
    else {
        QString error = QT_TR_NOOP("Detected incorrect font size: " + QString::number(height));
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorController::setFontColor(const QColor & color)
{
    if (color.isValid()) {
        execJavascriptCommand("foreColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid font color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorController::setBackgroundColor(const QColor & color)
{
    if (color.isValid()) {
        execJavascriptCommand("hiliteColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid background color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorController::insertHorizontalLine()
{
    execJavascriptCommand("insertHorizontalRule");
}

void NoteEditorController::changeIndentation(const bool increase)
{
    execJavascriptCommand((increase ? "indent" : "outdent"));
}

void NoteEditorController::insertBulletedList()
{
    execJavascriptCommand("insertUnorderedList");
}

void NoteEditorController::insertNumberedList()
{
    execJavascriptCommand("insertOrderedList");
}

void NoteEditorController::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    // TODO: implement
    Q_UNUSED(rows)
    Q_UNUSED(columns)
    Q_UNUSED(widthInPixels)
}

void NoteEditorController::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    // TODO: implement
    Q_UNUSED(rows)
    Q_UNUSED(columns)
    Q_UNUSED(relativeWidth)
}

void NoteEditorController::insertFromMimeData(const QMimeData * source)
{
    // TODO: implement
    Q_UNUSED(source)
}

void NoteEditorController::execJavascriptCommand(const QString & command)
{
    QWebFrame * frame = m_pWebView->page()->mainFrame();
    QString javascript = QString("document.execCommand(\"%1\", false, null)").arg(command);
    frame->evaluateJavaScript(javascript);
    QNDEBUG("Sent javascript command to note editor: " << javascript);
}

void NoteEditorController::execJavascriptCommand(const QString & command, const QString & args)
{
    QWebFrame * frame = m_pWebView->page()->mainFrame();
    QString javascript = QString("document.execCommand(\"%1\", false, \"%2\")").arg(command).arg(args);
    frame->evaluateJavaScript(javascript);
    QNDEBUG("Sent javascript command to note editor: " << javascript);
}

