#ifndef LIB_QUENTIER_TYPES_DATA_ACCOUNT_DATA_H
#define LIB_QUENTIER_TYPES_DATA_ACCOUNT_DATA_H

#include <quentier/types/Account.h>
#include <QSharedData>

namespace quentier {

class AccountData: public QSharedData
{
public:
    explicit AccountData();
    virtual ~AccountData();

    AccountData(const AccountData & other);
    AccountData(AccountData && other);

    void switchEvernoteAccountType(const Account::EvernoteAccountType::type evernoteAccountType);
    void setEvernoteAccountLimits(const qevercloud::AccountLimits & limits);

    QString                             m_name;
    Account::Type::type                 m_accountType;
    Account::EvernoteAccountType::type  m_evernoteAccountType;
    qint32                              m_mailLimitDaily;
    qint64                              m_noteSizeMax;
    qint64                              m_resourceSizeMax;
    qint32                              m_linkedNotebookMax;
    qint32                              m_noteCountMax;
    qint32                              m_notebookCountMax;
    qint32                              m_tagCountMax;
    qint32                              m_noteTagCountMax;
    qint32                              m_savedSearchCountMax;
    qint32                              m_noteResourceCountMax;

    qint32 mailLimitDaily() const;
    qint64 noteSizeMax() const;
    qint64 resourceSizeMax() const;
    qint32 linkedNotebookMax() const;
    qint32 noteCountMax() const;
    qint32 notebookCountMax() const;
    qint32 tagCountMax() const;
    qint32 noteTagCountMax() const;
    qint32 savedSearchCountMax() const;
    qint32 noteResourceCountMax() const;

private:
    AccountData & operator=(const AccountData & other) Q_DECL_EQ_DELETE;
    AccountData & operator=(AccountData && other) Q_DECL_EQ_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_ACCOUNT_DATA_H
