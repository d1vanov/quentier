#include "HyperlinkClickJavaScriptHandler.h"

namespace qute_note {

HyperlinkClickJavaScriptHandler::HyperlinkClickJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void HyperlinkClickJavaScriptHandler::onHyperlinkClicked(QString url)
{
    emit hyperlinkClicked(url);
}

} // namespace qute_note
