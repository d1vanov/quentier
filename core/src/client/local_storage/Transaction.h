#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__TRANSACTION_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__TRANSACTION_H

#include <QSqlDatabase>

namespace qute_note {

class Transaction
{
public:
    Transaction(QSqlDatabase & db);
    Transaction(Transaction && other);
    virtual ~Transaction();

    bool commit(QString & errorDescription);

private:
    Transaction() = delete;
    Transaction(const Transaction & other) = delete;
    Transaction & operator=(const Transaction & other) = delete;
    Transaction & operator=(Transaction && other) = delete;

    void init();

    QSqlDatabase & m_db;
    bool m_committed;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__TRANSACTION_H
