#include "SynchronizableColumnDelegate.h"
#include <QPainter>
#include <QCheckBox>

#define SIDE_SIZE (8)

SynchronizableColumnDelegate::SynchronizableColumnDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_icon(),
    m_iconSize(SIDE_SIZE, SIDE_SIZE)
{
    m_icon.addFile(QStringLiteral(":/sync_icons/stat_notify_sync.png"), m_iconSize);
}

int SynchronizableColumnDelegate::sideSize() const
{
    return qRound(SIDE_SIZE * 1.25);
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
    Q_UNUSED(option)

    if (Q_UNLIKELY(!index.isValid())) {
        return Q_NULLPTR;
    }

    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return Q_NULLPTR;
    }

    bool synchronizable = model->data(index).toBool();
    if (synchronizable) {
        // The item which is already synchronizable cannot be made non-synchronizable
        return Q_NULLPTR;
    }

    QCheckBox * checkbox = new QCheckBox(parent);
    checkbox->setCheckState(Qt::Unchecked);

    return checkbox;
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
    QCheckBox * checkbox = qobject_cast<QCheckBox*>(editor);
    if (Q_UNLIKELY(!checkbox)) {
        return;
    }

    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return;
    }

    bool synchronizable = model->data(index).toBool();
    checkbox->setCheckState(synchronizable ? Qt::Checked : Qt::Unchecked);
}

void SynchronizableColumnDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                                const QModelIndex & index) const
{
    if (Q_UNLIKELY(!model)) {
        return;
    }

    if (Q_UNLIKELY(!editor)) {
        return;
    }

    QCheckBox * checkbox = qobject_cast<QCheckBox*>(editor);
    if (Q_UNLIKELY(!checkbox)) {
        return;
    }

    bool synchronizable = (checkbox->checkState() == Qt::Checked);
    model->setData(index, synchronizable, Qt::EditRole);
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
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

