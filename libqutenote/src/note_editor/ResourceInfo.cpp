#include "ResourceInfo.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

void ResourceInfo::cacheResourceInfo(const QString & resourceHash,
                                     const QString & resourceDisplayName,
                                     const QString & resourceDisplaySize,
                                     const QString & resourceLocalFilePath)
{
    QNDEBUG("ResourceInfo::cacheResourceInfo: resource hash = "
            << resourceHash.toLocal8Bit().toHex()
            << ", resource display name = " << resourceDisplayName
            << ", resource display size = " << resourceDisplaySize
            << ", resource local file path = " << resourceLocalFilePath);

    Info & info = m_resourceInfoHash[resourceHash];
    info.m_resourceDisplayName = resourceDisplayName;
    info.m_resourceDisplaySize = resourceDisplaySize;
    info.m_resourceLocalFilePath = resourceLocalFilePath;
}

bool ResourceInfo::contains(const QString & resourceHash) const
{
    auto it = m_resourceInfoHash.find(resourceHash);
    return (it != m_resourceInfoHash.end());
}

bool ResourceInfo::findResourceInfo(const QString & resourceHash,
                                    QString & resourceDisplayName,
                                    QString & resourceDisplaySize,
                                    QString & resourceLocalFilePath) const
{
    QNDEBUG("ResourceInfo::findResourceInfo: resource hash = " << resourceHash.toLocal8Bit().toHex());

    auto it = m_resourceInfoHash.find(resourceHash);
    if (it == m_resourceInfoHash.end()) {
        QNTRACE("Resource info was not found");
        return false;
    }

    const Info & info = it.value();
    resourceDisplayName = info.m_resourceDisplayName;
    resourceDisplaySize = info.m_resourceDisplaySize;
    resourceLocalFilePath = info.m_resourceLocalFilePath;

    QNTRACE("Found resource info: name = " << resourceDisplayName
            << ", size = " << resourceDisplaySize << ", local file path = "
            << resourceLocalFilePath);
    return true;
}

bool ResourceInfo::removeResourceInfo(const QString & resourceHash)
{
    QNDEBUG("ResourceInfo::removeResourceInfo: resource hash = " << resourceHash.toLocal8Bit().toHex());

    auto it = m_resourceInfoHash.find(resourceHash);
    if (it == m_resourceInfoHash.end()) {
        QNTRACE("Resource info was not found hence not removed");
        return false;
    }

    Q_UNUSED(m_resourceInfoHash.erase(it));
    return true;
}

void ResourceInfo::clear()
{
    QNDEBUG("ResourceInfo::clear");
    m_resourceInfoHash.clear();
}

} // namespace qute_note
