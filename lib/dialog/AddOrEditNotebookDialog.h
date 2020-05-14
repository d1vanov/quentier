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

#ifndef QUENTIER_LIB_DIALOG_ADD_OR_EDIT_NOTEBOOK_DIALOG_H
#define QUENTIER_LIB_DIALOG_ADD_OR_EDIT_NOTEBOOK_DIALOG_H

#include <lib/model/notebook/NotebookModel.h>

#include <quentier/utility/Macros.h>
#include <quentier/utility/StringUtils.h>

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
    explicit AddOrEditNotebookDialog(
        NotebookModel * pNotebookModel,
        QWidget * parent = nullptr,
        const QString & editedNotebookLocalUid = {});

    ~AddOrEditNotebookDialog();

private Q_SLOTS:
    virtual void accept() override;

    void onNotebookNameEdited(const QString & notebookName);

    void onNotebookStackChanged(const QString & stack);

private:
    void createConnections();

private:
    Ui::AddOrEditNotebookDialog *   m_pUi;
    QPointer<NotebookModel>         m_pNotebookModel;
    QStringListModel *              m_pNotebookStacksModel = nullptr;
    QString                         m_editedNotebookLocalUid;
    StringUtils                     m_stringUtils;
};

} // namespace quentier

#endif // QUENTIER_LIB_DIALOG_ADD_OR_EDIT_NOTEBOOK_DIALOG_H
