#include "NoteEditorWidget.h"
#include "ui_NoteEditorWidget.h"
#include "ToDoCheckboxTextObject.h"
#include "HorizontalLineExtraData.h"
#include <QTextList>
#include <QColorDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDebug>

NoteEditorWidget::NoteEditorWidget(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::NoteEditorWidget),
    m_pToDoChkboxTxtObjUnchecked(nullptr),
    m_pToDoChkboxTxtObjChecked(nullptr)
{
    m_pUI->setupUi(this);

    m_pToDoChkboxTxtObjUnchecked = new ToDoCheckboxTextObjectUnchecked;
    m_pToDoChkboxTxtObjChecked   = new ToDoCheckboxTextObjectChecked;

    QAbstractTextDocumentLayout * pLayout = getTextEdit()->document()->documentLayout();
    pLayout->registerHandler(QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_UNCHECKED, m_pToDoChkboxTxtObjUnchecked);
    pLayout->registerHandler(QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_CHECKED, m_pToDoChkboxTxtObjChecked);

    QObject::connect(m_pUI->buttonFormatTextBold, SIGNAL(clicked()), this, SLOT(textBold()));
    QObject::connect(m_pUI->buttonFormatTextItalic, SIGNAL(clicked()), this, SLOT(textItalic()));
    QObject::connect(m_pUI->buttonFormatTextUnderlined, SIGNAL(clicked()), this, SLOT(textUnderline()));
    QObject::connect(m_pUI->buttonFormatTextStrikethrough, SIGNAL(clicked()), this, SLOT(textStrikeThrough()));
    QObject::connect(m_pUI->buttonFormatTextAlignLeft, SIGNAL(clicked()), this, SLOT(textAlignLeft()));
    QObject::connect(m_pUI->buttonFormatTextAlignCenter, SIGNAL(clicked()), this, SLOT(textAlignCenter()));
    QObject::connect(m_pUI->buttonFormatTextAlignRight, SIGNAL(clicked()), this, SLOT(textAlignRight()));
    QObject::connect(m_pUI->buttonIndentIncrease, SIGNAL(clicked()), this, SLOT(textIncreaseIndentation()));
    QObject::connect(m_pUI->buttonIndentDecrease, SIGNAL(clicked()), this, SLOT(textDecreaseIndentation()));
    QObject::connect(m_pUI->buttonInsertUnorderedList, SIGNAL(clicked()), this, SLOT(textInsertUnorderedList()));
    QObject::connect(m_pUI->buttonInsertOrderedList, SIGNAL(clicked()), this, SLOT(textInsertOrderedList()));
    QObject::connect(m_pUI->buttonInsertTodoCheckbox, SIGNAL(clicked()), this, SLOT(textInsertToDoCheckBox()));
    QObject::connect(m_pUI->toolButtonChooseTextColor, SIGNAL(clicked()), this, SLOT(chooseTextColor()));
    QObject::connect(m_pUI->toolButtonChooseSelectedTextColor, SIGNAL(clicked()), this, SLOT(chooseSelectedTextColor()));
    QObject::connect(m_pUI->buttonAddHorizontalLine, SIGNAL(clicked()), this, SLOT(textAddHorizontalLine()));
}

NoteEditorWidget::~NoteEditorWidget()
{
    if (!m_pUI) {
        delete m_pUI;
        m_pUI = nullptr;
    }

    if (!m_pToDoChkboxTxtObjUnchecked) {
        delete m_pToDoChkboxTxtObjUnchecked;
        m_pToDoChkboxTxtObjUnchecked = nullptr;
    }

    if (!m_pToDoChkboxTxtObjChecked) {
        delete m_pToDoChkboxTxtObjChecked;
        m_pToDoChkboxTxtObjChecked = nullptr;
    }
}

QPushButton * NoteEditorWidget::getTextBoldButton()
{
    return m_pUI->buttonFormatTextBold;
}

QPushButton *NoteEditorWidget::getTextItalicButton()
{
    return m_pUI->buttonFormatTextItalic;
}

