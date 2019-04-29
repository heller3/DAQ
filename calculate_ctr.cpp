// compile with
// g++ -o ./bin/calculate_ctr calculate_ctr.cpp `root-config --cflags --glibs` -Wl,--no-as-needed -lHist -lCore -lMathCore -lTree -lTreePlayer -lgsl -lgslcblas -lSpectrum

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
#include "TMatrixD.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <getopt.h>
#include <algorithm>    // std::sort
#include <gsl/gsl_matrix_double.h>
#include <gsl/gsl_linalg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// #include "Crystal.h"
// #include "./include/ConfigFile.h"


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


//------------------------------------------//
//      FUNCTIONS INPUT PARSING             //
//------------------------------------------//
struct split_t
{
  enum empties_t { empties_ok, no_empties };
};

template <typename Container>
Container& split(Container& result,const typename Container::value_type& s,const typename Container::value_type& delimiters,split_t::empties_t empties = split_t::empties_ok)
{
  // splits the strings into individual fields
  // useful if you want to pass many parameters in the same string
  result.clear();
  size_t current;
  size_t next = -1;
  do
  {
    if (empties == split_t::no_empties)
    {
      next = s.find_first_not_of( delimiters, next + 1 );
      if (next == Container::value_type::npos) break;
      next -= 1;
    }
    current = next + 1;
    next = s.find_first_of( delimiters, current );
    result.push_back( s.substr( current, next - current ) );
  }
  while (next != Container::value_type::npos);
  return result;
}

void trim( std::string& s )
{
  // Remove leading and trailing whitespace
  static const char whitespace[] = " \n\t\v\r\f";
  s.erase( 0, s.find_first_not_of(whitespace) );
  s.erase( s.find_last_not_of(whitespace) + 1U );
}
//------------------------------------------//
//      end of FUNCTIONS INPUT PARSING      //
//------------------------------------------//


void searchPeak(TH1F* h0,float lRange,float uRange,float perc, float* res)
{
  // float originalMin = h0->GetXaxis()->GetXmin();
  // float originalMax = h0->GetXaxis()->GetXmax();
  h0->GetXaxis()->SetRangeUser(lRange, uRange);
  TSpectrum *spectrum0 = new TSpectrum(5);
  Int_t n0 = spectrum0->Search(h0, 1, "", 0.5);
  Double_t *xvals0 = spectrum0->GetPositionX();
  Double_t *yvals0 = spectrum0->GetPositionY();

  float maxPeak = 0.0;
  int xmaxID = 0;
  for (int peakCounter = 0 ; peakCounter < n0 ; peakCounter++ )
  {
    if(yvals0[peakCounter] > maxPeak)
    {
      maxPeak = yvals0[peakCounter];
      xmaxID = peakCounter;
    }
  }
  float xmax0 = xvals0[xmaxID];
  float FWHM0 = fabs(perc*xmax0);
  float lb0 = xmax0 - FWHM0;
  float ub0 = xmax0 + FWHM0;
  // std::cout << lb0 << " " << ub0 << std::endl;

  TF1 *fgaus0 = new TF1("fgaus0", "gaus", lb0, ub0);
  h0->Fit(fgaus0, "Q", "", lb0, ub0 );
  float mean0 = fgaus0->GetParameter(1);
  float sigma0 = fgaus0->GetParameter(2);
  // fit the charge histograms again, using upper and lower bounds determined by the mean and standard dev. of the gaussian fit
  float lb0_g = mean0 - sigma0;
  float ub0_g = mean0 + 2.0*sigma0;
  TF1* fgaus0_g = new TF1("fgaus0_g", "gaus", lb0_g, ub0_g);
  h0->Fit(fgaus0_g, "Q", "", lb0_g, ub0_g );
  //redifine mean0 etc (otherwise why are you refitting them?)
  mean0 = fgaus0_g->GetParameter(1);
  sigma0 = fgaus0_g->GetParameter(2);
  res[0] = mean0;
  res[1] = sigma0;
  // h0->GetXaxis()->SetRangeUser(originalMin, originalMax);
}

// standard EMG inverted. x -> -x, then mu will be -mu
Double_t emginv_function(Double_t *x, Double_t *par)
{
   Double_t xx = x[0];
   Double_t m = par[1] + par[3];
   Double_t f;
   Double_t norm = par[0] /(2.0 * par[3]);
   Double_t expo = TMath::Exp( (1.0/(2.0*par[3])) * ( 2.0*par[1] + ( TMath::Power(par[2],2)/par[3] ) + 2.0*(xx) ) );
   Double_t erf_c = TMath::Erfc( ( par[1] + ( TMath::Power(par[2],2)/par[3] ) + (xx ) )/(TMath::Sqrt(2)  * par[2] )  ) ;
   f = norm * expo * erf_c;
   return f;
}

