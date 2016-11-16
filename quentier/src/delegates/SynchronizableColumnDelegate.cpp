#include "SynchronizableColumnDelegate.h"
#include <QPainter>

SynchronizableColumnDelegate::SynchronizableColumnDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_icon(),
    m_iconSize(8, 8)
{
    m_icon.addFile(QStringLiteral(":/sync_icons/stat_notify_sync.png"), m_iconSize);
}

QString SynchronizableColumnDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)
    return QString();
}

QWidget * SynchronizableColumnDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                                     const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void SynchronizableColumnDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                         const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    bool synchronizable = false;

    const QAbstractItemModel * model = index.model();
    if (model) {
        synchronizable = model->data(index).toBool();
    }

    if (synchronizable) {
        m_icon.paint(painter, option.rect);
    }

    painter->restore();
}

void SynchronizableColumnDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void SynchronizableColumnDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                                const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize SynchronizableColumnDelegate::sizeHint(const QStyleOptionViewItem & option,
                                             const QModelIndex & index) const
{
    Q_UNUSED(option)

    if (Q_UNLIKELY(!index.isValid())) {
        return QSize();
    }

    return m_iconSize;
}

void SynchronizableColumnDelegate::updateEditorGeometry(QWidget * editor,
                                                        const QStyleOptionViewItem & option,
                                                        const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

