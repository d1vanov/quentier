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

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_IMAGE_JAVA_SCRIPT_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_IMAGE_JAVA_SCRIPT_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>
#include <QHash>

namespace quentier {

class GenericResourceImageJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit GenericResourceImageJavaScriptHandler(const QHash<QByteArray, QString> & cache, QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void genericResourceImageFound(QByteArray resourceHash, QString genericResourceImageFilePath);

public Q_SLOTS:
    void findGenericResourceImage(QByteArray resourceHash);

private:
    const QHash<QByteArray, QString> & m_cache;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_IMAGE_JAVA_SCRIPT_HANDLER_H
