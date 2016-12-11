#ifndef QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H
#define QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H

#include "ItemView.h"
#include <quentier/utility/QNLocalizedString.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookModel)

class NotebookItemView: public ItemView
{
    Q_OBJECT
public:
    explicit NotebookItemView(QWidget * parent = Q_NULLPTR);

    virtual void setModel(QAbstractItemModel * pModel) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(QNLocalizedString error);

private Q_SLOTS:
    void onAllNotebooksListed();
    virtual void selectionChanged(const QItemSelection & selected,
                                  const QItemSelection & deselected) Q_DECL_OVERRIDE;

private:
    void selectLastUsedOrDefaultNotebook(const NotebookModel & model);
};

} // namespace quentier

#endif // QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H
