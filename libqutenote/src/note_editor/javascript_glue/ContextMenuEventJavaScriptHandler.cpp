#include "ContextMenuEventJavaScriptHandler.h"

namespace qute_note {

ContextMenuEventJavaScriptHandler::ContextMenuEventJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void ContextMenuEventJavaScriptHandler::setContextMenuContent(QString contentType, QString selectedHtml, quint64 sequenceNumber)
{
    emit contextMenuEventReply(contentType, selectedHtml, sequenceNumber);
}

} // namespace qute_note
