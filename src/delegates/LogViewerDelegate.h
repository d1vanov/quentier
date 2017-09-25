#ifndef QUENTIER_DELEGATES_LOG_VIEWER_DELEGATE_H
#define QUENTIER_DELEGATES_LOG_VIEWER_DELEGATE_H

#include "AbstractStyledItemDelegate.h"

namespace quentier {

class LogViewerDelegate: public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    LogViewerDelegate(QObject * parent = Q_NULLPTR);

private:
    // QStyledItemDelegate interface
    virtual QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                   const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option,
                       const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // QUENTIER_DELEGATES_LOG_VIEWER_DELEGATE_H
