#ifndef NOTERICHTEXTEDITOR_H
#define NOTERICHTEXTEDITOR_H

#include <QWidget>

namespace Ui {
class NoteRichTextEditor;
}

class NoteRichTextEditor : public QWidget
{
    Q_OBJECT
    
public:
    explicit NoteRichTextEditor(QWidget *parent = 0);
    ~NoteRichTextEditor();
    
private:
    Ui::NoteRichTextEditor *m_pUI;
};

#endif // NOTERICHTEXTEDITOR_H
