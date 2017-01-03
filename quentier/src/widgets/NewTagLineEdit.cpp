/*
 * Copyright 2016 Dmitry Ivanov
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

#include "NewTagLineEdit.h"
#include "ui_NewTagLineEdit.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QKeyEvent>
#include <QCompleter>

namespace quentier {

NewTagLineEdit::NewTagLineEdit(TagModel * pTagModel, QWidget *parent) :
    QLineEdit(parent),
    m_pUi(new Ui::NewTagLineEdit),
    m_pTagModel(pTagModel),
    m_pCompleter(new QCompleter(this))
{
    m_pUi->setupUi(this);
    setPlaceholderText(QStringLiteral("+"));
    setupCompleter();

    QObject::connect(m_pTagModel, QNSIGNAL(TagModel,sortingChanged),
                     this, QNSLOT(NewTagLineEdit,onTagModelSortingChanged));
}

NewTagLineEdit::~NewTagLineEdit()
{
    delete m_pUi;
}

void NewTagLineEdit::onTagModelSortingChanged()
{
    QNDEBUG(QStringLiteral("NewTagLineEdit::onTagModelSortingChanged"));
    m_pCompleter->setModelSorting((m_pTagModel->sortingColumn() == TagModel::Columns::Name)
                                   ? QCompleter::CaseSensitivelySortedModel : QCompleter::UnsortedModel);
}

void NewTagLineEdit::keyPressEvent(QKeyEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    int key = pEvent->key();
    QNTRACE(QStringLiteral("NewTagLineEdit::keyPressEvent: key = ") << key);

    if (key == Qt::Key_Tab) {
        emit editingFinished();
        return;
    }

    QLineEdit::keyPressEvent(pEvent);
}

void NewTagLineEdit::setupCompleter()
{
    QNDEBUG(QStringLiteral("NewTagLineEdit::setupCompleter"));

    m_pCompleter->setCaseSensitivity(Qt::CaseSensitive);
    m_pCompleter->setModelSorting((m_pTagModel->sortingColumn() == TagModel::Columns::Name)
                                   ? QCompleter::CaseSensitivelySortedModel : QCompleter::UnsortedModel);
    m_pCompleter->setModel(m_pTagModel);
    m_pCompleter->setCompletionColumn(TagModel::Columns::Name);
    setCompleter(m_pCompleter);
}

} // namespace quentier
