#include "RenameResourceUndoCommand.h"
#include "../NoteEditor_p.h"
#include "../GenericResourceImageManager.h"
#include "../delegates/RenameResourceDelegate.h"

namespace quentier {

RenameResourceUndoCommand::RenameResourceUndoCommand(const ResourceWrapper & resource, const QString & previousResourceName,
                                                     NoteEditorPrivate & noteEditor,
                                                     GenericResourceImageManager * pGenericResourceImageManager,
                                                     QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                                                     QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_resource(resource),
    m_previousResourceName(previousResourceName),
    m_newResourceName(resource.displayName()),
    m_pGenericResourceImageManager(pGenericResourceImageManager),
    m_genericResourceImageFilePathsByResourceHash(genericResourceImageFilePathsByResourceHash)
{
    setText(tr("Rename attachment"));
}

RenameResourceUndoCommand::RenameResourceUndoCommand(const ResourceWrapper & resource,
                                                     const QString & previousResourceName,
                                                     NoteEditorPrivate & noteEditor,
                                                     GenericResourceImageManager * pGenericResourceImageManager,
                                                     QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                                                     const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_resource(resource),
    m_previousResourceName(previousResourceName),
    m_newResourceName(resource.displayName()),
    m_pGenericResourceImageManager(pGenericResourceImageManager),
    m_genericResourceImageFilePathsByResourceHash(genericResourceImageFilePathsByResourceHash)
{}

RenameResourceUndoCommand::~RenameResourceUndoCommand()
{}

void RenameResourceUndoCommand::undoImpl()
{
    RenameResourceDelegate * delegate = new RenameResourceDelegate(m_resource, m_noteEditorPrivate,
                                                                   m_pGenericResourceImageManager,
                                                                   m_genericResourceImageFilePathsByResourceHash,
                                                                   /* performing undo = */ true);

    m_noteEditorPrivate.setRenameResourceDelegateSubscriptions(*delegate);
    delegate->startWithPresetNames(m_newResourceName, m_previousResourceName);
}

void RenameResourceUndoCommand::redoImpl()
{
    RenameResourceDelegate * delegate = new RenameResourceDelegate(m_resource, m_noteEditorPrivate,
                                                                   m_pGenericResourceImageManager,
                                                                   m_genericResourceImageFilePathsByResourceHash,
                                                                   /* performing undo = */ true);

    m_noteEditorPrivate.setRenameResourceDelegateSubscriptions(*delegate);
    delegate->startWithPresetNames(m_previousResourceName, m_newResourceName);
}

} // namespace quentier
