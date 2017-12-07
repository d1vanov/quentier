#ifndef QUENTIER_DELEGATES_SYNCHRONIZABLE_COLUMN_DELEGATE_H
#define QUENTIER_DELEGATES_SYNCHRONIZABLE_COLUMN_DELEGATE_H

#include <quentier/utility/Macros.h>
#include <QStyledItemDelegate>
#include <QIcon>

class SynchronizableColumnDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SynchronizableColumnDelegate(QObject * parent = Q_NULLPTR);

    int sideSize() const;

private:
    // QStyledItemDelegate interface
    virtual QString displayText(const QVariant & value, const QLocale & locale) const Q_DECL_OVERRIDE;
    virtual QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                   const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option,
                       const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual void setEditorData(QWidget * editor, const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual void setModelData(QWidget * editor, QAbstractItemModel * model,
                              const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual void updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                      const QModelIndex & index) const Q_DECL_OVERRIDE;

private:
    QIcon   m_icon;
    QSize   m_iconSize;
};

#endif // QUENTIER_DELEGATES_SYNCHRONIZABLE_COLUMN_DELEGATE_H
