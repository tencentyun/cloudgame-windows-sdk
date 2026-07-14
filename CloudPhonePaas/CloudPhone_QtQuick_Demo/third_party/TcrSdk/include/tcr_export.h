// Copyright 2026 Tencent.
// All rights reserved.

#ifndef CLOUD_GAMING_PC_TCRSDKPROJECT_TCRSDK_API_TCR_EXPORT_H_
#define CLOUD_GAMING_PC_TCRSDKPROJECT_TCRSDK_API_TCR_EXPORT_H_
#ifdef _WIN32
#  ifdef TCRSDK_EXPORTS
#    define TCRSDK_API __declspec(dllexport)
#  else
#    define TCRSDK_API __declspec(dllimport)
#  endif
#else
#  ifdef TCRSDK_EXPORTS
#    define TCRSDK_API __attribute__((visibility("default")))
#  else
#    define TCRSDK_API
#  endif
#endif

#endif  // CLOUD_GAMING_PC_TCRSDKPROJECT_TCRSDK_API_TCR_EXPORT_H_