#include "ThriftException.h"

namespace qute_note {

ThriftException::ThriftException(const QString & message) :
    IQuteNoteException(message)
{}

const QString qute_note::ThriftException::exceptionDisplayName() const
{
    return std::move(QString("ThriftException"));
}

}
