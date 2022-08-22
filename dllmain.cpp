// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "dc4100.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	UNREFERENCED_PARAMETER(hModule);
	UNREFERENCED_PARAMETER(lpReserved);
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}



/*	Check for an error and print the message.
This function is save within a mexEnter()/mexLeave section.
*/
#ifndef NDEBUG
void mexMessage(const char* file, const int line, ViStatus error)
#else
void mexMessage(ViStatus error)
#endif
{
	if (error != VI_SUCCESS)
	{
#ifndef NDEBUG
		mexPrintf("%s:%d ", file, line);
#endif
		ViChar description[DC4100_ERR_DESCR_BUFFER_SIZE];

		// Print error
		TLDC4100_error_message(instr, error, description);
		//mexPrintf("ERROR: %s\n", description);
		mexErrMsgTxt(description);
	}
}


/*	Free the powermeter and the driver.
*/
void mexCleanup(void)
{
	if (instr)
	{
		static ViStatus status;
		status = TLDC4100_close(instr);
#ifndef NDEBUG
		mexPrintf("Thorlabs LED driver connection %s shutted down!\n", aliasPtr[leddriver - 1]);
#endif
	}
	while (leddrivers)
	{
		leddriver = leddrivers;
		mxFree(rscPtr[leddriver - 1]);
		rscPtr[leddriver - 1] = NULL;
		leddrivers--;
	}
}


/*	Initialize driver
*/
int	 mexStartup(void)
{
	mexAtExit(mexCleanup);

	ViSession	defaultRM;
	ViFindList	findList;
	ViStatus	status;
	ViUInt32	numInstrs;							// counts found devices
	ViChar	rscStr[VI_FIND_BUFLEN];					// resource string
	ViChar	alias[DC4100_BUFFER_SIZE];				// string to copy the alias of a device to
	ViChar	name[DC4100_BUFFER_SIZE];				// string to copy the name of a device to
	ViAccessMode	lockState; 						// lock state of a device

	// Find resources
#ifndef NDEBUG
	mexPrintf("Scanning for DC4100 Series instruments ...\n");
#endif

	/* First we will need to open the default resource manager. */
	status = viOpenDefaultRM(&defaultRM);
	if (status < VI_SUCCESS)
	{
		printf("Could not open a session to the VISA Resource Manager!\n");
		exit(EXIT_FAILURE);
	}

	status = viFindRsrc(defaultRM, DC4100_FIND_PATTERN, &findList, &numInstrs, rscStr);
	if (status < VI_SUCCESS)
	{
		mexPrintf("An error occurred while finding resources.\n");
		viClose(defaultRM);
		return 0;
	}

	// Iterate all found devices
	while (numInstrs)
	{
		// Read information
		if (readInstrData(defaultRM, rscStr, name, alias, &lockState) == VI_SUCCESS)
		{
			// Check for the devices name
			if ((strstr(alias, "DC4100")) || (strstr(alias, "DC4104")))
			{
				// If found we store the device description pointer
				leddrivers++;
				mexMakeMemoryPersistent(rscPtr[leddrivers - 1] = static_cast<ViChar*>(mxCalloc(VI_FIND_BUFLEN, sizeof(ViChar))));
				mexMakeMemoryPersistent(aliasPtr[leddrivers - 1] = static_cast<ViChar*>(mxCalloc(VI_FIND_BUFLEN, sizeof(ViChar))));
				strcpy_s(rscPtr[leddrivers - 1], VI_FIND_BUFLEN, rscStr);
				strcpy_s(aliasPtr[leddrivers - 1], VI_FIND_BUFLEN, alias);
			}
		}
		numInstrs--;
		if (numInstrs)
		{
			// Find next device
			status = viFindNext(findList, rscStr);
			if (status < VI_SUCCESS)
				return (CleanupScan(defaultRM, findList, status));
		}
	}
	CleanupScan(defaultRM, findList, VI_SUCCESS);

#ifndef NDEBUG
	mexPrintf("%d Thorlabs LED driver%s available.\n", static_cast<int>(leddrivers), (leddrivers == 1) ? "" : "s");
#endif
	leddriver = leddrivers;

	if (leddriver > 0)
	{
		/* Now we will open a session to the last instrument we just found. */
		status = TLDC4100_init(rscPtr[leddriver - 1], VI_OFF, VI_OFF, &instr);
		if (status < VI_SUCCESS)
		{
			printf("An error occurred opening a session to %s\n", aliasPtr[leddriver - 1]);
		}
	}

	return static_cast<int>(leddrivers);
}


extern "C" void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	if (nrhs == 0 && nlhs == 0)
	{
		mexPrintf("\nThorlabs DC4104 4-channel LED driver.\n\n\tAndriy Chmyrov © 18.01.2018\n\n");
		return;
	}

	if (leddriver == -1)
	{
		if (mexStartup() == 0) return;
	}

	int n = 0;
	while (n < nrhs)
	{
		int index;
		int field;
		switch (mxGetClassID(prhs[n]))
		{
		default:
			mexErrMsgTxt("Parameter name expected as string.");
		case mxCHAR_CLASS:
		{
			char read[VI_FIND_BUFLEN];
			if (mxGetString(prhs[n], read, VI_FIND_BUFLEN)) mexErrMsgTxt("Unknown parameter.");
			if (++n < nrhs)
			{
				if (
					(_stricmp("brightness", read) == 0) || (_stricmp("current", read) == 0) || (_stricmp("current_max_limit", read) == 0) || (_stricmp("current_user_limit", read) == 0) || 
					(_stricmp("emission", read) == 0) || (_stricmp("emission", read) == 0) || (_stricmp("error_message", read) == 0) || (_stricmp("forward_bias", read) == 0) || 
					(_stricmp("head_info", read) == 0) || (_stricmp("wavelength", read) == 0)
					)
				{
					VALUE = getParameter2(read, prhs[n]);
					break;
				}
				else
					setParameter(read, prhs[n]);
				break;
			}
			if (nlhs > 1) mexErrMsgTxt("Too many output arguments.");
			VALUE = getParameter(read);
			return;
		}
		case mxSTRUCT_CLASS:
			for (index = 0; index < static_cast<int>(mxGetNumberOfElements(prhs[n])); index++)
				for (field = 0; field < mxGetNumberOfFields(prhs[n]); field++)
					setParameter(mxGetFieldNameByNumber(prhs[n], field), mxGetFieldByNumber(prhs[n], index, field));
		case mxDOUBLE_CLASS:
			index = static_cast<int>(getScalar(prhs[n]));
			if (index < 1 || index > leddrivers) mexErrMsgTxt("Invalid LED driver handle.");
			if (index != leddriver)
			{
				TLDC4100_close(instr);
				leddriver = index;

				/* Now we will open a session to the new instrument. */
				ViStatus	status;
				status = TLDC4100_init(rscPtr[leddriver - 1], VI_OFF, VI_OFF, &instr);
				if (status < VI_SUCCESS)
				{
					printf("An error occurred opening a session to %s\n", aliasPtr[leddriver - 1]);
				}

#ifndef NDEBUG
				mexPrintf("LED driver #%d (%s) selected\n", leddriver, aliasPtr[leddriver - 1]);
#endif
			}
			VALUE = mxCreateString(rscPtr[leddriver - 1]);
			return;
		}
		n++;
	}
	switch (nlhs)
	{
	default:
		mexErrMsgTxt("Too many output arguments.");
	case 1:
		VALUE = mxCreateDoubleScalar(static_cast<double>(leddrivers));
	case 0:;
	}
}