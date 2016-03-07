#include "ResizableImageJavaScriptHandler.h"

namespace qute_note {

ResizableImageJavaScriptHandler::ResizableImageJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void ResizableImageJavaScriptHandler::notifyImageResourceResized(bool pushUndoCommand)
{
    emit imageResourceResized(pushUndoCommand);
}

} // namespace qute_note
