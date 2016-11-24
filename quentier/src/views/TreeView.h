#ifndef QUENTIER_VIEWS_TREE_VIEW_H
#define QUENTIER_VIEWS_TREE_VIEW_H

#include <quentier/utility/Qt4Helper.h>
#include <QTreeView>

class TreeView: public QTreeView
{
    Q_OBJECT
public:
    explicit TreeView(QWidget * parent = Q_NULLPTR);

public Q_SLOTS:
    virtual void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                            )
#else
                            , const QVector<int> & roles = QVector<int>())
#endif
                            Q_DECL_OVERRIDE;
};

#endif // QUENTIER_VIEWS_TREE_VIEW_H
