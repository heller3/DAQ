// compile with
// g++ -o bin/thScan thScan.cpp `root-config --cflags --glibs` -Wl,--no-as-needed -lHist -lCore -lMathCore -lTree -lTreePlayer -lSpectrum

// small program to extract timing calibration and data

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


template <typename T>
void extractCTR(T* histo,double fitPercMin,double fitPercMax, int divs, double tagFwhm, double* res, double* fitRes)
{
  //first, dummy gaussian fit
  TCanvas *cTemp  = new TCanvas("temp","temp");
  TF1 *gaussDummy = new TF1("gaussDummy","gaus");
  histo->Fit(gaussDummy,"QN");

  double f1min = histo->GetXaxis()->GetXmin();
  double f1max = histo->GetXaxis()->GetXmax();
  // std::cout << f1min << " " << f1max << std::endl;
  TF1* f1  = new TF1("f1","crystalball");
  f1->SetLineColor(kRed);
  f1->SetParameters(gaussDummy->GetParameter(0),gaussDummy->GetParameter(1),gaussDummy->GetParameter(2),1,3);
  double fitMin = gaussDummy->GetParameter(1) - fitPercMin*(gaussDummy->GetParameter(2));
  double fitMax = gaussDummy->GetParameter(1) + fitPercMax*(gaussDummy->GetParameter(2));
  if(fitMin < f1min)
  {
    fitMin = f1min;
  }
  if(fitMax > f1max)
  {
    fitMax = f1max;
  }
  histo->Fit(f1,"Q","",fitMin,fitMax);
  double min,max,min10,max10;
  // int divs = 3000;
  double step = (f1max-f1min)/divs;
  double funcMax = f1->GetMaximum(fitMin,fitMax);
  for(int i = 0 ; i < divs ; i++)
  {
    if( (f1->Eval(f1min + i*step) < funcMax/2.0) && (f1->Eval(f1min + (i+1)*step) > funcMax/2.0) )
    {
      min = f1min + (i+0.5)*step;
    }
    if( (f1->Eval(f1min + i*step) > funcMax/2.0) && (f1->Eval(f1min + (i+1)*step) < funcMax/2.0) )
    {
      max = f1min + (i+0.5)*step;
    }
    if( (f1->Eval(f1min + i*step) < funcMax/10.0) && (f1->Eval(f1min + (i+1)*step) > funcMax/10.0) )
    {
      min10 = f1min + (i+0.5)*step;
    }
    if( (f1->Eval(f1min + i*step) > funcMax/10.0) && (f1->Eval(f1min + (i+1)*step) < funcMax/10.0) )
    {
      max10 = f1min + (i+0.5)*step;
    }
  }
  res[0] = sqrt(2)*sqrt(pow((max-min),2)-pow(tagFwhm,2));
  res[1] = sqrt(2)*sqrt(pow((max10-min10),2)-pow((tagFwhm/2.355)*4.29,2));

  fitRes[0] = f1->GetChisquare();
  fitRes[1] = f1->GetNDF();
  fitRes[2] = f1->GetProb();
  fitRes[3] = f1->GetParameter(1);
  fitRes[4] = f1->GetParError(1);
  // std::cout << f1->GetChisquare()/f1->GetNDF() << std::endl;
  delete cTemp;
}



void usage()
{
  std::cout << "\t\t" << "[-i|--input] <file_prefix>  [-o|--output] <plots> [-t|--text] text [OPTIONS]" << std::endl
            << "\t\t" << "<file_prefix>                                      - prefix of TTree files to analyze                                  - default = TTree_"   << std::endl
            << "\t\t" << "<plots>                                       - output file prefix                                                  - default = plots"   << std::endl
            << "\t\t" << "<text>                                         - txt file prefix (where ctr result will be stored)                  - default = text"   << std::endl
            << "\t\t" << "--chAnum                                           - number of first charge channel                                           - default = 12 " << std::endl
            << "\t\t" << "--chAmin                                           - min in chA charge histogram [ADC channels]          - default = 0 " << std::endl
            << "\t\t" << "--chAmax                                           - max in chA charge histogram [ADC channels]          - default = max of TTree files " << std::endl
            << "\t\t" << "--chAbins                                          - bins in chA charge histogram                        - default = 65535 " << std::endl
            
            << "\t\t" << "--chBnum                                           - number of second charge channel                                          - default = 32 " << std::endl
            << "\t\t" << "--chBmin                                           - min in chB charge histogram [ADC channels]          - default = 0 " << std::endl
            << "\t\t" << "--chBmax                                           - max in chB charge histogram [ADC channels]          - default = 65535 " << std::endl
            << "\t\t" << "--chBbins                                          - bins in chB charge histogram                        - default = 1000 " << std::endl
            
            
            << "\t\t" << "--tAnum                                            - number of first time channel                                           - default = chAnum " << std::endl 
            << "\t\t" << "--tBnum                                            - number of second time channel                                          - default = chBnum " << std::endl
            << "\t\t" << "--percDown                                         - lower limit of photopeak fit, in fraction of photopeak (1 = 100%) - default = 0.04 " << std::endl
            << "\t\t" << "--percUp                                           - upper limit of photopeak fit, in fraction of photopeak (1 = 100%) - default = 0.06" << std::endl
            << "\t\t" << "--EnRes                                            - expected Energy resoluton of photopeaks (1 = 100%)                - default = 0.1  " << std::endl
            << "\t\t" << "--sigmas                                           - width of photopeak cuts in sigmas                                 - default = 2.0 " << std::endl
            << "\t\t" << "--howManyRMS                                       - lower and upper limits of ctr fit are peak +/- howManyRMS*rms     - default = 1.0 " << std::endl
            << "\t\t" << "--ctrMin                                           - lower limit of ctr histogram                                      - default = -1e-9 " << std::endl
            << "\t\t" << "--ctrMax                                           - upper limit of ctr histogram                                      - default = 1e-9 " << std::endl
            << "\t\t" << "--ctrBins                                          - bins in ctr histogram                                             - default = 500 " << std::endl
            << "\t\t" << "--profile                                          - profile = 0 -> sliced acq, = 1 cont acq                           - default = 0 " << std::endl
            << "\t\t" << "--binning                                          - approx. time length of ctrVsTime bins in sec                      - default = 600 " << std::endl
            << "\t\t" << "--fitPercMin <value>                               - time fit min is set to ((gauss fit mean) - fitPercMin*(gauss fit sigma))  - default = 5"  << std::endl
            << "\t\t" << "--fitPercMax <value>                               - time fit max is set to ((gauus fit mean) - fitPercMax*(gauss fit sigma))  - default = 6" << std::endl
            << "\t\t" << "--divs <value>                                     - n of divisions when looking for FWHM - default = 10000"  << std::endl
            << "\t\t" << "--bins <value>                                     - n of bins in summary CTR histograms - deafult 40"  << std::endl
            << "\t\t" << "--tagFwhm <value>                                  - FWHM timing resolution of reference board, in sec - default = 70e-12"  << std::endl
            << "\t\t" << "--func <value>                                     - function for fitting, 0 = gauss, 1 = crystalball and sub of tagFwhm, then multipl by sqrt(2) (default = 0)"  << std::endl
            << "\t\t" << "--start-time <value>                               - start time to fill ctr plot [h]"  << std::endl
            << "\t\t" << std::endl
            << "\t\t" << "--end-time <value>                                 - end time to fill ctr plot [h]"  << std::endl
            << "\t\t" << "--crystal <value>                                 - crystal number [h]"  << std::endl
            << "\t\t" << std::endl;
}


