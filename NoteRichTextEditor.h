#ifndef NOTERICHTEXTEDITOR_H
#define NOTERICHTEXTEDITOR_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QTextCharFormat)
QT_FORWARD_DECLARE_CLASS(QTextEdit)

namespace Ui {
class NoteRichTextEditor;
}

class NoteRichTextEditor : public QWidget
{
    Q_OBJECT
    
public:
    explicit NoteRichTextEditor(QWidget *parent = 0);
    ~NoteRichTextEditor();
    
private slots:
    // format text slots
    void textBold();
    void textItalic();
    void textUnderline() { /* TODO: implement */ }
    void textStrikeThrough() { /* TODO: implement */ }
    void textAlignLeft() { /* TODO: implement */ }
    void textAlignCenter() { /* TODO: implement */ }
    void textAlignRight() { /* TODO: implement */ }
    void textAddHorizontalLine() { /* TODO: implement */ }
    void textIncreaseIndentation() { /* TODO: implement */ }
    void textDecreaseIndentation() { /* TODO: implement */ }
    void textInsertUnorderedList() { /* TODO: implement */ }
    void textInsertOrderedList() { /* TODO: implement */ }

    void chooseTextColor() { /* TODO: implement */ }
    void chooseSelectedTextColor() { /* TODO: implement */ }

    void textSpellCheck() { /* TODO: implement */ }
    void textInsertToDoCheckBox() { /* TODO: implement */ }

private:
    void mergeFormatOnWordOrSelection(const QTextCharFormat & format);
    inline QTextEdit * getTextEdit();

private:
    Ui::NoteRichTextEditor * m_pUI;
};

#endif // NOTERICHTEXTEDITOR_H
