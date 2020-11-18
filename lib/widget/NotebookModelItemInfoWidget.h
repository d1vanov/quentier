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

#ifndef QUENTIER_LIB_WIDGET_NOTEBOOK_MODEL_ITEM_INFO_WIDGET_H
#define QUENTIER_LIB_WIDGET_NOTEBOOK_MODEL_ITEM_INFO_WIDGET_H

#include <QWidget>

namespace Ui {
class NotebookModelItemInfoWidget;
}

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookItem)
QT_FORWARD_DECLARE_CLASS(StackItem)

class NotebookModelItemInfoWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit NotebookModelItemInfoWidget(
        const QModelIndex & index, QWidget * parent = nullptr);

    virtual ~NotebookModelItemInfoWidget();

private:
    void setCheckboxesReadOnly();

    void setNonNotebookModel();
    void setInvalidIndex();
    void setNoModelItem();
    void setMessedUpModelItemType();

    void hideAll();

    void hideNotebookStuff();
    void showNotebookStuff();
    void setNotebookStuffHidden(const bool flag);

    void hideStackStuff();
    void showStackStuff();
    void setStackStuffHidden(const bool flag);

    void setNotebookItem(const NotebookItem & item);
    void setStackItem(const StackItem & item, const int numChildren);

    virtual void keyPressEvent(QKeyEvent * pEvent) override;

private:
    Ui::NotebookModelItemInfoWidget * m_pUi;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NOTEBOOK_MODEL_ITEM_INFO_WIDGET_H
