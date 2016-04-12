#include "NotebookStackItem.h"
#include "NotebookModelItem.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

QTextStream & NotebookStackItem::print(QTextStream & strm) const
{
    strm << "Notebook stack: " << m_name;
    return strm;
}

} // namespace qute_note
