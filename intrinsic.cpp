// compile with
// g++ -o bin/intrinsic intrinsic.cpp `root-config --cflags --glibs` -Wl,--no-as-needed -lHist -lCore -lMathCore -lTree -lTreePlayer -lSpectrum

// small program to extract intrinsic resolution

#include "TROOT.h"
#include "TFile.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH3I.h"
#include "TString.h"
#include "TApplication.h"
#include "TLegend.h"
#include "TTree.h"
#include "TF2.h"
#include "TGraph2D.h"
#include "TGraph.h"
#include "TSpectrum.h"
#include "TSpectrum2.h"
#include "TTreeFormula.h"
#include "TMath.h"
#include "TChain.h"
#include "TCut.h"
#include "TLine.h"
#include "TError.h"
#include "TEllipse.h"
#include "TFormula.h"
#include "TGraphErrors.h"
#include "TGraph2DErrors.h"
#include "TMultiGraph.h"
#include "TCutG.h"
#include "TGaxis.h"
#include "TPaveStats.h"
#include "TProfile.h"
#include "TH1D.h"
#include "TPaveText.h"
#include "TGraphDelaunay.h"
#include "TVector.h"
#include "TNamed.h"
#include "TPaveLabel.h"
#include "THStack.h"
#include "TFitResult.h"
#include "TImage.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <getopt.h>
#include <algorithm>    // std::sort
#include <iomanip>      // std::setw

#include <sys/types.h>
#include <dirent.h>


// typedef std::vector<std::string> stringvec;
// list files in directory
// taken from
// http://www.martinbroadhurst.com/list-the-files-in-a-directory-in-c.html
void read_directory(const std::string& name, std::vector<std::string> &v)
{
    DIR* dirp = opendir(name.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL) {
        v.push_back(dp->d_name);
    }
    closedir(dirp);
}


void usage()
{
  std::cout << "--input       =" << " "<< std::endl;
  std::cout << "--output      =" << " "<< std::endl;
  std::cout << "--text        =" << " "<< std::endl;
  std::cout << "--daqCH1      =" << " "<< std::endl;
  std::cout << "--daqCH2      =" << " "<< std::endl;
  std::cout << "--ch1min      =" << " "<< std::endl;
  std::cout << "--ch2min      =" << " "<< std::endl;
  std::cout << "--ch1max      =" << " "<< std::endl;
  std::cout << "--ch2max      =" << " "<< std::endl;
  std::cout << "--ch1bins     =" << " "<< std::endl;
  std::cout << "--ch2bins     =" << " "<< std::endl;
  std::cout << "--sigmas      =" << " "<< std::endl;
  std::cout << "--percDown    =" << " "<< std::endl;
  std::cout << "--percUp      =" << " "<< std::endl;
  std::cout << "--ENres       =" << " "<< std::endl;
  std::cout << "--tStampMin   =" << " "<< std::endl;
  std::cout << "--tStampMax   =" << " "<< std::endl;
  std::cout << "--ctrBins     =" << " "<< std::endl;
  std::cout << "--ctrMin      =" << " "<< std::endl;
  std::cout << "--ctrMax      =" << " "<< std::endl;


}


