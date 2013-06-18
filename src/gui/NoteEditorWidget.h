#ifndef __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H
#define __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H

#include "QuteNoteTextEdit.h"
#include <QWidget>
#include <QTextListFormat>

QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace Ui {
class NoteEditorWidget;
}

class NoteEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoteEditorWidget(QWidget * parent = 0);
    ~NoteEditorWidget();
    
    QPushButton * getTextBoldButton();
    QPushButton * getTextItalicButton();
    QPushButton * getTextUnderlineButton();
    QPushButton * getTextStrikeThroughButton();
    QPushButton * getInsertUnorderedListButton();
    QPushButton * getInsertOrderedListButton();

public slots:
    // format text slots
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikeThrough();
    void textAlignLeft();
    void textAlignCenter();
    void textAlignRight();
    void textAddHorizontalLine();
    void textIncreaseIndentation();
    void textDecreaseIndentation();
    void textInsertUnorderedList();
    void textInsertOrderedList();

    void chooseTextColor();
    void chooseSelectedTextColor();

    void textSpellCheck() { /* TODO: implement */ }
    void textInsertToDoCheckBox();

private:
    enum ESelectedAlignment { ALIGNED_LEFT, ALIGNED_CENTER, ALIGNED_RIGHT };
    void setAlignButtonsCheckedState(const ESelectedAlignment alignment);

    inline QuteNoteTextEdit * getTextEdit();

    enum EChangeColor { CHANGE_COLOR_ALL, CHANGE_COLOR_SELECTED };
    void changeTextColor(const EChangeColor changeColorOption);

    void insertList(const QTextListFormat::Style style);

private:
    Ui::NoteEditorWidget * m_pUI;
    QObject * m_pToDoChkboxTxtObjUnchecked;
    QObject * m_pToDoChkboxTxtObjChecked;
};

#endif // __QUTE_NOTE__NOTE_RICH_TEXT_EDITOR_H
