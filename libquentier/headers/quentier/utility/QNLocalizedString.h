#ifndef LIB_QUENTIER_UTILITY_LOCALIZED_STRING_H
#define LIB_QUENTIER_UTILITY_LOCALIZED_STRING_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/Printable.h>
#include <QString>
#include <QObject>

namespace quentier {

/**
 * @brief The QNLocalizedString encapsulates in itself both the (potentially) user visible text which is localized
 * (i.e. transated) and the original non-localized English string.
 *
 * The primary purpose of this calss is to use the non-localized versions of strings for logging
 * so that the logs don't depend on the locale/translation and are thus actually useful for the troubleshooting
 */
class QNLocalizedString: public Printable
{
public:
    QNLocalizedString();
    QNLocalizedString(const char * nonLocalizedString,
                      QObject * pTranslationContext = Q_NULLPTR, const char * disambiguation = Q_NULLPTR,
                      const int plurals = -1);
    QNLocalizedString(const QNLocalizedString & other);
    QNLocalizedString & operator=(const QNLocalizedString & other);
    QNLocalizedString & operator=(const char * nonLocalizedString);
    virtual ~QNLocalizedString();

    bool isEmpty() const;
    void clear();

    void set(const char * nonLocalizedString, QObject * pTranslationContext = Q_NULLPTR,
             const char * disambiguation = Q_NULLPTR, const int plurals = -1);

    const QString & localizedString() const { return m_localizedString; }
    const QString & nonLocalizedString() const { return m_nonLocalizedString; }

    QNLocalizedString & operator+=(const QNLocalizedString & str);
    QNLocalizedString & operator+=(const char * nonLocalizedString);
    QNLocalizedString & operator+=(const QString & nonTranslatableString);

    void append(const QNLocalizedString & str);
    void append(const QString & localizedString, const char * nonLocalizedString);
    void append(const char * nonLocalizedString, QObject * pTranslationContext = Q_NULLPTR,
                const char * disambiguation = Q_NULLPTR, const int plurals = -1);

    void prepend(const QNLocalizedString & str);
    void prepend(const QString & localizedString, const char * nonLocalizedString);
    void prepend(const char * nonLocalizedString, QObject * pTranslationContext = Q_NULLPTR,
                 const char * disambiguation = Q_NULLPTR, const int plurals = -1);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString translateString(const char * nonLocalizedString, QObject * pTranslationContext,
                            const char * disambiguation, const int plurals);

private:
    QString         m_localizedString;
    QString         m_nonLocalizedString;
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_LOCALIZED_STRING_H
