/*
 * Copyright 2019 Dmitry Ivanov
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

#ifndef PANELWIDGET_H
#define PANELWIDGET_H

#include <QFrame>

namespace quentier {

/**
 * @brief The PanelWidget class is a subclass of QFrame without any actual
 * functional extension of its base class. The whole purpose to have a subclass
 * is for using it in Qt stylesheets.
 */
class PanelWidget: public QFrame
{
    Q_OBJECT
public:
    explicit PanelWidget(QWidget * parent = nullptr);

    virtual ~PanelWidget() override;
};

} // namespace quentier

#endif // PANELWIDGET_H
