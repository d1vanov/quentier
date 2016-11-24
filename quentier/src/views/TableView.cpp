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
}
