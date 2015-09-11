#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVA_SCRIPT_IN_ORDER_EXECUTOR_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVA_SCRIPT_IN_ORDER_EXECUTOR_H

#include <QObject>
#include <QWebEngineView>
#include <QQueue>

namespace qute_note {

class JavaScriptInOrderExecutor: public QObject
{
    Q_OBJECT
public:
    explicit JavaScriptInOrderExecutor(QWebEngineView & view, QObject * parent = nullptr);

    void append(const QString & script);
    int size() const { return m_javaScriptsQueue.size(); }
    void clear() { m_javaScriptsQueue.clear(); }

    void start();
    bool inProgress() const { return m_inProgress; }

Q_SIGNALS:
    void finished();

private:
    class JavaScriptCallback
    {
    public:
        JavaScriptCallback(JavaScriptInOrderExecutor & executor) :
            m_executor(executor)
        {}

        void operator()(const QVariant & result);

    private:
        JavaScriptInOrderExecutor & m_executor;
    };

    friend class JavaScriptCallback;

    void next();

private:
    QWebEngineView &    m_view;
    QQueue<QString>     m_javaScriptsQueue;
    bool                m_inProgress;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVA_SCRIPT_IN_ORDER_EXECUTOR_H
