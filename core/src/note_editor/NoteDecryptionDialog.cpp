#include "NoteDecryptionDialog.h"
#include "ui_NoteDecryptionDialog.h"

namespace qute_note {

NoteDecryptionDialog::NoteDecryptionDialog(QWidget *parent) :
    QDialog(parent),
    m_pUI(new Ui::NoteDecryptionDialog)
{
    m_pUI->setupUi(this);
}

NoteDecryptionDialog::~NoteDecryptionDialog()
{
    delete m_pUI;
}

QString NoteDecryptionDialog::passphrase() const
{
    return m_pUI->passwordLineEdit->text();
}

bool NoteDecryptionDialog::rememberPassphrase() const
{
    return m_pUI->rememberPasswordCheckBox->isChecked();
}

void NoteDecryptionDialog::setHint(const QString & hint)
{
    m_pUI->hintLabel->setText(QObject::tr("Hint: ") +
                              (hint.isEmpty() ? QObject::tr("No hint available") : hint));
}

void NoteDecryptionDialog::setRememberPassphraseDefaultState(const bool checked)
{
    m_pUI->rememberPasswordCheckBox->setChecked(checked);
}

} // namespace qute_note
