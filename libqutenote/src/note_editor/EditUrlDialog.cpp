#include "EditUrlDialog.h"
#include "ui_EditUrlDialog.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

EditUrlDialog::EditUrlDialog(QWidget * parent, const QString & startupText,
                             const QString & startupUrl, const quint64 idNumber) :
    QDialog(parent),
    m_pUI(new Ui::EditUrlDialog),
    m_idNumber(idNumber),
    m_startupUrlWasEmpty(startupUrl.isEmpty())
{
    m_pUI->setupUi(this);
    m_pUI->urlErrorLabel->setVisible(false);

    QObject::connect(m_pUI->urlLineEdit, QNSIGNAL(QLineEdit,textEdited,QString),
                     this, QNSLOT(EditUrlDialog,onUrlEdited,QString));
    QObject::connect(m_pUI->urlLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(EditUrlDialog,onUrlEditingFinished));

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

EditUrlDialog::~EditUrlDialog()
{
    delete m_pUI;
}

void EditUrlDialog::accept()
{
    QNDEBUG("EditUrlDialog::accept");

    QUrl url;
    bool res = validateAndGetUrl(url);
    if (!res) {
        return;
    }

    emit accepted(m_pUI->textLineEdit->text(), url, m_idNumber, m_startupUrlWasEmpty);
    QDialog::accept();
}

void EditUrlDialog::onUrlEdited(QString url)
{
    if (!url.isEmpty()) {
        m_pUI->urlErrorLabel->setVisible(false);
    }
}

void EditUrlDialog::onUrlEditingFinished()
{
    QUrl dummy;
    Q_UNUSED(validateAndGetUrl(dummy));
}

bool EditUrlDialog::validateAndGetUrl(QUrl & url)
{
    QNDEBUG("EditUrlDialog::validateAndGetUrl");

    url = QUrl();

    QString enteredUrl = m_pUI->urlLineEdit->text();
    QNTRACE("Entered URL string: " << enteredUrl);

    if (enteredUrl.isEmpty()) {
        m_pUI->urlErrorLabel->setText(QObject::tr("No URL is entered"));
        m_pUI->urlErrorLabel->setVisible(true);
        return false;
    }

    url = QUrl(enteredUrl, QUrl::TolerantMode);
    QNTRACE("Parsed URL: " << url << ", is empty = " << (url.isEmpty() ? "true" : "false")
            << ", is valid = " << (url.isValid() ? "true" : "false"));

    if (url.isEmpty()) {
        m_pUI->urlErrorLabel->setText(QObject::tr("Entered URL is empty"));
        m_pUI->urlErrorLabel->setVisible(true);
        return false;
    }

    if (!url.isValid()) {
        m_pUI->urlErrorLabel->setText(QObject::tr("Entered URL is not valid"));
        m_pUI->urlErrorLabel->setVisible(true);
        return false;
    }

    return true;
}

} // namespace qute_note
