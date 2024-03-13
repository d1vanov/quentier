/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include <quentier/local_storage/Fwd.h>
#include <quentier/utility/StringUtils.h>

#include <QDialog>
#include <QPointer>

#include <memory>

namespace Ui {

class AddOrEditSavedSearchDialog;

} // namespace Ui

namespace quentier {

class SavedSearchModel;

class AddOrEditSavedSearchDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditSavedSearchDialog(
        SavedSearchModel * savedSearchModel, QWidget * parent = nullptr,
        const QString & editedSavedSearchLocalId = {});

    ~AddOrEditSavedSearchDialog() override;

    [[nodiscard]] QString query() const;
    void setQuery(const QString & query);

private Q_SLOTS:
    void accept() override;
    void onSavedSearchNameEdited(const QString & savedSearchName);
    void onSearchQueryEdited();

private:
    void createConnections();
    void setupEditedSavedSearchItem();

private:
    Ui::AddOrEditSavedSearchDialog * m_ui;
    QPointer<SavedSearchModel> m_savedSearchModel;
    std::unique_ptr<local_storage::NoteSearchQuery> m_searchQuery;
    QString m_editedSavedSearchLocalId;
    StringUtils m_stringUtils;
};

} // namespace quentier
