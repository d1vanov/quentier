#include "GenericResourceOpenAndSaveButtonsOnClickHandler.h"

namespace qute_note {

GenericResourceOpenAndSaveButtonsOnClickHandler::GenericResourceOpenAndSaveButtonsOnClickHandler(QObject * parent) :
    QObject(parent)
{}

void GenericResourceOpenAndSaveButtonsOnClickHandler::onOpenResourceButtonPressed(const QString & resourceHash)
{
    emit openResourceRequest(resourceHash);
}

void GenericResourceOpenAndSaveButtonsOnClickHandler::onSaveResourceButtonPressed(const QString &resourceHash)
{
    emit saveResourceRequest(resourceHash);
}

} // namespace qute_note
