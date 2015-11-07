#include <qute_note/note_editor/NoteEditor.h>
#include "NoteEditor_p.h"

#ifndef USE_QT_WEB_ENGINE
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#endif

#include <qute_note/types/Note.h>
#include <qute_note/types/Notebook.h>
#include <QFont>
#include <QColor>
#include <QContextMenuEvent>

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

void NoteEditor::convertToNote()
{
    Q_D(NoteEditor);
    d->convertToNote();
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
    Q_D(NoteEditor);
    d->undo();
}

void NoteEditor::redo()
{
    Q_D(NoteEditor);
    d->redo();
}

void NoteEditor::cut()
{
    Q_D(NoteEditor);
    d->cut();
}

void NoteEditor::copy()
{
    Q_D(NoteEditor);
    d->copy();
}

void NoteEditor::paste()
{
    Q_D(NoteEditor);
    d->paste();
}

void NoteEditor::pasteUnformatted()
{
    Q_D(NoteEditor);
    d->pasteUnformatted();
}

void NoteEditor::selectAll()
{
    Q_D(NoteEditor);
    d->selectAll();
}

void NoteEditor::fontMenu()
{
    Q_D(NoteEditor);
    d->fontMenu();
}

void NoteEditor::textBold()
{
    Q_D(NoteEditor);
    d->textBold();
}

void NoteEditor::textItalic()
{
    Q_D(NoteEditor);
    d->textItalic();
}

void NoteEditor::textUnderline()
{
    Q_D(NoteEditor);
    d->textUnderline();
}

void NoteEditor::textStrikethrough()
{
    Q_D(NoteEditor);
    d->textStrikethrough();
}

void NoteEditor::textHighlight()
{
    Q_D(NoteEditor);
    d->textHighlight();
}

void NoteEditor::alignLeft()
{
    Q_D(NoteEditor);
    d->alignLeft();
}

void NoteEditor::alignCenter()
{
    Q_D(NoteEditor);
    d->alignCenter();
}

void NoteEditor::alignRight()
{
    Q_D(NoteEditor);
    d->alignRight();
}

void NoteEditor::insertToDoCheckbox()
{
    Q_D(NoteEditor);
    d->insertToDoCheckbox();
}

void NoteEditor::setSpellcheck(const bool enabled)
{
    Q_D(NoteEditor);
    d->setSpellcheck(enabled);
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

void NoteEditor::increaseFontSize()
{
    Q_D(NoteEditor);
    d->increaseFontSize();
}

void NoteEditor::decreaseFontSize()
{
    Q_D(NoteEditor);
    d->decreaseFontSize();
}

void NoteEditor::increaseIndentation()
{
    Q_D(NoteEditor);
    d->increaseIndentation();
}

void NoteEditor::decreaseIndentation()
{
    Q_D(NoteEditor);
    d->decreaseIndentation();
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

void NoteEditor::insertTableDialog()
{
    Q_D(NoteEditor);
    d->insertTableDialog();
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

void NoteEditor::addAttachmentDialog()
{
    Q_D(NoteEditor);
    d->addAttachmentDialog();
}

void NoteEditor::saveAttachmentDialog(const QString & resourceHash)
{
    Q_D(NoteEditor);
    d->saveAttachmentDialog(resourceHash);
}

void NoteEditor::saveAttachmentUnderCursor()
{
    Q_D(NoteEditor);
    d->saveAttachmentUnderCursor();
}

void NoteEditor::openAttachment(const QString & resourceHash)
{
    Q_D(NoteEditor);
    d->openAttachment(resourceHash);
}

void NoteEditor::openAttachmentUnderCursor()
{
    Q_D(NoteEditor);
    d->openAttachmentUnderCursor();
}

void NoteEditor::copyAttachment(const QString & resourceHash)
{
    Q_D(NoteEditor);
    d->copyAttachment(resourceHash);
}

void NoteEditor::copyAttachmentUnderCursor()
{
    Q_D(NoteEditor);
    d->copyAttachmentUnderCursor();
}

void NoteEditor::encryptSelectedTextDialog()
{
    Q_D(NoteEditor);
    d->encryptSelectedTextDialog();
}

void NoteEditor::encryptSelectedText(const QString & passphrase, const QString & hint, const bool rememberForSession)
{
    Q_D(NoteEditor);
    d->encryptSelectedText(passphrase, hint, rememberForSession);
}

void NoteEditor::decryptEncryptedTextUnderCursor()
{
    Q_D(NoteEditor);
    d->decryptEncryptedTextUnderCursor();
}

void NoteEditor::editHyperlinkDialog()
{
    Q_D(NoteEditor);
    d->editHyperlinkDialog();
}

void NoteEditor::copyHyperlink()
{
    Q_D(NoteEditor);
    d->copyHyperlink();
}

void NoteEditor::removeHyperlink()
{
    Q_D(NoteEditor);
    d->removeHyperlink();
}

void NoteEditor::onEncryptedAreaDecryption(QString encryptedText, QString decryptedText,
                                           bool rememberForSession, bool decryptPermanently)
{
    Q_D(NoteEditor);
    d->onEncryptedAreaDecryption(encryptedText, decryptedText, rememberForSession, decryptPermanently);
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

void NoteEditor::contextMenuEvent(QContextMenuEvent * pEvent)
{
    Q_D(NoteEditor);
    d->contextMenuEvent(pEvent);
}

} // namespace qute_note
