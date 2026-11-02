#include "LocalBuildHooks.h"

#if defined(_MSC_VER)
LocalBuildHooks* get_local_build_hooks_default() { return nullptr; }
#  pragma comment( \
      linker,      \
      "/alternatename:?get_local_build_hooks@@YAPEAVLocalBuildHooks@@XZ=?get_local_build_hooks_default@@YAPEAVLocalBuildHooks@@XZ")
#else
__attribute__((weak)) LocalBuildHooks* get_local_build_hooks() { return nullptr; }
#endif
