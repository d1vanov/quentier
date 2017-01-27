#ifndef QUENTIER_DELEGATES_FAVORITE_ITEM_DELEGATE_H
#define QUENTIER_DELEGATES_FAVORITE_ITEM_DELEGATE_H

#include "AbstractStyledItemDelegate.h"
#include <QIcon>

namespace quentier {

class FavoriteItemDelegate: public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject * parent = Q_NULLPTR);

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
    QSize favoriteItemNameSizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
    void drawFavoriteItemName(QPainter * painter, const QModelIndex & index, const QStyleOptionViewItem & option) const;

private:
    QIcon   m_notebookIcon;
    QIcon   m_tagIcon;
    QIcon   m_noteIcon;
    QIcon   m_savedSearchIcon;
    QIcon   m_unknownTypeIcon;
    QSize   m_iconSize;
};

} // namespace quentier

#endif // QUENTIER_DELEGATES_FAVORITE_ITEM_DELEGATE_H
