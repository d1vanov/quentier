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

#include "FilterByTagWidget.h"

#include <lib/model/tag/TagModel.h>

#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>

#include <qevercloud/types/Tag.h>

#include <utility>

namespace quentier {

FilterByTagWidget::FilterByTagWidget(QWidget * parent) :
    AbstractFilterByModelItemWidget{QStringLiteral("Tag"), parent}
{}

FilterByTagWidget::~FilterByTagWidget() = default;

void FilterByTagWidget::setLocalStorage(
    const local_storage::ILocalStorage & localStorage)
{
    auto * notifier = localStorage.notifier();

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagPut, this,
        &FilterByTagWidget::onTagPut);

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagExpunged, this,
        &FilterByTagWidget::onTagExpunged);
}

const TagModel * FilterByTagWidget::tagModel() const
{
    const auto * itemModel = model();
    if (!itemModel) {
        return nullptr;
    }

    return qobject_cast<const TagModel *>(itemModel);
}

void FilterByTagWidget::onTagPut(const qevercloud::Tag & tag)
{
    QNDEBUG(
        "widget::FilterByTagWidget",
        "FilterByTagWidget::onTagPut: tag = " << tag);

    if (Q_UNLIKELY(!tag.name())) {
        QNWARNING(
            "widget::FilterByTagWidget", "Found tag without a name: " << tag);
        onItemRemovedFromLocalStorage(tag.localId());
        return;
    }

    onItemUpdatedInLocalStorage(tag.localId(), *tag.name());
}

void FilterByTagWidget::onTagExpunged(
    const QString & tagLocalId, const QStringList & expungedChildTagLocalIds)
{
    QNDEBUG(
        "widget::FilterByTagWidget",
        "FilterByTagWidget::onTagExpunged: local id = "
            << tagLocalId << ", expunged child tag local ids: "
            << expungedChildTagLocalIds.join(QStringLiteral(", ")));

    QStringList expungedTagLocalIds;
    expungedTagLocalIds << tagLocalId;
    expungedTagLocalIds << expungedChildTagLocalIds;

    for (const auto & expungedTagLocalId: std::as_const(expungedTagLocalIds)) {
        onItemRemovedFromLocalStorage(expungedTagLocalId);
    }
}

} // namespace quentier
