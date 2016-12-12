#include "NotebookItemView.h"
#include "../models/NotebookModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <QMenu>
#include <QContextMenuEvent>

#define LAST_SELECTED_NOTEBOOK_KEY QStringLiteral("LastSelectedNotebookLocalUid")

#define REPORT_ERROR(error) \
    { \
        QNLocalizedString errorDescription = QNLocalizedString(error, this); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

NotebookItemView::NotebookItemView(QWidget * parent) :
    ItemView(parent),
    m_pNotebookItemContextMenu(Q_NULLPTR),
    m_pNotebookStackItemContextMenu(Q_NULLPTR)
{}

void NotebookItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("NotebookItemView::setModel"));

    NotebookModel * pPreviousModel = qobject_cast<NotebookModel*>(model());
    if (pPreviousModel) {
        QObject::disconnect(pPreviousModel, QNSIGNAL(NotebookModel,notifyError,QNLocalizedString),
                            this, QNSIGNAL(NotebookItemView,notifyError,QNLocalizedString));
        QObject::disconnect(pPreviousModel, QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                            this, QNSLOT(NotebookItemView,onAllNotebooksListed));
    }

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(pModel);
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model has been set to the notebook view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pNotebookModel, QNSIGNAL(NotebookModel,notifyError,QNLocalizedString),
                     this, QNSLOT(NotebookItemView,notifyError,QNLocalizedString));

    ItemView::setModel(pModel);

    if (pNotebookModel->allNotebooksListed()) {
        QNDEBUG(QStringLiteral("All notebooks are already listed within the model, "
                               "need to select the appropriate one"));
        selectLastUsedOrDefaultNotebook(*pNotebookModel);
        return;
    }

    QObject::connect(pNotebookModel, QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                     this, QNSLOT(NotebookItemView,onAllNotebooksListed));
}

void NotebookItemView::deleteSelectedItem()
{
    QNDEBUG(QStringLiteral("NotebookItemView::deleteSelectedItem"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(QStringLiteral("No notebooks are selected, nothing to deete"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current notebook"),
                                       tr("No notebook is selected currently"),
                                       tr("Please select the notebook you want to delete")))
        return;
    }

    QModelIndex index = singleRow(indexes, *pNotebookModel);
    if (!index.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one notebook within the selection"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current notebook"),
                                       tr("More than one notebook is currently selected"),
                                       tr("Please select only the notebook you want to delete")))
        return;
    }

    deleteItem(index, *pNotebookModel);
}

void NotebookItemView::onAllNotebooksListed()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onAllNotebooksListed"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        REPORT_ERROR("Can't cast the model set to the notebook item view to the notebook model")
        return;
    }

    QObject::disconnect(pNotebookModel, QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                        this, QNSLOT(NotebookItemView,onAllNotebooksListed));

    selectLastUsedOrDefaultNotebook(*pNotebookModel);
}

void NotebookItemView::onCreateNewNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onCreateNewNotebookAction"));
    emit newNotebookCreationRequested();
}

void NotebookItemView::onRenameNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onRenameNotebookAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't rename notebook, can't cast the slot invoker to QAction");
        return;
    }

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't rename notebook, can't get notebook's "
                     "local uid from QAction");
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't rename notebook: the model returned invalid "
                     "index for the notebook's local uid");
        return;
    }

    edit(itemIndex);
}

void NotebookItemView::onDeleteNotebookAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onDeleteNotebookAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't delete notebook, can't cast the slot invoker to QAction");
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't delete notebook, can't get notebook's "
                     "local uid from QAction");
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't delete notebook: the model returned invalid "
                     "index for the notebook's local uid");
        return;
    }

    deleteItem(itemIndex, *pNotebookModel);
}

void NotebookItemView::onSetNotebookDefaultAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onSetNotebookDefaultAction"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't set notebook as default, can't cast the slot invoker to QAction");
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't delete notebook, can't get notebook's "
                     "local uid from QAction");
        return;
    }

    QModelIndex itemIndex = pNotebookModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR("Internal error: can't delete notebook: the model returned invalid "
                     "index for the notebook's local uid");
        return;
    }

    itemIndex = pNotebookModel->index(itemIndex.row(), NotebookModel::Columns::Default,
                                      itemIndex.parent());
    bool res = pNotebookModel->setData(itemIndex, true);
    if (res) {
        QNDEBUG(QStringLiteral("Successfully set the notebook as default"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to set the notebook as default; "
                                              "Check the status bar for message from the notebook model "
                                              "explaining why the action was not successful")));
}

void NotebookItemView::onRenameNotebookStackAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onRenameNotebookStackAction"));

    // TODO: implement
}

void NotebookItemView::onDeleteNotebookStackAction()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onDeleteNotebookStackAction"));

    // TODO: implement
}

