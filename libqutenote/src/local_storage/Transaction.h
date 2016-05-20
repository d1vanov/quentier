#ifndef LIB_QUTE_NOTE_LOCAL_STORAGE_TRANSACTION_H
#define LIB_QUTE_NOTE_LOCAL_STORAGE_TRANSACTION_H

#include <QSqlDatabase>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerPrivate)

class Transaction
{
public:
    enum TransactionType
    {
        Default = 0,
        Selection,  // transaction type for speeding-up selection queries via holding the shared lock
        Immediate,
        Exclusive
    };

    Transaction(const QSqlDatabase & db, const LocalStorageManagerPrivate & localStorageManager,
                TransactionType type = Default);
    virtual ~Transaction();

    bool commit(QString & errorDescription);
    bool end(QString & errorDescription);

private:
    Q_DISABLE_COPY(Transaction)

    void init();

    const QSqlDatabase & m_db;
    const LocalStorageManagerPrivate & m_localStorageManager;

private:
    TransactionType m_type;
    bool m_committed;
    bool m_ended;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_LOCAL_STORAGE_TRANSACTION_H
