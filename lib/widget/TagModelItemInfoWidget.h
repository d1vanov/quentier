/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_TAG_MODEL_ITEM_INFO_WIDGET_H
#define QUENTIER_LIB_WIDGET_TAG_MODEL_ITEM_INFO_WIDGET_H

#include <QWidget>

namespace Ui {
class TagModelItemInfoWidget;
}

class QModelIndex;

namespace quentier {

class ITagModelItem;
class TagItem;

class TagModelItemInfoWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit TagModelItemInfoWidget(
        const QModelIndex & index, QWidget * parent = nullptr);

    ~TagModelItemInfoWidget() override;

private:
    void setCheckboxesReadOnly();

    void setNonTagModel();
    void setInvalidIndex();
    void setNoModelItem();
    void setNonTagItem();
    void setTagItem(const ITagModelItem & modelItem, const TagItem & item);

    void hideAll();

    void keyPressEvent(QKeyEvent * pEvent) override;

private:
    Ui::TagModelItemInfoWidget * m_pUi;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_TAG_MODEL_ITEM_INFO_WIDGET_H
