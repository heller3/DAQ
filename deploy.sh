#!/bin/bash

mkdir -p bin
cd bin
gcc -DLINUX ../src/readout.c ../src/dpp_qdc.c ../src/_CAENDigitizer_DPP-QDC.c  ../src/X742CorrectionRoutines.c -o readout -lCAENDigitizer -lCAENComm -fpermissive -w -lm
g++ -o binRead740 ../binRead740.cpp
g++ -o binRead742 ../binRead742.cpp
g++ -o binReadEvent ../binReadEvent.cpp
g++ -o binReadWave ../binReadWave.cpp
g++ -o interpolate ../interpolate.cpp


g++ -o sortEvents ../sortEvents.cpp `root-config --cflags --glibs`
g++ -o events ../events.cpp `root-config --cflags --glibs`
g++ -o convertToRoot740 ../convertToRoot740.cpp `root-config --cflags --glibs`
g++ -o convertToRoot ../convertToRoot.cpp `root-config --cflags --glibs`
#g++ -o bin/intrinsic intrinsic.cpp `root-config --cflags --glibs` -Wl,--no-as-needed -lHist -lCore -lMathCore -lTree -lTreePlayer -lSpectrum
#g++ -o bin/thScan thScan.cpp `root-config --cflags --glibs` -Wl,--no-as-needed -lHist -lCore -lMathCore -lTree -lTreePlayer -lSpectrum


g++ -o boardPerformance ../boardPerformance.cpp `root-config --cflags --glibs`
#g++ -o ./bin/calculate_ctr calculate_ctr.cpp `root-config --cflags --glibs` -Wl,--no-as-needed -lHist -lCore -lMathCore -lTree -lTreePlayer -lgsl -lgslcblas -lSpectrum


cd -
