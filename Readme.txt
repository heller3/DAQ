Acquisition software for combined V1740D and V1742


----------------------------
|         ACQUSITION       |
----------------------------


Configuration file -> config.txt 
Key meanings are explained in the file

Create a bin subfolder and compile from there with

gcc -DLINUX ../src/readout.c ../src/dpp_qdc.c ../src/_CAENDigitizer_DPP-QDC.c  ../src/X742CorrectionRoutines.c -o readout -lCAENDigitizer -lCAENComm -fpermissive -w

Run acquisition from bin/ with

./readout ../config.txt 100

where 100 here is the duration in seconds of the acquisition. Leave empty for infinite acquisition.

Acquisition starts pressing "s". It can be stopped pressing "q".



----------------------------
|        SORTING           |
----------------------------


Events are sorted by program "events". Enter in bin/ and compile with 

g++ -o events ../events.cpp `root-config --cflags --glibs`

if run without arguments, events provide the list of possible arguments, like this:

Usage: events
		[ -o <output file name> ] 
		[ --input0 <input file from V1740D digitizer> ] 
		[ --input1 <input file from V1740D digitizer> ] 
		[ --input2 <input file from V1740D digitizer> ] 
		[ --delta0 <clock distance between V1740D and first V1742 in nanosec> ] 
		[ --delta1 <clock distance between V1740D and first V1742 in nanosec> ] 
		[ --coincidence <coincidence window to accept an event in nanosec - default 1000> ] 

this will produce a file "events.dat" (or any other name passed after the flag -o) based on the three input files passed. Delta N are the offesets in time between the digitizers: this can be used to improve time matching. Coincidence is the time window to accept coincidences.



----------------------------
|    CONVERSION TO ROOT    |
----------------------------


The output of events can be converted to a ROOT file (format compatible with ModuleCalibration) with convertToRoot. If only the 740 part is relevant, the sorting can be skipped completely and the binary740.dat file can directly be converted to ROOT with convertToRoot740.

CTR perfomance of the board can be measured with an external test pulser. After processing data with events, the conversionToRoot produces also a root file "offsets.root" with histograms for the CTR analysis.


----------------------------
|   OFFLINE INTERPOLATION  |
----------------------------


If the acquistion saved the wave files (see how to do it in the acquisition part) they can be analyzed by the interpolation program. It's meant just to test, offline, the online interpolation algorithm. For this reason the input is harcoded (it's the 64 wave files), and the only input accepted is the choice of reference for the timing part: 

0 - the reference wave is the TRn wave in each group
1 - the reference is channel 0







