Acquisition software for combined V1740D and V1742

Configuration file -> config.txt 
Key meanings are explained in the file

Create a bin subfolder and compile from there with

gcc -DLINUX ../src/readout.c ../src/dpp_qdc.c ../src/_CAENDigitizer_DPP-QDC.c  ../src/X742CorrectionRoutines.c -o readout -lCAENDigitizer -lCAENComm -fpermissive -w

Run acquisition from bin/ with

./readout ../config.txt 100

where 100 here is the duration in seconds of the acquisition. Leave empty for infinite acquisition.

Acquisition starts pressing "s". It can be stopped pressing "q".


Events are sorted by program sort. Enter in bin/ and compile with 

g++ -o sort ../sort.cpp `root-config --cflags --glibs`

sort will cycle over the first 5000 events to find the time offsets, then restart from the beginning of the dataset and import
data using the offset previously found to correct the trigger time tags. It will then sort the events of the different boards in 
global events, using as criterium the corrected trigger time tags. The events will be saved in a file called 

events.dat

while some histograms are saved in the root file

histograms.root

The binary files can be read with binRead740, binRead742, binReadEvent programs. They can be compiled with commands written in the cpp files themselves.
These programs are meant for debugging purposes.