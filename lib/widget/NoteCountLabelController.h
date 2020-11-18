/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_NOTE_COUNT_LABEL_CONTROLLER_H
#define QUENTIER_LIB_WIDGET_NOTE_COUNT_LABEL_CONTROLLER_H

#include <QObject>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteModel)

class NoteCountLabelController final : public QObject
{
    Q_OBJECT
public:
    NoteCountLabelController(QLabel & label, QObject * parent = nullptr);

    void setNoteModel(NoteModel & noteModel);

private Q_SLOTS:
    void onNoteCountPerAccountUpdated(qint32 noteCount);
    void onFilteredNotesCountUpdated(qint32 noteCount);

private:
    void setNoteCountsFromNoteModel();
    void setNoteCountsToLabel();
    void connectToNoteModel();
    void disconnectFromNoteModel();

private:
    Q_DISABLE_COPY(NoteCountLabelController);

private:
    QPointer<NoteModel> m_pNoteModel;
    QLabel * m_pLabel;

    qint32 m_totalNoteCountPerAccount = 0;
    qint32 m_filteredNotesCount = 0;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NOTE_COUNT_LABEL_CONTROLLER_H
