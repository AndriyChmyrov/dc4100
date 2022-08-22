#define VALUE	plhs[0]
#define StringLength VI_FIND_BUFLEN
#define MAX_NUMBER_OF_SUPPORTED_INSTRUMENTS	10

#define MEXMESSAGE(error) mexMessage(__FILE__,__LINE__,error)


extern int leddriver, leddrivers;
extern ViSession	instr;

extern ViChar*	rscPtr[MAX_NUMBER_OF_SUPPORTED_INSTRUMENTS];						// array of pointer to resource string
extern ViChar*	aliasPtr[MAX_NUMBER_OF_SUPPORTED_INSTRUMENTS];


ViStatus readInstrData(ViSession resMgr, ViRsrc rscStr, ViChar *name, ViChar *descr, ViAccessMode *lockState);
ViStatus CleanupScan(ViSession resMgr, ViFindList findList, ViStatus err);

void mexCleanup(void);
void mexMessage(const char* file, const int line, ViStatus error);

double getScalar(const mxArray* array);
mxArray* getParameter(const char* name);
mxArray* getParameter2(const char* name, const mxArray* field);
void setParameter(const char* name, const mxArray* field);