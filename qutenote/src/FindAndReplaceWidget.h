#ifndef __QUTE_NOTE__NOTE_EDITOR__FIND_AND_REPLACE_WIDGET_H
#define __QUTE_NOTE__NOTE_EDITOR__FIND_AND_REPLACE_WIDGET_H

#include <qute_note/utility/Qt4Helper.h>
#include <QWidget>
#include <QString>

namespace Ui {
class FindAndReplaceWidget;
}

namespace qute_note {

class FindAndReplaceWidget: public QWidget
{
    Q_OBJECT
public:
    explicit FindAndReplaceWidget(QWidget * parent = Q_NULLPTR,
                                  const bool withReplace = false);
    virtual ~FindAndReplaceWidget();

Q_SIGNALS:
    void closed();
    void findNext(const QString & textToFind, const bool matchCase);
    void findPrevious(const QString & textToFind, const bool matchCase);
    void searchCaseSensitivityChanged(const bool matchCase);
    void replace(const QString & replacementText);
    void replaceAll(const QString & replacementText);

public Q_SLOTS:
    void setReplaceEnabled(const bool enabled);

private Q_SLOTS:
    void onCloseButtonPressed();
    void onFindTextEntered();
    void onNextButtonPressed();
    void onPreviousButtonPressed();
    void onMatchCaseCheckboxToggled(int state);
    void onReplaceTextEntered();
    void onReplaceButtonPressed();
    void onReplaceAllButtonPressed();

private:
    void createConnections();

private:
    Ui::FindAndReplaceWidget *  m_pUI;
};

} // namespace qute_note

#endif // __QUTE_NOTE__NOTE_EDITOR__FIND_AND_REPLACE_WIDGET_H