QPushButton *NoteEditorWidget::getTextUnderlineButton()
{
    return m_pUI->buttonFormatTextUnderlined;
}

QPushButton *NoteEditorWidget::getTextStrikeThroughButton()
{
    return m_pUI->buttonFormatTextStrikethrough;
}

QPushButton *NoteEditorWidget::getInsertUnorderedListButton()
{
    return m_pUI->buttonInsertUnorderedList;
}

QPushButton *NoteEditorWidget::getInsertOrderedListButton()
{
    return m_pUI->buttonInsertOrderedList;
}

void NoteEditorWidget::textBold()
{
    QTextCharFormat format;
    format.setFontWeight(m_pUI->buttonFormatTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    getTextEdit()->mergeFormatOnWordOrSelection(format);
}

void NoteEditorWidget::textItalic()
{
    QTextCharFormat format;
    format.setFontItalic(m_pUI->buttonFormatTextItalic->isChecked());
    getTextEdit()->mergeFormatOnWordOrSelection(format);
}

void NoteEditorWidget::textUnderline()
{
    QTextCharFormat format;
    format.setFontUnderline(m_pUI->buttonFormatTextUnderlined->isChecked());
    getTextEdit()->mergeFormatOnWordOrSelection(format);
}

void NoteEditorWidget::textStrikeThrough()
{
    QTextCharFormat format;
    format.setFontStrikeOut(m_pUI->buttonFormatTextStrikethrough->isChecked());
    getTextEdit()->mergeFormatOnWordOrSelection(format);
}

void NoteEditorWidget::textAlignLeft()
{
    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.beginEditBlock();

    getTextEdit()->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    setAlignButtonsCheckedState(ALIGNED_LEFT);

    cursor.endEditBlock();
}

void NoteEditorWidget::textAlignCenter()
{
    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.beginEditBlock();

    getTextEdit()->setAlignment(Qt::AlignHCenter);
    setAlignButtonsCheckedState(ALIGNED_CENTER);

    cursor.endEditBlock();
}

void NoteEditorWidget::textAlignRight()
{
    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.beginEditBlock();

    getTextEdit()->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    setAlignButtonsCheckedState(ALIGNED_RIGHT);

    cursor.endEditBlock();
}

void NoteEditorWidget::textAddHorizontalLine()
{
    QuteNoteTextEdit * noteTextEdit = getTextEdit();
    QTextCursor cursor = noteTextEdit->textCursor();
    cursor.beginEditBlock();

    QTextBlockFormat format = cursor.blockFormat();

    bool atStart = cursor.atStart();

    cursor.insertHtml("<hr><br>");
    cursor.setBlockFormat(format);
    cursor.movePosition(QTextCursor::Up);

    if (atStart)
    {
        cursor.movePosition(QTextCursor::Up);
        cursor.deletePreviousChar();
        cursor.movePosition(QTextCursor::Down);
    }
    else
    {
        cursor.movePosition(QTextCursor::Up);
        cursor.movePosition(QTextCursor::Up);
        QString line = cursor.block().text().trimmed();
        if (line.isEmpty()) {
            cursor.movePosition(QTextCursor::Down);
            cursor.deletePreviousChar();
            cursor.movePosition(QTextCursor::Down);
        }
        else {
            cursor.movePosition(QTextCursor::Down);
            cursor.movePosition(QTextCursor::Down);
        }
    }

    // add extra data
    cursor.movePosition(QTextCursor::Up);
    cursor.block().setUserData(new HorizontalLineExtraData);
    cursor.movePosition(QTextCursor::Down);

    cursor.endEditBlock();
}

void NoteEditorWidget::textIncreaseIndentation()
{
    getTextEdit()->changeIndentation(true);
}

void NoteEditorWidget::textDecreaseIndentation()
{
    getTextEdit()->changeIndentation(false);
}

void NoteEditorWidget::textInsertUnorderedList()
{
    QTextListFormat::Style style = QTextListFormat::ListDisc;
    insertList(style);
}

void NoteEditorWidget::textInsertOrderedList()
{
    QTextListFormat::Style style = QTextListFormat::ListDecimal;
    insertList(style);
}

void NoteEditorWidget::chooseTextColor()
{
    changeTextColor(CHANGE_COLOR_ALL);
}

void NoteEditorWidget::chooseSelectedTextColor()
{
    changeTextColor(CHANGE_COLOR_SELECTED);
}

void NoteEditorWidget::textInsertToDoCheckBox()
{
    QString checkboxUncheckedImgFileName(":/format_text_elements/checkbox_unchecked.gif");
    QFile checkboxUncheckedImgFile(checkboxUncheckedImgFileName);
    if (!checkboxUncheckedImgFile.exists()) {
        QMessageBox::warning(this, tr("Error Opening File"),
                             tr("Could not open '%1'").arg(checkboxUncheckedImgFileName));
        return;
    }

    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.beginEditBlock();

    QImage checkboxUncheckedImg(checkboxUncheckedImgFileName);
    QTextCharFormat toDoCheckboxUncheckedCharFormat;
    toDoCheckboxUncheckedCharFormat.setObjectType(QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_UNCHECKED);
    toDoCheckboxUncheckedCharFormat.setProperty(QuteNoteTextEdit::TODO_CHKBOX_TXT_DATA_UNCHECKED, checkboxUncheckedImg);

    cursor.insertText(QString(QChar::ObjectReplacementCharacter), toDoCheckboxUncheckedCharFormat);
    cursor.insertText(" ", QTextCharFormat());

    cursor.endEditBlock();
    getTextEdit()->setTextCursor(cursor);
}

void NoteEditorWidget::setAlignButtonsCheckedState(const NoteEditorWidget::ESelectedAlignment alignment)
{
    switch(alignment)
    {
    case ALIGNED_LEFT:
        m_pUI->buttonFormatTextAlignCenter->setChecked(false);
        m_pUI->buttonFormatTextAlignRight->setChecked(false);
        break;
    case ALIGNED_CENTER:
        m_pUI->buttonFormatTextAlignLeft->setChecked(false);
        m_pUI->buttonFormatTextAlignRight->setChecked(false);
        break;
    case ALIGNED_RIGHT:
        m_pUI->buttonFormatTextAlignLeft->setChecked(false);
        m_pUI->buttonFormatTextAlignCenter->setChecked(false);
        break;
    default:
        qWarning() << "Invalid action passed to setAlignButtonsCheckedState!";
        break;
    }
}

QuteNoteTextEdit * NoteEditorWidget::getTextEdit()
{
    return m_pUI->noteTextEdit;
}

void NoteEditorWidget::changeTextColor(const NoteEditorWidget::EChangeColor changeColorOption)
{
    QColor col = QColorDialog::getColor(getTextEdit()->textColor(), this);
    if (!col.isValid()) {
        qWarning() << "Invalid color";
        return;
    }

    QTextCharFormat format;
    format.setForeground(col);

    switch(changeColorOption)
    {
    case CHANGE_COLOR_ALL:
    {
        QTextCursor cursor = getTextEdit()->textCursor();
        cursor.beginEditBlock();
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(format);
        getTextEdit()->mergeCurrentCharFormat(format);
        cursor.endEditBlock();
        break;
    }
    case CHANGE_COLOR_SELECTED:
    {
        getTextEdit()->mergeFormatOnWordOrSelection(format);
        break;
    }
    default:
        qDebug() << "Warning: wrong option passed to NoteRichTextEditor::changeTextColor: "
                 << changeColorOption;
        return;
    }
}

void NoteEditorWidget::insertList(const QTextListFormat::Style style)
{
    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.beginEditBlock();

    QTextBlockFormat blockFormat = cursor.blockFormat();

    QTextListFormat listFormat;

    if (cursor.currentList()) {
        listFormat = cursor.currentList()->format();
    }
    else {
        listFormat.setIndent(blockFormat.indent() + 1);
        blockFormat.setIndent(0);
        cursor.setBlockFormat(blockFormat);
    }

    listFormat.setStyle(style);

    cursor.createList(listFormat);
    cursor.endEditBlock();
}
