#include "ResizableImageJavaScriptHandler.h"

namespace qute_note {

ResizableImageJavaScriptHandler::ResizableImageJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void ResizableImageJavaScriptHandler::notifyImageResourceResized()
{
    emit imageResourceResized();
}

} // namespace qute_note
