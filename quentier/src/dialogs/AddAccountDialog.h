/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H
#define QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H

#include <quentier/utility/Qt4Helper.h>
#include <QDialog>

namespace Ui {
class AddAccountDialog;
}

class AddAccountDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddAccountDialog(QWidget * parent = Q_NULLPTR);
    ~AddAccountDialog();

private Q_SLOTS:
    void onCurrentAccountTypeChanged(int index);

private:
    Ui::AddAccountDialog * m_pUi;
};

#endif // QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H
