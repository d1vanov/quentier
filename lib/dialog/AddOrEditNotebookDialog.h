/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#pragma once

#include <lib/model/notebook/NotebookModel.h>

#include <quentier/utility/StringUtils.h>

#include <QDialog>
#include <QPointer>

namespace Ui {

class AddOrEditNotebookDialog;

} // namespace Ui

class QStringListModel;

namespace quentier {

class AddOrEditNotebookDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditNotebookDialog(
        NotebookModel * notebookModel, QWidget * parent = nullptr,
        QString editedNotebookLocalId = {});

    ~AddOrEditNotebookDialog() override;

private Q_SLOTS:
    void accept() override;

    void onNotebookNameEdited(const QString & notebookName);

    void onNotebookStackIndexChanged(int stackIndex);

    void onNotebookStackChanged(const QString & stack);

private:
    void createConnections();

private:
    Ui::AddOrEditNotebookDialog * m_ui;
    QPointer<NotebookModel> m_notebookModel;
    QStringListModel * m_pNotebookStacksModel = nullptr;
    QString m_editedNotebookLocalId;
    StringUtils m_stringUtils;
};

} // namespace quentier
