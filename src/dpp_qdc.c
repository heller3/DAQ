/******************************************************************************
 * 
 * CAEN SpA - Front End Division
 * Via Vetraia, 11 - 55049 - Viareggio ITALY
 * +390594388398 - www.caen.it
 *
 ******************************************************************************/
#include "dpp_qdc.h"
#include "X742CorrectionRoutines.h"
#include <float.h> 
/* Globals */

//////////////////////modified apolesel////////////////////////

int GetNextEvent[2]={1,1};
int MatchingEvents=0;


/* Variable declarations */
unsigned int gActiveChannel;
unsigned int gEquippedChannels;
unsigned int gEquippedGroups;

long         gCurrTime;
long         gPrevTime;
long         gPrevWPlotTime;
long         gPrevHPlotTime;
long         tPrevWPlotTime;

long         gRunStartTime;
long         gRunElapsedTime;

uint32_t     gEvCnt[MAX_CHANNELS];
uint32_t     gEvCntOld[MAX_CHANNELS];
uint32_t**   gHisto; 

uint64_t     gExtendedTimeTag[MAX_CHANNELS];
uint64_t     gETT[MAX_CHANNELS];
uint64_t     gPrevTimeTag[MAX_CHANNELS];

BoardParameters   gParams;
Stats        gAcqStats  ;

FILE *       gPlotDataFile;
// FILE *       gListFiles[MAX_CHANNELS];
// FILE *       timeTags;
// FILE *       tttFile;
FILE *       sStamps740;
FILE **      sStamps742;

FILE *       binOut740;
FILE **      binOut742;


CAEN_DGTZ_BoardInfo_t             gBoardInfo;
_CAEN_DGTZ_DPP_QDC_Event_t*     gEvent[MAX_CHANNELS];
_CAEN_DGTZ_DPP_QDC_Waveforms_t* gWaveforms;

/* Global variables definition */
int          gSWTrigger       = 0;
int          grp4stats   = 0; /* Select group for statistics update on screen */
int          gLoops       = 0;
int          gToggleTrace = 0;
int          gAnalogTrace = 0;
int          gRestart     = 0;
int          gAcquisitionBufferAllocated = 0;

char *       gAcqBuffer      = NULL;
char *       gEventPtr      = NULL;
FILE *       gHistPlotFile       = NULL;
FILE *       gWavePlotFile       = NULL;


CAEN_DGTZ_X742_EVENT_t* Event742[2];
char* buffer742[2];
uint32_t buffer742Size[2];
DataCorrection_t* Table;

uint64_t** Nroll;
double **TTT, **PrevTTT;

Data740_t Data740;
Data742_t *Data742;

double Ts, Tt;
double realTime740 = 0;
uint64_t temp_gPrevTimeTag = 0;
uint64_t temp_gETT = 0;
uint64_t temp_gExtendedTimeTag = 0;

int foundEvent[3] = {0,0,0};

FILE *event_file = NULL;
FILE *plotter = NULL;

_CAEN_DGTZ_DPP_QDC_Event_t *gEventsGrp[8] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

#ifdef LINUX
#include <sys/time.h> /* struct timeval, select() */
#include <termios.h> /* tcgetattr(), tcsetattr() */
#include <stdlib.h> /* atexit(), exit() */
#include <unistd.h> /* read() */
#include <stdio.h> /* printf() */
#include <string.h> /* memcpy() */

#define CLEARSCR "clear"

#define WRITE_DATA 1
// #define OPTIMIZATION_RUN 1

static struct termios g_old_kbd_mode;

/*****************************************************************************/
static void cooked(void)
{
  tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}

static void raw(void)
{
  static char init;
  /**/
  struct termios new_kbd_mode;
  
  if(init)
    return;
  /* put keyboard (stdin, actually) in raw, unbuffered mode */
  tcgetattr(0, &g_old_kbd_mode);
  memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
  new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
  new_kbd_mode.c_cc[VTIME] = 0;
  new_kbd_mode.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &new_kbd_mode);
  /* when we exit, go back to normal, "cooked" mode */
  atexit(cooked);
  
  init = 1;
}

/*****************************************************************************/
/*  SLEEP  */
/*****************************************************************************/
void Sleep(int t) {
  usleep( t*1000 );
}

/*****************************************************************************/
/*  GETCH  */
/*****************************************************************************/
int getch(void)
{
  unsigned char temp;
  
  raw();
  /* stdin = fd 0 */
  if(read(0, &temp, 1) != 1)
    return 0;
  return temp;
  
}


/*****************************************************************************/
/*  KBHIT  */
/*****************************************************************************/
int kbhit()
{
  
  struct timeval timeout;
  fd_set read_handles;
  int status;
  
  raw();
  /* check stdin (fd 0) for activity */
  FD_ZERO(&read_handles);
  FD_SET(0, &read_handles);
  timeout.tv_sec = timeout.tv_usec = 0;
  status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
  if(status < 0)
  {
    printf("select() failed in kbhit()\n");
    exit(1);
  }
  return (status);
}


#else  /* Windows */

#include <conio.h>
#define CLEARSCR "cls"

#endif

/* Get time in milliseconds from the computer internal clock */
long get_time()
{
  long time_ms;
  #ifdef WIN32
  struct _timeb timebuffer;
  _ftime( &timebuffer );
  time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
  #else
  struct timeval t1;
  struct timezone tz;
  gettimeofday(&t1, &tz);
  time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
  #endif
  return time_ms;
}

void clear_screen()
{
  system(CLEARSCR);  
}

/*
 * * Set default parameters for the acquisition
 */
void set_default_parameters(BoardParameters *params) {
  int i;
  
  params->OutputMode        = OUTPUTMODE_BINARY;
  params->RecordLength      = 200;		   /* Number of samples in the acquisition window (waveform mode only)    */
  params->PreTrigger        = 100;  		   /* PreTrigger is in number of samples                                  */
  params->ActiveChannel     = 0;			   /* Channel used for the data analysis (plot, histograms, etc...)       */
  params->BaselineMode      = 2;			   /* Baseline: 0=Fixed, 1=4samples, 2=16 samples, 3=64samples            */
  params->FixedBaseline     = 2100;		   /* fixed baseline (used when BaselineMode = 0)                         */
  params->PreGate           = 20;			   /* Position of the gate respect to the trigger (num of samples before) */
  params->TrgHoldOff        = 10;            /* */ 
  params->TrgMode           = 0;             /* */
  params->TrgSmoothing      = 0;             /* */
  params->SaveList          = 0;             /*  Set flag to save list events to file */
  params->DCoffset[0]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
  params->DCoffset[1]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
  params->DCoffset[2]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
  params->DCoffset[3]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
  params->DCoffset[4]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
  params->DCoffset[5]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
  params->DCoffset[6]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
  params->DCoffset[7]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
  params->ChargeSensitivity = 2;             /* Charge sesnitivity (0=max, 7=min)                    */
  params->NevAggr           = 1;             /* Number of events per aggregate (buffer). 0=automatic */
  params->AcqMode           = ACQMODE_LIST;  /* Acquisition Mode (LIST or MIXED)                     */
  params->PulsePol          = 1;             /* Pulse Polarity (1=negative, 0=positive)              */
  params->EnChargePed       = 0;             /* Enable Fixed Charge Pedestal in firmware (0=off, 1=on (1024 pedestal)) */
  params->DisTrigHist       = 0;             /* 0 = Trigger Histeresys on; 1 = Trigger Histeresys off */
  params->DefaultTriggerThr = 10;            /* Default threshold for trigger                        */
  params->RunDelay          = 0;
  
  for(i = 0; i < 8; ++i)
    params->GateWidth[i]  = 40;	   /* Gate Width in samples                                */
    
  for(i = 0; i < 8; ++i)		
    params->DCoffset[i] = 0x8000;            /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
      
  for(i = 0; i < 64; ++i) {
        params->TriggerThreshold[i] = params->DefaultTriggerThr;
  }
      params->registerWritesCounter = 0;
    params->NumOfV1742            = 0;  // by default 0
    
    params->v1742_TriggerEdge     = 0;  // by default 0  
    params->v1742_RecordLength    = 0;  // by default 0
    params->v1742_MatchingWindow  = 0;  // by default 0
    params->v1742_IOlevel         = 0;  // by default 0
    params->v1742_TestPattern     = 0;  // by default 0
    params->v1742_DRS4Frequency   = 0;  // by default 0
    params->v1742_StartMode       = 0;  // by default 0
    params->v1742_EnableLog       = 0;  // by default 0
    //V1742
    
    for(i = 0; i < 8 ; i++) {
      params->v1742_ConnectionType[i]       = 0;
      params->v1742_LinkNum[i]              = 0;
      params->v1742_ConetNode[i]            = 0;
      params->v1742_BaseAddress[i]          = 0;
      params->v1742_FastTriggerThreshold[i] = 0;       
      params->v1742_FastTriggerOffset[i]    = 0;          
      params->v1742_DCoffset[i]             = 0;                   
      params->v1742_ChannelThreshold[i]     = 0;           
      params->v1742_TRThreshold[i]          = 0;                
      params->v1742_ChannelPulseEdge[i]     = 0;           
      params->v1742_PostTrigger[i]          = 0;
      params->v1742_RunDelay[i]             = 0;                       
      
    }
    
    for(i = 0; i < 64 ; i++) {
      params->RegisterWriteBoard[i] = 65;
      params->RegisterWriteAddr[i]  = 0;
      params->RegisterWriteValue[i] = 0;
    }
    
}

/*
 * * Read config file and assign new values to the parameters
 ** Returns 0 on success; -1 in case of any error detected.
 */
