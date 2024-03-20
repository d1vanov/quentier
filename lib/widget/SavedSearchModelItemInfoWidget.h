/*
 * Copyright 2017-2024 Dmitry Ivanov
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

class SavedSearchModelItemInfoWidget;

} // namespace Ui

class QModelIndex;

namespace quentier {

class SavedSearchItem;

class SavedSearchModelItemInfoWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit SavedSearchModelItemInfoWidget(
        const QModelIndex & index, QWidget * parent = nullptr);

    ~SavedSearchModelItemInfoWidget() override;

private:
    void setCheckboxesReadOnly();

    void setNonSavedSearchModel();
    void setInvalidIndex();
    void setNoModelItem();
    void setSavedSearchItem(const SavedSearchItem & item);

    void hideAll();

    void keyPressEvent(QKeyEvent * event) override;

private:
    Ui::SavedSearchModelItemInfoWidget * m_ui;
};

} // namespace quentier
