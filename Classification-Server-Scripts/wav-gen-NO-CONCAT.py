import os
import sys
import array
from pprint import pprint
import scipy.io.wavfile
import numpy as np
import soundfile as sf
import matplotlib.pyplot as plt
from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
import wave, sys, struct
import librosa
import librosa.display
from dtw import *
from numpy.linalg import norm
import timeit
import json
#sys.stdout = open("C:\\Users\\vjman\\Downloads\\python_sine_wave_script\\Logs.log", "w")

from numpy import array, zeros, full, argmin, inf
#from scipy.spatial.distance import cdist
from math import isinf


def IntraApp_WavCreation(sourceDirectory, destDirectory, appName):

    wav_list = []
    count = 0
    #this is creating the empty .wav files
    for file in os.listdir(sourceDirectory):
        #wav files will be appName-t0.wav, appName-t1.wav, etc..
        final_wav = destDirectory + "/" + appName + "-t" + str(count) + ".wav"
        count+=1
        #for every single file, create wav file
        f = open(sourceDirectory + "/" + file, "rb")
        my_array = bytearray(f.read())
        new_array = np.array(my_array, dtype=np.int16)
        sf.write(final_wav, new_array, 48000)
        print("Wave file " + str(count) + " is created")
        wav_list.append(final_wav)
        print("============================================================")
        
    return(sorted(wav_list))


# USAGE NOTES

# This script is for a host-based intrusion detection system for IoT devices
# It converts memory dumps to sound files so that they can be used to train the anomaly detection algorithm. 

# This version (NO-CONCAT) should be used for memory dumps not obtained from the memfetch utility
# Each of the dumps need to have all of the device memory for a given time in a single bin file
# Put those bin files in a folder and run the script like this

# python3 wav-gen-NO-CONCAT.py [sourceDirectory] [destDirectory] [appName]

#	-sourceDirectory: the folder you put the bin files in
#	-destDirectory: the folder that you want wav files to go into
#	-appName: the name of your application; this is used in naming the wav files so it can be whatever makes sense to you

if __name__ == "__main__":
    sourceDirName = sys.argv[1]
    destDirName = sys.argv[2]
    appName = sys.argv[3]
    print("Application Name:" + appName)
    wav_list = IntraApp_WavCreation(sourceDirName, destDirName, appName)

	

	