int load_configuration_from_file(char * fname, BoardParameters *params) {
  FILE *parameters_file;
  
  params->ConnectionType = CONNECTION_TYPE_AUTO;
  
  parameters_file = fopen(fname, "r");
  if (parameters_file != NULL) {
    while (!feof(parameters_file)) {
      char str[100];
      fscanf(parameters_file, "%s", str);
      if (str[0] == '#') {
        fgets(str, 100, parameters_file);
        continue;
      }
      
      //       printf("%s\n",str);
      
      
      
      if (strcmp(str, "OutputMode") == 0) {
        char str1[100];
        fscanf(parameters_file, "%s", str1);
        if (strcmp(str1, "BINARY") == 0) 
          params->OutputMode = OUTPUTMODE_BINARY;
        if (strcmp(str1, "TEXT")  == 0) 
          params->OutputMode = OUTPUTMODE_TEXT;
        if (strcmp(str1, "BOTH")  == 0) 
          params->OutputMode = OUTPUTMODE_BOTH;
      }
      
      if (strcmp(str, "AcquisitionMode") == 0) {
        char str1[100];
        fscanf(parameters_file, "%s", str1);
        if (strcmp(str1, "MIXED") == 0) 
          params->AcqMode = ACQMODE_MIXED;
        if (strcmp(str1, "LIST")  == 0) 
          params->AcqMode = ACQMODE_LIST;
      }
      
      if (strcmp(str, "ConnectionType") == 0) {
        char str1[100];
        fscanf(parameters_file, "%s", str1);
        if (strcmp(str1, "USB") == 0) 
          params->ConnectionType = CONNECTION_TYPE_USB;
        if (strcmp(str1, "OPT")  == 0) 
          params->ConnectionType = CONNECTION_TYPE_OPT;
      }
      
      
      if (strcmp(str, "ConnectionLinkNum") == 0) 
        fscanf(parameters_file, "%d", &params->ConnectionLinkNum);
      if (strcmp(str, "ConnectionConetNode") == 0) 
        fscanf(parameters_file, "%d", &params->ConnectionConetNode);
      if (strcmp(str, "ConnectionVmeBaseAddress") == 0) 
        fscanf(parameters_file, "%llx", &params->ConnectionVMEBaseAddress);	
      
      if (strcmp(str, "TriggerThreshold") == 0) {
        int ch;
        fscanf(parameters_file, "%d", &ch);
        fscanf(parameters_file, "%d", &params->TriggerThreshold[ch]);
      }
      
      if (strcmp(str, "RecordLength") == 0) 
        fscanf(parameters_file, "%d", &params->RecordLength);
      if (strcmp(str, "PreTrigger") == 0)
        fscanf(parameters_file, "%d", &params->PreTrigger);
      if (strcmp(str, "ActiveChannel") == 0) 
        fscanf(parameters_file, "%d", &params->ActiveChannel);
      if (strcmp(str, "BaselineMode") == 0) 
        fscanf(parameters_file, "%d", &params->BaselineMode);
      if (strcmp(str, "TrgMode") == 0) 
        fscanf(parameters_file, "%d", &params->TrgMode);
      if (strcmp(str, "TrgSmoothing") == 0) 
        fscanf(parameters_file, "%d", &params->TrgSmoothing);
      if (strcmp(str, "TrgHoldOff") == 0) 
        fscanf(parameters_file, "%d", &params->TrgHoldOff);
      if (strcmp(str, "FixedBaseline") == 0) 
        fscanf(parameters_file, "%d", &params->FixedBaseline);
      if (strcmp(str, "PreGate") == 0) 
        fscanf(parameters_file, "%d", &params->PreGate);
      if (strcmp(str, "GateWidth") == 0) {
        int gr;
        fscanf(parameters_file, "%d", &gr);
        fscanf(parameters_file, "%d", &params->GateWidth[gr]);
      }
      if (strcmp(str, "DCoffset") == 0) {
        int ch;
        fscanf(parameters_file, "%d", &ch);
        fscanf(parameters_file, "%d", &params->DCoffset[ch]);
      }
      if (strcmp(str, "ChargeSensitivity") == 0) 
        fscanf(parameters_file, "%d", &params->ChargeSensitivity);
      if (strcmp(str, "NevAggr") == 0) 
        fscanf(parameters_file, "%d", &params->NevAggr);
      if (strcmp(str, "SaveList") == 0) 
        fscanf(parameters_file, "%d", &params->SaveList);
      if (strcmp(str, "ChannelTriggerMask") == 0) 
        fscanf(parameters_file, "%llx", &params->ChannelTriggerMask);	
      if (strcmp(str, "PulsePolarity") == 0) 
        fscanf(parameters_file, "%d", &params->PulsePol);
      if (strcmp(str, "EnableChargePedestal") == 0) 
        fscanf(parameters_file, "%d", &params->EnChargePed);
      if (strcmp(str, "DisableTriggerHysteresis") == 0) 
        fscanf(parameters_file, "%d", &params->DisTrigHist);
      if (strcmp(str, "DisableSelfTrigger") == 0) 
        fscanf(parameters_file, "%d", &params->DisSelfTrigger);
      if (strcmp(str, "EnableTestPulses") == 0) 
        fscanf(parameters_file, "%d", &params->EnTestPulses);
      if (strcmp(str, "TestPulsesRate") == 0) 
        fscanf(parameters_file, "%d", &params->TestPulsesRate);
      if (strcmp(str, "DefaultTriggerThr") == 0) 
        fscanf(parameters_file, "%d", &params->DefaultTriggerThr);
      if (strcmp(str, "EnableExtendedTimeStamp") == 0) 
        fscanf(parameters_file, "%d", &params->EnableExtendedTimeStamp);
      if (strcmp(str, "RunDelay") == 0) 
        fscanf(parameters_file, "%d", &params->RunDelay);
      
      //V1742
      
      if (strcmp(str, "NumOfV1742") == 0) {
        fscanf(parameters_file, "%d", &params->NumOfV1742);
      }
      if (strcmp(str, "v1742_TriggerEdge") == 0) {
        fscanf(parameters_file, "%d", &params->v1742_TriggerEdge);
      }
      if (strcmp(str, "v1742_RecordLength") == 0) {
        fscanf(parameters_file, "%d", &params->v1742_RecordLength);
      } 
      if (strcmp(str, "v1742_MatchingWindow") == 0) {
        fscanf(parameters_file, "%d", &params->v1742_MatchingWindow);
      }
      if (strcmp(str, "v1742_IOlevel") == 0) {
        fscanf(parameters_file, "%d", &params->v1742_IOlevel);
      }      
      if (strcmp(str, "v1742_TestPattern") == 0) {
        fscanf(parameters_file, "%d", &params->v1742_TestPattern);
      }    
      if (strcmp(str, "v1742_DRS4Frequency") == 0) {
        fscanf(parameters_file, "%d", &params->v1742_DRS4Frequency);
      }  
      if (strcmp(str, "v1742_StartMode") == 0) {
        fscanf(parameters_file, "%d", &params->v1742_StartMode);
      }      
      if (strcmp(str, "v1742_EnableLog") == 0) {
        fscanf(parameters_file, "%d", &params->v1742_EnableLog);
      }      
      
      
      
      if (strcmp(str, "v1742_ConnectionType") == 0) {
        int digi;
        char str1[100];
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%s", str1);
        if (strcmp(str1, "USB") == 0) 
          params->v1742_ConnectionType[digi] = CONNECTION_TYPE_USB;
        if (strcmp(str1, "OPT")  == 0) 
          params->v1742_ConnectionType[digi] = CONNECTION_TYPE_OPT;
      }
      
      if (strcmp(str, "v1742_LinkNum") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_LinkNum[digi]);
      }
      
      if (strcmp(str, "v1742_ConetNode") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_ConetNode[digi]);
      }
      
      if (strcmp(str, "v1742_BaseAddress") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%llx", &params->v1742_BaseAddress[digi]);	
      }
      
      if (strcmp(str, "v1742_FastTriggerThreshold") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_FastTriggerThreshold[digi]);
      }        
      if (strcmp(str, "v1742_FastTriggerOffset") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_FastTriggerOffset[digi]);
      }              
      if (strcmp(str, "v1742_DCoffset") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%llx", &params->v1742_DCoffset[digi]);
      }                                              
      if (strcmp(str, "v1742_ChannelThreshold") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_ChannelThreshold[digi]);
      }                
      if (strcmp(str, "v1742_TRThreshold") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_TRThreshold[digi]);
      }                          
      if (strcmp(str, "v1742_ChannelPulseEdge") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_ChannelPulseEdge[digi]);
      }                
      if (strcmp(str, "v1742_PostTrigger") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_PostTrigger[digi]);
      }
      if (strcmp(str, "v1742_RunDelay") == 0) {
        int digi;
        fscanf(parameters_file, "%d", &digi);
        fscanf(parameters_file, "%d", &params->v1742_RunDelay[digi]);
      }
      
      //generic writes
      if (strcmp(str, "RegisterWrite") == 0) {
        fscanf(parameters_file, "%d", &params->RegisterWriteBoard[params->registerWritesCounter]);
        fscanf(parameters_file, "%llx", &params->RegisterWriteAddr[params->registerWritesCounter]);
        fscanf(parameters_file, "%llx", &params->RegisterWriteValue[params->registerWritesCounter]);
        params->registerWritesCounter++;
      }
    }
    fclose(parameters_file);
    
    return 0;
  }
  return -1;
}


/* Set parameters for the acquisition */
int setup_parameters(BoardParameters *params, char *fname) {
  int ret;
  set_default_parameters(params);
  
  ret = load_configuration_from_file(fname, params);
  return ret;
}


// ASSUMING NIM PULSE for trigger and negative pulse for nino, I.E. negative pulses on both trigger and signal
double interpolateWithMultiplePoints(float* data, unsigned int length, int threshold, CAEN_DGTZ_TriggerPolarity_t edge, double Tstart,int WaveType)
{
  // WaveType = 0   -> Pulse wave
  // WaveType = 1   -> Trigger wave
  
  unsigned int i,j;
  double crosspoint = -1.0;
  
  //find min and max of this wave
  float min = FLT_MAX;
  float max = -FLT_MAX;
  for (i=0; i < length; i++) {
    if(data[i] < min ) min = data[i];
    if(data[i] > max ) max = data[i];
  }
  
  //set the flex point to 50% vertical swing
  //we compute a distance of 50%
//   float halfDistance = fabs(min-max) / 2.0;
  
//   if(halfDistance < 300.0 ) return crosspoint;
  
  // first find the baseline for this wave
  int baseLineSamples = 300; 
  double baseline = 0.0;
  for (i=(int)(Tstart); i < baseLineSamples; ++i) {
    baseline += ((double)data[i])/((double) baseLineSamples);
  }
  
  //find the pulse beging and end, i.e. when the signal crosses a value that is more than  
  
//   printf("Baseline = %f\n",baseline);
  //then re-define the threshold as a distance 1100 from baseline
  //not general implementation, works well only for identical signals of this peculiar amplitude 
  //double thresholdReal = (double) threshold;
  
//   printf("%f\n",halfDistance);
//   halfDistance = 700.0;
  double distanceToFlexPoint;
  if(WaveType == 0)
    distanceToFlexPoint = 800;
  if(WaveType == 1)
    distanceToFlexPoint = 650;
  double thresholdReal = baseline - distanceToFlexPoint;
  
  int samplesNum = 7; // N = 3
  //now take +/- N bins and calculate the regression lines 
  float slope = 0; 
  float intercept = 0;
  float sumx =0;
  float sumy =0;
  float sumxy=0;
  float sumx2=0;
  int n = samplesNum * 2;
//   for(j = centerBin[0] - samplesNum; j < centerBin[0] + samplesNum; j++) 
//   { 
//     sumx += (float) j;
//     sumy += wave0[j];
//     sumxy += j*wave0[j];
//     sumx2 += j*j;
//   }
//   slope[0] =  (n*sumxy - sumx*sumy) / (n*sumx2 - sumx*sumx);
//   intercept[0] = (sumy - slope[0] * sumx) / (n);
  for (i=(int)(Tstart); i < length-1; ++i) {
    if ((edge == CAEN_DGTZ_TriggerOnFallingEdge) && (data[i] >= thresholdReal) && (data[i+1] < thresholdReal)) {
      for(j = i - samplesNum; j < i + samplesNum; j++) {
        sumx += (float) j;
        sumy += data[j];
        sumxy += j*data[j];
        sumx2 += j*j;
      }
      slope =  (n*sumxy - sumx*sumy) / (n*sumx2 - sumx*sumx);
      intercept = (sumy - slope * sumx) / (n);
      crosspoint = (thresholdReal - intercept) / slope;
      //crosspoint = i + ((double)(data[i] - threshold)/(double)(data[i]-data[i+1]));
      break;
    }
    if ((edge == CAEN_DGTZ_TriggerOnRisingEdge) && (data[i] <= thresholdReal) && (data[i+1] > thresholdReal)) {
      for(j = i - samplesNum; j < i + samplesNum; j++) {
        sumx += (float) j;
        sumy += data[j];
        sumxy += j*data[j];
        sumx2 += j*j;
      }
      slope =  (n*sumxy - sumx*sumy) / (n*sumx2 - sumx*sumx);
      intercept = (sumy - slope * sumx) / (n);
      crosspoint = (thresholdReal - intercept) / slope;
      //crosspoint = i + ((double)(threshold - data[i])/(double)(data[i+1]-data[i])) ;
      break;
    }
  }
  
  
  return crosspoint;
}





