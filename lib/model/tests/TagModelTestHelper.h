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

class ITagModelItem;
class TagModel;

class TagModelTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit TagModelTestHelper(
        local_storage::ILocalStoragePtr localStorage,
        QObject * parent = nullptr);

    ~TagModelTestHelper() override;

Q_SIGNALS:
    void failure(ErrorString errorDescription);
    void success();

public Q_SLOTS:
    void test();

private:
    [[nodiscard]] bool checkSorting(
        const TagModel & model, const ITagModelItem * rootItem,
        ErrorString & errorDescription) const;

    struct LessByName
    {
        [[nodiscard]] bool operator()(
            const ITagModelItem * lhs,
            const ITagModelItem * rhs) const noexcept;
    };

    struct GreaterByName
    {
        [[nodiscard]] bool operator()(
            const ITagModelItem * lhs,
            const ITagModelItem * rhs) const noexcept;
    };

private:
    const local_storage::ILocalStoragePtr m_localStorage;
};

} // namespace quentier
