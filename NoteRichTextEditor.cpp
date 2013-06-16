#include "NoteRichTextEditor.h"
#include "ui_NoteRichTextEditor.h"
#include "ToDoCheckboxTextObject.h"
#include "HorizontalLineExtraData.h"
#include <QTextList>
#include <QColorDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDebug>

NoteRichTextEditor::NoteRichTextEditor(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::NoteRichTextEditor),
    m_pToDoCheckboxTextObjectInterfaceUnchecked(nullptr),
    m_pToDoCheckboxTextObjectInterfaceChecked(nullptr)
{
    m_pUI->setupUi(this);
    setupToDoCheckboxTextObjects();

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

NoteRichTextEditor::~NoteRichTextEditor()
{
    if (!m_pUI) {
        delete m_pUI;
        m_pUI = nullptr;
    }

    if (!m_pToDoCheckboxTextObjectInterfaceUnchecked) {
        delete m_pToDoCheckboxTextObjectInterfaceUnchecked;
        m_pToDoCheckboxTextObjectInterfaceUnchecked = nullptr;
    }

    if (!m_pToDoCheckboxTextObjectInterfaceChecked) {
        delete m_pToDoCheckboxTextObjectInterfaceChecked;
        m_pToDoCheckboxTextObjectInterfaceChecked = nullptr;
    }
}

QPushButton * NoteRichTextEditor::getTextBoldButton()
{
    return m_pUI->buttonFormatTextBold;
}

QPushButton *NoteRichTextEditor::getTextItalicButton()
{
    return m_pUI->buttonFormatTextItalic;
}

QPushButton *NoteRichTextEditor::getTextUnderlineButton()
{
    return m_pUI->buttonFormatTextUnderlined;
}

QPushButton *NoteRichTextEditor::getTextStrikeThroughButton()
{
    return m_pUI->buttonFormatTextStrikethrough;
}

QPushButton *NoteRichTextEditor::getInsertUnorderedListButton()
{
    return m_pUI->buttonInsertUnorderedList;
}

QPushButton *NoteRichTextEditor::getInsertOrderedListButton()
{
    return m_pUI->buttonInsertOrderedList;
}

void NoteRichTextEditor::textBold()
{
    QTextCharFormat format;
    format.setFontWeight(m_pUI->buttonFormatTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(format);
}

void NoteRichTextEditor::textItalic()
{
    QTextCharFormat format;
    format.setFontItalic(m_pUI->buttonFormatTextItalic->isChecked());
    mergeFormatOnWordOrSelection(format);
}

void NoteRichTextEditor::textUnderline()
{
    QTextCharFormat format;
    format.setFontUnderline(m_pUI->buttonFormatTextUnderlined->isChecked());
    mergeFormatOnWordOrSelection(format);
}

void NoteRichTextEditor::textStrikeThrough()
{
    QTextCharFormat format;
    format.setFontStrikeOut(m_pUI->buttonFormatTextStrikethrough->isChecked());
    mergeFormatOnWordOrSelection(format);
}

void NoteRichTextEditor::textAlignLeft()
{
    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.beginEditBlock();

    getTextEdit()->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    setAlignButtonsCheckedState(ALIGNED_LEFT);

    cursor.endEditBlock();
}

void NoteRichTextEditor::textAlignCenter()
{
    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.beginEditBlock();

    getTextEdit()->setAlignment(Qt::AlignHCenter);
    setAlignButtonsCheckedState(ALIGNED_CENTER);

    cursor.endEditBlock();
}

void NoteRichTextEditor::textAlignRight()
{
    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.beginEditBlock();

    getTextEdit()->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    setAlignButtonsCheckedState(ALIGNED_RIGHT);

    cursor.endEditBlock();
}

void NoteRichTextEditor::textAddHorizontalLine()
{
    TextEditWithCheckboxes * noteTextEdit = getTextEdit();
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

void NoteRichTextEditor::textIncreaseIndentation()
{
    getTextEdit()->changeIndentation(true);
}

void NoteRichTextEditor::textDecreaseIndentation()
{
    getTextEdit()->changeIndentation(false);
}

void NoteRichTextEditor::textInsertUnorderedList()
{
    QTextListFormat::Style style = QTextListFormat::ListDisc;
    insertList(style);
}

void NoteRichTextEditor::textInsertOrderedList()
{
    QTextListFormat::Style style = QTextListFormat::ListDecimal;
    insertList(style);
}

void NoteRichTextEditor::chooseTextColor()
{
    changeTextColor(COLOR_ALL);
}

void NoteRichTextEditor::chooseSelectedTextColor()
{
    changeTextColor(COLOR_SELECTED);
}

void NoteRichTextEditor::textInsertToDoCheckBox()
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
    toDoCheckboxUncheckedCharFormat.setObjectType(CHECKBOX_TEXT_FORMAT_UNCHECKED);
    toDoCheckboxUncheckedCharFormat.setProperty(CHECKBOX_TEXT_DATA_UNCHECKED, checkboxUncheckedImg);

    cursor.insertText(QString(QChar::ObjectReplacementCharacter), toDoCheckboxUncheckedCharFormat);
    cursor.insertText(" ", QTextCharFormat());

    cursor.endEditBlock();
    getTextEdit()->setTextCursor(cursor);
}

void NoteRichTextEditor::mergeFormatOnWordOrSelection(const QTextCharFormat & format)
{
    TextEditWithCheckboxes * noteTextEdit = getTextEdit();
    QTextCursor cursor = noteTextEdit->textCursor();
    cursor.beginEditBlock();

    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }

    cursor.mergeCharFormat(format);
    cursor.clearSelection();
    cursor.endEditBlock();
    noteTextEdit->mergeCurrentCharFormat(format);
}

void NoteRichTextEditor::setAlignButtonsCheckedState(const NoteRichTextEditor::ESelectedAlignment alignment)
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
        qDebug() << "Warning! Invalid action passed to setAlignButtonsCheckedState!";
        break;
    }
}

