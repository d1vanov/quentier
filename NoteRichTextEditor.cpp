#include "NoteRichTextEditor.h"
#include "ui_NoteRichTextEditor.h"
#include <QTextList>
#include <QDebug>

NoteRichTextEditor::NoteRichTextEditor(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::NoteRichTextEditor)
{
    m_pUI->setupUi(this);
    QObject::connect(m_pUI->buttonFormatTextBold, SIGNAL(clicked()), this, SLOT(textBold()));
    QObject::connect(m_pUI->buttonFormatTextItalic, SIGNAL(clicked()), this, SLOT(textItalic()));
    QObject::connect(m_pUI->buttonFormatTextUnderlined, SIGNAL(clicked()), this, SLOT(textUnderline()));
    QObject::connect(m_pUI->buttonFormatTextStrikethrough, SIGNAL(clicked()), this, SLOT(textStrikeThrough()));
    QObject::connect(m_pUI->buttonFormatTextAlignLeft, SIGNAL(clicked()), this, SLOT(textAlignLeft()));
    QObject::connect(m_pUI->buttonFormatTextAlignCenter, SIGNAL(clicked()), this, SLOT(textAlignCenter()));
    QObject::connect(m_pUI->buttonFormatTextAlignRight, SIGNAL(clicked()), this, SLOT(textAlignRight()));
    QObject::connect(m_pUI->buttonIndentIncrease, SIGNAL(clicked()), this, SLOT(textIncreaseIndentation()));
    QObject::connect(m_pUI->buttonIndentDecrease, SIGNAL(clicked()), this, SLOT(textDecreaseIndentation()));
}

NoteRichTextEditor::~NoteRichTextEditor()
{
    delete m_pUI;
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
