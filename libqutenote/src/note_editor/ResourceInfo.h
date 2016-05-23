#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_RESOURCE_INFO_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_RESOURCE_INFO_H

#include <QString>
#include <QHash>

namespace qute_note {

class ResourceInfo
{
public:
    void cacheResourceInfo(const QByteArray & resourceHash,
                           const QString & resourceDisplayName,
                           const QString & resourceDisplaySize,
                           const QString & resourceLocalFilePath);

    bool contains(const QByteArray & resourceHash) const;

    bool findResourceInfo(const QByteArray & resourceHash,
                          QString & resourceDisplayName,
                          QString & resourceDisplaySize,
                          QString & resourceLocalFilePath) const;

    bool removeResourceInfo(const QByteArray & resourceHash);

    void clear();

private:
    struct Info
    {
        QString m_resourceDisplayName;
        QString m_resourceDisplaySize;
        QString m_resourceLocalFilePath;
    };

    typedef QHash<QByteArray, Info> ResourceInfoHash;
    ResourceInfoHash m_resourceInfoHash;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_RESOURCE_INFO_H
