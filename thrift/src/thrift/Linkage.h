#ifndef THRIFT_EXPORT
#if defined(_WIN32) | defined(_WIN64)
#if defined(BUILD_LIBTHRIFT)
#define THRIFT_EXPORT __declspec(dllexport)
#else
#define THRIFT_EXPORT __declspec(dllimport)
#endif
#else
#define THRIFT_EXPORT
#endif
#endif // not defined THRIFT_EXPORT
