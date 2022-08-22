// dc4100.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "dc4100.h"


int leddriver = -1;
int leddrivers = 0;

/*	LED driver states. Unused handles are set to 0.
*/
//static ViSession	instr = 0;
ViSession	instr = 0;
ViChar*	rscPtr[MAX_NUMBER_OF_SUPPORTED_INSTRUMENTS];						// array of pointer to resource string
ViChar*	aliasPtr[MAX_NUMBER_OF_SUPPORTED_INSTRUMENTS];						// array of pointer to resource string


/*---------------------------------------------------------------------------
Read instrument data. Reads the data from the opened VISA instrument
session.
Return value: VISA library error code.

Note: For Serial connections we use the VISA alias as name and the
'VI_ATTR_INTF_INST_NAME' as description.
---------------------------------------------------------------------------*/
ViStatus readInstrData(ViSession resMgr, ViRsrc rscStr, ViChar *name, ViChar *descr, ViAccessMode *lockState)
{
#define DEF_VI_OPEN_TIMEOUT	1500 				// open timeout in milliseconds
#define DEF_INSTR_NAME			"Unknown"		// default device name
#define DEF_INSTR_ALIAS			""					// default alias
#define DEF_LOCK_STATE			VI_NO_LOCK		// not locked by default

	ViStatus		err;
	ViSession	instr;
	ViUInt16		intfType, intfNum;
	ViInt16     ri_state = 0, cts_state = 0, dcd_state = 0;

	// Default values
	strcpy_s(name, VI_FIND_BUFLEN, DEF_INSTR_NAME);
	strcpy_s(descr, VI_FIND_BUFLEN, DEF_INSTR_ALIAS);
	*lockState = DEF_LOCK_STATE;

	// Get alias
	if ((err = viParseRsrcEx(resMgr, rscStr, &intfType, &intfNum, VI_NULL, VI_NULL, name)) < 0) return (err);
	if (intfType != VI_INTF_ASRL) return (VI_ERROR_INV_RSRC_NAME);
	// Open resource
	err = viOpen(resMgr, rscStr, VI_NULL, DEF_VI_OPEN_TIMEOUT, &instr);
	if (err == VI_ERROR_RSRC_BUSY)
	{
		// Port is open in another application
		if (strncmp(name, "LPT", 3) == 0)
		{
			// Probably we have a LPT port - do not show this
			return (VI_ERROR_INV_OBJECT);
		}
		// display port as locked
		*lockState = VI_EXCLUSIVE_LOCK;
		return (VI_SUCCESS);
	}
	if (err) return (err);
	// Get attribute data
	err = viGetAttribute(instr, VI_ATTR_RSRC_LOCK_STATE, lockState);
	if (!err) err = viGetAttribute(instr, VI_ATTR_INTF_INST_NAME, descr);
	// Get wire attributes to exclude LPT...
	if (!err) err = viGetAttribute(instr, VI_ATTR_ASRL_RI_STATE, &ri_state);
	if (!err) err = viGetAttribute(instr, VI_ATTR_ASRL_CTS_STATE, &cts_state);
	if (!err) err = viGetAttribute(instr, VI_ATTR_ASRL_DCD_STATE, &dcd_state);
	if (!err)
	{
		if ((ri_state == VI_STATE_UNKNOWN) && (cts_state == VI_STATE_UNKNOWN) && (dcd_state == VI_STATE_UNKNOWN)) err = VI_ERROR_INV_OBJECT;
	}
	// Closing
	viClose(instr);
	return (err);
}


/*---------------------------------------------------------------------------
Cleanup scan. Frees the data structures used for scanning instruments.
Return value: value passed via the 'err' parameter.
---------------------------------------------------------------------------*/
ViStatus CleanupScan(ViSession resMgr, ViFindList findList, ViStatus err)
{
	if (findList != VI_NULL) viClose(findList);
	if (resMgr != VI_NULL) viClose(resMgr);
	return (err);
}


