/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#ifndef QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H
#define QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H

#include "../models/TagModel.h"
#include <quentier/utility/Macros.h>
#include <quentier/utility/StringUtils.h>
#include <QDialog>
#include <QPointer>

namespace Ui {
class AddOrEditTagDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

class AddOrEditTagDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditTagDialog(TagModel * pTagModel,
                                QWidget * parent = Q_NULLPTR,
                                const QString & editedTagLocalUid = QString());
    virtual ~AddOrEditTagDialog();

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;
    void onTagNameEdited(const QString & tagName);
    void onParentTagNameChanged(const QString & parentTagName);

private:
    void createConnections();
    bool setupEditedTagItem(QStringList & tagNames, int & currentIndex);

private:
    Ui::AddOrEditTagDialog *    m_pUi;
    QPointer<TagModel>          m_pTagModel;
    QStringListModel *          m_pTagNamesModel;
    QString                     m_editedTagLocalUid;

    // The name specified at any given moment in the line editor
    QString                     m_currentTagName;

    StringUtils                 m_stringUtils;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H
