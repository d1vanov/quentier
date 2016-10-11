#include <quentier/types/Account.h>
#include "data/AccountData.h"

namespace quentier {

Account::Account(const QString & name, const Type::type type,
                 const EvernoteAccountType::type evernoteAccountType) :
    d(new AccountData)
{
    d->m_name = name;
    d->m_accountType = type;
    d->switchEvernoteAccountType(evernoteAccountType);
}

Account::Account(const Account & other) :
    d(other.d)
{}

Account & Account::operator=(const Account & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

Account::~Account()
{}

void Account::setEvernoteAccountType(const EvernoteAccountType::type evernoteAccountType)
{
    d->switchEvernoteAccountType(evernoteAccountType);
}

void Account::setEvernoteAccountLimits(const qevercloud::AccountLimits & limits)
{
    // TODO: implement
    Q_UNUSED(limits)
}

QString Account::name() const
{
    return d->m_name;
}

Account::Type::type Account::type() const
{
    return d->m_accountType;
}

Account::EvernoteAccountType::type Account::evernoteAccountType() const
{
    return d->m_evernoteAccountType;
}

qint32 Account::mailLimitDaily() const
{
    return d->m_mailLimitDaily;
}

qint64 Account::noteSizeMax() const
{
    return d->m_noteSizeMax;
}

qint64 Account::resourceSizeMax() const
{
    return d->m_resourceSizeMax;
}

qint32 Account::linkedNotebookMax() const
{
    return d->m_linkedNotebookMax;
}

qint32 Account::noteCountMax() const
{
    return d->m_noteCountMax;
}

qint32 Account::notebookCountMax() const
{
    return d->m_notebookCountMax;
}

qint32 Account::tagCountMax() const
{
    return d->m_tagCountMax;
}

qint32 Account::noteTagCountMax() const
{
    return d->m_noteTagCountMax;
}

qint32 Account::savedSearchCountMax() const
{
    return d->m_savedSearchCountMax;
}

qint32 Account::noteResourceCountMax() const
{
    return d->m_noteResourceCountMax;
}

QTextStream & Account::print(QTextStream & strm) const
{
    strm << QStringLiteral("Account: {\n");
    strm << QStringLiteral("    name = ") << d->m_name << QStringLiteral(";\n");

    strm << QStringLiteral("    type = ");
    switch(d->m_accountType)
    {
    case Account::Type::Local:
        strm << QStringLiteral("Local");
        break;
    case Account::Type::Evernote:
        strm << QStringLiteral("Evernote");
        break;
    default:
        strm << QStringLiteral("Unknown");
        break;
    }
    strm << QStringLiteral(";\n");

    strm << QStringLiteral("    Evernote account type = ");
    switch(d->m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Free:
        strm << QStringLiteral("Free");
        break;
    case Account::EvernoteAccountType::Plus:
        strm << QStringLiteral("Plus");
        break;
    case Account::EvernoteAccountType::Premium:
        strm << QStringLiteral("Premium");
        break;
    case Account::EvernoteAccountType::Business:
        strm << QStringLiteral("Business");
        break;
    default:
        strm << QStringLiteral("Unknown");
        break;
    }
    strm << QStringLiteral(";\n");

    strm << QStringLiteral("    mail limit daily = ") << d->m_mailLimitDaily << QStringLiteral(";\n");
    strm << QStringLiteral("    note size max = ") << d->m_noteSizeMax << QStringLiteral(";\n");
    strm << QStringLiteral("    resource size max = ") << d->m_resourceSizeMax << QStringLiteral(";\n");
    strm << QStringLiteral("    linked notebook max = ") << d->m_linkedNotebookMax << QStringLiteral(";\n");
    strm << QStringLiteral("    note count max = ") << d->m_noteCountMax << QStringLiteral(";\n");
    strm << QStringLiteral("    notebook count max = ") << d->m_notebookCountMax << QStringLiteral(";\n");
    strm << QStringLiteral("    tag count max = ") << d->m_tagCountMax << QStringLiteral(";\n");
    strm << QStringLiteral("    note tag count max = ") << d->m_noteTagCountMax << QStringLiteral(";\n");
    strm << QStringLiteral("    saved search count max = ") << d->m_savedSearchCountMax << QStringLiteral(";\n");
    strm << QStringLiteral("    note resource count max = ") << d->m_noteResourceCountMax << QStringLiteral(";\n");
    strm << QStringLiteral("};\n");

    return strm;
}

} // namespace quentier
