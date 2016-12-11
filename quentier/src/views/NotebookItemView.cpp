#include "NotebookItemView.h"
#include "../models/NotebookModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#define LAST_SELECTED_NOTEBOOK_KEY QStringLiteral("LastSelectedNotebookLocalUid")

namespace quentier {

NotebookItemView::NotebookItemView(QWidget * parent) :
    ItemView(parent)
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

void NotebookItemView::onAllNotebooksListed()
{
    QNDEBUG(QStringLiteral("NotebookItemView::onAllNotebooksListed"));

    NotebookModel * pNotebookModel = qobject_cast<NotebookModel*>(model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNLocalizedString errorDescription = QNLocalizedString("Can't cast the model "
                                                               "set to the notebook item view "
                                                               "to the notebook model", this);
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QObject::disconnect(pNotebookModel, QNSIGNAL(NotebookModel,notifyAllNotebooksListed),
                        this, QNSLOT(NotebookItemView,onAllNotebooksListed));

    selectLastUsedOrDefaultNotebook(*pNotebookModel);
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
    int row = -1;
    QModelIndex sourceIndex;
    for(auto it = selectedIndexes.constBegin(), end = selectedIndexes.constEnd(); it != end; ++it)
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
            row = -1;
            sourceIndex = QModelIndex();
            break;
        }
    }

    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pModelItem))
    {
        QNLocalizedString errorDescription = QNLocalizedString("Can't find notebook "
                                                               "model item corresponging "
                                                               "to the selected index", this);
        QNWARNING(errorDescription << QStringLiteral(": is valid = ")
                  << (sourceIndex.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
                  << QStringLiteral(", row = ") << sourceIndex.row()
                  << QStringLiteral(", column = ") << sourceIndex.column());
        emit notifyError(errorDescription);
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    if (pModelItem->type() != NotebookModelItem::Type::Notebook) {
        QNDEBUG(QStringLiteral("Non-notebook item is selected"));
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    const NotebookItem * pNotebookItem = pModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem))
    {
        QNLocalizedString errorDescription = QNLocalizedString("Selected notebook model item "
                                                               "seems to point to notebook "
                                                               "but returns null pointer "
                                                               "to the notebook item", this);
        QNWARNING(errorDescription << QStringLiteral(": is valid = ")
                  << (sourceIndex.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
                  << QStringLiteral(", row = ") << sourceIndex.row()
                  << QStringLiteral(", column = ") << sourceIndex.column());
        emit notifyError(errorDescription);
        ItemView::selectionChanged(selected, deselected);
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(QStringLiteral("NotebookItemView"));
    appSettings.setValue(LAST_SELECTED_NOTEBOOK_KEY, pNotebookItem->localUid());
    appSettings.endGroup();
}

void NotebookItemView::selectLastUsedOrDefaultNotebook(const NotebookModel & model)
{
    QNDEBUG(QStringLiteral("NotebookItemView::selectLastUsedOrDefaultNotebook"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNLocalizedString errorDescription = QNLocalizedString("Can't select last used "
                                                               "or default notebook: "
                                                               "no selection model "
                                                               "in the view", this);
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
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
