#include "NoteRichTextEditor.h"
#include "ui_NoteRichTextEditor.h"
#include "ToDoCheckboxTextObject.h"
#include <QTextList>
#include <QColorDialog>
#include <QKeyEvent>
#include <QCheckBox>
#include <QDebug>

NoteRichTextEditor::NoteRichTextEditor(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::NoteRichTextEditor)
{
    m_pUI->setupUi(this);
    setupToDoCheckboxTextObject();

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
}

NoteRichTextEditor::~NoteRichTextEditor()
{
    delete m_pUI;
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
    getTextEdit()->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    setAlignButtonsCheckedState(ALIGNED_LEFT);
}

void NoteRichTextEditor::textAlignCenter()
{
    getTextEdit()->setAlignment(Qt::AlignHCenter);
    setAlignButtonsCheckedState(ALIGNED_CENTER);
}

void NoteRichTextEditor::textAlignRight()
{
    getTextEdit()->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    setAlignButtonsCheckedState(ALIGNED_RIGHT);
}

void NoteRichTextEditor::textIncreaseIndentation()
{
    changeIndentation(true);
}

void NoteRichTextEditor::textDecreaseIndentation()
{
    changeIndentation(false);
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
    QTextCharFormat toDoCheckboxCharFormat;
    toDoCheckboxCharFormat.setObjectType(ToDoCheckboxTextObject::CheckboxTextFormat);

    QCheckBox * pCheckbox = new QCheckBox;
    pCheckbox->setChecked(false);
    pCheckbox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    qDebug() << "Size of checkbox object: " << pCheckbox->size();
    QImage toDoCheckboxImage(pCheckbox->size(), QImage::Format_RGB32);
    pCheckbox->render(&toDoCheckboxImage);  // FIXME: looks like rendering doesn't go good

    // FIXME: for debug purposes, remove before merging to master
    m_pUI->label->setPixmap(QPixmap::fromImage(toDoCheckboxImage));

    delete pCheckbox;
    pCheckbox = nullptr;

    toDoCheckboxCharFormat.setProperty(ToDoCheckboxTextObject::CheckboxTextData, toDoCheckboxImage);

    QTextCursor cursor = getTextEdit()->textCursor();
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), toDoCheckboxCharFormat);
    getTextEdit()->setTextCursor(cursor);
}

void NoteRichTextEditor::mergeFormatOnWordOrSelection(const QTextCharFormat & format)
{
    QTextEdit * noteTextEdit = getTextEdit();
    QTextCursor cursor = noteTextEdit->textCursor();

    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }

    cursor.mergeCharFormat(format);
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

void NoteRichTextEditor::changeIndentation(const bool increase)
{
    QTextCursor cursor = getTextEdit()->textCursor();
    if (cursor.currentList())
    {
        QTextListFormat listFormat = cursor.currentList()->format();

        if (increase) {
            listFormat.setIndent(listFormat.indent() + 1);
        }
        else {
            listFormat.setIndent(std::max(listFormat.indent() - 1, 0));
        }

        cursor.beginEditBlock();
        cursor.createList(listFormat);
        cursor.endEditBlock();
    }
    else
    {
        int start = cursor.anchor();
        int end = cursor.position();
        if (start > end)
        {
            start = cursor.position();
            end = cursor.anchor();
        }

        QList<QTextBlock> blocks;
        QTextBlock b = getTextEdit()->document()->begin();
        while (b.isValid())
        {
            b = b.next();
            if ( ((b.position() >= start) && (b.position() + b.length() <= end) ) ||
                 b.contains(start) || b.contains(end) )
            {
                blocks << b;
            }
        }

        foreach(QTextBlock b, blocks)
        {
            QTextCursor c(b);
            QTextBlockFormat bf = c.blockFormat();

            if (increase) {
                bf.setIndent(bf.indent() + 1);
            }
            else {
                bf.setIndent(std::max(bf.indent() - 1, 0));
            }

            c.setBlockFormat(bf);
        }
    }
}

QTextEdit * NoteRichTextEditor::getTextEdit()
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
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(format);
        getTextEdit()->mergeCurrentCharFormat(format);
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

void NoteRichTextEditor::setupToDoCheckboxTextObject()
{
    QObject * toDoCheckboxTextObjectInterface = new ToDoCheckboxTextObject;
    getTextEdit()->document()->documentLayout()->registerHandler(ToDoCheckboxTextObject::CheckboxTextFormat,
                                                                 toDoCheckboxTextObjectInterface);
}
