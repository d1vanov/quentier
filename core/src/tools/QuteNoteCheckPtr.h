#ifndef __QUTE_NOTE__TOOLS_QUTE_NOTE_CHECK_PTR_H
#define __QUTE_NOTE__TOOLS_QUTE_NOTE_CHECK_PTR_H

#include "QuteNoteNullPtrException.h"

#ifndef QUTE_NOTE_CHECK_PTR
#define QUTE_NOTE_CHECK_PTR(pointer, ...) \
{ \
    if (!pointer) \
    { \
        using qute_note::QuteNoteNullPtrException; \
        QString qute_note_null_ptr_error = "Found NULL pointer at "; \
        qute_note_null_ptr_error += __FILE__; \
        qute_note_null_ptr_error += " ("; \
        qute_note_null_ptr_error += QString::number(__LINE__); \
        qute_note_null_ptr_error += ") "; \
        qute_note_null_ptr_error = qute_note_null_ptr_error.sprintf(qPrintable(qute_note_null_ptr_error), ##__VA_ARGS__); \
        throw QuteNoteNullPtrException(qute_note_null_ptr_error); \
    } \
}
#endif

#endif // __QUTE_NOTE__TOOLS_QUTE_NOTE_CHECK_PTR_H
