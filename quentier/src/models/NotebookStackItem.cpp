#include "NotebookStackItem.h"
#include "NotebookModelItem.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

QTextStream & NotebookStackItem::print(QTextStream & strm) const
{
    strm << "Notebook stack: " << m_name;
    return strm;
}

} // namespace quentier
