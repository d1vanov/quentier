#ifndef QUENTIER_WIDGETS_TAG_MODEL_ITEM_INFO_WIDGET_H
#define QUENTIER_WIDGETS_TAG_MODEL_ITEM_INFO_WIDGET_H

#include <quentier/utility/Qt4Helper.h>
#include <QWidget>

namespace Ui {
class TagModelItemInfoWidget;
}

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModelItem)

class TagModelItemInfoWidget: public QWidget
{
    Q_OBJECT
public:
    explicit TagModelItemInfoWidget(const QModelIndex & index,
                                    QWidget * parent = Q_NULLPTR);
    virtual ~TagModelItemInfoWidget();

private:
    void setCheckboxesReadOnly();

    void setNonTagModel();
    void setInvalidIndex();
    void setNoModelItem();
    void setTagItem(const TagModelItem & item);

    void hideAll();

    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;

private:
    Ui::TagModelItemInfoWidget * m_pUi;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_TAG_MODEL_ITEM_INFO_WIDGET_H
