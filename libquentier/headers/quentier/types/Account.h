#ifndef LIB_QUENTIER_TYPES_ACCOUNT_H
#define LIB_QUENTIER_TYPES_ACCOUNT_H

#include <quentier/utility/Printable.h>
#include <quentier/utility/Macros.h>
#include <QString>
#include <QSharedDataPointer>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountData)

/**
 * @brief The Account class encapsulates some details about the account: its name,
 * whether it is local or synchronized to Evernote and for the latter case -
 * some additional details like upload limit etc.
 */
class QUENTIER_EXPORT Account: public Printable
{
public:
    struct Type
    {
        enum type
        {
            Local = 0,
            Evernote
        };
    };

    struct EvernoteAccountType
    {
        enum type
        {
            Free = 0,
            Plus,
            Premium,
            Business
        };
    };

public:
    explicit Account();
    explicit Account(const QString & name, const Type::type type,
                     const qevercloud::UserID userId = -1,
                     const EvernoteAccountType::type evernoteAccountType = EvernoteAccountType::Free,
                     const QString & evernoteHost = QString());
    Account(const Account & other);
    Account & operator=(const Account & other);
    virtual ~Account();

    bool operator==(const Account & other) const;
    bool operator!=(const Account & other) const;

    /**
     * @return true if either the account is local but the name is empty or if the account is Evernote but user id is negative;
     * in all other cases return false
     */
    bool isEmpty() const;

    /**
     * @return username for either local or Evernote account
     */
    QString name() const;

    /**
     * @return the type of the account: either local of Evernote
     */
    Type::type type() const;

    /**
     * @return user id for Evernote accounts, -1 for local accounts (as the concept of user id is not defined for local accounts)
     */
    qevercloud::UserID id() const;

    /**
     * @return the type of the Evernote account; if applied to free account, returns "Free"
     */
    EvernoteAccountType::type evernoteAccountType() const;

    /**
     * @return the Evernote server host with which the account is associated
     */
    QString evernoteHost() const;

    void setEvernoteAccountType(const EvernoteAccountType::type evernoteAccountType);
    void setEvernoteHost(const QString & evernoteHost);

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
    void setEvernoteAccountLimits(const qevercloud::AccountLimits & limits);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<AccountData> d;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_ACCOUNT_H
