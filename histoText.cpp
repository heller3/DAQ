// g++ -o histo histo.cpp `root-config --cflags --glibs`

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

// class of input from 742 digi
class input742_t
{
public:
  int groups;
  int chPerGroup;
  
  input742_t(int a,int b){ groups = a; chPerGroup = b;};
  
  std::vector<double> TTT;
  std::vector<double> PulseEdgeTime;
  
  void clear()
  {
    TTT.clear();
    PulseEdgeTime.clear();
  };
  friend std::istream& operator>>(std::istream& input, input742_t& s)
  {
    for(int i = 0 ; i < s.groups ; i++)
    {
      double x;
      input >> x;
      s.TTT.push_back(x);
      for(int j = 0 ; j < s.chPerGroup ; j++)
      {
        double y;
        input >> y;
        s.PulseEdgeTime.push_back(y);
      }
    }
    return input;
  }
};



class input740_t
{
public:
  int groups;
  int chPerGroup;
  
  input740_t(int a,int b){ groups = a; chPerGroup = b;};
  
  double TTT;
  std::vector<uint16_t> Charge;
  void clear()
  {
    TTT = 0;
    Charge.clear();
  };
  friend std::istream& operator>>(std::istream& input, input740_t& s)
  {
    input >> s.TTT;
    for(int i = 0 ; i < s.groups ; i++)
    {
      for(int j = 0 ; j < s.chPerGroup ; j++)
      {
        uint16_t x;
        input >> x;
        s.Charge.push_back(x);
      }
    }
    return input;
  }
};



