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

#ifndef QUENTIER_MODELS_ICON_MANAGEMENT_MODEL_H
#define QUENTIER_MODELS_ICON_MANAGEMENT_MODEL_H

#include <quentier/utility/Macros.h>
#include <QAbstractTableModel>
#include <QStringList>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(IconThemeManager)

class IconManagementModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    class IProvider
    {
    public:
        virtual QStringList usedIconNames() const = 0;
        virtual IconThemeManager & iconThemeManager() = 0;
    };

    explicit IconManagementModel(IProvider & provider, QObject * parent = Q_NULLPTR);

    struct Columns
    {
        enum type
        {
            OriginalThemeIcon = 0,
            OverrideThemeIcon,
            FallbackThemeIcon
        };
    };

private:
    // QAbstractTableModel interface
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(IconManagementModel)

private:
    IProvider &     m_provider;
    QStringList     m_usedIconNames;
};

} // namespace quentier

#endif // QUENTIER_MODELS_ICON_MANAGEMENT_MODEL_H
