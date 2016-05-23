#include "GenericResourceOpenAndSaveButtonsOnClickHandler.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

GenericResourceOpenAndSaveButtonsOnClickHandler::GenericResourceOpenAndSaveButtonsOnClickHandler(QObject * parent) :
    QObject(parent)
{}

void GenericResourceOpenAndSaveButtonsOnClickHandler::onOpenResourceButtonPressed(const QString & resourceHash)
{
    QNDEBUG("GenericResourceOpenAndSaveButtonsOnClickHandler::onOpenResourceButtonPressed: " << resourceHash);
    emit openResourceRequest(QByteArray::fromHex(resourceHash.toLocal8Bit()));
}

void GenericResourceOpenAndSaveButtonsOnClickHandler::onSaveResourceButtonPressed(const QString & resourceHash)
{
    QNDEBUG("GenericResourceOpenAndSaveButtonsOnClickHandler::onSaveResourceButtonPressed: " << resourceHash);
    emit saveResourceRequest(QByteArray::fromHex(resourceHash.toLocal8Bit()));
}

} // namespace qute_note
