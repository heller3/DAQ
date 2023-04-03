# Multichannel DAQ

Acquisition software for combined V1740D and V1742. Written by Marco Pizzichemi - marco.pizzichemi@cern.ch

## 1. Compile program

Simply run the compile script from main folder

```
./deploy.sh
```

CAEN libraries need to be installed (at least to compile the main DAQ program). The `deploy.sh` script will create a `bin/` subfolder. For ease of use, put this folder in your bash `$PATH`.

## 2. Run full acquisition

The digitizers are configured using a configuration text file, e.g.

```
config_two_febs.txt
```

The keys are (kind of) explained in the config file itself. 

Although the acquisition can be run step-by-step (see later), the entire chain can be controlled by a single Python script (if the `scripts` subfolder is not in your bash `$PATH`, use full path for the python script)

```
fullAcq.py -c fullAcq.cfg
```

This script will take care of turning on the SiPM power supplies, setting the position of the rotating motors, setting the position of the linear stages, and start 1 or more acquisitions. If the user does not want one of these tasks, they can be switched off in the `[GENERAL]` section.
For each desired acquisition, a `[ACQ_N]` section can be added (N integer from 0, and it is forbidden to skip a number). Each acquisition can have a different digitizer configuration file `config`, bias voltages `V_FEB_1` and `V_FEB_2`, rotation angles `angle1` and `angle2`, relative movement of the linear stages `x_distance` and `z_distance`, and absolute positions of the linear stages `x_position` and `z_position`. The script will run the acquisition, then create the merged events combining the output of the 3 digitizers, and finally convert the binary output to ROOT files.

## 3. A bit more step by step

### 3.1 Run only acquisition and conversion to ROOT

To run only the acquisition (hence no SiPM bias etc), use this pythong script

```
runAcquisition.py -c config_two_febs.txt -k 0 -s 1 -t 120 -f folder
```

The options are found at the end of the python script, and they are:

```
-c     digitizer config file
-t     acquisition time in seconds
-f     name of subfolder for output
-k     keep raw data after conversion to ROOT
-s     single slice (stops when time is finished)
```
## 4. Completely step by step

### 4.1 Run only acquisition

If you want to run only the acquisition, just use 

```
./bin/readout config_two_febs.txt 100
```

where in this case 100 is the acquisition duration, in seconds. Leave empty for infinite acquisition. Acquisition starts pressing "s". It can be stopped pressing "q". Pressin "p" starts the plotting. Once the plotting is started, pressing "c" followed by a number and Enter allows to change the channel visualized (same as CAEN Wavedump). Output files are in binary format (one for each digitizer), and need to be sorted (and converted to ROOT, if desired).

### 4.2 Sort events

Events are sorted by program `events`. If run without arguments, events provide the list of its possible arguments:

Usage: events
		[ -o <output file name> ] 
		[ --input0 <input file from V1740D digitizer> ] 
		[ --input1 <input file from V1740D digitizer> ] 
		[ --input2 <input file from V1740D digitizer> ] 
		[ --delta0 <clock distance between V1740D and first V1742 in nanosec> ] 
		[ --delta1 <clock distance between V1740D and first V1742 in nanosec> ] 
		[ --coincidence <coincidence window to accept an event in nanosec - default 1000> ] 

The program will produce a file `events.dat` (or any other name passed after the flag -o) based on the three input files passed, which are of course the output of the previous step (the acquisition). `--deltaN` are the offesets in time between the digitizers: this can be used to improve time matching. Coincidence is the time window to accept coincidences.

### 4.3 Conversion to ROOT

The `events.dat` can be converted to ROOT using 

```
convertToRoot --input events.dat
```

If only the 740 part is relevant, the sorting can be skipped completely and the binary740.dat file (output of the 740 digitizer) can directly be converted to ROOT with convertToRoot740

```
convertToRoot binary740.dat
```


# OLD readme


----------------------------
|         ACQUSITION       |
----------------------------


Configuration file -> config.txt 
Key meanings are explained in the file

Create a bin subfolder and compile from there with

gcc -DLINUX ../src/readout.c ../src/dpp_qdc.c ../src/_CAENDigitizer_DPP-QDC.c  ../src/X742CorrectionRoutines.c -o readout -lCAENDigitizer -lCAENComm -fpermissive -w -lm

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