/*	Check for and read a scalar value.
*/
double getScalar(const mxArray* array)
{
	if (!mxIsNumeric(array) || mxGetNumberOfElements(array) != 1) mexErrMsgTxt("Not a scalar.");
	return mxGetScalar(array);
}


/*	Check for and read a matrix
*/
double* getArray(const mxArray* array, int size)
{
	if (!mxIsNumeric(array) || mxGetNumberOfElements(array) != size) mexErrMsgTxt("Supplied wrong number of elements in array.");
	return mxGetPr(array);
}


/*	Check for and read a string
*/
char* getString(const mxArray *array, const int StrLength = VI_FIND_BUFLEN)
{
	if (!mxIsChar(array))
	{
		mexErrMsgIdAndTxt("MATLAB:mxcreatecharmatrixfromstr:invalidInputType",
			"Input must be of type char.");
	}

	/* Allocate enough memory to hold the converted string. */
	size_t ValueLen = mxGetNumberOfElements(array) + 1;
	if (static_cast<int>(ValueLen) >= StrLength)
	{
		mexErrMsgTxt("Supplied string is longer than the maximum supported string length.");
	}
	char *Value;
	Value = static_cast<char*>(mxCalloc(ValueLen, sizeof(char)));
	mxGetString(array, Value, ValueLen);
	return Value;
}


