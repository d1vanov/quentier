/*
 * Copyright 2016-2025 Dmitry Ivanov
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
    QWidget{parent},
    m_ui{new Ui::FindAndReplaceWidget}
{
    m_ui->setupUi(this);
    m_ui->findLineEdit->setClearButtonEnabled(true);

    m_ui->findLineEdit->setDragEnabled(true);
    m_ui->replaceLineEdit->setDragEnabled(true);

    setReplaceEnabled(withReplace);
    createConnections();
}

FindAndReplaceWidget::~FindAndReplaceWidget()
{
    delete m_ui;
}

QString FindAndReplaceWidget::textToFind() const
{
    return m_ui->findLineEdit->text();
}

void FindAndReplaceWidget::setTextToFind(const QString & text)
{
    m_ui->findLineEdit->setText(text);
}

QString FindAndReplaceWidget::replacementText() const
{
    return m_ui->replaceLineEdit->text();
}

void FindAndReplaceWidget::setReplacementText(const QString & text)
{
    m_ui->replaceLineEdit->setText(text);
}

bool FindAndReplaceWidget::matchCase() const
{
    return m_ui->matchCaseCheckBox->isChecked();
}

void FindAndReplaceWidget::setMatchCase(const bool flag)
{
    m_ui->matchCaseCheckBox->setChecked(flag);
}

bool FindAndReplaceWidget::replaceEnabled() const
{
    return !m_ui->replaceLineEdit->isHidden();
}

void FindAndReplaceWidget::setReplaceEnabled(const bool enabled)
{
    if (enabled) {
        m_ui->replaceLineEdit->setDisabled(false);
        m_ui->replaceButton->setDisabled(false);
        m_ui->replaceAllButton->setDisabled(false);
        m_ui->replaceLabel->setDisabled(false);

        m_ui->replaceLineEdit->show();
        m_ui->replaceButton->show();
        m_ui->replaceAllButton->show();
        m_ui->replaceLabel->show();
    }
    else {
        m_ui->replaceLineEdit->hide();
        m_ui->replaceButton->hide();
        m_ui->replaceAllButton->hide();
        m_ui->replaceLabel->hide();

        m_ui->replaceLineEdit->setDisabled(true);
        m_ui->replaceButton->setDisabled(true);
        m_ui->replaceAllButton->setDisabled(true);
        m_ui->replaceLabel->setDisabled(true);
    }
}

void FindAndReplaceWidget::setFocus()
{
    m_ui->findLineEdit->setFocus();
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
    const QString textToFind = m_ui->findLineEdit->text();
    if (textToFind.isEmpty()) {
        return;
    }

    Q_EMIT findNext(textToFind, m_ui->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onNextButtonPressed()
{
    onFindTextEntered();
}

void FindAndReplaceWidget::onPreviousButtonPressed()
{
    const QString textToFind = m_ui->findLineEdit->text();
    if (textToFind.isEmpty()) {
        return;
    }

    Q_EMIT findPrevious(textToFind, m_ui->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onMatchCaseCheckboxToggled(const int state)
{
    Q_EMIT searchCaseSensitivityChanged(static_cast<bool>(state));
}

void FindAndReplaceWidget::onReplaceTextEntered()
{
    Q_EMIT replace(
        m_ui->findLineEdit->text(), m_ui->replaceLineEdit->text(),
        m_ui->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onReplaceButtonPressed()
{
    onReplaceTextEntered();
}

void FindAndReplaceWidget::onReplaceAllButtonPressed()
{
    Q_EMIT replaceAll(
        m_ui->findLineEdit->text(), m_ui->replaceLineEdit->text(),
        m_ui->matchCaseCheckBox->isChecked());
}

QSize FindAndReplaceWidget::sizeHint() const
{
    constexpr bool minimal = false;
    return sizeHintImpl(minimal);
}

QSize FindAndReplaceWidget::minimumSizeHint() const
{
    constexpr bool minimal = true;
    return sizeHintImpl(minimal);
}

void FindAndReplaceWidget::createConnections()
{
    QObject::connect(
        m_ui->closeButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onCloseButtonPressed);

    QObject::connect(
        m_ui->findLineEdit, &QLineEdit::textEdited, this,
        &FindAndReplaceWidget::textToFindEdited);

    QObject::connect(
        m_ui->findLineEdit, &QLineEdit::returnPressed, this,
        &FindAndReplaceWidget::onFindTextEntered);

    QObject::connect(
        m_ui->findNextButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onNextButtonPressed);

    QObject::connect(
        m_ui->findPreviousButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onPreviousButtonPressed);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    QObject::connect(
        m_ui->matchCaseCheckBox, &QCheckBox::checkStateChanged, this,
        &FindAndReplaceWidget::onMatchCaseCheckboxToggled);
#else
    QObject::connect(
        m_ui->matchCaseCheckBox, &QCheckBox::stateChanged, this,
        &FindAndReplaceWidget::onMatchCaseCheckboxToggled);
#endif

    QObject::connect(
        m_ui->replaceLineEdit, &QLineEdit::returnPressed, this,
        &FindAndReplaceWidget::onReplaceTextEntered);

    QObject::connect(
        m_ui->replaceButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onReplaceButtonPressed);

    QObject::connect(
        m_ui->replaceAllButton, &QPushButton::released, this,
        &FindAndReplaceWidget::onReplaceAllButtonPressed);
}

QSize FindAndReplaceWidget::sizeHintImpl(const bool minimal) const
{
    QSize sizeHint =
        (minimal ? QWidget::minimumSizeHint() : QWidget::sizeHint());

    if (!m_ui->replaceLineEdit->isEnabled()) {
        const QSize findSizeHint =
            (minimal ? m_ui->findLineEdit->minimumSizeHint()
                     : m_ui->findLineEdit->sizeHint());

        auto * layout = m_ui->gridLayout->layout();
        const auto margins = layout->contentsMargins();

        sizeHint.setHeight(
            findSizeHint.height() + margins.top() + margins.bottom());
    }

    return sizeHint;
}

} // namespace quentier
