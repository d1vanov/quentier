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

#include "NewTagLineEditor.h"
#include "ui_NewTagLineEditor.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QKeyEvent>
#include <QCompleter>

namespace quentier {

NewTagLineEditor::NewTagLineEditor(TagModel * pTagModel, QWidget *parent) :
    QLineEdit(parent),
    m_pUi(new Ui::NewTagLineEditor),
    m_pTagModel(pTagModel),
    m_pCompleter(new QCompleter(this))
{
    m_pUi->setupUi(this);
    setPlaceholderText("+");
    setupCompleter();

    QObject::connect(m_pTagModel, QNSIGNAL(TagModel,sortingChanged),
                     this, QNSLOT(NewTagLineEditor,onTagModelSortingChanged));
}

NewTagLineEditor::~NewTagLineEditor()
{
    delete m_pUi;
}

void NewTagLineEditor::onTagModelSortingChanged()
{
    QNDEBUG("NewTagLineEditor::onTagModelSortingChanged");
    m_pCompleter->setModelSorting((m_pTagModel->sortingColumn() == TagModel::Columns::Name)
                                   ? QCompleter::CaseSensitivelySortedModel : QCompleter::UnsortedModel);
}

void NewTagLineEditor::keyPressEvent(QKeyEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    int key = pEvent->key();
    QNTRACE("NewTagLineEditor::keyPressEvent: key = " << key);

    if (key == Qt::Key_Tab) {
        emit editingFinished();
        return;
    }

    QLineEdit::keyPressEvent(pEvent);
}

void NewTagLineEditor::setupCompleter()
{
    QNDEBUG("NewTagLineEditor::setupCompleter");

    m_pCompleter->setCaseSensitivity(Qt::CaseSensitive);
    m_pCompleter->setModelSorting((m_pTagModel->sortingColumn() == TagModel::Columns::Name)
                                   ? QCompleter::CaseSensitivelySortedModel : QCompleter::UnsortedModel);
    m_pCompleter->setModel(m_pTagModel);
    setCompleter(m_pCompleter);
}

} // namespace quentier
