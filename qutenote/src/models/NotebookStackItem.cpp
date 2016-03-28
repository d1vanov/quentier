#include "NotebookStackItem.h"

namespace qute_note {

QTextStream & NotebookStackItem::Print(QTextStream & strm) const
{
    strm << "Notebook stack: " << m_name;
    return strm;
}

} // namespace qute_note
