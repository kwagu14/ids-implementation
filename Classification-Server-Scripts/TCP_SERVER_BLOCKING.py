'''
This code accepts a full non-secure flash bank memory dump from an STM32 MCU. 

Since the STM32 does not have built-in Wi-Fi/networking capabilities, the dumps get sent through
an ESP32 MCU attached to the STM32 via SPI. 

This code expects the TCP data to arrive in a very specific format which is 

[<SIGNAL>] (padded to 1024 bytes)
[DATA BLOCK 1]
[DATA BLOCK 2]
...
[DATA BLOCK N]
[<END_SIGNAL] (padded to 1024 bytes)

The data blocks can be up to 1024 bytes in size; this is controlled by the BUFFER_SIZE variable

'''


import socket
import os
from _thread import *
import signal
import sys
import time
import struct
#this will "import" the ML python file into the code
# exec(open("./cnn-model.py", "rb").read())


#This is the server IP address; we'll use our public address
SERVER_HOST = "192.168.0.94"
#The port that clients have to enter 
SERVER_PORT = 1337
#block size
BUFFER_SIZE = 1024
#separator used for ASCII messages
SEPARATOR = "<SEPARATOR>"
#signals; will get padded to block size
START_TRANSMISSION = "<START_TRANSMISSION>"
END_TRANSMISSION = "<END_TRANSMISSION>"
START_CLASSIFICATION = "<START_CLASSIFICATION>"
END_CLASSIFICATION = "<END_CLASSIFICATION>"
#expecting 256KB of data (all of flash bank 2)
EXPECTED_MEM_SIZE = 262144 


#TODO: need to get rid of block signals 
#since I'm still sending the signals padded to 1024 bytes, that doesn't have to change for now



#parses TCP stream sent by the MCU and saves the memory in a binary file
def receiveMemDump(client_socket):
    path = "sample_dump.bin"
    blockNum = 0
    numBytesReceived = 0
    #create a binary file (if doesn't exist); overwrite if it does exist
    f = open(path, "wb")
    
    #loop that searches for the start of a memory transmission
    print("Looking for: <START_TRANSMISSION>...\n")
    while True:
        #first see if we've received all the memory, then break if so
        if (numBytesReceived >= EXPECTED_MEM_SIZE):
            break
        #else receive more bytes
        bytes_received = recvall(client_socket, BUFFER_SIZE)
        #if client hasn't sent us anything, then stop reception
        if not bytes_received:
            print("No more data from the client...")
            break
        try:
            #else, look for <START_TRANSMISSION> signal; try to decode to ASCII
            result = bytes_received.decode()
            if(result != START_TRANSMISSION):
                continue
            else:
                #if succeeded, we got the start of the dump
                print("Received <START_TRANSMISSION> signal from client")
                while True:
                    #Now receive memory blocks as they come in
                    bytes_received = recvall(client_socket, BUFFER_SIZE)
                    try: 
                        #first check if we got an end transmission signal
                        result = bytes_received.decode()
                        if(result == END_TRANSMISSION):
                            #stop receiving blocks
                            print("Received <END_TRANSMISSION> from client.")
                            break
                        else:
                            #we just have normal data that should be written
                            try: 
                                f.write(bytes_received)
                            except Exception as e:
                                print("Error with file write operation:")
                                print(e)
                            numBytesReceived += BUFFER_SIZE
                            #and print it out for validation
                            print("Received Data: ")
                            print("=====================================================================")
                            for x in range(len(bytes_received)):
                                print(hex(bytes_received[x]), "\t", end= "")
                                if((x+1)%16 == 0):
                                    print("\n")
                            # blockNum +=1
                    except:
                        #if the data can't be decoded into ascii, it's probably memory
                        #try writing the data to a file
                        try: 
                            f.write(bytes_received)
                        except Exception as e:
                            print(e)
                        numBytesReceived += BUFFER_SIZE
                        #and print it out for validation
                        print("Received Data: ")
                        print("=====================================================================")
                        for x in range(len(bytes_received)):
                            print(hex(bytes_received[x]), "\t", end= "")
                            if((x+1)%16 == 0):
                                print("\n")
                        # blockNum +=1
        except: 
            pass
    
    #close the binary file
    f.close()
    #now check if correct num of bytes were received
    if(numBytesReceived > EXPECTED_MEM_SIZE):
        print("Error: received too many bytes from MCU.")
        #TODO: send signal to MCU & reset device
    elif(numBytesReceived < EXPECTED_MEM_SIZE):
        print("Error: received too little bytes from MCU.")
        #TODO: send signal to MCU & reset device
    else:
        print("All memory written to file. Closing connection...")
    
    print("Bytes received: ", str(numBytesReceived))

    #at this point, we have the full memory dump; get classification
    classification = 1
    #now send to client
    response = START_CLASSIFICATION + str(classification) + END_CLASSIFICATION
    print("Sending classification to server.")
    client_socket.send(response.encode())
    print("Finished.")

    #then close connection
    client_socket.close()
    return



