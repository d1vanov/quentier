#include "FindAndReplaceWidget.h"
#include "ui_FindAndReplaceWidget.h"
#include <qute_note/utility/ShortcutManager.h>

namespace qute_note {

FindAndReplaceWidget::FindAndReplaceWidget(QWidget * parent,
                                           const bool withReplace) :
    QWidget(parent),
    m_pUI(new Ui::FindAndReplaceWidget)
{
    m_pUI->setupUi(this);
    setReplaceEnabled(withReplace);
    createConnections();
}

FindAndReplaceWidget::~FindAndReplaceWidget()
{
    delete m_pUI;
}

QString FindAndReplaceWidget::textToFind() const
{
    return m_pUI->findLineEdit->text();
}

QString FindAndReplaceWidget::replacementText() const
{
    return m_pUI->replaceLineEdit->text();
}

bool FindAndReplaceWidget::matchCase() const
{
    return m_pUI->matchCaseCheckBox->isChecked();
}

void FindAndReplaceWidget::setReplaceEnabled(const bool enabled)
{
    m_pUI->replaceLineEdit->setHidden(!enabled);
    m_pUI->replaceButton->setHidden(!enabled);
    m_pUI->replaceAllButton->setHidden(!enabled);
}

void FindAndReplaceWidget::setupShortcuts(const ShortcutManager & shortcutManager)
{
    m_pUI->closeButton->setShortcut(Qt::Key_Escape);
    m_pUI->findNextButton->setShortcut(shortcutManager.shortcut(QKeySequence::FindNext));
    m_pUI->findPreviousButton->setShortcut(shortcutManager.shortcut(QKeySequence::FindPrevious));
    m_pUI->replaceButton->setShortcut(shortcutManager.shortcut(QKeySequence::Replace));
}

void FindAndReplaceWidget::setFocus()
{
    m_pUI->findLineEdit->setFocus();
}

void FindAndReplaceWidget::show()
{
    QWidget::show();
    setFocus();
}

void FindAndReplaceWidget::onCloseButtonPressed()
{
    emit closed();
    setHidden(true);
}

void FindAndReplaceWidget::onFindTextEntered()
{
    QString textToFind = m_pUI->findLineEdit->text();
    if (textToFind.isEmpty()) {
        return;
    }

    emit findNext(textToFind, m_pUI->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onNextButtonPressed()
{
    onFindTextEntered();
}

void FindAndReplaceWidget::onPreviousButtonPressed()
{
    QString textToFind = m_pUI->findLineEdit->text();
    if (textToFind.isEmpty()) {
        return;
    }

    emit findPrevious(textToFind, m_pUI->matchCaseCheckBox);
}

void FindAndReplaceWidget::onMatchCaseCheckboxToggled(int state)
{
    emit searchCaseSensitivityChanged(static_cast<bool>(state));
}

void FindAndReplaceWidget::onReplaceTextEntered()
{
    emit replace(m_pUI->findLineEdit->text(), m_pUI->replaceLineEdit->text(), m_pUI->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onReplaceButtonPressed()
{
    onReplaceTextEntered();
}

void FindAndReplaceWidget::onReplaceAllButtonPressed()
{
    emit replaceAll(m_pUI->findLineEdit->text(), m_pUI->replaceLineEdit->text(), m_pUI->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::createConnections()
{
    QObject::connect(m_pUI->closeButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onCloseButtonPressed));
    QObject::connect(m_pUI->findLineEdit, QNSIGNAL(QLineEdit,textEdited,const QString&),
                     this, QNSIGNAL(FindAndReplaceWidget,textToFindEdited,const QString&));
    QObject::connect(m_pUI->findLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(FindAndReplaceWidget,onFindTextEntered));
    QObject::connect(m_pUI->findNextButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onNextButtonPressed));
    QObject::connect(m_pUI->findPreviousButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onPreviousButtonPressed));
    QObject::connect(m_pUI->matchCaseCheckBox, QNSIGNAL(QCheckBox,stateChanged,int),
                     this, QNSLOT(FindAndReplaceWidget,onMatchCaseCheckboxToggled,int));
    QObject::connect(m_pUI->replaceLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(FindAndReplaceWidget,onReplaceTextEntered));
    QObject::connect(m_pUI->replaceButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onReplaceButtonPressed));
    QObject::connect(m_pUI->replaceAllButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onReplaceAllButtonPressed));
}

} // namespace qute_note
