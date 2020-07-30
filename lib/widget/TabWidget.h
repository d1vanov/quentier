/*
 * Copyright 2017-2020 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_LIB_WIDGET_TAB_WIDGET_H
#define QUENTIER_LIB_WIDGET_TAB_WIDGET_H

#include <quentier/utility/Macros.h>

#include <QTabWidget>

namespace quentier {

class TabWidget: public QTabWidget
{
    Q_OBJECT
public:
    explicit TabWidget(QWidget * parent = nullptr);

    virtual ~TabWidget() override;

    QTabBar * tabBar() const;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_TAB_WIDGET_H