TextEditWithCheckboxes * NoteRichTextEditor::getTextEdit()
{
    return m_pUI->noteTextEdit;
}

void NoteRichTextEditor::changeTextColor(const NoteRichTextEditor::EChangeColor changeColorOption)
{
    QColor col = QColorDialog::getColor(getTextEdit()->textColor(), this);
    if (!col.isValid()) {
        return;
    }

    QTextCharFormat format;
    format.setForeground(col);

    switch(changeColorOption)
    {
    case COLOR_ALL:
    {
        QTextCursor cursor = getTextEdit()->textCursor();
        cursor.beginEditBlock();
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(format);
        getTextEdit()->mergeCurrentCharFormat(format);
        cursor.endEditBlock();
        break;
    }
    case COLOR_SELECTED:
    {
        mergeFormatOnWordOrSelection(format);
        break;
    }
    default:
        qDebug() << "Warning: wrong option passed to NoteRichTextEditor::changeTextColor: "
                 << changeColorOption;
        return;
    }
}

void NoteRichTextEditor::insertList(const QTextListFormat::Style style)
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

void NoteRichTextEditor::setupToDoCheckboxTextObjects()
{
    m_pToDoCheckboxTextObjectInterfaceUnchecked = new ToDoCheckboxTextObjectUnchecked;
    m_pToDoCheckboxTextObjectInterfaceChecked   = new ToDoCheckboxTextObjectChecked;

    QAbstractTextDocumentLayout * pLayout = getTextEdit()->document()->documentLayout();
    pLayout->registerHandler(CHECKBOX_TEXT_FORMAT_UNCHECKED, m_pToDoCheckboxTextObjectInterfaceUnchecked);
    pLayout->registerHandler(CHECKBOX_TEXT_FORMAT_CHECKED, m_pToDoCheckboxTextObjectInterfaceChecked);
}
