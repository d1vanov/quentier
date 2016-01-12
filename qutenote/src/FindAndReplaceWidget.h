#ifndef __QUTE_NOTE__NOTE_EDITOR__FIND_AND_REPLACE_WIDGET_H
#define __QUTE_NOTE__NOTE_EDITOR__FIND_AND_REPLACE_WIDGET_H

#include <qute_note/utility/Qt4Helper.h>
#include <QWidget>
#include <QString>

namespace Ui {
class FindAndReplaceWidget;
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ShortcutManager)

class FindAndReplaceWidget: public QWidget
{
    Q_OBJECT
public:
    explicit FindAndReplaceWidget(QWidget * parent = Q_NULLPTR,
                                  const bool withReplace = false);
    virtual ~FindAndReplaceWidget();

    QString textToFind() const;
    void setTextToFind(const QString & text);

    QString replacementText() const;
    void setReplacementText(const QString & text);

    bool matchCase() const;
    void setMatchCase(const bool flag);

    bool replaceEnabled() const;
    void setReplaceEnabled(const bool enabled);

Q_SIGNALS:
    void closed();
    void textToFindEdited(const QString & textToFind);
    void findNext(const QString & textToFind, const bool matchCase);
    void findPrevious(const QString & textToFind, const bool matchCase);
    void searchCaseSensitivityChanged(const bool matchCase);
    void replace(const QString & textToReplace, const QString & replacementText, const bool matchCase);
    void replaceAll(const QString & textToReplace, const QString & replacementText, const bool matchCase);

public Q_SLOTS:
    void setupShortcuts(const ShortcutManager & shortcutManager);

    void setFocus();
    void show();

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
