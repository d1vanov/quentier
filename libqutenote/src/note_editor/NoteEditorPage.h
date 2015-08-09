#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/Linkage.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebPage>
#else
#include <QWebEnginePage>
#endif

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditor)

class QUTE_NOTE_EXPORT NoteEditorPage:
#ifndef USE_QT_WEB_ENGINE
        public QWebPage
#else
        public QWebEnginePage
#endif
{
    Q_OBJECT
public:
    explicit NoteEditorPage(NoteEditor & parent);

Q_SIGNALS:
    void noteLoadCancelled();

public Q_SLOTS:
    bool shouldInterruptJavaScript();

private:
#ifndef USE_QT_WEB_ENGINE
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
    NoteEditor * m_parent;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H
