#include "NoteEditorWidget.h"

// Doh, Qt Designer's inability to work with namespaces in the expected way
// is deeply disappointing
#include "NoteTagsWidget.h"
#include "FindAndReplaceWidget.h"
#include <quentier/note_editor/NoteEditor.h>
using quentier::FindAndReplaceWidget;
using quentier::NoteEditor;
using quentier::NoteTagsWidget;
#include "ui_NoteEditorWidget.h"

#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

NoteEditorWidget::NoteEditorWidget(LocalStorageManagerThreadWorker & localStorageWorker,
                                   NoteCache & noteCache, NotebookCache & notebookCache,
                                   TagCache & tagCache, QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::NoteEditorWidget),
    m_noteCache(noteCache),
    m_notebookCache(notebookCache),
    m_tagCache(tagCache),
    m_pCurrentNote(),
    m_pCurrentNotebook()
{
    m_pUi->setupUi(this);
    createConnections(localStorageWorker);
}

NoteEditorWidget::~NoteEditorWidget()
{
    delete m_pUi;
}

void NoteEditorWidget::setNoteLocalUid(const QString & noteLocalUid)
{
    Q_UNUSED(noteLocalUid)
    // TODO: implement
}

void NoteEditorWidget::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                          QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindNoteFailed(Note note, bool withResourceBinaryData, QNLocalizedString errorDescription,
                                        QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onExpungeNoteComplete(Note note, QUuid requestId)
{
    Q_UNUSED(note)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindTagComplete(Tag tag, QUuid requestId)
{
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(tag)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
    // TODO: implement
}

void NoteEditorWidget::onEditorNoteUpdate(Note note)
{
    Q_UNUSED(note)
    // TODO: implement
}

void NoteEditorWidget::onEditorNoteUpdateFailed(QNLocalizedString error)
{
    QNDEBUG("NoteEditorWidget::onEditorNoteUpdateFailed: " << error);

    emit notifyError(error);
}

void NoteEditorWidget::createConnections(LocalStorageManagerThreadWorker & localStorageWorker)
{
    Q_UNUSED(localStorageWorker);
    // TODO: implement
}

} // namespace quentier