int configure_timing_digitizers(/*int *tHandle,*/ BoardParameters *params)
{
  int ret = 0;
  int i;
  CAEN_DGTZ_BoardInfo_t BoardInfo[8];
  
  for(i = 0 ; i < params->NumOfV1742; i++)
  {
    ret |= CAEN_DGTZ_OpenDigitizer(params->v1742_ConnectionType[i], params->v1742_LinkNum[i], params->v1742_ConetNode[i], params->v1742_BaseAddress[i], &tHandle[i]); 
    
    //get info
    ret |= CAEN_DGTZ_GetInfo(tHandle[i], &BoardInfo[i]);
    if (ret) {
      printf("Can't read board info for timing digitizer n. %d\n", i);
      return -1;
    }
    
    /* Check BoardType */
    if (BoardInfo[i].FamilyCode!= 6) {
      printf("This digitizer is not a V1742; Not supported\n");      
      return -1;
    } 
    
    printf("Connected to CAEN Digitizer Model %s\n", BoardInfo[i].ModelName);
    printf("ROC FPGA Release is %s\n", BoardInfo[i].ROC_FirmwareRel);
    printf("AMC FPGA Release is %s\n", BoardInfo[i].AMC_FirmwareRel);
    printf("VME Handle is %d\n", BoardInfo[i].VMEHandle);
    printf("Serial Number is %d\n\n", BoardInfo[i].SerialNumber);
    //     printf("%d %d %d\n",i,tHandle[i],ret);
    ret |= CAEN_DGTZ_Reset(tHandle[i]);
    
    if (params->v1742_TestPattern) {
      ret = CAEN_DGTZ_WriteRegister(tHandle[i], CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);
    }
    
    if (params->v1742_DRS4Frequency) {
      ret |= CAEN_DGTZ_SetDRS4SamplingFrequency(tHandle[i], params->v1742_DRS4Frequency);
    }
    
    CAEN_DGTZ_DRS4Frequency_t frequency;
    ret |= CAEN_DGTZ_GetDRS4SamplingFrequency (tHandle[i], &frequency);
    
    double Ts;
//     printf("frequency %ld\n",frequency);
    printf("DRS4 Sampling Frequency is ");
    if( frequency == CAEN_DGTZ_DRS4_5GHz )   Ts = 0.2;
    if( frequency == CAEN_DGTZ_DRS4_2_5GHz ) Ts = 0.4;
    if( frequency == CAEN_DGTZ_DRS4_1GHz )   Ts = 1.0;
    if( frequency == CAEN_DGTZ_DRS4_750MHz ) Ts = 1.0/0.75;
//     switch((int) frequency) {
//         case CAEN_DGTZ_DRS4_5GHz: Ts = 0.2; break;
//         case CAEN_DGTZ_DRS4_2_5GHz: Ts =  0.4; break;
//         case CAEN_DGTZ_DRS4_1GHz: Ts =  1.0; break;
//         default: Ts = 0.2; break;
//     }
    printf("%f GHz\n\n",1.0/Ts);
    
    if (ret = LoadCorrectionTables(tHandle[i], &Table[i], 0, params->v1742_DRS4Frequency))
    {
      printf("Error loading data correction tables\n");      
      return -1;
    }
    printf("Correction Tables Loaded!\n");
    if ((ret = CAEN_DGTZ_EnableDRS4Correction(tHandle[i])) != CAEN_DGTZ_Success)
    {
      printf("Error loading data correction tables\n");      
      return -1;
    }
    printf("Correction Tables enabled!\n\n");
    ret |= CAEN_DGTZ_SetRecordLength(tHandle[i], params->v1742_RecordLength); 
    ret |= CAEN_DGTZ_SetPostTriggerSize(tHandle[i], params->v1742_PostTrigger[i]);
    ret |= CAEN_DGTZ_SetIOLevel(tHandle[i], params->v1742_IOlevel);
    ret |= CAEN_DGTZ_SetMaxNumEventsBLT(tHandle[i], MAX_EVENTS_XFER);
    ret |= CAEN_DGTZ_SetGroupEnableMask(tHandle[i], 0x000F);   // fixed to enable all groups
    
    int gr,ch;
    for(gr = 0 ; gr < 4 ; gr++)
    {
      ret |= CAEN_DGTZ_SetGroupFastTriggerThreshold(tHandle[i], gr, params->v1742_FastTriggerThreshold[i]);
      ret |= CAEN_DGTZ_SetGroupFastTriggerDCOffset(tHandle[i], gr, params->v1742_FastTriggerOffset[i]);
      ret |= CAEN_DGTZ_SetTriggerPolarity(tHandle[i],gr,params->v1742_TriggerEdge);
    }
    ret |= CAEN_DGTZ_SetFastTriggerMode(tHandle[i],CAEN_DGTZ_TRGMODE_ACQ_ONLY);
    ret |= CAEN_DGTZ_SetFastTriggerDigitizing(tHandle[i],CAEN_DGTZ_ENABLE);
    for(ch = 0 ; ch < 32 ; ch++)
    {
      ret |= CAEN_DGTZ_SetChannelDCOffset(tHandle[i], ch, params->v1742_DCoffset[i]);
    }
    
    //     printf("%d\n",params->registerWritesCounter );
    int r;
    
    for(r = 0 ; r < params->registerWritesCounter ; r++) {
      if(params->RegisterWriteBoard[r] == i)
      {
        ret |= CAEN_DGTZ_WriteRegister(tHandle[i],params->RegisterWriteAddr[r],params->RegisterWriteValue[r]);
        // 	printf("%d %d %llx %llx %d \n",i,params->RegisterWriteBoard[r],params->RegisterWriteAddr[r],params->RegisterWriteValue[r],ret);
      }
    }
    
  }
  
  
  return ret;
}


/* ============================================================================== */
int SetSyncMode(int handle, int timing, uint32_t runDelay)
{
  unsigned int i, ret=0;
  uint32_t reg,d32;
  if (timing){  // Run starts with S-IN on the 2nd board and all board afterwards
    ret = CAEN_DGTZ_WriteRegister(handle, ADDR_ACQUISITION_MODE, RUN_START_ON_SIN_LEVEL);
  } 
  //writes 1101 (i.e. D) on register 0x8100, for ALL boards excepts MASTER (here master is v1740d)
  // [1:0] -> 01 which means acq is S-IN CONTROLLED
  // [2]   -> 1  which means acq is armed and starts when S-IN comes
  // [3]   -> 1  accepts triggers from combination of channels, in addition to trg-in and sw trg - //FIXME what does this mean?
  ret = CAEN_DGTZ_WriteRegister(handle, ADDR_GLOBAL_TRG_MASK, (1<<30/*|1<<Params.RefChannel[i])*/));  // this set triggers to external trigger -> [30] = 1
  ret = CAEN_DGTZ_WriteRegister(handle, ADDR_TRG_OUT_MASK, 0);   // no tigger propagation to TRGOUT
  //   globalret |= ret; 
  printf("Clock n for sync = %d \n",runDelay);
  ret = CAEN_DGTZ_WriteRegister(handle, ADDR_RUN_DELAY, runDelay);   // Run Delay decreases with the position (to compensate for run the propagation delay) //
  // Set TRGOUT=RUN to propagate run through S-IN => TRGOUT daisy chain
  ret = CAEN_DGTZ_ReadRegister(handle, ADDR_FRONT_PANEL_IO_SET, &reg);
  reg = reg & 0xFFF0FFFF | 0x00010000; 
  if(handle < 2)
    ret = CAEN_DGTZ_WriteRegister(handle, ADDR_FRONT_PANEL_IO_SET, reg);
  return ret;
}


int ForceClockSync(int handle,int tHandle[],int size)
{
  int ret;
  Sleep(500);
  /* Force clock phase alignment */
  ret = CAEN_DGTZ_WriteRegister(handle, ADDR_FORCE_SYNC, 1);
  unsigned int i;
  for(i=0;i< size;i++)
  {
//     ret = CAEN_DGTZ_WriteRegister(tHandle[i], ADDR_FORCE_SYNC, 1);
  }
  /* Wait an appropriate time before proceeding */
  Sleep(1000);
  return ret;
}


