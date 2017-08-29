// g++ -o events ../events.cpp `root-config --cflags --glibs`


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

void usage()
{
  std::cout << "\t\t" << "[ -o <output file name> ] " << std::endl
            << "\t\t" << "[ --input0 <input file from V1740D digitizer> ] " << std::endl
            << "\t\t" << "[ --input1 <input file from V1740D digitizer> ] " << std::endl
            << "\t\t" << "[ --input2 <input file from V1740D digitizer> ] " << std::endl
            << "\t\t" << "[ --delta0 <clock distance between V1740D and first V1742 in nanosec> ] " << std::endl
            << "\t\t" << "[ --delta1 <clock distance between V1740D and first V1742 in nanosec> ] " << std::endl
            << "\t\t" << "[ --coincidence <coincidence window to accept an event in nanosec - default 1000> ] " << std::endl
            << "\t\t" << std::endl;
}


template <typename T>
T greatest(const std::vector<T>& vec){
    T greatest = std::numeric_limits<T>::min(); // smallest possible number
    // there are a number of ways to structure this loop, this is just one
    for (int i = 0; i < vec.size(); ++i)
    {
        greatest = std::max(greatest, vec[i]);
    }
    return greatest;
}

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
  if(argc < 2) // check input from command line
  {
    std::cout	<< "Usage: " << argv[0] << std::endl;
    usage();
    return 1;
  }

  //----------------------------//
  // PARAMETERS
  //----------------------------//
  // float tolerance = 6.0;
  //---------------------------

  gStyle->SetOptFit(1);
  // if(argc < 3)
  // {
  //   std::cout << "You need to provide 3 files!" << std::endl;
  //   return 1;
  // }

  char* file0;
  char* file1;
  char* file2;
  // file0 = argv[1];
  // file1 = argv[2];
  // file2 = argv[3];
  char* eventsFileName;
  std::vector<Data740_t> input740;
  std::vector<Data742_t> input742_0;
  std::vector<Data742_t> input742_1;

  bool input0given = false;
  bool input1given = false;
  bool input2given = false;
  bool outputFileNameGiven = false;

  double delta740to742_0 = 0.0;
  double delta740to742_1 = 0.0;
  double coincidence = 1000.0;

  static struct option longOptions[] =
  {
			{ "input0", required_argument, 0, 0 },
      { "input1", required_argument, 0, 0 },
      { "input2", required_argument, 0, 0 },
      { "delta0", required_argument, 0, 0 },
      { "delta1", required_argument, 0, 0 },
      { "coincidence", required_argument, 0, 0 },
			{ NULL, 0, 0, 0 }
	};

  while(1) {
		int optionIndex = 0;
		int c = getopt_long(argc, argv, "o:", longOptions, &optionIndex);
		if (c == -1) {
			break;
		}
		if (c == 'o'){
			eventsFileName = (char *)optarg;
      outputFileNameGiven = true;
    }
    else if (c == 0 && optionIndex == 0){
      file0 = (char *)optarg;
      input0given = true;
    }
    else if (c == 0 && optionIndex == 1){
      file1= (char *)optarg;
      input1given = true;
    }
    else if (c == 0 && optionIndex == 2){
      file2 = (char *)optarg;
      input2given = true;
    }
    else if (c == 0 && optionIndex == 3){
      delta740to742_0 = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 4){
      delta740to742_1 = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 5){
      coincidence = atof((char *)optarg);
    }
		else {
      std::cout	<< "Usage: " << argv[0] << std::endl;
			usage();
			return 1;
		}
	}

  if( (!input0given) || (!input1given) || (!input2given) )
  {
    std::cout	<< "Usage: " << argv[0] << std::endl;
    usage();
    return 1;
  }

  if(!outputFileNameGiven)
  {
    eventsFileName = "events.dat";
  }
  std::cout	<< "\n" << "Sorted events will be saved in file " << eventsFileName << std::endl;

  // TCanvas *c1 = new TCanvas("c1","c1",1200,800);   //useless but avoid root output



  Data740_t ev740;
  Data742_t ev742_0;
  Data742_t ev742_1;

  FILE * in0 = NULL;
  FILE * in1 = NULL;
  FILE * in2 = NULL;

  //calculate number of events in each file
  long int file0N = filesize(file0) /  sizeof(ev740);
  long int file1N = filesize(file1) /  sizeof(ev742_0);
  long int file2N = filesize(file2) /  sizeof(ev742_1);
  std::cout << "File " << file0 << "\t = " <<  file0N << " events" <<std::endl;
  std::cout << "File " << file1 << "\t = " <<  file1N << " events" <<std::endl;
  std::cout << "File " << file2 << "\t = " <<  file2N << " events" <<std::endl;

  in0 = fopen(file0, "rb");
  in1 = fopen(file1, "rb");
  in2 = fopen(file2, "rb");

  long long int counter = 0;
  long long int MatchingEvents = 0;

  FILE *eventsFile;
  eventsFile = fopen(eventsFileName,"wb");

  // std::cout << std::endl;
  long long int fileCounter[3] = {0,0,0};
  bool GetNextEvent[3] = {true,true,true};
  double seedTime = 0;
  bool GetNewSeed = true;
  // long long int counter;
  bool thereIsStillData = true;
  while(thereIsStillData) //sorting loop
  {
    counter++;
    EventFormat_t event;
    bool isSeed[3] = {false,false,false};

    if(GetNextEvent[0])
    {
      fileCounter[0]++;
      if(fread((void*)&ev740, sizeof(ev740), 1, in0) != 1 )
        thereIsStillData = false;
    }
    if(GetNextEvent[1])
    {
      fileCounter[1]++;
      if(fread((void*)&ev742_0, sizeof(ev742_0), 1, in1) != 1 )
        thereIsStillData = false;
    }
    if(GetNextEvent[2])
    {
      fileCounter[2]++;
      if( fread((void*)&ev742_1, sizeof(ev742_1), 1, in2) != 1)
        thereIsStillData = false;
    }


    // std::cout << std::fixed << std::showpoint << std::setprecision(4) << counter << "\t" << ev740.TTT << "\t" << ev742_0.TTT[0] << "\t" << ev742_1.TTT[0] << std::endl;
    // std::cout << counter << "\r";
    GetNextEvent[0] = false;
    GetNextEvent[1] = false;
    GetNextEvent[2] = false;

    // if(!thereIsStillData)
    //   break;
    // if(counter > 50)
    //   break;

    // look for seed time if there is not already one
    std::vector<double> vec;
    vec.push_back(ev740.TTT);
    vec.push_back(ev742_0.TTT[0] + delta740to742_0);
    vec.push_back(ev742_1.TTT[0] + delta740to742_1);
    if(GetNewSeed)
    {
      seedTime = greatest<double>(vec);
      GetNewSeed = false;
    }


    //find who is the seed
    for(int i = 0 ; i < vec.size(); i++)
    {
      if(seedTime == vec[i])
        isSeed[i] = true;
    }
    // std::cout << "\t" << seedTime << "\t"<< isSeed[0] << isSeed[1] << isSeed[2] << std::endl;
    // check if the other two times are in the matching window
    bool matchingEvent[3] = {true,true,true};
    for(int i = 0 ; i < vec.size(); i++)
    {
      // std::cout << "\t" << seedTime << " " << vec[i] << " " << fabs(seedTime - vec[i]) << " ";
      if(fabs(seedTime - vec[i]) > coincidence )
        matchingEvent[i] = false;
      // std::cout << matchingEvent[i] << std::endl;

    }
    // std::cout <<std::endl;
    if(matchingEvent[0] && matchingEvent[1] && matchingEvent[2])// if they are, save the event and set flags to get next
    {
      event.TTT740 = ev740.TTT;  //put the TTT of 740
      event.TTT742_0 = ev742_0.TTT[0];  //FIXME correct by delta?
      event.TTT742_1 = ev742_1.TTT[0];  //FIXME correct by delta?
      for(int j = 0 ; j < 64 ; j++)
      {
        event.Charge[j] = ev740.Charge[j];  // put the charges
      }
      for(int j = 0 ; j < 32 ; j++)
      {
        event.PulseEdgeTime[j] = ev742_0.PulseEdgeTime[j];
      }
      for(int j = 32 ; j < 64 ; j++)
      {
        event.PulseEdgeTime[j] = ev742_1.PulseEdgeTime[j-32];
      }

      fwrite(&event,sizeof(event),1,eventsFile);
      GetNextEvent[0] = true;
      GetNextEvent[1] = true;
      GetNextEvent[2] = true;
      GetNewSeed = true;
      MatchingEvents++;
    }
    else// if they are not,
    {
      // check the one(s) not in the matching window
      for(int i = 0 ; i < vec.size(); i++)
      {
        if(!matchingEvent[i])
        {
          if(vec[i] < seedTime) // all is fine, the digitizer not in matching window is behind in time, and we skip this event
          {
            GetNextEvent[i] = true;
          }
        }
        else // the other digitizer(s) are
        {
          // freeze the buffer and repeat while loop, but looking for another seed. this way
          // the digitizer that jumped in front will force the other one (or two) to follow
          GetNewSeed = true;
        }
      }
    }
    if( (fileCounter[0] % file0N)  == 0)
    {
      std::cout << 100 * fileCounter[0]/file0N << "%\t"
                << 100 * fileCounter[1]/file1N << "%\t"
                << 100 * fileCounter[2]/file2N << "%\t"
                << "\r";
    }
  }

  std::cout << std::endl;

//
//   //close everything
  std::cout << "Coincidences found = "<<MatchingEvents << std::endl;
  fclose(in0);
  fclose(in1);
  fclose(in2);
  fclose(eventsFile);

  return 0;
}
