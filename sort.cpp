// g++ -o sort ../sort.cpp `root-config --cflags --glibs`


//-------------------------------------------
// converts binary data from single boards to a combined event file
//
// cycle over the first N (N=5000) events of the input to find the time offsets
// then restart from the beginning of the dataset and import data using the offset previously found to correct the trigger time tags.
// It then sorts the events of the different boards in global events, using as criterium the corrected trigger time tags.
// The events will be saved in a file called
//
// events.dat
//
// while some histograms are saved in the root file
//
// histograms.root
//-------------------------------------------


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
#include <TROOT.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1F.h>
#include <TGraphErrors.h>
#include <TPad.h>
#include <TGraph.h>
#include <THStack.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TMath.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>



typedef struct
{
  double TTT;                                 /*Trigger time tag of the event, i.e. of the entire board (we always operate with common external trigger) */
  uint16_t Charge[64];                        /*All 64 channels for now*/
} Data740_t;

typedef struct
{
  double TTT[4];                              /*Trigger time tag of the groups for this board */
  double PulseEdgeTime[32];                 /*PulseEdgeTime for each channel in the group*/
} Data742_t;

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

  //----------------------------//
  // PARAMETERS
  //----------------------------//
  int eventsForCalibration = 5000;     // first N events of the dataset used to find the time offsets
  int minDeltaCalibration = -400;      // min difference in time offset, in ns, for the fast histograms
  int maxDeltaCalibration = 400;       // max difference in time offset, in ns, for the fast histograms
  int misDepth = 1000;                   // in how many follwing events the program will look for a match
  float tolerance = 6.0;               // N of sigmas for accepting a simultaneous event
  int Nbins = 100;                     // N of bins in the fast histograms for time alignment
  //---------------------------

  gStyle->SetOptFit(1);
  if(argc < 3)
  {
    std::cout << "You need to provide 3 files!" << std::endl;
    return 1;
  }

  char* file0;
  char* file1;
  char* file2;
  file0 = argv[1];
  file1 = argv[2];
  file2 = argv[3];
  std::vector<Data740_t> input740;
  std::vector<Data742_t> input742_0;
  std::vector<Data742_t> input742_1;

  TCanvas *c1 = new TCanvas("c1","c1",1200,800);   //useless but avoid root output

  FILE * in0 = NULL;
  FILE * in1 = NULL;
  FILE * in2 = NULL;

  in0 = fopen(file0, "rb");
  in1 = fopen(file1, "rb");
  in2 = fopen(file2, "rb");

  long long int counter = 0;

  //------------------------------//
  // FAST READING FOR OFFSETS
  //------------------------------//
  std::cout << std::endl <<  "Looking for offsets..." /*<< std::endl*/;
  Data740_t ev740;
  while(fread((void*)&ev740, sizeof(ev740), 1, in0) == 1)
  {

    counter++;
    if(counter == eventsForCalibration) break;
    input740.push_back(ev740);
  }
  counter = 0;
  Data742_t ev742;
  while(fread((void*)&ev742, sizeof(ev742), 1, in1) == 1)
  {
    counter++;
    if(counter == eventsForCalibration) break;
    input742_0.push_back(ev742);
  }
  counter = 0;
  while(fread((void*)&ev742, sizeof(ev742), 1, in2) == 1)
  {

    counter++;
    if(counter == eventsForCalibration) break;
    input742_1.push_back(ev742);
  }
  //fills histograms and find average offsets
  TH1F *fast740_742_0 = new TH1F("fast740_742_0","fast740_742_0",Nbins,minDeltaCalibration,maxDeltaCalibration);
  TH1F *fast740_742_1 = new TH1F("fast740_742_1","fast740_742_1",Nbins,minDeltaCalibration,maxDeltaCalibration);
  for(int i = 0 ; i < input740.size() ; i++)
  {
    fast740_742_0->Fill(( input740[i].TTT - input742_0[i].TTT[2]  ));
    fast740_742_1->Fill(( input740[i].TTT - input742_1[i].TTT[2]  ));
  }
  TF1 *gaussFast0 = new TF1("gaussFast0","gaus",minDeltaCalibration,maxDeltaCalibration);
  TF1 *gaussFast1 = new TF1("gaussFast1","gaus",minDeltaCalibration,maxDeltaCalibration);
  fast740_742_0->Fit("gaussFast0","Q");
  fast740_742_1->Fit("gaussFast1","Q");
  double delta740to742_0 = gaussFast0->GetParameter(1);
  double delta740to742_1 = gaussFast1->GetParameter(1);
  double sigmaT = gaussFast0->GetParameter(2);
  if( gaussFast1->GetParameter(2) >  sigmaT)
    sigmaT = gaussFast1->GetParameter(2);

  std::cout << "done"<< std::endl;
  std::cout << std::endl;
  std::cout << "Delta 740 to 742_0 = (" <<  gaussFast0->GetParameter(1) << " +/- " << gaussFast0->GetParameter(2) << ") ns" << std::endl;
  std::cout << "Delta 740 to 742_0 = (" <<  gaussFast1->GetParameter(1) << " +/- " << gaussFast1->GetParameter(2) << ") ns" << std::endl;
  std::cout << std::endl;

  fclose(in0);
  fclose(in1);
  fclose(in2);
  input740.clear();
  input742_0.clear();
  input742_1.clear();


  //------------------------------//
  // COMPLETE READING
  //------------------------------//
  // open again the input files and imports data correcting with offsets
  in0 = fopen(file0, "rb");
  in1 = fopen(file1, "rb");
  in2 = fopen(file2, "rb");
  std::cout << "Reading File " << file0 << "..." /*<< std::endl*/;

  while(fread((void*)&ev740, sizeof(ev740), 1, in0) == 1)
  {
    input740.push_back(ev740);
    counter++;
  }
  std::cout << " done" << std::endl;

  counter = 0;
  std::cout << "Reading File " << file1 << "..."/*<< std::endl*/;
  while(fread((void*)&ev742, sizeof(ev742), 1, in1) == 1)
  {
    for(int i = 0 ; i < 4 ; i++)
    {
      ev742.TTT[i] = ev742.TTT[i] + delta740to742_0;
    }
    input742_0.push_back(ev742);
    counter++;
  }
