#include <qute_note/note_editor/NoteEditor.h>
#include "NoteEditor_p.h"

#ifndef USE_QT_WEB_ENGINE
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#endif

#include <qute_note/types/Note.h>
#include <qute_note/types/Notebook.h>
#include <QFont>
#include <QColor>

namespace qute_note {

NoteEditor::NoteEditor(QWidget * parent) :
    WebView(parent),
    d_ptr(new NoteEditorPrivate(*this))
{}

NoteEditor::~NoteEditor()
{}

void NoteEditor::setNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    Q_D(NoteEditor);
    d->setNoteAndNotebook(note, notebook);
}

const Note * NoteEditor::getNote()
{
    Q_D(NoteEditor);
    return d->getNote();
}

const Notebook * NoteEditor::getNotebook() const
{
    Q_D(const NoteEditor);
    return d->getNotebook();
}

bool NoteEditor::isModified() const
{
    Q_D(const NoteEditor);
    return d->isModified();
}

#ifndef USE_QT_WEB_ENGINE
const NoteEditorPluginFactory & NoteEditor::pluginFactory() const
{
    Q_D(const NoteEditor);
    return d->pluginFactory();
}

NoteEditorPluginFactory & NoteEditor::pluginFactory()
{
    Q_D(NoteEditor);
    return d->pluginFactory();
}
#endif

void NoteEditor::undo()
{
    page()->triggerAction(WebPage::Undo);
}

void NoteEditor::redo()
{
    page()->triggerAction(WebPage::Redo);
}

void NoteEditor::cut()
{
    page()->triggerAction(WebPage::Cut);
}

void NoteEditor::copy()
{
    page()->triggerAction(WebPage::Copy);
}

void NoteEditor::paste()
{
    page()->triggerAction(WebPage::Paste);
}

void NoteEditor::textBold()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::ToggleBold);
#else
    Q_D(NoteEditor);
    d->execJavascriptCommand("document.execCommand('bold');");
#endif
}

void NoteEditor::textItalic()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::ToggleItalic);
#else
    Q_D(NoteEditor);
    d->execJavascriptCommand("document.execCommand('italic');");
#endif
}

void NoteEditor::textUnderline()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::ToggleUnderline);
#else
    Q_D(NoteEditor);
    d->execJavascriptCommand("document.execCommand('underline');");
#endif
}

void NoteEditor::textStrikethrough()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::ToggleStrikethrough);
#else
    Q_D(NoteEditor);
    d->execJavascriptCommand("document.execCommand('strikethrough');");
#endif
}

void NoteEditor::alignLeft()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::AlignLeft);
#else
    Q_D(NoteEditor);
    d->execJavascriptCommand("document.execCommand('justifyleft');");
#endif
}

void NoteEditor::alignCenter()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::AlignCenter);
#else
    Q_D(NoteEditor);
    d->execJavascriptCommand("document.execCommand('justifycenter');");
#endif
}

void NoteEditor::alignRight()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::AlignRight);
#else
    Q_D(NoteEditor);
    d->execJavascriptCommand("document.execCommand('justifyright');");
#endif
}

void NoteEditor::insertToDoCheckbox()
{
    Q_D(NoteEditor);
    d->insertToDoCheckbox();
}

void NoteEditor::setSpellcheck(const bool enabled)
{
    // TODO: implement
    Q_UNUSED(enabled)
}

void NoteEditor::setFont(const QFont & font)
{
    Q_D(NoteEditor);
    d->setFont(font);
}

void NoteEditor::setFontHeight(const int height)
{
    Q_D(NoteEditor);
    d->setFontHeight(height);
}

void NoteEditor::setFontColor(const QColor & color)
{
    Q_D(NoteEditor);
    d->setFontColor(color);
}

void NoteEditor::setBackgroundColor(const QColor & color)
{
    Q_D(NoteEditor);
    d->setBackgroundColor(color);
}

void NoteEditor::insertHorizontalLine()
{
    Q_D(NoteEditor);
    d->insertHorizontalLine();
}

void NoteEditor::changeIndentation(const bool increase)
{
    Q_D(NoteEditor);
    d->changeIndentation(increase);
}

void NoteEditor::insertBulletedList()
{
    Q_D(NoteEditor);
    d->insertBulletedList();
}

void NoteEditor::insertNumberedList()
{
    Q_D(NoteEditor);
    d->insertNumberedList();
}

void NoteEditor::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    Q_D(NoteEditor);
    d->insertFixedWidthTable(rows, columns, widthInPixels);
}

void NoteEditor::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    Q_D(NoteEditor);
    d->insertRelativeWidthTable(rows, columns, relativeWidth);
}

void NoteEditor::encryptSelectedText(const QString & passphrase, const QString & hint)
{
    Q_D(NoteEditor);
    d->encryptSelectedText(passphrase, hint);
}

void NoteEditor::onEncryptedAreaDecryption(QString encryptedText, QString decryptedText, bool rememberForSession)
{
    Q_D(NoteEditor);
    d->onEncryptedAreaDecryption(encryptedText, decryptedText, rememberForSession);
}

void NoteEditor::onNoteLoadCancelled()
{
    Q_D(NoteEditor);
    d->onNoteLoadCancelled();
}

void NoteEditor::dropEvent(QDropEvent * pEvent)
{
    Q_D(NoteEditor);
    d->onDropEvent(pEvent);
}

} // namespace qute_note
