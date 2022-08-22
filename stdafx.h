// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>



// TODO: reference additional headers your program requires here
#include <mex.h>
#include <visa.h>
#include "TLDC4100.h" 	// the device driver header

#pragma comment(lib, "libmx.lib")
#pragma comment(lib, "libmex.lib")
#pragma comment(lib, "libmat.lib")

#pragma comment(lib, "visa64.lib")
#pragma comment(lib, "TLDC4100_64.lib")




#define MATLAB_MEX_FILE