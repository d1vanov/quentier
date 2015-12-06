#include "EditHyperlinkDialog.h"
#include "ui_EditHyperlinkDialog.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

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
    QNDEBUG("EditHyperlinkDialog::accept");

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
    QNDEBUG("EditHyperlinkDialog::validateAndGetUrl");

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