int configure_digitizer(int handle, int EquippedGroups, BoardParameters *params) {
  
  int ret = 0;
  int i;
  uint32_t DppCtrl1;
  uint32_t GroupMask = 0;
  
  /* Reset Digitizer */
  ret |= CAEN_DGTZ_Reset(handle);                                                  
  
  for(i=0; i<EquippedGroups; i++) {
    uint8_t mask = (params->ChannelTriggerMask>>(i*8)) & 0xFF;
    ret |= _CAEN_DGTZ_SetChannelGroupMask(handle, i, mask); 
    if (mask)
      GroupMask |= (1<<i);
  }
  
  
  ret |= CAEN_DGTZ_SetGroupEnableMask(handle, GroupMask);
  
  
  /* 
   * * Set selfTrigger threshold 
   ** Check if the module has 64 (VME) or 32 channels available (Desktop NIM) 
   */
  if ((gBoardInfo.FormFactor == 0) || (gBoardInfo.FormFactor == 1))
    for(i=0; i<64; i++) 
      ret |= _CAEN_DGTZ_SetChannelTriggerThreshold(handle,i,params->TriggerThreshold[i]);
    else
      for(i=0; i<32; i++) 
        ret |= _CAEN_DGTZ_SetChannelTriggerThreshold(handle,i,params->TriggerThreshold[i]);     
      
      /* Disable Group self trigger for the acquisition (mask = 0) */
      ret |= CAEN_DGTZ_SetGroupSelfTrigger(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY,0x00);    
    /* Set the behaviour when a SW tirgger arrives */
    ret |= CAEN_DGTZ_SetSWTriggerMode(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY);     
  /* Set the max number of events/aggregates to transfer in a sigle readout */
  ret |= CAEN_DGTZ_SetMaxNumAggregatesBLT(handle, MAX_AGGR_NUM_PER_BLOCK_TRANSFER);                    
  /* Set the start/stop acquisition control */
  ret |= CAEN_DGTZ_SetAcquisitionMode(handle,CAEN_DGTZ_SW_CONTROLLED);             
  
  /* Trigger Hold Off */
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x8074, params->TrgHoldOff);
  
  /* DPP Control 1 register */
  DppCtrl1 = (((params->DisTrigHist & 0x1)      << 30)   | 
  ((params->DisSelfTrigger & 1)     << 24)   |
  ((params->BaselineMode & 0x7)     << 20)   |
  ((params->TrgMode & 3)            << 18)   |
  ((params->ChargeSensitivity & 0x7)<<  0)   | 
  ((params->PulsePol & 1)           << 16)   | 
  ((params->EnChargePed & 1)        <<  8)   | 
  ((params->TestPulsesRate & 3)     <<  5)   |
  ((params->EnTestPulses & 1)       <<  4)   |
  ((params->TrgSmoothing & 7)       << 12));
  
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x8040, DppCtrl1);
  
  /* Set Pre Trigger (in samples) */
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x803C, params->PreTrigger);  
  
  /* Set Gate Offset (in samples) */
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x8034, params->PreGate);     
  
  /* Set Baseline (used in fixed baseline mode only) */
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x8038, params->FixedBaseline);        
  
  /* Set Gate Width (in samples) */
  for(i=0; i<EquippedGroups; i++) {
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x1030 + 0x100*i, params->GateWidth[i]);                
  }
  
  /* Set the waveform lenght (in samples) */
  ret |= _CAEN_DGTZ_SetRecordLength(handle,params->RecordLength);           
  
  /* Set DC offset */
  for (i=0; i<EquippedGroups; i++)
    ret |= CAEN_DGTZ_SetGroupDCOffset(handle, i, params->DCoffset[i]);
  
  /* enable Charge mode */
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00080000); 
  /* enable Timestamp */
  ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00040000);             
  
  /* set Scope mode */
  if (params->AcqMode == ACQMODE_MIXED)
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00010000);         
  else
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8008, 0x00010000);             
  
  /* Set number of events per memory buffer */
  _CAEN_DGTZ_DPP_QDC_SetNumEvAggregate(handle, params->NevAggr);        
  
  /* enable test pulses on TRGOUT/GPO */
  if (ENABLE_TEST_PULSE) {
    uint32_t d32;
    ret |= CAEN_DGTZ_ReadRegister(handle, 0x811C, &d32);  
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x811C, d32 | (1<<15));         
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8168, 2);         
  }
  
  /* Set Extended Time Stamp if enabled*/ 
  if (params->EnableExtendedTimeStamp)
    ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 1 << 17); 
  
  /* Check errors */
  if(ret != CAEN_DGTZ_Success) {
    printf("Errors during Digitizer Configuration.\n");
    return -1;
  }
  
  return 0;
}

/*  
 * * Read data from board and decode events.
 ** The function has a number of side effects since it relies
 ** on global variables.
 ** Actions performed by this function are:
 **  - send a software trigger (if enabled)
 **  - read available data from board
 **  - get events fromm acquisition buffer
 **  - updates energy histogram
 **  - plot histogram every second
 **  - saves list files (if enabled)
 **  - gets waveforms from events and plots them
 **  - saves data to disk
 **
 ** If software triggers are enabled, it generates a trigger prior to
 ** reading board data.
 */  

// int calibrateDigitizerOffset(){
//   int ret;
//   unsigned int i,step;
//   uint32_t     bsize;
//   
//   // basically, empty the current buffer
//   /* Read a block of data from the 740 digitizer */
//   ret = CAEN_DGTZ_ReadData(gHandle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, gAcqBuffer, &bsize); /* Read the buffer from the digitizer */
//   /* Same for 742 digitizers*/
//   for(i = 0 ; i < gParams.NumOfV1742 ; i++) {
//     ret |= CAEN_DGTZ_ReadData(tHandle[i], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer742[i], &buffer742Size[i]);
//   }
//   
//   for(step = 0 ; step < 1000 ; step++)
//   {
//     //send software triggers to all digitizers
//     CAEN_DGTZ_SendSWtrigger(gHandle); 
//     for(i = 0 ; i < gParams.NumOfV1742 ; i++) {
//       CAEN_DGTZ_SendSWtrigger(tHandle[i]); 
//     }
//     
//     /* Read a block of data from the 740 digitizer */
//     ret = CAEN_DGTZ_ReadData(gHandle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, gAcqBuffer, &bsize); /* Read the buffer from the digitizer */
//     /* Same for 742 digitizers*/
//     for(i = 0 ; i < gParams.NumOfV1742 ; i++) {
//       ret |= CAEN_DGTZ_ReadData(tHandle[i], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer742[i], &buffer742Size[i]);
//     }
//     
//     
//     
//     
//     
//   }
//   
//   
//   
//   return 0;
// }




