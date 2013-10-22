#ifndef __QUTE_NOTE__TOOLS__TYPE_WITH_ERROR_H
#define __QUTE_NOTE__TOOLS__TYPE_WITH_ERROR_H

#include <QString>

namespace qute_note {

/**
 * @brief The TypeWithError class - base class for any type
 * which can have error message set instead of value or together with value
 */
class TypeWithError
{
public:
    void SetError(const QString & error);

    const QString GetError() const;

    bool HasError() const;

    void ClearError();

protected:
    TypeWithError();
    TypeWithError(const TypeWithError & other);
    TypeWithError & operator =(const TypeWithError & other);
    virtual ~TypeWithError();

private:
    QString m_error;
};

}

#endif // __QUTE_NOTE__TOOLS__TYPE_WITH_ERROR_H
