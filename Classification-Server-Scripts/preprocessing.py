#!/usr/bin/env python
# coding: utf-8

# In[1]:
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import librosa
from librosa import display
from tqdm import tqdm


# # Preprocessing data

# In[2]:


#preprocessing entire sample folder; used for training; CSV is needed for this; supply path to sample folder
def preprocessAll(csvFilePath, folderPath):
    x_train=[]
    x_test=[]
    y_train=[]
    y_test=[]
    
    #forming a panda dataframe from the metadata file
    data=pd.read_csv(csvFilePath)
    print("data val: ")
    print(data)
    old_label = data["Type"]
    print (old_label)
    label = data.iloc[0:]
    print (label)
    new_label = data['Class_ID']
    print (list(set(new_label)))
#     data["Folder_Name"].value_counts()
    
    path=folderPath
    for i in tqdm(range(len(data))):
        file=data.iloc[i]["File_Name"]
        print("file name: ", file)
        label = data.iloc[i]["Class_ID"]
        print("label: ", label)
        filename=folderPath + "/" + file
        print("filepath: ", filename)
        dataType = data.iloc[i]["Type"]
        print("type: ", type)
        print("starting librosa.load...")
        y,sr=librosa.load(filename)
        #these are what actually get used in preprocessing
        mfccs = np.mean(librosa.feature.mfcc(y=y, sr=sr, n_mfcc=40).T,axis=0)
        melspectrogram = np.mean(librosa.feature.melspectrogram(y=y, sr=sr, n_mels=40,fmax=8000).T,axis=0)
        chroma_stft=np.mean(librosa.feature.chroma_stft(y=y, sr=sr,n_chroma=40).T,axis=0)
        chroma_cq = np.mean(librosa.feature.chroma_cqt(y=y, sr=sr, bins_per_octave=40, n_chroma=40).T,axis=0)
        chroma_cens = np.mean(librosa.feature.chroma_cens(y=y, sr=sr, bins_per_octave=40, n_chroma=40).T,axis=0)
        features=np.reshape(np.vstack((mfccs,melspectrogram,chroma_stft,chroma_cq,chroma_cens)),(40,5))
        
        print("feature extraction complete")
        #append to appropriate array
        if (dataType == "train"):
            x_train.append(features)
            y_train.append(label)
        else:
            x_test.append(features)
            y_test.append(label)
                
    #convert the arrays to numpy arrays
    x_train=np.array(x_train)
    x_test=np.array(x_test)
    y_train=np.array(y_train)
    y_test=np.array(y_test)
    x_train.shape,x_test.shape,y_train.shape,y_test.shape
    
    #reshape to 2d arrays
    x_train_2d=np.reshape(x_train,(x_train.shape[0],x_train.shape[1]*x_train.shape[2]))
    x_test_2d=np.reshape(x_test,(x_test.shape[0],x_test.shape[1]*x_test.shape[2]))
    x_train_2d.shape,x_test_2d.shape
    
    #saving to csv files
    np.savetxt("train_data.csv", x_train_2d, delimiter=",")
    np.savetxt("test_data.csv", x_test_2d, delimiter=",")
    np.savetxt("train_labels.csv", y_train, delimiter=",")
    np.savetxt("test_labels.csv", y_test, delimiter=",")


# In[3]:


#preprocessing of a single sample; no csv needed; supply the path to the wav file
def preprocessSingle(filePath):
    
    #this is where we're going to store the final data
    single_sample=[]
    
    #preprocessing starts
    y,sr=librosa.load(filePath)
    mfccs = np.mean(librosa.feature.mfcc(y=y, sr=sr, n_mfcc=40).T,axis=0)
    melspectrogram = np.mean(librosa.feature.melspectrogram(y=y, sr=sr, n_mels=40,fmax=8000).T,axis=0)
    chroma_stft=np.mean(librosa.feature.chroma_stft(y=y, sr=sr,n_chroma=40).T,axis=0)
    chroma_cq = np.mean(librosa.feature.chroma_cqt(y=y, sr=sr, bins_per_octave=40, n_chroma=40).T,axis=0)
    chroma_cens = np.mean(librosa.feature.chroma_cens(y=y, sr=sr, bins_per_octave=40, n_chroma=40).T,axis=0)
    features=np.reshape(np.vstack((mfccs,melspectrogram,chroma_stft,chroma_cq,chroma_cens)),(40,5))
    single_sample.append(features)
    
    #convert to numpy array
    single_sample=np.array(single_sample)
    single_sample.shape
    
    #reshape to 2D array
    single_sample_2d=np.reshape(single_sample,(single_sample.shape[0],single_sample.shape[1]*single_sample.shape[2]))
    
    #save to csv file
    np.savetxt("single_sample_data.csv", single_sample_2d, delimiter=",")


# # Testing/visualization

# In[4]:



#         print("============= Feature 1: mfcc's ===============")
#         mfccs = librosa.feature.mfcc(y=y, sr=sr, n_mfcc=40)
#         plt.figure(figsize=(25,10))
#         librosa.display.specshow(mfccs, x_axis='time', sr=sr)
#         plt.title('MFCC')
#         plt.colorbar(format="%+2f")
#         plt.show()
        
#         print("============= Feature 2: melspectrogram ===============")
#         melspectrogram = librosa.feature.melspectrogram(y=y, sr=sr, n_mels=40,fmax=8000)
#         plt.figure(figsize=(25,10))
#         librosa.display.specshow(melspectrogram, y_axis='mel', fmax=8000, x_axis='time')
#         plt.title('Melspectrogram')
#         plt.colorbar(format='%+2.0f dB');
#         plt.show()
        
        
#         print("============= Feature 3: chromagram (stft) ===============")
#         chroma_stft=librosa.feature.chroma_stft(y=y, sr=sr,n_chroma=40)
#         plt.figure(figsize=(25,10))
#         librosa.display.specshow(chroma_stft, x_axis='time', y_axis='chroma', cmap='coolwarm')
#         plt.title('Chromagram STFT')
#         plt.show()
        
#         print("============= Feature 4: chromagram (cqt) ===============")
#         chroma_cq = librosa.feature.chroma_cqt(y=y, sr=sr, bins_per_octave=40, n_chroma=40)
#         plt.figure(figsize=(25,10))
#         librosa.display.specshow(chroma_cq, x_axis='time', y_axis='chroma', cmap='coolwarm')
#         plt.title('Chromagram CQT')
#         plt.show()
        
        
#         print("============= Feature 5: chroma energy normalized ===============")
#         chroma_cens = librosa.feature.chroma_cens(y=y, sr=sr, bins_per_octave=40, n_chroma=40)
#         plt.figure(figsize=(25,10))
#         librosa.display.specshow(chroma_cens, x_axis='time', y_axis='chroma', cmap='coolwarm')
#         plt.title('CENS')
#         plt.show()