/*	Get a parameter or status value.
*/
mxArray* getParameter(const char* name)
{

	if (_stricmp("close", name) == 0)
	{
		mexCleanup();
		leddriver = -1;
		return mxCreateDoubleScalar(1);
	}

	if (_stricmp("display_brightness", name) == 0)
	{
		ViInt32 displayBrightness;
		MEXMESSAGE(TLDC4100_getDispBright(instr, &displayBrightness));
		return mxCreateDoubleScalar(static_cast<double>(displayBrightness)/100);
	}

	if (_stricmp("id", name) == 0)
	{
		ViChar manufacturerName[DC4100_BUFFER_SIZE];
		ViChar deviceName[DC4100_BUFFER_SIZE];
		ViChar serialNumber[DC4100_BUFFER_SIZE];
		ViChar firmwareRevision[DC4100_BUFFER_SIZE];
		MEXMESSAGE(TLDC4100_identificationQuery(instr, manufacturerName, deviceName, serialNumber, firmwareRevision));
#ifndef NDEBUG
		mexPrintf("manufacturerName = %s;\n", manufacturerName);
		mexPrintf("deviceName = %s;\n", deviceName);
		mexPrintf("serialNumber = %s;\n", serialNumber);
		mexPrintf("firmwareRevision = %s;\n", firmwareRevision);
#endif
		/*
		mwSize dims[2];
		dims[0] = 4;
		dims[1] = 1;
		mxArray* result = mxCreateCellArray(2, dims);
		mxSetCell(result, 0, mxCreateString(manufacturerName));
		mxSetCell(result, 1, mxCreateString(deviceName));
		mxSetCell(result, 2, mxCreateString(serialNumber));
		mxSetCell(result, 3, mxCreateString(firmwareRevision));
		*/
		mwSize	dims[2] = {4,2};
		mxArray* result = mxCreateCellArray(2, dims);
		size_t	nsubs = 2, subs[2];
		subs[0] = 0; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("manufacturerName"));
		subs[0] = 0; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(manufacturerName));
		subs[0] = 1; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("deviceName"));
		subs[0] = 1; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(deviceName));
		subs[0] = 2; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("serialNumber"));
		subs[0] = 2; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(serialNumber));
		subs[0] = 3; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("firmwareRevision"));
		subs[0] = 3; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(firmwareRevision));

		return result;
	}

	if (_stricmp("error_query", name) == 0)
	{
		ViInt32 error_code;
		ViChar error_message[DC4100_BUFFER_SIZE];
		ViStatus result = TLDC4100_error_query(instr, &error_code, error_message);
		if (result == VI_SUCCESS)
			return mxCreateString(error_message);
		else if (result == VI_WARN_NSUP_ERROR_QUERY)
			return mxCreateString("The Error Query is not supported by the device.");
		else
			MEXMESSAGE(result);
	}

	if (_stricmp("mode", name) == 0)
	{
		ViInt32 operationMode;
		MEXMESSAGE(TLDC4100_getOperationMode(instr, &operationMode));
		return mxCreateDoubleScalar(static_cast<double>(operationMode));
	}

	if (_stricmp("mode_str", name) == 0)
	{
		ViInt32 operationMode;
		MEXMESSAGE(TLDC4100_getOperationMode(instr, &operationMode));
		char *cMode = NULL;
		switch (operationMode){
		case MODUS_CONST_CURRENT:
			cMode = static_cast<char*>(mxCalloc(8, sizeof(char)));
			strcpy_s(cMode, 8, "current");
			break;
		case MODUS_PERCENT_CURRENT:
			cMode = static_cast<char*>(mxCalloc(11, sizeof(char)));
			strcpy_s(cMode, 11, "brightness");
			break;
		case MODUS_EXTERNAL_CONTROL:
			cMode = static_cast<char*>(mxCalloc(9, sizeof(char)));
			strcpy_s(cMode, 9, "external");
		}
		return mxCreateString(cMode);
	}
	
	if (_stricmp("reset", name) == 0)
	{
		ViStatus result = TLDC4100_reset(instr);
		if (result == VI_SUCCESS) 
			return mxCreateDoubleScalar(1);
		else if (result == VI_WARN_NSUP_RESET)
		{
#ifndef NDEBUG
			mexPrintf("Reset is not supported!\n");
#endif
			return mxCreateDoubleScalar(0);
		}
		else
			MEXMESSAGE(result);
	}

	if (_stricmp("revision", name) == 0)
	{
		ViChar driver_rev[DC4100_BUFFER_SIZE];
		ViChar instr_rev[DC4100_BUFFER_SIZE];
		MEXMESSAGE(TLDC4100_revision_query(instr, driver_rev, instr_rev));
#ifndef NDEBUG
		mexPrintf("driver_rev = %s;\n", driver_rev);
		mexPrintf("instr_rev = %s;\n", instr_rev);
#endif
		mwSize dims[2] = {2,2};
		mxArray* result = mxCreateCellArray(2, dims);
		size_t	nsubs = 2, subs[2];
		subs[0] = 0; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("driver_rev"));
		subs[0] = 0; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(driver_rev));
		subs[0] = 1; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("instr_rev"));
		subs[0] = 1; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(instr_rev));
		return result;
	}

	if (_stricmp("selection", name) == 0)
	{
		ViInt32 selectionMode;
		MEXMESSAGE(TLDC4100_getSelectionMode(instr, &selectionMode));
		return mxCreateDoubleScalar(static_cast<double>(selectionMode));
	}

	if (_stricmp("selection_str", name) == 0)
	{
		ViInt32 selectionMode;
		MEXMESSAGE(TLDC4100_getSelectionMode(instr, &selectionMode));
		char *cMode = NULL;
		switch (selectionMode){
		case SINGLE_SELECT:
			cMode = static_cast<char*>(mxCalloc(7, sizeof(char)));
			strcpy_s(cMode, 7, "single");
			break;
		case MULTI_SELECT:
			cMode = static_cast<char*>(mxCalloc(6, sizeof(char)));
			strcpy_s(cMode, 6, "multi");
		}
		return mxCreateString(cMode);
	}

	if (_stricmp("self_test", name) == 0)
	{
		ViInt16 test_result = 0;
		ViChar test_message[DC4100_BUFFER_SIZE] = "";
		ViStatus result = TLDC4100_self_test(instr, &test_result, test_message);
		if (result == VI_SUCCESS)
		{
#ifndef NDEBUG
			mexPrintf("test_result = %s;\n", test_result);
			mexPrintf("test_message = %s;\n", test_message);
#endif
			mwSize dims[2] = { 2, 2 };
			mxArray* result = mxCreateCellArray(2, dims);
			size_t	nsubs = 2, subs[2];
			subs[0] = 0; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("test_result"));
			subs[0] = 0; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateDoubleScalar(test_result));
			subs[0] = 1; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("test_message"));
			subs[0] = 1; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(test_message));
			return mxCreateDoubleScalar(1);
		}
		else if (result == VI_WARN_NSUP_SELF_TEST)
		{
#ifndef NDEBUG
			mexPrintf("Self test is not supported!\n");
#endif
			return mxCreateDoubleScalar(0);
		}
		else
			MEXMESSAGE(result); // shoot error message
	}

