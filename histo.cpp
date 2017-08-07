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
  
  while(in0 >> temp_input740)
  {
    input740.push_back(temp_input740);
    temp_input740.clear();
  }
  
  while(in1 >> temp_input742_0)
  {
    input742_0.push_back(temp_input742_0);
    temp_input742_0.clear();
  }
  
  while(in2 >> temp_input742_1)
  {
    input742_1.push_back(temp_input742_1);
    temp_input742_1.clear();
  }
  
  
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
  
  
  TFile *fRoot = new TFile("test.root","RECREATE");
  
  double maxDelta = -INFINITY;
  double minDelta = INFINITY;
  
  double maxDeltaTTT = -INFINITY;
  double minDeltaTTT = INFINITY;
  
  double maxDelta740_742_0 = -INFINITY;
  double minDelta740_742_0 =  INFINITY;
  
  double maxDelta740_742_1 = -INFINITY;
  double minDelta740_742_1 =  INFINITY;
  
  for(int i = 0 ; i < input742_0.size() ; i++)
  {
    double diff = (input742_0[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[16])*200.0;
    if( diff < minDelta )
      minDelta = diff;
    if( diff > maxDelta )
      maxDelta = diff;
    
    double diffTTT = (input742_0[i].TTT[2] - input742_1[i].TTT[2]);
    if( diffTTT < minDeltaTTT )
      minDeltaTTT = diffTTT;
    if( diffTTT > maxDeltaTTT )
      maxDeltaTTT = diffTTT;
    
    double diff740_742_0 = (input740[i].TTT - input742_0[i].TTT[2]);
    if( diff740_742_0 < minDelta740_742_0 )
      minDelta740_742_0 = diff740_742_0;
    if( diff740_742_0 > maxDelta740_742_0 )
      maxDelta740_742_0 = diff740_742_0;
    
    double diff740_742_1 = (input740[i].TTT - input742_1[i].TTT[2]);
    if( diff740_742_1 < minDelta740_742_1 )
      minDelta740_742_1 = diff740_742_1;
    if( diff740_742_1 > maxDelta740_742_1 )
      maxDelta740_742_1 = diff740_742_1;
  }
  
  
  // std::cout << "Max delta T between V1740D and V1742_1 = " <<
  int misaligned = 0;
  TH1F *hRes = new TH1F("hRes","hRes",100,minDelta,maxDelta);
  TH1F *h742_742 = new TH1F("h742_742","h742_742",100,minDeltaTTT,maxDeltaTTT);
  TH1F *h740_742_0 = new TH1F("h740_742_0","h740_742_0",100,minDelta740_742_0,maxDelta740_742_0);
  TH1F *h740_742_1 = new TH1F("h740_742_1","h740_742_1",100,minDelta740_742_1,maxDelta740_742_1);
  
  TH2F *TTT742 = new TH2F("TTT742","TTT742",1000,0,1e12,1000,0,1e12);
  
  std::vector<double> t , delta740_742_0,delta740_742_1,delta742_742;
  
  for(int i = 0 ; i < input742_0.size() ; i++)
  {
    TTT742->Fill( input742_0[i].TTT[2] , input742_1[i].TTT[2] );
    if( fabs(input742_0[i].TTT[2] - input742_1[i].TTT[2]) > 200.0 )
    {
      misaligned++;
    }
    else
    {
      hRes->Fill( (input742_0[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[16])*200.0 );
      h742_742->Fill( input742_0[i].TTT[2] - input742_1[i].TTT[2] );
      h740_742_0->Fill( input740[i].TTT - input742_0[i].TTT[2] );
      h740_742_1->Fill( input740[i].TTT - input742_1[i].TTT[2] );
      if( (i % (input740.size() / 200) ) == 0) // roughly just 200 points per TGraph
      {
        t.push_back(input740[i].TTT);
        delta740_742_0.push_back(input740[i].TTT - input742_0[i].TTT[2]);
        delta740_742_1.push_back(input740[i].TTT - input742_1[i].TTT[2]);
        delta742_742.push_back(input742_0[i].TTT[2] - input742_1[i].TTT[2]);
      }
      
    }
    
  }
  
  TGraph* g_delta740_742_0 = new TGraph(t.size(),&t[0],&delta740_742_0[0]);
  g_delta740_742_0->SetTitle("Trigger Time Tag delta vs. time (V1740D - V1742_0) ");
  g_delta740_742_0->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta740_742_0->GetYaxis()->SetTitle("delta [ns]");
  
  TGraph* g_delta740_742_1 = new TGraph(t.size(),&t[0],&delta740_742_1[0]);
  g_delta740_742_1->SetTitle("Trigger Time Tag delta vs. time (V1740D - V1742_1) ");
  g_delta740_742_1->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta740_742_1->GetYaxis()->SetTitle("delta [ns]");
  
  TGraph* g_delta742_742 = new TGraph(t.size(),&t[0],&delta742_742[0]);
  g_delta742_742->SetTitle("Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  g_delta742_742->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta742_742->GetYaxis()->SetTitle("delta [ns]");
  // gr->Draw("AC*");
  
  
  std::cout << "misaligned = " << misaligned << std::endl;
  
  in0.close();
  in1.close();
  in2.close();
  
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
