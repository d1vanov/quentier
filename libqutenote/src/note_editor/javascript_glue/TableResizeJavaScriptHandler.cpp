#include "TableResizeJavaScriptHandler.h"

namespace qute_note {

TableResizeJavaScriptHandler::TableResizeJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void TableResizeJavaScriptHandler::onTableResize()
{
    emit tableResized();
}

} // namespace qute_note
