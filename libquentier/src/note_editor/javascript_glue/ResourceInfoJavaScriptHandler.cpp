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

#include "ResourceInfoJavaScriptHandler.h"
#include "../ResourceInfo.h"

namespace quentier {

ResourceInfoJavaScriptHandler::ResourceInfoJavaScriptHandler(const ResourceInfo & resourceInfo,
                                                             QObject *parent) :
    QObject(parent),
    m_resourceInfo(resourceInfo)
{}

void ResourceInfoJavaScriptHandler::findResourceInfo(const QString & resourceHash)
{
    QString resourceDisplayName;
    QString resourceDisplaySize;
    QString resourceLocalFilePath;

    bool found = m_resourceInfo.findResourceInfo(QByteArray::fromHex(resourceHash.toLocal8Bit()),
                                                 resourceDisplayName, resourceDisplaySize, resourceLocalFilePath);
    if (found) {
        emit notifyResourceInfo(resourceHash, resourceLocalFilePath, resourceDisplayName, resourceDisplaySize);
    }
}

} // namespace quentier
