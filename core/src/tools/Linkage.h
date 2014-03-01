#ifndef __QUTE_NOTE__CORE__LINKAGE_H
#define __QUTE_NOTE__CORE__LINKAGE_H

#if defined (_WIN32) & defined(_MSC_VER)
#if defined(BUILDING_QUTE_NOTE_DLL)
    #define QUTE_NOTE_EXPORT __declspec(dllexport)
#else
    #define QUTE_NOTE_EXPORT __declspec(dllimport)
#endif
#else
    #define QUTE_NOTE_EXPORT
#endif

#endif // __QUTE_NOTE__CORE__LINKAGE_H
