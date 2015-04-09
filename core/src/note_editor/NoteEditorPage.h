#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H

#include <QWebPage>

QT_FORWARD_DECLARE_CLASS(NoteEditorController)
QT_FORWARD_DECLARE_CLASS(QWebView)

class NoteEditorPage: public QWebPage
{
    Q_OBJECT
public:
    explicit NoteEditorPage(NoteEditorController * pNoteEditorController,
                            QWebView * parentView,
                            QObject * parent = nullptr);

Q_SIGNALS:
    void noteLoadCancelled();

public Q_SLOTS:
    bool shouldInterruptJavaScript();

private:
    virtual void javaScriptAlert(QWebFrame * pFrame, const QString & message);
    virtual bool javaScriptConfirm(QWebFrame * pFrame, const QString & message);
    virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID);

private:
    QWebView * m_parentView;
};

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H
