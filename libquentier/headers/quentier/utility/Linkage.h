#ifndef LIB_QUENTIER_UTILITY_LINKAGE_H
#define LIB_QUENTIER_UTILITY_LINKAGE_H

#if defined (_WIN32) & defined(_MSC_VER)
#if defined(BUILDING_QUENTIER_DLL)
    #define QUENTIER_EXPORT __declspec(dllexport)
#else
    #define QUENTIER_EXPORT __declspec(dllimport)
#endif
#else
    #define QUENTIER_EXPORT
#endif

#endif // LIB_QUENTIER_UTILITY_LINKAGE_H
