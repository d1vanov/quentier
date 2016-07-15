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

#ifndef LIB_QUENTIER_NOTE_EDITOR_RESOURCE_INFO_H
#define LIB_QUENTIER_NOTE_EDITOR_RESOURCE_INFO_H

#include <QString>
#include <QHash>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_RESOURCE_INFO_H
