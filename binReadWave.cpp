// g++ -o binReadWave ../binReadWave.cpp

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

struct WaveFormat_t
{
  double TTT;
  float sample[1024];
};


//----------------//
//  MAIN PROGRAM  //
//----------------//
int main(int argc,char **argv)
{

  char* file0;
  char filenameWave[50];
  file0 = argv[1];
  long long int waveNum = -1;
  if(argc > 2)
  {
    waveNum = atoi(argv[2]);
  }

  FILE * fIn = NULL;

  fIn = fopen(file0, "rb");

  if (fIn == NULL) {
    fprintf(stderr, "File %s does not exist\n", file0);
    return 1;
  }

  //optionally put one wave in a text file, for gluplot

  long long int counter = 0;

  WaveFormat_t wave;
  while(fread((void*)&wave, sizeof(wave), 1, fIn) == 1)
  {
    if(waveNum == -1)
    {
      std::cout << wave.TTT << " " ;
      for(int i = 0 ; i < 1024 ; i ++)
      {
        std::cout << wave.sample[i] << " ";
      }
      std::cout << std::endl;
    }

    if(waveNum >= 0 && waveNum == counter)
    {
      FILE *fWave = NULL;

      sprintf(filenameWave,"%lld%s",waveNum,file0);
      fWave = fopen(filenameWave, "w");
      for(int i = 0 ; i < 1024 ; i ++)
      {
        fprintf(fWave,"%f\n",wave.sample[i]);
      }
      fclose(fWave);
      break;
    }
    counter++;
  }

  if(waveNum < 0)
  {
    std::cout << "Waves in file " << file0 << " = " << counter << std::endl;
  }
  else
  {
    std::cout << "Waves data exported to file " << filenameWave << std::endl;
  }

  fclose(fIn);
  return 0;
}