void fitWithEMG(TH1F* histo,float* res)
{
  TF1* gaussDummy  = new TF1("gaussDummy","gaus");
  gaussDummy->SetLineColor(kOrange);
  float fitGaussMin = histo->GetMean() - 2.0*histo->GetRMS();
  float fitGaussMax = histo->GetMean() + 2.0*histo->GetRMS();
  float f1min       = histo->GetXaxis()->GetXmin();
  float f1max       = histo->GetXaxis()->GetXmax();
  if(fitGaussMin < f1min)
  {
    fitGaussMin = f1min;
  }
  if(fitGaussMax > f1max)
  {
    fitGaussMax = f1max;
  }

  TFitResultPtr rGauss = histo->Fit(gaussDummy,"SQ","",fitGaussMin,fitGaussMax);
  Int_t fitStatusGauss = rGauss;
  float fitSigmasMin = 6.0;
  float fitSigmasMax = 5.0;

  float fitMin = gaussDummy->GetParameter(1) - fitSigmasMin*(gaussDummy->GetParameter(2));
  float fitMax = gaussDummy->GetParameter(1) + fitSigmasMax*(gaussDummy->GetParameter(2));

  // use inverted emg
  TF1* gexp_inv  = new TF1("gexp_inv",emginv_function,f1min,f1max,4);
  gexp_inv->SetLineColor(kGreen);
  gexp_inv->SetParName(0,"N");
  gexp_inv->SetParName(1,"mu");
  gexp_inv->SetParName(2,"Sigma");
  gexp_inv->SetParName(3,"tau");

  gexp_inv->SetParameters(histo->GetEntries()*histo->GetXaxis()->GetBinWidth(1),-histo->GetMean(),histo->GetRMS(),histo->GetRMS());
  histo->Fit(gexp_inv,"SQ","",fitMin,fitMax);

  // extract fit parameters
  float mu    = gexp_inv->GetParameter(1);
  float e_mu  = gexp_inv->GetParError(1);
  float s     = gexp_inv->GetParameter(2);
  float e_s   = gexp_inv->GetParError(2);
  float tau   = gexp_inv->GetParameter(3);
  float e_tau = gexp_inv->GetParError(3);

  Double_t mean;
  mean = -(mu + tau);
  Double_t sigma = TMath::Sqrt(TMath::Power(s,2) + TMath::Power(tau,2));
  Double_t meanErr = TMath::Sqrt(TMath::Power(e_mu,2) + TMath::Power(e_tau,2));
  Double_t sigmaErr = TMath::Sqrt( ( (TMath::Power(s,2))/(TMath::Power(s,2) + TMath::Power(tau,2))  )*TMath::Power(e_s,2) + ((TMath::Power(tau,2))/(TMath::Power(s,2) + TMath::Power(tau,2)))*TMath::Power(e_tau,2));

  res[0] = mean;
  res[1] = sigma;
  res[2] = meanErr;
  res[3] = sigmaErr;

}


void usage()
{
  std::cout << " [--option] <value>" << std::endl
            << "\t\t" << "--folder          - path to input file(s) folder                , default = ./"      << std::endl
            << "\t\t" << "--prefix          - prefix of input file(s)                     , default = TTree"   << std::endl
            << "\t\t" << "--output          - name of output files, no extension          , default = results" << std::endl
            << "\t\t" << "--refCh           - channel number for reference                , default = 62"      << std::endl
            << "\t\t" << "--detCh           - csv list of channel number(s) for SiPM(s)   , default = \"\""    << std::endl
            << "\t\t" << "--voltage         - bias voltage value                          , default = 58.0"    << std::endl
            << "\t\t" << "--refFWHM         - FWHM contribution to CTR of reference [s]   , default = 90e-12"  << std::endl   
            << "\t\t" << "--bins            - bins in 1D histograms                       , default = 1000"    << std::endl
            << "\t\t" << "--h0max           - max for reference charge spectrum           , default = 60000"   << std::endl
            << "\t\t" << "--h1max           - max for SiPM(s) charge spectrum             , default = 500000"  << std::endl            
            << "\t\t" << "--sigmac          - numb of sigmas for charge cut               , default = 2"       << std::endl
            << "\t\t" << "--sigmat          - numb of sigmas for time cut                 , default = 20"      << std::endl            
            << "\t\t" << "--h0low           - min for peak search in ref charge spectrum  , default = 500"     << std::endl
            << "\t\t" << "--h0up            - max for peak search in ref charge spectrum  , default = 350000"  << std::endl
            << "\t\t" << "--h1low           - min for peak search in ref charge spectrum  , default = 500"     << std::endl
            << "\t\t" << "--h1up            - max for peak search in ref charge spectrum  , default = 350000"  << std::endl
            << "\t\t" << "--dtBins          - bins in ctr time histograms                 , default = 1000 "   << std::endl
            << "\t\t" << "--dtMin           - min of ctr time histograms                  , default = -2e-9"   << std::endl
            << "\t\t" << "--dtMax           - max of ctr time histograms                  , default = 2e-9"    << std::endl
            << "\t\t" << "--function        - fitting function for time alignment "                            << std::endl
            << "\t\t" << "                  - 0 = gaussian (default) "                                         << std::endl
            << "\t\t" << "                  - 1 = exponentially modified gaussian "                            << std::endl
            << std::endl;
}


