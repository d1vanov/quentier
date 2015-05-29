#include "NoteEditor.h"
#include "NoteEditor_p.h"
#include "INoteEditorResourceInserter.h"
#include <client/types/Note.h>
#include <QFont>
#include <QColor>

namespace qute_note {

NoteEditor::NoteEditor(QWidget * parent) :
    QWebView(parent),
    d_ptr(new NoteEditorPrivate(*this))
{}

NoteEditor::~NoteEditor()
{}

void NoteEditor::setNote(const Note & note)
{
    Q_D(NoteEditor);
    d->setNote(note);
}

const Note * NoteEditor::getNote() const
{
    Q_D(const NoteEditor);
    return d->getNote();
}

bool NoteEditor::isModified() const
{
    Q_D(const NoteEditor);
    return d->isModified();
}

void NoteEditor::addResourceInserterForMimeType(const QString & mimeTypeName,
                                                INoteEditorResourceInserter * pResourceInserter)
{
    Q_D(NoteEditor);
    d->addResourceInserterForMimeType(mimeTypeName, pResourceInserter);
}

bool NoteEditor::removeResourceInserterForMimeType(const QString & mimeTypeName)
{
    Q_D(NoteEditor);
    return d->removeResourceInserterForMimeType(mimeTypeName);
}

bool NoteEditor::hasResourceInsertedForMimeType(const QString & mimeTypeName) const
{
    Q_D(const NoteEditor);
    return d->hasResourceInsertedForMimeType(mimeTypeName);
}

void NoteEditor::undo()
{
    page()->triggerAction(QWebPage::Undo);
}

void NoteEditor::redo()
{
    page()->triggerAction(QWebPage::Redo);
}

void NoteEditor::cut()
{
    page()->triggerAction(QWebPage::Cut);
}

void NoteEditor::copy()
{
    page()->triggerAction(QWebPage::Copy);
}

void NoteEditor::paste()
{
    page()->triggerAction(QWebPage::Paste);
}

void NoteEditor::textBold()
{
    page()->triggerAction(QWebPage::ToggleBold);
}

void NoteEditor::textItalic()
{
    page()->triggerAction(QWebPage::ToggleItalic);
}

void NoteEditor::textUnderline()
{
    page()->triggerAction(QWebPage::ToggleUnderline);
}

void NoteEditor::textStrikethrough()
{
    page()->triggerAction(QWebPage::ToggleStrikethrough);
}

void NoteEditor::alignLeft()
{
    page()->triggerAction(QWebPage::AlignLeft);
}

void NoteEditor::alignCenter()
{
    page()->triggerAction(QWebPage::AlignCenter);
}

void NoteEditor::alignRight()
{
    page()->triggerAction(QWebPage::AlignRight);
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

void NoteEditor::onNoteLoadCancelled()
{
    // TODO: implement
}

void NoteEditor::dropEvent(QDropEvent * pEvent)
{
    Q_D(NoteEditor);
    d->onDropEvent(pEvent);
}

} // namespace qute_note
