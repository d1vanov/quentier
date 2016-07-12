#ifndef LIB_QUENTIER_LOCAL_STORAGE_TRANSACTION_H
#define LIB_QUENTIER_LOCAL_STORAGE_TRANSACTION_H

#include <quentier/utility/QNLocalizedString.h>
#include <QSqlDatabase>

namespace quentier {

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

    bool commit(QNLocalizedString & errorDescription);
    bool end(QNLocalizedString & errorDescription);

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

} // namespace quentier

#endif // LIB_QUENTIER_LOCAL_STORAGE_TRANSACTION_H
