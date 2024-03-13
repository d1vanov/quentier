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

#include <quentier/utility/StringUtils.h>

#include <qevercloud/types/Note.h>

#include <QDialog>
#include <QPointer>

namespace Ui {

class EditNoteDialog;

} // namespace Ui

class QStringListModel;
class QModelIndex;

namespace quentier {

class NotebookModel;

class EditNoteDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit EditNoteDialog(
        qevercloud::Note note, NotebookModel * notebookModel,
        QWidget * parent = nullptr, bool readOnlyMode = false);

    ~EditNoteDialog() override;

    [[nodiscard]] const qevercloud::Note & note() const noexcept
    {
        return m_note;
    }

private Q_SLOTS:
    void accept() override;

    // Slots to track the updates of notebook model
    void dataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = {});

    void rowsInserted(const QModelIndex & parent, int start, int end);

    void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);

    /**
     * Slots for QDateTimeEdit and QDoubleSpinBox edits capturing.
     * Unfortunately, Qt doesn't offer a simple way to have null/empty values
     * in these editors, instead they have some default value on creation;
     * workaround: capture editing events from these editors, treat each
     * editing event as setting the value to some 'non-empty' one; otherwise
     * treat the value from the editor as an empty one
     */

    void onCreationDateTimeEdited(const QDateTime & dateTime);

    void onModificationDateTimeEdited(const QDateTime & dateTime);

    void onDeletionDateTimeEdited(const QDateTime & dateTime);

    void onSubjectDateTimeEdited(const QDateTime & dateTime);

    void onLatitudeValueChanged(double value);

    void onLongitudeValueChanged(double value);

    void onAltitudeValueChanged(double value);

private:
    void createConnections();
    void fillNotebookNames();
    void fillDialogContent();

private:
    Ui::EditNoteDialog * m_ui;
    qevercloud::Note m_note;
    QPointer<NotebookModel> m_notebookModel;
    QStringListModel * m_notebookNamesModel;

    StringUtils m_stringUtils;

    bool m_creationDateTimeEdited = false;
    bool m_modificationDateTimeEdited = false;
    bool m_deletionDateTimeEdited = false;
    bool m_subjectDateTimeEdited = false;
    bool m_latitudeEdited = false;
    bool m_longitudeEdited = false;
    bool m_altitudeEdited = false;

    bool m_readOnlyMode;
};

} // namespace quentier
