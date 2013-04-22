#include "NoteRichTextEditor.h"
#include "ui_NoteRichTextEditor.h"

NoteRichTextEditor::NoteRichTextEditor(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::NoteRichTextEditor)
{
    m_pUI->setupUi(this);
    QObject::connect(m_pUI->buttonFormatTextBold, SIGNAL(clicked()), this, SLOT(textBold()));
    QObject::connect(m_pUI->buttonFormatTextItalic, SIGNAL(clicked()), this, SLOT(textItalic()));
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

void NoteRichTextEditor::mergeFormatOnWordOrSelection(const QTextCharFormat & format)
{
    QTextEdit * noteTextEdit = getTextEdit();
    QTextCursor cursor = noteTextEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    noteTextEdit->mergeCurrentCharFormat(format);
}

QTextEdit * NoteRichTextEditor::getTextEdit()
{
    return m_pUI->noteTextEdit;
}
