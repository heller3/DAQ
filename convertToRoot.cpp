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

#include <time.h>
#include <sys/timeb.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

//ROOT includes
#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"
#include "TH1F.h"
#include "TGraph.h"
#include "TF1.h"

#define ROOTFILELENGTH 100000

/* ============================================================================== */
/* Get time of the day 
/* ============================================================================== */
void TimeOfDay(char *actual_time)
{
  struct tm *timeinfo;
  time_t currentTime;
  time(&currentTime);
  timeinfo = localtime(&currentTime);
  strftime (actual_time,20,"%Y_%d_%m_%H_%M_%S",timeinfo);
}



/* ============================================================================== */
/* Get time in milliseconds from the computer internal clock */
/* ============================================================================== */
// long get_time()
// {
//   long time_ms;
//   #ifdef WIN32
//   struct _timeb timebuffer;
//   _ftime( &timebuffer );
//   time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
//   #else
//   struct timeval t1;
//   struct timezone tz;
//   gettimeofday(&t1, &tz);
//   time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
//   #endif
//   return time_ms;
// }

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
  
  // get time of day and create the listmode file name
//   char actual_time[20];
//   TimeOfDay(actual_time);
//   std::string listFileName = "ListFile_"; 
//   listFileName += actual_time;
//   listFileName += ".txt";
  //std::cout << "listFileName " << listFileName << std::endl;
  

  //----------------------------------------//
  // Create Root TTree Folder and variables //
  //----------------------------------------//
  
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
  Short_t charge[64];
  Float_t timestamp[64];
  //the ttree variable
  TTree *t1 ;
  //strings for the names
  std::stringstream snames;
  std::stringstream stypes;
  std::string names;
  std::string types;
  
  long long int counter = 0;

  EventFormat_t ev;
  int filePart = 0;
  long long int runNumber = 0;
  long long int listNum = 0;
  int NumOfRootFile = 0;
  
  
  //also in parallel perform a small analysis on TTT (makes sense basically only with external pulser, but could be useful also without
  int Nbins = 50;
  //histograms
  TH1F *histo_deltaT_740_742_0 = new TH1F("histo_deltaT_740_742_0","histo_deltaT_740_742_0",Nbins,-50,50);
  TH1F *histo_deltaT_740_742_1 = new TH1F("histo_deltaT_740_742_1","histo_deltaT_740_742_1",Nbins,-50,50);
  std::vector<double> t , delta740_742_0,delta740_742_1,delta742_742;
  //For CTR analysis, makes sense if 
  double boundary = 1000;
  int nbinsCTR = 2000;
  TH1F *histo_sameGroup = new TH1F("sameGroup","sameGroup",nbinsCTR,-boundary,boundary);
  TH1F *histo_AltSameGroup = new TH1F("histo_AltSameGroup","histo_AltSameGroup",nbinsCTR,-boundary,boundary);
  TH1F *histo_sameTR    = new TH1F("sameTR","sameTR",nbinsCTR,-boundary,boundary);
  TH1F *histo_sameBoard = new TH1F("sameBoard","sameBoard",nbinsCTR,-boundary,boundary);
  TH1F *histo_diffBoard = new TH1F("diffBoard","diffBoard",nbinsCTR,-boundary,boundary);
  

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
      if( ((100 * counter / file0N) % 10 ) == 0)
        std::cout << "Progress = " <<  100 * counter / file0N << "%\r";
      //std::cout << counter << std::endl;
      
    }    
    
    
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
    //also fill the histrograms and graphs
    histo_deltaT_740_742_0->Fill( ev.TTT740 - ev.TTT742_0 );
    histo_deltaT_740_742_1->Fill( ev.TTT740 - ev.TTT742_1 );
    t.push_back(GlobalTTT);
    delta740_742_0.push_back(ev.TTT740 - ev.TTT742_0);
    delta740_742_1.push_back(ev.TTT740 - ev.TTT742_1);
    delta742_742.push_back(ev.TTT742_0 - ev.TTT742_1);
    
    // PulseEdgeTime are in bins of V1742, 1 bin is 200ps 
    histo_sameGroup->Fill(200.0 * (ev.PulseEdgeTime[0] - ev.PulseEdgeTime[2] )); // channels on same group
    histo_AltSameGroup->Fill(200.0 * (ev.PulseEdgeTime[2] - ev.PulseEdgeTime[4]) ); //channels in same group but reference wave was the ch0
    histo_sameTR   ->Fill(200.0 * (ev.PulseEdgeTime[0] - ev.PulseEdgeTime[8] ));  // channels on same tr (same half board)
    histo_sameBoard->Fill(200.0 * (ev.PulseEdgeTime[0] - ev.PulseEdgeTime[16])); // channels on same board
    histo_diffBoard->Fill(200.0 * (ev.PulseEdgeTime[0] - ev.PulseEdgeTime[32])); // channels on different boards
