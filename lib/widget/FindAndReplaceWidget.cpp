/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include "FindAndReplaceWidget.h"
#include "ui_FindAndReplaceWidget.h"

namespace quentier {

FindAndReplaceWidget::FindAndReplaceWidget(
    QWidget * parent, const bool withReplace) :
    QWidget(parent),
    m_pUI(new Ui::FindAndReplaceWidget)
{
    m_pUI->setupUi(this);
    m_pUI->findLineEdit->setClearButtonEnabled(true);

    m_pUI->findLineEdit->setDragEnabled(true);
    m_pUI->replaceLineEdit->setDragEnabled(true);

    setReplaceEnabled(withReplace);
    createConnections();
}

FindAndReplaceWidget::~FindAndReplaceWidget()
{
    delete m_pUI;
}

QString FindAndReplaceWidget::textToFind() const
{
    return m_pUI->findLineEdit->text();
}

void FindAndReplaceWidget::setTextToFind(const QString & text)
{
    m_pUI->findLineEdit->setText(text);
}

QString FindAndReplaceWidget::replacementText() const
{
    return m_pUI->replaceLineEdit->text();
}

void FindAndReplaceWidget::setReplacementText(const QString & text)
{
    m_pUI->replaceLineEdit->setText(text);
}

bool FindAndReplaceWidget::matchCase() const
{
    return m_pUI->matchCaseCheckBox->isChecked();
}

void FindAndReplaceWidget::setMatchCase(const bool flag)
{
    m_pUI->matchCaseCheckBox->setChecked(flag);
}

bool FindAndReplaceWidget::replaceEnabled() const
{
    return !m_pUI->replaceLineEdit->isHidden();
}

void FindAndReplaceWidget::setReplaceEnabled(const bool enabled)
{
    if (enabled) {
        m_pUI->replaceLineEdit->setDisabled(false);
        m_pUI->replaceButton->setDisabled(false);
        m_pUI->replaceAllButton->setDisabled(false);
        m_pUI->replaceLabel->setDisabled(false);

        m_pUI->replaceLineEdit->show();
        m_pUI->replaceButton->show();
        m_pUI->replaceAllButton->show();
        m_pUI->replaceLabel->show();
    }
    else {
        m_pUI->replaceLineEdit->hide();
        m_pUI->replaceButton->hide();
        m_pUI->replaceAllButton->hide();
        m_pUI->replaceLabel->hide();

        m_pUI->replaceLineEdit->setDisabled(true);
        m_pUI->replaceButton->setDisabled(true);
        m_pUI->replaceAllButton->setDisabled(true);
        m_pUI->replaceLabel->setDisabled(true);
    }
}

void FindAndReplaceWidget::setFocus()
{
    m_pUI->findLineEdit->setFocus();
}

void FindAndReplaceWidget::show()
{
    QWidget::show();
    setFocus();
}

void FindAndReplaceWidget::onCloseButtonPressed()
{
    Q_EMIT closed();
    setReplaceEnabled(false);
    hide();
}

void FindAndReplaceWidget::onFindTextEntered()
{
    QString textToFind = m_pUI->findLineEdit->text();
    if (textToFind.isEmpty()) {
        return;
    }

    Q_EMIT findNext(textToFind, m_pUI->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onNextButtonPressed()
{
    onFindTextEntered();
}

void FindAndReplaceWidget::onPreviousButtonPressed()
{
    QString textToFind = m_pUI->findLineEdit->text();
    if (textToFind.isEmpty()) {
        return;
    }

    Q_EMIT findPrevious(textToFind, m_pUI->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onMatchCaseCheckboxToggled(int state)
{
    Q_EMIT searchCaseSensitivityChanged(static_cast<bool>(state));
}

void FindAndReplaceWidget::onReplaceTextEntered()
{
    Q_EMIT replace(
        m_pUI->findLineEdit->text(), m_pUI->replaceLineEdit->text(),
        m_pUI->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onReplaceButtonPressed()
{
    onReplaceTextEntered();
}

void FindAndReplaceWidget::onReplaceAllButtonPressed()
{
    Q_EMIT replaceAll(
        m_pUI->findLineEdit->text(), m_pUI->replaceLineEdit->text(),
        m_pUI->matchCaseCheckBox->isChecked());
}

QSize FindAndReplaceWidget::sizeHint() const
{
    bool minimal = false;
    return sizeHintImpl(minimal);
}

QSize FindAndReplaceWidget::minimumSizeHint() const
{
    bool minimal = true;
    return sizeHintImpl(minimal);
}

void FindAndReplaceWidget::createConnections()
{
    QObject::connect(
        m_pUI->closeButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onCloseButtonPressed);

    QObject::connect(
        m_pUI->findLineEdit, &QLineEdit::textEdited, this,
        &FindAndReplaceWidget::textToFindEdited);

    QObject::connect(
        m_pUI->findLineEdit, &QLineEdit::returnPressed, this,
        &FindAndReplaceWidget::onFindTextEntered);

    QObject::connect(
        m_pUI->findNextButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onNextButtonPressed);

    QObject::connect(
        m_pUI->findPreviousButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onPreviousButtonPressed);

    QObject::connect(
        m_pUI->matchCaseCheckBox, &QCheckBox::stateChanged, this,
        &FindAndReplaceWidget::onMatchCaseCheckboxToggled);

    QObject::connect(
        m_pUI->replaceLineEdit, &QLineEdit::returnPressed, this,
        &FindAndReplaceWidget::onReplaceTextEntered);

    QObject::connect(
        m_pUI->replaceButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onReplaceButtonPressed);

    QObject::connect(
        m_pUI->replaceAllButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onReplaceAllButtonPressed);
}

QSize FindAndReplaceWidget::sizeHintImpl(const bool minimal) const
{
    QSize sizeHint =
        (minimal ? QWidget::minimumSizeHint() : QWidget::sizeHint());

    if (!m_pUI->replaceLineEdit->isEnabled()) {
        QSize findSizeHint =
            (minimal ? m_pUI->findLineEdit->minimumSizeHint()
                     : m_pUI->findLineEdit->sizeHint());

        auto * pLayout = m_pUI->gridLayout->layout();
        auto margins = pLayout->contentsMargins();

        sizeHint.setHeight(
            findSizeHint.height() + margins.top() + margins.bottom());
    }

    return sizeHint;
}

} // namespace quentier
