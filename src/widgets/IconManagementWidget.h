/*
 * Copyright 2018 Dmitry Ivanov
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

#ifndef QUENTIER_WIDGETS_ICON_MANAGEMENT_WIDGET_H
#define QUENTIER_WIDGETS_ICON_MANAGEMENT_WIDGET_H

#include <quentier/utility/Macros.h>
#include <QWidget>
#include <QStringList>
#include <QVector>

namespace Ui {
class IconManagementWidget;
}

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(IconThemeManager)

class IconManagementWidget : public QWidget
{
    Q_OBJECT
public:
    class IProvider
    {
    public:
        virtual QStringList usedIconNames() const = 0;
        virtual IconThemeManager & iconThemeManager() = 0;
    };

    explicit IconManagementWidget(IProvider & provider, QWidget * parent = Q_NULLPTR);
    ~IconManagementWidget();

private:
    void setupOverrideIconsTableWidget();

private:
    Q_DISABLE_COPY(IconManagementWidget)

private:
    Ui::IconManagementWidget *  m_pUi;
    IProvider &                 m_provider;

    QVector<QPushButton*>       m_pickOverrideIconPushButtons;
    QVector<QLineEdit*>         m_overrideIconFilePathLineEdits;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_ICON_MANAGEMENT_WIDGET_H
