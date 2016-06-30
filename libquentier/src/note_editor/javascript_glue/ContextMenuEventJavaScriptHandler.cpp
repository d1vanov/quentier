#include "ContextMenuEventJavaScriptHandler.h"

namespace quentier {

ContextMenuEventJavaScriptHandler::ContextMenuEventJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void ContextMenuEventJavaScriptHandler::setContextMenuContent(QString contentType, QString selectedHtml,
                                                              bool insideDecryptedTextFragment,
                                                              QStringList extraData, quint64 sequenceNumber)
{
    emit contextMenuEventReply(contentType, selectedHtml, insideDecryptedTextFragment, extraData, sequenceNumber);
}

} // namespace quentier
