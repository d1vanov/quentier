#include "FromLinkedNotebookColumnDelegate.h"

FromLinkedNotebookColumnDelegate::FromLinkedNotebookColumnDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

QString FromLinkedNotebookColumnDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)
    return QString();
}

QWidget * FromLinkedNotebookColumnDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                                         const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void FromLinkedNotebookColumnDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

void FromLinkedNotebookColumnDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void FromLinkedNotebookColumnDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize FromLinkedNotebookColumnDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize();
}

void FromLinkedNotebookColumnDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                                            const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}
