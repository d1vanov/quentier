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

#include "FilterByTagWidget.h"
#include "../models/TagModel.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterByTagWidget::FilterByTagWidget(QWidget * parent) :
    AbstractFilterByModelItemWidget(QStringLiteral("Tag"), parent),
    m_pLocalStorageManager()
{}

void FilterByTagWidget::setLocalStorageManager(LocalStorageManagerThreadWorker & localStorageManager)
{
    m_pLocalStorageManager = &localStorageManager;

    QObject::connect(m_pLocalStorageManager.data(), QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(FilterByTagWidget,onUpdateTagCompleted,Tag,QUuid));
    QObject::connect(m_pLocalStorageManager.data(), QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(FilterByTagWidget,onExpungeTagCompleted,Tag,QUuid));
    QObject::connect(m_pLocalStorageManager.data(), QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotelessTagsFromLinkedNotebooksComplete,QUuid),
                     this, QNSLOT(FilterByTagWidget,onExpungeNotelessTagsFromLinkedNotebooksCompleted,QUuid));
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
    QNDEBUG(QStringLiteral("FilterByTagWidget::onUpdateTagCompleted: request id = ")
            << requestId << QStringLiteral(", tag = ") << tag);

    if (Q_UNLIKELY(!tag.hasName())) {
        QNWARNING(QStringLiteral("Found tag without a name: ") << tag);
        onItemRemovedFromLocalStorage(tag.localUid());
        return;
    }

    onItemUpdatedInLocalStorage(tag.localUid(), tag.name());
}

void FilterByTagWidget::onExpungeTagCompleted(Tag tag, QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByTagWidget::onExpungeTagCompleted: request id = ")
            << requestId << QStringLiteral(", tag = ") << tag);

    onItemRemovedFromLocalStorage(tag.localUid());
}

void FilterByTagWidget::onExpungeNotelessTagsFromLinkedNotebooksCompleted(QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByTagWidget::onExpungeNotelessTagsFromLinkedNotebooksCompleted: request id = ")
            << requestId);

    update();
}

} // namespace quentier
