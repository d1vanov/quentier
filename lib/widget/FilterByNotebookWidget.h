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

#ifndef QUENTIER_LIB_WIDGET_FILTER_BY_NOTEBOOK_WIDGET_H
#define QUENTIER_LIB_WIDGET_FILTER_BY_NOTEBOOK_WIDGET_H

#include "AbstractFilterByModelItemWidget.h"

#include <quentier/types/ErrorString.h>

#include <qevercloud/types/Notebook.h>

#include <QPointer>
#include <QUuid>

namespace quentier {

class LocalStorageManagerAsync;
class NotebookModel;

class FilterByNotebookWidget final: public AbstractFilterByModelItemWidget
{
    Q_OBJECT
public:
    explicit FilterByNotebookWidget(QWidget * parent = nullptr);

    ~FilterByNotebookWidget() override;

    void setLocalStorageManager(
        LocalStorageManagerAsync & localStorageManagerAsync);

    [[nodiscard]] const NotebookModel * notebookModel() const;

private Q_SLOTS:
    void onUpdateNotebookCompleted(
        qevercloud::Notebook notebook, QUuid requestId);

    void onExpungeNotebookCompleted(
        qevercloud::Notebook notebook, QUuid requestId);

private:
    QPointer<LocalStorageManagerAsync> m_pLocalStorageManager;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_FILTER_BY_NOTEBOOK_WIDGET_H