int run_acquisition() {
  int ret;
  unsigned int i;
  unsigned int j;
  uint32_t     bin;
  uint32_t     bsize;
  uint32_t     NumEvents740[MAX_CHANNELS];
  uint32_t     *NumEvents742;
  
  CAEN_DGTZ_EventInfo_t *EventInfo;
  
  char **EventPtr;
//   float *Wave[2];
//   float *Trigger[2];
//   uint32_t EIndx[2]={0,0};
  
  NumEvents742 = (uint32_t*) calloc (gParams.NumOfV1742,sizeof(uint32_t));
  EventInfo    = (CAEN_DGTZ_EventInfo_t*) calloc (gParams.NumOfV1742,sizeof(CAEN_DGTZ_EventInfo_t));
  EventPtr     = (char**) calloc (gParams.NumOfV1742,sizeof(char*));
  
  memset(NumEvents740, 0, MAX_CHANNELS*sizeof(NumEvents740[0])); //set all NumEvents740 to 0
//   memset(NumEvents742, 0, gParams.NumOfV1742*sizeof(NumEvents742[0])); //set all NumEvents740 to 0
  
  /* Send a SW Trigger if requested by user */
  if (gSWTrigger) {
    if (gSWTrigger == 1)    gSWTrigger = 0;
    CAEN_DGTZ_SendSWtrigger(gHandle); 
  }
  
  /*----------------------------------------*/
  /*       READ DATA FROM ACQ BUFFERS       */
  /*----------------------------------------*/
  
  /* Read a block of data from the 740 digitizer */
  ret = CAEN_DGTZ_ReadData(gHandle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, gAcqBuffer, &bsize); /* Read the buffer from the digitizer */
  /* Same for 742 digitizers*/
  for(i = 0 ; i < gParams.NumOfV1742 ; i++) {
    ret |= CAEN_DGTZ_ReadData(tHandle[i], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer742[i], &buffer742Size[i]);
  }
  gLoops += 1;
  if (ret) {
    printf("Readout Error\n");
    return -1;        
  }   
  
  
  /*----------------------------------------*/
  /*       ANALYZE DATA                     */
  /*----------------------------------------*/
  
  
  
  // analyze the 740 data
  if (bsize != 0)          // attempt analysis only if there was data from card
  {
    ret |= CAEN_DGTZ_GetNumEvents(gHandle, gAcqBuffer, bsize, &NumEvents740);
    /* Decode and analyze 740 Events */
    if ( (ret = _CAEN_DGTZ_GetDPPEvents(gHandle, gAcqBuffer, bsize, (void **)gEvent, NumEvents740)) != CAEN_DGTZ_Success)
    {
      printf("Errro during _CAEN_DGTZ_GetDPPEvents() call (ret = %d): exiting ....\n", ret);
      exit(-1);
    }
   
    /* calculate trigger time tag taking into account roll over */
    unsigned int i,j;
    for(j=0;j<NumEvents740[0];j++) //time tags are the same for all channels because we trigger on a common external signal
    {
      if (gEvent[0][j].TimeTag < temp_gPrevTimeTag)    
        temp_gETT++;
      temp_gExtendedTimeTag = ( (temp_gETT << 32) + (uint64_t)(gEvent[0][j].TimeTag));
      realTime740 = temp_gExtendedTimeTag * 16.0;
      temp_gPrevTimeTag = gEvent[0][j].TimeTag;
      
      if(WRITE_DATA)
      {
        if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
          Data740.TTT = realTime740;
        
        if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
          fprintf(sStamps740,"%f ", realTime740);
        for (i=0; i < gEquippedChannels; ++i) 
        { 
          if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
            fprintf(sStamps740,"%u ",(gEvent[i][j].Charge & 0xFFFF));
          if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
            Data740.Charge[i] = (gEvent[i][j].Charge & 0xFFFF);
        }
        if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
          fprintf(sStamps740,"\n");
        if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
          fwrite(&Data740,sizeof(Data740_t),1,binOut740);
      }
    }
  }
  
    
  
  //analyze V1742 data 
  for(i = 0 ; i < gParams.NumOfV1742 ; i++) 
  {  
    int RefChannel =16; 
    ret |= CAEN_DGTZ_GetNumEvents(tHandle[i], buffer742[i], buffer742Size[i], &NumEvents742[i]);
    if (ret) {
      printf("Readout Error\n");
      return -1;
    }
    
    unsigned int j;
    Tt = 1000.0/(2.0*58.59375);
    if(NumEvents742[i])
    {
      for(j=0 ; j<NumEvents742[i] ; j++)
      {
        ret = CAEN_DGTZ_GetEventInfo(tHandle[i], buffer742[i], buffer742Size[i], j, &EventInfo[i], &EventPtr[i]);
        ret = CAEN_DGTZ_DecodeEvent(tHandle[i], EventPtr[i], (void**)&Event742[i]);
        
//         //check all time tags of each v1742 digitizer
        unsigned int gr;
        
        //save all time tags of each v1742 digitizer
        for(gr = 0 ; gr < 4 ; gr++)
        {
          if (Event742[i]->GrPresent[gr])
          {
            // TTT Calculation
            TTT[i][gr] = ((Nroll[i][gr]<<30) + (Event742[i]->DataGroup[gr].TriggerTimeTag & 0x3FFFFFFF))*Tt;
            if (TTT[i][gr] < PrevTTT[i][gr]) 
            {
              Nroll[i][gr]++;
              TTT[i][gr] += (1<<30)*Tt;
            }
            PrevTTT[i][gr] = TTT[i][gr];
            
            if(WRITE_DATA)
            {
              if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
                Data742[i].TTT[gr] = TTT[i][gr];
              if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
                fprintf(sStamps742[i],"%f ", TTT[i][gr]);
            }
            /* Correct event of both boards */
            
//             ApplyDataCorrection(gr, 7,gParams.v1742_DRS4Frequency, &(Event742[i]->DataGroup[gr]), &Table[i]);
//             
            float *Wave;
            
            //interpolate the trigger wave
            uint32_t nSamples = Event742[i]->DataGroup[gr].ChSize[8];
            Wave = Event742[i]->DataGroup[gr].DataChannel[8]; //see ./home/caenvme/Programs/CAEN/CAENDigitizer_2.7.1/include
            
            double TriggerEdgeTime = interpolateWithMultiplePoints(Wave, nSamples, gParams.v1742_TRThreshold[i], gParams.v1742_TriggerEdge, 0,1); // interpolate with line, Type = 1 means trigger wave 
            
            
            unsigned int ch;
            for(ch = 0 ; ch < 8 ; ch ++)
            {
              //interpolate the wave
              uint32_t nSamples = Event742[i]->DataGroup[gr].ChSize[ch];
              Wave = Event742[i]->DataGroup[gr].DataChannel[ch];
              double PulseEdgeTime = interpolateWithMultiplePoints(Wave, nSamples, gParams.v1742_ChannelThreshold[i], gParams.v1742_ChannelPulseEdge[i], 0,0); // interpolate with line, Type = 0 means pulse wave 
              
              if(PulseEdgeTime >= 0)
              {
                PulseEdgeTime = TriggerEdgeTime - PulseEdgeTime;
                if(WRITE_DATA)
                {
                  if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
                    Data742[i].PulseEdgeTime[gr * 8 + ch] = PulseEdgeTime;
                  if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
                    fprintf(sStamps742[i],"%f ",PulseEdgeTime);             
                }
              }
              else
              {
                if(WRITE_DATA)
                {
                  if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
                    Data742[i].PulseEdgeTime[gr * 8 + ch] = 0;
                  if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
                    fprintf(sStamps742[i],"%f ",0);             
                }
              }
              
              
            }           
          }
          else
          {
            if(WRITE_DATA)
            {
              unsigned int ch;
              for(ch = 0 ; ch < 9 ; ch ++) // to 9 because the TRn is also included
              {
                if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
                  Data742[i].PulseEdgeTime[gr * 8 + ch] = 0;
              }
              if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
                fprintf(sStamps742[i],"0 0 0 0 0 0 0 0 0 "); // for groups with no data
            }
          }          
        }
        if(WRITE_DATA)
        {
          if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
            fprintf(sStamps742[i],"\n"); 
          if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
            fwrite(&Data742[i],sizeof(Data742_t),1,binOut742[i]);
        }
      }
      
      
      
      
      
      
//       outputData.TTT[i] = TTT[i];
//       
//       //    fprintf(timeTags,"%f ",TTT[i]);
//       
//       //     printf("fine\n");
//       //    int ind;
//       //    for(ind = 0 ; ind < 1024 ; ind++){
//       outputData.Wave[i] = Event742[i]->DataGroup[RefChannel/8].DataChannel[RefChannel%8];
//       outputData.Trigger[i] = Event742[i]->DataGroup[RefChannel/8].DataChannel[8];
//       //    }
//       outputData.PulseEdgeTime[i] = -1; //set to -1 by default
//       outputData.TrEdgeTime[i] = -1;    //set to -1 by default
    }
    
    

    
//   for(i=0; i<2; i++) 
//   { // get the wave regardless of any time stamp consideration
//     ApplyDataCorrection((Params.RefChannel[i]/8), 7,Params.DRS4Frequency, &(Event742[i]->DataGroup[Params.RefChannel[i]/8]), &Table[i]); //correct data every time. this was moved here for debugging purpose, might slow down everything 
    
    
//   }


	
	
    
    
    
//     tPrevWPlotTime = gCurrTime;
  }
  
//   if(NumEvents740 && NumEvents742[0] && NumEvents742[1])
//   {
//     //clear_screen();
//     //printf("TTT 740   %f\n",realTime740);  
//     //printf("TTT 742_1 %f\n",TTT[0]);
//     //printf("TTT 742_2 %f\n",TTT[1]);
//     fprintf(tttFile,"%lf %lf %lf %lf %lf\n",realTime740,TTT[0],TTT[1],(TTT[0]-realTime740)/*/1000000*/,(TTT[1]-TTT[0])/*/1000000*/);
// //     if (((TTT[0]-realTime740)/1000000)>100) printf("TTT 740   %f\n",(TTT[0]-realTime740)/1000000);
// //     event_file = fopen("event.txt", "w");
// //     for(i=0; i < 1024; i++) {
// //       fprintf(event_file, "%f\t%f\t%f\t%f\n", outputData.Wave[0][i],outputData.Trigger[0][i], outputData.Wave[1][i], outputData.Trigger[1][i]);
// //     }
// //     int RefChannel =16; 
// //     fclose(event_file);
// //     fprintf(plotter, "set xlabel 'Samples' \n");
// //     fprintf(plotter, "plot 'event.txt' u 0:1 title 'Ch%d_Board0' w step,'event.txt' u 0:2 title 'TR%d_Board0' w step,'event.txt' u 0:3  title 'Ch%d_Board1' w step, 'event.txt' u 0:4 title 'TR%d_Board1' w step\n",
// // 	    RefChannel,(RefChannel/16),RefChannel, (RefChannel/16));
// //     fflush(plotter);
//   }
//   else
//   {
//     return 0; 
//   }
  
  
  
//   while(1)
//   {
//     for(i = 0 ; i < gParams.NumOfV1742 ; i++) {
//       //////modified apolesel///////////////////////////////////////
//       if (GetNextEvent[i]) {
// 	if (EIndx[i] >= NumEvents742[i]) {
// 	  EIndx[i] = 0;
// 	  ret = CAEN_DGTZ_ReadData(tHandle[i], CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer742[i], &buffer742Size[i]);
// 	  
// 	  if (buffer742Size[i]>0)
// 	    ret=ret;
// 	  //     printf("buffersize %d\n",buffer742Size[i]);
// 	    ret |= CAEN_DGTZ_GetNumEvents(tHandle[i], buffer742[i], buffer742Size[i], &NumEvents742[i]);
// 	    if (ret) {
// 	      printf("Readout Error\n");
// 	      return -1;
// 	    }
// 	}
// 	//     printf("NumEvents742[%d] = %d\n",i,NumEvents742[i]);
// 	// //     printf("NumEvents[1] = %d\n",NumEvents[1]);
// 	
// 	
// 	if (NumEvents742[i]) {
// 	  //printf("INSIDE NumEvents742[%d] = %d\n",i,NumEvents742[i]);
// 	  // 	      printf("EIndx[%d] = %d\n",i,EIndx[i]);
// 	  
// 	  //test 
// 	  //EIndx[i] = 0; //FIXME ???
// 	  
// 	  ret = CAEN_DGTZ_GetEventInfo(tHandle[i], buffer742[i], buffer742Size[i], EIndx[i], &EventInfo[i], &EventPtr[i]);
// 	  ret = CAEN_DGTZ_DecodeEvent(tHandle[i], EventPtr[i], (void**)&Event742[i]);
// 	  // //       TrgCnt[i]++;
// 	  if (ret) {
// 	    printf("Event build error\n");
// 	    // 	goto QuitProgram;
// 	    return -1;
// 	  }
// 	  GetNextEvent[i]=0;
// 	}
//       }
//       ////////////////////////////////////////////////////
//       
//     }
//     if (GetNextEvent[0] || GetNextEvent[1])  // missing data from one or both boards
//             continue;
//     
//     //TEMP REF CHANNEL
//     int RefChannel = 16; 
//     //   double TTT[2];
//     Tt = 8.533333; // step per il Local_TTT
//     Ts = 0.2;
// 
//     // ----------------------------------------------------------------
//     // Analyze data
//     // ----------------------------------------------------------------
//     // calculate extended Trigger Time Tag (take roll over into account)
//     for(i=0; i<2; i++) 
//     {
//       if (Event742[i]->GrPresent[(RefChannel/8)])
//       {
//         TTT[i] = ((Nroll[i]<<30) + (Event742[i]->DataGroup[(RefChannel/8)].TriggerTimeTag & 0x3FFFFFFF))*Tt;
//         if (TTT[i] < PrevTTT[i]) 
// 	{
//           Nroll[i]++;
//           TTT[i] += (1<<30)*Tt;
//         }
//         PrevTTT[i] = TTT[i];
//       }
//       else
//       {
//     //       EvNoData[i]++;
//       }
//     }
//     ///////////////////modified apolesel//////////////////////////////////////////
//     
//     // use only events whose time stamp differ of less than the matching window:
//     // CASE1: board 0 is behind board 1; keep event from 1 and take next event from 0
//     if (TTT[0] < (TTT[1] - gParams.v1742_MatchingWindow*Tt)) 
//     {
//       EIndx[0]++; 
//       // 	    printf("board 0 is behind board 1\n");
//       GetNextEvent[0]=1;
//       continue;
//       // CASE2: board 1 is behind board 0; keep event from 0 and take next event from 1
//     } 
//     else if (TTT[1] < (TTT[0] - gParams.v1742_MatchingWindow*Tt)) 
//     {
//       EIndx[1]++; 
//       GetNextEvent[1]=1;
//       // 	    printf("board 1 is behind board 0\n");
//       continue;
//       // CASE3: trigger time tags match: calculate DeltaT between edges
//     } 
//     else 
//     {  
//       // 	  printf("Accepted\n");
//       MatchingEvents++;
//       
//       printf("Delta TTT = %f \n",TTT[1]-TTT[0]);
//       
// //       for(i=0; i<2; i++) 
// //       {
// // 	EIndx[i]++;
// // 	GetNextEvent[i]=1;  
// //       }
//       break; //FIXME correct?
//     }
//     
//     ////////////////////////////////////////////////////////////////////////////
//     
//     
//   }
  


  
  
  
  
  
//   timeTags = fopen("timeTags.txt", "a");
  //printf("%f %f\n",TTT[0],TTT[1]);
  
  
  
  
  
  
  
 
  
  
  
  
//   Plot Waveforms
  int plotWaveforms = 0; //for now set here //FIXME
  
  
  
  if (plotWaveforms && ((gCurrTime-tPrevWPlotTime) > 2000) )   {
    
    OutputData_t outputData;
    int RefChannel =16; 
    for(i=0; i<2; i++) 
    { // get the wave regardless of any time stamp consideration
      
      //     ApplyDataCorrection(RefChannel, 7,gParams.v1742_DRS4Frequency, &(Event742[i]->DataGroup[Params.RefChannel[i]/8]), &Table[i]); //correct data every time. this was moved here for debugging purpose, might slow down everything 
      
      outputData.TTT[i] = TTT[i][2];
      
      //     fprintf(timeTags,"%f ",TTT[i]);
      
      //     printf("fine\n");
      //    int ind;
      //    for(ind = 0 ; ind < 1024 ; ind++){
      outputData.Wave[i] = Event742[i]->DataGroup[RefChannel/8].DataChannel[RefChannel%8];
      outputData.Trigger[i] = Event742[i]->DataGroup[RefChannel/8].DataChannel[8];
      //    }
      outputData.PulseEdgeTime[i] = -1; //set to -1 by default
      outputData.TrEdgeTime[i] = -1;    //set to -1 by default
    }
    
    //   if(1){
    //     	  if (!ContinousWaveplot) {
    //     	    plot = 0;
    //     	  }
    
    event_file = fopen("event.txt", "w");
    for(i=0; i < 1024; i++) {
      fprintf(event_file, "%f\t%f\t%f\t%f\n", outputData.Wave[0][i],outputData.Trigger[0][i], outputData.Wave[1][i], outputData.Trigger[1][i]);
    }
//     int RefChannel =16; 
    fclose(event_file);
    fprintf(plotter, "set xlabel 'Samples' \n");
    fprintf(plotter, "plot 'event.txt' u 0:1 title 'Ch%d_Board0' w step,'event.txt' u 0:2 title 'TR%d_Board0' w step,'event.txt' u 0:3  title 'Ch%d_Board1' w step, 'event.txt' u 0:4 title 'TR%d_Board1' w step\n",
            RefChannel,(RefChannel/16),RefChannel, (RefChannel/16));
    fflush(plotter);
    tPrevWPlotTime = gCurrTime;
    //     sleep(1);
  }
  
  
  
  
  
  /* Loop over all channels and events */
//   for (i=0; i < gEquippedChannels; ++i) {              
//     for(j=0; j<NumEvents[i]; ++j) {
//       
//       
//       
//       uint32_t Charge;
//       
//       Charge = (gEvent[i][j].Charge & 0xFFFF);  /* rebin charge to 4Kchannels */
//       
//       
//       /* Update energy histogram */
//       if ((Charge < HISTO_NBIN) && (Charge >= CHARGE_LLD_CUT) && (Charge <= CHARGE_ULD_CUT) && (gEvent[i][j].Overrange == 0))
//         gHisto[i][Charge]++;
//       
//       /* Plot Histogram */
//       if ((gCurrTime-gPrevHPlotTime) > 2000) {
//         gPlotDataFile = fopen("PlotData.txt", "w");
//         for(bin=0; bin<HISTO_NBIN; bin++)
//           fprintf(gPlotDataFile, "%d\n", gHisto[gActiveChannel][bin]);
//         fclose(gPlotDataFile);
//         fprintf(gHistPlotFile, "plot 'PlotData.txt' with step\n");
//         fflush(gHistPlotFile);
//         gPrevHPlotTime = gCurrTime;
//       }
//       
//       /* Check roll over of time tag */
//       if (gEvent[i][j].TimeTag < gPrevTimeTag[i])    
//         gETT[i]++;
//       
//       gExtendedTimeTag[i] = (gETT[i] << 32) + (uint64_t)(gEvent[i][j].TimeTag);
//       
//       
//       /* Save event to output file */
//       if (gParams.SaveList) { 
//         fprintf(gListFiles[i], "%16llu %8d\n", gExtendedTimeTag[i], gEvent[i][j].Charge);
//       }
//       
//       if (i==gActiveChannel)
//       {
//         fprintf(timeTags,"%16llu %f %16llu\n",gExtendedTimeTag[i],TTT[1]-TTT[0],gExtendedTimeTag[i]-TTT[0]);
// 	//printf("%f %f %16llu %f %16llu\n",TTT[0],TTT[1],gExtendedTimeTag[i],TTT[1]-TTT[0],gExtendedTimeTag[i]-TTT[0]);  
// 	
//       }
//       
//       /* Plot Waveforms (if enabled) */
//       if ((i==gActiveChannel) && (gParams.AcqMode == ACQMODE_MIXED) && ((gCurrTime-gPrevWPlotTime) > 2000) && (Charge >= CHARGE_LLD_CUT) && (Charge <= CHARGE_ULD_CUT)) {
//         _CAEN_DGTZ_DecodeDPPWaveforms(&gEvent[i][j], gWaveforms);
//         gPlotDataFile = fopen("PlotWave.txt", "w");
//         for(j=0; j<gWaveforms->Ns; j++) {
//           fprintf(gPlotDataFile, "%d ", gWaveforms->Trace1[j]);                 /* samples */
//           fprintf(gPlotDataFile, "%d ", 2000 + 200 *  gWaveforms->DTrace1[j]);  /* gate    */
//           fprintf(gPlotDataFile, "%d ", 1000 + 200 *  gWaveforms->DTrace2[j]);  /* trigger */
//           fprintf(gPlotDataFile, "%d ", 500 + 200 *  gWaveforms->DTrace3[j]);   /* trg hold off */
//           fprintf(gPlotDataFile, "%d\n", 100 + 200 *  gWaveforms->DTrace4[j]);  /* overthreshold */
//         }
//         fclose(gPlotDataFile);
//         
//         
//         
//         
//         switch (gAnalogTrace) {
//           case 0:
//             fprintf(gWavePlotFile, "plot 'PlotWave.txt' u 1 t 'Input' w step, 'PlotWave.txt' u 2 t 'Gate' w step, 'PlotWave.txt' u 3 t 'Trigger' w step, 'PlotWave.txt' u 4 t 'TrgHoldOff' w step, 'PlotWave.txt' u 5 t 'OverThr' w step\n");
//             break;
//           case 1:
//             fprintf(gWavePlotFile, "plot 'PlotWave.txt' u 1 t 'Smooth' w step, 'PlotWave.txt' u 2 t 'Gate' w step, 'PlotWave.txt' u 3 t 'Trigger' w step, 'PlotWave.txt' u 4 t 'TrgHoldOff' w step, 'PlotWave.txt' u 5 t 'OverThr' w step\n");
//             break;
//           case 2:
//             fprintf(gWavePlotFile, "plot 'PlotWave.txt' u 1 t 'Baseline' w step, 'PlotWave.txt' u 2 t 'Gate' w step, 'PlotWave.txt' u 3 t 'Trigger' w step, 'PlotWave.txt' u 4 t 'TrgHoldOff' w step, 'PlotWave.txt' u 5 t 'OverThr' w step\n");
//             break;
//           default:
//             fprintf(gWavePlotFile, "plot 'PlotWave.txt' u 1 t 'Input' w step, 'PlotWave.txt' u 2 t 'Gate' w step, 'PlotWave.txt' u 3 t 'Trigger' w step, 'PlotWave.txt' u 4 t 'TrgHoldOff' w step, 'PlotWave.txt' u 5 t 'OverThr' w step\n");
//             break;
//         }
//         
//         fflush(gWavePlotFile);
//         gPrevWPlotTime = gCurrTime;
//       }
//       
//       
//       gPrevTimeTag[i]   = gEvent[i][j].TimeTag;
//     }
//     
//     
//     gEvCnt[i]        += NumEvents[i];
//     if (gToggleTrace) {
//       
//       switch (gAnalogTrace) {
//         case 0:
//           CAEN_DGTZ_WriteRegister(gHandle, 0x8008, 3<<12);
//           break;
//         case 1:
//           CAEN_DGTZ_WriteRegister(gHandle, 0x8004, 1<<12);
//           CAEN_DGTZ_WriteRegister(gHandle, 0x8008, 1<<13);
//           break;
//         case 2:
//           CAEN_DGTZ_WriteRegister(gHandle, 0x8008, 1<<12);
//           CAEN_DGTZ_WriteRegister(gHandle, 0x8004, 1<<13);
//           break;
//         default:
//           CAEN_DGTZ_WriteRegister(gHandle, 0x8004, 3<<12);
//           break;
//       }
//       
//       
//       gToggleTrace = 0;
//     }
//     
//   }
//   /* END OF Loop over all channels and events */
  
  
  gAcqStats.nb += bsize;
//   fclose(timeTags);
  return 0;
  
}


