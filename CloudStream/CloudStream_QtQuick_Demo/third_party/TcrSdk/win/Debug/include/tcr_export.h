#ifndef TCRSDK_EXPORT_H_
#define TCRSDK_EXPORT_H_

#ifdef TCRSDK_EXPORTS
  #define TCRSDK_API __declspec(dllexport)
#else
  #define TCRSDK_API __declspec(dllimport)
#endif

#endif // TCRSDK_EXPORT_H_