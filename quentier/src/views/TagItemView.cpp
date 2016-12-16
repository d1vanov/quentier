/*
 * Copyright 2016 Dmitry Ivanov
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

#include "TagItemView.h"
#include "AccountToKey.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScopedPointer>

#ifndef QT_MOC_RUN
#include <boost/scope_exit.hpp>
#endif

#define LAST_SELECTED_TAG_KEY QStringLiteral("_LastSelectedTagLocalUid")

#define REPORT_ERROR(error) \
    { \
        QNLocalizedString errorDescription = QNLocalizedString(error, this); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

TagItemView::TagItemView(QWidget * parent) :
    ItemView(parent),
    m_pTagItemContextMenu(Q_NULLPTR),
    m_restoringTagItemsState(false)
{
    QObject::connect(this, QNSIGNAL(TagItemView,expanded,const QModelIndex&),
                     this, QNSLOT(TagItemView,onTagItemCollapsedOrExpanded,const QModelIndex&));
    QObject::connect(this, QNSIGNAL(TagItemView,collapsed,const QModelIndex&),
                     this, QNSLOT(TagItemView,onTagItemCollapsedOrExpanded,const QModelIndex&));
}

void TagItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("TagItemView::setModel"));

    TagModel * pPreviousModel = qobject_cast<TagModel*>(model());
    if (pPreviousModel) {
        QObject::disconnect(pPreviousModel, QNSIGNAL(TagModel,notifyError,QNLocalizedString),
                            this, QNSIGNAL(TagItemView,notifyError,QNLocalizedString));
    }

    TagModel * pTagModel = qobject_cast<TagModel*>(pModel);
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model has been set to the tag view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pTagModel, QNSIGNAL(TagModel,notifyError,QNLocalizedString),
                     this, QNSIGNAL(TagItemView,notifyError,QNLocalizedString));

    ItemView::setModel(pModel);

    if (pTagModel->allTagsListed()) {
        QNDEBUG(QStringLiteral("All tags are already listed within the model"));
        restoreTagItemsState(*pTagModel);
        // TODO: restore the last saved selection, if any
        return;
    }

    QObject::connect(pTagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                     this, QNSLOT(TagItemView,onAllTagsListed));
}

QModelIndex TagItemView::currentlySelectedItemIndex() const
{
    QNDEBUG(QStringLiteral("TagItemView::currentlySelectedItemIndex"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return QModelIndex();
    }

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG(QStringLiteral("No selection model in the view"));
        return QModelIndex();
    }

    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The selection contains no model indexes"));
        return QModelIndex();
    }

    return singleRow(indexes, *pTagModel, TagModel::Columns::Name);
}

void TagItemView::onAllTagsListed()
{
    QNDEBUG(QStringLiteral("TagItemView::onAllTagsListed"));

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        REPORT_ERROR("Can't cast the model set to the tag item view "
                     "to the tag model");
        return;
    }

    QObject::disconnect(pTagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                        this, QNSLOT(TagItemView,onAllTagsListed));

    restoreTagItemsState(*pTagModel);
    // TODO: restore the last saved selection, if any
}

void TagItemView::onTagItemCollapsedOrExpanded(const QModelIndex & index)
{
    QNTRACE(QStringLiteral("TagItemView::onNotebookStackItemCollapsedOrExpanded: index: valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row() << QStringLiteral(", column = ") << index.column()
            << QStringLiteral(", parent: valid = ") << (index.parent().isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.parent().row() << QStringLiteral(", column = ") << index.parent().column());

    if (m_restoringTagItemsState) {
        QNDEBUG(QStringLiteral("Ignoring the event as the tag items are being restored currently"));
        return;
    }

    saveTagItemsState();
}

void TagItemView::selectionChanged(const QItemSelection & selected,
                                   const QItemSelection & deselected)
{
    QNDEBUG(QStringLiteral("TagItemView::selectionChanged"));

    BOOST_SCOPE_EXIT(this_, &selected, &deselected) {
        this_->ItemView::selectionChanged(selected, deselected);
    } BOOST_SCOPE_EXIT_END

    TagModel * pTagModel = qobject_cast<TagModel*>(model());
    if (Q_UNLIKELY(!pTagModel)) {
        QNDEBUG(QStringLiteral("Non-tag model is used"));
        return;
    }

    QModelIndexList selectedIndexes = selected.indexes();

    if (selectedIndexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The new selection is empty"));
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    QModelIndex sourceIndex = singleRow(selectedIndexes, *pTagModel, TagModel::Columns::Name);
    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        return;
    }

    const TagModelItem * pTagItem = pTagModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR("Internal error: can't find the tag model item corresponding "
                     "to the selected index");
        return;
    }

    QNTRACE(QStringLiteral("Currently selected tag item: ") << *pTagItem);

    QString accountKey = accountToKey(pTagModel->account());
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        REPORT_ERROR("Internal error: can't create application settings key from account");
        QNWARNING(pTagModel->account());
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/TagItemView"));
    appSettings.setValue(LAST_SELECTED_TAG_KEY, pTagItem->localUid());
    appSettings.endGroup();

    QNDEBUG(QStringLiteral("Persisted the currently selected tag local uid: ")
            << pTagItem->localUid());
}

void TagItemView::saveTagItemsState()
{
    QNDEBUG(QStringLiteral("TagItemView::saveTagItemsState"));

    // TODO: implement
}

void TagItemView::restoreTagItemsState(const TagModel & model)
{
    QNDEBUG(QStringLiteral("TagItemView::restoreTagItemsState"));

    // TODO: implement
    Q_UNUSED(model)
}

} // namespace quentier
