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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <getopt.h>
#include <algorithm>    // std::sort

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
  std::cout << "\t\t" << "[-i|--input] <file_prefix>  [-o|--output] <plots.root> [-t|--text] text.txt [OPTIONS]" << std::endl
            << "\t\t" << "<file_prefix>                                      - prefix of TTree files to analyze                                  - default = TTree_"   << std::endl
            << "\t\t" << "<plots.root>                                       - output file name                                                  - default = plots.root"   << std::endl
            << "\t\t" << "<text.txt>                                         - trxt file name (where ctr result will be stored)                  - default = plots.root"   << std::endl
            << "\t\t" << "--chAnum                                           - number of first channel                                           - default = 2 " << std::endl
            << "\t\t" << "--chBnum                                           - number of second channel                                          - default = 9 " << std::endl
            << "\t\t" << "--percUp                                           - lower limit of photopeak fit, in fraction of photopeak (1 = 100%) - default = 0.04 " << std::endl
            << "\t\t" << "--percDown                                         - upper limit of photopeak fit, in fraction of photopeak (1 = 100%) - default = 0.06" << std::endl
            << "\t\t" << "--EnRes                                            - expected Energy resoluton of photopeaks (1 = 100%)                - default = 0.1  " << std::endl
            << "\t\t" << "--sigmas                                           - width of photopeak cuts in sigmas                                 - default = 2.0 " << std::endl
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
  int chA = 2;
  int chB = 9;
  double sigmas = 2.0;
  double percDown = 0.04;
  double percUp = 0.06;
  double ENres = 0.10;

  static struct option longOptions[] =
  {
			{ "input", required_argument, 0, 0 },
      { "output", required_argument, 0, 0 },
      { "text", required_argument, 0, 0 },
      { "chAnum", required_argument, 0, 0 },
      { "chBnum", no_argument, 0, 0 },
      { "sigmas", required_argument, 0, 0 },
      { "percUp", required_argument, 0, 0 },
      { "percDown", required_argument, 0, 0 },
      { "ENres", required_argument, 0, 0 },
			{ NULL, 0, 0, 0 }
	};

  while(1) {
		int optionIndex = 0;
		int c = getopt_long(argc, argv, "i:o:t:", longOptions, &optionIndex);
		if (c == -1) {
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
		else {
      std::cout	<< "Usage: " << argv[0] << std::endl;
			usage();
			return 1;
		}
	}

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

  std::stringstream sname;
  sname << "t_ch" << chA;
  TH1F *t_chA = new TH1F(sname.str().c_str(),sname.str().c_str(),1000,-0.15e-6,0.05e-6);
  sname.str("");

  sname << "c_" << chA;
  TCanvas *c_tA = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
  sname.str("");
  sname << "t" << chA << " >> " << "t_ch" << chA;
  tree->Draw(sname.str().c_str());
  sname.str("");

  sname << "t_ch" << chB;
  TH1F *t_chB = new TH1F(sname.str().c_str(),sname.str().c_str(),1000,-0.15e-6,0.05e-6);
  sname.str("");

  sname << "c_" << chB;
  TCanvas *c_tB = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
  sname.str("");
  sname << "t" << chB << " >> " << "t_ch" << chB;
  tree->Draw(sname.str().c_str());
  sname.str("");

  // TH1F *t_ch9 = new TH1F("t_ch9","t_ch9",1000,-0.15e-6,0.05e-6);
  // TCanvas *c_t9 = new TCanvas("c_t9","c_t9",1200,800);
  // tree->Draw("t9 >> t_ch9");


  sname << "h_ch" << chA;
  TH1F *h_chA = new TH1F(sname.str().c_str(),sname.str().c_str(),1000,0,40000);
  sname.str("");

  sname << "c_h" << chA;
  TCanvas *c_hA = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
  sname.str("");
  sname << "ch" << chA << " >> " << "h_ch" << chA;
  tree->Draw(sname.str().c_str());
  sname.str("");

  sname << "h_ch" << chB;
  TH1F *h_chB = new TH1F(sname.str().c_str(),sname.str().c_str(),1000,0,40000);
  sname.str("");

  sname << "c_h" << chB;
  TCanvas *c_hB = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
  sname.str("");
  sname << "ch" << chB << " >> " << "h_ch" << chB;
  tree->Draw(sname.str().c_str());
  sname.str("");

  // find cuts
  //ch2
  c_hA->cd();
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

  //ch9
  c_hB->cd();
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

  // TCut peakch9 = "ch9 > 19000 && ch9 < 20500";

  //avoid zeroes!
  sname << "t" << chA << " != 0 && t" << chB <<" != 0";
  TCut noZeros = sname.str().c_str();
  sname.str("");

  TH1F *ctr = new TH1F("ctr","ctr",500,-1e-9,1e-9);
  TCanvas *c_ht = new TCanvas("c_ht","c_ht",1200,800);
  sname << "t" << chB << " - t" << chA <<" >> ctr";
  tree->Draw(sname.str().c_str(),peakch2+peakch9+noZeros);
  sname.str("");

  TF1 *gaussCTR = new TF1("gaussCTR","gaus");
  ctr->Fit(gaussCTR,"Q");

  Double_t chi2 = gaussCTR->GetChisquare();
  Int_t NDF = gaussCTR->GetNDF();
  Double_t chi2red = chi2 / NDF;

  std::cout << "CTR FWHM [ps]    CTR error [ps]     Chi^2/NDF " << std::endl;
  std::cout << gaussCTR->GetParameter(2)*2.355*1e12 << " " << gaussCTR->GetParError(2)*2.355*1e12 << " "  << chi2red << std::endl;



  std::ofstream textfile;
  textfile.open (textFileName.c_str(),std::ofstream::out);
  textfile << "#CTR FWHM [ps]    CTR error [ps]     Chi^2/NDF " << std::endl;
  textfile << gaussCTR->GetParameter(2)*2.355*1e12 << " " << gaussCTR->GetParError(2)*2.355*1e12 << " "  << chi2red << std::endl;
  textfile.close();

  TFile *outputFile = new TFile(outputFileName.c_str(),"RECREATE");
  outputFile->cd();
  h_chA->Write();
  h_chB->Write();
  t_chA->Write();
  t_chB->Write();
  ctr->Write();
  outputFile->Close();

  return 0;


}
