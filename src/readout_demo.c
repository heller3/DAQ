/******************************************************************************
 * 
 * CAEN SpA - Front End Division
 * Via Vetraia, 11 - 55049 - Viareggio ITALY
 * +390594388398 - www.caen.it
 *
 ******************************************************************************/

#include "dpp_qdc.h"

// int	gHandle; /* CAEN library handle */
// int      tHandle[8]; /* CAEN library handle */



void usage() {
  printf("Syntax : readout_demo <configuration file>\n");
  //   printf("acquisition_runtime must be an integer > 0 and < %d\n", LONG_MAX);
}

/* ============================================================================== */
/* main                                                                           */
/* ============================================================================== */


int main(int argc, char* argv[])
{
  int ret;
  int acquisition_runtime = 0; /* 0 = infinite run (exit with 'q') */
  int          running    = 0;
  char*             fname = 0;
  
  /* Command line processing */
  if (argc > 1) 
  {
    fname = argv[1];
  }
  else
  {
    fname = "config.txt";
  }
  if (argc == 3) 
  {
    acquisition_runtime = atoi(argv[2]) * 1000;
    if (acquisition_runtime < 0) {
      usage();
      exit(-1);
    }
    if (errno == ERANGE) {
      perror("acquisition_runtime parameter too big:\n");
      exit(-1);
    }
  }
  
  //   }
  
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
  
}
