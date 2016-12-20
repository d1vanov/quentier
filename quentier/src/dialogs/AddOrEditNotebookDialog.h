#ifndef QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H
#define QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H

#include "../models/NotebookModel.h"
#include <quentier/utility/Qt4Helper.h>
#include <QDialog>
#include <QPointer>

namespace Ui {
class AddOrEditNotebookDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

class AddOrEditNotebookDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditNotebookDialog(NotebookModel * pNotebookModel,
                                     QWidget * parent = Q_NULLPTR,
                                     const QString & editedNotebookLocalUid = QString());
    ~AddOrEditNotebookDialog();

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;
    void onNotebookNameEdited(const QString & notebookName);
    void onNotebookStackChanged(const QString & stack);

private:
    void createConnections();

private:
    Ui::AddOrEditNotebookDialog *   m_pUi;
    QPointer<NotebookModel>         m_pNotebookModel;
    QStringListModel *              m_pNotebookStacksModel;
    QString                         m_editedNotebookLocalUid;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H
