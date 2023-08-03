#ifndef TCRDEMO_CRASH_DUMP_H_
#define TCRDEMO_CRASH_DUMP_H_

#include <windows.h>
#include <dbghelp.h>

class CrashDump
{
public:
	// ×¢²áCrashDump
	static void InitCrashDump();
private:
	// create mini dump file
	static int GenerateMiniDump(PEXCEPTION_POINTERS pExceptionPointers);
	static LONG _stdcall ExceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo);

};
#endif

