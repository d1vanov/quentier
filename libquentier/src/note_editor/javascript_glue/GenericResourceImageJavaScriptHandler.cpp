#include "GenericResourceImageJavaScriptHandler.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

GenericResourceImageJavaScriptHandler::GenericResourceImageJavaScriptHandler(const QHash<QByteArray, QString> & cache, QObject * parent) :
    QObject(parent),
    m_cache(cache)
{}

void GenericResourceImageJavaScriptHandler::findGenericResourceImage(QByteArray resourceHash)
{
    QNDEBUG("GenericResourceImageJavaScriptHandler::findGenericResourceImage: resource hash = "
            << resourceHash);

    auto it = m_cache.find(QByteArray::fromHex(resourceHash));
    if (it != m_cache.end()) {
        QNTRACE("Found generic resouce image, path is " << it.value());
        emit genericResourceImageFound(resourceHash, it.value());
    }
    else {
        QNINFO("Can't find generic resource image for hash " << resourceHash);
    }
}

} // namespace quentier
