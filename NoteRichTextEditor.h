#ifndef __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H
#define __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H

#include "TextEditWithCheckboxes.h"
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

    enum ECheckboxTextFormat
    {
        CHECKBOX_TEXT_FORMAT_UNCHECKED = QTextFormat::UserObject + 1,
        CHECKBOX_TEXT_FORMAT_CHECKED = QTextFormat::UserObject + 2
    };

    enum ECheckboxTextProperties
    {
        CHECKBOX_TEXT_DATA_UNCHECKED = 1,
        CHECKBOX_TEXT_DATA_CHECKED = 2
    };

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
    inline TextEditWithCheckboxes * getTextEdit();

    enum EChangeColor { COLOR_ALL, COLOR_SELECTED };
    void changeTextColor(const EChangeColor changeColorOption);

    void insertList(const QTextListFormat::Style style);

    void setupToDoCheckboxTextObjects();

private:
    Ui::NoteRichTextEditor * m_pUI;
};

#endif // __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H
