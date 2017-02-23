/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef QUENTIER_WIDGETS_TAG_MODEL_ITEM_INFO_WIDGET_H
#define QUENTIER_WIDGETS_TAG_MODEL_ITEM_INFO_WIDGET_H

#include <quentier/utility/Macros.h>
#include <QWidget>

namespace Ui {
class TagModelItemInfoWidget;
}

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModelItem)

class TagModelItemInfoWidget: public QWidget
{
    Q_OBJECT
public:
    explicit TagModelItemInfoWidget(const QModelIndex & index,
                                    QWidget * parent = Q_NULLPTR);
    virtual ~TagModelItemInfoWidget();

private:
    void setCheckboxesReadOnly();

    void setNonTagModel();
    void setInvalidIndex();
    void setNoModelItem();
    void setTagItem(const TagModelItem & item);

    void hideAll();

    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;

private:
    Ui::TagModelItemInfoWidget * m_pUi;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_TAG_MODEL_ITEM_INFO_WIDGET_H
