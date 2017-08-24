/******************************************************************************
 * 
 * CAEN SpA - Front End Division
 * Via Vetraia, 11 - 55049 - Viareggio ITALY
 * +390594388398 - www.caen.it
 *
 ******************************************************************************/

#ifndef _DPP_QDC_H
#define _DPP_QDC_H

/* System library includes */
#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
// #include <process.h>  //mod, doesn't exists in linux
#include <limits.h>      //mod to use under linux
#include <errno.h>

#include "CAENDigitizer.h"
#include "_CAENDigitizer_DPP-QDC.h"

/* Macro definition */ 
// #define popen  _popen    /* redefine POSIX 'deprecated' popen as _popen */     // mod to work on linux
// #define pclose _pclose   /* redefine POSIX 'deprecated' pclose as _pclose */  //mod to work on linux


#define CAEN_USE_DIGITIZERS
#define IGNORE_DPP_DEPRECATED

#define ACQMODE_LIST    0
#define ACQMODE_MIXED   1

#define OUTPUTMODE_TEXT    0
#define OUTPUTMODE_BINARY  1
#define OUTPUTMODE_BOTH    2


#define CONNECTION_TYPE_USB    0
#define CONNECTION_TYPE_OPT    1
#define CONNECTION_TYPE_AUTO   255

#define HISTO_NBIN    4096

#define MAX_AGGR_NUM_PER_BLOCK_TRANSFER   1023 /* MAX 1023 */ 
#define MAX_EVENTS_XFER   255

#define GROUPS_742 4

#define ENABLE_TEST_PULSE 0

// #define CONFIG_FILE_NAME "config.txt"

/* Charge cuts */
#define CHARGE_LLD_CUT 0
#define CHARGE_ULD_CUT (HISTO_NBIN-1)

//****************************************************************************
// Some register addresses
//****************************************************************************
#define ADDR_GLOBAL_TRG_MASK     0x810C
#define ADDR_TRG_OUT_MASK        0x8110
#define ADDR_FRONT_PANEL_IO_SET  0x811C
#define ADDR_ACQUISITION_MODE    0x8100
#define ADDR_EXT_TRG_INHIBIT     0x817C
#define ADDR_RUN_DELAY           0x8170
#define ADDR_FORCE_SYNC			 0x813C
#define ADDR_RELOAD_PLL			 0xEF34


//****************************************************************************
// Run Modes
//****************************************************************************
// start on software command
#define RUN_START_ON_SOFTWARE_COMMAND     0xC
// start on S-IN level (logical high = run; logical low = stop)
// #define RUN_START_ON_SIN_LEVEL            0xD
#define RUN_START_ON_SIN_LEVEL            0x5       // MOD after CAEN suggestion
// start on first TRG-IN or Software Trigger
#define RUN_START_ON_TRGIN_RISING_EDGE    0xE
// start on LVDS I/O level
#define RUN_START_ON_LVDS_IO              0xF


/* Data structures */

typedef struct 
{
  // V1740D
  uint32_t OutputMode;
  uint32_t OutputWaves742;
  uint32_t ConnectionType;
  uint32_t ConnectionLinkNum;
  uint32_t ConnectionConetNode;
  uint32_t ConnectionVMEBaseAddress;
  uint32_t RecordLength;   
  uint32_t PreTrigger;     
  uint32_t ActiveChannel;
  uint32_t GateWidth[8];
  uint32_t PreGate;
  uint32_t ChargeSensitivity;
  uint32_t FixedBaseline;
  uint32_t BaselineMode;
  uint32_t EnablePlots740;
  uint32_t EnablePlots742;
  uint32_t Groups742;
  uint32_t ChannelsPerGroup742;
  uint32_t TrgMode;
  uint32_t TrgSmoothing;
  uint32_t TrgHoldOff;
  uint32_t TriggerThreshold[64];
  uint32_t DCoffset[8];
  uint32_t AcqMode;
  uint32_t NevAggr;
  uint32_t PulsePol;
  uint32_t EnChargePed;
  uint32_t SaveList;
  uint32_t DisTrigHist;
  uint32_t DisSelfTrigger;
  uint32_t EnTestPulses;
  uint32_t TestPulsesRate;
  uint32_t DefaultTriggerThr;
  uint32_t EnableExtendedTimeStamp;
  uint64_t ChannelTriggerMask;
  uint32_t RunDelay;
  
  //V1742
  uint32_t NumOfV1742;                   //number of V1742 digitizers in the system - NB max is 8
  uint32_t registerWritesCounter;
  uint32_t v1742_TriggerEdge;   
  uint32_t v1742_RecordLength;  
  uint32_t v1742_MatchingWindow;
  uint32_t v1742_IOlevel;       
  uint32_t v1742_TestPattern;   
  uint32_t v1742_DRS4Frequency; 
  uint32_t v1742_StartMode;     
  uint32_t v1742_EnableLog;
  
  
  uint32_t v1742_ConnectionType[8];
  uint32_t v1742_LinkNum[8];
  uint32_t v1742_ConetNode[8];
  uint32_t v1742_BaseAddress[8];  
  uint32_t v1742_FastTriggerThreshold[8];
  uint32_t v1742_FastTriggerOffset[8];   
  uint32_t v1742_DCoffset[8];            
  uint32_t v1742_ChannelThreshold[8];    
  uint32_t v1742_TRThreshold[8];         
  CAEN_DGTZ_TriggerPolarity_t v1742_ChannelPulseEdge[8];    
  uint32_t v1742_PostTrigger[8]; 
  uint32_t v1742_RunDelay[8]; 
  
  uint32_t RegisterWriteBoard[64];                           // no more than 64 register writes, sorry...
  uint32_t RegisterWriteAddr[64];                           // no more than 64 register writes, sorry...
  uint32_t RegisterWriteValue[64];                           // no more than 64 register writes, sorry...
  
} BoardParameters;

