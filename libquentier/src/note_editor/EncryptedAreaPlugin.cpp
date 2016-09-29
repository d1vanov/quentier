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

#include "EncryptedAreaPlugin.h"
#include "ui_EncryptedAreaPlugin.h"
#include "NoteEditorPluginFactory.h"
#include "NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

EncryptedAreaPlugin::EncryptedAreaPlugin(NoteEditorPrivate & noteEditor, QWidget * parent) :
    QWidget(parent),
    m_pUI(new Ui::EncryptedAreaPlugin),
    m_noteEditor(noteEditor),
    m_hint(),
    m_cipher(),
    m_keyLength(),
    m_id()
{
    QNDEBUG(QStringLiteral("EncryptedAreaPlugin: constructor"));

    m_pUI->setupUi(this);

    QAction * showEncryptedTextAction = new QAction(this);
    showEncryptedTextAction->setText(tr("Show encrypted text") + QStringLiteral("..."));
    showEncryptedTextAction->setEnabled(m_noteEditor.isPageEditable());
    QObject::connect(showEncryptedTextAction, QNSIGNAL(QAction,triggered), this, QNSLOT(EncryptedAreaPlugin,decrypt));
    m_pUI->toolButton->addAction(showEncryptedTextAction);

    if (m_noteEditor.isPageEditable()) {
        QObject::connect(m_pUI->iconPushButton, QNSIGNAL(QPushButton,released), this, QNSLOT(EncryptedAreaPlugin,decrypt));
    }
}

EncryptedAreaPlugin::~EncryptedAreaPlugin()
{
    QNDEBUG(QStringLiteral("EncryptedAreaPlugin: destructor"));
    delete m_pUI;
}

bool EncryptedAreaPlugin::initialize(const QStringList & parameterNames, const QStringList & parameterValues,
                                     const NoteEditorPluginFactory & pluginFactory, QNLocalizedString & errorDescription)
{
    QNDEBUG(QStringLiteral("EncryptedAreaPlugin::initialize: parameter names = ") << parameterNames.join(QStringLiteral(", "))
            << QStringLiteral(", parameter values = ") << parameterValues.join(QStringLiteral(", ")));

    Q_UNUSED(pluginFactory)

    const int numParameterValues = parameterValues.size();

    int cipherIndex = parameterNames.indexOf(QStringLiteral("cipher"));
    if (numParameterValues <= cipherIndex) {
        errorDescription = QT_TR_NOOP("no value was found for cipher attribute");
        return false;
    }

    int encryptedTextIndex = parameterNames.indexOf(QStringLiteral("encrypted_text"));
    if (encryptedTextIndex < 0) {
        errorDescription = QT_TR_NOOP("encrypted text parameter was not found within object with encrypted text");
        return false;
    }

    int keyLengthIndex = parameterNames.indexOf(QStringLiteral("length"));
    if (numParameterValues <= keyLengthIndex) {
        errorDescription = QT_TR_NOOP("no value was found for length attribute");
        return false;
    }

    if (keyLengthIndex >= 0) {
        m_keyLength = parameterValues[keyLengthIndex];
    }
    else {
        m_keyLength = QStringLiteral("128");
        QNDEBUG(QStringLiteral("Using the default value of key length = ") << m_keyLength << QStringLiteral(" instead of missing HTML attribute"));
    }

    if (cipherIndex >= 0) {
        m_cipher = parameterValues[cipherIndex];
    }
    else {
        m_cipher = QStringLiteral("AES");
        QNDEBUG(QStringLiteral("Using the default value of cipher = ") << m_cipher << QStringLiteral(" instead of missing HTML attribute"));
    }

    m_encryptedText = parameterValues[encryptedTextIndex];

    int hintIndex = parameterNames.indexOf(QStringLiteral("hint"));
    if ((hintIndex < 0) || (numParameterValues <= hintIndex)) {
        m_hint.clear();
    }
    else {
        m_hint = parameterValues[hintIndex];
    }

    int enCryptIndexIndex = parameterNames.indexOf(QStringLiteral("en-crypt-id"));
    if ((enCryptIndexIndex < 0) || (numParameterValues <= enCryptIndexIndex)) {
        m_id.clear();
    }
    else {
        m_id = parameterValues[enCryptIndexIndex];
    }

    QNTRACE(QStringLiteral("Initialized encrypted area plugin: cipher = ") << m_cipher
            << QStringLiteral(", length = ") << m_keyLength << QStringLiteral(", hint = ") << m_hint
            << QStringLiteral(", en-crypt-id = ") << m_id << QStringLiteral(", encrypted text = ") << m_encryptedText);
    return true;
}

QString EncryptedAreaPlugin::name() const
{
    return QStringLiteral("EncryptedAreaPlugin");
}

QString EncryptedAreaPlugin::description() const
{
    return tr("Encrypted area plugin - note editor plugin used for the display and convenient work "
              "with encrypted text within notes");
}

void EncryptedAreaPlugin::decrypt()
{
    m_noteEditor.decryptEncryptedText(m_encryptedText, m_cipher, m_keyLength, m_hint, m_id);
}

} // namespace quentier