//----------------//
//  MAIN PROGRAM  //
//----------------//
int main(int argc,char **argv)
{
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
  ifstream in0,in1,in2;
  //  std::vector<input742_t> input742;
  input740_t temp_input740(8,8);
  input742_t temp_input742_0(4,8);
  input742_t temp_input742_1(4,8);
  std::vector<input740_t> input740;
  std::vector<input742_t> input742_0;
  std::vector<input742_t> input742_1;
  
  
  in0.open(file0,std::ios::in);
  in1.open(file1,std::ios::in);
  in2.open(file2,std::ios::in);
  
  long long int counter = 0;
  std::cout << "Reading File " << file0 << "..." << std::endl;
  while(in0 >> temp_input740)
  {
    input740.push_back(temp_input740);
    temp_input740.clear();
    counter++;
    if(counter == 1000000) break;
    if((counter % 100000) == 0) std::cout << "\r" << counter << std::flush ;
  }
  std::cout << std::endl;
  std::cout << " done" << std::endl;
  
  counter = 0;
  std::cout << "Reading File " << file1 << "..."<< std::endl;
  while(in1 >> temp_input742_0)
  {
    input742_0.push_back(temp_input742_0);
    temp_input742_0.clear();
    
    counter++;
    if(counter == 1000000) break;
    if((counter % 100000) == 0) std::cout << "\r" << counter << std::flush ;
  }
  std::cout << std::endl;
  std::cout << " done" << std::endl;
  
  counter = 0;
  std::cout << "Reading File " << file2 << "..."<< std::endl;
  while(in2 >> temp_input742_1)
  {
    input742_1.push_back(temp_input742_1);
    temp_input742_1.clear();
    counter++;
    if(counter == 1000000) break;
    if((counter % 100000) == 0) std::cout << "\r" << counter << std::flush ;
  }
  std::cout << std::endl;
  std::cout << " done" << std::endl;
  
  //  //DEBUG input
  //  for(int i = 0 ; i < input740.size() ; i++)
  //  {
  //    std::cout << input740[i].TTT << " ";
  //    for(int j = 0 ; j < input740[i].Charge.size() ; j++)
  //    {
  //      std::cout << input740[i].Charge[j]<< " " ;
  //
  //    }
  //    std::cout << std::endl;
  //  }
  //
  //  std::cout << "-----------------------------------" <<  std::endl;
  //  //DEBUG input
  //  for(int i = 0 ; i < input742_1.size() ; i++)
  //  {
  //    for(int gr = 0 ; gr < input742_1[i].groups ; gr++)
  //    {
  //       std::cout << input742_1[i].TTT[gr] << " ";
  //      for(int ch = 0 ; ch < input742_1[i].chPerGroup ; ch++)
  //      {
  //        std::cout << input742_1[i].PulseEdgeTime[gr * input742_1[i].chPerGroup + ch] << " ";
  //      }
  //    }
  //    std::cout << std::endl;
  //  }
  
  
  TFile *fRoot = new TFile("testText.root","RECREATE");
  
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
  
  double Tt = 1000.0/(2.0*58.59375);
  double Ts = 16.0;
  
  std::cout << "Looking for maxs ..." << std::endl;
  counter = 0;
  
  
  //min events num for all
  long long int events = input742_0.size();
  if(input742_0.size() < events) 
    events = input742_0.size();
  if(input742_1.size() < events) 
    events = input742_1.size();
  
       
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
    
    counter++;
    if((counter % 100000) == 0) std::cout << "\r" << counter << std::flush ;
    
    
  }
  std::cout << std::endl;
  std::cout << " done" << std::endl;
  std::cout << minDeltaTTT << " " << maxDeltaTTT <<std::endl;
  
  // std::cout << "Max delta T between V1740D and V1742_1 = " <<
  int misaligned = 0;
  
  
  int h742_742_bins = (maxDeltaTTT - minDeltaTTT) / Tt;
  int h740_742_0_bins = (maxDelta740_742_0-minDelta740_742_0) / Tt;
  int h740_742_1_bins = (maxDelta740_742_1-minDelta740_742_1) / Tt;
  
  
  TH1F *hRes = new TH1F("hRes","hRes",((int) (maxDelta-minDelta)),minDelta,maxDelta);
  TH1F *hResSameGroup = new TH1F("hResSameGroup","hResSameGroup",((int) (maxDeltaGroup-minDeltaGroup)),minDeltaGroup,maxDeltaGroup);
  TH1F *hResSameBoard = new TH1F("hResSameBoard","hResSameBoard",((int) (maxDeltaBoard-minDeltaBoard)),minDeltaBoard,maxDeltaBoard);
  
  TH1F *h742_742 = new TH1F("h742_742","h742_742",20,minDeltaTTT,maxDeltaTTT);
  TH1F *h740_742_0 = new TH1F("h740_742_0","h740_742_0",20,minDelta740_742_0,maxDelta740_742_0);
  TH1F *h740_742_1 = new TH1F("h740_742_1","h740_742_1",20,minDelta740_742_1,maxDelta740_742_1);
  
  TH2F *TTT742 = new TH2F("TTT742","TTT742",1000,0,1e12,1000,0,1e12);
  
  std::vector<double> t , t2, delta740_742_0,delta740_742_1,delta742_742;
  
  std::cout << "Producing plots ..." << std::endl;
  counter = 0;
  
                                      
  
  
  for(int i = 0 ; i < events ; i++)
  {
    TTT742->Fill( input742_0[i].TTT[2] , input742_1[i].TTT[2] );
    if( fabs(input742_0[i].TTT[2] - input742_1[i].TTT[2]) > 200.0 )
    {
      misaligned++;
    }
    
    
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
//       std::cout << input742_0[i].TTT[2] - input742_1[i].TTT[2] << std::endl;
    }
    
    
    counter++;
    if((counter % 100000) == 0) std::cout << "\r" << counter << std::flush ;
    
  }
  std::cout << std::endl;
  std::cout << " done" << std::endl;
  
  
  TF1 *gauss2 = new TF1("gauss2","gaus",minDelta,maxDelta);
  TF1 *gauss1 = new TF1("gauss1","gaus",minDeltaBoard,maxDeltaBoard);
  TF1 *gauss0 = new TF1("gauss0","gaus",minDeltaGroup,maxDeltaGroup);
  
  hResSameGroup->Fit("gauss0");
  hResSameBoard->Fit("gauss1");
  hRes->Fit("gauss2");
  
  std::cout << std::endl;
  std::cout << std::endl;
  
  std::cout << "CTR FWHM same group         = " <<  gauss0->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM same board         = " <<  gauss1->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM different boards   = " <<  gauss2->GetParameter(2) * 2.355 << "ps" << std::endl;
  
  TGraph* g_delta740_742_0 = new TGraph(t.size(),&t[0],&delta740_742_0[0]);
  g_delta740_742_0->SetTitle("Trigger Time Tag delta vs. time (V1740D - V1742_0) ");
  g_delta740_742_0->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta740_742_0->GetYaxis()->SetTitle("delta [ns]");
  
  TGraph* g_delta740_742_1 = new TGraph(t.size(),&t[0],&delta740_742_1[0]);
  g_delta740_742_1->SetTitle("Trigger Time Tag delta vs. time (V1740D - V1742_1) ");
  g_delta740_742_1->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta740_742_1->GetYaxis()->SetTitle("delta [ns]");
  
  TGraph* g_delta742_742 = new TGraph(t2.size(),&t2[0],&delta742_742[0]);
  g_delta742_742->SetTitle("Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  g_delta742_742->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta742_742->GetYaxis()->SetTitle("delta [ns]");
  // gr->Draw("AC*");
  
  
  std::cout << "misaligned = " << misaligned << std::endl;
  
  in0.close();
  in1.close();
  in2.close();
  
  hResSameGroup->Write();
  hResSameBoard->Write();
  hRes->Write();
  
  h742_742->Write();
  h740_742_0->Write();
  h740_742_1->Write();
  
  TTT742->Write();
  g_delta742_742->Write();
  g_delta740_742_0->Write();
  g_delta740_742_1->Write();
  
  
  // gr->Write();
  fRoot->Close();
  return 0;
}
