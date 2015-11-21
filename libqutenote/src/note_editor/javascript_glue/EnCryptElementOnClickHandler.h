#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__EN_CRYPT_ELEMENT_ON_CLICK_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__EN_CRYPT_ELEMENT_ON_CLICK_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

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
    void decrypt(QString encryptedText, QString cipher, QString length, QString hint, bool * pCancelled);

public Q_SLOTS:
    void onEnCryptElementClicked(QString encryptedText, QString cipher, QString length, QString hint);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__EN_CRYPT_ELEMENT_ON_CLICK_HANDLER_H
