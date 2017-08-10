// g++ -o interpolate ../interpolate.cpp 

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




// ASSUMING NIM PULSE for trigger and negative pulse for nino, I.E. negative pulses on both trigger and signal
double interpolateWithMultiplePoints(float* data, unsigned int length, int threshold, CAEN_DGTZ_TriggerPolarity_t edge, double Tstart,int WaveType)
{
  // WaveType = 0   -> Pulse wave
  // WaveType = 1   -> Trigger wave
  
  unsigned int i,j;
  double crosspoint = -1.0;
  
  //find min and max of this wave
  float min = FLT_MAX;
  float max = -FLT_MAX;
  for (i=0; i < length; i++) {
    if(data[i] < min ) min = data[i];
    if(data[i] > max ) max = data[i];
  }
  
  //set the flex point to 50% vertical swing
  //we compute a distance of 50%
//   float halfDistance = fabs(min-max) / 2.0;
  
//   if(halfDistance < 300.0 ) return crosspoint;
  
  // first find the baseline for this wave
  int baseLineSamples = 300; 
  double baseline = 0.0;
  for (i=(int)(Tstart); i < baseLineSamples; ++i) {
    baseline += ((double)data[i])/((double) baseLineSamples);
  }
  
  //find the pulse beging and end, i.e. when the signal crosses a value that is more than  
  
//   printf("Baseline = %f\n",baseline);
  //then re-define the threshold as a distance 1100 from baseline
  //not general implementation, works well only for identical signals of this peculiar amplitude 
  //double thresholdReal = (double) threshold;
  
//   printf("%f\n",halfDistance);
//   halfDistance = 700.0;
  double distanceToFlexPoint;
  if(WaveType == 0)
    distanceToFlexPoint = 800;
  if(WaveType == 1)
    distanceToFlexPoint = 650;
  double thresholdReal = baseline - distanceToFlexPoint;
  
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
      std::cout << std::endl;
//       counter++;
    }
  }
  
  
  for(int f = 0; f < (NumOfV1742*groups*channelsPerGroup) ; f++){ //loop on waveFile
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
      std::cout << std::endl;
//       counter++;
    }
  }
  
  FILE **      binOut742;
  binOut742 = (FILE**) malloc ( NumOfV1742 * sizeof(FILE*));
  char b742FileName[50];
  
  for(j=0; j<gParams.NumOfV1742;j++)
  {
      sprintf(b742FileName,"offline_binary742_%d.dat",j);
      binOut742[j] = fopen(b742FileName,"wb");
  }
  int WRITE_DATA = 1;
  
  for(int ev = 0; ev < triggerData[0].size(); ev++) // run on events
  {
    for(i = 0 ; i < gParams.NumOfV1742 ; i++) // loop on digitizers
    {
      for(gr = 0 ; gr < 4 ; gr++) // loop on groups
      {
        if(WRITE_DATA)
        {
//           if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
            Data742[i].TTT[gr] = TTT[i][gr];
//           if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
//             fprintf(sStamps742[i],"%f ", TTT[i][gr]);
        }
        
        double TriggerEdgeTime = interpolateWithMultiplePoints(Wave, nSamples, gParams.v1742_TRThreshold[i], gParams.v1742_TriggerEdge, 0,1); // interpolate with line, Type = 1 means trigger wave 
        
        for(ch = 0 ; ch < 8 ; ch ++) // loop on channels
        {
          double PulseEdgeTime = interpolateWithMultiplePoints(Wave, nSamples, gParams.v1742_ChannelThreshold[i], gParams.v1742_ChannelPulseEdge[i], 0,0); // interpolate with line, Type = 0 means pulse wave 
          
          if(PulseEdgeTime >= 0)
          {
            PulseEdgeTime = TriggerEdgeTime - PulseEdgeTime;
            if(WRITE_DATA)
            {
//               if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
                Data742[i].PulseEdgeTime[gr * 8 + ch] = PulseEdgeTime;
             /* if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
                fprintf(sStamps742[i],"%f ",PulseEdgeTime);       */      
            }
          }
          else
          {
            if(WRITE_DATA)
            {
//               if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
                Data742[i].PulseEdgeTime[gr * 8 + ch] = 0;
              /*if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
                fprintf(sStamps742[i],"%f ",0);   */          
            }
          }
        }
        
      }
      
      if(WRITE_DATA)
      {
//         if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
//           fprintf(sStamps742[i],"\n"); 
//         if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
          fwrite(&Data742[i],sizeof(Data742_t),1,binOut742[i]);
      }
      
      
    }
  }
  
  for(i=0; i<NumOfV1742; i++) {  
//       if(gParams.OutputMode == OUTPUTMODE_TEXT | gParams.OutputMode == OUTPUTMODE_BOTH)
//         fclose(sStamps742[i]);
//       if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
        fclose(binOut742[i]);
    }
//     if(gParams.OutputMode == OUTPUTMODE_BINARY | gParams.OutputMode == OUTPUTMODE_BOTH)
      free(binOut742);
  
  
  
//   std::cout << "Waves in file " << file0 << " = " << counter << std::endl;


//   fclose(fIn);
  
  return 0; 
}