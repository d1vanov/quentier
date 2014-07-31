#include "Transaction.h"
#include "DatabaseSqlErrorException.h"
#include <logging/QuteNoteLogger.h>
#include <QSqlQuery>
#include <QSqlError>

namespace qute_note {

Transaction::Transaction(QSqlDatabase & db, TransactionType type) :
    m_db(db),
    m_type(type),
    m_committed(false)
{
    init();
}

Transaction::Transaction(Transaction && other) :
    m_db(other.m_db),
    m_type(std::move(other.m_type)),
    m_committed(std::move(other.m_committed))
{
    init();
}

Transaction::~Transaction()
{
    if (!m_committed)
    {
        QSqlQuery query(m_db);
        bool res = query.exec("ROLLBACK");
        if (!res) {
            QNCRITICAL("Error rolling back the SQL transaction: " << query.lastError());
            throw DatabaseSqlErrorException("Can't rollback SQL transaction, last error: " +
                                            query.lastError().text());
        }
    }
}

bool Transaction::commit(QString & errorDescription)
{
    QSqlQuery query(m_db);
    bool res = query.exec("COMMIT");
    if (!res) {
        errorDescription += QT_TR_NOOP("can't commit SQL transaction, error: ");
        errorDescription += query.lastError().text();
        QNWARNING(errorDescription + ", full last query error: " << query.lastError());;
        return false;
    }

    m_committed = true;
    return true;
}

void Transaction::init()
{
    QString queryString = "BEGIN";
    if (m_type == Immediate) {
        queryString += " IMMEDIATE";
    }
    else if (m_type == Exclusive) {
        queryString += " EXCLUSIVE";
    }

    QSqlQuery query(m_db);
    bool res = query.exec(queryString);
    if (!res) {
        QNCRITICAL("Error beginning the SQL transaction: " << query.lastError());
        throw DatabaseSqlErrorException("Can't begin SQL transaction, last error: " +
                                        query.lastError().text());
    }
}

} // namespace qute_note
