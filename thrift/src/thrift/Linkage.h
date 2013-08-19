#ifndef __THRIFT__LIB_CPP__LINKAGE_H
#define __THRIFT__LIB_CPP__LINKAGE_H

#if defined(_WIN32) | defined(_WIN64)
#if defined(BUILD_LIBTHRIFT)
#define THRIFT_EXPORT __declspec(dllexport)
#else
#define THRIFT_EXPORT __declspec(dllimport)
#endif
#else
#define THRIFT_EXPORT
#endif

#endif // __THRIFT__LIB_CPP__LINKAGE_H
