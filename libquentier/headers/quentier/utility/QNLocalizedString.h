#ifndef LIB_QUENTIER_UTILITY_LOCALIZED_STRING_H
#define LIB_QUENTIER_UTILITY_LOCALIZED_STRING_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/Printable.h>
#include <QString>
#include <QObject>

namespace quentier {

/**
 * @brief The QNLocalizedString encapsulates in itself both the text which has been localized (as a QString)
 * and the original non-localized English string.
 *
 * The primary purpose of this calss is to use the non-localized versions of strings for logging
 * so that the logs don't depend on the locale/translation and are thus actually useful for the troubleshooting
 */
class QNLocalizedString: public QString,
                         public Printable
{
public:
    static QNLocalizedString create(const char * str, QObject * trSource = Q_NULLPTR);

public:
    QNLocalizedString();
    QNLocalizedString(const QNLocalizedString & other);
    QNLocalizedString(const char * str);
    QNLocalizedString & operator=(const QNLocalizedString & other);
    virtual ~QNLocalizedString();

    void setNonLocalizedString(const char * str) { m_nonLocalizedStr = str; }
    const char * nonLocalizedString() const { return m_nonLocalizedStr; }

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

protected:
    QNLocalizedString(const QString & localizedString, const char * nonLocalizedString);

private:
    const char *    m_nonLocalizedStr;
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_LOCALIZED_STRING_H
