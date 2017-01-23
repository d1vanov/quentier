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

#include "NoteListView.h"
#include "../models/NoteFilterModel.h"
#include "../models/NoteModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QContextMenuEvent>
#include <QMenu>

namespace quentier {

NoteListView::NoteListView(QWidget * parent) :
    QListView(parent),
    m_pNoteItemContextMenu(Q_NULLPTR)
{}

void NoteListView::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                               )
#else
                               , const QVector<int> & roles)
#endif
{
    QListView::dataChanged(topLeft, bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                               );
#else
                               , roles);
#endif
}

void NoteListView::onCreateNewNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onCreateNewNoteAction"));

    // TODO: implement
}

void NoteListView::onDeleteNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onDeleteNoteAction"));

    // TODO: implement
}

void NoteListView::onEditNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onEditNoteAction"));

    // TODO: implement
}

void NoteListView::onUnfavoriteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onUnfavoriteAction"));

    // TODO: implement
}

void NoteListView::onFavoriteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onFavoriteAction"));

    // TODO: implement
}

void NoteListView::onShowNoteInfoAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onShowNoteInfoAction"));

    // TODO: implement
}

void NoteListView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("NoteListView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: note list view received "
                                 "context menu event with null pointer "
                                 "to the context menu event"));
        return;
    }

    const NoteFilterModel * pNoteFilterModel = qobject_cast<const NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNLocalizedString error = QNLocalizedString("Wrong model connected to the note list view", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNLocalizedString error = QNLocalizedString("Can't get the source model from the note filter model "
                                                    "connected to the note list view", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QModelIndex clickedFilterModelItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedFilterModelItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    QModelIndex clickedItemIndex = pNoteFilterModel->mapToSource(clickedFilterModelItemIndex);
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNLocalizedString error = QNLocalizedString("Internal error: note item index mapped from "
                                                    "the proxy index of note filter model is invalid", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    const NoteModelItem * pItem = pNoteModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        QNLocalizedString error = QNLocalizedString("Can't show the context menu for the note model item: "
                                                    "no item corresponding to the clicked item's index", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    delete m_pNoteItemContextMenu;
    m_pNoteItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteListView,slot)); \
        menu->addAction(pAction); \
    }

    // TODO: need to track the current notebook selection and only enable the note creation action
    // if the notebook allows to create notes in it
    ADD_CONTEXT_MENU_ACTION(tr("Create new note"), m_pNoteItemContextMenu,
                            onCreateNewNoteAction, QString(), true);

    m_pNoteItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Delete"), m_pNoteItemContextMenu,
                            onDeleteNoteAction, pItem->localUid(),
                            !pItem->isSynchronizable());

    // TODO: should only enable this action if the currently selected notebook
    // allows notes modification
    bool canUpdate = true;
    ADD_CONTEXT_MENU_ACTION(tr("Edit") + QStringLiteral("..."),
                            m_pNoteItemContextMenu, onEditNoteAction,
                            pItem->localUid(), canUpdate);

    // TODO: add "move to notebook" sub-menu

    if (pItem->isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(tr("Unfavorite"), m_pNoteItemContextMenu,
                                onUnfavoriteAction, pItem->localUid(), true);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(tr("Favorite"), m_pNoteItemContextMenu,
                                onFavoriteAction, pItem->localUid(), true);
    }

    m_pNoteItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pNoteItemContextMenu,
                            onShowNoteInfoAction, pItem->localUid(), true);

    m_pNoteItemContextMenu->show();
    m_pNoteItemContextMenu->exec(pEvent->globalPos());
}

void NoteListView::currentChanged(const QModelIndex & current,
                                  const QModelIndex & previous)
{
    QNTRACE(QStringLiteral("NoteListView::currentChanged"));

    Q_UNUSED(previous)

    if (!current.isValid()) {
        QNTRACE(QStringLiteral("Current index is invalid"));
        return;
    }

    const NoteFilterModel * pNoteFilterModel = qobject_cast<const NoteFilterModel*>(current.model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNLocalizedString error = QNLocalizedString("Wrong model connected to the note list view", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNLocalizedString error = QNLocalizedString("Can't get the source model from the note filter model "
                                                    "connected to the note list view", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QModelIndex sourceIndex = pNoteFilterModel->mapToSource(current);
    const NoteModelItem * pItem = pNoteModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pItem)) {
        QNLocalizedString error = QNLocalizedString("Can't retrieve the new current note item for note model index", this);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit currentNoteChanged(pItem->localUid());
}

} // namespace quentier
