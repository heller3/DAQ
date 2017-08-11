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

#include <float.h>  //FLT_MAX


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
double interpolateWithMultiplePoints(float* data, unsigned int length, int threshold, CAEN_DGTZ_TriggerPolarity_t edge, double Tstart,int WaveType,int baseLineSamples)
{
  // WaveType = 0   -> Pulse wave
  // WaveType = 1   -> Trigger wave



  unsigned int i,j;
  double crosspoint = -1.0;

  // test with histogram

  // int histo[4096];

  //find min and max of this wave
  float min = FLT_MAX;
  float max = -FLT_MAX;
  for (i=0; i < length; i++) {
    if(data[i] < min ) min = data[i];
    if(data[i] > max ) max = data[i];
    // int HistoBin = (int) data[i];
    // histo[HistoBin]++;
  }

  //set the flex point to 50% vertical swing
  //we compute a distance of 50%
//   float halfDistance = fabs(min-max) / 2.0;

//   if(halfDistance < 300.0 ) return crosspoint;

  // first find the baseline for this wave
  // int baseLineSamples = 200;
  double baseline = 0.0;
  float minBaseline = FLT_MAX;
  float maxBaseline = -FLT_MAX;
  // double stdevBaseline = 0.0;
  for (i=(int)(Tstart); i < baseLineSamples; ++i) {
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
  double timesBase = 4.0;

  for (i=(int)(Tstart); i < length-1; ++i) {


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

  for (i = length-1; i > squareStartPoint; --i) {


    if ((edge == CAEN_DGTZ_TriggerOnFallingEdge))
    {
      if(!foundEnd)
      {
        if(data[i-1] < (baseline - fabs( timesBase*widthBaseline ) ) )
        {
          squareEndPoint = i;
          foundEnd = 1;
        }
      }
    }

    if ((edge == CAEN_DGTZ_TriggerOnRisingEdge))
    {
      if(!foundEnd)
      {
        if(data[i-1] > (baseline + fabs( timesBase*widthBaseline ) ) )
        {
          squareEndPoint = i;
          foundEnd = 1;
        }
      }
    }

    if(foundEnd)
    {
      break;
    }

  }

  if(!foundEnd) squareEndPoint = length - 1;

  if(squareEndPoint < squareStartPoint)
  {
    std::cout << "error" << std::endl;
    return -1;
  }

  // if((squareEndPoint - squareStartPoint) < 100) //too short
  // {
  //   std::cout << "short" << std::endl;
  //   return -1;
  // }

  for (i=squareStartPoint; i < squareEndPoint; ++i) {
    secondBaseline += data[i] / ((double)(squareEndPoint - squareStartPoint ));
  }


  // secondBaseline = secondBaseline / counterSecondBaseline;
  // std::cout << baseline << " " << widthBaseline << " " << secondBaseline << std::endl;
  //find the pulse beging and end, i.e. when the signal crosses a value that is more than

//   printf("Baseline = %f\n",baseline);
  //then re-define the threshold as a distance 1100 from baseline
  //not general implementation, works well only for identical signals of this peculiar amplitude
  //double thresholdReal = (double) threshold;

//   printf("%f\n",halfDistance);
//   halfDistance = 700.0;
  // double distanceToFlexPoint;
  // if(WaveType == 0)
  //   distanceToFlexPoint = 800;
  // if(WaveType == 1)
  //   distanceToFlexPoint = 650;
  // double thresholdReal = baseline - distanceToFlexPoint;

  double thresholdReal = (baseline + secondBaseline) / 2.0;

  int samplesNum = 7; // N = 3
  //now take +/- N bins and calculate the regression lines
  float slope = 0;
  float intercept = 0;
  float sumx =0;
  float sumy =0;
  float sumxy=0;
  float sumx2=0;
  int n = samplesNum * 2;
//   for(j = centerBin[0] - samplesNum; j < centerBin[0] + samplesNum; j++)
//   {
//     sumx += (float) j;
//     sumy += wave0[j];
//     sumxy += j*wave0[j];
//     sumx2 += j*j;
//   }
//   slope[0] =  (n*sumxy - sumx*sumy) / (n*sumx2 - sumx*sumx);
//   intercept[0] = (sumy - slope[0] * sumx) / (n);

  // std::cout << WaveType << " "
  //         << squareStartPoint << " "
  //         << squareEndPoint << " "
  //         << baseline << " "
  //         << widthBaseline << " "
  //         << secondBaseline << " "
  //         << thresholdReal << std::endl;

  for (i=(int)(Tstart); i < length-1; ++i) {
    if ((edge == CAEN_DGTZ_TriggerOnFallingEdge) && (data[i] >= thresholdReal) && (data[i+1] < thresholdReal)) {
      for(j = i - samplesNum; j < i + samplesNum; j++) {
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
    if ((edge == CAEN_DGTZ_TriggerOnRisingEdge) && (data[i] <= thresholdReal) && (data[i+1] > thresholdReal)) {
      for(j = i - samplesNum; j < i + samplesNum; j++) {
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


  int baseLineSamples = 300;
  if(argc > 1)
  {
    baseLineSamples = atoi(argv[1]);
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


  long long int counter = 0;


  for(int f = 0; f < (NumOfV1742*groups) ; f++)
  { //loop on trFile
    WaveFormat_t wave;
    while(fread((void*)&wave, sizeof(wave), 1, trFile[f]) == 1)
    {
      triggerData[f].push_back(wave);

//       std::cout << wave.TTT << " " ;
//       for(int i = 0 ; i < 1024 ; i ++)
//       {
//         std::cout << wave.sample[i] << " ";
//       }
      /*
       * std::cout << std::fixed << std::showpoint << std::setprecision(4) << ev.GlobalTTT << " ";
       * for(int i = 0 ; i < 64 ; i ++)
       * {
       *   std::cout << ev.Charge[i] << " ";
    }
    for(int i = 0 ; i < 64 ; i ++)
    {
    std::cout << std::setprecision(6) << ev.PulseEdgeTime[i] << " ";
    }*/
      // std::cout << std::endl;
//       counter++;
    }
  }


  for(int f = 0; f < (NumOfV1742*groups*channelsPerGroup) ; f++) //loop on waveFile
  {
    WaveFormat_t wave;
    while(fread((void*)&wave, sizeof(wave), 1, waveFile[f]) == 1)
    {
      waveData[f].push_back(wave);
//       std::cout << wave.TTT << " " ;
//       for(int i = 0 ; i < 1024 ; i ++)
//       {
//         std::cout << wave.sample[i] << " ";
//       }
      /*
       * std::cout << std::fixed << std::showpoint << std::setprecision(4) << ev.GlobalTTT << " ";
       * for(int i = 0 ; i < 64 ; i ++)
       * {
       *   std::cout << ev.Charge[i] << " ";
    }
    for(int i = 0 ; i < 64 ; i ++)
    {
    std::cout << std::setprecision(6) << ev.PulseEdgeTime[i] << " ";
    }*/
      // std::cout << std::endl;
//       counter++;
    }
  }

  FILE **      binOut742;
  binOut742 = (FILE**) malloc ( NumOfV1742 * sizeof(FILE*));
  char b742FileName[50];



  for(j=0; j<NumOfV1742;j++)
  {
    sprintf(b742FileName,"offline_binary742_%d.dat",j);
    binOut742[j] = fopen(b742FileName,"wb");
  }

  int WRITE_DATA = 1;

  for(int ev = 0; ev < triggerData[0].size(); ev++) // run on events
  // for(int ev = 0; ev < 1; ev++)
  {
    Data742_t *Data742;
    Data742 = (Data742_t*) malloc ( NumOfV1742 * sizeof(Data742_t));

    unsigned int i;
    for(i = 0 ; i < NumOfV1742 ; i++) // loop on digitizers
    {
      for(gr = 0 ; gr < 4 ; gr++) // loop on groups
      {
        // std::cout << "event " << ev << " tr " << i*4 + gr << std::endl;
        Data742[i].TTT[gr] = triggerData[i*groups+gr].at(ev).TTT;
        float* WaveT = triggerData[i*groups+gr].at(ev).sample;
        int nSamples = 1024;
        double TriggerEdgeTime = interpolateWithMultiplePoints(WaveT, nSamples, 700,CAEN_DGTZ_TriggerOnFallingEdge , 1,1,baseLineSamples); // interpolate with line, Type = 1 means trigger wave
        // std::cout << "TriggerEdgeTime " << TriggerEdgeTime  << std::endl;
        for(ch = 0 ; ch < 8 ; ch ++) // loop on channels
        {
          // std::cout << "event " << ev << " ch " << i*32 + gr * 8 + ch  << std::endl;
          float* WaveP = waveData[i*groups*channelsPerGroup+gr*channelsPerGroup+ch].at(ev).sample;
          double PulseEdgeTime = interpolateWithMultiplePoints(WaveP, nSamples, 700,CAEN_DGTZ_TriggerOnFallingEdge , 1,0,baseLineSamples); // interpolate with line, Type = 0 means pulse wave
          // std::cout << "PulseEdgeTime " << PulseEdgeTime  << std::endl;
          if(PulseEdgeTime >= 0)
          {

            PulseEdgeTime = TriggerEdgeTime - PulseEdgeTime;
            //check
            // if(fabs(PulseEdgeTime) > 400)
            // {
            //   std::cout << "event " << ev << " ch " << i*32 + gr * 8 + ch << " " << PulseEdgeTime << " " << TriggerEdgeTime << std::endl;
            // }
            Data742[i].PulseEdgeTime[gr * 8 + ch] = PulseEdgeTime;
          }
          else
          {
            Data742[i].PulseEdgeTime[gr * 8 + ch] = 0;
          }
        }

      }

      fwrite(&Data742[i],sizeof(Data742_t),1,binOut742[i]);


    }
    if((ev % 100) == 0)
      std::cout << ev << "\r" << std::flush;
  }
  std::cout << std::endl;
  unsigned int i;
  for(i=0; i<NumOfV1742; i++) {
    fclose(binOut742[i]);
  }
  free(binOut742);



//   std::cout << "Waves in file " << file0 << " = " << counter << std::endl;


//   fclose(fIn);

  return 0;
}
