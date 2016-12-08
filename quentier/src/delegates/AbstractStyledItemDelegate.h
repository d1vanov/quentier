#ifndef QUENTIER_DELEGATES_ABSTRACT_STYLED_ITEM_DELEGATE_H
#define QUENTIER_DELEGATES_ABSTRACT_STYLED_ITEM_DELEGATE_H

#include <quentier/utility/Qt4Helper.h>
#include <QStyledItemDelegate>

namespace quentier {

/**
 * @brief The AbstractStyledItemDelegate adds some bits of functionality useful
 * for almost all delegates used in Quentier app
 */
class AbstractStyledItemDelegate: public QStyledItemDelegate
{
    Q_OBJECT
protected:
    explicit AbstractStyledItemDelegate(QObject * parent = Q_NULLPTR);

public:
    virtual ~AbstractStyledItemDelegate();

protected:
    /**
     * Returns the approximate width of the column name, with a tiny bit of margin space, so that the caller
     * can find out where the stuff can be placed within the column to be relatively close horizontally
     * to the column name center and/or boundaries
     * @param option - style option from which the font is determined
     * @param index - the model index of the item for which column the name width is needed
     * @param orientation - horizontal or vertical, by default the horizontal one is used
     * @return the column name width or negative value if that width could not be determined
     */
    int columnNameWidth(const QStyleOptionViewItem & option, const QModelIndex & index,
                        const Qt::Orientation orientation = Qt::Horizontal) const;
};

} // namespace quentier

#endif // QUENTIER_DELEGATES_ABSTRACT_STYLED_ITEM_DELEGATE_H
