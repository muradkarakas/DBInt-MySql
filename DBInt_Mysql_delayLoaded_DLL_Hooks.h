#pragma once

#define DELAYIMP_INSECURE_WRITABLE_HOOKS
#include <delayimp.h>
#include "..\SodiumShared\SodiumShared.h"
#include "DBInt_Mysql_delayLoaded_DLL_FuncImps.h"


// delay-loaded DLLs implementation functions
FARPROC WINAPI		delayedDllFailHook(unsigned dliNotify, PDelayLoadInfo pdli);
FARPROC WINAPI		delayHook(unsigned dliNotify, PDelayLoadInfo pdli);






