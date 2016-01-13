#include "FindAndReplaceWidget.h"
#include "ui_FindAndReplaceWidget.h"

namespace qute_note {

FindAndReplaceWidget::FindAndReplaceWidget(QWidget * parent,
                                           const bool withReplace) :
    QWidget(parent),
    m_pUI(new Ui::FindAndReplaceWidget)
{
    m_pUI->setupUi(this);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    m_pUI->findLineEdit->setClearButtonEnabled(true);
#endif

    m_pUI->findLineEdit->setDragEnabled(true);
    m_pUI->replaceLineEdit->setDragEnabled(true);

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

void FindAndReplaceWidget::setTextToFind(const QString & text)
{
    m_pUI->findLineEdit->setText(text);
}

QString FindAndReplaceWidget::replacementText() const
{
    return m_pUI->replaceLineEdit->text();
}

void FindAndReplaceWidget::setReplacementText(const QString & text)
{
    m_pUI->replaceLineEdit->setText(text);
}

bool FindAndReplaceWidget::matchCase() const
{
    return m_pUI->matchCaseCheckBox->isChecked();
}

void FindAndReplaceWidget::setMatchCase(const bool flag)
{
    m_pUI->matchCaseCheckBox->setChecked(flag);
}

bool FindAndReplaceWidget::replaceEnabled() const
{
    return !m_pUI->replaceLineEdit->isHidden();
}

void FindAndReplaceWidget::setReplaceEnabled(const bool enabled)
{
    if (enabled)
    {
        m_pUI->replaceLineEdit->setDisabled(false);
        m_pUI->replaceButton->setDisabled(false);
        m_pUI->replaceAllButton->setDisabled(false);
        m_pUI->replaceLabel->setDisabled(false);

        m_pUI->replaceLineEdit->show();
        m_pUI->replaceButton->show();
        m_pUI->replaceAllButton->show();
        m_pUI->replaceLabel->show();
    }
    else
    {
        m_pUI->replaceLineEdit->hide();
        m_pUI->replaceButton->hide();
        m_pUI->replaceAllButton->hide();
        m_pUI->replaceLabel->hide();

        m_pUI->replaceLineEdit->setDisabled(true);
        m_pUI->replaceButton->setDisabled(true);
        m_pUI->replaceAllButton->setDisabled(true);
        m_pUI->replaceLabel->setDisabled(true);
    }
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
    setReplaceEnabled(false);
    hide();
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

    emit findPrevious(textToFind, m_pUI->matchCaseCheckBox->isChecked());
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

QSize FindAndReplaceWidget::sizeHint() const
{
    bool minimal = false;
    return sizeHintImpl(minimal);
}

QSize FindAndReplaceWidget::minimumSizeHint() const
{
    bool minimal = true;
    return sizeHintImpl(minimal);
}

void FindAndReplaceWidget::createConnections()
{
    QObject::connect(m_pUI->closeButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onCloseButtonPressed));
    QObject::connect(m_pUI->findLineEdit, QNSIGNAL(QLineEdit,textEdited,const QString&),
                     this, QNSIGNAL(FindAndReplaceWidget,textToFindEdited,const QString&));
    QObject::connect(m_pUI->findLineEdit, QNSIGNAL(QLineEdit,returnPressed),
                     this, QNSLOT(FindAndReplaceWidget,onFindTextEntered));
    QObject::connect(m_pUI->findNextButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onNextButtonPressed));
    QObject::connect(m_pUI->findPreviousButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onPreviousButtonPressed));
    QObject::connect(m_pUI->matchCaseCheckBox, QNSIGNAL(QCheckBox,stateChanged,int),
                     this, QNSLOT(FindAndReplaceWidget,onMatchCaseCheckboxToggled,int));
    QObject::connect(m_pUI->replaceLineEdit, QNSIGNAL(QLineEdit,returnPressed),
                     this, QNSLOT(FindAndReplaceWidget,onReplaceTextEntered));
    QObject::connect(m_pUI->replaceButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onReplaceButtonPressed));
    QObject::connect(m_pUI->replaceAllButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(FindAndReplaceWidget,onReplaceAllButtonPressed));
}

QSize FindAndReplaceWidget::sizeHintImpl(const bool minimal) const
{
    QSize sizeHint = (minimal ? QWidget::minimumSizeHint() : QWidget::sizeHint());

    if (!m_pUI->replaceLineEdit->isEnabled())
    {
        QSize findSizeHint = (minimal
                              ? m_pUI->findLineEdit->minimumSizeHint()
                              : m_pUI->findLineEdit->sizeHint());
        QLayout * pLayout = m_pUI->gridLayout->layout();
        QMargins margins = pLayout->contentsMargins();

        sizeHint.setHeight(findSizeHint.height() + margins.top() + margins.bottom());
    }

    return sizeHint;
}

} // namespace qute_note