void NotebookItemView::selectionChanged(const QItemSelection & selected,
                                        const QItemSelection & deselected)
{
    QNDEBUG(QStringLiteral("NotebookItemView::selectionChanged"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used"));
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    QModelIndexList selectedIndexes = selected.indexes();

    if (selectedIndexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The new selection is empty"));
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    QModelIndex sourceIndex = singleRow(selectedIndexes, *pNotebookModel);
    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pModelItem))
    {
        REPORT_ERROR("Internal error: can't find the notebook model item corresponging "
                     "to the selected index");
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    if (pModelItem->type() != NotebookModelItem::Type::Notebook) {
        QNDEBUG(QStringLiteral("Non-notebook item is selected"));
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    const NotebookItem * pNotebookItem = pModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem)) {
        REPORT_ERROR("Internal error: selected notebook model item seems to point "
                     "to notebook (not to stack) but returns null pointer to "
                     "the notebook item");
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));
    appSettings.setValue(LAST_SELECTED_NOTEBOOK_KEY, pNotebookItem->localUid());
    appSettings.endGroup();

    ItemView::selectionChanged(selected, deselected);
}

void NotebookItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("NotebookItemView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: notebook item view received context menu event with null pointer to "
                                 "the context menu event"));
        return;
    }

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("Non-notebook model is used, not doing anything"));
        return;
    }

    QModelIndex clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR("Can't show the context menu for the notebook model item: "
                     "no item corresponding to the clicked item's index")
        return;
    }

    if (pModelItem->type() == NotebookModelItem::Type::Notebook)
    {
        const NotebookItem * pNotebookItem = pModelItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem)) {
            REPORT_ERROR("Can't show the context menu for the notebook item: "
                         "the model item reported that it points to the notebook item "
                         "but the pointer to the notebook item is null")
            return;
        }

        showNotebookItemContextMenu(*pNotebookItem, pEvent->globalPos(), *pNotebookModel);
    }
    else if (pModelItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * pNotebookStackItem = pModelItem->notebookStackItem();
        if (Q_UNLIKELY(!pNotebookStackItem)) {
            REPORT_ERROR("Can't show the context menu for the notebook stack item: "
                         "the model item reported that it points to the notebook stack item "
                         "but the pointer to the notebook stack item is null")
            return;
        }

        showNotebookStackItemContextMenu(*pNotebookStackItem, pEvent->globalPos(), *pNotebookModel);
    }
}

void NotebookItemView::deleteItem(const QModelIndex & itemIndex,
                                  NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::deleteItem"));

    const NotebookModelItem * pModelItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR("Internal error: can't find the notebook model item meant to be deleted");
        return;
    }

    if (pModelItem->type() == NotebookModelItem::Type::Notebook)
    {
        const NotebookItem * pNotebookItem = pModelItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem)) {
            REPORT_ERROR("Internal error: can't delete notebook: "
                         "the model item reported that it points to the notebook item "
                         "but the pointer to the notebook item is null")
            return;
        }

        int confirm = warningMessageBox(this, tr("Confirm the notebook deletion"),
                                        tr("Are you sure you want to delete the notebook?"),
                                        tr("Note that this action is not reversible and the removal "
                                           "of the notebook would also mean the removal of all the notes "
                                           "stored inside it!"), QMessageBox::Ok | QMessageBox::No);
        if (confirm != QMessageBox::Ok) {
            QNDEBUG(QStringLiteral("Notebook removal was not confirmed"));
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG(QStringLiteral("Successfully removed notebook"));
            return;
        }

        Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to delete the notebook; "
                                                  "Check the status bar for message from the notebook model "
                                                  "explaining why the notebook could not be removed")));
    }
    else if (pModelItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * pNotebookStackItem = pModelItem->notebookStackItem();
        if (Q_UNLIKELY(!pNotebookStackItem)) {
            REPORT_ERROR("Internal error: can't delete notebook stack: "
                         "the model item reported that it points to the notebook stack "
                         "but the pointer to the notebook stack item is null")
            return;
        }

        int confirm = warningMessageBox(this, tr("Confirm the notebook stack deletion"),
                                        tr("Are you sure you want to delete the whole stack of notebooks?"),
                                        tr("Note that this action is not reversible and the removal "
                                           "of the whole stack of notebooks would also mean "
                                           "the removal of all the notes stored inside the notebooks!"),
                                         QMessageBox::Ok | QMessageBox::No);
        if (confirm != QMessageBox::Ok) {
            QNDEBUG(QStringLiteral("Notebook stack removal was not confirmed"));
            return;
        }

        bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
        if (res) {
            QNDEBUG(QStringLiteral("Successfully removed notebook stack"));
            return;
        }

        Q_UNUSED(internalErrorMessageBox(this, tr("The notebook model refused to delete the notebook stack; "
                                                  "Check the status bar for message from the notebook model "
                                                  "explaining why the notebook stack could not be removed")));
    }
    else
    {
        REPORT_ERROR("Internal error: can't delete notebook(s): found notebook model item "
                     "of unknown type");
        QNWARNING(*pModelItem);
    }
}

