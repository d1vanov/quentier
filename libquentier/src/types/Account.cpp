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
    strm << "Account: {\n";
    strm << "    name = " << d->m_name << ";\n";

    strm << "    type = ";
    switch(d->m_accountType)
    {
    case Account::Type::Local:
        strm << "Local";
        break;
    case Account::Type::Evernote:
        strm << "Evernote";
        break;
    default:
        strm << "Unknown";
        break;
    }
    strm << ";\n";

    strm << "    Evernote account type = ";
    switch(d->m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Free:
        strm << "Free";
        break;
    case Account::EvernoteAccountType::Plus:
        strm << "Plus";
        break;
    case Account::EvernoteAccountType::Premium:
        strm << "Premium";
        break;
    case Account::EvernoteAccountType::Business:
        strm << "Business";
        break;
    default:
        strm << "Unknown";
        break;
    }
    strm << ";\n";

    strm << "    mail limit daily = " << d->m_mailLimitDaily << ";\n";
    strm << "    note size max = " << d->m_noteSizeMax << ";\n";
    strm << "    resource size max = " << d->m_resourceSizeMax << ";\n";
    strm << "    linked notebook max = " << d->m_linkedNotebookMax << ";\n";
    strm << "    note count max = " << d->m_noteCountMax << ";\n";
    strm << "    notebook count max = " << d->m_notebookCountMax << ";\n";
    strm << "    tag count max = " << d->m_tagCountMax << ";\n";
    strm << "    note tag count max = " << d->m_noteTagCountMax << ";\n";
    strm << "    saved search count max = " << d->m_savedSearchCountMax << ";\n";
    strm << "    note resource count max = " << d->m_noteResourceCountMax << ";\n";
    strm << "};\n";

    return strm;
}

} // namespace quentier
