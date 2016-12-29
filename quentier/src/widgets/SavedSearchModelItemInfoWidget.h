#ifndef QUENTIER_WIDGETS_SAVED_SEARCH_MODEL_ITEM_INFO_WIDGET_H
#define QUENTIER_WIDGETS_SAVED_SEARCH_MODEL_ITEM_INFO_WIDGET_H

#include <quentier/utility/Qt4Helper.h>
#include <QWidget>

namespace Ui {
class SavedSearchModelItemInfoWidget;
}

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SavedSearchModelItem)

class SavedSearchModelItemInfoWidget: public QWidget
{
    Q_OBJECT
public:
    explicit SavedSearchModelItemInfoWidget(const QModelIndex & index,
                                            QWidget * parent = Q_NULLPTR);
    virtual ~SavedSearchModelItemInfoWidget();

private:
    void setCheckboxesReadOnly();

    void setNonSavedSearchModel();
    void setInvalidIndex();
    void setNoModelItem();
    void setSavedSearchItem(const SavedSearchModelItem & item);

    void hideAll();

    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;

private:
    Ui::SavedSearchModelItemInfoWidget * m_pUi;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_SAVED_SEARCH_MODEL_ITEM_INFO_WIDGET_H
