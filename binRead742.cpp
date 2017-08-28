// g++ -o binRead742 ../binRead742.cpp

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


typedef struct
{
  double TTT[4];                              /*Trigger time tag of the groups for this board */
  double PulseEdgeTime[32];                 /*PulseEdgeTime for each channel in the group*/
} Data742_t;


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
  int groups = 4;
  int chPerGroup = 8;

  Data742_t ev742;
  while(fread((void*)&ev742, sizeof(ev742), 1, fIn) == 1)
  {
    std::cout << std::fixed << std::showpoint ;
    for(int gr = 0 ; gr < groups ; gr ++)
    {
      std::cout << std::setprecision(4) << ev742.TTT[gr] << " ";
      for(int ch = 0 ; ch < chPerGroup ; ch ++)
      {
        std::cout << std::setprecision(6) << ev742.PulseEdgeTime[gr * chPerGroup + ch] << " ";
      }
    }
    std::cout << std::endl;
    counter++;
  }

  // std::cout << "Events in file " << file0 << " = " << counter << std::endl;


  fclose(fIn);

  return 0;
}
