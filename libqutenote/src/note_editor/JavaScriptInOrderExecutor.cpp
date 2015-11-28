#include "JavaScriptInOrderExecutor.h"
#include <qute_note/logging/QuteNoteLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace qute_note {

JavaScriptInOrderExecutor::JavaScriptInOrderExecutor(WebView & view, QObject * parent) :
    QObject(parent),
    m_view(view),
    m_javaScriptsQueue(),
    m_inProgress(false)
{}

void JavaScriptInOrderExecutor::append(const QString &script)
{
    m_javaScriptsQueue.enqueue(script);
    QNTRACE("JavaScriptInOrderExecutor: appended new script, there are "
            << m_javaScriptsQueue.size() << " to execute now");
}

void JavaScriptInOrderExecutor::start()
{
    QString script = m_javaScriptsQueue.dequeue();

    m_inProgress = true;

#ifdef USE_QT_WEB_ENGINE
    m_view.page()->runJavaScript(script, JavaScriptCallback(*this));
#else
    m_view.page()->mainFrame()->evaluateJavaScript(script);
    next();
#endif
}

void JavaScriptInOrderExecutor::next()
{
    QNTRACE("JavaScriptInOrderExecutor::next");

    if (m_javaScriptsQueue.empty()) {
        QNTRACE("JavaScriptInOrderExecutor: done");
        m_inProgress = false;
        emit finished();
        return;
    }

    QNTRACE("JavaScriptInOrderExecutor: " << m_javaScriptsQueue.size() << " more scripts to execute");
    start();
}

void JavaScriptInOrderExecutor::JavaScriptCallback::operator()(const QVariant & result)
{
    Q_UNUSED(result);
    m_executor.next();
}

} // namespace qute_note
