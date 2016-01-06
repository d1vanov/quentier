#include "FindAndReplaceWidget.h"
#include "ui_FindAndReplaceWidget.h"

namespace qute_note {

FindAndReplaceWidget::FindAndReplaceWidget(QWidget * parent,
                                           const bool withReplace) :
    QWidget(parent),
    m_pUI(new Ui::FindAndReplaceWidget)
{
    m_pUI->setupUi(this);

    if (!withReplace)
    {
        m_pUI->replaceLineEdit->setHidden(true);
        m_pUI->replaceButton->setHidden(true);
        m_pUI->replaceAllButton->setHidden(true);
    }
}

FindAndReplaceWidget::~FindAndReplaceWidget()
{
    delete m_pUI;
}

void FindAndReplaceWidget::onCloseButtonPressed()
{
    emit closed();
    Q_UNUSED(close());
}

void FindAndReplaceWidget::onFindTextEntered(const QString & textToFind)
{
    if (textToFind.isEmpty()) {
        return;
    }

    emit findNext(textToFind, m_pUI->matchCaseCheckBox->isChecked());
}

void FindAndReplaceWidget::onNextButtonPressed()
{
    onFindTextEntered(m_pUI->findLineEdit->text());
}

void FindAndReplaceWidget::onPreviusButtonPressed()
{
    QString textToFind = m_pUI->findLineEdit->text();
    if (textToFind.isEmpty()) {
        return;
    }

    emit findPrevious(textToFind, m_pUI->matchCaseCheckBox);
}

void FindAndReplaceWidget::onMatchCaseCheckboxToggled()
{
    // TODO: implement
}

void FindAndReplaceWidget::onReplaceTextEntered(const QString & replacementText)
{
    // TODO: implement
    Q_UNUSED(replacementText);
}

void FindAndReplaceWidget::onReplaceButtonPressed()
{
    // TODO: implement
}

void FindAndReplaceWidget::onReplaceAllButtonPressed()
{
    // TODO: implement
}

} // namespace qute_note
