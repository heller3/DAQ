// g++ -o convertToRoot740 ../convertToRoot740.cpp `root-config --cflags --glibs`

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

//ROOT includes
#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"

#define ROOTFILELENGTH 100000

// struct EventFormat_t
// {
//   double GlobalTTT;                         /*Trigger time tag of the event. For now, it's the TTT of the 740 digitizer */
//   uint16_t Charge[64];                      /*Integrated charge for all the channels of 740 digitizer*/
//   double PulseEdgeTime[64];                 /*PulseEdgeTime for each channel in both timing digitizers*/
// } __attribute__((__packed__));

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

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
  //first, create a new directory
  std::string dirName = "./RootTTrees";
//   dirName += actual_time;
  std::string MakeFolder;
  MakeFolder = "mkdir " + dirName;
  system(MakeFolder.c_str());
  
  //declare ROOT ouput TTree and file
  ULong64_t DeltaTimeTag = 0;
  ULong64_t ExtendedTimeTag = 0;
  ULong64_t startTimeTag = 0;
  UShort_t charge[64];
  Float_t timestamp[64];
  //the ttree variable
  TTree *t1 ;
  //strings for the names
  std::stringstream snames;
  std::stringstream stypes;
  std::string names;
  std::string types;
//   t1 = new TTree("adc","adc");
//   t1->Branch("ExtendedTimeTag",&ExtendedTimeTag,"ExtendedTimeTag/l"); 	//absolute time tag of the event
//   t1->Branch("DeltaTimeTag",&DeltaTimeTag,"DeltaTimeTag/l"); 			//delta time from previous event
//   for (int i = 0 ; i < 64 ; i++)
//   {
//     //empty the stringstreams
//     snames.str(std::string());
//     stypes.str(std::string());
//     charge[i] = 0;
//     snames << "ch" << i;
//     stypes << "ch" << i << "/S";
//     names = snames.str();
//     types = stypes.str();
//     t1->Branch(names.c_str(),&charge[i],types.c_str());
//   }
//   for (int i = 0 ; i < 64 ; i++)
//   {
//     //empty the stringstreams
//     snames.str(std::string());
//     stypes.str(std::string());
//     timestamp[i] = 0;
//     snames << "t" << i;
//     stypes << "t" << i << "/F";
//     names = snames.str();
//     types = stypes.str();
//     t1->Branch(names.c_str(),&timestamp[i],types.c_str());
//   }
  long long int counter = 0;
  int filePart = 0;
  // EventFormat_t ev;
  Data740_t ev;
  // while(fread((void*)&ev740, sizeof(ev740), 1, fIn) == 1)
  std::cout << "Reading file " << file0 << "..." <<std::endl;
  long long int listNum = 0;
  int NumOfRootFile = 0;
  long long int file0N = filesize(file0) /  sizeof(ev);
  std::cout << "Events in file " << file0 << " = " << file0N << std::endl;
  
  while(fread((void*)&ev, sizeof(ev), 1, fIn) == 1)
  {
    if( listNum == 0 ){
      t1 = new TTree("adc","adc");
      t1->Branch("ExtendedTimeTag",&ExtendedTimeTag,"ExtendedTimeTag/l");   //absolute time tag of the event
      t1->Branch("DeltaTimeTag",&DeltaTimeTag,"DeltaTimeTag/l");                    //delta time from previous event
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
//       std::cout << "RunNumber = " << counter << std::endl;
    }
    else if( (listNum != 0) && ( (int)(listNum/ROOTFILELENGTH) > NumOfRootFile ))
    {
      NumOfRootFile++;
      //save previous ttree
      //file name
      std::stringstream fileRootStream;
      std::string fileRoot;
      fileRootStream << dirName << "/TTree_" << filePart << ".root";
      fileRoot = fileRootStream.str();
//       std::cout << "Saving root file "<< fileRoot << "..." << std::endl;
      TFile* fTree = new TFile(fileRoot.c_str(),"recreate");
      fTree->cd();
      t1->Write();
      fTree->Close();
      //delete previous ttree
      delete t1;
      
      //create new ttree
      t1 = new TTree("adc","adc");
      t1->Branch("ExtendedTimeTag",&ExtendedTimeTag,"ExtendedTimeTag/l");   //absolute time tag of the event
      t1->Branch("DeltaTimeTag",&DeltaTimeTag,"DeltaTimeTag/l");                    //delta time from previous event
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
      filePart++;
      
      //std::cout << counter << std::endl;
      
    }    
//     if(counter >= 3000000)
//       break;
    // std::cout << std::fixed << std::showpoint << std::setprecision(4) << ev.TTT << " ";
    if(counter == 0)
      startTimeTag = (ULong64_t) ev.TTT;

    ExtendedTimeTag = (ULong64_t) ev.TTT;
    DeltaTimeTag    = (ULong64_t) ev.TTT - startTimeTag;
    for(int i = 0 ; i < 64 ; i ++)
    {
      // std::cout << ev.Charge[i] << " ";
      charge[i] = (UShort_t) ev.Charge[i];
    }
    for(int i = 0 ; i < 64 ; i ++)
    {
      // std::cout << std::setprecision(6) << ev.PulseEdgeTime[i] << " ";
      timestamp[i] = 0.0; //(Float_t) ev.PulseEdgeTime[i]; //i.e. ignore timing files
    }
    t1->Fill();

    if( ((100 * counter / file0N) % 10 ) == 0)
        std::cout << "Progress = " <<  100 * counter / file0N << "%\r";
    // std::cout << std::endl;

//     if((counter % 1000) == 0)
//     {
//       std::cout << counter << "\r" ;
//     }

    listNum++;
    counter++;
  }

//   std::cout << "Events in file " << file0 << " = " << counter << std::endl;
  std::stringstream fileRootStreamFinal;
  std::string fileRootFinal;
  fileRootStreamFinal << dirName << "/TTree_" << filePart << ".root";
  fileRootFinal = fileRootStreamFinal.str();
//   std::cout << "Saving root file "<< fileRootFinal << "..." << std::endl;
  TFile* fTreeFinal = new TFile(fileRootFinal.c_str(),"recreate");
  fTreeFinal->cd();
  t1->Write();
  fTreeFinal->Close();
  
  std::cout << "Events exported = " << counter << std::endl;

  fclose(fIn);

//   TFile* fTree = new TFile("testTree.root","recreate");
//   fTree->cd();
//   t1->Write();
//   fTree->Close();

  return 0;
}
