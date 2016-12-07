#include "TableView.h"

TableView::TableView(QWidget * parent) :
    QTableView(parent)
{}

void TableView::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                           )
#else
                           , const QVector<int> & roles)
#endif
{
    QTableView::dataChanged(topLeft, bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                           );
#else
                           , roles);
#endif

    // NOTE: the table views used in Quentier currently have automatic column width adjustment;
    // As the default implementation doesn't seem to really care much about data changes,
    // let's force it to do so
    if (Q_UNLIKELY(!topLeft.isValid() || !bottomRight.isValid())) {
        return;
    }

    int minColumn = topLeft.column();
    int maxColumn = bottomRight.column();
    for(int i = minColumn; i <= maxColumn; ++i) {
        resizeColumnToContents(i);
    }
}
