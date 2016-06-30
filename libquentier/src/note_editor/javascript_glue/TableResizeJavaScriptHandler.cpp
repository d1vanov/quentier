#include "TableResizeJavaScriptHandler.h"

namespace quentier {

TableResizeJavaScriptHandler::TableResizeJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void TableResizeJavaScriptHandler::onTableResize()
{
    emit tableResized();
}

} // namespace quentier
