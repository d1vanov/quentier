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

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H

#include <quentier/utility/Macros.h>
#include <QObject>

namespace quentier {

class GenericResourceOpenAndSaveButtonsOnClickHandler: public QObject
{
    Q_OBJECT
public:
    explicit GenericResourceOpenAndSaveButtonsOnClickHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void openResourceRequest(const QByteArray & resourceHash);
    void saveResourceRequest(const QByteArray & resourceHash);

public Q_SLOTS:
    void onOpenResourceButtonPressed(const QString & resourceHash);
    void onSaveResourceButtonPressed(const QString & resourceHash);
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_GENERIC_RESOURCE_OPEN_AND_SAVE_BUTTONS_ON_CLICK_HANDLER_H
