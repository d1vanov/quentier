/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#include "NoteCountLabelController.h"

#include <lib/model/note/NoteModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QLabel>
#include <QTextStream>

namespace quentier {

NoteCountLabelController::NoteCountLabelController(
    QLabel & label, QObject * parent) :
    QObject{parent},
    m_label{&label}
{}

void NoteCountLabelController::setNoteModel(NoteModel & noteModel)
{
    QNDEBUG(
        "widget::NoteCountLabelController",
        "NoteCountLabelController::setNoteModel");

    if (!m_noteModel.isNull() && m_noteModel.data() == &noteModel) {
        return;
    }

    disconnectFromNoteModel();
    m_noteModel = &noteModel;
    setNoteCountsFromNoteModel();
    connectToNoteModel();
}

void NoteCountLabelController::onNoteCountPerAccountUpdated(
    const qint32 noteCount)
{
    QNDEBUG(
        "widget::NoteCountLabelController",
        "NoteCountLabelController::onNoteCountPerAccountUpdated: "
            << noteCount);

    m_totalNoteCountPerAccount = noteCount;
    setNoteCountsToLabel();
}

void NoteCountLabelController::onFilteredNotesCountUpdated(
    const qint32 noteCount)
{
    QNDEBUG(
        "widget::NoteCountLabelController",
        "NoteCountLabelController::onFilteredNotesCountUpdated: " << noteCount);

    m_filteredNotesCount = noteCount;
    setNoteCountsToLabel();
}

void NoteCountLabelController::setNoteCountsFromNoteModel()
{
    QNDEBUG(
        "widget::NoteCountLabelController",
        "NoteCountLabelController::setNoteCountsFromNoteModel");

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        m_totalNoteCountPerAccount = 0;
        m_filteredNotesCount = 0;
    }
    else {
        auto * noteModel = m_noteModel.data();
        m_totalNoteCountPerAccount = noteModel->totalAccountNotesCount();
        m_filteredNotesCount = noteModel->totalFilteredNotesCount();
    }

    setNoteCountsToLabel();
}

void NoteCountLabelController::setNoteCountsToLabel()
{
    QNDEBUG(
        "widget::NoteCountLabelController",
        "NoteCountLabelController::setNoteCountsToLabel: "
            << "total note count per account = " << m_totalNoteCountPerAccount
            << ", filtered note count per account = " << m_filteredNotesCount);

    QString text;
    QTextStream strm{&text};

    strm << tr("Notes");
    strm << ": ";
    strm << m_filteredNotesCount;
    strm << "/";
    strm << m_totalNoteCountPerAccount;
    strm.flush();

    m_label->setText(text);
}

void NoteCountLabelController::disconnectFromNoteModel()
{
    QNDEBUG(
        "widget::NoteCountLabelController",
        "NoteCountLabelController::disconnectFromNoteModel");

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG(
            "widget::NoteCountLabelController",
            "Note model is null, nothing to do");
        return;
    }

    auto * noteModel = m_noteModel.data();

    QObject::disconnect(
        noteModel, &NoteModel::noteCountPerAccountUpdated, this,
        &NoteCountLabelController::onNoteCountPerAccountUpdated);

    QObject::disconnect(
        noteModel, &NoteModel::filteredNotesCountUpdated, this,
        &NoteCountLabelController::onFilteredNotesCountUpdated);
}

void NoteCountLabelController::connectToNoteModel()
{
    QNDEBUG(
        "widget::NoteCountLabelController",
        "NoteCountLabelController::connectToNoteModel");

    if (Q_UNLIKELY(m_noteModel.isNull())) {
        QNDEBUG(
            "widget::NoteCountLabelController",
            "Note model is null, nothing to do");
        return;
    }

    auto * noteModel = m_noteModel.data();

    QObject::connect(
        noteModel, &NoteModel::noteCountPerAccountUpdated, this,
        &NoteCountLabelController::onNoteCountPerAccountUpdated,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        noteModel, &NoteModel::filteredNotesCountUpdated, this,
        &NoteCountLabelController::onFilteredNotesCountUpdated,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
}

} // namespace quentier
