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

#ifndef LIB_QUENTIER_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H
#define LIB_QUENTIER_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QWidget>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptedAreaPlugin)
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)

class EncryptedAreaPlugin: public QWidget
{
    Q_OBJECT
public:
    explicit EncryptedAreaPlugin(NoteEditorPrivate & noteEditor, QWidget * parent = Q_NULLPTR);
    virtual ~EncryptedAreaPlugin();

    bool initialize(const QStringList & parameterNames, const QStringList & parameterValues,
                    const NoteEditorPluginFactory & pluginFactory, QNLocalizedString & errorDescription);
    QString name() const;
    QString description() const;

private Q_SLOTS:
    void decrypt();

private:
    Ui::EncryptedAreaPlugin *           m_pUI;
    NoteEditorPrivate &                 m_noteEditor;
    QString                             m_hint;
    QString                             m_cipher;
    QString                             m_encryptedText;
    QString                             m_keyLength;
    QString                             m_id;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H
