#ifndef __QUTE_NOTE__TOOLS_QUTE_NOTE_CHECK_PTR_H
#define __QUTE_NOTE__TOOLS_QUTE_NOTE_CHECK_PTR_H

#include "QuteNoteNullPtrException.h"

#ifndef QUTE_NOTE_CHECK_PTR
#define QUTE_NOTE_CHECK_PTR(pointer, message) \
{ \
    if (pointer == nullptr) { \
        using qute_note::QuteNoteNullPtrException; \
        throw QuteNoteNullPtrException(QString(message)); \
    } \
}
#endif

#endif // __QUTE_NOTE__TOOLS_QUTE_NOTE_CHECK_PTR_H
