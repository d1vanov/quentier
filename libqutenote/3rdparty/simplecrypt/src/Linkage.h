#ifndef SIMPLECRYPT_LINKAGE_H
#define SIMPLECRYPT_LINKAGE_H

#if defined(_WIN32) | defined(_WIN64)
#if defined(BUILD_LIBSIMPLECRYPT)
#define SIMPLECRYPT_EXPORT __declspec(dllexport)
#else
#define SIMPLECRYPT_EXPORT __declspec(dllimport)
#endif
#else
#define SIMPLECRYPT_EXPORT
#endif

#endif // SIMPLECRYPT_LINKAGE_H
