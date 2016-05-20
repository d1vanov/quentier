#ifndef LIB_QUTE_NOTE_UTILITY_LINKAGE_H
#define LIB_QUTE_NOTE_UTILITY_LINKAGE_H

#if defined (_WIN32) & defined(_MSC_VER)
#if defined(BUILDING_QUTE_NOTE_DLL)
    #define QUTE_NOTE_EXPORT __declspec(dllexport)
#else
    #define QUTE_NOTE_EXPORT __declspec(dllimport)
#endif
#else
    #define QUTE_NOTE_EXPORT
#endif

#endif // LIB_QUTE_NOTE_UTILITY_LINKAGE_H