/*  
 * * Prints statistics to console.
 */  
void print_statistics() {
  long elapsed;
  unsigned int i;
  uint64_t     PrevEvCnt   = 0;
  
  gCurrTime = get_time();
  elapsed = (gCurrTime-gPrevTime);
  gAcqStats.gRunElapsedTime = gCurrTime - gRunStartTime;
  if (elapsed>1000) {
    uint64_t diff;
    gAcqStats.TotEvCnt = 0;
    clear_screen();
    
    for(i=0; i < gEquippedChannels; ++i) {
      gAcqStats.TotEvCnt += gEvCnt[i];             
    }
    
    printf("*********** Global statistics ***************\n");
    printf("=============================================\n\n");
    printf("Elapsed time         = %d ms\n", gAcqStats.gRunElapsedTime);
    printf("Readout Loops        = %d\n", gLoops );
    printf("Bytes read           = %d\n", gAcqStats.nb);
    printf("Events               = %lld\n", gAcqStats.TotEvCnt);
    printf("Readout Rate         = %.2f MB/s\n\n", (float)gAcqStats.nb / 1024 / (elapsed));
    
    
    printf("******** Group  %d statistics **************\n\n", grp4stats);
    printf("-- Channel -- Event Rate (KHz) -----\n\n");
    
    for(i=0; i < 8; ++i) {
      diff = gEvCnt[grp4stats*8+i]-gEvCntOld[grp4stats*8+i];
      printf("    %2d      %.2f\n", grp4stats*8+i, (float)(diff) / (elapsed));
      gEvCntOld[grp4stats*8+i] = gEvCnt[grp4stats*8+i];
    }
    
    gAcqStats.nb        = 0;
    gPrevTime  = gCurrTime;
    PrevEvCnt = gAcqStats.TotEvCnt;
    
  }
  
}