void NotebookItemView::showNotebookItemContextMenu(const NotebookItem & item,
                                                   const QPoint & point,
                                                   NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::showNotebookItemContextMenu: (")
            << point.x() << QStringLiteral(", ") << point.y()
            << QStringLiteral("), item: ") << item);

    delete m_pNotebookItemContextMenu;
    m_pNotebookItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, slot, enabled) \
    { \
        QAction * action = new QAction(name, m_pNotebookItemContextMenu); \
        action->setData(item.localUid()); \
        action->setEnabled(enabled); \
        QObject::connect(action, QNSIGNAL(QAction,triggered), this, QNSLOT(NotebookItemView,slot)); \
        m_pNotebookItemContextMenu->addAction(action); \
    }

    ADD_CONTEXT_MENU_ACTION(tr("Create new notebook") + QStringLiteral("..."),
                            onCreateNewNotebookAction, true);

    m_pNotebookItemContextMenu->addSeparator();

    bool canRename = (item.isUpdatable() && item.nameIsUpdatable());
    ADD_CONTEXT_MENU_ACTION(tr("Rename") + QStringLiteral("..."),
                            onRenameNotebookAction, canRename);

    if (model.account().type() == Account::Type::Local) {
        ADD_CONTEXT_MENU_ACTION(tr("Delete"), onDeleteNotebookAction, true);
    }

    m_pNotebookItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Set default"), onSetNotebookDefaultAction, true);

#undef ADD_CONTEXT_MENU_ACTION

    m_pNotebookItemContextMenu->show();
    m_pNotebookItemContextMenu->exec(point);
}

void NotebookItemView::showNotebookStackItemContextMenu(const NotebookStackItem & item,
                                                        const QPoint & point,
                                                        NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::showNotebookStackItemContextMenu: (")
            << point.x() << QStringLiteral(", ") << point.y()
            << QStringLiteral(", item: ") << item);

    // TODO: implement
    Q_UNUSED(model)
}

QModelIndex NotebookItemView::singleRow(const QModelIndexList & indexes,
                                        const NotebookModel & model) const
{
    int row = -1;
    QModelIndex sourceIndex;
    for(auto it = indexes.constBegin(), end = indexes.constEnd(); it != end; ++it)
    {
        const QModelIndex & index = *it;
        if (Q_UNLIKELY(!index.isValid())) {
            continue;
        }

        if (row < 0) {
            row = index.row();
            sourceIndex = index;
            continue;
        }

        if (row != index.row()) {
            sourceIndex = QModelIndex();
            break;
        }
    }

    if (!sourceIndex.isValid()) {
        return sourceIndex;
    }

    return model.index(sourceIndex.row(), NotebookModel::Columns::Name, sourceIndex.parent());
}

void NotebookItemView::selectLastUsedOrDefaultNotebook(const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::selectLastUsedOrDefaultNotebook"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR("Can't select last used or default notebook: "
                     "no selection model in the view");
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));
    QString lastSelectedNotebookLocalUid = appSettings.value(LAST_SELECTED_NOTEBOOK_KEY).toString();
    appSettings.endGroup();

    QNTRACE(QStringLiteral("Last selected notebook local uid: ") << lastSelectedNotebookLocalUid);

    if (!lastSelectedNotebookLocalUid.isEmpty())
    {
        QModelIndex lastSelectedNotebookIndex = model.indexForLocalUid(lastSelectedNotebookLocalUid);
        if (lastSelectedNotebookIndex.isValid()) {
            QNDEBUG(QStringLiteral("Selecting the last selected notebook item: local uid = ")
                    << lastSelectedNotebookLocalUid);
            QModelIndex left = model.index(lastSelectedNotebookIndex.row(), NotebookModel::Columns::Name);
            QModelIndex right = model.index(lastSelectedNotebookIndex.row(), NotebookModel::Columns::NumNotesPerNotebook);
            QItemSelection selection(left, right);
            pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }

    QModelIndex defaultNotebookIndex = model.defaultNotebookIndex();
    if (defaultNotebookIndex.isValid()) {
        QNDEBUG(QStringLiteral("Selecting the default notebook item"));
        QModelIndex left = model.index(defaultNotebookIndex.row(), NotebookModel::Columns::Name);
        QModelIndex right = model.index(defaultNotebookIndex.row(), NotebookModel::Columns::NumNotesPerNotebook);
        QItemSelection selection(left, right);
        pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
        return;
    }
}

} // namespace quentier
