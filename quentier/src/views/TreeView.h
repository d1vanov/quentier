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
// NOTE: don't attempt to use QT_VERSION_CHECK here: Qt4 doesn't expand macro functions during moc run which leads
// to the check not working and the code not building
    virtual void dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            )
#else
                            , const QVector<int> & roles = QVector<int>())
#endif
                            Q_DECL_OVERRIDE;
};

#endif // QUENTIER_VIEWS_TREE_VIEW_H
