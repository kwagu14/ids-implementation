import socket
import os
from _thread import *
import signal
import sys
import struct
#this will "import" the python file into the code
# exec(open("./cnn-model.py", "rb").read())


#This is the server IP address; we'll use our public address
SERVER_HOST = "192.168.0.94"
#The port that clients have to enter 
SERVER_PORT = 1337
BUFFER_SIZE = 32
SEPARATOR = "<SEPARATOR>"

START_TRANSMISSION = "<START_TRANSMISSION>------------"
END_TRANSMISSION = "<END_TRANSMISSION>--------------"
START_DATA_BLOCK = "<START_DATA_BLOCK>--------------"
END_DATA_BLOCK = "<END_DATA_BLOCK>----------------"


def receiveMemDump(client_socket):
    path = "sample_dump.bin"
    #create a binary file (if not exists)
    # with open(path, "wb") as f:
    
    #loop that searches for the start of a memory transmission
    print("Looking for: <START_TRANSMISSION>...\n")
    while True:
        bytes_received = client_socket.recv(BUFFER_SIZE)
        #stop receiving if the client is not sending
        if not bytes_received:
            break
        try:
            #look for <START_TRANSMISSION> signal; try to decode to ASCII
            result = bytes_received.decode()
            if(result != START_TRANSMISSION):
                continue
            else:
                #we got the start of the dump
                print("Received <START_TRANSMISSION> signal from client")
                while True:
                    #Receive memory blocks
                    try: 
                        #this needs to be a signal; signals are padded to 32 bytes long
                        bytes_received = client_socket.recv(BUFFER_SIZE)
                        result = bytes_received.decode()
                        #first make sure we haven't reached the end of the transmission
                        #TODO add check to force correct # of bytes to be received (protect against injection)
                        if(result == END_TRANSMISSION):
                            print("Received <END_TRANSMISSION> from client.")
                            break
                        #do we have a start data block?
                        if(result == START_DATA_BLOCK):
                            print("Received <START_DATA_BLOCK> from client")
                            #now receive data block
                            memBlock = recvall(client_socket, BUFFER_SIZE)
                            #print it
                            for x in range(len(memBlock)):
                                print(memBlock[x], " ", end= "")
                            print("\n")
                        else:
                            print("Error: TCP stream improperly formatted. Received wrong signal.")
                            client_socket.close()
                            return
                        #then check for the <END_DATA_BLOCK> signal after the mem block is received
                        bytes_received = client_socket.recv(BUFFER_SIZE)
                        result = bytes_received.decode()
                        if(result == END_DATA_BLOCK):
                            print("Received <END_DATA_BLOCK> from client")
                        else:
                            print("Error: TCP stream improperly formatted. Received wrong signal.")
                            client_socket.close()
                            return
                    except:
                        print("Error: TCP stream improperly formatted. Was not able to decode signal.")
                        client_socket.close()
                        return
        except: 
            pass

    print("Transmission complete. Closing connection...")
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
    
    
# def sendSimularity(client_socket, identity, filename):
#     path = identity + "/" + filename
#     print("[FROM FILESERVER]: Passing file to classifier...")
#     simularity = classify(path)
#     print("[FROM FILESERVER]: Simularity found to be ", simularity)
#     #send to the client
#     client_socket.send(simularity.encode())



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

    # #first, we need to get the client's request
    # received = client_socket.recv(BUFFER_SIZE).decode()
    # print(received)
    # identity, op, filePath = received.split(SEPARATOR)
    # print("REQUEST: identity: ", identity, "operation: ", op, "file path: ", filePath)    
    # #Once gotten, use that socket to handle the request

    #start receiving memdumps for each client that connects
    start_new_thread(receiveMemDump, (client_socket, ))
    
    
