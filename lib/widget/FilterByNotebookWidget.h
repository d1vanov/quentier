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

#include "AbstractFilterByModelItemWidget.h"

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/ErrorString.h>

#include <qevercloud/types/Fwd.h>

namespace quentier {

class NotebookModel;

class FilterByNotebookWidget : public AbstractFilterByModelItemWidget
{
    Q_OBJECT
public:
    explicit FilterByNotebookWidget(QWidget * parent = nullptr);

    ~FilterByNotebookWidget() override;

    void setLocalStorage(const local_storage::ILocalStorage & localStorage);
    [[nodiscard]] const NotebookModel * notebookModel() const;

private Q_SLOTS:
    void onNotebookPut(const qevercloud::Notebook & notebook);
    void onNotebookExpunged(const QString & notebookLocalId);
};

} // namespace quentier
