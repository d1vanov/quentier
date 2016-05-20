#ifndef LIB_QUTE_NOTE_UTILITY_PRINTABLE_H
#define LIB_QUTE_NOTE_UTILITY_PRINTABLE_H

#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QHash>
#include <QSet>
#include <QEverCloud.h>
#include <oauth.h>

namespace qute_note {

/**
 * @brief The Printable class is the interface for QuteNote's internal classes
 * which should be able to write themselves into QTextStream and/or convert to QString
 */
class QUTE_NOTE_EXPORT Printable
{
public:
    virtual QTextStream & print(QTextStream & strm) const = 0;

    virtual const QString toString() const;

    friend QUTE_NOTE_EXPORT QTextStream & operator << (QTextStream & strm,
                                                       const Printable & printable);
    friend QUTE_NOTE_EXPORT QDebug & operator << (QDebug & debug,
                                                  const Printable & printable);
protected:
    Printable();
    Printable(const Printable & other);
    virtual ~Printable();
};

} // namespace qute_note

// printing operators for existing classes not inheriting from Printable

template <class T>
const QString ToString(const T & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << object;
    return str;
}

template <class TKey, class TValue>
const QString ToString(const QHash<TKey, TValue> & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << "QHash: \n";

    typedef typename QHash<TKey,TValue>::const_iterator CIter;
    CIter hashEnd = object.end();
    for(CIter it = object.begin(); it != hashEnd; ++it) {
        strm << "[" << it.key() << "] = " << it.value() << ";\n";
    }
    return str;
}

template <class T>
const QString ToString(const QSet<T> & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << "QSet: \n";

    typedef typename QSet<T>::const_iterator CIter;
    CIter setEnd = object.end();
    for(CIter it = object.begin(); it != setEnd; ++it) {
        strm << "[" << *it << "];\n";
    }
    return str;
}

#define QUTE_NOTE_DECLARE_PRINTABLE(type, ...) \
    QTextStream & operator << (QTextStream & strm, const type & obj); \
    inline QDebug & operator << (QDebug & debug, const type & obj) \
    { \
        debug << ToString<type, ##__VA_ARGS__>(obj); \
        return debug; \
    }

QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::BusinessUserInfo)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::PremiumInfo)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Accounting)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::UserAttributes)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::NoteAttributes)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::PrivilegeLevel::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::QueryFormat::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SharedNotebookPrivilegeLevel::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::NoteSortOrder::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::NotebookRestrictions)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SharedNotebookInstanceRestrictions::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::ResourceAttributes)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Resource)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SyncChunk)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Tag)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SavedSearch)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::LinkedNotebook)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Notebook)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Publishing)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SharedNotebook)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::BusinessNotebook)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::User)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SharedNotebookRecipientSettings)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::ReminderEmailConfig::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::PremiumOrderStatus::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::BusinessUserRole::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::SponsoredGroupRole::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::Note)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::EDAMErrorCode::type)
QUTE_NOTE_DECLARE_PRINTABLE(qevercloud::EvernoteOAuthWebView::OAuthResult)

#endif // LIB_QUTE_NOTE_UTILITY_PRINTABLE_H
