// g++ -o interpolate ../interpolate.cpp

/*
program to test interpolation methods
for the moment, finds 2 "baselines" and compute edge time as the time of passing the half distance between them
FIXME
the second baseline is approx (computation includes pat of the rise and of the fall)
the first baseline suffers from non ideal corrections (probably)
need to skip first bin of the wave (sometimes it's an outlier, need to find a workaround)

run this program in the folders where the waves are (waves name are hardcoded!!), then run the sort program on the results
*/

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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <ctime>

#include <float.h>  //FLT_MAX
#define DEBUG 0

// struct EventFormat_t
// {
//   double GlobalTTT;                         /*Trigger time tag of the event. For now, it's the TTT of the 740 digitizer */
//   uint16_t Charge[64];                      /*Integrated charge for all the channels of 740 digitizer*/
//   double PulseEdgeTime[64];                 /*PulseEdgeTime for each channel in both timing digitizers*/
// } __attribute__((__packed__));

struct WaveFormat_t
{
  double TTT;
  float sample[1024];
};

typedef struct
{
  double TTT[4];                              /*Trigger time tag of the groups for this board */
  double PulseEdgeTime[32];                 /*PulseEdgeTime for each channel in the group*/
} Data742_t;

typedef enum {
    CAEN_DGTZ_TriggerOnRisingEdge        = 0L,
    CAEN_DGTZ_TriggerOnFallingEdge        = 1L,
} CAEN_DGTZ_TriggerPolarity_t;

// ASSUMING NIM PULSE for trigger and negative pulse for nino, I.E. negative pulses on both trigger and signal
double interpolateWithMultiplePoints(float* data, unsigned int length, int threshold, CAEN_DGTZ_TriggerPolarity_t edge, double Tstart,int baseLineSamples)
{

  unsigned int i,j;
  double crosspoint = -1.0;
  double baseline = 0.0;
  float minBaseline = FLT_MAX;
  float maxBaseline = -FLT_MAX;
  for (i=(int)(Tstart); i < baseLineSamples; i++) {
    baseline += ((double)data[i])/((double) baseLineSamples);
    if(data[i] < minBaseline ) minBaseline = data[i];
    if(data[i] > maxBaseline ) maxBaseline = data[i];
  }
  float widthBaseline = fabs(maxBaseline-minBaseline);

  int squareStartPoint = 0;
  int squareEndPoint = length-1;
  int foundStart = 0;
  int foundEnd = 0;
  double secondBaseline = 0;
  double timesBase = 1.0;
  
  //look for start of fall or rise of the pulse
  for (i=(int)(Tstart); i < length-1; i++) {


    if ((edge == CAEN_DGTZ_TriggerOnFallingEdge))
    {
      if(!foundStart)
      {
        if(data[i+1] < (baseline - fabs( timesBase*widthBaseline ) ) )
        {
          squareStartPoint = i;
          foundStart = 1;
        }
      }
    }

    if ((edge == CAEN_DGTZ_TriggerOnRisingEdge))
    {
      if(!foundStart)
      {
        if(data[i+1] > (baseline + fabs( timesBase*widthBaseline ) ) )
        {
          squareStartPoint = i;
          foundStart = 1;
        }
      }
    }

    if(foundStart)
    {
      break;
    }

  }

  if(!foundStart) return -1;
  
  int startSecondBaseline = squareStartPoint + 90; // extremely hardcoded - circa 800
  int endSecondBaseline = startSecondBaseline + 50; // extremely hardcoded - circa 850
  
  for (i=startSecondBaseline; i < endSecondBaseline; i++) 
  {
    secondBaseline += ((double)data[i])/((double) (endSecondBaseline - startSecondBaseline)); // i.e. 50
  }

  double thresholdReal = (baseline + secondBaseline) / 2.0;
  int samplesNum = 4; // N = 3
  //now take +/- N bins and calculate the regression lines
  float slope = 0.0;
  float intercept = 0.0;
  float sumx =0.0;
  float sumy =0.0;
  float sumxy=0.0;
  float sumx2=0.0;
  int n = samplesNum * 2;
//   int crossSample = -1;

  for (i=(int)(Tstart); i < length-1; ++i) {
    if ((edge == CAEN_DGTZ_TriggerOnFallingEdge) && (data[i] >= thresholdReal) && (data[i+1] < thresholdReal)) 
    {
//       crossSample = i;
      for(j = i - samplesNum; j < i + samplesNum; j++) 
      {
        sumx += (float) j;
        sumy += data[j];
        sumxy += j*data[j];
        sumx2 += j*j;
      }
      slope =  (n*sumxy - sumx*sumy) / (n*sumx2 - sumx*sumx);
      intercept = (sumy - slope * sumx) / (n);
      crosspoint = (thresholdReal - intercept) / slope;
      //crosspoint = i + ((double)(data[i] - threshold)/(double)(data[i]-data[i+1]));
      break;
    }
    if ((edge == CAEN_DGTZ_TriggerOnRisingEdge) && (data[i] <= thresholdReal) && (data[i+1] > thresholdReal)) 
    {
//       crossSample = i;
      for(j = i - samplesNum; j < i + samplesNum; j++) 
      {
        sumx += (float) j;
        sumy += data[j];
        sumxy += j*data[j];
        sumx2 += j*j;
      }
      slope =  (n*sumxy - sumx*sumy) / (n*sumx2 - sumx*sumx);
      intercept = (sumy - slope * sumx) / (n);
      crosspoint = (thresholdReal - intercept) / slope;
      //crosspoint = i + ((double)(threshold - data[i])/(double)(data[i+1]-data[i])) ;
      break;
    }
  }
  return crosspoint;
}




