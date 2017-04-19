#ifndef QUENTIER_DIALOGS_ENEX_IMPORT_DIALOG_H
#define QUENTIER_DIALOGS_ENEX_IMPORT_DIALOG_H

#include <quentier/utility/Macros.h>
#include <quentier/types/Account.h>
#include <QDialog>
#include <QPointer>

namespace Ui {
class EnexImportDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ErrorString)
QT_FORWARD_DECLARE_CLASS(NotebookModel)

class EnexImportDialog: public QDialog
{
    Q_OBJECT

public:
    explicit EnexImportDialog(const Account & account,
                              NotebookModel & notebookModel,
                              QWidget * parent = Q_NULLPTR);
    ~EnexImportDialog();

    QString importEnexFilePath(ErrorString * pErrorDescription = Q_NULLPTR) const;
    QString notebookName(ErrorString * pErrorDescription = Q_NULLPTR) const;

private Q_SLOTS:
    void onBrowsePushButtonClicked();
    void onNotebookNameEdited(const QString & name);
    void onEnexFilePathEdited(const QString & path);

    // Slots to track the updates of notebook model
    void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            );
#else
                            , const QVector<int> & roles = QVector<int>());
#endif

    void rowsInserted(const QModelIndex & parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);

private:
    void createConnections();
    void fillNotebookNames();
    void fillDialogContents();

    void setStatusText(const QString & text);
    void clearAndHideStatus();

    void checkConditionsAndEnableDisableOkButton();

private:
    Ui::EnexImportDialog *  m_pUi;
    Account                 m_currentAccount;
    QPointer<NotebookModel> m_pNotebookModel;
    QStringListModel *      m_pNotebookNamesModel;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ENEX_IMPORT_DIALOG_H
