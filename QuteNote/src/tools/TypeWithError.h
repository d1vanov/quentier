#ifndef __QUTE_NOTE__TOOLS__TYPE_WITH_ERROR_H
#define __QUTE_NOTE__TOOLS__TYPE_WITH_ERROR_H

#include <string>
#include <QString>

namespace qute_note {

class TypeWithError
{
public:
    void SetError(const char * error);
    void SetError(const std::string & error);
    void SetError(const QString & error);

    const char * GetError() const;
    bool HasError() const;

    void ClearError();

protected:
    TypeWithError();
    TypeWithError(const TypeWithError & other);
    TypeWithError & operator =(const TypeWithError & other);
    virtual ~TypeWithError();

    virtual void Clear();

private:
    char * m_errorText;
};

}

#endif // __QUTE_NOTE__TOOLS__TYPE_WITH_ERROR_H
