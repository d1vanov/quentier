#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__RENAME_RESOURCE_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__RENAME_RESOURCE_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/ResourceWrapper.h>
#include <QObject>
#include <QUuid>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(GenericResourceImageWriter)

/**
 * @brief The RenameResourceDelegate class encapsulates a chain of callbacks required
 * for proper implementation of renaming a resource displayed on the note editor's page
 * considering the details of wrapping this action around undo stack
 */
class RenameResourceDelegate: public QObject
{
    Q_OBJECT
public:
    explicit RenameResourceDelegate(const ResourceWrapper & resource, NoteEditorPrivate & noteEditor,
                                    GenericResourceImageWriter * pGenericResourceImageWriter);
    void start();

Q_SIGNALS:
    void finished(QString oldResourceName, QString newResourceName, QString newResourceImageFilePath);
    void cancelled();
    void notifyError(QString);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onRenameResourceDialogFinished(QString newResourceName);

private:
    void doStart();

private:
    NoteEditorPrivate &             m_noteEditor;
    GenericResourceImageWriter *    m_pGenericResourceImageWriter;
    ResourceWrapper                 m_resource;

    QString                         m_oldResourceName;
    QString                         m_newResourceName;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__RENAME_RESOURCE_DELEGATE_H
