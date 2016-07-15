/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "JavaScriptInOrderExecutor.h"
#include <quentier/logging/QuentierLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace quentier {

JavaScriptInOrderExecutor::JavaScriptInOrderExecutor(WebView & view, QObject * parent) :
    QObject(parent),
    m_view(view),
    m_javaScriptsQueue(),
    m_currentPendingCallback(0),
    m_inProgress(false)
{}

void JavaScriptInOrderExecutor::append(const QString &script, JavaScriptInOrderExecutor::Callback callback)
{
    m_javaScriptsQueue.enqueue(QPair<QString, Callback>(script, callback));
    QNTRACE("JavaScriptInOrderExecutor: appended new script, there are "
            << m_javaScriptsQueue.size() << " to execute now");
}

void JavaScriptInOrderExecutor::start()
{
    QPair<QString, Callback> scriptCallbackPair = m_javaScriptsQueue.dequeue();

    m_inProgress = true;

    const QString & script = scriptCallbackPair.first;
    const Callback & callback = scriptCallbackPair.second;

    m_currentPendingCallback = callback;

#ifdef USE_QT_WEB_ENGINE
    m_view.page()->runJavaScript(script, JavaScriptCallback(*this));
#else
    QVariant data = m_view.page()->mainFrame()->evaluateJavaScript(script);
    next(data);
#endif
}

void JavaScriptInOrderExecutor::next(const QVariant & data)
{
    QNTRACE("JavaScriptInOrderExecutor::next");

    if (!m_currentPendingCallback.empty()) {
        m_currentPendingCallback(data);
        m_currentPendingCallback = 0;
    }

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
    m_executor.next(result);
}

} // namespace quentier