//----------------//
//  MAIN PROGRAM  //
//----------------//
int main(int argc,char **argv)
{
  
  // files input standard
  const int groups = 4;
  const int channelsPerGroup = 8;
  const int NumOfV1742 = 2;
  int gr,ch,j;
  FILE **waveFile;
  FILE **trFile;

  bool alternativeReference = false;
  int baseLineSamples = 600;
  if(argc > 1)
  {
    alternativeReference = atoi(argv[1]);
  }


  waveFile = (FILE**) malloc ( (NumOfV1742 * groups * channelsPerGroup) * sizeof(FILE*));
  trFile =   (FILE**) malloc ( (NumOfV1742 * groups) * sizeof(FILE*));

  for(j=0;j<NumOfV1742;j++) // loop on 742 digitizers
  {
    for(gr = 0 ; gr < groups ; gr++)
    {
      char TrFileName[30];
      sprintf(TrFileName,"waveTR%d.dat",j*groups + gr); // TRn
      trFile[j*groups + gr] = fopen(TrFileName,"rb");
      for(ch = 0 ; ch < channelsPerGroup ; ch++)
      {
        char waveFileName[30];
        sprintf(waveFileName,"waveCh%d.dat",j*(groups*channelsPerGroup)+gr*(channelsPerGroup)+ch);
        waveFile[j*(groups*channelsPerGroup)+gr*(channelsPerGroup)+ch] = fopen(waveFileName,"rb");
      }
    }


  }


  std::vector<WaveFormat_t> waveData[groups*channelsPerGroup*NumOfV1742];
  std::vector<WaveFormat_t> triggerData[groups*channelsPerGroup];


//   long long int counterTR[groups*channelsPerGroup];
//   for(int i = 0 ; i < groups*channelsPerGroup ; i++)
//   {
//     counterTR[i] = 0;
//   }
//   long long int counterWave[groups*channelsPerGroup*NumOfV1742];
//   for(int i = 0 ; i < groups*channelsPerGroup*NumOfV1742 ; i++)
//   {
//     counterWave[i] = 0;
//   }
//   for (int i = 0 ; i < triggerData


  FILE **      binOut742;
  binOut742 = (FILE**) malloc ( NumOfV1742 * sizeof(FILE*));
  char b742FileName[100];
  
  for(j=0; j<NumOfV1742;j++)
  {
    if(alternativeReference)
      sprintf(b742FileName,"alternativeReference_offline_binary742_%d.dat",j);
    else
      sprintf(b742FileName,"offline_binary742_%d.dat",j);
    binOut742[j] = fopen(b742FileName,"wb");
  }
  std::cout << "Reading files..." <<std::endl;
//   for(int f = 0; f < (NumOfV1742*groups) ; f++)
    
  bool thereIsStillData = true;
  long long int counter = 0;
  
  double durationTR;
  long long int counterTR = 0;
  double durationWave;
  long long int counterWave = 0;
  
  while(thereIsStillData)
  {
    for(j=0;j<NumOfV1742;j++) // loop on digitizers
    {
      Data742_t Data742; //prepare data output
      for(gr = 0 ; gr < groups ; gr++) // loop on groups
      {
        WaveFormat_t trWave;
        if((fread((void*)&trWave, sizeof(trWave), 1, trFile[j*groups + gr])) != 1) //read and check if there is still data
          thereIsStillData = false;
        Data742.TTT[gr] = trWave.TTT;
        float* WaveT = trWave.sample;
        int nSamples = 1024;
        std::clock_t start;
        start = std::clock();
        
        // temp mod: force trigger to be ch0 
        
        double TriggerEdgeTime = interpolateWithMultiplePoints(WaveT, nSamples, 700,CAEN_DGTZ_TriggerOnFallingEdge , 0,baseLineSamples); // interpolate with line, Type = 1 means trigger wave
        durationTR += (std::clock() - start)/((double) CLOCKS_PER_SEC);
        counterTR++;
        
        for(ch = 0 ; ch < channelsPerGroup ; ch++) // loop on channels
        {
          WaveFormat_t chWave;
          if((fread((void*)&chWave, sizeof(chWave), 1, waveFile[j*(groups*channelsPerGroup)+gr*(channelsPerGroup)+ch])) != 1) //read and check if there is still data
            thereIsStillData = false;
          
          if(alternativeReference)
          {
            if( (j*(groups*channelsPerGroup)+gr*(channelsPerGroup)+ch) == 0 ) //only ch0 as reference, so only for first v1742 and first gr is affected
            {
              
              float* AltWave = chWave.sample;
              TriggerEdgeTime = interpolateWithMultiplePoints(AltWave, nSamples, 700,CAEN_DGTZ_TriggerOnFallingEdge , 0,baseLineSamples); //overwrite TriggerEdgeTime
            }
          }
          
          float* WaveP = chWave.sample;
          start = std::clock();
          double PulseEdgeTime = interpolateWithMultiplePoints(WaveP, nSamples, 700,CAEN_DGTZ_TriggerOnFallingEdge , 0,baseLineSamples); // interpolate with line, Type = 0 means pulse wave
          durationWave += (std::clock() - start)/((double) CLOCKS_PER_SEC);
          counterWave++;
//           if(DEBUG)
//             std::cout << std::endl;
          if(PulseEdgeTime >= 0)
          {

            PulseEdgeTime = TriggerEdgeTime - PulseEdgeTime;
            Data742.PulseEdgeTime[gr * channelsPerGroup + ch] = PulseEdgeTime;
          }
          else
          {
            Data742.PulseEdgeTime[gr * channelsPerGroup + ch] = 0;
          }
        }
      }
      fwrite(&Data742,sizeof(Data742_t),1,binOut742[j]);
    }
    if((counter % 500) == 0)
      std::cout << counter << "\r" <<std::flush;
    counter++;
  }
  std::cout << std::endl;
  
  durationTR = durationTR / ((double) counterTR);
  durationWave = durationWave / ((double) counterWave);
  
  std::cout << "durationTR   = " << durationTR << std::endl;
  std::cout << "durationWave = " << durationWave << std::endl;
  unsigned int i;
  for(i=0; i<NumOfV1742; i++) {
    fclose(binOut742[i]);
  }
  free(binOut742);



//   std::cout << "Waves in file " << file0 << " = " << counter << std::endl;


//   fclose(fIn);

  return 0;
}
