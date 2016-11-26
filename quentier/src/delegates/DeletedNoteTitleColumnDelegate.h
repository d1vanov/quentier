#ifndef QUENTIER_DELEGATES_DELETED_NOTE_TITLE_COLUMN_DELEGATE_H
#define QUENTIER_DELEGATES_DELETED_NOTE_TITLE_COLUMN_DELEGATE_H

#include <quentier/utility/Qt4Helper.h>
#include <QStyledItemDelegate>

/**
 * @brief The DeletedNoteTitleColumnDelegate class represents the column delegate for the deleted notes table view
 * which uses deleted note's preview text if its title is not set; if the preview text is also empty, the replacement text
 * is displayed
 */
class DeletedNoteTitleColumnDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DeletedNoteTitleColumnDelegate(QObject * parent = Q_NULLPTR);

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
    QString     m_titleReplacementText;
};

#endif // QUENTIER_DELEGATES_DELETED_NOTE_TITLE_COLUMN_DELEGATE_H
