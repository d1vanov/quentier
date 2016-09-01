#ifndef LIB_QUENTIER_TYPES_ACCOUNT_H
#define LIB_QUENTIER_TYPES_ACCOUNT_H

#include <quentier/utility/Printable.h>
#include <quentier/utility/Qt4Helper.h>
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
class Account: public Printable
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
    explicit Account(const QString & name, const Type::type type,
                     const EvernoteAccountType::type evernoteAccountType = EvernoteAccountType::Free);
    Account(const Account & other);
    Account & operator=(const Account & other);
    virtual ~Account();

    void setEvernoteAccountType(const EvernoteAccountType::type evernoteAccountType);

    QString name() const;
    Type::type type() const;
    EvernoteAccountType::type evernoteAccountType() const;
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

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<AccountData> d;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_ACCOUNT_H
