#include "FavoriteItemDelegate.h"
#include "../models/FavoritesModel.h"
#include "../models/FavoritesModelItem.h"
#include <QPainter>
#include <QTextDocument>

#define ICON_SIDE_SIZE (16)

using namespace quentier;

FavoriteItemDelegate::FavoriteItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_notebookIcon(QIcon::fromTheme(QStringLiteral("x-office-address-book"))),
    m_tagIcon(),
    m_noteIcon(QIcon::fromTheme(QStringLiteral("x-office-document"))),
    m_savedSearchIcon(QIcon::fromTheme(QStringLiteral("edit-find"))),
    m_unknownTypeIcon(QIcon::fromTheme(QStringLiteral("image-missing"))),
    m_iconSize(ICON_SIDE_SIZE, ICON_SIDE_SIZE),
    m_doc()
{
    m_tagIcon.addFile(QStringLiteral(":/label/tag.png"), m_iconSize);
}

int FavoriteItemDelegate::sideSize() const
{
    return ICON_SIDE_SIZE;
}

QString FavoriteItemDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)
    return QString();
}

QWidget * FavoriteItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                             const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void FavoriteItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                 const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    int column = index.column();
    if (column == FavoritesModel::Columns::Type)
    {
        QVariant type = index.model()->data(index);
        bool conversionResult = false;
        int typeInt = type.toInt(&conversionResult);
        if (conversionResult)
        {
            switch(typeInt)
            {
            case FavoritesModelItem::Type::Notebook:
                m_notebookIcon.paint(painter, option.rect);
                break;
            case FavoritesModelItem::Type::Tag:
                m_tagIcon.paint(painter, option.rect);
                break;
            case FavoritesModelItem::Type::Note:
                m_noteIcon.paint(painter, option.rect);
                break;
            case FavoritesModelItem::Type::SavedSearch:
                m_savedSearchIcon.paint(painter, option.rect);
                break;
            default:
                m_unknownTypeIcon.paint(painter, option.rect);
                break;
            }
        }
    }
    else if (column == FavoritesModel::Columns::DisplayName)
    {
        const QAbstractItemModel * model = index.model();
        QModelIndex numItemsIndex = model->index(index.row(),
                                                 FavoritesModel::Columns::NumNotesTargeted,
                                                 index.parent());
        QVariant numItems = model->data(numItemsIndex);
        bool conversionResult = false;
        int numItemsInt = numItems.toInt(&conversionResult);

        QString displayName = model->data(index).toString();
        if (!displayName.isEmpty())
        {
            if (conversionResult && (numItemsInt != 0)) {
                displayName += QStringLiteral(" ");
                displayName += QStringLiteral("<font color=\"gray\"");
                displayName += QStringLiteral(" (");
                displayName += QString::number(numItemsInt);
                displayName += QStringLiteral(")");
                displayName += QStringLiteral("</font>");
            }

            m_doc.setHtml(displayName);
            m_doc.drawContents(painter);
        }
    }

    painter->restore();
}

void FavoriteItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void FavoriteItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                        const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize FavoriteItemDelegate::sizeHint(const QStyleOptionViewItem & option,
                                     const QModelIndex & index) const
{
    int column = index.column();
    if (column == FavoritesModel::Columns::Type) {
        return m_iconSize;
    }
    else {
        return QStyledItemDelegate::sizeHint(option, index);
    }
}

void FavoriteItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                                const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}
