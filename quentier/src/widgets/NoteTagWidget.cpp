#include "NoteTagWidget.h"
#include "ui_NoteTagWidget.h"

namespace quentier {

NoteTagWidget::NoteTagWidget(const QString & tagName, QWidget *parent) :
    QWidget(parent),
    m_pUi(new Ui::NoteTagWidget)
{
    m_pUi->setupUi(this);
    setTagName(tagName);
    QObject::connect(m_pUi->pushButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(NoteTagWidget,onRemoveTagButtonPressed));
}

NoteTagWidget::~NoteTagWidget()
{
    delete m_pUi;
}

QString NoteTagWidget::tagName() const
{
    return m_pUi->tagNameLabel->text();
}

void NoteTagWidget::setTagName(const QString & name)
{
    m_pUi->tagNameLabel->setText(name);
}

void NoteTagWidget::onCanCreateTagRestrictionChanged(bool canCreateTag)
{
    m_pUi->pushButton->setHidden(!canCreateTag);
}

void NoteTagWidget::onRemoveTagButtonPressed()
{
    emit removeTagFromNote(m_pUi->tagNameLabel->text());
}

} // namespace quentier
