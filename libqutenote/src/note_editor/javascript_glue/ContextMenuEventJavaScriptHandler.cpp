#include "ContextMenuEventJavaScriptHandler.h"

namespace qute_note {

ContextMenuEventJavaScriptHandler::ContextMenuEventJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void ContextMenuEventJavaScriptHandler::setContextMenuContent(QString contentType, bool hasSelection, quint64 sequenceNumber)
{
    emit contextMenuEventReply(contentType, hasSelection, sequenceNumber);
}

} // namespace qute_note
