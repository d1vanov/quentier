#ifndef __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H
#define __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H

#include <QWidget>
#include <QTextListFormat>

QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace Ui {
class NoteRichTextEditor;
}

class NoteRichTextEditor : public QWidget
{
    Q_OBJECT
    
public:
    explicit NoteRichTextEditor(QWidget *parent = 0);
    ~NoteRichTextEditor();
    
    QPushButton * getTextBoldButton();
    QPushButton * getTextItalicButton();
    QPushButton * getTextUnderlineButton();
    QPushButton * getTextStrikeThroughButton();

public slots:
    // format text slots
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikeThrough();
    void textAlignLeft();
    void textAlignCenter();
    void textAlignRight();
    void textAddHorizontalLine() { /* TODO: implement */ }
    void textIncreaseIndentation();
    void textDecreaseIndentation();
    void textInsertUnorderedList();
    void textInsertOrderedList();

    void chooseTextColor();
    void chooseSelectedTextColor();

    void textSpellCheck() { /* TODO: implement */ }
    void textInsertToDoCheckBox();

private:
    void mergeFormatOnWordOrSelection(const QTextCharFormat & format);

    enum ESelectedAlignment { ALIGNED_LEFT, ALIGNED_CENTER, ALIGNED_RIGHT };
    void setAlignButtonsCheckedState(const ESelectedAlignment alignment);

    void changeIndentation(const bool increase);
    inline QTextEdit * getTextEdit();

    enum EChangeColor { COLOR_ALL, COLOR_SELECTED };
    void changeTextColor(const EChangeColor changeColorOption);

    void insertList(const QTextListFormat::Style style);

    void setupToDoCheckboxTextObject();

private:
    Ui::NoteRichTextEditor * m_pUI;
};

#endif // __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H
