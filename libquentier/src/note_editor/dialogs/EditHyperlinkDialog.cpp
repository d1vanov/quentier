/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "EditHyperlinkDialog.h"
#include "ui_EditHyperlinkDialog.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

EditHyperlinkDialog::EditHyperlinkDialog(QWidget * parent, const QString & startupText,
                                         const QString & startupUrl, const quint64 idNumber) :
    QDialog(parent),
    m_pUI(new Ui::EditHyperlinkDialog),
    m_idNumber(idNumber),
    m_startupUrlWasEmpty(startupUrl.isEmpty())
{
    m_pUI->setupUi(this);
    m_pUI->urlErrorLabel->setVisible(false);

    QObject::connect(m_pUI->urlLineEdit, QNSIGNAL(QLineEdit,textEdited,QString),
                     this, QNSLOT(EditHyperlinkDialog,onUrlEdited,QString));
    QObject::connect(m_pUI->urlLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(EditHyperlinkDialog,onUrlEditingFinished));

    if (!startupText.isEmpty()) {
        m_pUI->textLineEdit->setText(startupText);
    }

    if (!startupUrl.isEmpty()) {
        QUrl url(startupUrl, QUrl::TolerantMode);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        m_pUI->urlLineEdit->setText(url.toString(QUrl::FullyEncoded));
#else
        m_pUI->urlLineEdit->setText(url.toString(QUrl::None));
#endif
        onUrlEditingFinished();
    }
}

EditHyperlinkDialog::~EditHyperlinkDialog()
{
    delete m_pUI;
}

void EditHyperlinkDialog::accept()
{
    QNDEBUG(QStringLiteral("EditHyperlinkDialog::accept"));

    QUrl url;
    bool res = validateAndGetUrl(url);
    if (!res) {
        return;
    }

    emit accepted(m_pUI->textLineEdit->text(), url, m_idNumber, m_startupUrlWasEmpty);
    QDialog::accept();
}

void EditHyperlinkDialog::onUrlEdited(QString url)
{
    if (!url.isEmpty()) {
        m_pUI->urlErrorLabel->setVisible(false);
    }
}

void EditHyperlinkDialog::onUrlEditingFinished()
{
    QUrl dummy;
    Q_UNUSED(validateAndGetUrl(dummy));
}

bool EditHyperlinkDialog::validateAndGetUrl(QUrl & url)
{
    QNDEBUG(QStringLiteral("EditHyperlinkDialog::validateAndGetUrl"));

    url = QUrl();

    QString enteredUrl = m_pUI->urlLineEdit->text();
    QNTRACE(QStringLiteral("Entered URL string: ") << enteredUrl);

    if (enteredUrl.isEmpty()) {
        m_pUI->urlErrorLabel->setText(tr("No URL is entered"));
        m_pUI->urlErrorLabel->setVisible(true);
        return false;
    }

    url = QUrl(enteredUrl, QUrl::TolerantMode);
    QNTRACE(QStringLiteral("Parsed URL: ") << url << QStringLiteral(", is empty = ")
            << (url.isEmpty() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", is valid = ") << (url.isValid() ? QStringLiteral("true") : QStringLiteral("false")));

    if (url.isEmpty()) {
        m_pUI->urlErrorLabel->setText(tr("Entered URL is empty"));
        m_pUI->urlErrorLabel->setVisible(true);
        return false;
    }

    if (!url.isValid()) {
        m_pUI->urlErrorLabel->setText(tr("Entered URL is not valid"));
        m_pUI->urlErrorLabel->setVisible(true);
        return false;
    }

    return true;
}

} // namespace quentier
