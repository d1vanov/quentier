#ifndef QUENTIER_WIDGETS_NOTEBOOK_MODEL_ITEM_INFO_WIDGET_H
#define QUENTIER_WIDGETS_NOTEBOOK_MODEL_ITEM_INFO_WIDGET_H

#include <quentier/utility/Qt4Helper.h>
#include <QWidget>

namespace Ui {
class NotebookModelItemInfoWidget;
}

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookItem)
QT_FORWARD_DECLARE_CLASS(NotebookStackItem)

class NotebookModelItemInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NotebookModelItemInfoWidget(const QModelIndex & index,
                                         QWidget * parent = Q_NULLPTR);
    virtual ~NotebookModelItemInfoWidget();

private:
    void setCheckboxesReadOnly();

    void setNonNotebookModel();
    void setInvalidIndex();
    void setNoModelItem();
    void setMessedUpModelItemType();

    void hideAll();

    void hideNotebookStuff();
    void showNotebookStuff();
    void setNotebookStuffHidden(const bool flag);

    void hideStackStuff();
    void showStackStuff();
    void setStackStuffHidden(const bool flag);

    void setNotebookItem(const NotebookItem & item);
    void setStackItem(const NotebookStackItem & item, const int numChildren);

private:
    Ui::NotebookModelItemInfoWidget * m_pUi;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTEBOOK_MODEL_ITEM_INFO_WIDGET_H
