#include "ContextMenuEventJavaScriptHandler.h"

namespace qute_note {

ContextMenuEventJavaScriptHandler::ContextMenuEventJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void ContextMenuEventJavaScriptHandler::setContextMenuContent(QString contentType, QString selectedHtml,
                                                              bool insideDecryptedTextFragment, quint64 sequenceNumber)
{
    emit contextMenuEventReply(contentType, selectedHtml, insideDecryptedTextFragment, sequenceNumber);
}

} // namespace qute_note
