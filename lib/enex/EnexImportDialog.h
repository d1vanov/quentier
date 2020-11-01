/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DIALOG_ENEX_IMPORT_DIALOG_H
#define QUENTIER_LIB_DIALOG_ENEX_IMPORT_DIALOG_H

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

class EnexImportDialog final: public QDialog
{
    Q_OBJECT

public:
    explicit EnexImportDialog(
        const Account & account, NotebookModel & notebookModel,
        QWidget * parent = nullptr);

    virtual ~EnexImportDialog();

    QString importEnexFilePath(ErrorString * pErrorDescription = nullptr) const;
    QString notebookName(ErrorString * pErrorDescription = nullptr) const;

private Q_SLOTS:
    void onBrowsePushButtonClicked();
    void onNotebookIndexChanged(int notebookNameIndex);
    void onNotebookNameEdited(const QString & name);
    void onEnexFilePathEdited(const QString & path);

    // Slots to track the updates of notebook model
    void dataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = QVector<int>());

    void rowsInserted(const QModelIndex & parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);

    virtual void accept() override;

private:
    void createConnections();
    void fillNotebookNames();
    void fillDialogContents();

    void setStatusText(const QString & text);
    void clearAndHideStatus();

    void checkConditionsAndEnableDisableOkButton();

private:
    Ui::EnexImportDialog * m_pUi;
    Account m_currentAccount;
    QPointer<NotebookModel> m_pNotebookModel;
    QStringListModel * m_pNotebookNamesModel;
};

} // namespace quentier

#endif // QUENTIER_LIB_DIALOG_ENEX_IMPORT_DIALOG_H