TCut find_limits(TH1F *h_chA,
                 int chA,
                 int type,
                 float min,
                 float max,
                 float sigmas,
                 float percDown,
                 float percUp,
                 float ENres)
{
  std::stringstream sname;
  std::string var;
  if(type == 0)
  {
    var = "ch";
  }
  else
  {
    var = "t";
  }
  TSpectrum *spectrumCH2;
  spectrumCH2 = new TSpectrum(5);
  h_chA->GetXaxis()->SetRangeUser(min,max);
  Int_t peaksN = spectrumCH2->Search(h_chA,1,"goff",0.5);
  Double_t *PeaksCH2  = spectrumCH2->GetPositionX();
  Double_t *PeaksYCH2 = spectrumCH2->GetPositionY();
  float maxPeak = 0.0;
  int peakID = 0;
  for (int peakCounter = 0 ; peakCounter < peaksN ; peakCounter++ )
  {
    if(PeaksYCH2[peakCounter] > maxPeak)
    {
      maxPeak = PeaksYCH2[peakCounter];
      peakID = peakCounter;
    }
  }
  TF1 *gaussCH2 = new TF1("gaussCH2","gaus");
  gaussCH2->SetParameter(0,PeaksYCH2[peakID]); // heigth
  gaussCH2->SetParameter(1,PeaksCH2[peakID]); //mean
  gaussCH2->SetParameter(2,(ENres*PeaksCH2[peakID])/2.355); // sigma
  h_chA->Fit(gaussCH2,"Q","",PeaksCH2[peakID] - (PeaksCH2[peakID] * percDown),PeaksCH2[peakID] + (PeaksCH2[peakID] * percUp));

  // sname << "ch" << chA << " > " << PeaksCH2[peakID] - (sigmas * gaussCH2->GetParameter(2)) << "&& ch" << chA << " < " << PeaksCH2[peakID] + (sigmas * gaussCH2->GetParameter(2)) ;
  sname << var << chA << " > " << gaussCH2->GetParameter(1) - (sigmas * gaussCH2->GetParameter(2)) << "&& " << var << chA << " < " << gaussCH2->GetParameter(1) + (sigmas * gaussCH2->GetParameter(2)) ;
  TCut peakch2 = sname.str().c_str();
  if(type == 0)
  {
    h_chA->GetXaxis()->SetRangeUser(0,65535);
  }
  else
  {
    h_chA->GetXaxis()->SetRangeUser(-0.2e-6,0.2e-6);
  }


  return peakch2;
}



