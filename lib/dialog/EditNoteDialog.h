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

#ifndef QUENTIER_LIB_DIALOG_EDIT_NOTE_DIALOG_H
#define QUENTIER_LIB_DIALOG_EDIT_NOTE_DIALOG_H

#include <quentier/types/Note.h>
#include <quentier/utility/Macros.h>
#include <quentier/utility/StringUtils.h>

#include <QDialog>
#include <QPointer>

namespace Ui {
class EditNoteDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookModel)

class EditNoteDialog: public QDialog
{
    Q_OBJECT
public:
    explicit EditNoteDialog(const Note & note, NotebookModel * pNotebookModel,
                            QWidget * parent = Q_NULLPTR,
                            const bool readOnlyMode = false);
    virtual ~EditNoteDialog();

    const Note & note() const { return m_note; }

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;

    // Slots to track the updates of notebook model
    void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                     );
#else
                     , const QVector<int> & roles = QVector<int>());
#endif

    void rowsInserted(const QModelIndex & parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);

    // Slots for QDateTimeEdit and QDoubleSpinBox edits capturing. Unfortunately,
    // Qt doesn't offer a simple way to have null/empty values in these editors,
    // instead they have some default value on creation; workaround:
    // capture editing events from these editors, treat each editing event as
    // setting the value to some 'non-empty' one; otherwise treat the value from
    // the editor as an empty one
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
    Ui::EditNoteDialog *    m_pUi;
    Note                    m_note;
    QPointer<NotebookModel> m_pNotebookModel;
    QStringListModel *      m_pNotebookNamesModel;

    StringUtils             m_stringUtils;

    bool                    m_creationDateTimeEdited;
    bool                    m_modificationDateTimeEdited;
    bool                    m_deletionDateTimeEdited;
    bool                    m_subjectDateTimeEdited;
    bool                    m_latitudeEdited;
    bool                    m_longitudeEdited;
    bool                    m_altitudeEdited;

    bool                    m_readOnlyMode;
};

} // namespace quentier

#endif // QUENTIER_LIB_DIALOG_EDIT_NOTE_DIALOG_H
