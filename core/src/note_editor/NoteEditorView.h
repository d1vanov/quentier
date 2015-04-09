#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_VIEW_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_VIEW_H

#include <tools/qt4helper.h>
#include <QWebView>
#include <QMimeData>

class NoteEditorView: public QWebView
{
    Q_OBJECT
public:
    explicit NoteEditorView(QWidget * parent = nullptr);

private:
    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;
};

#endif //__QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_VIEW_H
