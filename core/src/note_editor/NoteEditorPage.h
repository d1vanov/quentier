#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H

#include <tools/qt4helper.h>
#include <QWebPage>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditor)

class NoteEditorPage: public QWebPage
{
    Q_OBJECT
public:
    explicit NoteEditorPage(NoteEditor & parent);

Q_SIGNALS:
    void noteLoadCancelled();

public Q_SLOTS:
    bool shouldInterruptJavaScript();

private:
    virtual void javaScriptAlert(QWebFrame * pFrame, const QString & message) Q_DECL_OVERRIDE;
    virtual bool javaScriptConfirm(QWebFrame * pFrame, const QString & message) Q_DECL_OVERRIDE;
    virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID) Q_DECL_OVERRIDE;

private:
    NoteEditor * m_parent;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PAGE_H
