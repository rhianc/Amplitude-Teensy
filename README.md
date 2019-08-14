# PROPRIETARY AMPLITUDE LIGHTING SOFTWARE/HARDWARE REPOSITORY

##  HOW TO CLONE THIS REPO

in unix terminal:

$ git clone --recurse-submodules git@github.com:vfink/amplitude_lights.git

If you do not include the "--recurse-submodules" you will not have the correct library to run the lights

You may need to associate your git account with your computer and should add an SSH key to both your laptop and github account so you don't have to type your password every time. 

## WHERE IS EVERYTHING

24 strip reactive lights:

	master teensy -> amplitude/multi_strip_lights/AmplitudeLights/fftBigBoyMaster
	slave teensy (what you should touch) -> amplitude/multi_strip_lights/AmplitudeLights/fftBigBoyReceiver

8 strip reactive lights:

	amplitude/multi_strip_lights/AmplitudeLights/fftSmallBoy

## TO DO

list ideas and current work here

## NOTES

Cool explanations of why we have to use FreqBinGen and why we SHOULD add some sort of EQ to the processing.
https://www.audiocheck.net/soundtests_nonlinear.php
