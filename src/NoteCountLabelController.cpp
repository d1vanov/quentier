/*
 * Copyright 2019 Dmitry Ivanov
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
#include "models/NoteModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QLabel>

namespace quentier {

NoteCountLabelController::NoteCountLabelController(QLabel & label, QObject * parent) :
    QObject(parent),
    m_pNoteModel(),
    m_pLabel(&label),
    m_totalNoteCountPerAccount(0),
    m_filteredNotesCount(0)
{}

void NoteCountLabelController::setNoteModel(NoteModel & noteModel)
{
    QNDEBUG(QStringLiteral("NoteCountLabelController::setNoteModel"));

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
    QNDEBUG(QStringLiteral("NoteCountLabelController::onNoteCountPerAccountUpdated: ")
            << noteCount);

    m_totalNoteCountPerAccount = noteCount;
    setNoteCountsToLabel();
}

void NoteCountLabelController::onFilteredNotesCountUpdated(qint32 noteCount)
{
    QNDEBUG(QStringLiteral("NoteCountLabelController::onFilteredNotesCountUpdated: ")
            << noteCount);

    m_filteredNotesCount = noteCount;
    setNoteCountsToLabel();
}

void NoteCountLabelController::setNoteCountsFromNoteModel()
{
    QNDEBUG(QStringLiteral("NoteCountLabelController::setNoteCountsFromNoteModel"));

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        m_totalNoteCountPerAccount = 0;
        m_filteredNotesCount = 0;
    }
    else {
        NoteModel * pNoteModel = m_pNoteModel.data();
        m_totalNoteCountPerAccount = pNoteModel->totalAccountNotesCount();
        m_filteredNotesCount = pNoteModel->totalFilteredNotesCount();
    }

    setNoteCountsToLabel();
}

void NoteCountLabelController::setNoteCountsToLabel()
{
    QNDEBUG(QStringLiteral("NoteCountLabelController::setNoteCountsToLabel: ")
            << QStringLiteral("total note count per account = ")
            << m_totalNoteCountPerAccount
            << QStringLiteral(", filtered note count per account = ")
            << m_filteredNotesCount);

    QString text = tr("Notes");
    text += QStringLiteral(": ");
    text += QString::number(m_filteredNotesCount);
    text += QStringLiteral("/");
    text += QString::number(m_totalNoteCountPerAccount);
    m_pLabel->setText(text);
}

void NoteCountLabelController::disconnectFromNoteModel()
{
    QNDEBUG(QStringLiteral("NoteCountLabelController::disconnectFromNoteModel"));

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG(QStringLiteral("Note model is null, nothing to do"));
        return;
    }

    NoteModel * pNoteModel = m_pNoteModel.data();

    QObject::disconnect(pNoteModel,
                        QNSIGNAL(NoteModel,noteCountPerAccountUpdated,qint32),
                        this,
                        QNSLOT(NoteCountLabelController,
                               onNoteCountPerAccountUpdated,qint32));
    QObject::disconnect(pNoteModel,
                        QNSIGNAL(NoteModel,filteredNotesCountUpdated,qint32),
                        this,
                        QNSLOT(NoteCountLabelController,
                               onFilteredNotesCountUpdated,qint32));
}

void NoteCountLabelController::connectToNoteModel()
{
    QNDEBUG(QStringLiteral("NoteCountLabelController::connectToNoteModel"));

    if (Q_UNLIKELY(m_pNoteModel.isNull())) {
        QNDEBUG(QStringLiteral("Note model is null, nothing to do"));
        return;
    }

    NoteModel * pNoteModel = m_pNoteModel.data();

    QObject::connect(pNoteModel,
                     QNSIGNAL(NoteModel,noteCountPerAccountUpdated,qint32),
                     this,
                     QNSLOT(NoteCountLabelController,
                            onNoteCountPerAccountUpdated,qint32),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    QObject::connect(pNoteModel,
                     QNSIGNAL(NoteModel,filteredNotesCountUpdated,qint32),
                     this,
                     QNSLOT(NoteCountLabelController,
                            onFilteredNotesCountUpdated,qint32),
                     Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
}

} // namespace quentier
