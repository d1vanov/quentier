/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#include <lib/model/TagModel.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterByTagWidget::FilterByTagWidget(QWidget * parent) :
    AbstractFilterByModelItemWidget(QStringLiteral("Tag"), parent),
    m_pLocalStorageManager()
{}

void FilterByTagWidget::setLocalStorageManager(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    m_pLocalStorageManager = &localStorageManagerAsync;

    QObject::connect(m_pLocalStorageManager.data(),
                     QNSIGNAL(LocalStorageManagerAsync,updateTagComplete,
                              Tag,QUuid),
                     this,
                     QNSLOT(FilterByTagWidget,onUpdateTagCompleted,Tag,QUuid));
    QObject::connect(m_pLocalStorageManager.data(),
                     QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,
                              Tag,QStringList,QUuid),
                     this,
                     QNSLOT(FilterByTagWidget,onExpungeTagCompleted,
                            Tag,QStringList,QUuid));
    QObject::connect(m_pLocalStorageManager.data(),
                     QNSIGNAL(LocalStorageManagerAsync,
                              expungeNotelessTagsFromLinkedNotebooksComplete,
                              QUuid),
                     this,
                     QNSLOT(FilterByTagWidget,
                            onExpungeNotelessTagsFromLinkedNotebooksCompleted,
                            QUuid));
}

const TagModel * FilterByTagWidget::tagModel() const
{
    const ItemModel * pItemModel = model();
    if (!pItemModel) {
        return Q_NULLPTR;
    }

    return qobject_cast<const TagModel*>(pItemModel);
}

void FilterByTagWidget::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    QNDEBUG("FilterByTagWidget::onUpdateTagCompleted: request id = "
            << requestId << ", tag = " << tag);

    if (Q_UNLIKELY(!tag.hasName())) {
        QNWARNING("Found tag without a name: " << tag);
        onItemRemovedFromLocalStorage(tag.localUid());
        return;
    }

    onItemUpdatedInLocalStorage(tag.localUid(), tag.name());
}

void FilterByTagWidget::onExpungeTagCompleted(
    Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNDEBUG("FilterByTagWidget::onExpungeTagCompleted: request id = "
            << requestId << ", tag = " << tag
            << "\nExpunged child tag local uids: "
            << expungedChildTagLocalUids.join(QStringLiteral(", ")));

    QStringList expungedTagLocalUids;
    expungedTagLocalUids << tag.localUid();
    expungedTagLocalUids << expungedChildTagLocalUids;
    for(auto it = expungedTagLocalUids.constBegin(),
        end = expungedTagLocalUids.constEnd(); it != end; ++it)
    {
        onItemRemovedFromLocalStorage(*it);
    }
}

void FilterByTagWidget::onExpungeNotelessTagsFromLinkedNotebooksCompleted(
    QUuid requestId)
{
    QNDEBUG("FilterByTagWidget::"
            << "onExpungeNotelessTagsFromLinkedNotebooksCompleted: request id = "
            << requestId);

    update();
}

} // namespace quentier
