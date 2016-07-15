/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ResourceInfo.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

void ResourceInfo::cacheResourceInfo(const QByteArray & resourceHash,
                                     const QString & resourceDisplayName,
                                     const QString & resourceDisplaySize,
                                     const QString & resourceLocalFilePath)
{
    QNDEBUG("ResourceInfo::cacheResourceInfo: resource hash = "
            << resourceHash.toHex()
            << ", resource display name = " << resourceDisplayName
            << ", resource display size = " << resourceDisplaySize
            << ", resource local file path = " << resourceLocalFilePath);

    Info & info = m_resourceInfoHash[resourceHash];
    info.m_resourceDisplayName = resourceDisplayName;
    info.m_resourceDisplaySize = resourceDisplaySize;
    info.m_resourceLocalFilePath = resourceLocalFilePath;
}

bool ResourceInfo::contains(const QByteArray & resourceHash) const
{
    auto it = m_resourceInfoHash.find(resourceHash);
    return (it != m_resourceInfoHash.end());
}

bool ResourceInfo::findResourceInfo(const QByteArray & resourceHash,
                                    QString & resourceDisplayName,
                                    QString & resourceDisplaySize,
                                    QString & resourceLocalFilePath) const
{
    QNDEBUG("ResourceInfo::findResourceInfo: resource hash = " << resourceHash.toHex());

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

bool ResourceInfo::removeResourceInfo(const QByteArray & resourceHash)
{
    QNDEBUG("ResourceInfo::removeResourceInfo: resource hash = " << resourceHash.toHex());

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

} // namespace quentier