//     std::cout << std::endl;
    counter++;
    listNum++;
//     if((counter % file0N) == 0)
//     {
//     std::cout << 100 * counter / file0N << "%\r" ;
//     }

    // if((file0N % counter) == 0)
    // {
//       std::cout << 100 * counter / file0N << "%\r" ;
    // }
  }
  
  
  
  
  
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
  
  TFile *fOut = new TFile("offsets.root","RECREATE");
  fOut->cd();
  //graphs
  TGraph* g_delta740_742_0 = new TGraph(t.size(),&t[0],&delta740_742_0[0]);
  g_delta740_742_0->SetName("Trigger Time Tag delta vs. time (V1740D - V1742_0) ");
  g_delta740_742_0->SetTitle("Trigger Time Tag delta vs. time (V1740D - V1742_0) ");
  g_delta740_742_0->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta740_742_0->GetYaxis()->SetTitle("delta [ns]");

  TGraph* g_delta740_742_1 = new TGraph(t.size(),&t[0],&delta740_742_1[0]);
  g_delta740_742_1->SetName("Trigger Time Tag delta vs. time (V1740D - V1742_1) ");
  g_delta740_742_1->SetTitle("Trigger Time Tag delta vs. time (V1740D - V1742_1) ");
  g_delta740_742_1->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta740_742_1->GetYaxis()->SetTitle("delta [ns]");

  TGraph* g_delta742_742 = new TGraph(t.size(),&t[0],&delta742_742[0]);
  g_delta742_742->SetName("Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  g_delta742_742->SetTitle("Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  g_delta742_742->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta742_742->GetYaxis()->SetTitle("delta [ns]");
  
  histo_deltaT_740_742_0->Write();
  histo_deltaT_740_742_1->Write();
  g_delta740_742_0->Write();
  g_delta740_742_1->Write();
  g_delta742_742->Write();
  
  TF1 *gauss_sameGroup = new TF1("gauss_sameGroup","gaus",-boundary,boundary);
  TF1 *gauss_AltSameGroup = new TF1("gauss_AltSameGroup","gaus",-boundary,boundary);
  TF1 *gauss_sameTR    = new TF1("gauss_sameTR","gaus",-boundary,boundary);
  TF1 *gauss_sameBoard = new TF1("gauss_sameBoard","gaus",-boundary,boundary);
  TF1 *gauss_diffBoard = new TF1("gauss_diffBoard","gaus",-boundary,boundary);

  histo_sameGroup->Fit("gauss_sameGroup","Q");
  histo_AltSameGroup->Fit("gauss_AltSameGroup","Q");
  histo_sameTR   ->Fit("gauss_sameTR"   ,"Q");
  histo_sameBoard->Fit("gauss_sameBoard","Q");
  histo_diffBoard->Fit("gauss_diffBoard","Q");
  
  std::cout << std::endl;

  std::cout << "CTR FWHM same group         = " <<  gauss_sameGroup   ->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM alt. same group    = " <<  gauss_AltSameGroup->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM same tr            = " <<  gauss_sameTR      ->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM same board         = " <<  gauss_sameBoard   ->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM different boards   = " <<  gauss_diffBoard   ->GetParameter(2) * 2.355 << "ps" << std::endl;
  
  histo_sameGroup->Write();
  histo_sameTR   ->Write();
  histo_sameBoard->Write();
  histo_diffBoard->Write();
  
  
  fOut->Close();
//   TFile* fTree = new TFile("testTree.root","recreate");
//   fTree->cd();
//   t1->Write();
//   fTree->Close();

  return 0;
}
