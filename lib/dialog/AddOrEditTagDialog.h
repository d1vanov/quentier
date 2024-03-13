/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include <lib/model/tag/TagModel.h>

#include <quentier/utility/StringUtils.h>

#include <QDialog>
#include <QPointer>

namespace Ui {

class AddOrEditTagDialog;

} // namespace Ui

class QStringListModel;

namespace quentier {

class AddOrEditTagDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditTagDialog(
        TagModel * tagModel, QWidget * parent = nullptr,
        QString editedTagLocalId = {});

    ~AddOrEditTagDialog() override;

private Q_SLOTS:
    void accept() override;
    void onTagNameEdited(const QString & tagName);
    void onParentTagNameChanged(const QString & parentTagName);

private:
    void createConnections();

    [[nodiscard]] bool setupEditedTagItem(
        QStringList & tagNames, int & currentIndex);

private:
    Ui::AddOrEditTagDialog * m_ui;
    QPointer<TagModel> m_tagModel;
    QStringListModel * m_tagNamesModel = nullptr;
    QString m_editedTagLocalId;

    // The name specified at any given moment in the line editor
    QString m_currentTagName;

    StringUtils m_stringUtils;
};

} // namespace quentier
