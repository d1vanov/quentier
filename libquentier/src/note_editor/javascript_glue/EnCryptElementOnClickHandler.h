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

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_EN_CRYPT_ELEMENT_ON_CLICK_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_EN_CRYPT_ELEMENT_ON_CLICK_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>

namespace quentier {

/**
 * @brief The EnCryptElementOnClickHandler class represents an object providing slot
 * to be called from JavaScript when user clicks on en-crypt img tag; it doesn't do
 * anything fancy on its own but just emits the signal containing the encrypted text,
 * cipher, key length and hint for someone to handle the actual decryption
 */
class EnCryptElementOnClickHandler: public QObject
{
    Q_OBJECT
public:
    explicit EnCryptElementOnClickHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void decrypt(QString encryptedText, QString cipher, QString length, QString hint, QString enCryptIndex, bool * pCancelled);

public Q_SLOTS:
    void onEnCryptElementClicked(QString encryptedText, QString cipher, QString length, QString hint, QString enCryptIndex);
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_EN_CRYPT_ELEMENT_ON_CLICK_HANDLER_H
