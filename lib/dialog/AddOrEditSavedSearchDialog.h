/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DIALOG_ADD_OR_EDIT_SAVED_SEARCH_DIALOG_H
#define QUENTIER_LIB_DIALOG_ADD_OR_EDIT_SAVED_SEARCH_DIALOG_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/StringUtils.h>

#include <QDialog>
#include <QPointer>

#include <memory>

namespace Ui {
class AddOrEditSavedSearchDialog;
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteSearchQuery)
QT_FORWARD_DECLARE_CLASS(SavedSearchModel)

class AddOrEditSavedSearchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditSavedSearchDialog(
        SavedSearchModel * pSavedSearchModel, QWidget * parent = nullptr,
        const QString & editedSavedSearchLocalUid = {});

    virtual ~AddOrEditSavedSearchDialog() override;

    void setQuery(const QString & query);

private Q_SLOTS:
    virtual void accept() override;
    void onSavedSearchNameEdited(const QString & savedSearchName);
    void onSearchQueryEdited();

private:
    void createConnections();
    void setupEditedSavedSearchItem();

private:
    Ui::AddOrEditSavedSearchDialog * m_pUi;
    QPointer<SavedSearchModel> m_pSavedSearchModel;
    std::unique_ptr<NoteSearchQuery> m_pSearchQuery;
    QString m_editedSavedSearchLocalUid;
    StringUtils m_stringUtils;
};

} // namespace quentier

#endif // QUENTIER_LIB_DIALOG_ADD_OR_EDIT_SAVED_SEARCH_DIALOG_H
