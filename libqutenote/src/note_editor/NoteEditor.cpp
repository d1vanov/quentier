#include "NoteEditor_p.h"
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/note_editor/INoteEditorBackend.h>
#include <qute_note/types/Notebook.h>
#include <QUndoStack>
#include <QFont>
#include <QColor>
#include <QVBoxLayout>
#include <QDragMoveEvent>

namespace qute_note {

NoteEditor::NoteEditor(QWidget * parent, Qt::WindowFlags flags) :
    QWidget(parent, flags),
    m_backend(new NoteEditorPrivate(*this))
{
    QVBoxLayout * pLayout = new QVBoxLayout;
    pLayout->addWidget(m_backend->widget());
    pLayout->setMargin(0);
    setLayout(pLayout);
    setAcceptDrops(true);
}

NoteEditor::~NoteEditor()
{}

void NoteEditor::setBackend(INoteEditorBackend * backend)
{
    QLayout * pLayout = layout();
    pLayout->removeWidget(m_backend->widget());

    backend->widget()->setParent(this, this->windowFlags());
    m_backend = backend;
    pLayout->addWidget(m_backend->widget());
}

void NoteEditor::setUndoStack(QUndoStack * pUndoStack)
{
    m_backend->setUndoStack(pUndoStack);
}

void NoteEditor::setNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    m_backend->setNoteAndNotebook(note, notebook);
}

void NoteEditor::convertToNote()
{
    m_backend->convertToNote();
}

void NoteEditor::undo()
{
    m_backend->undo();
}

void NoteEditor::redo()
{
    m_backend->redo();
}

void NoteEditor::cut()
{
    m_backend->cut();
}

void NoteEditor::copy()
{
    m_backend->copy();
}

void NoteEditor::paste()
{
    m_backend->paste();
}

void NoteEditor::pasteUnformatted()
{
    m_backend->pasteUnformatted();
}

void NoteEditor::selectAll()
{
    m_backend->selectAll();
}

void NoteEditor::fontMenu()
{
    m_backend->fontMenu();
}

void NoteEditor::textBold()
{
    m_backend->textBold();
}

void NoteEditor::textItalic()
{
    m_backend->textItalic();
}

void NoteEditor::textUnderline()
{
    m_backend->textUnderline();
}

void NoteEditor::textStrikethrough()
{
    m_backend->textStrikethrough();
}

void NoteEditor::textHighlight()
{
    m_backend->textHighlight();
}

void NoteEditor::alignLeft()
{
    m_backend->alignLeft();
}

void NoteEditor::alignCenter()
{
    m_backend->alignCenter();
}

void NoteEditor::alignRight()
{
    m_backend->alignRight();
}

QString NoteEditor::selectedText() const
{
    return m_backend->selectedText();
}

bool NoteEditor::hasSelection() const
{
    return m_backend->hasSelection();
}

void NoteEditor::findNext(const QString & text, const bool matchCase) const
{
    m_backend->findNext(text, matchCase);
}

void NoteEditor::findPrevious(const QString & text, const bool matchCase) const
{
    m_backend->findPrevious(text, matchCase);
}

void NoteEditor::replace(const QString & textToReplace, const QString & replacementText, const bool matchCase)
{
    m_backend->replace(textToReplace, replacementText, matchCase);
}

void NoteEditor::replaceAll(const QString & textToReplace, const QString & replacementText, const bool matchCase)
{
    m_backend->replaceAll(textToReplace, replacementText, matchCase);
}

void NoteEditor::insertToDoCheckbox()
{
    m_backend->insertToDoCheckbox();
}

void NoteEditor::setSpellcheck(const bool enabled)
{
    m_backend->setSpellcheck(enabled);
}

bool NoteEditor::spellCheckEnabled() const
{
    return m_backend->spellCheckEnabled();
}

void NoteEditor::setFont(const QFont & font)
{
    m_backend->setFont(font);
}

void NoteEditor::setFontHeight(const int height)
{
    m_backend->setFontHeight(height);
}

void NoteEditor::setFontColor(const QColor & color)
{
    m_backend->setFontColor(color);
}

void NoteEditor::setBackgroundColor(const QColor & color)
{
    m_backend->setBackgroundColor(color);
}

void NoteEditor::insertHorizontalLine()
{
    m_backend->insertHorizontalLine();
}

void NoteEditor::increaseFontSize()
{
    m_backend->increaseFontSize();
}

void NoteEditor::decreaseFontSize()
{
    m_backend->decreaseFontSize();
}

void NoteEditor::increaseIndentation()
{
    m_backend->increaseIndentation();
}

void NoteEditor::decreaseIndentation()
{
    m_backend->decreaseIndentation();
}

void NoteEditor::insertBulletedList()
{
    m_backend->insertBulletedList();
}

void NoteEditor::insertNumberedList()
{
    m_backend->insertNumberedList();
}

void NoteEditor::insertTableDialog()
{
    m_backend->insertTableDialog();
}

void NoteEditor::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    m_backend->insertFixedWidthTable(rows, columns, widthInPixels);
}

void NoteEditor::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    m_backend->insertRelativeWidthTable(rows, columns, relativeWidth);
}

void NoteEditor::insertTableRow()
{
    m_backend->insertTableRow();
}

void NoteEditor::insertTableColumn()
{
    m_backend->insertTableColumn();
}

void NoteEditor::removeTableRow()
{
    m_backend->removeTableRow();
}

void NoteEditor::removeTableColumn()
{
    m_backend->removeTableColumn();
}

void NoteEditor::addAttachmentDialog()
{
    m_backend->addAttachmentDialog();
}

void NoteEditor::saveAttachmentDialog(const QString & resourceHash)
{
    m_backend->saveAttachmentDialog(resourceHash);
}

void NoteEditor::saveAttachmentUnderCursor()
{
    m_backend->saveAttachmentUnderCursor();
}

void NoteEditor::openAttachment(const QString & resourceHash)
{
    m_backend->openAttachment(resourceHash);
}

void NoteEditor::openAttachmentUnderCursor()
{
    m_backend->openAttachmentUnderCursor();
}

void NoteEditor::copyAttachment(const QString & resourceHash)
{
    m_backend->copyAttachment(resourceHash);
}

void NoteEditor::copyAttachmentUnderCursor()
{
    m_backend->copyAttachmentUnderCursor();
}

void NoteEditor::encryptSelectedText()
{
    m_backend->encryptSelectedText();
}

void NoteEditor::decryptEncryptedTextUnderCursor()
{
    m_backend->decryptEncryptedTextUnderCursor();
}

void NoteEditor::editHyperlinkDialog()
{
    m_backend->editHyperlinkDialog();
}

void NoteEditor::copyHyperlink()
{
    m_backend->copyHyperlink();
}

void NoteEditor::removeHyperlink()
{
    m_backend->removeHyperlink();
}

void NoteEditor::onNoteLoadCancelled()
{
    m_backend->onNoteLoadCancelled();
}

void NoteEditor::setFocus()
{
    QWidget * pWidget = m_backend->widget();
    if (Q_LIKELY(pWidget)) {
        pWidget->setFocus();
    }
}

void NoteEditor::dragMoveEvent(QDragMoveEvent * pEvent)
{
    if (Q_LIKELY(pEvent)) {
        pEvent->acceptProposedAction();
    }
}

void NoteEditor::dropEvent(QDropEvent * pEvent)
{
    if (Q_LIKELY(pEvent)) {
        pEvent->acceptProposedAction();
    }
}

} // namespace qute_note