#ifndef NDEBUG
	mexPrintf("%s:%d - ", __FILE__, __LINE__);
#endif
	mexPrintf("\"%s\" unknown.\n", name);
	return NULL;
}


/*	Get a parameter or status value.
*/
mxArray* getParameter2(const char* name, const mxArray* field)
{
	if (mxGetNumberOfElements(field) < 1) return NULL;

	if (leddriver < 1 || leddriver > leddrivers) mexErrMsgTxt("Invalid powermeter handle.");

	if (_stricmp("brightness", name) == 0)
	{
		if ((mxGetNumberOfElements(field) < 1) || (mxGetNumberOfElements(field) > 2))
			mexErrMsgTxt("You need to specify either 1 or 2 parameters - [channel, brightness], where channel = [1..4] and brightness = [0,1]!");

		if (mxGetNumberOfElements(field) == 1)
		{
			double channel = getScalar(field);
			ViReal32 brightness;

			MEXMESSAGE(TLDC4100_getPercentalBrightness(instr, static_cast<ViInt32>(channel - 1), &brightness));
			return mxCreateDoubleScalar(static_cast<double>(brightness/100));
		}
		else
		{
			double* arguments = getArray(field, 2);

			ViInt32 channel = static_cast<ViInt32>(arguments[0]);
			if ((channel < 1) || (channel > NUM_CHANNELS))
			{
				char *errStr = static_cast<char*>(mxCalloc(StringLength, sizeof(char)));
				sprintf_s(errStr, StringLength, "Channel should be an integer in a range[1, %d]!", NUM_CHANNELS);
				mexErrMsgTxt(errStr);
			}

			ViReal32 brightness = 0;
			if ((arguments[1] >= 0.0) || (arguments[1] <= 1.0))
				brightness = static_cast<ViReal32>(arguments[1]*100);
			else
				mexErrMsgTxt("Brighness parameter has to be a value from 0 to 1!");

			MEXMESSAGE(TLDC4100_setPercentalBrightness(instr, channel - 1, brightness));
			return NULL;
		}
	}

	if (_stricmp("current", name) == 0)
	{
		if ((mxGetNumberOfElements(field) < 1) || (mxGetNumberOfElements(field) > 2))
			mexErrMsgTxt("You need to specify either 1 or 2 parameters - [channel, current], where channel = [1..4] and current is in Amp!");

		if (mxGetNumberOfElements(field) == 1)
		{
			double channel = getScalar(field);
			ViReal32 constantCurrent;
			MEXMESSAGE(TLDC4100_getConstCurrent(instr, static_cast<ViInt32>(channel - 1), &constantCurrent));
			return mxCreateDoubleScalar(static_cast<double>(constantCurrent));
		}
		else
		{
			double* arguments = getArray(field, 2);

			ViInt32 channel = static_cast<ViInt32>(arguments[0]);
			if ((channel < 1) || (channel > NUM_CHANNELS))
			{
				char *errStr = static_cast<char*>(mxCalloc(StringLength, sizeof(char)));
				sprintf_s(errStr, StringLength, "Channel should be an integer in a range[1, %d]!", NUM_CHANNELS);
				mexErrMsgTxt(errStr);
			}

			ViReal32 constantCurrent = static_cast<ViReal32>(arguments[1]);

			MEXMESSAGE(TLDC4100_setConstCurrent(instr, channel - 1, constantCurrent));
			return NULL;
		}
	}

	if (_stricmp("current_max_limit", name) == 0)
	{
		if ((mxGetNumberOfElements(field) < 1) || (mxGetNumberOfElements(field) > 2))
			mexErrMsgTxt("You need to specify either 1 or 2 parameters - [channel, current_limit], where channel = [1..4] and current_limit is in Amp!");

		double* arguments = getArray(field, static_cast<int>(mxGetNumberOfElements(field)));
		ViInt32 channel = static_cast<ViInt32>(arguments[0]);
		if ((channel < 1) || (channel > NUM_CHANNELS))
		{
			char *errStr = static_cast<char*>(mxCalloc(StringLength, sizeof(char)));
			sprintf_s(errStr, StringLength, "Channel should be an integer in a range[1, %d]!", NUM_CHANNELS);
			mexErrMsgTxt(errStr);
		}

		if (mxGetNumberOfElements(field) == 1)
		{
			ViReal32 maximumCurrentLimit;
			MEXMESSAGE(TLDC4100_getMaxLimit(instr, static_cast<ViInt32>(channel - 1), &maximumCurrentLimit));
			return mxCreateDoubleScalar(static_cast<double>(maximumCurrentLimit));
		}
		else
		{
			ViReal32 maximumCurrentLimit = static_cast<ViReal32>(arguments[1]);
			MEXMESSAGE(TLDC4100_setMaxLimit(instr, channel - 1, maximumCurrentLimit));
			return NULL;
		}
	}

	if (_stricmp("current_user_limit", name) == 0)
	{
		if ((mxGetNumberOfElements(field) < 1) || (mxGetNumberOfElements(field) > 2))
			mexErrMsgTxt("You need to specify either 1 or 2 parameters - [channel, current_limit], where channel = [1..4] and current_limit is in Amp!");

		double* arguments = getArray(field, static_cast<int>(mxGetNumberOfElements(field)));
		ViInt32 channel = static_cast<ViInt32>(arguments[0]);
		if ((channel < 1) || (channel > NUM_CHANNELS))
		{
			char *errStr = static_cast<char*>(mxCalloc(StringLength, sizeof(char)));
			sprintf_s(errStr, StringLength, "Channel should be an integer in a range[1, %d]!", NUM_CHANNELS);
			mexErrMsgTxt(errStr);
		}

		if (mxGetNumberOfElements(field) == 1)
		{
			ViReal32 currentLimit;
			MEXMESSAGE(TLDC4100_getLimitCurrent(instr, static_cast<ViInt32>(channel - 1), &currentLimit));
			return mxCreateDoubleScalar(static_cast<double>(currentLimit));
		}
		else
		{
			ViReal32 currentLimit = static_cast<ViReal32>(arguments[1]);
			MEXMESSAGE(TLDC4100_setLimitCurrent(instr, channel - 1, currentLimit));
			return NULL;
		}
	}

	if (_stricmp("emission", name) == 0)
	{
		if ((mxGetNumberOfElements(field) < 1) || (mxGetNumberOfElements(field) > 2))
			mexErrMsgTxt("You need to specify either 1 or 2 parameters - [channel, state], where channel = [1..4] and state = [0;1]!");

		if (mxGetNumberOfElements(field) == 1)
		{
			double channel = getScalar(field);
			ViBoolean LEDOutputState;

			MEXMESSAGE(TLDC4100_getLedOnOff(instr, static_cast<ViInt32>(channel - 1), &LEDOutputState));
			return mxCreateDoubleScalar(static_cast<double>(LEDOutputState));
		}
		else
		{
			double* arguments = getArray(field, 2);

			ViInt32 channel = static_cast<ViInt32>(arguments[0]);
			if ((channel < 1) || (channel > NUM_CHANNELS))
			{
				char *errStr = static_cast<char*>(mxCalloc(StringLength, sizeof(char)));
				sprintf_s(errStr, StringLength, "Channel should be an integer in a range[1, %d]!", NUM_CHANNELS);
				mexErrMsgTxt(errStr);
			}

			ViBoolean state = false;
			if ((arguments[1] == 0) || (arguments[1] == 1))
				state = static_cast<ViBoolean>(arguments[1]);
			else
				mexErrMsgTxt("State parameter has to be 0 for Off or 1 for On!");

			MEXMESSAGE(TLDC4100_setLedOnOff(instr, channel - 1, state));
			return NULL;
		}
	}

	if (_stricmp("error_message", name) == 0)
	{
		if ((mxGetNumberOfElements(field) < 1) || (mxGetNumberOfElements(field) > 1))
			mexErrMsgTxt("You need to specify 1 parameter - error code!");

		ViStatus status_code = static_cast<ViStatus >(getScalar(field));

		ViChar message[2*DC4100_BUFFER_SIZE] = "";
		ViStatus result = TLDC4100_error_message(instr, status_code, message);
		if (result == VI_SUCCESS)
			return mxCreateString(message);
		else if (result == VI_WARN_UNKNOWN_STATUS)
			return mxCreateString("The Error Code cannot be interpreted.");
		else
			MEXMESSAGE(result);
	}

	if (_stricmp("forward_bias", name) == 0)
	{
		if (mxGetNumberOfElements(field) != 1)
			mexErrMsgTxt("You need to specify 1 parameters - channel number!");

		ViInt32 channel = static_cast<ViInt32>(getScalar(field));
		if ((channel < 1) || (channel > NUM_CHANNELS))
		{
			char *errStr = static_cast<char*>(mxCalloc(StringLength, sizeof(char)));
			sprintf_s(errStr, StringLength, "Channel should be an integer in a range[1, %d]!", NUM_CHANNELS);
			mexErrMsgTxt(errStr);
		}

		ViReal32 forwardBias;
		MEXMESSAGE(TLDC4100_getForwardBias(instr, static_cast<ViInt32>(channel - 1), &forwardBias));
		return mxCreateDoubleScalar(static_cast<double>(forwardBias));
	}

	if (_stricmp("head_info", name) == 0)
	{
		if ((mxGetNumberOfElements(field) < 1) || (mxGetNumberOfElements(field) > 1))
			mexErrMsgTxt("You need to specify 1 parameter - channel!");

		ViInt32 channel = static_cast<ViInt32>(getScalar(field) - 1);
		ViChar serialNumber[DC4100_BUFFER_SIZE] = "";
		ViChar name[DC4100_BUFFER_SIZE] = "";
		ViInt32 type;
		MEXMESSAGE(TLDC4100_getHeadInfo(instr, channel, serialNumber, name, &type));
#ifndef NDEBUG
		mexPrintf("serialNumber = %s;\n", serialNumber);
		mexPrintf("name = %s;\n", name);
		mexPrintf("type = %i;\n", type);
#endif
		mwSize dims[2] = { 3, 2 };
		mxArray* result = mxCreateCellArray(2, dims);
		size_t	nsubs = 2, subs[2];
		subs[0] = 0; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("serialNumber"));
		subs[0] = 0; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(serialNumber));
		subs[0] = 1; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("name"));
		subs[0] = 1; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString(name));
		subs[0] = 2; subs[1] = 0; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateString("type"));
		subs[0] = 2; subs[1] = 1; mxSetCell(result, mxCalcSingleSubscript(result, nsubs, subs), mxCreateDoubleScalar(type));
		return result;
	}

	if (_stricmp("wavelength", name) == 0)
	{
		if ((mxGetNumberOfElements(field) < 1) || (mxGetNumberOfElements(field) > 1))
			mexErrMsgTxt("You need to specify 1 parameter - channel!");

		ViInt32 channel = static_cast<ViInt32>(getScalar(field)-1);
		ViReal32 wavelength;
		MEXMESSAGE(TLDC4100_getWavelength(instr, channel, &wavelength));
		return mxCreateDoubleScalar(static_cast<double>(wavelength));
	}

