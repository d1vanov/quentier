#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__TRANSACTION_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__TRANSACTION_H

#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QSqlDatabase>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerPrivate)

class QUTE_NOTE_EXPORT Transaction
{
public:
    enum TransactionType {
        Default,
        Selection,  // transaction type for speeding-up selection queries via holding the shared lock
        Immediate,
        Exclusive
    };

    Transaction(const QSqlDatabase & db, const LocalStorageManagerPrivate & localStorageManager,
                TransactionType type = Default);
    Transaction(Transaction && other);
    virtual ~Transaction();

    bool commit(QString & errorDescription);
    bool end(QString & errorDescription);

private:
    Transaction() Q_DECL_DELETE;
    Transaction(const Transaction & other) Q_DECL_DELETE;
    Transaction & operator=(const Transaction & other) Q_DECL_DELETE;
    Transaction & operator=(Transaction && other) Q_DECL_DELETE;

    void init();

    const QSqlDatabase & m_db;
    const LocalStorageManagerPrivate & m_localStorageManager;

    TransactionType m_type;
    bool m_committed;
    bool m_ended;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__TRANSACTION_H
