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

#include <QObject>

namespace quentier {

class INotebookModelItem;
class NotebookModel;

class NotebookModelTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit NotebookModelTestHelper(
        local_storage::ILocalStoragePtr localStorage,
        QObject * parent = nullptr);

    ~NotebookModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private:
    [[nodiscard]] bool checkSorting(
        const NotebookModel & model,
        const INotebookModelItem * modelItem) const;

    struct LessByName
    {
        [[nodiscard]] bool operator()(
            const INotebookModelItem * lhs,
            const INotebookModelItem * rhs) const noexcept;
    };

    struct GreaterByName
    {
        [[nodiscard]] bool operator()(
            const INotebookModelItem * lhs,
            const INotebookModelItem * rhs) const noexcept;
    };

private:
    const local_storage::ILocalStoragePtr m_localStorage;
};

} // namespace quentier
