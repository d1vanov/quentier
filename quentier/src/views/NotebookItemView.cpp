#include "NotebookItemView.h"
#include "../models/NotebookModel.h"
#include <quentier/logging/QuentierLogger.h>

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

    QModelIndex defaultNotebookIndex = model.defaultNotebookIndex();
    if (defaultNotebookIndex.isValid()) {
        QNDEBUG(QStringLiteral("Selecting the default notebook item"));
        QModelIndex left = model.index(defaultNotebookIndex.row(), NotebookModel::Columns::Name);
        QModelIndex right = model.index(defaultNotebookIndex.row(), NotebookModel::Columns::NumNotesPerNotebook);
        QItemSelection selection(left, right);
        pSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
        return;
    }

    // TODO: otherwise should attempt to select the last used notebook for this account
}

} // namespace quentier
