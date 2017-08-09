// g++ -o binRead740 ../binRead740.cpp 

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
  double TTT;                                 /*Trigger time tag of the event, i.e. of the entire board (we always operate with common external trigger) */
  uint16_t Charge[64];                        /*All 64 channels for now*/
} Data740_t;


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
//   std::cout << "Reading File " << file0 << "..." << std::endl;
  
  Data740_t ev740;
  while(fread((void*)&ev740, sizeof(ev740), 1, fIn) == 1)
  {
    std::cout << std::fixed << std::showpoint << std::setprecision(4) << ev740.TTT << " ";
    for(int i = 0 ; i < 64 ; i ++)
    {
      std::cout << ev740.Charge[i] << " ";
    }
    std::cout << std::endl;
    counter++;
  }
  
  std::cout << "Events in file " << file0 << " = " << counter << std::endl;


  fclose(fIn);
  
  return 0; 
}