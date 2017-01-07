/*
 * Copyright 2017 Dmitry Ivanov
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
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterByNotebookWidget::FilterByNotebookWidget(QWidget * parent) :
    AbstractFilterByModelItemWidget(QStringLiteral("Notebook"), parent),
    m_pLocalStorageManager()
{}

void FilterByNotebookWidget::setLocalStorageManager(LocalStorageManagerThreadWorker & localStorageManager)
{
    m_pLocalStorageManager = &localStorageManager;

    QObject::connect(m_pLocalStorageManager.data(), QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FilterByNotebookWidget,onUpdateNotebookCompleted,Notebook,QUuid));
    QObject::connect(m_pLocalStorageManager.data(), QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FilterByNotebookWidget,onExpungeNotebookCompleted,Notebook,QUuid));
}

void FilterByNotebookWidget::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByNotebookWidget::onUpdateNotebookCompleted: request id = ")
            << requestId << QStringLiteral(", notebook = ") << notebook);

    if (Q_UNLIKELY(!notebook.hasName())) {
        QNWARNING(QStringLiteral("Found notebook without a name: ") << notebook);
        onItemRemovedFromLocalStorage(notebook.localUid());
        return;
    }

    onItemUpdatedInLocalStorage(notebook.localUid(), notebook.name());
}

void FilterByNotebookWidget::onExpungeNotebookCompleted(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByNotebookWidget::onExpungeNotebookCompleted: request id = ")
            << requestId << QStringLiteral(", notebook = ") << notebook);

    onItemRemovedFromLocalStorage(notebook.localUid());
}

} // namespace quentier
