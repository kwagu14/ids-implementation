'''


'''


import socket
import os
from _thread import *
import signal
import sys
import binascii
import time
import struct
#this will "import" the ML python file into the code
# exec(open("./cnn-model.py", "rb").read())


#This is the server IP address; we'll use our public address
SERVER_HOST = "172.16.14.82"
#The port that clients have to enter 
SERVER_PORT = 1337
#block size
BUFFER_SIZE = 1024
#separator used for ASCII messages
SEPARATOR = "<SEPARATOR>"
#signals
START_TRANSMISSION = "<START_TRANSMISSION>"
END_TRANSMISSION = "<END_TRANSMISSION>"
START_CLASSIFICATION = "<START_CLASSIFICATION>"
END_CLASSIFICATION = "<END_CLASSIFICATION>"
START_SIZE = "<START_SIZE>"
END_SIZE = "<END_SIZE>"
START_BLOCK_NUMS = "<START_BLOCK_NUMS>"
END_BLOCK_NUMS = "<END_BLOCK_NUMS>"
START_BLOCK_LIST_SIZE = "<START_BLOCK_LIST_SIZE>"
END_BLOCK_LIST_SIZE = "<END_BLOCK_LIST_SIZE>"
#signal sizes
startClassificationLen = 22
endClassificationLen = 20
endTransmissionLen = 18
startTransmissionLen = 20
startSizeLen = 12
endSizeLen = 10 
startBlockNumsLen = 18
endBlockNumsLen = 16
startBlockListSizeLen = 23
endBlockListSizeLen = 21

#expecting 256KB of data (all of flash bank 3)
EXPECTED_MEM_SIZE = 262144 


def recvMem(client_socket):
    #Before calling this function, the server should be populated with an initial dump which can be obtained through receiveMemDump()
    path = "sample_dump.bin"
    numBytesReceived = 0
    #open for reading and writing in binary mode; create if doesn't exist; file offset @ end of file if exists
    f = open(path, "rb+")
    f.seek(0)

    #receive all data coming in over the network 
    bytes_received = receiveAllBytes(client_socket)

    #search for the number of blocks in the data & extract
    dataInd = bytes_received.find(START_SIZE.encode())
    dataInd += startSizeLen
    #convert encoded bytes to short int
    numBlocks = struct.unpack_from(">H", bytes_received, dataInd)
    #prev method returns a tuple; get first item
    numBlocks = numBlocks[0]
    print("The number of blocks received: ", str(numBlocks))


    if(numBlocks > 0 and numBlocks < 256):
        print("Receiving modified blocks (not a full dump)...")
        #search for the size of the block list
        dataInd = bytes_received.find(START_BLOCK_LIST_SIZE.encode())
        dataInd += startBlockListSizeLen
        block_list_size_bytes = bytes_received[dataInd:dataInd+2]
        blockListSize = struct.unpack(">H", block_list_size_bytes)
        blockListSize = blockListSize[0]

        #search for the block list in the data & extract
        dataInd = bytes_received.find(START_BLOCK_NUMS.encode())
        dataInd += startBlockNumsLen
        blockListTuple = struct.unpack_from("{length}c".format(length=blockListSize), bytes_received, dataInd)
        blockList = ""
        for x in blockListTuple:
            blockList += x.decode()
        
        blockNumStrArr = blockList.split(",")
        blockNumArr = [int(numeric_string) for numeric_string in blockNumStrArr]
        print("The modified block list is: ", blockNumArr)

        #search for start transmission
        dataInd = bytes_received.find(START_TRANSMISSION.encode())
        memInd = dataInd + startTransmissionLen

        for i in range(numBlocks):
            #get the correct memory block
            currMemBlock = bytes_received[memInd:memInd+1024]
            # print(currMemBlock)
            fileOffset = blockNumArr[i]*1024
            #append block to that positon in the file
            f.seek(fileOffset)
            f.write(currMemBlock)
            memInd += 1024
            print("Wrote memory block ", str(blockNumArr[i]))
    elif(numBlocks == 256): 
        print("Receiving full memory dump...")
        #search for start transmission
        f.seek(0)
        dataInd = bytes_received.find(START_TRANSMISSION.encode())
        memInd = dataInd + startTransmissionLen

        for i in range(numBlocks):
            #get the correct memory block
            currMemBlock = bytes_received[memInd:memInd+1024]
            # print(currMemBlock)
            #append block to that positon in the file
            f.write(currMemBlock)
            memInd += 1024

    #exit
    f.close()
    print("Memory written.")

    #at this point, we have the full memory dump; get classification here: 
    classification = 1
    #now send to client
    response = START_CLASSIFICATION + str(classification) + END_CLASSIFICATION
    print("Sending classification to server.")
    client_socket.send(response.encode())
    print("Finished.")
    client_socket.close()
    return



#receive all bytes that a client sends 
def receiveAllBytes(client_socket):
    output = b''
    print("Receiving data...")
    #search for the number of data blocks
    bytes_received = client_socket.recv(BUFFER_SIZE)
    output += bytes_received
    while True:
        bytes_received = client_socket.recv(BUFFER_SIZE)
        output += bytes_received
        #once client disconnects, this condition will become true
        if not bytes_received: 
            print("Client closed connection.")
            return output
        if(output.find(END_TRANSMISSION.encode()) >= 0):
            print("Successfully received all memory.")
            return output



#a function to receive exactly n bytes of data from the client
# def recvall(sock, n):
#     data = bytearray()
#     while len(data) < n:
#         packet = sock.recv(n - len(data))
#         if not packet:
#             return None
#         data.extend(packet)
#     return data



# #a function to pad the signals to the correct length
# def padString(signal):
#     padding = BUFFER_SIZE - len(signal)
#     for x in range(padding):
#         signal += "-"
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

#create a separate thread for each connection
while(True):
    
    print("[FROM FILESERVER]: Waiting for the client to specify the request... ")
    #accept clients as they come
    client_socket, address = s.accept()
    print(f"[FROM FILESERVER]: {address} is connected.")

    #start receiving memdumps for each client that connects
    start_new_thread(recvMem, (client_socket, ))