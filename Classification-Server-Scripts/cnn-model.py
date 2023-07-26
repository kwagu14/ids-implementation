#!/usr/bin/env python
# coding: utf-8

#Usage:
# python3 cnn-model.py train [csv-file] [audioFilesFolder]
# or
# python3 cnn-model.py classify [wavFilePath]

# # Model 1 - CNN

import numpy as np
import pickle
from numpy import genfromtxt
from keras.utils.np_utils import to_categorical
import import_ipynb
from keras import Sequential
from keras.layers import Dense,Conv2D,MaxPooling2D,Flatten,Dropout
from keras import metrics
import sys
from keras import backend as K
exec(open("./preprocessing.py", "rb").read())


# ### Building the Model

def buildModel():
    global model
    #forming model
    model=Sequential()
    #adding layers and forming the new model
    model.add(Conv2D(32,kernel_size=3,strides=1,padding="same",activation="relu",input_shape=(40,5,1)))
    model.add(MaxPooling2D(padding="same"))
    model.add(Conv2D(64,kernel_size=3,strides=1,padding="same",activation="relu",input_shape=(40,5,1)))
    model.add(MaxPooling2D(padding="same"))
    model.add(Conv2D(128,kernel_size=3,strides=1,padding="same",activation="relu"))
    model.add(MaxPooling2D(padding="same"))
    model.add(Dropout(0.3))
    model.add(Flatten())
    model.add(Dense(256,activation="relu"))
    model.add(Dropout(0.3))
    model.add(Dense(512,activation="relu"))
    model.add(Dropout(0.3))
    model.add(Dense(2,activation="sigmoid"))
    return model
  
    

# ### Train and Save the Model

def trainModel(model):
    # compile the model
    model.compile(optimizer="adam",loss="binary_crossentropy",metrics=["acc",f1_m,precision_m, recall_m])
    print(model.summary())
    # fit the model
    history = model.fit(x_train, y_train, batch_size=6, epochs=250, verbose=1, validation_data=(x_test,y_test))
    #save the model
    filename = 'finalized_cnn_model.sav'
    pickle.dump(model, open(filename, 'wb'))
    return model



# ### Evaluate the Model

def recall_m(y_true, y_pred):
    true_positives = K.sum(K.round(K.clip(y_true * y_pred, 0, 1)))
    possible_positives = K.sum(K.round(K.clip(y_true, 0, 1)))
    recall = true_positives / (possible_positives + K.epsilon())
    return recall

def precision_m(y_true, y_pred):
    true_positives = K.sum(K.round(K.clip(y_true * y_pred, 0, 1)))
    predicted_positives = K.sum(K.round(K.clip(y_pred, 0, 1)))
    precision = true_positives / (predicted_positives + K.epsilon())
    return precision

def f1_m(y_true, y_pred):
    precision = precision_m(y_true, y_pred)
    recall = recall_m(y_true, y_pred)
    return 2*((precision*recall)/(precision+recall+K.epsilon())) 
    
def evaluateModel(model):
    loss, accuracy, f1_score, precision, recall = model.evaluate(x_test, y_test, verbose=0)
    print("loss : " + str(loss))
    print("Accuracy : " + str(accuracy))
    print("F1_Score : " + str(f1_score))
    print("Precision : " + str(precision))
    print("Recall : " + str(recall))


#takes a csv path and a folder containing audio files, saves a trained model to disk
def trainModel(csvPath, folderName):
    #preprocess the data
    preprocessAll(csvPath, folderName)
    #now get the data from the csv's
    x_train = genfromtxt('train_data.csv', delimiter=',') 
    y_train = genfromtxt('train_labels.csv', delimiter=',')
    x_test = genfromtxt('test_data.csv', delimiter=',')
    #generate numpy array of number of samples by MFCCs e.g. 30 x 200
    y_test = genfromtxt('test_labels.csv', delimiter=',')
    #shape
    x_train.shape,x_test.shape,y_train.shape,y_test.shape
    #converting to one hot
    y_train = to_categorical(y_train, num_classes=None)
    y_test = to_categorical(y_test, num_classes=None)
    y_train.shape,y_test.shape
    #reshaping to 2D 
    x_train=np.reshape(x_train,(x_train.shape[0], 40,5))
    x_test=np.reshape(x_test,(x_test.shape[0], 40,5))
    x_train.shape,x_test.shape
    #reshaping to shape required by CNN
    x_train=np.reshape(x_train,(x_train.shape[0], 40,5,1))
    x_test=np.reshape(x_test,(x_test.shape[0], 40,5,1))
    x_train.shape,x_test.shape

    #build, train, and save model
    print("Building model")
    model = buildModel()
    print("Training model")
    model = trainModel(model)
    #evaluate
    print("evaluating")
    evaluateModel(model)
    
    
    
#takes the path of a wav file and returns the simularity score    
def classify(wavPath):
    preprocessSingle(wavPath)
    #get data from csv's
    single_sample = genfromtxt('single_sample_data.csv', delimiter=',')
    #shape
    single_sample.shape[:1]
    #reshape to 2D
    single_sample=np.reshape(single_sample,(1, 40,5))
    #shape required by CNN
    single_sample=np.reshape(single_sample,(1, 40,5,1))
    single_sample.shape

    #load the previously trained cnn model
    model = pickle.load(open("finalized_cnn_model.sav", 'rb'))

    #predict the class
    pred = model.predict(single_sample) 
    pred=np.where(pred[0:] > 0.9, 1,0)
    simularity=pred[:,1]
    simularity=' '.join(map(str, simularity))
    return simularity
