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

#ifndef LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_PAGE_H
#define LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_PAGE_H

#include "JavaScriptInOrderExecutor.h"
#include <quentier/utility/Macros.h>

#ifndef QUENTIER_USE_QT_WEB_ENGINE
#include <QWebPage>
#else
#include <QWebEnginePage>
#endif

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteEditor)
QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

class NoteEditorPage:
#ifndef QUENTIER_USE_QT_WEB_ENGINE
        public QWebPage
#else
        public QWebEnginePage
#endif
{
    Q_OBJECT
public:
    typedef JavaScriptInOrderExecutor::Callback Callback;

public:
    explicit NoteEditorPage(NoteEditorPrivate & parent);
    virtual ~NoteEditorPage();

    bool javaScriptQueueEmpty() const;

    void setInactive();
    void setActive();

    /**
     * @brief stopJavaScriptAutoExecution method can be used to prevent the actual execution of JavaScript code immediately
     * on calling executeJavaScript; instead the code would be put on the queue for subsequent execution and the signal
     * javaScriptLoaded would only be emitted when the whole queue is executed
     */
    void stopJavaScriptAutoExecution();

    /**
     * @brief startJavaScriptAutoExecution is the counterpart of stopJavaScriptAutoExecution: when called on stopped
     * JavaScript queue it starts the execution of the code in the queue until it is empty; if the auto execution
     * was not stopped or the queue of JavaScript code is empty, calling this method has no effect
     */
    void startJavaScriptAutoExecution();

Q_SIGNALS:
    void javaScriptLoaded();
    void noteLoadCancelled();

public Q_SLOTS:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)) && !defined(QUENTIER_USE_QT_WEB_ENGINE)
    virtual bool shouldInterruptJavaScript() Q_DECL_OVERRIDE;
#else
    bool shouldInterruptJavaScript();
#endif

    void executeJavaScript(const QString & script, Callback callback = 0, const bool clearPreviousQueue = false);

private Q_SLOTS:
    void onJavaScriptQueueEmpty();

private:
#ifndef QUENTIER_USE_QT_WEB_ENGINE
    virtual void javaScriptAlert(QWebFrame * pFrame, const QString & message) Q_DECL_OVERRIDE;
    virtual bool javaScriptConfirm(QWebFrame * pFrame, const QString & message) Q_DECL_OVERRIDE;
    virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID) Q_DECL_OVERRIDE;
#else
    virtual void javaScriptAlert(const QUrl & securityOrigin, const QString & msg) Q_DECL_OVERRIDE;
    virtual bool javaScriptConfirm(const QUrl & securityOrigin, const QString & msg) Q_DECL_OVERRIDE;
    virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString & message,
                                          int lineNumber, const QString & sourceID) Q_DECL_OVERRIDE;
#endif

private:
    NoteEditorPrivate *         m_parent;
    JavaScriptInOrderExecutor * m_pJavaScriptInOrderExecutor;
    bool                        m_javaScriptAutoExecution;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_PAGE_H
