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

#include "FilterByNotebookWidget.h"

#include <lib/model/notebook/NotebookModel.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterByNotebookWidget::FilterByNotebookWidget(QWidget * parent) :
    AbstractFilterByModelItemWidget(QStringLiteral("Notebook"), parent),
    m_pLocalStorageManager()
{}

void FilterByNotebookWidget::setLocalStorageManager(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    m_pLocalStorageManager = &localStorageManagerAsync;

    QObject::connect(m_pLocalStorageManager.data(),
                     QNSIGNAL(LocalStorageManagerAsync,updateNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(FilterByNotebookWidget,onUpdateNotebookCompleted,
                            Notebook,QUuid));
    QObject::connect(m_pLocalStorageManager.data(),
                     QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,
                              Notebook,QUuid),
                     this,
                     QNSLOT(FilterByNotebookWidget,onExpungeNotebookCompleted,
                            Notebook,QUuid));
}

const NotebookModel * FilterByNotebookWidget::notebookModel() const
{
    const ItemModel * pItemModel = model();
    if (!pItemModel) {
        return nullptr;
    }

    return qobject_cast<const NotebookModel*>(pItemModel);
}

void FilterByNotebookWidget::onUpdateNotebookCompleted(
    Notebook notebook, QUuid requestId)
{
    QNDEBUG("FilterByNotebookWidget::onUpdateNotebookCompleted: request id = "
            << requestId << ", notebook = " << notebook);

    if (Q_UNLIKELY(!notebook.hasName())) {
        QNWARNING("Found notebook without a name: " << notebook);
        onItemRemovedFromLocalStorage(notebook.localUid());
        return;
    }

    onItemUpdatedInLocalStorage(notebook.localUid(), notebook.name());
}

void FilterByNotebookWidget::onExpungeNotebookCompleted(
    Notebook notebook, QUuid requestId)
{
    QNDEBUG("FilterByNotebookWidget::onExpungeNotebookCompleted: request id = "
            << requestId << ", notebook = " << notebook);

    onItemRemovedFromLocalStorage(notebook.localUid());
}

} // namespace quentier