//   std::cout << std::endl;
  std::cout << "done" << std::endl;

  counter = 0;
  std::cout << "Reading File " << file2 << "..."/*<< std::endl*/;
  while(fread((void*)&ev742, sizeof(ev742), 1, in2) == 1)
  {
    for(int i = 0 ; i < 4 ; i++)
    {
      ev742.TTT[i] = ev742.TTT[i] + delta740to742_1;
    }
    input742_1.push_back(ev742);
    counter++;
  }
//   std::cout << std::endl;
  std::cout << "done" << std::endl;


  TFile *fRoot = new TFile("histograms.root","RECREATE");

  double maxDelta = -INFINITY;
  double minDelta = INFINITY;

  double maxDeltaTTT = -INFINITY;
  double minDeltaTTT = INFINITY;

  double maxDelta740_742_0 = -INFINITY;
  double minDelta740_742_0 =  INFINITY;

  double maxDelta740_742_1 = -INFINITY;
  double minDelta740_742_1 =  INFINITY;

  double maxDeltaGroup = -INFINITY;
  double minDeltaGroup =  INFINITY;

  double maxDeltaBoard = -INFINITY;
  double minDeltaBoard =  INFINITY;

  //time binning of the 2 bords
  double Tt = 1000.0/(2.0*58.59375);
  double Ts = 16.0;

  //min events num for all files
  long long int events = input742_0.size();
  if(input742_0.size() < events)
    events = input742_0.size();
  if(input742_1.size() < events)
    events = input742_1.size();


  //looks for maxima and minima in order to have reasonable plots.
  //could be skipped...
  std::cout << "Looking for maxima and minima..." /*<< std::endl*/;
  counter = 0;
  for(int i = 0 ; i < events ; i++)
  {
    double diff = (input742_0[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[16])*200.0;
    if( diff < minDelta )
      minDelta = diff;
    if( diff > maxDelta )
      maxDelta = diff;

    double diffTTT = (input742_0[i].TTT[2] - input742_1[i].TTT[2]) ;
    if( diffTTT < minDeltaTTT )
      minDeltaTTT = diffTTT;
    if( diffTTT > maxDeltaTTT )
      maxDeltaTTT = diffTTT;

    double diff740_742_0 = (input740[i].TTT - input742_0[i].TTT[2]) ;
    if( diff740_742_0 < minDelta740_742_0 )
      minDelta740_742_0 = diff740_742_0;
    if( diff740_742_0 > maxDelta740_742_0 )
      maxDelta740_742_0 = diff740_742_0;

    double diff740_742_1 = (input740[i].TTT - input742_1[i].TTT[2]) ;
    if( diff740_742_1 < minDelta740_742_1 )
      minDelta740_742_1 = diff740_742_1;
    if( diff740_742_1 > maxDelta740_742_1 )
      maxDelta740_742_1 = diff740_742_1;

    double diffGroup = (input742_0[i].PulseEdgeTime[16] - input742_0[i].PulseEdgeTime[18])*200.0;
    if( diffGroup < minDeltaGroup )
      minDeltaGroup = diffGroup;
    if( diffGroup > maxDeltaGroup )
      maxDeltaGroup = diffGroup;

    double diffBoard = (input742_1[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[24])*200.0;
    if( diffBoard < minDeltaBoard )
      minDeltaBoard = diffBoard;
    if( diffBoard > maxDeltaBoard )
      maxDeltaBoard = diffBoard;

  }
  std::cout << "done" << std::endl;
  std::cout << std::endl;


  //creates histograms and graphs
  int h742_742_bins = (maxDeltaTTT - minDeltaTTT) / Tt;
  int h740_742_0_bins = (maxDelta740_742_0-minDelta740_742_0) / Tt;
  int h740_742_1_bins = (maxDelta740_742_1-minDelta740_742_1) / Tt;

  TH1F *hRes = new TH1F("hRes","hRes",((int) (maxDelta-minDelta)),minDelta,maxDelta);
  TH1F *hResSameGroup = new TH1F("hResSameGroup","hResSameGroup",((int) (maxDeltaGroup-minDeltaGroup)),minDeltaGroup,maxDeltaGroup);
  TH1F *hResSameBoard = new TH1F("hResSameBoard","hResSameBoard",((int) (maxDeltaBoard-minDeltaBoard)),minDeltaBoard,maxDeltaBoard);

  TH1F *h742_742 = new TH1F("h742_742","h742_742",20,minDeltaTTT,maxDeltaTTT);
  TH1F *h740_742_0 = new TH1F("h740_742_0","h740_742_0",20,minDelta740_742_0,maxDelta740_742_0);
  TH1F *h740_742_1 = new TH1F("h740_742_1","h740_742_1",20,minDelta740_742_1,maxDelta740_742_1);

  TH1F *sorted_h742_742 = new TH1F("sorted_h742_742","sorted_h742_742",20,minDeltaTTT,maxDeltaTTT);
  TH1F *sorted_h740_742_0 = new TH1F("sortedh740_742_0","sorted_h740_742_0",20,minDelta740_742_0,maxDelta740_742_0);
  TH1F *sorted_h740_742_1 = new TH1F("sorted_h740_742_1","sorted_h740_742_1",20,minDelta740_742_1,maxDelta740_742_1);

  TH2F *TTT742 = new TH2F("TTT742","TTT742",1000,0,1e12,1000,0,1e12);

  std::vector<double> t , t2, delta740_742_0,delta740_742_1,delta742_742;
  std::vector<double> sorted_t , sorted_t2, sorted_delta740_742_0,sorted_delta740_742_1,sorted_delta742_742;
  std::cout << "Producing plots ..." /*<< std::endl*/;

  int counterMatch = 0;
  // do general histograms/graphs and at the same time perform the event sorting scheme
  FILE *eventsFile;
  eventsFile = fopen("events.dat","wb");

  long long int index742_0 = 0;
  long long int index742_1 = 0;


  events = input742_0.size();

  for(int i = 0 ; i < events ; i++)
  {
    EventFormat_t event;

    event.TTT740 = input740[i].TTT;  //put the TTT of 740


    for(int j = 0 ; j < 64 ; j++)
    {
      event.Charge[j] = input740[i].Charge[j];  // put the charges
      event.PulseEdgeTime[j] = 0;  // set the times to 0 for now
    }

    bool foundMatch0 = false;
    bool foundMatch1 = false;

    // search matching event on first 742 board...
    for(int k = -misDepth ; k < misDepth ; k++)  //run on this and the next misDepth-1 events, looking for a match
    {
      if( ((index742_0+k) >= events) | ((index742_0+k) < 0)  ) // stop if you are the the end of the dataset, otherwise it's gonna be seg fault...
        continue;
      if( fabs(input740[i].TTT - input742_0[index742_0+k].TTT[2]) > tolerance*sigmaT ) // if the event is not within the desired time window, skip to the next in the for loop.
        continue;
      else        //match found, copy the values and set foundMatch0 to true
      {
	event.TTT742_0 = input742_0[i].TTT[0];  //put the TTT of 742_0
        for(int j = 0 ; j < 32 ; j++)
        {
          event.PulseEdgeTime[j] = input742_0[index742_0+k].PulseEdgeTime[j];
        }
        foundMatch0 = true;
	index742_0 = index742_0+k;
        break;    // if the match has been found, stop the for loop
      }
    }

    // search a matching event on second 742 board...
    for(int k = -misDepth ; k < misDepth ; k++)
    {
      if( ((index742_1+k) >= events) | ((index742_1+k) < 0) ) // stop if you are the the end of the dataset, otherwise it's gonna be seg fault...
        continue;
      if( fabs(input740[i].TTT - input742_1[index742_1+k].TTT[2]) > tolerance*sigmaT ) // if the event is not within the desired time window, skip to the next in the for loop.
        continue;
      else        //match found, copy the values and set foundMatch1 to true
      {
	event.TTT742_1 = input742_1[i].TTT[0];  //put the TTT of 742_1
        for(int j = 32 ; j < 64 ; j++)
        {
          event.PulseEdgeTime[j] = input742_1[index742_1+k].PulseEdgeTime[j-32];
        }
        foundMatch1 = true;
	index742_1 = index742_1+k;
        break;    // if the match has been found, stop the for loop
      }
    }

    if(foundMatch0 && foundMatch1) // if the matching events on both the 742 boards have been found, save the global event to file
    {
      fwrite(&event,sizeof(event),1,eventsFile);
      sorted_h742_742->Fill  (( event.TTT742_0 - event.TTT742_1));
      sorted_h740_742_0->Fill(( event.TTT740 - event.TTT742_0     ));
      sorted_h740_742_1->Fill(( event.TTT740 - event.TTT742_1     ));
      if( (i % (input740.size() / 200) ) == 0) // roughly just 200 points per TGraph
      {
        sorted_t.push_back(event.TTT740);
        sorted_t2.push_back(event.TTT742_0);

        sorted_delta740_742_0.push_back(event.TTT740 - event.TTT742_0);
        sorted_delta740_742_1.push_back(event.TTT740 - event.TTT742_1);
        sorted_delta742_742.push_back(event.TTT742_0 - event.TTT742_1);
      }


      counterMatch++;
    }
    else  // otherwise skip this event and warn the user
    {
      std::cout << "No matching event for sample " << i  << "\t"  << std::fixed << std::showpoint<< std::setprecision(2)<< input740[i].TTT << "\t" << input742_0[i].TTT[2] << "\t" << input742_1[i].TTT[2] << std::endl;
    }


    // fill histograms and vectors for graphs
    TTT742->Fill( input742_0[i].TTT[2] , input742_1[i].TTT[2] );

    hRes->Fill( (input742_0[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[16])*200.0 );
    hResSameGroup->Fill( (input742_0[i].PulseEdgeTime[16] - input742_0[i].PulseEdgeTime[18])*200.0 );
    hResSameBoard->Fill( (input742_1[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[24])*200.0 );

    h742_742->Fill  (( input742_0[i].TTT[2] - input742_1[i].TTT[2]));
    h740_742_0->Fill(( input740[i].TTT - input742_0[i].TTT[2]     ));
    h740_742_1->Fill(( input740[i].TTT - input742_1[i].TTT[2]     ));
    if( (i % (input740.size() / 200) ) == 0) // roughly just 200 points per TGraph
    {
      t.push_back(input740[i].TTT);
      t2.push_back(input742_0[i].TTT[2]);

      delta740_742_0.push_back(input740[i].TTT - input742_0[i].TTT[2]);
      delta740_742_1.push_back(input740[i].TTT - input742_1[i].TTT[2]);
      delta742_742.push_back(input742_0[i].TTT[2] - input742_1[i].TTT[2]);
    }

  }

  fclose(eventsFile);
  std::cout << "done" << std::endl;
  std::cout <<std::endl<< "Matching events found " << counterMatch << std::endl;

  TF1 *gauss2 = new TF1("gauss2","gaus",minDelta,maxDelta);
  TF1 *gauss1 = new TF1("gauss1","gaus",minDeltaBoard,maxDeltaBoard);
  TF1 *gauss0 = new TF1("gauss0","gaus",minDeltaGroup,maxDeltaGroup);

  hResSameGroup->Fit("gauss0","Q");
  hResSameBoard->Fit("gauss1","Q");
  hRes->Fit("gauss2","Q");

  std::cout << std::endl;

  std::cout << "CTR FWHM same group         = " <<  gauss0->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM same board         = " <<  gauss1->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM different boards   = " <<  gauss2->GetParameter(2) * 2.355 << "ps" << std::endl;

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

  TGraph* g_delta742_742 = new TGraph(t2.size(),&t2[0],&delta742_742[0]);
  g_delta742_742->SetName("Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  g_delta742_742->SetTitle("Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  g_delta742_742->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta742_742->GetYaxis()->SetTitle("delta [ns]");


  TGraph* sorted_g_delta740_742_0 = new TGraph(sorted_t.size(),&sorted_t[0],&sorted_delta740_742_0[0]);
  sorted_g_delta740_742_0->SetName("sorted_Trigger Time Tag delta vs. time (V1740D - V1742_0) ");
  sorted_g_delta740_742_0->SetTitle("sorted_Trigger Time Tag delta vs. time (V1740D - V1742_0) ");
  sorted_g_delta740_742_0->GetXaxis()->SetTitle("Acquisition time [ns]");
  sorted_g_delta740_742_0->GetYaxis()->SetTitle("delta [ns]");

  TGraph* sorted_g_delta740_742_1 = new TGraph(sorted_t.size(),&sorted_t[0],&sorted_delta740_742_1[0]);
  sorted_g_delta740_742_1->SetName("sorted_Trigger Time Tag delta vs. time (V1740D - V1742_1) ");
  sorted_g_delta740_742_1->SetTitle("sorted_Trigger Time Tag delta vs. time (V1740D - V1742_1) ");
  sorted_g_delta740_742_1->GetXaxis()->SetTitle("Acquisition time [ns]");
  sorted_g_delta740_742_1->GetYaxis()->SetTitle("delta [ns]");

  TGraph* sorted_g_delta742_742 = new TGraph(sorted_t2.size(),&sorted_t2[0],&sorted_delta742_742[0]);
  sorted_g_delta742_742->SetName("sorted_Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  sorted_g_delta742_742->SetTitle("sorted_Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  sorted_g_delta742_742->GetXaxis()->SetTitle("Acquisition time [ns]");
  sorted_g_delta742_742->GetYaxis()->SetTitle("delta [ns]");

  std::cout << t.size() << " "
            << t2.size() << " "
	    << sorted_t.size() << " "
	    << sorted_t2.size() << " " << std::endl;

  //close everything
  fclose(in0);
  fclose(in1);
  fclose(in2);

  fast740_742_0->Write();
  fast740_742_1->Write();

  hResSameGroup->Write();
  hResSameBoard->Write();
  hRes->Write();

  h742_742->Write();
  h740_742_0->Write();
  h740_742_1->Write();

  sorted_h742_742->Write();
  sorted_h740_742_0->Write();
  sorted_h740_742_1->Write();

  TTT742->Write();
  g_delta742_742->Write();
  g_delta740_742_0->Write();
  g_delta740_742_1->Write();
  sorted_g_delta742_742->Write();
  sorted_g_delta740_742_0->Write();
  sorted_g_delta740_742_1->Write();
  fRoot->Close();
  return 0;
}
