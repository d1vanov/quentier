#ifndef QUENTIER_DELEGATES_ABSTRACT_STYLED_ITEM_DELEGATE_H
#define QUENTIER_DELEGATES_ABSTRACT_STYLED_ITEM_DELEGATE_H

#include <quentier/utility/Macros.h>
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

    /**
     * @brief adjusts (shortens, elides) the text to be displayed by the item according to the option's rect width;
     * if no adjustment is required, the displayed text is left untouched
     *
     * @param displayedText - the text to be displayed which might need to be shortened
     * @param option - the used style option
     * @param textSuffix - the text suffix that needs to be displayed nevertheless, even if the primary part of the text
     * exceeds the width of the option's rect
     */
    void adjustDisplayedText(QString & displayedText, const QStyleOptionViewItem & option,
                             const QString & nameSuffix = QString()) const;
};

} // namespace quentier

#endif // QUENTIER_DELEGATES_ABSTRACT_STYLED_ITEM_DELEGATE_H
