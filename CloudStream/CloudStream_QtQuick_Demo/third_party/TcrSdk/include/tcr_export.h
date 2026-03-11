#ifndef TCRSDK_EXPORT_H_
#define TCRSDK_EXPORT_H_

#ifdef _WIN32
  // Windows 平台使用 __declspec
  #ifdef TCRSDK_EXPORTS
    #define TCRSDK_API __declspec(dllexport)
  #else
    #define TCRSDK_API __declspec(dllimport)
  #endif
#else
  // macOS/Linux 平台使用 __attribute__
  #ifdef TCRSDK_EXPORTS
    #define TCRSDK_API __attribute__((visibility("default")))
  #else
    #define TCRSDK_API
  #endif
#endif

#endif // TCRSDK_EXPORT_H_