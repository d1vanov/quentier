#include "FavoriteItemDelegate.h"
#include "../models/FavoritesModel.h"
#include "../models/FavoritesModelItem.h"
#include "../models/NotebookModel.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>

#define ICON_SIDE_SIZE (16)

using namespace quentier;

FavoriteItemDelegate::FavoriteItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_notebookIcon(QIcon::fromTheme(QStringLiteral("x-office-address-book"))),
    m_tagIcon(),
    m_noteIcon(QIcon::fromTheme(QStringLiteral("x-office-document"))),
    m_savedSearchIcon(QIcon::fromTheme(QStringLiteral("edit-find"))),
    m_unknownTypeIcon(QIcon::fromTheme(QStringLiteral("image-missing"))),
    m_iconSize(ICON_SIDE_SIZE, ICON_SIDE_SIZE)
{
    m_tagIcon.addFile(QStringLiteral(":/tag/tag.png"), m_iconSize);
}

int FavoriteItemDelegate::sideSize() const
{
    return ICON_SIDE_SIZE;
}

QString FavoriteItemDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    return QStyledItemDelegate::displayText(value, locale);
}

QWidget * FavoriteItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                             const QModelIndex & index) const
{
    // Only allow to edit the favorited item's display name but no other favorites model columns
    if (index.isValid() && (index.column() == FavoritesModel::Columns::DisplayName)) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    else {
        return Q_NULLPTR;
    }
}

void FavoriteItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                 const QModelIndex & index) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return;
    }

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    int column = index.column();
    if (column == FavoritesModel::Columns::Type)
    {
        QVariant type = model->data(index);
        bool conversionResult = false;
        int typeInt = type.toInt(&conversionResult);
        if (conversionResult)
        {
            if (option.state & QStyle::State_Selected) {
                painter->fillRect(option.rect, option.palette.highlight());
            }

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
        drawFavoriteItemName(painter, index, option);
    }

    painter->restore();
}

void FavoriteItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == FavoritesModel::Columns::DisplayName)) {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void FavoriteItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                        const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == FavoritesModel::Columns::DisplayName)) {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize FavoriteItemDelegate::sizeHint(const QStyleOptionViewItem & option,
                                     const QModelIndex & index) const
{
    int column = index.column();
    if (column == FavoritesModel::Columns::Type) {
        QSize size = m_iconSize;
        size.setWidth(size.width() + 15);    // Some margin so that the icon is not tightly near the text
        return size;
    }
    else {
        return favoriteItemNameSizeHint(option, index);
    }
}

void FavoriteItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                                const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == FavoritesModel::Columns::DisplayName)) {
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}

QSize FavoriteItemDelegate::favoriteItemNameSizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QString name = model->data(index).toString();
    if (Q_UNLIKELY(name.isEmpty())) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);
    int fontHeight = fontMetrics.height();

    QModelIndex itemTypeIndex = model->index(index.row(), FavoritesModel::Columns::Type, index.parent());
    QVariant itemType = model->data(itemTypeIndex);
    bool conversionResult = false;
    FavoritesModelItem::Type::type itemTypeInt = static_cast<FavoritesModelItem::Type::type>(itemType.toInt(&conversionResult));
    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the favorite item's type to int: ") << itemType);
        return QStyledItemDelegate::sizeHint(option, index);
    }

    if ((itemTypeInt != FavoritesModelItem::Type::Notebook) &&
        (itemTypeInt != FavoritesModelItem::Type::Tag))
    {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QModelIndex numNotesIndex = model->index(index.row(), FavoritesModel::Columns::NumNotesTargeted, index.parent());
    QVariant numNotes = model->data(numNotesIndex);
    conversionResult = false;
    int numNotesInt = numNotes.toInt(&conversionResult);

    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes to int: ") << numNotes);
        return QStyledItemDelegate::sizeHint(option, index);
    }

    if (numNotesInt <= 0) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    double margin = 1.1;
    return QSize(std::max(static_cast<int>(std::floor(nameWidth * margin + 0.5)), option.rect.width()),
                 std::max(static_cast<int>(std::floor(fontHeight * margin + 0.5)), option.rect.height()));
}

void FavoriteItemDelegate::drawFavoriteItemName(QPainter * painter, const QModelIndex & index,
                                                const QStyleOptionViewItem & option) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return;
    }

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    QString name = model->data(index).toString();
    if (name.isEmpty()) {
        QNDEBUG(QStringLiteral("FavoriteItemDelegate::drawFavoriteItemName: item name is empty"));
        return;
    }

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.highlightedText().color()
                    : option.palette.windowText().color());
    painter->drawText(option.rect, name, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);

    QModelIndex numNotesPerItemIndex = model->index(index.row(), FavoritesModel::Columns::NumNotesTargeted, index.parent());
    QVariant numNotesPerItem = model->data(numNotesPerItemIndex);
    bool conversionResult = false;
    int numNotesPerItemInt = numNotesPerItem.toInt(&conversionResult);
    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes per favorite item to int: ") << numNotesPerItem);
        return;
    }

    if (numNotesPerItemInt <= 0) {
        return;
    }

    QString nameSuffix = QStringLiteral(" (");
    nameSuffix += QString::number(numNotesPerItemInt);
    nameSuffix += QStringLiteral(")");

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.color(QPalette::Active, QPalette::WindowText)
                    : option.palette.color(QPalette::Active, QPalette::Highlight));
    painter->drawText(option.rect.translated(nameWidth, 0), nameSuffix, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}
