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

#include "GenericResourceImageJavaScriptHandler.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

GenericResourceImageJavaScriptHandler::GenericResourceImageJavaScriptHandler(const QHash<QByteArray, QString> & cache, QObject * parent) :
    QObject(parent),
    m_cache(cache)
{}

void GenericResourceImageJavaScriptHandler::findGenericResourceImage(QByteArray resourceHash)
{
    QNDEBUG(QStringLiteral("GenericResourceImageJavaScriptHandler::findGenericResourceImage: resource hash = ")
            << resourceHash);

    auto it = m_cache.find(QByteArray::fromHex(resourceHash));
    if (it != m_cache.end()) {
        QNTRACE(QStringLiteral("Found generic resouce image, path is ") << it.value());
        emit genericResourceImageFound(resourceHash, it.value());
    }
    else {
        QNINFO(QStringLiteral("Can't find generic resource image for hash ") << resourceHash);
    }
}

} // namespace quentier
