// g++ -o boardPerformance ../boardPerformance.cpp `root-config --cflags --glibs`

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

  histo_sameGroup->GetXaxis()->SetTitle("Time [ps]");
  histo_sameGroup->SetTitle("Time difference between channels on same group");
  histo_AltSameGroup->GetXaxis()->SetTitle("Time [ps]");
  histo_AltSameGroup->SetTitle("Time difference between channels on same group, ch0 as ref.");
  histo_sameTR->GetXaxis()->SetTitle("Time [ps]");
  histo_sameTR->SetTitle("Time difference between channels on same board and same TRn");
  histo_sameBoard->GetXaxis()->SetTitle("Time [ps]");
  histo_sameBoard->SetTitle("Time difference between channels on same board, but different TRn");
  histo_diffBoard->GetXaxis()->SetTitle("Time [ps]");
  histo_diffBoard->SetTitle("Time difference between channels on different 742 boards");
  

  long long int file0N = filesize(file0) /  sizeof(ev);
  std::cout << "Events in file " << file0 << " = " << file0N << std::endl;
  while(fread((void*)&ev, sizeof(ev), 1, fIn) == 1)
  {
    // fill the histrograms and graphs
    histo_deltaT_740_742_0->Fill( ev.TTT740 - ev.TTT742_0 );
    histo_deltaT_740_742_1->Fill( ev.TTT740 - ev.TTT742_1 );
//     t.push_back(GlobalTTT);
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
  }

  std::cout << "Events analized = " << counter << std::endl;
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