#ifndef NDEBUG
	mexPrintf("%s:%d - ", __FILE__, __LINE__);
#endif
	mexPrintf("\"%s\" unknown.\n", name);
	return NULL;
}


/*	Set a measurement parameter.
*/
void setParameter(const char* name, const mxArray* field)
{
	if (mxGetNumberOfElements(field) < 1) return;
	if (_stricmp("leddriver", name) == 0) return;
	if (_stricmp("leddrivers", name) == 0) return;
	if (leddriver < 1 || leddriver > leddrivers) mexErrMsgTxt("Invalid powermeter handle.");

	if (_stricmp("display_brightness", name) == 0)
	{
		double displayBrightness = getScalar(field);
		MEXMESSAGE(TLDC4100_setDispBright(instr, static_cast<ViInt32>(displayBrightness*100)));
		return;
	}

	if (_stricmp("mode", name) == 0)
	{
		ViInt32 operationMode = -1;
		if (mxIsChar(field))
		{ 
			char *cMode;
			cMode = getString(field);
			if (_stricmp("current", cMode) == 0)
				operationMode = MODUS_CONST_CURRENT;
			else if (_stricmp("brightness", cMode) == 0)
				operationMode = MODUS_PERCENT_CURRENT;
			else if (_stricmp("external", cMode) == 0)
				operationMode = MODUS_EXTERNAL_CONTROL;
			mxFree(cMode);
		}
		else
		{
			double dMode = getScalar(field);
			if ((dMode == MODUS_CONST_CURRENT) || (dMode == MODUS_PERCENT_CURRENT) || (dMode == MODUS_EXTERNAL_CONTROL))
				operationMode = static_cast<ViInt32>(dMode);
			/*
			switch (static_cast<int>(dMode)){
			case 0:
				operationMode = MODUS_CONST_CURRENT;
				break;
			case 1:
				operationMode = MODUS_PERCENT_CURRENT;
				break;
			case 2:
				operationMode = MODUS_EXTERNAL_CONTROL;
			}
			*/
		}
		if ( operationMode == -1 )
			mexErrMsgTxt("Operation mode should be either [0;1;2] or ['current','brightness','external']\n");
		else
			MEXMESSAGE(TLDC4100_setOperationMode(instr, operationMode));
		return;
	}

	if (_stricmp("selection", name) == 0)
	{
		ViInt32 selectionMode = -1;
		if (mxIsChar(field))
		{
			char *cMode;
			cMode = getString(field);
			if (_stricmp("single", cMode) == 0)
				selectionMode = SINGLE_SELECT;
			else if (_stricmp("multi", cMode) == 0)
				selectionMode = MULTI_SELECT;
			mxFree(cMode);
		}
		else
		{
			double dMode = getScalar(field);
			if ((dMode == MULTI_SELECT) || (dMode == SINGLE_SELECT))
				selectionMode = static_cast<ViInt32>(dMode);
		}
		if (selectionMode == -1)
			mexErrMsgTxt("Selection mode should be either [0;1] or ['single','multi']\n");
		else
			MEXMESSAGE(TLDC4100_setSelectionMode(instr, selectionMode));
		return;
	}


#ifndef NDEBUG
	mexPrintf("%s:%d - ", __FILE__, __LINE__);
#endif
	mexPrintf("\"%s\" unknown.\n", name);
	return;
}