//----------------//
//  MAIN PROGRAM  //
//----------------//
int main (int argc, char** argv)
{
  // check if there are args, otherwise print the usage info
  if(argc < 2)
  {
    std::cout << "USAGE: " << argv[0];
    usage();
    return 1;
  }

  // save the entire command line
  std::stringstream streamCommand;
  for(int i=0 ; i < argc; i++)
  {
    streamCommand << argv[i] << " ";
  }

  // set stat and fit information level in root files
  gStyle->SetOptStat(1111);
  gStyle->SetOptFit(1111);

  //--------------------------------//
  // DEFAULT ARGS                   //
  //--------------------------------//
  std::string folder    = "./";
  std::string prefix    = "TTree";
  float ref       = 90e-12;
  float voltage   = 58.0;
  std::string output    = "results";
  int fitFunction = 1;
  int bins1D_t = 1000;
  float h0max = 60000;
  float h1max = 500000;
  float h0low = 500;
  float h0up = 350000;
  float h1low = 500;
  float h1up = 350000;
  int refCh = 62;
  std::string detCh = "";
  float time_cut_sigmas = 20.0;
  float charge_cut_sigmas = 2.0;
  int dtBins = 1000;
  float dtMin = -2e-9;
  float dtMax = 2e-9;


  //--------------------------------//
  // READ COMMAND LINE PARAMETERS   //
  //--------------------------------//
  static struct option longOptions[] =
  {
      { "folder", required_argument, 0, 0 },
      { "prefix", required_argument, 0, 0 },
      { "refFWHM", required_argument, 0, 0 },
      { "voltage", required_argument, 0, 0 },
      { "output", required_argument, 0, 0 },
      { "function", required_argument, 0, 0 },
      { "bins", required_argument, 0, 0 },
      { "h1max", required_argument, 0, 0 },
      { "refCh", required_argument, 0, 0 },
      { "detCh", required_argument, 0, 0 },
      { "sigmac", required_argument, 0, 0 },
      { "sigmat", required_argument, 0, 0 },
      { "h0max", required_argument, 0, 0 },
      { "h0low", required_argument, 0, 0 },
      { "h0up", required_argument, 0, 0 },
      { "h1low", required_argument, 0, 0 },
      { "h1up", required_argument, 0, 0 },
      { "dtBins", required_argument, 0, 0 },
      { "dtMin", required_argument, 0, 0 },
      { "dtMax", required_argument, 0, 0 },
      { NULL, 0, 0, 0 }
        };

  while(1) {
                int optionIndex = 0;
                int c = getopt_long(argc, argv, "f:p:o", longOptions, &optionIndex);
                if (c == -1) {
                        break;
                }
                if (c == 'f'){
                        folder = (char *)optarg;
    }
    else if (c == 'p'){
                        prefix = (char *)optarg;
    }
    else if (c == 'o'){
                        output = (char *)optarg;
    }
                else if (c == 0 && optionIndex == 0){
      folder = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 1){
      prefix = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 2){
      ref = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 3){
      voltage = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 4){
      output = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 5){
      fitFunction = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 6){
      bins1D_t = atoi((char *)optarg);;
    }
    else if (c == 0 && optionIndex == 7){
      h1max = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 8){
      refCh = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 9){
      detCh = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 10){
      charge_cut_sigmas = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 11){
      time_cut_sigmas = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 12){
      h0max = atof((char *)optarg);
    }
    
    else if (c == 0 && optionIndex == 13){
      h0low = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 14){
      h0up = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 15){
      h1low = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 16){
      h1up = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 17){
      dtBins = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 18){
      dtMin = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 19){
      dtMax = atof((char *)optarg);
    }
    else {
      std::cout << "Usage: " << argv[0];
                        usage();
                        return 1;
                }
  }
  
  // check and limit h0up and h1up to the histogram max 
  if(h0up > h0max)
  {
    h0up = h0max;
  }
  if(h1up > h1max)
  {
    h1up = h1max;
  }

  std::string outputFileName_root = output + ".root";
  std::string outputFileName_text = output + ".txt";
  // get list of detector channels
  std::vector<std::string> str_listDetectorChannels;
  split( str_listDetectorChannels, detCh, "," );  // split the entry
  std::vector<int> listDetectorChannels;
  for(unsigned int i = 0 ; i < str_listDetectorChannels.size(); i++)
  {
    listDetectorChannels.push_back( atoi( str_listDetectorChannels[i].c_str() ) );
  }


  std::string fString;
  if(fitFunction == 0)
  {
    fString = "gauss";
  }
  else
  {
    fString = "emg";
  }
  //---------------------------------------//
  // FEEDBACK PARAMETERS                   //
  //---------------------------------------//
  std::cout << std::endl;
  std::cout << "//-------------------------------------//"         << std::endl;
  std::cout << "// INPUT PARAMETERS                    //"         << std::endl;
  std::cout << "//-------------------------------------//"         << std::endl;
  std::cout << "Path to folder with input data                 = " << folder << std::endl;
  std::cout << "Prefix of input files                          = " << prefix << std::endl;
  std::cout << "Reference detector FWHM timing resolution [s]  = " << ref    << std::endl;
  std::cout << "Bias voltage of detector(s) [V]                = " << voltage    << std::endl;
  std::cout << "Type of functon for 1D fits                    = " << fString    << std::endl;
  std::cout << "Bins in 1D histograms                          = " << bins1D_t    << std::endl;
  std::cout << "Maximum of sum charge plot                     = " << h1max    << std::endl;
  std::cout << "Reference detector channel                     = " << refCh    << std::endl;
  std::cout << "Number of detector channel(s)                  = " << listDetectorChannels.size()    << std::endl;
  std::cout << "Detector channel(s)                            = " << detCh    << std::endl;
  std::cout << "Output file name [ROOT]                        = " << outputFileName_root    << std::endl;
  std::cout << "Output file name [text]                        = " << outputFileName_text    << std::endl;
  std::cout << std::endl;




  //----------------------------------//
  // GET INPUT FILES(S)               //
  //----------------------------------//
  // read file in dir
  std::cout << std::endl;
  std::cout << "|----------------------------------------|" << std::endl;
  std::cout << "|         ANALYSIS FILES                 |" << std::endl;
  std::cout << "|----------------------------------------|" << std::endl;
  std::cout << std::endl;
  // get input files list
  std::vector<std::string> v;
  read_directory(folder, v);
  // extract files with correct prefix
  std::vector<std::string> listInputFiles;
  for(unsigned int i = 0 ; i < v.size() ; i++)
  {
    if(!v[i].compare(0,prefix.size(),prefix))
    {
      listInputFiles.push_back(folder + "/" + v[i]);
    }
  }
  // check if it's empty
  if(listInputFiles.size() == 0)
  {
    std::cout << std::endl;
    std::cout << "ERROR! Some input files do not exists! Aborting." << std::endl;
    std::cout << "See program usage below..." << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << argv[0];
    usage();
    return 1;
  }

  //----------------------------------------------------------//
  //  Get TChain from input TTree files                       //
  //----------------------------------------------------------//
  TChain* tree = new TChain("adc");  // create the input tchain and the analysis ttree
  for(unsigned int i = 0 ; i < listInputFiles.size(); i++)
  {
    std::cout << "Adding file " << listInputFiles[i] << std::endl;
    tree->Add(listInputFiles[i].c_str());
  }
  std::cout << "|----------------------------------------|" << std::endl;
  std::cout << std::endl;
  std::vector<int> detector_channels;
  TObjArray *leavescopy = tree->GetListOfLeaves();
  int nLeaves = leavescopy->GetEntries();
  std::vector<std::string> leavesName;
  // fill a vector with the leaves names
  for(int i = 0 ; i < nLeaves ; i++)
  {
    leavesName.push_back(leavescopy->At(i)->GetName());
  }
  // count the entries that start with "ch"
  int numOfCh = 0;
  std::string ch_prefix("ch");
  std::string t_prefix("t");
  for(int i = 0 ; i < nLeaves ; i++)
  {
    if (!leavesName[i].compare(0, ch_prefix.size(), ch_prefix))
    {
      numOfCh++;
      detector_channels.push_back(atoi( (leavesName[i].substr(ch_prefix.size(),leavesName[i].size()-ch_prefix.size())).c_str() )) ;
    }
  }
  std::cout << "Detector Channels \t= " << numOfCh << std::endl;

  //set variables and branches
  ULong64_t     ChainExtendedTimeTag;                                // extended time tag
  ULong64_t     ChainDeltaTimeTag;                                   // delta tag from previous
  UShort_t      *charge;
  Float_t      *timeStamp;
  TBranch      *bChainExtendedTimeTag;                               // branches for above data
  TBranch      *bChainDeltaTimeTag;                                  // branches for above data
  TBranch      **bCharge;
  TBranch      **btimeStamp;
  charge = new UShort_t[numOfCh];
  timeStamp = new Float_t[numOfCh];
  bCharge = new TBranch*[numOfCh];
  btimeStamp = new TBranch*[numOfCh];
  // set branches for reading the input files
  tree->SetBranchAddress("ExtendedTimeTag", &ChainExtendedTimeTag, &bChainExtendedTimeTag);
  tree->SetBranchAddress("DeltaTimeTag", &ChainDeltaTimeTag, &bChainDeltaTimeTag);
  for (int i = 0 ; i < detector_channels.size() ; i++)
  {
    //empty the stringstreams
    std::stringstream sname;
    sname << "ch" << detector_channels[i];
    tree->SetBranchAddress(sname.str().c_str(),&charge[detector_channels[i]],&bCharge[detector_channels[i]]);
    sname.str("");
    sname << "t" << detector_channels[i];
    tree->SetBranchAddress(sname.str().c_str(),&timeStamp[detector_channels[i]],&btimeStamp[detector_channels[i]]);
    sname.str("");
  }



  // FIRST LOOP - basic histograms
  // prepare histograms
  // one for ref detector
  TH1F* h0 = new TH1F("h0","h0",1000,0, h0max);
  // one for "sum" detector
  TH1F* h1 = new TH1F("h1","h1",1000,0, h1max);

  TH1F* h0_highlight = new TH1F("h0_highlight","h0_highlight",1000,0, h0max);
  h0_highlight->SetFillStyle(3001);
  h0_highlight->SetFillColor(kGreen);
  // one for "sum" detector
  TH1F* h1_highlight = new TH1F("h1_highlight","h1_highlight",1000,0, h1max);
  h1_highlight->SetFillStyle(3001);
  h1_highlight->SetFillColor(kGreen);

  // one for each time histogram involved (ref + detectors)
  std::vector<TH1F*> h_t_det;
  std::vector<TH1F*> h_t_det_highlight ;
  // TH1F** h_t_det = new TH1F*[listDetectorChannels.size() ];
  TH1F* h_t_ref = new TH1F("h_t_ref","h_t_ref",4200,-0.21e-6, 0.21e-6);
  TH1F* h_t_ref_highlight = new TH1F("h_t_ref_highlight","h_t_ref_highlight",4200,-0.21e-6, 0.21e-6);
  h_t_ref_highlight->SetFillStyle(3001);
  h_t_ref_highlight->SetFillColor(kGreen);

  for(unsigned int i = 0 ; i < listDetectorChannels.size(); i++)
  {
    std::stringstream sname;
    sname << "h_t_det" << listDetectorChannels[i];

    TH1F* temp_h = new TH1F(sname.str().c_str(),sname.str().c_str(),4200,-0.21e-6, 0.21e-6);
    h_t_det.push_back(temp_h);

    sname.str("");
    sname << "h_t_det_highlight" << listDetectorChannels[i];
    TH1F* temp_h2 = new TH1F(sname.str().c_str(),sname.str().c_str(),4200,-0.21e-6, 0.21e-6);
    temp_h2->SetFillStyle(3001);
    temp_h2->SetFillColor(kGreen);
    h_t_det_highlight.push_back(temp_h2);
  }

  long long int counter = 0;
  // tree->SetNotify(formulasAnalysis);
  long long int nevent = tree->GetEntries();
  std::cout << "Total number of events in input files = " << nevent << std::endl;
  long int goodEventsAnalysis = 0;
  long int counterAnalysis = 0;
  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    std::cout <<listDetectorChannels[iDet] << " ";
  }
  std::cout << std::endl;

  for (long long int i=0;i<nevent;i++)
  {

    tree->GetEvent(i);              //read complete accepted event in memory

    h0->Fill(charge[refCh]);
    h_t_ref->Fill(timeStamp[refCh]);

    float sumCharge = 0;
    for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
    {
      sumCharge += charge[listDetectorChannels[iDet]];
      h_t_det[iDet]->Fill(timeStamp[listDetectorChannels[iDet]]);
    }
    h1->Fill(sumCharge);

    counter++;
    int perc = ((100*counter)/nevent); //should strictly have not decimal part, written like this...
    if( (perc % 10) == 0 )
    {
      std::cout << "\r";
      std::cout << perc << "% done... ";
      //std::cout << counter << std::endl;
    }
  }
  std::cout << std::endl;


  //peak searching
  float peak_results[2];
  searchPeak(h0,h0low,h0up,0.03,peak_results);
  float mean0 = peak_results[0];
  float sigma0 = peak_results[1];

  searchPeak(h1,h1low,h1up,0.03,peak_results);
  float mean1 = peak_results[0];
  float sigma1 = peak_results[1];

  searchPeak(h_t_ref,-0.2e-6,-0.02e-6,0.01,peak_results);
  float t_ref_mean = peak_results[0];
  float t_ref_sigma = peak_results[1];

  std::vector<float> t_det_mean;
  std::vector<float> t_det_sigma;
  for(unsigned int iDet = 0 ; iDet < h_t_det.size(); iDet++)
  {
    searchPeak(h_t_det[iDet],-0.2e-6,-0.02e-6,0.01,peak_results);
    t_det_mean.push_back(peak_results[0]);
    t_det_sigma.push_back(peak_results[1]);
  }




  // some histograms
  TH1F* h_t_base = new TH1F("h_t_base", "H(t_ref - avg); t_ref - t_avg; Number of Events", dtBins,dtMin,dtMax);
  std::vector<TH2F*> g;
  std::vector<TH1F*> h_det_uc;

  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    std::stringstream sname;
    sname << "(t_ref - t"<< listDetectorChannels[iDet] << ") vs. ch" << listDetectorChannels[iDet];
    TH2F* temp_h2 = new TH2F(sname.str().c_str(), sname.str().c_str(), 100, 0, (h1max/listDetectorChannels.size()), 100, -2.5e-9, 2.5e-9);
    g.push_back(temp_h2);
    sname.str("");
    sname << "Uncorrected t_ref - t" << listDetectorChannels[iDet];
    TH1F* temp_h1 = new TH1F(sname.str().c_str(), sname.str().c_str(), bins1D_t,-2e-9, 2e-9);
    h_det_uc.push_back(temp_h1);
  }


  counter = 0;
  // tree->SetNotify(formulasAnalysis);
  // long long int nevent = tree->GetEntries();
  // std::cout << "Total number of events in input files = " << nevent << std::endl;
  // long int goodEventsAnalysis = 0;
  // long int counterAnalysis = 0;
  // for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  // {
    // std::cout <<listDetectorChannels[iDet] << " ";
  // }
  // std::cout << std::endl;


  for (long long int i=0;i<nevent;i++)
  {

    tree->GetEvent(i);              //read complete accepted event in memory

    //fill highlighted time histos
    if( (timeStamp[refCh] >= (t_ref_mean - time_cut_sigmas * t_ref_sigma)) && (timeStamp[refCh] <= (t_ref_mean + time_cut_sigmas * t_ref_sigma)) )
    {
      h_t_ref_highlight->Fill(timeStamp[refCh]);
    }

    for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
    {
      if( (timeStamp[listDetectorChannels[iDet]] >= (t_det_mean[iDet] - time_cut_sigmas * t_det_sigma[iDet])) &&
          (timeStamp[listDetectorChannels[iDet]] <= (t_det_mean[iDet] + time_cut_sigmas * t_det_sigma[iDet])) )
      {
        h_t_det_highlight[iDet]->Fill(timeStamp[listDetectorChannels[iDet]]);
        // timeOK = false;
      }
    }

    float sumCharge = 0;
    for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
    {
      sumCharge += charge[listDetectorChannels[iDet]];
      // h_t_det[iDet]->Fill(timeStamp[listDetectorChannels[iDet]]);
    }
    if( (sumCharge >= ( mean1 - charge_cut_sigmas*sigma1) ) && (sumCharge <= ( mean1 + charge_cut_sigmas*sigma1)) )// photopeak of "sum"
    {
      h1_highlight->Fill(sumCharge);
    }


    //conditions on charges to fill the delta t histos
    if( (charge[refCh] >= (mean0 - charge_cut_sigmas *sigma0)) && (charge[refCh] <= (mean0 + charge_cut_sigmas *sigma0)) ) // photopeak of reference
    {
      //fill h0_highlight
      h0_highlight->Fill(charge[refCh]);
      if( (sumCharge >= ( mean1 - charge_cut_sigmas*sigma1) ) && (sumCharge <= ( mean1 + charge_cut_sigmas*sigma1)) )// photopeak of "sum"
      {

        if(timeStamp[refCh] != 0) // no zero for ref
        {
          bool noZeroes = true;
          for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
          {
            if(timeStamp[listDetectorChannels[iDet]] == 0)
            {
              noZeroes = false;
            }
          }
          if(noZeroes)// no zeroes for det
          {
            // time range restriction
            bool timeOK = true;
            if( (timeStamp[refCh] <= (t_ref_mean - time_cut_sigmas * t_ref_sigma)) || (timeStamp[refCh] >= (t_ref_mean + time_cut_sigmas * t_ref_sigma)) )
            {
              // h_t_ref_highlight->Fill(timeStamp[refCh]);
              timeOK = false;
            }

            for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
            {
              if( (timeStamp[listDetectorChannels[iDet]] <= (t_det_mean[iDet] - time_cut_sigmas * t_det_sigma[iDet])) ||
                  (timeStamp[listDetectorChannels[iDet]] >= (t_det_mean[iDet] + time_cut_sigmas * t_det_sigma[iDet])) )
              {
                // h_t_det_highlight[iDet]->Fill(timeStamp[listDetectorChannels[iDet]]);
                timeOK = false;
              }
            }

            if(timeOK)   // time range restriction OK
            {
              float t_diff = 0;
              float t_avg = 0;
              for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
              {
                t_avg += timeStamp[listDetectorChannels[iDet]];
              }
              t_avg = t_avg/listDetectorChannels.size();
              t_diff =  timeStamp[refCh] - t_avg;

              h_t_base->Fill(t_diff);
              for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
              {
                g[iDet]->Fill (charge[listDetectorChannels[iDet]] , timeStamp[refCh] - timeStamp[listDetectorChannels[iDet]]);
                h_det_uc[iDet]->Fill(timeStamp[refCh] - timeStamp[listDetectorChannels[iDet]]);
              }



            }
          }
        }
      }
    }


    counter++;
    int perc = ((100*counter)/nevent); //should strictly have not decimal part, written like this...
    if( (perc % 10) == 0 )
    {
      std::cout << "\r";
      std::cout << perc << "% done... ";
      //std::cout << counter << std::endl;
    }
  }
  std::cout << std::endl;




  //Create ProfileX's for each histogram
  std::vector<TProfile*> fx;
  for(unsigned int iDet = 0 ; iDet < g.size(); iDet++)
  {
    std::stringstream sname;
    sname << "fx" << listDetectorChannels[iDet];
    TProfile* temp_fx = g[iDet]->ProfileX(sname.str().c_str());
    fx.push_back(temp_fx);
  }

  //fit with lines
  std::vector<float> midpoint;
  std::vector<float> lb_2D;
  std::vector<float> ub_2D;
  std::vector<TF1*>  flinear;
  std::vector<float> tref;
  for(unsigned int iDet = 0 ; iDet < fx.size(); iDet++)
  {
    float mean_2D = g[iDet]->GetMean(1);
    float lbb     = g[iDet]->GetMean(1) - 2.0*g[iDet]->GetRMS(1);
    float ubb     = g[iDet]->GetMean(1) + 2.0*g[iDet]->GetRMS(1);

    midpoint.push_back(mean_2D);
    lb_2D.push_back(lbb);
    ub_2D.push_back(ubb);

    TF1* temp_lin = new TF1("flinear", "pol1", 0, h1max/listDetectorChannels.size());
    flinear.push_back(temp_lin);
  }

  for(unsigned int iDet = 0 ; iDet < fx.size(); iDet++)
  {
    fx[iDet]->Fit(flinear[iDet], "Q", "",lb_2D[iDet], ub_2D[iDet]);
    float temp_tref = flinear[iDet]->Eval(midpoint[iDet]);
    tref.push_back(temp_tref);
  }


  TH1F* h_t_corrected = new TH1F("h_t_corrected", "Time histogram corrected by amplitude walk; Number of Events", dtBins,dtMin,dtMax);
  std::vector<TH2F*> gc;
  std::vector<TH1F*> h_det_c;

  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    std::stringstream sname;
    sname << "Corrected (t_ref - t"<< listDetectorChannels[iDet] << ") vs. ch" << listDetectorChannels[iDet];
    TH2F* temp_h2 = new TH2F(sname.str().c_str(), sname.str().c_str(), 100, 0, (h1max/listDetectorChannels.size()), 100, -2.5e-9, 2.5e-9);
    gc.push_back(temp_h2);
    sname.str("");
    sname << "Corrected t_ref - t" << listDetectorChannels[iDet];
    TH1F* temp_h1 = new TH1F(sname.str().c_str(), sname.str().c_str(), bins1D_t,-2e-9, 2e-9);
    h_det_c.push_back(temp_h1);
  }


  counter = 0;
  // tree->SetNotify(formulasAnalysis);
  // long long int nevent = tree->GetEntries();
  // std::cout << "Total number of events in input files = " << nevent << std::endl;
  // long int goodEventsAnalysis = 0;
  // long int counterAnalysis = 0;
  // for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  // {
    // std::cout <<listDetectorChannels[iDet] << " ";
  // }
  // std::cout << std::endl;

  for (long long int i=0;i<nevent;i++)
  {

    tree->GetEvent(i);              //read complete accepted event in memory

    //conditions
    if( (charge[refCh] >= (mean0 - charge_cut_sigmas *sigma0)) && (charge[refCh] <= (mean0 + charge_cut_sigmas *sigma0)) ) // photopeak of reference
    {
      float sumCharge = 0;
      for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
      {
        sumCharge += charge[listDetectorChannels[iDet]];
        // h_t_det[iDet]->Fill(timeStamp[listDetectorChannels[iDet]]);
      }
      if( (sumCharge >= ( mean1 - charge_cut_sigmas*sigma1) ) && (sumCharge <= ( mean1 + charge_cut_sigmas*sigma1)) )// photopeak of "sum"
      {
        if(timeStamp[refCh] != 0) // no zero for ref
        {
          bool noZeroes = true;
          for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
          {
            if(timeStamp[listDetectorChannels[iDet]] == 0)
            {
              noZeroes = false;
            }
          }
          if(noZeroes)// no zeroes for det
          {
            // time range restriction
            bool timeOK = true;
            if( (timeStamp[refCh] <= (t_ref_mean - time_cut_sigmas * t_ref_sigma)) || (timeStamp[refCh] >= (t_ref_mean + time_cut_sigmas * t_ref_sigma)) )
            {
              timeOK = false;
            }

            for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
            {
              if( (timeStamp[listDetectorChannels[iDet]] <= (t_det_mean[iDet] - time_cut_sigmas * t_det_sigma[iDet])) ||
                  (timeStamp[listDetectorChannels[iDet]] >= (t_det_mean[iDet] + time_cut_sigmas * t_det_sigma[iDet])) )
              {
                timeOK = false;
              }
            }

            if(timeOK)   // time range restriction OK
            {
              // calculate corrected timestamps
              // prepare vector
              std::vector<float> corrected_timestamp;
              for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
              {
                float temp_corr = timeStamp[listDetectorChannels[iDet]] + (flinear[iDet]->Eval(charge[listDetectorChannels[iDet]]) - tref[iDet]);
                corrected_timestamp.push_back(temp_corr);
              }
              float t_avg = 0;
              for(unsigned int iDet = 0 ; iDet < corrected_timestamp.size(); iDet++)
              {
                h_det_c[iDet]->Fill( timeStamp[refCh] - corrected_timestamp[iDet] );
                gc[iDet]->Fill( charge[listDetectorChannels[iDet]] , timeStamp[refCh] - corrected_timestamp[iDet] );
                t_avg += corrected_timestamp[iDet];

              }
              t_avg = t_avg/corrected_timestamp.size();
              float t_diffc = timeStamp[refCh] - t_avg;
              h_t_corrected->Fill(t_diffc);



              // float t_diff = 0;
              // float t_avg = 0;
              // for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
              // {
              //   t_avg += timeStamp[listDetectorChannels[iDet]];
              // }
              // t_avg = t_avg/listDetectorChannels.size();
              // t_diff =  timeStamp[refCh] - t_avg;
              //
              // h_t_base->Fill(t_diff);
              // for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
              // {
              //   g[iDet]->Fill (charge[listDetectorChannels[iDet]] , timeStamp[refCh] - timeStamp[listDetectorChannels[iDet]]);
              //   h_det_uc[iDet]->Fill(timeStamp[refCh] - timeStamp[listDetectorChannels[iDet]]);
              // }



            }
          }
        }
      }
    }


    counter++;
    int perc = ((100*counter)/nevent); //should strictly have not decimal part, written like this...
    if( (perc % 10) == 0 )
    {
      std::cout << "\r";
      std::cout << perc << "% done... ";
      //std::cout << counter << std::endl;
    }
  }
  std::cout << std::endl;




  // std::stringstream sfunc;
  // sfunc << "emg";
  // int compare_result = strcmp(fitFunction,sfunc.str());
  std::vector<float> meanc;
  std::vector<float> sigmac;
  if(fitFunction == 0)
  {
    for(unsigned int iDet = 0 ; iDet < h_det_c.size(); iDet++)
    {
      TF1 *fgaus_c = new TF1("fgaus_c", "gaus", -2e-9, 2e-9);
      h_det_c[iDet]->Fit(fgaus_c, "Q", "", -2e-9, 2e-9 );
      meanc.push_back(fgaus_c->GetParameter(1));
      sigmac.push_back(fgaus_c->GetParameter(2));
    }
  }
  else
  {
    //fit with emg
    float results_emg[4];
    for(unsigned int iDet = 0 ; iDet < h_det_c.size(); iDet++)
    {
      fitWithEMG(h_det_c[iDet],results_emg);
      meanc.push_back(results_emg[0]);
      sigmac.push_back(results_emg[1]);
    }
  }


  TH1F* hshift_avg = new TH1F("hshift_avg", "Average Shifted Timing Histogram; Number of Events", dtBins,dtMin,dtMax);
  TH1F* h_weighted_avg = new TH1F("h_weighted_avg", "Average Weighted Timing Histogram; Number of Events", dtBins,dtMin,dtMax);
  std::vector<TH1F*> h_shift;


  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    std::stringstream sname;
    sname << "Shifted t_ref - t_corr" << listDetectorChannels[iDet];
    TH1F* temp_h1 = new TH1F(sname.str().c_str(), sname.str().c_str(), bins1D_t,-2e-9, 2e-9);
    h_shift.push_back(temp_h1);
  }


  counter = 0;
  // tree->SetNotify(formulasAnalysis);
  // long long int nevent = tree->GetEntries();
  // std::cout << "Total number of events in input files = " << nevent << std::endl;
  // long int goodEventsAnalysis = 0;
  // long int counterAnalysis = 0;
  // for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  // {
    // std::cout <<listDetectorChannels[iDet] << " ";
  // }
  // std::cout << std::endl;

  for (long long int i=0;i<nevent;i++)
  {

    tree->GetEvent(i);              //read complete accepted event in memory

    //conditions
    if( (charge[refCh] >= (mean0 - charge_cut_sigmas *sigma0)) && (charge[refCh] <= (mean0 + charge_cut_sigmas *sigma0)) ) // photopeak of reference
    {
      float sumCharge = 0;
      for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
      {
        sumCharge += charge[listDetectorChannels[iDet]];
        // h_t_det[iDet]->Fill(timeStamp[listDetectorChannels[iDet]]);
      }
      if( (sumCharge >= ( mean1 - charge_cut_sigmas*sigma1) ) && (sumCharge <= ( mean1 + charge_cut_sigmas*sigma1)) )// photopeak of "sum"
      {
        if(timeStamp[refCh] != 0) // no zero for ref
        {
          bool noZeroes = true;
          for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
          {
            if(timeStamp[listDetectorChannels[iDet]] == 0)
            {
              noZeroes = false;
            }
          }
          if(noZeroes)// no zeroes for det
          {
            // time range restriction
            bool timeOK = true;
            if( (timeStamp[refCh] <= (t_ref_mean - time_cut_sigmas * t_ref_sigma)) || (timeStamp[refCh] >= (t_ref_mean + time_cut_sigmas * t_ref_sigma)) )
            {
              timeOK = false;
            }

            for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
            {
              if( (timeStamp[listDetectorChannels[iDet]] <= (t_det_mean[iDet] - time_cut_sigmas * t_det_sigma[iDet])) ||
                  (timeStamp[listDetectorChannels[iDet]] >= (t_det_mean[iDet] + time_cut_sigmas * t_det_sigma[iDet])) )
              {
                timeOK = false;
              }
            }

            if(timeOK)   // time range restriction OK
            {
              // calculate corrected timestamps
              // prepare vector
              std::vector<float> corrected_timestamp;
              for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
              {
                float temp_corr = timeStamp[listDetectorChannels[iDet]] + (flinear[iDet]->Eval(charge[listDetectorChannels[iDet]]) - tref[iDet]);
                corrected_timestamp.push_back(temp_corr + meanc[iDet]);
              }
              float t_avg = 0;
              float weight_sum = 0;
              float weights = 0;
              for(unsigned int iDet = 0 ; iDet < corrected_timestamp.size(); iDet++)
              {
                h_shift[iDet]->Fill( timeStamp[refCh] - corrected_timestamp[iDet] );
                t_avg += corrected_timestamp[iDet];

                weight_sum += (timeStamp[refCh] - corrected_timestamp[iDet])/(pow(sigmac[iDet],2));
                weights    += (1.0 / (pow(sigmac[iDet],2)) );
              }
              t_avg = t_avg/corrected_timestamp.size();
              float t_diff_shiftc = timeStamp[refCh] - t_avg;
              float avg_w_sum = weight_sum / weights;
              hshift_avg->Fill(t_diff_shiftc);
              h_weighted_avg->Fill(avg_w_sum);

            }
          }
        }
      }
    }


    counter++;
    int perc = ((100*counter)/nevent); //should strictly have not decimal part, written like this...
    if( (perc % 10) == 0 )
    {
      std::cout << "\r";
      std::cout << perc << "% done... ";
      //std::cout << counter << std::endl;
    }
  }
  std::cout << std::endl;








  TF1 *fgaust = new TF1("fgaust", "gaus", -2e-9, 2e-9);
  fgaust->SetParameter(1, h_t_base->GetMean());
  fgaust->SetParameter(2, h_t_base->GetRMS());
  h_t_base->Fit(fgaust, "Q", "", -2e-9, 2e-9);
  float CTR_uc = sqrt(2) * sqrt(pow(2.355 * fgaust->GetParameter(2),2) - pow(ref,2));

  fgaust->SetParameter(1, h_t_corrected->GetMean());
  fgaust->SetParameter(2, h_t_corrected->GetRMS());
  h_t_corrected->Fit(fgaust, "Q", "", -2e-9, 2e-9);
  float CTR_c = sqrt(2) * sqrt(pow(2.355 * fgaust->GetParameter(2),2) - pow(ref,2));

  fgaust->SetParameter(1, hshift_avg->GetMean());
  fgaust->SetParameter(2, hshift_avg->GetRMS());
  hshift_avg->Fit(fgaust, "Q", "", -2e-9, 2e-9);
  float CTR_shift = sqrt(2) * sqrt(pow(2.355 * fgaust->GetParameter(2),2) - pow(ref,2));

  fgaust->SetParameter(1, h_weighted_avg->GetMean());
  fgaust->SetParameter(2, h_weighted_avg->GetRMS());
  h_weighted_avg->Fit(fgaust, "Q", "", -2e-9, 2e-9);
  float CTR_w = sqrt(2) * sqrt(pow(2.355 * fgaust->GetParameter(2),2) - pow(ref,2));


  std::cout << "CTR (no correction)                                     = "  << CTR_uc << std::endl;
  std::cout << "CTR (amplitude correction)                              = "  << CTR_c << std::endl;
  std::cout << "CTR (amplitude correction + shift)                      = "  << CTR_shift << std::endl;
  std::cout << "CTR (amplitude correction + shift + weighted average)   = "  << CTR_c << std::endl;


  std::cout << "Saving results..." << std::endl;


  TCanvas *c_h0 = new TCanvas("c_h0","Reference Detector Spectrum",1200,800);
  c_h0->cd();
  h0->Draw();
  h0_highlight->Draw("same");

  TCanvas *c_h1 = new TCanvas("c_h1","(Sum of) SiPM(s) Spectrum",1200,800);
  c_h1->cd();
  h1->Draw();
  h1_highlight->Draw("same");

  TCanvas *c_h_t_ref = new TCanvas("c_h_t_ref","Reference Detector Time Spectrum",1200,800);
  c_h_t_ref->cd();
  h_t_ref->Draw();
  h_t_ref_highlight->Draw("same");

  std::vector<TCanvas*> c_h_t_det;
  for(unsigned int i = 0 ; i < h_t_det.size(); i++)
  {
    std::stringstream sname;
    sname << "c_h_t_ref" << listDetectorChannels[i];
    TCanvas *c_temp = new TCanvas(sname.str().c_str(),sname.str().c_str(),1200,800);
    c_temp->cd();
    h_t_det[i]->Draw();
    h_t_det_highlight[i]->Draw("same");
    c_h_t_det.push_back(c_temp);
  }

  //ROOT
  TFile *outputFile = new TFile(outputFileName_root.c_str(),"RECREATE");
  outputFile->cd();


  c_h0->Write();
  c_h1->Write();
  c_h_t_ref->Write();

  h0->Write();
  h0_highlight->Write();


  h1->Write();
  h1_highlight->Write();

  h_t_ref->Write();
  h_t_ref_highlight->Write();
  for(unsigned int i = 0 ; i < h_t_det.size(); i++)
  {
    c_h_t_det[i]->Write();
    h_t_det[i]->Write();
    h_t_det_highlight[i]->Write();
  }


  h_t_base->Write();
  h_t_corrected->Write();
  hshift_avg->Write();
  h_weighted_avg->Write();

  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    g[iDet]->Write();
    // h_det_uc[iDet]->Write();
  }
  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    // g[iDet]->Write();
    h_det_uc[iDet]->Write();
  }
  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    fx[iDet]->Write();
  }
  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    h_det_c[iDet]->Write();
  }
  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    gc[iDet]->Write();
  }
  for(unsigned int iDet = 0 ; iDet < listDetectorChannels.size(); iDet++)
  {
    h_shift[iDet]->Write();
  }
  outputFile->Close();

  std::ofstream textfile;
  textfile.open (outputFileName_text.c_str(),std::ofstream::out);

  textfile << "#V CTR_uc CTR_c CTR_shift CTR_w" << std::endl;
  textfile << voltage << " "
           << CTR_uc  << " "
           << CTR_c  << " "
           << CTR_shift  << " "
           << CTR_w  << std::endl;
  textfile.close();


  return 0;
}
