#include "TreeView.h"

TreeView::TreeView(QWidget * parent) :
    QTreeView(parent)
{}

void TreeView::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                           )
#else
                           , const QVector<int> & roles)
#endif
{
    QTreeView::dataChanged(topLeft, bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                           );
#else
                           , roles);
#endif
}
