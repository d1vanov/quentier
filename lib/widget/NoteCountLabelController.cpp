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

#include "NoteCountLabelController.h"

#include <lib/model/note/NoteModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QLabel>

namespace quentier {

NoteCountLabelController::NoteCountLabelController(
    QLabel & label, QObject * parent) :
    QObject(parent),
    m_pLabel(&label)
{}

void NoteCountLabelController::setNoteModel(NoteModel & noteModel)
{
    QNDEBUG(
        "widget:note_count_label", "NoteCountLabelController::setNoteModel");

    if (!m_pNoteModel.isNull() && m_pNoteModel.data() == &noteModel) {
        return;
    }

    disconnectFromNoteModel();
    m_pNoteModel = &noteModel;
    setNoteCountsFromNoteModel();
    connectToNoteModel();
}

void NoteCountLabelController::onNoteCountPerAccountUpdated(qint32 noteCount)
{
    QNDEBUG(
        "widget:note_count_label",
        "NoteCountLabelController::onNoteCountPerAccountUpdated: "
            << noteCount);

    m_totalNoteCountPerAccount = noteCount;
    setNoteCountsToLabel();
}

void NoteCountLabelController::onFilteredNotesCountUpdated(qint32 noteCount)
{
    QNDEBUG(
        "widget:note_count_label",
        "NoteCountLabelController::onFilteredNotesCountUpdated: " << noteCount);

    m_filteredNotesCount = noteCount;
    setNoteCountsToLabel();
}

void NoteCountLabelController::setNoteCountsFromNoteModel()
{
    QNDEBUG(
        "widget:note_count_label",
        "NoteCountLabelController::setNoteCountsFromNoteModel");

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        m_totalNoteCountPerAccount = 0;
        m_filteredNotesCount = 0;
    }
    else {
        auto * pNoteModel = m_pNoteModel.data();
        m_totalNoteCountPerAccount = pNoteModel->totalAccountNotesCount();
        m_filteredNotesCount = pNoteModel->totalFilteredNotesCount();
    }

    setNoteCountsToLabel();
}

void NoteCountLabelController::setNoteCountsToLabel()
{
    QNDEBUG(
        "widget:note_count_label",
        "NoteCountLabelController::setNoteCountsToLabel: "
            << "total note count per account = " << m_totalNoteCountPerAccount
            << ", filtered note count per account = " << m_filteredNotesCount);

    QString text = tr("Notes");
    text += QStringLiteral(": ");
    text += QString::number(m_filteredNotesCount);
    text += QStringLiteral("/");
    text += QString::number(m_totalNoteCountPerAccount);
    m_pLabel->setText(text);
}

void NoteCountLabelController::disconnectFromNoteModel()
{
    QNDEBUG(
        "widget:note_count_label",
        "NoteCountLabelController::disconnectFromNoteModel");

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_count_label", "Note model is null, nothing to do");
        return;
    }

    auto * pNoteModel = m_pNoteModel.data();

    QObject::disconnect(
        pNoteModel, &NoteModel::noteCountPerAccountUpdated, this,
        &NoteCountLabelController::onNoteCountPerAccountUpdated);

    QObject::disconnect(
        pNoteModel, &NoteModel::filteredNotesCountUpdated, this,
        &NoteCountLabelController::onFilteredNotesCountUpdated);
}

void NoteCountLabelController::connectToNoteModel()
{
    QNDEBUG(
        "widget:note_count_label",
        "NoteCountLabelController::connectToNoteModel");

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG("widget:note_count_label", "Note model is null, nothing to do");
        return;
    }

    auto * pNoteModel = m_pNoteModel.data();

    QObject::connect(
        pNoteModel, &NoteModel::noteCountPerAccountUpdated, this,
        &NoteCountLabelController::onNoteCountPerAccountUpdated,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

    QObject::connect(
        pNoteModel, &NoteModel::filteredNotesCountUpdated, this,
        &NoteCountLabelController::onFilteredNotesCountUpdated,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
}

} // namespace quentier
