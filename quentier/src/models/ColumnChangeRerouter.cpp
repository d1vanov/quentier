#include "ColumnChangeRerouter.h"
#include <quentier/logging/QuentierLogger.h>
#include <algorithm>

ColumnChangeRerouter::ColumnChangeRerouter(const int columnFrom, const int columnTo, QObject * parent) :
    QObject(parent),
    m_columnFrom(columnFrom),
    m_columnTo(columnTo)
{}

void ColumnChangeRerouter::setModel(QAbstractItemModel * model)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(model, QNSIGNAL(QAbstractItemModel,dataChanged,const QModelIndex&,const QModelIndex&),
                     this, QNSLOT(ColumnChangeRerouter,onModelDataChanged,const QModelIndex&,const QModelIndex&));
#else
    QObject::connect(model, QNSIGNAL(QAbstractItemModel,dataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&),
                     this, QNSLOT(ColumnChangeRerouter,onModelDataChanged,const QModelIndex&,const QModelIndex&,const QVector<int>&));
#endif
}

void ColumnChangeRerouter::onModelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                            )
#else
                            , const QVector<int> & roles)
#endif
{
    QNTRACE(QStringLiteral("ColumnChangeRerouter::onModelDataChanged: top left: is valid = ")
            << (topLeft.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << topLeft.row() << QStringLiteral(", column = ")
            << topLeft.column() << QStringLiteral("; bottom right: is valid = ")
            << (bottomRight.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << bottomRight.row() << QStringLiteral(", column = ")
            << bottomRight.column() << QStringLiteral(", column from = ") << m_columnFrom
            << QStringLiteral(", column to = ") << m_columnTo);

    if (!topLeft.isValid() || !bottomRight.isValid()) {
        return;
    }

    int columnLeft = topLeft.column();
    int columnRight = bottomRight.column();

    if ((columnLeft <= m_columnTo) || (columnRight >= m_columnTo)) {
        QNTRACE(QStringLiteral("Already includes column to"));
        return;
    }

    if ((columnLeft >= m_columnFrom) || (columnRight <= m_columnFrom)) {
        QNTRACE(QStringLiteral("Doesn't include column from"));
        return;
    }

    const QAbstractItemModel * model = topLeft.model();
    if (Q_UNLIKELY(!model)) {
        model = bottomRight.model();
    }

    if (Q_UNLIKELY(!model)) {
        QNDEBUG(QStringLiteral("No model"));
        return;
    }

    int newColumnLeft = std::min(columnLeft, m_columnTo);
    int newColumnRight = std::max(columnRight, m_columnTo);
    QNTRACE(QStringLiteral("New column left = ") << newColumnLeft << QStringLiteral(", new column right = ") << newColumnRight);

    QModelIndex newTopLeft = model->index(topLeft.row(), newColumnLeft, topLeft.parent());
    QModelIndex newBottomRight = model->index(bottomRight.row(), newColumnRight, bottomRight.parent());

    emit dataChanged(newTopLeft, newBottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                    );
#else
                    , roles);
#endif
}