int main(int argc, char** argv)
{
  if(argc < 2)
  {
    std::cout << argv[0] << std::endl;
    usage();
    return 1;
  }

  gStyle->SetOptFit(1111111);

  //default values
  std::string inputFileName = "TTree_";
  std::string outputFileName = "intrinsic_plots_";
  std::string textFileName = "intrinsic_text_";
  int   daqCH1 = 0;
  int   daqCH2 = 1;
  float ch1min = 0;
  float ch2min = 0;
  float ch1max = 65535;
  float ch2max = 65535;
  float ch1bins = 1000;
  float ch2bins = 1000;
  float sigmas = 2.0;
  float percDown = 0.04;
  float percUp = 0.06;
  float ENres = 0.10;
  float tStampMin = -61e-9;
  float tStampMax = -52e-9;
  int ctrBins = 100;
  float ctrMin = -1e-9;
  float ctrMax = 1e-9;
  float t_sigmas= 2.0;
  float t_percDown= 0.03;
  float t_percUp= 0.04;
  float t_ENres= 0.05;


  static struct option longOptions[] =
  {
      { "input", required_argument, 0, 0 },
      { "output", required_argument, 0, 0 },
      { "text", required_argument, 0, 0 },
      { "daqCH1", required_argument, 0, 0 },
      { "daqCH2", required_argument, 0, 0 },
      { "ch1min", required_argument, 0, 0 },
      { "ch2min", required_argument, 0, 0 },
      { "ch1max", required_argument, 0, 0 },
      { "ch2max", required_argument, 0, 0 },
      { "ch1bins", required_argument, 0, 0 },
      { "ch2bins", required_argument, 0, 0 },
      { "sigmas", required_argument, 0, 0 },
      { "percDown", required_argument, 0, 0 },
      { "percUp", required_argument, 0, 0 },
      { "ENres", required_argument, 0, 0 },
      { "tStampMin", required_argument, 0, 0 },
      { "tStampMax", required_argument, 0, 0 },
      { "ctrBins", required_argument, 0, 0 },
      { "ctrMin", required_argument, 0, 0 },
      { "ctrMax", required_argument, 0, 0 },
      { "t_sigmas", required_argument, 0, 0 },
      { "t_percDown", required_argument, 0, 0 },
      { "t_percUp", required_argument, 0, 0 },
      { "t_ENres", required_argument, 0, 0 },
      { NULL, 0, 0, 0 }
  };

  while(1)
  {
    int optionIndex = 0;
    int c = getopt_long(argc, argv, "i:o:t:", longOptions, &optionIndex);
    if (c == -1)
    {
      break;
    }
    if (c == 'i')
    {
      inputFileName = (char *)optarg;
    }
    else if (c == 'o'){
      outputFileName = (char *)optarg;
    }
    else if (c == 't'){
      textFileName = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 0){
      inputFileName = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 1){
      outputFileName = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 2){
      textFileName = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 3){
      daqCH1 = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 4){
      daqCH2 = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 5){
      ch1min = atof((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 6){
      ch2min = atof((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 7){
      ch1max = atof((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 8){
      ch2max = atof((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 9){
      ch1bins = atof((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 10){
      ch2bins = atof((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 11){
      sigmas = atof((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 12){
      percDown = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 13){
      percUp = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 14){
      ENres = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 15){
      tStampMin = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 16){
      tStampMax = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 17){
      ctrBins = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 18){
      ctrMin = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 19){
      ctrMax = atof((char *)optarg);
    }

  }

  std::stringstream s_file;
  s_file << outputFileName << "_" << daqCH1 << "_" << daqCH2 << ".root";
  std::stringstream t_file;
  t_file << textFileName << "_" << daqCH1 << "_" << daqCH2 << ".txt";

  //feedback
  std::cout << "------------------------" << std::endl;
  std::cout << "|   PARAMETERS         |" << std::endl;
  std::cout << "------------------------" << std::endl;
  std::cout << std::endl;
  std::cout << "prefix of input  = " << inputFileName  << std::endl;
  std::cout << "outputFileName   = " << s_file.str()   << std::endl;
  std::cout << "textFileName     = " << t_file.str()   << std::endl;
  std::cout << "daqCH1           = " << daqCH1         << std::endl;
  std::cout << "daqCH2           = " << daqCH2         << std::endl;
  std::cout << "ch1min           = " << ch1min         << std::endl;
  std::cout << "ch2min           = " << ch2min         << std::endl;
  std::cout << "ch1max           = " << ch1max         << std::endl;
  std::cout << "ch2max           = " << ch2max         << std::endl;
  std::cout << "ch1bins          = " << ch1bins        << std::endl;
  std::cout << "ch2bins          = " << ch2bins        << std::endl;

  //open output files
  TFile *outputFile = new TFile(s_file.str().c_str(),"RECREATE");
  std::ofstream textfile;
  textfile.open (t_file.str().c_str(),std::ofstream::out);

  //open input files
  std::vector<std::string> v;
  read_directory(".", v);
  // extract files with correct prefix
  std::vector<std::string> listInputFiles;
  // std::string inputFileName = "TTree_";
  for(unsigned int i = 0 ; i < v.size() ; i++)
  {
    if(!v[i].compare(0,inputFileName.size(),inputFileName))
    {
      listInputFiles.push_back(v[i]);
    }
  }

  TChain *tree = new TChain("adc"); // read input files
  std::cout << "Adding files... " << std::endl;
  for(unsigned int i = 0 ; i < listInputFiles.size() ; i++)
  {
    std::cout << listInputFiles[i] << std::endl;
    tree->Add(listInputFiles[i].c_str());
  }
  std::cout << "... done." << std::endl;

  //charge histos
  //daqCH1
  std::stringstream sname;
  sname << "histo_ch" << daqCH1;
  TH1F *h_ch1 = new TH1F(sname.str().c_str(),sname.str().c_str(),ch1bins,0,65535);
  sname << "_highlight";
  TH1F *h_ch1_highlight = new TH1F(sname.str().c_str(),sname.str().c_str(),ch1bins,0,65535);
  sname.str("");

  //daqCH1
  sname << "histo_ch" << daqCH2;
  TH1F *h_ch2 = new TH1F(sname.str().c_str(),sname.str().c_str(),ch2bins,0,65535);
  sname << "_highlight";
  TH1F *h_ch2_highlight = new TH1F(sname.str().c_str(),sname.str().c_str(),ch2bins,0,65535);
  sname.str("");

  //draw histos
  sname << "ch" << daqCH1 << " >> " << "histo_ch" << daqCH1;
  tree->Draw(sname.str().c_str());
  sname.str("");
  sname << "ch" << daqCH2 << " >> " << "histo_ch" << daqCH2;
  tree->Draw(sname.str().c_str());
  sname.str("");

  TCut cutCharge1 = find_limits(h_ch1,daqCH1,0,ch1min,ch1max,sigmas,percDown,percUp,ENres);
  TCut cutCharge2 = find_limits(h_ch2,daqCH2,0,ch2min,ch2max,sigmas,percDown,percUp,ENres);


  //draw highlights

  sname << "ch" << daqCH1 << " >> " << "histo_ch" << daqCH1 << "_highlight";
  tree->Draw(sname.str().c_str(),cutCharge1);
  h_ch1_highlight->SetLineColor(kGreen);
  h_ch1_highlight->SetFillColor(kGreen);
  h_ch1_highlight->SetFillStyle(3001);
  sname.str("");
  sname << "ch" << daqCH2 << " >> " << "histo_ch" << daqCH2 << "_highlight";
  tree->Draw(sname.str().c_str(),cutCharge1);
  h_ch2_highlight->SetLineColor(kGreen);
  h_ch2_highlight->SetFillColor(kGreen);
  h_ch2_highlight->SetFillStyle(3001);
  sname.str("");



  //draw t spectra
  //daqCH1
  sname << "histo_t" << daqCH1;
  TH1F *h_t1 = new TH1F(sname.str().c_str(),sname.str().c_str(),4000,-0.2e-6,0.2e-6);
  sname << "_highlight";
  TH1F *h_t1_highlight = new TH1F(sname.str().c_str(),sname.str().c_str(),4000,-0.2e-6,0.2e-6);
  h_t1_highlight->SetLineColor(kGreen);
  h_t1_highlight->SetFillColor(kGreen);
  h_t1_highlight->SetFillStyle(3001);
  sname.str("");

  //daqCH2
  sname << "histo_t" << daqCH2;
  TH1F *h_t2 = new TH1F(sname.str().c_str(),sname.str().c_str(),4000,-0.2e-6,0.2e-6);
  sname << "_highlight";
  TH1F *h_t2_highlight = new TH1F(sname.str().c_str(),sname.str().c_str(),4000,-0.2e-6,0.2e-6);
  h_t2_highlight->SetLineColor(kGreen);
  h_t2_highlight->SetFillColor(kGreen);
  h_t2_highlight->SetFillStyle(3001);
  sname.str("");

  //draw t histos
  sname << "t" << daqCH1 << " >> " << "histo_t" << daqCH1;
  tree->Draw(sname.str().c_str());

  sname.str("");
  sname << "t" << daqCH2 << " >> " << "histo_t" << daqCH2;
  tree->Draw(sname.str().c_str());
  sname.str("");



  //TCut cutTime1 = find_limits(h_t1,daqCH1,1,tStampMin,tStampMax,t_sigmas,t_percDown,t_percUp,t_ENres);
  //TCut cutTime2 = find_limits(h_t1,daqCH2,1,tStampMin,tStampMax,t_sigmas,t_percDown,t_percUp,t_ENres);



  sname << "t" << daqCH1 << ">" << tStampMin << " && " << "t" << daqCH1 << "<" << tStampMax;
  TCut cutTime1 = sname.str().c_str();
  sname.str("");
  sname << "t" << daqCH2 << ">" << tStampMin << " && " << "t" << daqCH2 << "<" << tStampMax;
  TCut cutTime2 = sname.str().c_str();
  sname.str("");

  //draw highlights

  sname << "t" << daqCH1 << " >> " << "histo_t" << daqCH1 << "_highlight";
  tree->Draw(sname.str().c_str(),cutTime1);
  sname.str("");
  sname << "t" << daqCH2 << " >> " << "histo_t" << daqCH2 << "_highlight";
  tree->Draw(sname.str().c_str(),cutTime2);
  sname.str("");











  //TCut cutCharge1 = find_limits(h_ch1,daqCH1,ch1min,ch1max,sigmas,percDown,percUp,ENres);
  //TCut cutCharge2 = find_limits(h_ch2,daqCH2,ch2min,ch2max,sigmas,percDown,percUp,ENres);




  sname << "t" << daqCH1 << "!= 0 && t" << daqCH2 << "!= 0" ;
  TCut noZeros = sname.str().c_str();
  sname.str("");
  
  //dummy histo just to get the range 
  TH1F *dummy = new TH1F("dummy","dummy",ctrBins,ctrMin,ctrMax);
  
  sname << "t" << daqCH2 << "- t" << daqCH1 << " >> dummy";
  tree->Draw(sname.str().c_str(),cutCharge1+cutCharge2+cutTime1+cutTime2+noZeros);
  sname.str("");
  
  float dummy_mean  = dummy->GetMean();
  float dummy_rms   = dummy->GetRMS();
  float ctrHistoMin = dummy_mean - 10*dummy_rms;
  float ctrHistoMax = dummy_mean + 10*dummy_rms;
  
  
  
  TH1F *ctr_histo = new TH1F("ctr_histo","Time resolution histogram",500,ctrHistoMin,ctrHistoMax);
  sname << "t" << daqCH2 << "- t" << daqCH1 << " >> ctr_histo";
  tree->Draw(sname.str().c_str(),cutCharge1+cutCharge2+cutTime1+cutTime2+noZeros);
  sname.str("");

  TF1 *gaussFunc = new TF1("gaussFunc","gaus");
  ctr_histo->Fit(gaussFunc,"Q");
  std::cout << "intrinsic resolution FWHM [ps] = "<< gaussFunc->GetParameter(2)*2.355*1e12 << std::endl;



  textfile << "# ch1 ch2 CTR FWHM [ps] " << std::endl;
  textfile << daqCH1 << " " << daqCH2 << " " << gaussFunc->GetParameter(2)*2.355*1e12<< std::endl;

//   sname << "c_h" << chA;
//   TCanvas *c_hA = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
//   sname.str("");


  TCanvas *c1 = new TCanvas("charge1","charge1",1200,800);
  TCanvas *c2 = new TCanvas("charge2","charge2",1200,800);
  c1->cd();
  h_ch1->Draw("HIST");
  h_ch1_highlight->Draw("HIST same");
  c2->cd();
  h_ch2->Draw("HIST");
  h_ch2_highlight->Draw("HIST same");

  TCanvas *ct1 = new TCanvas("time1","time1",1200,800);
  TCanvas *ct2 = new TCanvas("time2","time2",1200,800);
  ct1->cd();
  h_t1->Draw("HIST");
  h_t1_highlight->Draw("HIST same");
  ct2->cd();
  h_t2->Draw("HIST");
  h_t2_highlight->Draw("HIST same");


  TCanvas *c_ctr = new TCanvas("ctr","ctr",1200,800);
  ctr_histo->Draw();
  sname << "ctr_" << daqCH2 << "_" << daqCH1 << ".png";
  c_ctr->SaveAs(sname.str().c_str());
  sname.str("");





  outputFile->cd();
  c_ctr->Write();
  ctr_histo->Write();

  c1->Write();
  ct1->Write();

  h_ch1->Write();
  h_ch1_highlight->Write();
  h_t1->Write();
  h_t1_highlight->Write();

  c2->Write();
  ct2->Write();
  h_ch2->Write();
  h_ch2_highlight->Write();
  h_t2->Write();
  h_t2_highlight->Write();

  outputFile->Close();
  textfile.close();
  //close output files

  return 0;
}
