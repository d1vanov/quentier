#include "NoteRichTextEditor.h"
#include "ui_NoteRichTextEditor.h"

NoteRichTextEditor::NoteRichTextEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NoteRichTextEditor)
{
    ui->setupUi(this);
}

NoteRichTextEditor::~NoteRichTextEditor()
{
    delete ui;
}
