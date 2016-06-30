#include "HyperlinkClickJavaScriptHandler.h"

namespace quentier {

HyperlinkClickJavaScriptHandler::HyperlinkClickJavaScriptHandler(QObject * parent) :
    QObject(parent)
{}

void HyperlinkClickJavaScriptHandler::onHyperlinkClicked(QString url)
{
    emit hyperlinkClicked(url);
}

} // namespace quentier
