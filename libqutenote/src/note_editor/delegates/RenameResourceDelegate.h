#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__RENAME_RESOURCE_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__RENAME_RESOURCE_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/ResourceWrapper.h>
#include <QObject>
#include <QUuid>
#include <QHash>

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
                                    GenericResourceImageWriter * pGenericResourceImageWriter,
                                    QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                                    const bool performingUndo = false);
    void start();
    void startWithPresetNames(const QString & oldResourceName, const QString & newResourceName);

Q_SIGNALS:
    void finished(QString oldResourceName, QString newResourceName, ResourceWrapper resource,
                  bool performingUndo);
    void cancelled();
    void notifyError(QString);

// private signals
#ifdef USE_QT_WEB_ENGINE
    void saveGenericResourceImageToFile(QString noteLocalUid, QString resourceLocalUid, QByteArray resourceImageData,
                                        QString resourceFileSuffix, QByteArray resourceActualHash,
                                        QString resourceDisplayName, QUuid requestId);
#endif

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onRenameResourceDialogFinished(QString newResourceName);

#ifdef USE_QT_WEB_ENGINE
    void onGenericResourceImageWriterFinished(bool success, QByteArray resourceHash, QString filePath,
                                              QString errorDescription, QUuid requestId);
    void onGenericResourceImageUpdated(const QVariant & data);
#endif

private:
    void doStart();
    void raiseRenameResourceDialog();

#ifdef USE_QT_WEB_ENGINE
    void buildAndSaveGenericResourceImage();
#endif

private:
    typedef JsResultCallbackFunctor<RenameResourceDelegate> JsCallback;

private:
    NoteEditorPrivate &             m_noteEditor;
    GenericResourceImageWriter *    m_pGenericResourceImageWriter;
    QHash<QByteArray, QString> &    m_genericResourceImageFilePathsByResourceHash;
    ResourceWrapper                 m_resource;

    QString                         m_oldResourceName;
    QString                         m_newResourceName;
    bool                            m_shouldGetResourceNameFromDialog;

    bool                            m_performingUndo;

    Note *                          m_pNote;

#ifdef USE_QT_WEB_ENGINE
    QUuid                           m_genericResourceImageWriterRequestId;
#endif
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__RENAME_RESOURCE_DELEGATE_H
