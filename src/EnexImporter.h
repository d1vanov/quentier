#ifndef QUENTIER_ENEX_IMPORTER_H
#define QUENTIER_ENEX_IMPORTER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <QObject>
#include <QUuid>
#include <QHash>

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

private Q_SLOTS:
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

    void onAllTagsListed();

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

private:
    LocalStorageManagerThreadWorker &       m_localStorageWorker;
    TagModel &                              m_tagModel;
    QString                                 m_enexFilePath;
    QSet<QUuid>                             m_addTagRequestIds;
    QHash<QString, Tag>                     m_tagsByName;
    bool                                    m_connectedToLocalStorage;
};

} // namespace quentier

#endif // QUENTIER_ENEX_IMPORTER_H