/*  
 * * Cleanup heap memory
 ** Returns 0 on success; -1 in case of any error detected.
 */  
int cleanup_on_exit() {
  
  int ret;
  int i;
  
  /* Free the buffers and close the digitizer */
  CAEN_DGTZ_SWStopAcquisition(gHandle);
  CAEN_DGTZ_SWStopAcquisition(1);
  CAEN_DGTZ_SWStopAcquisition(2);
  ret = _CAEN_DGTZ_FreeReadoutBuffer(&gAcqBuffer);
  
  if (gHistPlotFile != NULL)  pclose(gHistPlotFile); 
  if (gWavePlotFile != NULL)  pclose(gWavePlotFile); 
  
  if(gWaveforms) free(gWaveforms);
  
//   for(i=0; i<MAX_CHANNELS; ++i) {
//     if (gListFiles[i] != NULL)
//       fclose(gListFiles[i]);
//   }
  
  for(i=0; i<MAX_CHANNELS; ++i) {
    if (gEvent[i] != NULL)
      free(gEvent[i]);
  }
  
  /* deallocate memory for group events */
  for(i=0; i<8; i++) {  
    if (gEventsGrp[i] != NULL)
      free(gEventsGrp[i]);
  }
  
  for(i=0; i<gParams.NumOfV1742; i++) {  
      free(TTT[i]);
      free(PrevTTT[i]);
      free(Nroll[i]);
  }
  free(TTT);
  free(PrevTTT);
  free(Nroll);
  
  ret = CAEN_DGTZ_CloseDigitizer(gHandle);
  
  for(i=0; i<gParams.NumOfV1742; i++) {
    ret = CAEN_DGTZ_CloseDigitizer(tHandle[i]);
  }
  
//   fclose(tttFile);
  
//   for(i=0; i<gParams.NumOfV1742; i++) {  
//     fclose(sStamps742[i]);
//   }
//   free(sStamps740);
  
  free(Data742);
  
  if(WRITE_DATA){
    if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
      fclose(sStamps740);
    if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
      fclose(binOut740);
  
    for(i=0; i<gParams.NumOfV1742; i++) {  
      if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
        fclose(sStamps742[i]);
      if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
        fclose(binOut742[i]);
    }
    if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
      free(binOut742);
    if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
      free(sStamps742);
  }
  return 0;
  
}

/*  
 * * Setup acquisition.
 **   - setup board according to configuration file
 **   - open a connection with the module
 **   - allocates memory for acquisition buffers and structures
 ** Returns 0 on success; -1 in case of any error detected.
 */  

int setup_acquisition(char *fname) {
  
  int ret;
  unsigned int i;
  uint32_t     size = 0;
  uint32_t AllocatedSize = 0;
  uint32_t rom_version;
  uint32_t rom_board_id;
  
  clear_screen();
  plotter = popen("gnuplot", "w");    // Open plotter pipe (gnuplot)
  /* ---------------------------------------------------------------------------------------
   * * Set Parameters (default values or read from config file)
   ** ---------------------------------------------------------------------------------------
   */
  ret = setup_parameters(&gParams, fname);
  if (ret < 0) {
    printf("Error in setting parameters\n");
    return -1; 
  }
  
//   tttFile      = fopen("ttt.txt","w");
  
//   sStamps740 = (FILE*) malloc (  sizeof(FILE*)) 
  char stamps742FileName[50],b742FileName[50];
  unsigned int j;
  
  if(WRITE_DATA)
  {
    if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
      sStamps742 = (FILE*) malloc ( gParams.NumOfV1742 * sizeof(FILE*));
    
    if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
      binOut742 = (FILE*) malloc ( gParams.NumOfV1742 * sizeof(FILE*));
    
    for(j=0; j<gParams.NumOfV1742;j++)
    {
      if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
      {
        sprintf(stamps742FileName,"stamps742_%d.txt",j);
        sStamps742[j] = fopen(stamps742FileName,"w");
      }
      if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
      {
        sprintf(b742FileName,"binary742_%d.dat",j);
        binOut742[j] = fopen(b742FileName,"wb");
      }
    }
    if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
      sStamps740   = fopen("stamps740.txt","w");
    
    if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
      binOut740 = fopen("binary740.dat","wb");
    
  }
  
//   if(OPTIMIZATION_RUN)
//   {
//     
//   }
  

  //   sStamps742_1 = fopen("stamps742_1.txt","w");
  
  
  
  Data742 = (Data742_t*) malloc(gParams.NumOfV1742 * sizeof(Data742_t));
  
  /* ---------------------------------------------------------------------------------------
   *    // Open the digitizer and read board information
   *    // ---------------------------------------------------------------------------------------
   */
  
  CAEN_DGTZ_CloseDigitizer(gHandle);
  
  /* Open a conection with module and gets library handle */
  if (gParams.ConnectionType == CONNECTION_TYPE_AUTO) {
    /* Try USB connection first */
    ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, gParams.ConnectionLinkNum, gParams.ConnectionConetNode, gParams.ConnectionVMEBaseAddress, &gHandle);
    if(ret != CAEN_DGTZ_Success) {
      /* .. if not successful try opticallink connection then ...*/
      ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_OpticalLink, gParams.ConnectionLinkNum, gParams.ConnectionConetNode, gParams.ConnectionVMEBaseAddress, &gHandle);
      if(ret != CAEN_DGTZ_Success) {
        printf("Can't open digitizer\n");
        return -1;
      }
    }
  }
  else {
    if (gParams.ConnectionType == CONNECTION_TYPE_USB)
      ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, gParams.ConnectionLinkNum, gParams.ConnectionConetNode, gParams.ConnectionVMEBaseAddress, &gHandle);
    if (gParams.ConnectionType == CONNECTION_TYPE_OPT)
      ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_OpticalLink, gParams.ConnectionLinkNum, gParams.ConnectionConetNode, gParams.ConnectionVMEBaseAddress, &gHandle);
    if(ret != CAEN_DGTZ_Success) {
      printf("Can't open digitizer\n");
      return -1;
    }
  }
  
  /* Check board type */    
  ret = CAEN_DGTZ_GetInfo(gHandle, &gBoardInfo);
  
  /* Patch to handle ModelName which is not decoded by CAENDigitizer library <= 2.6.7 */
  CAEN_DGTZ_ReadRegister(gHandle, 0xF030, &rom_version);
  if((rom_version & 0xFF) == 0x54) 
  {
    CAEN_DGTZ_ReadRegister(gHandle, 0xF034, &rom_board_id);
    switch (rom_board_id) 
    {
      case 0:
        sprintf(gBoardInfo.ModelName, "V1740D");
        break;
      case 1: 
        sprintf(gBoardInfo.ModelName, "VX1740D");
        break;
      case 2: 
        sprintf(gBoardInfo.ModelName,"DT5740D");
        break;
      case 3: 
        sprintf(gBoardInfo.ModelName,"N6740D");
        break;
      default: 
        break;
    }
  }
  else
  {
    printf("This software is not appropriate for module %s\n", gBoardInfo.ModelName);
    printf("Press any key to exit!\n");
    getch();
    exit(0);
  }
  
  
  printf("\nConnected to CAEN Digitizer Model %s\n", gBoardInfo.ModelName);
  printf("ROC FPGA Release is %s\n", gBoardInfo.ROC_FirmwareRel);
  printf("AMC FPGA Release is %s\n\n", gBoardInfo.AMC_FirmwareRel);
  
  gEquippedChannels = gBoardInfo.Channels * 8; 
  gEquippedGroups = gEquippedChannels/8;
  
  gActiveChannel = gParams.ActiveChannel;
  if (gActiveChannel >= gEquippedChannels)
    gActiveChannel = gEquippedChannels-1;
  
  ret = CAEN_DGTZ_SWStopAcquisition(gHandle);
  
  ret = configure_digitizer(gHandle, gEquippedGroups, &gParams);
  if (ret)  {
    printf("Error during charge digitizer configuration\n");
    return -1;
  }
  else {
    printf("\nCharge Digitizer configured\n");
  }
  
  /* ---------------------------------------------------------------------------------------
   *    // Memory allocation and Initialization of counters, histograms, etc...
   */
  memset(gExtendedTimeTag, 0, MAX_CHANNELS*sizeof(uint64_t));
  memset(gETT, 0, MAX_CHANNELS*sizeof(uint64_t));
  memset(gPrevTimeTag, 0, MAX_CHANNELS*sizeof(uint64_t));
  memset(gPrevTimeTag, 0, MAX_CHANNELS*sizeof(uint64_t));
  memset(gEvCnt, 0, MAX_CHANNELS*sizeof(gEvCnt[0]));
  memset(gEvCntOld, 0, MAX_CHANNELS*sizeof(gEvCntOld[0]));
  
  
  int iMalloc;
  TTT = (double **) calloc(gParams.NumOfV1742 , sizeof(double*));
  for(iMalloc = 0; iMalloc < gParams.NumOfV1742; iMalloc++) 
    TTT[iMalloc] = (double *) calloc (GROUPS_742 , sizeof(double));
  
  PrevTTT = (double **)calloc(gParams.NumOfV1742 , sizeof(double*));
  for(iMalloc = 0; iMalloc < gParams.NumOfV1742; iMalloc++) 
    PrevTTT[iMalloc] = (double *) calloc (GROUPS_742 , sizeof(double));
  
  Nroll = (uint64_t **)calloc(gParams.NumOfV1742 , sizeof(uint64_t*));
  for(iMalloc = 0; iMalloc < gParams.NumOfV1742; iMalloc++) 
    Nroll[iMalloc] = (uint64_t *) calloc (GROUPS_742 , sizeof(uint64_t));
  
  Table = (DataCorrection_t*) calloc (gParams.NumOfV1742,sizeof(DataCorrection_t));
  
