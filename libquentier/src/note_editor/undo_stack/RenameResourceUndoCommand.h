#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_RENAME_RESOURCE_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_RENAME_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include <quentier/types/Resource.h>
#include <QHash>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(GenericResourceImageManager)

class RenameResourceUndoCommand: public INoteEditorUndoCommand
{
    Q_OBJECT
public:
    RenameResourceUndoCommand(const Resource & resource, const QString & previousResourceName,
                              NoteEditorPrivate & noteEditor, GenericResourceImageManager * pGenericResourceImageManager,
                              QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                              QUndoCommand * parent = Q_NULLPTR);
    RenameResourceUndoCommand(const Resource & resource, const QString & previousResourceName,
                              NoteEditorPrivate & noteEditor, GenericResourceImageManager * pGenericResourceImageManager,
                              QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                              const QString & text,  QUndoCommand * parent = Q_NULLPTR);
    virtual ~RenameResourceUndoCommand();

    virtual void undoImpl() Q_DECL_OVERRIDE;
    virtual void redoImpl() Q_DECL_OVERRIDE;

private:
    Resource                        m_resource;
    QString                         m_previousResourceName;
    QString                         m_newResourceName;
    GenericResourceImageManager *   m_pGenericResourceImageManager;
    QHash<QByteArray, QString> &    m_genericResourceImageFilePathsByResourceHash;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_RENAME_RESOURCE_UNDO_COMMAND_H
