// g++ -o binReadEvent ../binReadEvent.cpp

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <getopt.h>
#include <iostream>
#include <set>
#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>      // std::setprecision

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

struct EventFormat_t
{
  double TTT740;                         /*Trigger time tag of the event according to 740*/
  double TTT742_0;                       /*Trigger time tag of the event according to 742_0*/
  double TTT742_1;                       /*Trigger time tag of the event according to 742_1*/
  uint16_t Charge[64];                      /*Integrated charge for all the channels of 740 digitizer*/
  double PulseEdgeTime[64];                 /*PulseEdgeTime for each channel in both timing digitizers*/
} __attribute__((__packed__));


//----------------//
//  MAIN PROGRAM  //
//----------------//
int main(int argc,char **argv)
{

  char* file0;
  file0 = argv[1];
  FILE * fIn = NULL;

  fIn = fopen(file0, "rb");

  if (fIn == NULL) {
    fprintf(stderr, "File %s does not exist\n", file0);
    return 1;
  }

  long long int counter = 0;

  EventFormat_t ev;
  while(fread((void*)&ev, sizeof(ev), 1, fIn) == 1)
  {
    std::cout << std::fixed << std::showpoint << std::setprecision(4) << ev.TTT740 << " " << ev.TTT742_0 << " " << ev.TTT742_1 << " " ;

    for(int i = 0 ; i < 64 ; i ++)
    {
      std::cout << ev.Charge[i] << " ";
    }
    for(int i = 0 ; i < 64 ; i ++)
    {
      std::cout << std::setprecision(6) << ev.PulseEdgeTime[i] << " ";
    }
    std::cout << std::endl;
    counter++;
  }

  std::cout << "Events in file " << file0 << " = " << counter << std::endl;


  fclose(fIn);

  return 0;
}
