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
#include <quentier/types/ErrorString.h>

#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/Note.h>
#include <qevercloud/types/SavedSearch.h>
#include <qevercloud/types/Tag.h>

#include <QObject>

namespace quentier {

class FavoritesModel;
class FavoritesModelItem;

class FavoritesModelTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit FavoritesModelTestHelper(
        local_storage::ILocalStoragePtr localStorage,
        QObject * parent = nullptr);

    ~FavoritesModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onNoteUpdated(const qevercloud::Note & note);
    void onNotebookPut(const qevercloud::Notebook & notebook);
    void onTagPut(const qevercloud::Tag & tag);
    void onSavedSearchPut(const qevercloud::SavedSearch & savedSearch);

private:
    void checkSorting(const FavoritesModel & model);

private:
    struct LessByType
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct GreaterByType
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct LessByDisplayName
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct GreaterByDisplayName
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct LessByNoteCount
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

    struct GreaterByNoteCount
    {
        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;
    };

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    FavoritesModel * m_model = nullptr;

    qevercloud::Notebook m_firstNotebook;
    qevercloud::Notebook m_secondNotebook;
    qevercloud::Notebook m_thirdNotebook;

    qevercloud::Note m_firstNote;
    qevercloud::Note m_secondNote;
    qevercloud::Note m_thirdNote;
    qevercloud::Note m_fourthNote;
    qevercloud::Note m_fifthNote;
    qevercloud::Note m_sixthNote;

    qevercloud::Tag m_firstTag;
    qevercloud::Tag m_secondTag;
    qevercloud::Tag m_thirdTag;
    qevercloud::Tag m_fourthTag;

    qevercloud::SavedSearch m_firstSavedSearch;
    qevercloud::SavedSearch m_secondSavedSearch;
    qevercloud::SavedSearch m_thirdSavedSearch;
    qevercloud::SavedSearch m_fourthSavedSearch;

    bool m_expectingNoteUpdateFromLocalStorage = false;
    bool m_expectingNotebookUpdateFromLocalStorage = false;
    bool m_expectingTagUpdateFromLocalStorage = false;
    bool m_expectingSavedSearchUpdateFromLocalStorage = false;

    bool m_expectingNoteUnfavoriteFromLocalStorage = false;
    bool m_expectingNotebookUnfavoriteFromLocalStorage = false;
    bool m_expectingTagUnfavoriteFromLocalStorage = false;
    bool m_expectingSavedSearchUnfavoriteFromLocalStorage = false;
};

} // namespace quentier
