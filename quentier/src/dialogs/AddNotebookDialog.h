#ifndef QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H
#define QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H

#include "../models/NotebookModel.h"
#include <quentier/utility/Qt4Helper.h>
#include <QDialog>
#include <QPointer>

namespace Ui {
class AddNotebookDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

class AddNotebookDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AddNotebookDialog(NotebookModel * pNotebookModel,
                               QWidget * parent = Q_NULLPTR);
    ~AddNotebookDialog();

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;
    void onNotebookNameEdited(const QString & notebookName);

private:
    void createConnections();

private:
    Ui::AddNotebookDialog * m_pUi;
    QPointer<NotebookModel> m_pNotebookModel;
    QStringListModel *      m_pNotebookStacksModel;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H
