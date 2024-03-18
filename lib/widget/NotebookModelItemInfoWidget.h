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

#pragma once

#include <QWidget>

namespace Ui {

class NotebookModelItemInfoWidget;

} // namespace Ui

class QModelIndex;

namespace quentier {

class NotebookItem;
class StackItem;

class NotebookModelItemInfoWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit NotebookModelItemInfoWidget(
        const QModelIndex & index, QWidget * parent = nullptr);

    ~NotebookModelItemInfoWidget() override;

private:
    void setCheckboxesReadOnly();

    void setNonNotebookModel();
    void setInvalidIndex();
    void setNoModelItem();
    void setMessedUpModelItemType();

    void hideAll();

    void hideNotebookStuff();
    void showNotebookStuff();
    void setNotebookStuffHidden(bool flag);

    void hideStackStuff();
    void showStackStuff();
    void setStackStuffHidden(bool flag);

    void setNotebookItem(const NotebookItem & item);
    void setStackItem(const StackItem & item, const int numChildren);

    void keyPressEvent(QKeyEvent * event) override;

private:
    Ui::NotebookModelItemInfoWidget * m_ui;
};

} // namespace quentier
