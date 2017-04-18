#ifndef QUENTIER_ENEX_EXPORTER_H
#define QUENTIER_ENEX_EXPORTER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/Note.h>
#include <QObject>
#include <QStringList>
#include <QSet>
#include <QHash>
#include <QUuid>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(NoteEditorTabsAndWindowsCoordinator)
QT_FORWARD_DECLARE_CLASS(TagModel)

class EnexExporter: public QObject
{
    Q_OBJECT
public:
    explicit EnexExporter(LocalStorageManagerThreadWorker & localStorageWorker,
                          NoteEditorTabsAndWindowsCoordinator & coordinator,
                          TagModel & tagModel, QObject * parent = Q_NULLPTR);

    const QString & targetEnexFilePath() const { return m_targetEnexFilePath; }
    void setTargetEnexFilePath(const QString & path) { m_targetEnexFilePath = path; }

    const QStringList & noteLocalUids() const { return m_noteLocalUids; }
    void setNoteLocalUids(const QStringList & noteLocalUids);

    bool includeTags() const { return m_includeTags; }
    void setIncludeTags(const bool includeTags);

    bool isInProgress() const;
    void start();

    void clear();

Q_SIGNALS:
    void notesExportedToEnex(QString enex);
    void failedToExportNotesToEnex(ErrorString errorDescription);

// private signals:
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId);

private Q_SLOTS:
    void onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData,
                          ErrorString errorDescription, QUuid requestId);

    void onAllTagsListed();

private:
    void findNoteInLocalStorage(const QString & noteLocalUid);
    QString convertNotesToEnex(ErrorString & errorDescription);

    void connectToLocalStorage();
    void disconnectFromLocalStorage();

private:
    LocalStorageManagerThreadWorker &       m_localStorageWorker;
    NoteEditorTabsAndWindowsCoordinator &   m_noteEditorTabsAndWindowsCoordinator;
    QPointer<TagModel>                      m_pTagModel;
    QString                                 m_targetEnexFilePath;
    QStringList                             m_noteLocalUids;
    QSet<QUuid>                             m_findNoteRequestIds;
    QHash<QString, Note>                    m_notesByLocalUid;
    bool                                    m_includeTags;
    bool                                    m_connectedToLocalStorage;
};

} // namespace quentier

#endif // QUENTIER_ENEX_EXPORTER_H
