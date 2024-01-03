'''
This code accepts data from an IoT application running on an STM32. 

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
SERVER_HOST = "172.16.14.97"
#The port that clients have to enter 
SERVER_PORT = 1337
#block size
BUFFER_SIZE = 1024


#parses TCP stream sent by the MCU and saves the memory in a binary file
def receiveData(client_socket):
    path = "IR-app/database.txt"
    #create a binary file (if doesn't exist); overwrite if it does exist

    msg = client_socket.recv(1024)
    while msg:
        print('Received:' + msg.decode())
        f = open(path, "a")
        f.write(msg.decode())
        f.close()
        msg = client_socket.recv(1024)
    
    # #loop that searches for the start of a memory transmission
    # while True:
    #     #Receive more bytes
    #     bytes_received = recvall(client_socket, 200)
    #     #if client hasn't sent us anything, then stop reception
    #     # if not bytes_received:
    #     #     print("No more data from the client...")
    #     #     break
    #     try:
    #         #else, look for valid data
    #         result = bytes_received.decode()
    #         if(result == "ENTITY DEPARTED" or "ENTITY APPROACHING"):
    #             print("the data: ", result)
    #             try: 
    #                 f.write(result)
    #             except Exception as e:
    #                 print("Error writing the data to the database:")
    #                 print(e)
    #         else:
    #             #if succeeded, we got the start of the dump
    #             print("Invalid data received")
    #     except: 
    #         print("Data is not ASCII")
    #         pass
    
    #close the database file
    # f.close()
    # #then close connection
    # client_socket.close()
    # print("connection closed.")
    # return



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

#create a separate thread for each connection
while(True):
    print("[FROM FILESERVER]: Waiting for the client to specify the request... ")
    #accept clients as they come
    client_socket, address = s.accept()
    print(f"[FROM FILESERVER]: {address} is connected.")
    #start receiving memdumps for each client that connects
    start_new_thread(receiveData, (client_socket, ))
    
    
