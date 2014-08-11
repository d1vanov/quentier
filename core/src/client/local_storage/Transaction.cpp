#include "Transaction.h"
#include "DatabaseSqlErrorException.h"
#include <logging/QuteNoteLogger.h>
#include <QSqlQuery>
#include <QSqlError>

namespace qute_note {

Transaction::Transaction(const QSqlDatabase & db, TransactionType type) :
    m_db(db),
    m_type(type),
    m_committed(false),
    m_ended(false)
{
    init();
}

Transaction::Transaction(Transaction && other) :
    m_db(other.m_db),
    m_type(std::move(other.m_type)),
    m_committed(std::move(other.m_committed)),
    m_ended(std::move(other.m_ended))
{
    init();
}

Transaction::~Transaction()
{
    if ((m_type != Selection) && !m_committed)
    {
        QSqlQuery query(m_db);
        bool res = query.exec("ROLLBACK");
        if (!res) {
            QNCRITICAL("Error rolling back the SQL transaction: " << query.lastError());
            throw DatabaseSqlErrorException("Can't rollback SQL transaction, last error: " + query.lastError().text());
        }
    }
    else if ((m_type == Selection) && !m_ended)
    {
        QSqlQuery query(m_db);
        bool res = query.exec("END");
        if (!res) {
            QNCRITICAL("Error ending the SQL transaction of selection type: " << query.lastError());
            throw DatabaseSqlErrorException("Can't end SQL transaction, last error: " + query.lastError().text());
        }
    }
}

bool Transaction::commit(QString & errorDescription)
{
    if (m_type == Selection) {
        errorDescription += QT_TR_NOOP("Can't commit the transaction of selection type");
        return false;
    }

    QSqlQuery query(m_db);
    bool res = query.exec("COMMIT");
    if (!res) {
        errorDescription += QT_TR_NOOP("can't commit SQL transaction, error: ");
        errorDescription += query.lastError().text();
        QNWARNING(errorDescription + ", full last query error: " << query.lastError());
        return false;
    }

    m_committed = true;
    return true;
}

bool Transaction::end(QString & errorDescription)
{
    if (m_type != Selection) {
        errorDescription += QT_TR_NOOP("Only transactions used for selection queries should be "
                                       "explicitly ended without committing the changes");
        return false;
    }

    QSqlQuery query(m_db);
    bool res = query.exec("END");
    if (!res) {
        errorDescription += QT_TR_NOOP("can't end SQL transaction, error: ");
        errorDescription += query.lastError().text();
        QNWARNING(errorDescription + ", full last query error: " << query.lastError());
        return false;
    }

    m_ended = true;
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