#a function to receive exactly n bytes of data from the client
def recvall(sock, n):
    data = bytearray()
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return None
        data.extend(packet)
    return data


#a function to pad the signals to the correct length
def padString(signal):
    padding = BUFFER_SIZE - len(signal)
    for x in range(padding):
        signal += "-"
    return signal


#code for handling each client that connects
# def receiveFile(client_socket, identity, filename):
#     path = identity + "/" + filename
#     #this is for getting the file size sent by client
#     filesize = client_socket.recv(BUFFER_SIZE).decode()
#     filename = os.path.basename(path)
#     filesize = int(filesize)
#     progress = tqdm(range(filesize), f"Receiving {filename}", unit="B", unit_scale=True, unit_divisor=1024)
#     with open(path, "wb") as f:
#         while True:
#             bytes_read = client_socket.recv(BUFFER_SIZE)
#             if not bytes_read:
#                 break
#             f.write(bytes_read)
#             progress.update(len(bytes_read))



# #this function will be used every time a new connection request comes in
# def clientThread(client_socket, identity, op, filePath):
    
#     #here the client is trying to send us a file
#     if op == "fileTransfer":
#         receiveFile(client_socket, identity, filePath)      
#     #here it is requesting the simularity for a specific file
#     elif op == "getSimularity":
#         sendSimularity(client_socket, identity, filePath)
#     #In this case, an invalid operation was supplied
#     else:    
#         print("[FROM FILESERVER]: Bad reqest received from client.")
#         #when done, close connection

#     client_socket.close()

    
#code for exiting gracefully upon ctrl+C signal
def sig_handler(sig, frame):
    print("[FROM FILESERVER]: Received SIGINT. Shutting down server...")
    global s
    s.close()
    sys.exit(0)
    
    
    
#install the signal handler
signal.signal(signal.SIGINT, sig_handler)

#create the server socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#bind address info
s.bind((SERVER_HOST, SERVER_PORT))
#start listening for clients
s.listen(10)
print(f"[FROM FILESERVER]: Listening as {SERVER_HOST}:{SERVER_PORT}")

#pad the signals
START_TRANSMISSION = padString(START_TRANSMISSION)
END_TRANSMISSION = padString(END_TRANSMISSION)

#create a separate thread for each connection
while(True):
    
    print("[FROM FILESERVER]: Waiting for the client to specify the request... ")
    #accept clients as they come
    client_socket, address = s.accept()
    print(f"[FROM FILESERVER]: {address} is connected.")

    # #first, we need to get the client's request
    # received = client_socket.recv(BUFFER_SIZE).decode()
    # print(received)
    # identity, op, filePath = received.split(SEPARATOR)
    # print("REQUEST: identity: ", identity, "operation: ", op, "file path: ", filePath)    
    # #Once gotten, use that socket to handle the request

    #start receiving memdumps for each client that connects
    start_new_thread(receiveMemDump, (client_socket, ))
    
    
