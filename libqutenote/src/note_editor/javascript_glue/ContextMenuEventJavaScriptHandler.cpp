#include "ContextMenuEventJavaScriptHandler.h"

namespace qute_note {

ContextMenuEventJavaScriptHandler::ContextMenuEventJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void ContextMenuEventJavaScriptHandler::setContextMenuContent(QString contentType, quint64 sequenceNumber)
{
    emit contextMenuEventReply(contentType, sequenceNumber);
}

} // namespace qute_note
