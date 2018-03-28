/******************************************************************************
 * 
 * CAEN SpA - Front End Division
 * Via Vetraia, 11 - 55049 - Viareggio ITALY
 * +390594388398 - www.caen.it
 *
 ******************************************************************************/

#include "dpp_qdc.h"
#include <getopt.h>

// int	gHandle; /* CAEN library handle */
// int      tHandle[8]; /* CAEN library handle */



// void usage() {
//   printf("Syntax : readout <configuration file>\n");
//   //   printf("acquisition_runtime must be an integer > 0 and < %d\n", LONG_MAX);
// }

void usage()
{
  printf("\t\t [-c|--config] <config_file> [-t|--time] <acq_duration> [-f|--frame] <frame_name>\n");
  printf("\t\t <config_file>                              - full name of config file, mandatory parameter\n");
  printf("\t\t <acq_duration>                             - duration of acquisition, in [s]. integer num. if not given, endless acq (needs to be killed with 'q' keystroke) \n");
  printf("\t\t <frame_name>                               - frame name (for multiple acquisitions)\n");
}


/* ============================================================================== */
/* main                                                                           */
/* ============================================================================== */


int main(int argc, char* argv[])
{
  if(argc < 2)
  {
    printf("Usage: %s\n",argv[0]);
    usage();
    exit(-1);
  }
  
  int ret;
  int acquisition_runtime = 0; /* 0 = infinite run (exit with 'q') */
  int          running    = 0;
  char*             fname = 0;
  gAcqStats.frameName     = 0;
  gAcqStats.start         = 0;
//   int frame = -1;
  
  // parse arguments
  static struct option longOptions[] =
  {
    { "config", required_argument, 0, 0 },
    { "time", required_argument, 0, 0 },
    { "frame", required_argument, 0, 0 },
    { "start", no_argument, 0, 0 },
    { NULL, 0, 0, 0 }
  };
  
  while(1) {
    int optionIndex = 0;
    int c = getopt_long(argc, argv, "c:t:f:", longOptions, &optionIndex);
    if (c == -1) {
      break;
    }
    if (c == 'c'){
      fname = (char *)optarg;
    }
    else if (c == 't'){
      acquisition_runtime = 1000 * atoi((char *)optarg);
    }
    else if (c == 'f'){
      gAcqStats.frameName = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 0){
      fname = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 1){
      acquisition_runtime = 1000 * atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 2){
      gAcqStats.frameName = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 3){
      gAcqStats.start = 1;
    }
    else {
      printf("Usage: %s\n",argv[0]);
      usage();
      exit(-1);
    }
  }
  
  if(fname == 0)
  {
    printf("ERROR! You need to provide a valid configuration file!\n\n");
    printf("Usage: %s\n",argv[0]);
    usage();
    exit(-1);
  }
  
  
  
  /* Init statistics */
  gAcqStats.TotEvCnt = 0;
  gAcqStats.nb       = 0;
  
  running        = 1;
  
  /* Board configuration prior to run */
  ret = setup_acquisition(fname);
  if (ret) {
    printf("Error during acquisition setup (ret = %d) .... Exiting\n", ret);
    running = 0;
  }
  
//   printf("gHandle = %d\n",gHandle);
  /* Init time global variables */
  gCurrTime      = get_time();
  gRunStartTime  = gCurrTime;
  gPrevTime      = gCurrTime;
  gPrevHPlotTime = gCurrTime;
  gPrevWPlotTime = gCurrTime;
  
  //     ret = CAEN_DGTZ_Reset(0);
  //     printf("Reset Result = %d \n",ret);
  /* Readout Loop */
  while(running) {
    
    if (gRestart) {
      gRestart = 0;
      if(load_configuration_from_file(fname, &gParams)) {
	printf("Error during acquisition restart: configuration file %s reload failed! .... Exiting\n", fname);
	break;
      }
      
      
      if(setup_acquisition(fname)) {
	printf("Error during acquisition restart: acquisition setup failed! .... Exiting\n");
	break;
      }
      
      gAcqStats.nb = 0;
      gLoops = 0;
      
    }
    
    /* Get data */
    ret = run_acquisition();
    if (ret) { 
      printf("Error during acquisition loop (ret = %d) .... Exiting\n", ret);
      break;
    }
    
    /* Periodic console update -... could be a separate thread .... */
    print_statistics();
    
    /* 
     * * Check acquisition run time.
     ** exit from loop if programmed time is elapsed.
     */
    if (acquisition_runtime)
      if (gAcqStats.gRunElapsedTime > acquisition_runtime)
	running = 0;
      
      /* Periodic keyboard input -... could be a separate thread .... */
      ret = check_user_input();
    if (ret) running = 0;
    
    
    
  } /* end of readout loop */
  
  /* Final memory cleanup */
  cleanup_on_exit();
  return ret;
  
}
