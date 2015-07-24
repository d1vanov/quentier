#ifndef __SIMPLECRYPT__LINKAGE_H
#define __SIMPLECRYPT__LINKAGE_H

#if defined(_WIN32) | defined(_WIN64)
#if defined(BUILD_LIBSIMPLECRYPT)
#define SIMPLECRYPT_EXPORT __declspec(dllexport)
#else
#define SIMPLECRYPT_EXPORT __declspec(dllimport)
#endif
#else
#define SIMPLECRYPT_EXPORT
#endif

#endif // __SIMPLECRYPT__LINKAGE_H
