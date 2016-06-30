#include "ResizableImageJavaScriptHandler.h"

namespace quentier {

ResizableImageJavaScriptHandler::ResizableImageJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void ResizableImageJavaScriptHandler::notifyImageResourceResized(bool pushUndoCommand)
{
    emit imageResourceResized(pushUndoCommand);
}

} // namespace quentier
