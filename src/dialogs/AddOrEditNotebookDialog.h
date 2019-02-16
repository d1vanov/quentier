/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H
#define QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H

#include "../models/NotebookModel.h"
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
    StringUtils                     m_stringUtils;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_NOTEBOOK_DIALOG_H
