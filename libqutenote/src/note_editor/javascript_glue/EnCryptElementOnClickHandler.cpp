#include "EnCryptElementOnClickHandler.h"

namespace qute_note {

EnCryptElementOnClickHandler::EnCryptElementOnClickHandler(QObject * parent) :
    QObject(parent)
{}

void EnCryptElementOnClickHandler::onEnCryptElementClicked(QString encryptedText, QString cipher,
                                                           QString length, QString hint)
{
    emit decrypt(encryptedText, cipher, length, hint, Q_NULLPTR);
}

} // namespace qute_note
