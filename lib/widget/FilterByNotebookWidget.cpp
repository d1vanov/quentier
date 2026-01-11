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

#include "FilterByNotebookWidget.h"

#include <lib/model/notebook/NotebookModel.h>

#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>

#include <qevercloud/types/Notebook.h>

namespace quentier {

FilterByNotebookWidget::FilterByNotebookWidget(QWidget * parent) :
    AbstractFilterByModelItemWidget{QStringLiteral("Notebook"), parent}
{}

FilterByNotebookWidget::~FilterByNotebookWidget() = default;

void FilterByNotebookWidget::setLocalStorage(
    const local_storage::ILocalStorage & localStorage)
{
    auto * notifier = localStorage.notifier();

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookPut, this,
        &FilterByNotebookWidget::onNotebookPut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookExpunged, this,
        &FilterByNotebookWidget::onNotebookExpunged);
}

const NotebookModel * FilterByNotebookWidget::notebookModel() const
{
    const auto * itemModel = model();
    if (!itemModel) {
        return nullptr;
    }

    return qobject_cast<const NotebookModel *>(itemModel);
}

void FilterByNotebookWidget::onNotebookPut(
    const qevercloud::Notebook & notebook)
{
    QNDEBUG(
        "widget::FilterByNotebookWidget",
        "FilterByNotebookWidget::onNotebookPut: notebook = " << notebook);

    if (Q_UNLIKELY(!notebook.name())) {
        QNWARNING(
            "widget::FilterByNotebookWidget",
            "Found notebook without a name: " << notebook);
        onItemRemovedFromLocalStorage(notebook.localId());
        return;
    }

    onItemUpdatedInLocalStorage(notebook.localId(), *notebook.name());
}

void FilterByNotebookWidget::onNotebookExpunged(const QString & notebookLocalId)
{
    QNDEBUG(
        "widget::FilterByNotebookWidget",
        "FilterByNotebookWidget::onNotebookExpunged: local id = "
            << notebookLocalId);

    onItemRemovedFromLocalStorage(notebookLocalId);
}

} // namespace quentier
