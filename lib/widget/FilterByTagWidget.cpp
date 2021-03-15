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

#include "FilterByTagWidget.h"

#include <lib/model/tag/TagModel.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterByTagWidget::FilterByTagWidget(QWidget * parent) :
    AbstractFilterByModelItemWidget(QStringLiteral("Tag"), parent)
{}

FilterByTagWidget::~FilterByTagWidget() = default;

void FilterByTagWidget::setLocalStorageManager(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    m_pLocalStorageManager = &localStorageManagerAsync;

    QObject::connect(
        m_pLocalStorageManager.data(),
        &LocalStorageManagerAsync::updateTagComplete, this,
        &FilterByTagWidget::onUpdateTagCompleted);

    QObject::connect(
        m_pLocalStorageManager.data(),
        &LocalStorageManagerAsync::expungeTagComplete, this,
        &FilterByTagWidget::onExpungeTagCompleted);

    QObject::connect(
        m_pLocalStorageManager.data(),
        &LocalStorageManagerAsync::
            expungeNotelessTagsFromLinkedNotebooksComplete,
        this,
        &FilterByTagWidget::onExpungeNotelessTagsFromLinkedNotebooksCompleted);
}

const TagModel * FilterByTagWidget::tagModel() const
{
    const auto * pItemModel = model();
    if (!pItemModel) {
        return nullptr;
    }

    return qobject_cast<const TagModel *>(pItemModel);
}

void FilterByTagWidget::onUpdateTagCompleted(
    qevercloud::Tag tag, QUuid requestId) // NOLINT
{
    QNDEBUG(
        "widget:tag_filter",
        "FilterByTagWidget::onUpdateTagCompleted: "
            << "request id = " << requestId << ", tag = " << tag);

    if (Q_UNLIKELY(!tag.name())) {
        QNWARNING("widget:tag_filter", "Found tag without a name: " << tag);
        onItemRemovedFromLocalStorage(tag.localId());
        return;
    }

    onItemUpdatedInLocalStorage(tag.localId(), *tag.name());
}

void FilterByTagWidget::onExpungeTagCompleted(
    qevercloud::Tag tag, QStringList expungedChildTagLocalIds, // NOLINT
    QUuid requestId)
{
    QNDEBUG(
        "widget:tag_filter",
        "FilterByTagWidget::onExpungeTagCompleted: "
            << "request id = " << requestId << ", tag = " << tag
            << "\nExpunged child tag local ids: "
            << expungedChildTagLocalIds.join(QStringLiteral(", ")));

    QStringList expungedTagLocalIds;
    expungedTagLocalIds << tag.localId();
    expungedTagLocalIds << expungedChildTagLocalIds;

    for (const auto & expungedTagLocalId: qAsConst(expungedTagLocalIds)) {
        onItemRemovedFromLocalStorage(expungedTagLocalId);
    }
}

void FilterByTagWidget::onExpungeNotelessTagsFromLinkedNotebooksCompleted(
    QUuid requestId)
{
    QNDEBUG(
        "widget:tag_filter",
        "FilterByTagWidget::onExpungeNotelessTagsFromLinkedNotebooksCompleted: "
            << "request id = " << requestId);

    update();
}

} // namespace quentier