int main(int argc, char** argv)
{
  if(argc < 2)
  {
    std::cout << argv[0] << std::endl;
    usage();
    return 1;
  }

  std::string inputFileName = "TTree_";
  std::string outputFileName = "plots.root";
  std::string textFileName = "text.txt";
  int chA = 12;
  int chB = 32;
  int tA = 12;
  int tB = 32;
  
  float chAmin = 0;
  float chBmin = 0;
  float chAmax = 65535;
  float chBmax = 65535;
  float chAbins = 1000;
  float chBbins = 1000;
  
  
  double sigmas = 2.0;
  double percDown = 0.04;
  double percUp = 0.06;
  double ENres = 0.10;
  double howManyRMS = 1.0;
  double ctrMin = -1e-9;
  double ctrMax = 1e-9;
  int    ctrBins = 500;
  bool tAgiven = false;
  bool tBgiven = false;
  int prof = 0;
  double binning = 600;
  Float_t fitPercMin = 5;
  Float_t fitPercMax = 6;
  int divs       = 10000;
  int bins = 40;  
  Float_t tagFwhm = 70.0e-12; //s
  int func = 0;
  double start_time = 0;
  double end_time = 0;
  bool start_time_given = false;
  bool end_time_given = false;
  int crystal = 0;
  

  static struct option longOptions[] =
  {
      { "input", required_argument, 0, 0 },
      { "output", required_argument, 0, 0 },
      { "text", required_argument, 0, 0 },
      { "chAnum", required_argument, 0, 0 },
      { "chBnum", required_argument, 0, 0 },
      { "sigmas", required_argument, 0, 0 },
      { "percDown", required_argument, 0, 0 },
      { "percUp", required_argument, 0, 0 },
      { "ENres", required_argument, 0, 0 },
      { "howManyRMS", required_argument, 0, 0 },
      { "ctrMin", required_argument, 0, 0 },
      { "ctrMax", required_argument, 0, 0 },
      { "ctrBins", required_argument, 0, 0 },
      { "tAnum", required_argument, 0, 0 },
      { "tBnum", required_argument, 0, 0 },
      { "chAmin", required_argument, 0, 0 },
      { "chAmax", required_argument, 0, 0 },
      { "chAbins", required_argument, 0, 0 },
      { "chBmin", required_argument, 0, 0 },
      { "chBmax", required_argument, 0, 0 },
      { "chBbins", required_argument, 0, 0 },  
      { "profile", required_argument, 0, 0 },  
      { "binning", required_argument, 0, 0 }, 
      { "fitPercMin", required_argument, 0, 0 },
      { "fitPercMax", required_argument, 0, 0 },
      { "divs", required_argument, 0, 0 },
      { "bins", required_argument, 0, 0 },
      { "tagFwhm", required_argument, 0, 0 },
      { "func", required_argument, 0, 0 },
      { "start-time", required_argument, 0, 0 },
      { "end-time", required_argument, 0, 0 },
      { "crystal", required_argument, 0, 0 },
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
      if (c == 'i'){
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
      chA = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 4){
      chB = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 5){
      sigmas = atof((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 6){
      percDown = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 7){
      percUp = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 8){
      ENres = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 9){
      howManyRMS = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 10){
      ctrMin = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 11){
      ctrMax = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 12){
      ctrBins = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 13){
      tAgiven = true;
      tA = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 14){
      tBgiven = true;
      tB = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 15){
      chAmin = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 16){
      chAmax = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 17){
      chAbins = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 18){
      chBmin = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 19){
      chBmax = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 20){
      chBbins = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 21){
      prof = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 22){
      binning = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 23){
      fitPercMin = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 24){
      fitPercMax = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 25){
      divs = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 26){
      bins = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 27){
      tagFwhm = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 28){
      func = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 29){
      start_time_given = true;
      start_time = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 30){
      end_time_given = true;
      end_time = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 31){
      crystal = atoi((char *)optarg);
    }
    else {
      std::cout	<< "Usage: " << argv[0] << std::endl;
      usage();
      return 1;
    }
  }
  
  //temporary correction //FIXME 
  //p0 + p1*x
  float p0 = -1.097e-9;
  float p1 = -1.246e-11;
  
  float yCenter = p0 + 2.0*p1;
  
 
  
  
  if(!tAgiven)
  {
    tA = chA;
  }
  if(!tBgiven)
  {
    tB = chB;
  }
  
  //chose appropriate time tag 
  std::string timeTag = "";
  if(prof == 0)
  {
    timeTag = "ExtendedTimeTag";
  }
  else 
  {
    timeTag = "DeltaTimeTag";
    
  }
  
//   std::cout << timeCut << std::endl;
  
  std::cout << std::endl;
  std::cout << "Channel    Charge ch      Time ch " << std::endl;
  std::cout << "   A    " << chA << "   " << tA << std::endl;
  std::cout << "   B    " << chB << "   " << tB << std::endl;
  std::cout << std::endl;

  gStyle->SetOptFit(1111111);
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
  
  double tStart = tree->GetMinimum(timeTag.c_str())/(1e9*3600);
  double tEnd   = (tree->GetMaximum(timeTag.c_str()) - tree->GetMinimum(timeTag.c_str()))/(1e9*3600);
  
  TCut timeCut;
  
  if(start_time_given)
  {
    std::stringstream ssCut;
    ssCut << " ((" << timeTag << " )/(1e9*3600) - "<< tStart << ")" << " >= " << start_time;
    timeCut += ssCut.str().c_str();
  }
  if(end_time_given)
  {
    std::stringstream ssCut;
    ssCut << " ((" << timeTag << " )/(1e9*3600) - "<< tStart << ")" << " <= " << end_time;
    timeCut += ssCut.str().c_str();
  }
  
  std::stringstream s_file;
  s_file << outputFileName << "_" << crystal << ".root";
//   outputFileName = outputFileName + "_" + itoa(crystal) + ".root";
  TFile *outputFile = new TFile(s_file.str().c_str(),"RECREATE");
  double ret[2];
  double fitRes[5];

    
  std::stringstream sname;
  sname << "t_" << tA;
  TH1F *t_chA = new TH1F(sname.str().c_str(),sname.str().c_str(),1000,-0.15e-6,0.05e-6);
  sname.str("");

//   sname << "c_" << chA;
//   TCanvas *c_tA = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
//   sname.str("");
  sname << "t" << tA << " >> " << "t_" << tA;
  tree->Draw(sname.str().c_str());
  sname.str("");

  sname << "t_" << tB;
  TH1F *t_chB = new TH1F(sname.str().c_str(),sname.str().c_str(),1000,-0.15e-6,0.05e-6);
  sname.str("");

//   sname << "c_" << chB;
//   TCanvas *c_tB = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
//   sname.str("");
  sname << "t" << tB << " >> " << "t_" << tB;
  tree->Draw(sname.str().c_str());
  sname.str("");

  // TH1F *t_ch9 = new TH1F("t_ch9","t_ch9",1000,-0.15e-6,0.05e-6);
  // TCanvas *c_t9 = new TCanvas("c_t9","c_t9",1200,800);
  // tree->Draw("t9 >> t_ch9");


  sname << "h_ch" << chA;
  TH1F *h_chA = new TH1F(sname.str().c_str(),sname.str().c_str(),chAbins,chAmin,chAmax);
  sname << "_highlight";
  TH1F *h_chA_highlight = new TH1F(sname.str().c_str(),sname.str().c_str(),chAbins,chAmin,chAmax);
  sname.str("");

//   sname << "c_h" << chA;
//   TCanvas *c_hA = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
//   sname.str("");
  sname << "ch" << chA << " >> " << "h_ch" << chA;
  tree->Draw(sname.str().c_str(),timeCut);
  sname.str("");
  

  sname << "h_ch" << chB;
  TH1F *h_chB = new TH1F(sname.str().c_str(),sname.str().c_str(),chBbins,chBmin,chBmax);
  sname << "_highlight";
  TH1F *h_chB_highlight = new TH1F(sname.str().c_str(),sname.str().c_str(),chBbins,chBmin,chBmax);
  sname.str("");

//   sname << "c_h" << chB;
//   TCanvas *c_hB = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
//   sname.str("");
  sname << "ch" << chB << " >> " << "h_ch" << chB;
  tree->Draw(sname.str().c_str(),timeCut);
  sname.str("");
  

  //---------------------------//
  // find cuts                 //
  //---------------------------//
  //ch2
//   c_hA->cd();
  TSpectrum *spectrumCH2;
  spectrumCH2 = new TSpectrum(5);
  Int_t peaksN = spectrumCH2->Search(h_chA,1,"",0.5);
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

  sname << "ch" << chA << " > " << PeaksCH2[peakID] - (sigmas * gaussCH2->GetParameter(2)) << "&& ch" << chA << " < " << PeaksCH2[peakID] + (sigmas * gaussCH2->GetParameter(2)) ;
  TCut peakch2 = sname.str().c_str();
  sname.str("");
  
//   std::cout << peakch2 std::endl;
  sname << "ch" << chA << " >> " << "h_ch" << chA << "_highlight";
  tree->Draw(sname.str().c_str(),timeCut+peakch2);
  sname.str("");
  
  

  //ch9
//   c_hB->cd();
  TSpectrum *spectrumCH9;
  spectrumCH9 = new TSpectrum(5);
  Int_t peaksN9 = spectrumCH9->Search(h_chB,1,"",0.5);
  Double_t *PeaksCH9  = spectrumCH9->GetPositionX();
  Double_t *PeaksYCH9 = spectrumCH9->GetPositionY();
  float maxPeak9 = 0.0;
  int peakID9 = 0;
  for (int peakCounter = 0 ; peakCounter < peaksN9 ; peakCounter++ )
  {
    if(PeaksYCH9[peakCounter] > maxPeak9)
    {
      maxPeak9 = PeaksYCH9[peakCounter];
      peakID9 = peakCounter;
    }
  }
  TF1 *gaussCH9 = new TF1("gaussCH9","gaus");
  gaussCH9->SetParameter(0,PeaksYCH9[peakID9]); // heigth
  gaussCH9->SetParameter(1,PeaksCH9[peakID9]); //mean
  gaussCH9->SetParameter(2,(ENres*PeaksCH9[peakID9])/2.355); // sigma
  h_chB->Fit(gaussCH9,"Q","",PeaksCH9[peakID9] - (PeaksCH9[peakID9] * percDown),PeaksCH9[peakID9] + (PeaksCH9[peakID9] * percUp));
  // std::stringstream sname;
  sname << "ch" << chB << " > " << PeaksCH9[peakID9] - (sigmas * gaussCH9->GetParameter(2)) << "&& ch" << chB << " < " << PeaksCH9[peakID9] + (sigmas * gaussCH9->GetParameter(2)) ;
  TCut peakch9 = sname.str().c_str();
  sname.str("");
  
//   std::cout << peakch9 std::endl;
  sname << "ch" << chB << " >> " << "h_ch" << chB << "_highlight";
  tree->Draw(sname.str().c_str(),timeCut+peakch9);
  sname.str("");
  
  //avoid zeroes!
  sname << "t" << tA << " != 0 && t" << tB <<" != 0";
  TCut noZeros = sname.str().c_str();
  sname.str("");
  
  
  
  //---------------------------//
  // ctr plot                  //
  //---------------------------//
  
  
  
  
  
  
  
  
  
  TH1F *ctr = new TH1F("ctr","ctr",ctrBins,ctrMin,ctrMax);
  ctr->GetXaxis()->SetTitle("t_b - t_a [s]");
  TCanvas *c_ht = new TCanvas("c_ht","c_ht",1200,800);
  sname << "t" << tB << " - t" << tA <<" >> ctr";
  tree->Draw(sname.str().c_str(),peakch2+peakch9+noZeros+timeCut);
  sname.str("");
  
  TSpectrum *spectrumCTR;
  spectrumCTR = new TSpectrum(5);
  Int_t peaksNCTR = spectrumCTR->Search(ctr,1,"",0.5);
  Double_t *PeaksCTR  = spectrumCTR->GetPositionX();
  Double_t *PeaksYCTR = spectrumCTR->GetPositionY();
  float maxPeakCTR = 0.0;
  int peakIDCTR = 0;
  for (int peakCounter = 0 ; peakCounter < peaksNCTR ; peakCounter++ )
  {
    if(PeaksYCTR[peakCounter] > maxPeakCTR)
    {
      maxPeakCTR = PeaksYCTR[peakCounter];
      peakIDCTR = peakCounter;
    }
  }
  double peakPos = PeaksCTR[peakIDCTR];
  double rmsCTR = ctr->GetRMS();
  

  TF1 *gaussCTR = new TF1("gaussCTR","gaus");
  ctr->Fit(gaussCTR,"Q","",peakPos - howManyRMS*rmsCTR,peakPos + howManyRMS*rmsCTR);

  Double_t chi2 = gaussCTR->GetChisquare();
  Int_t NDF = gaussCTR->GetNDF();
  Double_t chi2red = chi2 / NDF;
  
  if(func == 1)
  {
    extractCTR(ctr,fitPercMin,fitPercMax,divs,tagFwhm,ret,fitRes);
  }
  
//   std::cout << ret[0]*1e12 << "\t"
//             << ret[1]*1e12 << std::endl;
  
  
  
  //ctr corr
  std::stringstream var;
  var << "(t" << tB << " - t" << tA << ") + (" << yCenter << " - (((( " << timeTag << " )/(1e9*3600) - "<< tStart << "))* "<< p1 << ") + " << p0 << ") >> ctrCorr" ;
  std::cout << var.str() << std::endl;
  
  TH1F *ctrCorr = new TH1F("ctrCorr","ctrCorr",ctrBins,-4e-9,-2e-9);
  ctrCorr->GetXaxis()->SetTitle("t_b - t_a [s]");
  TCanvas *c_ht_corr = new TCanvas("c_ht_corr","c_ht_corr",1200,800);
//   sname << "t" << tB << " - t" << tA <<" >> c_ht_corr";
  tree->Draw(var.str().c_str(),peakch2+peakch9+noZeros+timeCut);
//   sname.str("");
  
  TSpectrum *spectrumCTR_corr;
  spectrumCTR_corr = new TSpectrum(5);
  Int_t peaksNCTR_corr = spectrumCTR_corr->Search(ctrCorr,1,"",0.5);
  Double_t *PeaksCTR_corr  = spectrumCTR_corr->GetPositionX();
  Double_t *PeaksYCTR_corr = spectrumCTR_corr->GetPositionY();
  float maxPeakCTR_corr = 0.0;
  int peakIDCTR_corr = 0;
  for (int peakCounter = 0 ; peakCounter < peaksNCTR_corr ; peakCounter++ )
  {
    if(PeaksYCTR_corr[peakCounter] > maxPeakCTR_corr)
    {
      maxPeakCTR_corr = PeaksYCTR_corr[peakCounter];
      peakIDCTR_corr = peakCounter;
    }
  }
  double peakPos_corr = PeaksCTR_corr[peakIDCTR_corr];
  double rmsCTR_corr = ctrCorr->GetRMS();
  

  TF1 *gaussCTR_corr = new TF1("gaussCTR_corr","gaus");
  ctrCorr->Fit(gaussCTR_corr,"Q","",peakPos_corr - howManyRMS*rmsCTR_corr,peakPos_corr + howManyRMS*rmsCTR_corr);

  Double_t chi2_corr = gaussCTR_corr->GetChisquare();
  Int_t NDF_corr = gaussCTR_corr->GetNDF();
  Double_t chi2red_corr = chi2_corr / NDF_corr;
  var.str("");
  double ret_corr[2];
  double fitRes_corr[5];
  
  if(func == 1)
  {
    extractCTR(ctrCorr,fitPercMin,fitPercMax,divs,tagFwhm,ret_corr,fitRes_corr);
  }
  
  
  std::cout << "Corr " << ret_corr[0]*1e12 << "\t"
            << ret_corr[1]*1e12 << std::endl;
  
//   std::cout << "CTR FWHM [ps]\tCTR error [ps]\tChi^2/NDF\tPeak A Pos [ADC]\tEn Res A [FWHM]\tPeak B Pos [ADC]\tEn Res B [FWHM] " << std::endl;
//     std::cout << std::setw(20);
  
  std::ofstream textfile;
   std::stringstream s_file2;
  s_file2 << textFileName << "_" << crystal << ".txt";
//   outputFileName = outputFileName + "_" + itoa(crystal) + ".root";
//   TFile *outputFile = new TFile(s_file.str().c_str(),"RECREATE");
  textfile.open (s_file2.str().c_str(),std::ofstream::out);
  
  if(func == 0)
  {
    std::cout << "CTR FWHM [ps]\tCTR error [ps]\tChi^2/NDF\tPeak A Pos [ADC]\tEn Res A [FWHM]\tPeak B Pos [ADC]\tEn Res B [FWHM]\t Entries" << std::endl;
    std::cout << gaussCTR->GetParameter(2)*2.355*1e12 << "\t" 
             << gaussCTR->GetParError(2)*2.355*1e12 << "\t"  
            << chi2red                   << "\t"  
            << gaussCH2->GetParameter(1) << "\t"  
            << (gaussCH2->GetParameter(2)*2.355)/gaussCH2->GetParameter(1) << "\t"
            << gaussCH9->GetParameter(1) << "\t"
            << (gaussCH9->GetParameter(2)*2.355)/gaussCH9->GetParameter(1) << "\t"
            << ctr->GetEntries() << "\t"
            << std::endl;

    textfile << "#CTR FWHM [ps]\tCTR error [ps]\tChi^2/NDF\tPeak A Pos [ADC]\tEn Res A [FWHM]\tPeak B Pos [ADC]\tEn Res B [FWHM]\t Entries" << std::endl;

    textfile << gaussCTR->GetParameter(2)*2.355*1e12 << "\t" 
           << gaussCTR->GetParError(2)*2.355*1e12 << "\t"  
           << chi2red                   << "\t"  
           << gaussCH2->GetParameter(1) <<  "\t"
           << (gaussCH2->GetParameter(2)*2.355)/gaussCH2->GetParameter(1) << "\t"
           << gaussCH9->GetParameter(1) <<  "\t"
           << (gaussCH9->GetParameter(2)*2.355)/gaussCH9->GetParameter(1) << "\t"
           << ctr->GetEntries() << "\t"
           << std::endl;
  
  }
  else
  {
    std::cout << "CTR FWHM [ps]\tCTR error [ps]\tChi^2/NDF\tPeak A Pos [ADC]\tEn Res A [FWHM]\tPeak B Pos [ADC]\tEn Res B [FWHM]\t Entries" << std::endl;
    std::cout << ret[0]*1e12 << "\t" 
              << 0 << "\t"  
              << fitRes[0]/fitRes[1]                << "\t"  
              << gaussCH2->GetParameter(1) << "\t"  
              << (gaussCH2->GetParameter(2)*2.355)/gaussCH2->GetParameter(1) << "\t"
              << gaussCH9->GetParameter(1) << "\t"
              << (gaussCH9->GetParameter(2)*2.355)/gaussCH9->GetParameter(1) << "\t"
              << ctr->GetEntries() << "\t"
              << std::endl;
    
    textfile << "#CTR FWHM [ps]\tCTR error [ps]\tChi^2/NDF\tPeak A Pos [ADC]\tEn Res A [FWHM]\tPeak B Pos [ADC]\tEn Res B [FWHM]\t Entries" << std::endl;      
    textfile  << ret[0]*1e12 << "\t" 
              << 0 << "\t"  
              << fitRes[0]/fitRes[1]                << "\t"  
              << gaussCH2->GetParameter(1) << "\t"  
              << (gaussCH2->GetParameter(2)*2.355)/gaussCH2->GetParameter(1) << "\t"
              << gaussCH9->GetParameter(1) << "\t"
              << (gaussCH9->GetParameter(2)*2.355)/gaussCH9->GetParameter(1) << "\t"
              << ctr->GetEntries() << "\t"
              << std::endl;       
    
  }
  textfile.close();
  
  
  
  //draw ctr vs time tag
  
  
  
  
  //bins around every "binning" seconds 
  // tEnd is length in hours, so tEnd * (3600/binning) bins 
  int TenMinBins = (int) (tEnd * (3600/binning));
  double tPerBin = tEnd / TenMinBins;
  if(TenMinBins < 4)
  {
    TenMinBins = 4;
  }
  
//   std::cout << tStart << " " << tEnd << std::endl;
  
  var << "t" << tB << "- t" << tA << " :((" << timeTag << " )/(1e9*3600) - "<< tStart << ") >> ctrVsTime";
  TH2F *ctrVsTime = new TH2F("ctrVsTime","ctrVsTime",TenMinBins,0,tEnd,ctrBins,ctrMin,ctrMax);
  ctrVsTime->GetXaxis()->SetTitle("Acq. time [h]");
  ctrVsTime->GetYaxis()->SetTitle("t-t_tag [s]");
  tree->Draw(var.str().c_str(),peakch2+peakch9+noZeros+timeCut);
  var.str("");
//   double tStart2 = (tree->GetMinimum("DeltaTimeTag"))/(1e9*3600);
//   double tEnd2   = (tree->GetMaximum("DeltaTimeTag") - tree->GetMinimum("DeltaTimeTag"))/(1e9*3600);
//   
//   //bins around every 10 minutes 
//   // tEnd2 is length in hours, so tEnd2 * 6 bins 
//   std::cout << "tEnd2" << " " << tEnd2 << std::endl;
//   int TenMinBins2 = (int) (tEnd2 * (3600/binning));
// //   std::cout << "TenMinBins2" << " " << TenMinBins2 << std::endl;
//   if(TenMinBins2 < 4)
//   {
//     TenMinBins2 = 4;
//   }
// //   std::cout << "TenMinBins2" << " " << TenMinBins2 << std::endl;
//   
//   
//   TH2F *ctrVsDeltaTime = new TH2F("ctrVsDeltaTime","ctrVsDeltaTime",TenMinBins2,tStart2,tEnd2,ctrBins,ctrMin,ctrMax);
//   var << "t" << tB << "- t" << tA << " :(DeltaTimeTag )/(1e9*3600) - "<< tStart2 <<" >> ctrVsDeltaTime";
//   ctrVsDeltaTime->GetXaxis()->SetTitle("Acq. time [h]");
//   ctrVsDeltaTime->GetYaxis()->SetTitle("t-t_tag [s]");
//   tree->Draw(var.str().c_str(),peakch2+peakch9+noZeros);
//   var.str("");
  std::stringstream s_title;
  TProfile *profile;
  TCanvas *newCanvas = new TCanvas("newCanvas","newCanvas",1200,800);
  gStyle->SetOptStat(0);
  ctrVsTime->ProfileX();
  profile = (TProfile*) gDirectory->Get("ctrVsTime_pfx");  
  profile->GetYaxis()->SetTitle("CTR peak position [s]");
  profile->GetXaxis()->SetRangeUser(-1.2e-9,1.05e-9);
  s_title << "CTR peak position vs. acq time - crystal " << crystal;
  profile->SetTitle(s_title.str().c_str());
  s_title.str("");
  profile->Draw();
  gSystem->ProcessEvents();
  TImage *img = TImage::Create();
  img->FromPad(newCanvas);
  s_title << "ctrVsTime_" << crystal << ".png";
//   img->WriteImage(s_title.str().c_str());
  s_title.str("");
  
  
  

  
  //--------------------//
  //peak A vs time      //
  //--------------------//
  var << "ch" << chA << " :((" << timeTag << " )/(1e9*3600) - "<< tStart << ") >> peakAvsTime";
  TH2F *peakAvsTime = new TH2F("peakAvsTime","peakAvsTime",TenMinBins,0,tEnd,chAbins,chAmin,chAmax);
  tree->Draw(var.str().c_str());
  peakAvsTime->GetXaxis()->SetTitle("Acq. time [h]");
  peakAvsTime->GetYaxis()->SetTitle("[ADC channels]");
  std::vector<double> x_peakAvsTime;
  std::vector<double> y_peakAvsTime;
  std::vector<double> ex_peakAvsTime;
  std::vector<double> ey_peakAvsTime;
  
  for(int i = 0 ; i < TenMinBins; i++)
  {
    std::stringstream s_title_in;
    s_title_in << "projTag_"<< i;
    TString title = s_title_in.str().c_str();
    TH1D *projection = (TH1D*) peakAvsTime->ProjectionY(title,i+1,i+2);
    TSpectrum *spectrum_profile;
    spectrum_profile = new TSpectrum(5);
    Int_t peaksN_profile = spectrum_profile->Search(projection,1,"",0.5);
    Double_t *Peaks_profile  = spectrum_profile->GetPositionX();
    Double_t *PeaksY_profile = spectrum_profile->GetPositionY();
    float maxPeak_profile = 0.0;
    int peakID_profile = 0;
    for (int peakCounter = 0 ; peakCounter < peaksN_profile ; peakCounter++ )
    {
      if(PeaksY_profile[peakCounter] > maxPeak_profile)
      {
        maxPeak_profile = PeaksY_profile[peakCounter];
        peakID_profile = peakCounter;
      }
    }
    projection->GetXaxis()->SetRangeUser( Peaks_profile[peakID_profile] - (Peaks_profile[peakID_profile] * percDown),Peaks_profile[peakID_profile] + (Peaks_profile[peakID_profile] * percUp));
    extractCTR(projection,fitPercMin,fitPercMax,divs,tagFwhm,ret,fitRes);
    y_peakAvsTime.push_back(fitRes[3]);
    ey_peakAvsTime.push_back(fitRes[4]);
    x_peakAvsTime.push_back(peakAvsTime->GetXaxis()->GetBinCenter(i+1));
    ex_peakAvsTime.push_back(tPerBin/sqrt(12));
    
    outputFile->cd();
    projection->Write();
//     std::cout << fitRes[3] << "\n";
  }
//   std::cout << std::endl;
  TGraphErrors *graph_peakAvsTime = new TGraphErrors(x_peakAvsTime.size(),&x_peakAvsTime[0],&y_peakAvsTime[0],&ex_peakAvsTime[0],&ey_peakAvsTime[0]);
  graph_peakAvsTime->SetName("graph_peakAvsTime");
  s_title << "Tag peak position vs. acq. time - crystal " << crystal;
  graph_peakAvsTime->SetTitle(s_title.str().c_str());
  s_title.str("");
//   graph_peakAvsTime->SetTitle("Tag peak position vs. acq. time");
  graph_peakAvsTime->GetXaxis()->SetTitle("Time [h]");
  graph_peakAvsTime->GetYaxis()->SetTitle("[ADC channels]");
  outputFile->cd();
  graph_peakAvsTime->Write();
  TCanvas *newCanvas_chA = new TCanvas("newCanvas_chA","newCanvas_chA",1200,800);
  graph_peakAvsTime->Draw("AP");
  newCanvas_chA->cd();
  gSystem->ProcessEvents();
  TImage *img_chA = TImage::Create();
  img_chA->FromPad(newCanvas_chA);
  s_title << "peakTag_" << crystal << "_VsTime.png";
//   img_chA->WriteImage(s_title.str().c_str());
  peakAvsTime->GetYaxis()->SetRangeUser(chAmin,chAmax);
  s_title.str("");
  var.str("");
  
  
  
  
  //--------------------//
  //peak B vs time
  //--------------------//
  var << "ch" << chB << " :((" << timeTag << " )/(1e9*3600) - "<< tStart << ") >> peakBvsTime";
  TH2F *peakBvsTime = new TH2F("peakBvsTime","peakBvsTime",TenMinBins,0,tEnd,chBbins,chBmin,chBmax);
  peakBvsTime->GetXaxis()->SetTitle("Acq. time [h]");
  peakBvsTime->GetYaxis()->SetTitle("[ADC channels]");
  tree->Draw(var.str().c_str());
  
  std::vector<double> x_peakBvsTime;
  std::vector<double> y_peakBvsTime;
  std::vector<double> ex_peakBvsTime;
  std::vector<double> ey_peakBvsTime;
  
  for(int i = 0 ; i < TenMinBins; i++)
  {
    std::stringstream s_title_in;
    s_title_in << "projCry_"<< i;
    TString title = s_title_in.str().c_str();
    TH1D *projection = (TH1D*) peakBvsTime->ProjectionY(title,i+1,i+2);
    TSpectrum *spectrum_profile;
    spectrum_profile = new TSpectrum(5);
    Int_t peaksN_profile = spectrum_profile->Search(projection,1,"",0.5);
    Double_t *Peaks_profile  = spectrum_profile->GetPositionX();
    Double_t *PeaksY_profile = spectrum_profile->GetPositionY();
    float maxPeak_profile = 0.0;
    int peakID_profile = 0;
    for (int peakCounter = 0 ; peakCounter < peaksN_profile ; peakCounter++ )
    {
      if(PeaksY_profile[peakCounter] > maxPeak_profile)
      {
        maxPeak_profile = PeaksY_profile[peakCounter];
        peakID_profile = peakCounter;
      }
    }
    projection->GetXaxis()->SetRangeUser( Peaks_profile[peakID_profile] - (Peaks_profile[peakID_profile] * percDown),Peaks_profile[peakID_profile] + (Peaks_profile[peakID_profile] * percUp));
    extractCTR(projection,fitPercMin,fitPercMax,divs,tagFwhm,ret,fitRes);
    y_peakBvsTime.push_back(fitRes[3]);
    ey_peakBvsTime.push_back(fitRes[4]);
    x_peakBvsTime.push_back(peakBvsTime->GetXaxis()->GetBinCenter(i+1));
    ex_peakBvsTime.push_back(tPerBin/sqrt(12));
    
    outputFile->cd();
    projection->Write();
//     std::cout << fitRes[3] << "\n";
  }
//   std::cout << std::endl;
  TGraphErrors *graph_peakBvsTime = new TGraphErrors(x_peakBvsTime.size(),&x_peakBvsTime[0],&y_peakBvsTime[0],&ex_peakBvsTime[0],&ey_peakBvsTime[0]);
  graph_peakBvsTime->SetName("graph_peakBvsTime");
  s_title << "Crystal peak position vs. acq. time - crystal " << crystal;
  graph_peakBvsTime->SetTitle(s_title.str().c_str());
  s_title.str("");
//   graph_peakBvsTime->SetTitle("Crystal peak position vs. acq. time");
  graph_peakBvsTime->GetXaxis()->SetTitle("Time [h]");
  graph_peakBvsTime->GetYaxis()->SetTitle("[ADC channels]");
  outputFile->cd();
  graph_peakBvsTime->Write();
  TCanvas *newCanvas_chB = new TCanvas("newCanvas_chB","newCanvas_chB",1200,800);
  graph_peakBvsTime->Draw("AP");
  newCanvas_chB->cd();
  gSystem->ProcessEvents();
  TImage *img_chB = TImage::Create();
  img_chB->FromPad(newCanvas_chB);
  s_title << "peakCry_" << crystal << "_VsTime.png";
//   img_chB->WriteImage(s_title.str().c_str());
  peakBvsTime->GetYaxis()->SetRangeUser(chBmin,chBmax);
  s_title.str("");
  var.str("");
  
  
  //tA vs time
  var << "t" << tA << " :((" << timeTag << " )/(1e9*3600) - "<< tStart << ") >> tAvsTime";
  TH2F *tAvsTime = new TH2F("tAvsTime","tAvsTime",TenMinBins,0,tEnd,1000,-0.10e-6,0.05e-6);
  tAvsTime->GetXaxis()->SetTitle("Acq. time [h]");
  tAvsTime->GetYaxis()->SetTitle("[ADC channels]");
  tree->Draw(var.str().c_str(),peakch2+noZeros);
  
  std::vector<double>  x_tAvsTime;
  std::vector<double>  y_tAvsTime;
  std::vector<double> ex_tAvsTime;
  std::vector<double> ey_tAvsTime;
  
  for(int i = 0 ; i < TenMinBins; i++)
  {
    std::stringstream s_title_in;
    s_title_in << "projTtag_"<< i;
    TString title = s_title_in.str().c_str();
    TH1D *projection = (TH1D*) tAvsTime->ProjectionY(title,i+1,i+2);
    TSpectrum *spectrum_profile;
    spectrum_profile = new TSpectrum(5);
    Int_t peaksN_profile = spectrum_profile->Search(projection,1,"",0.5);
    Double_t *Peaks_profile  = spectrum_profile->GetPositionX();
    Double_t *PeaksY_profile = spectrum_profile->GetPositionY();
    float minPeak_profile = INFINITY;
    int peakID_profile = 0;
    for (int peakCounter = 0 ; peakCounter < peaksN_profile ; peakCounter++ )
    {
      if(Peaks_profile[peakCounter] < minPeak_profile)
      {
        minPeak_profile = Peaks_profile[peakCounter];
        peakID_profile = peakCounter;
      }
    }
    projection->GetXaxis()->SetRangeUser( Peaks_profile[peakID_profile] - 3e-9,Peaks_profile[peakID_profile] +4e-9);
    extractCTR(projection,fitPercMin,fitPercMax,divs,tagFwhm,ret,fitRes);
     y_tAvsTime.push_back(fitRes[3]);
    ey_tAvsTime.push_back(fitRes[4]);
     x_tAvsTime.push_back(tAvsTime->GetXaxis()->GetBinCenter(i+1));
    ex_tAvsTime.push_back(tPerBin/sqrt(12));
    
    outputFile->cd();
    projection->Write();
//     std::cout << fitRes[3] << "\n";
  }
//   std::cout << std::endl;
  TGraphErrors *graph_tAvsTime = new TGraphErrors(x_tAvsTime.size(),&x_tAvsTime[0],&y_tAvsTime[0],&ex_tAvsTime[0],&ey_tAvsTime[0]);
  graph_tAvsTime->SetName("graph_tAvsTime");
  s_title << "Average time stamp vs. acq. time   | Tag - crystal " << crystal;
  graph_tAvsTime->SetTitle(s_title.str().c_str());
  s_title.str("");
//   graph_tAvsTime->SetTitle("Average time stamp vs. acq. time   | Tag");
  graph_tAvsTime->GetXaxis()->SetTitle("Time [h]");
  graph_tAvsTime->GetYaxis()->SetTitle("Time [ns]");
  outputFile->cd();
  graph_tAvsTime->Write();
  TCanvas *newCanvas_tA = new TCanvas("newCanvas_tA","newCanvas_tA",1200,800);
  graph_tAvsTime->GetYaxis()->SetRangeUser(-65.5e-9,-64.5e-9);
  graph_tAvsTime->Draw("AP");
  
  newCanvas_tA->cd();
  gSystem->ProcessEvents();
  TImage *img_tA = TImage::Create();
  img_tA->FromPad(newCanvas_tA);
  s_title << "timeTag_" << crystal << "_VsTime.png";
//   img_tA->WriteImage(s_title.str().c_str());
  s_title.str("");
  tAvsTime->GetYaxis()->SetRangeUser(-0.10e-6,0.05e-6);
  
  
  
  var.str("");
  
  //tB vs time
  var << "t" << tB << " :((" << timeTag << " )/(1e9*3600) - "<< tStart << ") >> tBvsTime";
  TH2F *tBvsTime = new TH2F("tBvsTime","tBvsTime",TenMinBins,0,tEnd,1000,-0.10e-6,0.05e-6);
  tBvsTime->GetXaxis()->SetTitle("Acq. time [h]");
  tBvsTime->GetYaxis()->SetTitle("[ADC channels]");
  tree->Draw(var.str().c_str(),noZeros);
  
  std::vector<double>  x_tBvsTime;
  std::vector<double>  y_tBvsTime;
  std::vector<double> ex_tBvsTime;
  std::vector<double> ey_tBvsTime;
  
  for(int i = 0 ; i < TenMinBins; i++)
  {
    std::stringstream s_title_in;
    s_title_in << "projTcry_"<< i;
    TString title = s_title_in.str().c_str();
    TH1D *projection = (TH1D*) tBvsTime->ProjectionY(title,i+1,i+2);
    TSpectrum *spectrum_profile;
    spectrum_profile = new TSpectrum(5);
    Int_t peaksN_profile = spectrum_profile->Search(projection,1,"",0.5);
    Double_t *Peaks_profile  = spectrum_profile->GetPositionX();
    Double_t *PeaksY_profile = spectrum_profile->GetPositionY();
    float minPeak_profile = INFINITY;
    int peakID_profile = 0;
    for (int peakCounter = 0 ; peakCounter < peaksN_profile ; peakCounter++ )
    {
      if(Peaks_profile[peakCounter] < minPeak_profile)
      {
        minPeak_profile = Peaks_profile[peakCounter];
        peakID_profile = peakCounter;
      }
    }
    projection->GetXaxis()->SetRangeUser( Peaks_profile[peakID_profile] - 3e-9,Peaks_profile[peakID_profile] +4e-9);
    extractCTR(projection,fitPercMin,fitPercMax,divs,tagFwhm,ret,fitRes);
     y_tBvsTime.push_back(fitRes[3]);
    ey_tBvsTime.push_back(fitRes[4]);
     x_tBvsTime.push_back(tBvsTime->GetXaxis()->GetBinCenter(i+1));
    ex_tBvsTime.push_back(tPerBin/sqrt(12));
    
    outputFile->cd();
    projection->Write();
//     std::cout << fitRes[3] << "\n";
  }
//   std::cout << std::endl;
  TGraphErrors *graph_tBvsTime = new TGraphErrors(x_tBvsTime.size(),&x_tBvsTime[0],&y_tBvsTime[0],&ex_tBvsTime[0],&ey_tBvsTime[0]);
  graph_tBvsTime->SetName("graph_tBvsTime");
  s_title << "Average time stamp vs. acq. time   | cry - crystal " << crystal;
  graph_tBvsTime->SetTitle(s_title.str().c_str());
  s_title.str("");
//   graph_tBvsTime->SetTitle("Average time stamp vs. acq. time   | Crystal");
  graph_tBvsTime->GetXaxis()->SetTitle("Time [h]");
  graph_tBvsTime->GetYaxis()->SetTitle("Time [ns]");
  outputFile->cd();
  graph_tBvsTime->Write();
  TCanvas *newCanvas_tB = new TCanvas("newCanvas_tB","newCanvas_tB",1200,800);
  graph_tBvsTime->GetYaxis()->SetRangeUser(-65e-9,-64e-9);
  graph_tBvsTime->Draw("AP");
  
  newCanvas_tB->cd();
  gSystem->ProcessEvents();
  TImage *img_tB = TImage::Create();
  img_tB->FromPad(newCanvas_tB);
  s_title << "timeCry_" << crystal << "_VsTime.png";
//   img_tB->WriteImage(s_title.str().c_str());
  s_title.str("");
  tBvsTime->GetYaxis()->SetRangeUser(-0.10e-6,0.05e-6);
  
  var.str("");
  
  
  gStyle->SetOptStat(1111);
  
  sname.str("");
  sname << "Charge histogram - ch "<< chA;
  TCanvas *c_histoA = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
  c_histoA->cd();
  h_chA->Draw();
  h_chA_highlight->SetLineColor(kGreen);
  h_chA_highlight->SetFillColor(kGreen);
  h_chA_highlight->SetFillStyle(3001);
  h_chA_highlight->Draw("same");
  
  sname.str("");
  
  sname << "Charge histogram - ch "<< chB;
  TCanvas *c_histoB = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
  c_histoB->cd();
  h_chB->Draw();
  h_chB_highlight->SetLineColor(kGreen);
  h_chB_highlight->SetFillColor(kGreen);
  h_chB_highlight->SetFillStyle(3001);
  h_chB_highlight->Draw("same");
  
  outputFile->cd();
  c_histoA->Write();
  c_histoB->Write();
  
  h_chA->Write();
  h_chB->Write();
  t_chA->Write();
  t_chB->Write();
  ctr->Write();
  ctrCorr->Write();
  ctrVsTime->Write();
  peakAvsTime->Write();
  peakBvsTime->Write();
  tAvsTime->Write();
  tBvsTime->Write();
//   ctrVsDeltaTime->Write();
  profile->Write();
  outputFile->Close();

  return 0;


}
