#include "NoteRichTextEditor.h"
#include "ui_NoteRichTextEditor.h"

NoteRichTextEditor::NoteRichTextEditor(QWidget *parent) :
    QWidget(parent),
    m_pUI(new Ui::NoteRichTextEditor)
{
    m_pUI->setupUi(this);
}

NoteRichTextEditor::~NoteRichTextEditor()
{
    delete m_pUI;
}