/* Data structures */
typedef struct 
{
  long gRunElapsedTime;                                        /* Acquisition time (elapsed since start) */
  uint64_t TotEvCnt;                                           /* Total number of events                 */
  unsigned int nb;                                             /* Number of bytes read                   */
} Stats;


typedef struct
{
  float TTT[2];
  float *Wave[2];
  float *Trigger[2];
  float PulseEdgeTime[2];
  float TrEdgeTime[2];
} OutputData_t;

typedef struct
{
  double TTT;                                 /*Trigger time tag of the event, i.e. of the entire board (we always operate with common external trigger) */
  uint16_t Charge[64];                        /*All 64 channels for now*/
} Data740_t;

typedef struct
{
  double TTT[4];                              /*Trigger time tag of the groups for this board */
  double PulseEdgeTime[32];                 /*PulseEdgeTime for each channel in the group*/
} Data742_t;



/* Globals */
// extern int	        gHandle;                                   /* CAEN library handle */
int	gHandle; /* CAEN library handle */
int      tHandle[8]; /* CAEN library handle */

/* Variable declarations */                                    
extern unsigned int gActiveChannel;                            /* Active channel for data analysis */
extern unsigned int gEquippedChannels;                         /* Number of equipped channels      */
extern unsigned int gEquippedGroups;                           /* Number of equipped groups        */

extern long         gCurrTime;                                 /* Current time                     */
extern long         gPrevTime;                                 /* Previous saved time              */
extern long         gPrevWPlotTime;                            /* Previous time of Waveform plot   */
extern long         gPrevHPlotTime;                            /* Previous time of Histogram plot  */
extern long         gRunStartTime;                             /* Time of run start                */
extern long         gRunElapsedTime;                           /* Elapsed time                     */

extern uint32_t     gEvCnt[MAX_CHANNELS];                      /* Event counters per channel       */
extern uint32_t     gEvCntOld[MAX_CHANNELS];                   /* Event counters per channel (old) */
extern uint32_t**   gHisto;                                    /* Histograms                       */

extern uint64_t     gExtendedTimeTag[MAX_CHANNELS];            /* Extended Time Tag                */
extern uint64_t     gETT[MAX_CHANNELS];                        /* Extended Time Tag                */
extern uint64_t     gPrevTimeTag[MAX_CHANNELS];                /* Previous Time Tag                */

extern BoardParameters   gParams;                              /* Board parameters structure       */
extern Stats        gAcqStats;                                 /* Acquisition statistics           */

extern FILE *       gPlotDataFile;                             /* Target file for plotting data    */
extern FILE *       gListFiles[MAX_CHANNELS];                  /* Output file for list data        */

extern CAEN_DGTZ_BoardInfo_t             gBoardInfo;           /* Board Informations               */ 
extern _CAEN_DGTZ_DPP_QDC_Event_t*     gEvent[MAX_CHANNELS]; /* Events                           */
extern _CAEN_DGTZ_DPP_QDC_Waveforms_t* gWaveforms;           /* Waveforms                        */

/* Variable definitions */
extern int          gSWTrigger;                                /* Signal for software trigger          */
extern int          grp4stats;                                 /* Selected group for statistics        */
extern int          gLoops;                                    /* Number of acquisition loops          */
extern int          gToggleTrace;                              /* Signal for trace toggle from user    */
extern int          gAnalogTrace;                              /* Signal for analog trace toggle       */
extern int          gRestart    ;                              /* Signal acquisition restart from user */
extern char *       gAcqBuffer;                                /* Acquisition buffer                   */
extern char *       gEventPtr;                                 /* Events buffer pointer                */
extern FILE *       gHistPlotFile;                             /* Source file for histogram plot       */
extern FILE *       gWavePlotFile;                             /* Source file for waveform plot        */


/* *************************************************************************************************
 * * Function prototypes 
 ****************************************************************************************************/

/* readout_demo functions prototypes */
int  setup_acquisition(char* fname);
int  run_acquisition();
void print_statistics();
int  check_user_input();                    
int  cleanup_on_exit();

/* Board parameters utility functions */
void set_default_parameters(BoardParameters *params);
int  load_configuration_from_file(char * fname, BoardParameters *params);
int  setup_parameters(BoardParameters *params, char *fname);
int  configure_digitizer(int handle, int gEquippedGroups, BoardParameters *params);
int  configure_timing_digitizers(/*int* tHandle,*/ BoardParameters *params);
int  SetSyncMode(int handle, int timing, uint32_t runDelay);
int  ForceClockSync(int handle,int tHandle[],int size);


/* Utility functions prototypes */
long get_time();
void clear_screen( void );
double interpolateWithMultiplePoints(float* data, unsigned int length, int threshold, CAEN_DGTZ_TriggerPolarity_t edge, double Tstart, int WaveType);

#ifdef LINUX
#include <sys/time.h> /* struct timeval, select() */
#include <termios.h> /* tcgetattr(), tcsetattr() */
#include <stdlib.h> /* atexit(), exit() */
#include <unistd.h> /* read() */
#include <stdio.h> /* printf() */
#include <string.h> /* memcpy() */

#define CLEARSCR "clear"

/*****************************************************************************/
/*  SLEEP  */
/*****************************************************************************/
void Sleep(int t);

/*****************************************************************************/
/*  GETCH  */
/*****************************************************************************/
int getch(void);

/*****************************************************************************/
/*  KBHIT  */
/*****************************************************************************/
int kbhit();


#else  /* Windows */

#include <conio.h>
#define getch _getch
#define kbhit _kbhit

#define CLEARSCR "cls"
#endif

#endif