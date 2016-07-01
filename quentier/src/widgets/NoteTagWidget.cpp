#include "NoteTagWidget.h"
#include "ui_NoteTagWidget.h"

NoteTagWidget::NoteTagWidget(QWidget *parent) :
    QWidget(parent),
    m_pUi(new Ui::NoteTagWidget),
    m_tagName()
{
    m_pUi->setupUi(this);
    QObject::connect(m_pUi->pushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteTagWidget,onRemoveTagButtonPressed));
}

NoteTagWidget::~NoteTagWidget()
{
    delete m_pUi;
}

const QString & NoteTagWidget::tagName() const
{
    return m_pUi->tagNameLabel->text();
}

void NoteTagWidget::setTagName(const QString & name)
{
    m_pUi->tagNameLabel->setText(name);
}

void NoteTagWidget::onRemoveTagButtonPressed()
{
    emit removeTagFromNote(m_pUi->tagNameLabel->text());
}
