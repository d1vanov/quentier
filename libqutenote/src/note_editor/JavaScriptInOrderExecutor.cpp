#include "JavaScriptInOrderExecutor.h"

namespace qute_note {

JavaScriptInOrderExecutor::JavaScriptInOrderExecutor(QWebEngineView & view, QObject * parent) :
    QObject(parent),
    m_view(view),
    m_javaScriptsQueue(),
    m_inProgress(false)
{}

void JavaScriptInOrderExecutor::start()
{
    QString script = m_javaScriptsQueue.takeFirst();
    m_view.page()->runJavaScript(script, JavaScriptCallback(*this));
}

void JavaScriptInOrderExecutor::next()
{
    if (m_javaScriptsQueue.empty()) {
        emit finished();
        m_inProgress = false;
        return;
    }

    start();
}

void JavaScriptInOrderExecutor::JavaScriptCallback::operator()(const QVariant & result)
{
    Q_UNUSED(result);
    m_executor.next();
}

} // namespace qute_note
