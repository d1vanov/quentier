#ifndef QUENTIER_ENEX_IMPORTER_H
#define QUENTIER_ENEX_IMPORTER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <QObject>
#include <QUuid>
#include <QHash>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/bimap.hpp>
#endif

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(TagModel)

class EnexImporter: public QObject
{
    Q_OBJECT
public:
    explicit EnexImporter(const QString & enexFilePath,
                          LocalStorageManagerThreadWorker & localStorageWorker,
                          TagModel & tagModel, QObject * parent = Q_NULLPTR);

    bool isInProgress() const;
    void start();

    void clear();

Q_SIGNALS:
    void enexImportedSuccessfully(QString enexFilePath);
    void enexImportFailed(ErrorString errorDescription);

// private signals:
    void addTag(Tag tag, QUuid requestId);
    void addNote(Note note, QUuid requestId);

private Q_SLOTS:
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId);

    void onAddNoteComplete(Note note, QUuid requestId);
    void onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId);

    void onAllTagsListed();

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    void processNotesPendingTagAddition();

    void addNoteToLocalStorage(const Note & note);
    void addTagToLocalStorage(const QString & tagName);

private:
    LocalStorageManagerThreadWorker &       m_localStorageWorker;
    TagModel &                              m_tagModel;
    QString                                 m_enexFilePath;

    QHash<QString, QStringList>             m_tagNamesByImportedNoteLocalUid;

    typedef boost::bimap<QString, QUuid> AddTagRequestIdByTagNameBimap;
    AddTagRequestIdByTagNameBimap           m_addTagRequestIdByTagNameBimap;

    QSet<QString>                           m_expungedTagLocalUids;

    QVector<Note>                           m_notesPendingTagAddition;
    QSet<QUuid>                             m_addNoteRequestIds;

    bool                                    m_connectedToLocalStorage;
};

} // namespace quentier

#endif // QUENTIER_ENEX_IMPORTER_H
