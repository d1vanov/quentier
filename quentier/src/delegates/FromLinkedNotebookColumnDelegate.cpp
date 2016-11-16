#include "FromLinkedNotebookColumnDelegate.h"
#include <QPainter>

FromLinkedNotebookColumnDelegate::FromLinkedNotebookColumnDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_icon(),
    m_iconSize(16, 16)
{
    m_icon.addFile(QStringLiteral(":/user/user.png"), m_iconSize);
}

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

void FromLinkedNotebookColumnDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                             const QModelIndex & index) const
{
    Q_UNUSED(index)

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    bool fromLinkedNotebook = false;

    const QAbstractItemModel * model = index.model();
    if (model)
    {
        QVariant data = model->data(index);

        // The data here might be a string - linked notebook guid - or just a bool
        // indicating whether the corresponding item is from linked notebook or not
        if (data.type() == QVariant::String) {
            fromLinkedNotebook = !data.toString().isEmpty();
        }
        else {
            fromLinkedNotebook = data.toBool();
        }
    }

    if (fromLinkedNotebook) {
        m_icon.paint(painter, option.rect);
    }

    painter->restore();
}

void FromLinkedNotebookColumnDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void FromLinkedNotebookColumnDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                                    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize FromLinkedNotebookColumnDelegate::sizeHint(const QStyleOptionViewItem & option,
                                                 const QModelIndex & index) const
{
    Q_UNUSED(option)

    if (!index.isValid()) {
        return QSize();
    }

    return m_iconSize;
}

void FromLinkedNotebookColumnDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                                            const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}
