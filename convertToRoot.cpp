// g++ -o convertToRoot ../convertToRoot.cpp `root-config --cflags --glibs`

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
#include <iomanip>      // satd::setprecision

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

//ROOT includes
#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"


struct EventFormat_t
{
  double TTT740;                         /*Trigger time tag of the event according to 740*/
  double TTT742_0;                       /*Trigger time tag of the event according to 742_0*/
  double TTT742_1;                       /*Trigger time tag of the event according to 742_1*/
  uint16_t Charge[64];                      /*Integrated charge for all the channels of 740 digitizer*/
  double PulseEdgeTime[64];                 /*PulseEdgeTime for each channel in both timing digitizers*/
} __attribute__((__packed__));


std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}


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

  //declare ROOT ouput TTree and file
  ULong64_t DeltaTimeTag = 0;
  ULong64_t ExtendedTimeTag = 0;
  ULong64_t startTimeTag = 0;
  Short_t charge[64];
  Float_t timestamp[64];
  //the ttree variable
  TTree *t1 ;
  //strings for the names
  std::stringstream snames;
  std::stringstream stypes;
  std::string names;
  std::string types;
  t1 = new TTree("adc","adc");
  t1->Branch("ExtendedTimeTag",&ExtendedTimeTag,"ExtendedTimeTag/l"); 	//absolute time tag of the event
  t1->Branch("DeltaTimeTag",&DeltaTimeTag,"DeltaTimeTag/l"); 			//delta time from previous event
  for (int i = 0 ; i < 64 ; i++)
  {
    //empty the stringstreams
    snames.str(std::string());
    stypes.str(std::string());
    charge[i] = 0;
    snames << "ch" << i;
    stypes << "ch" << i << "/S";
    names = snames.str();
    types = stypes.str();
    t1->Branch(names.c_str(),&charge[i],types.c_str());
  }
  for (int i = 0 ; i < 64 ; i++)
  {
    //empty the stringstreams
    snames.str(std::string());
    stypes.str(std::string());
    timestamp[i] = 0;
    snames << "t" << i;
    stypes << "t" << i << "/F";
    names = snames.str();
    types = stypes.str();
    t1->Branch(names.c_str(),&timestamp[i],types.c_str());
  }
  long long int counter = 0;

  EventFormat_t ev;

  long long int file0N = filesize(file0) /  sizeof(ev);

  while(fread((void*)&ev, sizeof(ev), 1, fIn) == 1)
  {
    ULong64_t GlobalTTT = ev.TTT740;
//     std::cout << std::fixed << std::showpoint << std::setprecision(4) << GlobalTTT << " ";
    if(counter == 0)
      startTimeTag = (ULong64_t) GlobalTTT;

    ExtendedTimeTag = (ULong64_t) GlobalTTT;
    DeltaTimeTag    = (ULong64_t) GlobalTTT - startTimeTag;
    for(int i = 0 ; i < 64 ; i ++)
    {
//       std::cout << ev.Charge[i] << " ";
      charge[i] = (Short_t) ev.Charge[i];
    }
    for(int i = 0 ; i < 64 ; i ++)
    {
//       std::cout << std::setprecision(6) << ev.PulseEdgeTime[i] << " ";
      timestamp[i] = (Float_t) ev.PulseEdgeTime[i];
    }
    t1->Fill();
//     std::cout << std::endl;
    counter++;
    if((file0N % counter) == 0)
    {
      std::cout << 100 * counter / file0N << "%\r" ;
    }
  }

  std::cout << "Events in file " << file0 << " = " << counter << std::endl;


  fclose(fIn);

  TFile* fTree = new TFile("testTree.root","recreate");
  fTree->cd();
  t1->Write();
  fTree->Close();

  return 0;
}
