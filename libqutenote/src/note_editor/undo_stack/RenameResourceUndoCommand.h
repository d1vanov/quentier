#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_RENAME_RESOURCE_UNDO_COMMAND_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_RENAME_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include <qute_note/types/ResourceWrapper.h>
#include <QHash>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(GenericResourceImageManager)

class RenameResourceUndoCommand: public INoteEditorUndoCommand
{
public:
    RenameResourceUndoCommand(const ResourceWrapper & resource, const QString & previousResourceName,
                              NoteEditorPrivate & noteEditor, GenericResourceImageManager * pGenericResourceImageManager,
                              QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                              QUndoCommand * parent = Q_NULLPTR);
    RenameResourceUndoCommand(const ResourceWrapper & resource, const QString & previousResourceName,
                              NoteEditorPrivate & noteEditor, GenericResourceImageManager * pGenericResourceImageManager,
                              QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                              const QString & text,  QUndoCommand * parent = Q_NULLPTR);
    virtual ~RenameResourceUndoCommand();

    virtual void undoImpl() Q_DECL_OVERRIDE;
    virtual void redoImpl() Q_DECL_OVERRIDE;

private:
    ResourceWrapper                 m_resource;
    QString                         m_previousResourceName;
    QString                         m_newResourceName;
    GenericResourceImageManager *   m_pGenericResourceImageManager;
    QHash<QByteArray, QString> &    m_genericResourceImageFilePathsByResourceHash;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_RENAME_RESOURCE_UNDO_COMMAND_H