//   memset(TTT, 0, sizeof(TTT[0][0]) * gParams.NumOfV1742 * GROUPS_742);
//   memset(PrevTTT, 0, sizeof(PrevTTT[0][0]) * gParams.NumOfV1742 * GROUPS_742);
//   memset(Nroll, 0, sizeof(Nroll[0][0]) * gParams.NumOfV1742 * GROUPS_742);
  
  /* Malloc Readout Buffer.
   *    NOTE: The mallocs must be done AFTER digitizer's configuration! */
  
  if (!gAcquisitionBufferAllocated) {
    gAcquisitionBufferAllocated = 1;
    ret = _CAEN_DGTZ_MallocReadoutBuffer(gHandle,&gAcqBuffer,&size);
    if (ret) {printf("Cannot allocate %d bytes for the acquisition buffer! Exiting...", size);exit(-1);}
    printf("Allocated %d bytes for readout buffer \n", size);
    
    /* NOTE : allocates a fixed size.
     * * Needs CAENDigitizer library implementation.
     */
    for (i= 0; i < gEquippedChannels; ++i) {
      int allocated_size = MAX_AGGR_NUM_PER_BLOCK_TRANSFER * MAX_EVENT_QUEUE_DEPTH * sizeof(_CAEN_DGTZ_DPP_QDC_Event_t);
      gEvent[i] = (_CAEN_DGTZ_DPP_QDC_Event_t *)malloc(allocated_size);
      
      if (gEvent[i] == NULL)
      {printf("Cannot allocate memory for event list for channel %d\n. Exiting....", i); exit(-1);};
    }
    
    /* allocate memory buffer for the data readout */
    ret = _CAEN_DGTZ_MallocDPPWaveforms(gHandle, &gWaveforms, &AllocatedSize);
    
    /* Allocate memory for the histograms */
    gHisto = (uint32_t **)malloc(gEquippedChannels * sizeof(uint32_t *));
    if(gHisto == NULL) {printf("Cannot allocate %d bytes for histogram! Exiting...", gEquippedChannels * sizeof(uint32_t *));exit(-1);}
    for(i= 0 ; i < gEquippedChannels; ++i) {
      gHisto[i] = (uint32_t *)malloc(HISTO_NBIN * sizeof(uint32_t));
      if(gHisto[i] == NULL) {printf("Cannot allocate %d bytes for histogram! Exiting...", HISTO_NBIN * sizeof(uint32_t));exit(-1);}
    }
    
    /* open gnuplot in a pipe and the data file*/
    gHistPlotFile = popen("gnuplot", "w");  //mod for gnuplot under linux
    if (gHistPlotFile==NULL) {
      printf("Can't open gnuplot\n");
      return -1; 
    }
    if (gParams.AcqMode == ACQMODE_MIXED) {
      gWavePlotFile = popen("gnuplot", "w");  //mod for gnuplot under linux
      fprintf(gWavePlotFile, "set yrange [0:4096]\n");
    }
    
  }
  
  
  
  for(i= 0 ; i < gEquippedChannels; ++i) {
    memset(gHisto[i], 0, HISTO_NBIN * sizeof(uint32_t));
    gEvCnt[i] = 0;
  }
  
  /* Crete output list files (one per channel) */
//   for(i=0; i < gEquippedChannels; ++i) {
//     char filename[200];
//     sprintf(filename, "list_ch%d.txt", i);
//     if (gListFiles[i] == NULL) gListFiles[i] = fopen(filename, "w");	 
//   }
  
  
  //configure timing digitizer(s)
  if(gParams.NumOfV1742) {
    int ret;
    ret = configure_timing_digitizers(&gParams);
    if (ret) {
      printf("\nError Configuring Timing Digitizer(s)\n");
      return -1;
    }
    else {
      printf("\nTiming Digitizer(s) configured\n");
    }
  }
  else {
    printf("\nNo timing digitizers in acq chain\n");
  }
  
  //configure sync run
  // for charge digi
  ret = SetSyncMode(gHandle,0,gParams.RunDelay); 
  if (ret) {
    printf("\nError Configuring Sync Mode for Charge Digitizer\n");
    return -1;
  }
  
  // for timing digi
  for(i=0 ; i < gParams.NumOfV1742 ; i++) {
    ret = SetSyncMode(tHandle[i],1,gParams.v1742_RunDelay[i]);
    if (ret) {
      printf("\nError Configuring Sync Mode for Timing Digitizer %d\n",i);
      return -1;
    }
  }
  
  /* MEMORY ALLOCATION - TIMING DIGITIZERS ONLY                                                  */
  for(i = 0; i < gParams.NumOfV1742; i++) {
    uint32_t AllocatedSize;
    /* Memory allocation for event buffer and readout buffer */
    ret = CAEN_DGTZ_AllocateEvent(tHandle[i],(void**)&Event742[i]);
    /* NOTE : This malloc must be done after the digitizer programming */
    ret |= CAEN_DGTZ_MallocReadoutBuffer(tHandle[i], &buffer742[i], &AllocatedSize);
    if (ret) {
      printf("Can't allocate memory for the acquisition\n");
      return -1;
    }
  }
  
  // Force Clock sync (should we?)
  ForceClockSync(gHandle,tHandle,gParams.NumOfV1742);
//   for(i = 0; i < gParams.NumOfV1742; i++) {
//     ForceClockSync(tHandle[i]);
//   }
  
  //CHECK REGISTERS FOR RUN PROPAGATION
//   printf("\nCHECK REGISTERS FOR RUN PROPAGATION\n");
//   uint32_t rdata;
//   ret = CAEN_DGTZ_ReadRegister(gHandle,0x811c,&rdata);
//   printf("\nMASTER register 0x811c = 0x%08x \n",rdata);
//   ret = CAEN_DGTZ_ReadRegister(tHandle[0],0x811c,&rdata);
//   printf("SLAVE1 register 0x811c = 0x%08x \n\n",rdata);
//   
//   ret = CAEN_DGTZ_ReadRegister(gHandle,0x8100,&rdata);
//   printf("MASTER register 0x8100 = 0x%08x \n\n",rdata);
//   
//   ret = CAEN_DGTZ_ReadRegister(tHandle[0],0x8100,&rdata);  // check, should be 0x5 but is 0xd
//   printf("SLAVE1 register 0x8100 = 0x%08x \n",rdata);      
//   ret = CAEN_DGTZ_ReadRegister(tHandle[1],0x8100,&rdata);  // check, should be 0x5 but is 0xd
//   printf("SLAVE2 register 0x8100 = 0x%08x \n\n",rdata);
//   
//   ret = CAEN_DGTZ_ReadRegister(tHandle[1],0x811c,&rdata);  // should be bit[20:16] = 01101 but is bit[20:16] = 00001
//   printf("SLAVE2 register 0x811c = 0x%08x \n\n",rdata);
  
  
  //   ret = CAEN_DGTZ_ReadRegister(gHandle,0x811c,&rdata);
  //   printf("SLAVE2 register 0x811c = %02x \n",rdata);
  
  
  
  printf("Boards Configured. Press [s] to start run or [c] to check clock alignment\n\n");
  char c;
  c = getch();
  if (c == 'c') {
    uint32_t rdata0,rdata1,rdata2;
    
    /*    rdata = (uint32_t *) malloc(sizeof(uint32_t) * totalNumbOfDigi);*/
    // propagate CLK to trgout on both (all?) boards
    /*    for(unsigned int i=0; i < 3; i++) {*/
    
    CAEN_DGTZ_ReadRegister(gHandle, ADDR_FRONT_PANEL_IO_SET, &rdata0);
    CAEN_DGTZ_WriteRegister(gHandle, ADDR_FRONT_PANEL_IO_SET, 0x00050000);
    CAEN_DGTZ_ReadRegister(tHandle[0], ADDR_FRONT_PANEL_IO_SET, &rdata1);
    CAEN_DGTZ_WriteRegister(tHandle[0], ADDR_FRONT_PANEL_IO_SET, 0x00050000);
    CAEN_DGTZ_ReadRegister(tHandle[1], ADDR_FRONT_PANEL_IO_SET, &rdata2);
    CAEN_DGTZ_WriteRegister(tHandle[1], ADDR_FRONT_PANEL_IO_SET, 0x00050000);
    
    /*    }*/
    printf("Trigger Clk is now output on TRGOUT.\n");
    printf("Press [r] to reload PLL config, [s] to start acquisition, any other key to quit\n");
    while( (c=getch()) == 'r') {
      CAEN_DGTZ_WriteRegister(gHandle, ADDR_RELOAD_PLL, 0);
      CAEN_DGTZ_WriteRegister(tHandle[0], ADDR_RELOAD_PLL, 0);
      CAEN_DGTZ_WriteRegister(tHandle[1], ADDR_RELOAD_PLL, 0);
      ForceClockSync(gHandle,tHandle,gParams.NumOfV1742);
      printf("PLL reloaded\n");
    }
    /*    for(unsigned int i=0; i < totalNumbOfDigi; i++)*/
    
    CAEN_DGTZ_WriteRegister(gHandle, ADDR_FRONT_PANEL_IO_SET, rdata0);
    CAEN_DGTZ_WriteRegister(tHandle[0], ADDR_FRONT_PANEL_IO_SET, rdata1);
    CAEN_DGTZ_WriteRegister(tHandle[1], ADDR_FRONT_PANEL_IO_SET, rdata2);
    free(rdata0);
    free(rdata1);
    free(rdata2);
  }
  
  
  /*  printf("\nPress a key to start the acquisition\n");*/
  
  if (c != 's') {
    	GetNextEvent[0]=1;
	GetNextEvent[1]=1;

    return -1;
  }
  ret = CAEN_DGTZ_SWStartAcquisition(gHandle);
  printf("Acquisition started\n");
  
  return 0; 
}

/*  
 * * Checks if a key has been pressed and sets global variables accordingly.
 ** Returns 0 on success; -1 in case of any error detected or when exit key is pressed.
 */
int check_user_input() {
  int c;
  unsigned int i;
  
  /* check if any key has been pressed */
  if (kbhit()) {    
    c = getch(); 
    if (c=='q') return -1;
    if (c=='t') gSWTrigger = 1;
    if (c=='0') {gAnalogTrace = 0; gToggleTrace = 1;}
    if (c=='1') {gAnalogTrace = 1; gToggleTrace = 1;}
    if (c=='2') {gAnalogTrace = 2; gToggleTrace = 1;}
    if (c=='l') gParams.SaveList = 1;
    if (c=='T') gSWTrigger = gSWTrigger ^ 2;
    if (c=='r') {	
      for(i= 0 ; i < gEquippedChannels; ++i) {
        memset(gHisto[i], 0, HISTO_NBIN * sizeof(uint32_t));
        gEvCnt[i] = 0;
      }
      gAcqStats.nb = 0;
      gLoops = 0;
    }
    if (c=='R') {
      gRestart = 1;
    }
    
    if (c=='c') {
      printf("Channel = ");
      scanf("%d", &gActiveChannel);
    }
    if (c=='g') {
      printf("Group = ");
      scanf("%d", &grp4stats);
    }
  } /*  if (kbhit()) */
  
  return 0;
}
