#include "ESP32DMASPISlave.h"
#include <WiFi.h>
#include <WiFiMulti.h>


/**
* Author: Karley Waguespack
*
* This code accepts modified memory blocks from an STM32 TrustZone-Enabled MCU. There are two cases which this code handles
*     (I) Eight or fewer 1024-byte blocks have changed
*     (II) More than eight 1024-byte blocks have changed
*
* The dump is sent in a blocking manner, and works fastest In the first case. 
* In the first case, the memory acquisition is extremely fast because all 8KB can be buffered in this Wi-Fi module
* After the buffering occurs, the memory dumps get sent to the classification server through TCP
* And this can happen in parallel to the execution of the IoT application on STM32 since all memory has already been acquired. 
* It essentially removes the need for the STM32 to be involved any more than it has to, and transfers the load to ESP32.
*
* In the second case, the blocks are not buffered on this chip. Instead they're sent slowly
* from the STM32 (in a blocking manner), but with delays in place to allow time for TCP processing on this chip. 
* In this case, the sending of the memory to the classification server does not occur in parallel to the IoT app
* and the app is hugely disrupted by the delays. (I was not able to get concurrent programming to work on STM32 but it can be done in the future)
*
* With each 8KB increase, the memory acquisition increases about 8 seconds because of an 8 second delay. 
* I found that this was a safe enough delay to not cause any data loss
* 
* Although not idea, the increased processing time is not detrimental to our work for the following reasons: 
*     (1) Since IoT memory remains relatively fixed, it is unlikely that more than 8 blocks will need to be transmitted
*     (2) Although the chip used in the implementation has limitations, better hardware that fully supports the design could be used 
*         in a real implementation
*
* To receive data through SPI from the STM32, I am using the ESP32DMASPISlave library by
* Hideaki Tai. All modified blocks get received and placed into an 8KB buffer, then each block is sent through TCP. 
* Signals are used to delimit various pieces of information to allow for easier parsing.
* 
* NOTE: TO USE 
*   1. change the IP address to the IP of the server 
*   2. change the WiFi credentials to allow the ESP32 to connect to an available access point
*/


//SPI data structures
ESP32DMASPI::Slave slave;
static const uint32_t BLOCK_SIZE = 1024;
static const uint32_t MAX_BLOCKS = 8; //(per transfer)
//for sending things to STM32
uint8_t* spi_slave_classification_buf;
//for receiving memory blocks through SPI
uint8_t* spi_slave_mem_block_buf;
//this is for receiving signals
uint8_t* spi_slave_message_buf;
//for receiving the number of blocks
uint8_t* numBlocksToReceive;
uint8_t* spi_slave_block_list_buf;
uint8_t* spi_slave_rx_buf;
char blockNumListAsStr[100];

//TCP Transmission structures
uint8_t tcp_tx_buf[BLOCK_SIZE];


//flags used for detecting the start and end of SPI transfer
bool transmissionStarted = false;
bool transmissionEnded = false;

//wifi data structures
//Port #-- must match with python server
const uint16_t port = 1337; 
//IP Address of my server
const char * host = "172.16.14.82"; 
// Use WiFiClient class to create TCP connections
WiFiClient client;
WiFiMulti wifiMulti;

//signals to help with parsing the data stream received through SPI & sent through TCP
char* endTransmission = "<END_TRANSMISSION>";
char* startTransmission = "<START_TRANSMISSION>";
char* startClassification = "<START_CLASSIFICATION>";
char* endClassification = "<END_CLASSIFICATION>";
char* startSize = "<START_SIZE>";
char* endSize = "<END_SIZE>";
char* startBlockNums = "<START_BLOCK_NUMS>";
char* endBlockNums = "<END_BLOCK_NUMS>";
char* startBlockListSize = "<START_BLOCK_LIST_SIZE>";
char* endBlockListSize = "<END_BLOCK_LIST_SIZE>";

//lengths of above signals
int startClassificationLen = 22;
int endClassificationLen = 20; 
int endTransmissionLen = 18; 
int startTransmissionLen = 20;
int startSizeLen = 12;
int endSizeLen = 10; 
int startBlockNumsLen = 18;
int endBlockNumsLen = 16;
int startBlockListSizeLen = 23;
int endBlockListSizeLen = 21;

int max_message_size = 100;
int max_block_num_list = 3*MAX_BLOCKS+MAX_BLOCKS;
int maxBytes = 262144;
int bytesSent = 0;
int blockNum = 0;



void setup() {

  //configure serial communication
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  //allocate buffers for DMA transfer:

  //holds the received memory classification
  spi_slave_classification_buf = slave.allocDMABuffer(BLOCK_SIZE);
  spi_slave_rx_buf = slave.allocDMABuffer(BLOCK_SIZE);
  //holds the modified memory blocks
  spi_slave_mem_block_buf = slave.allocDMABuffer(MAX_BLOCKS*BLOCK_SIZE+max_message_size);
  //holds signals sent through SPI
  spi_slave_message_buf = slave.allocDMABuffer(max_message_size);
  //holds the block number list for modified blocks
  spi_slave_block_list_buf = slave.allocDMABuffer(max_block_num_list);
  //holds the number of modified blocks that this chip has to receive (note: this may need to be bigger than one)
  numBlocksToReceive = slave.allocDMABuffer(1);

  //configure SPI
  slave.setDataMode(SPI_MODE0);
  slave.setMaxTransferSize(BLOCK_SIZE*MAX_BLOCKS);
  //default SPI pins on my board is VSPI
  Serial.println("=============== SPI PINOUT ==================");
  Serial.begin(115200);
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);
  Serial.println("=============================================");
  Serial.println();
  //Params: VSPI/HSPI, CLK pin, MISO pin, MOSI pin, SS pin
  slave.begin(VSPI, SCK, MISO, MOSI, SS);

  //connect to the Wi-Fi network
  wifiMulti.addAP("LA-OAPA", "ExtraordinaryRover"); //Put wi-fi password here
  Serial.print("Waiting for WiFi... ");

  while(wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}


void loop() {

  //reset global buffers
  clearBuf(spi_slave_classification_buf, BLOCK_SIZE);
  clearBuf(spi_slave_mem_block_buf, MAX_BLOCKS*BLOCK_SIZE+max_message_size);
  clearBuf(spi_slave_message_buf, max_message_size);
  clearBuf(numBlocksToReceive, 1);
  clearBuf(spi_slave_block_list_buf, max_block_num_list);
  clearBuf(spi_slave_rx_buf, BLOCK_SIZE);

  //holds the raw bytes (big endian) of the # of data blocks and the # of characters in block list, respectively
  uint8_t data_blocks_bytes[2];
  uint8_t block_num_list_size_bytes[2];
  uint16_t blockNumListSize = 0;

  //Step 1: search for the number of blocks
  int dataInd = searchForDataInStream(startSize, startSizeLen, endSize, endSizeLen, 1, spi_slave_message_buf, max_message_size);
  data_blocks_bytes[0] = spi_slave_message_buf[dataInd];
  data_blocks_bytes[1] = spi_slave_message_buf[dataInd+1];
  uint16_t dataBlocks = (spi_slave_message_buf[dataInd] << 8) + (spi_slave_message_buf[dataInd+1]);
  Serial.printf("The value of dataBlocks is: %d\n", dataBlocks);
  //if no data blocks received, start over
  if(dataBlocks == 0){
    Serial.printf("No memory blocks were found to be modified.\n");
    delay(5000);
    return;
  }
  int maxMemSize = 8*1024; //(per transfer)
  slave.pop();

  //Step 2: Gather metadata & send to server
  if(dataBlocks >= 256){
    //if getting all memory, just send all blocks to server; no need to send block list stuff
    Serial.printf("Received a full memory dump from the STM32\n");
    initCommunication();
    //send <START_SIZE>[# blocks modified]<END_SIZE>
    sendTCPData(client, (uint8_t *)"---------------------", 10); //padding because sometimes first few blocks are missed ???
    sendTCPData(client, (uint8_t *)startSize, startSizeLen);
    sendTCPData(client, (uint8_t *)data_blocks_bytes, 2);
    sendTCPData(client, (uint8_t *)endSize, endSizeLen);
  }else{
    //Else if a full dump is not being received, need to send some metadata so server knows how to affix memory
    Serial.printf("Received modified blocks from the STM32\n");
    //search for the size of the block number list
    dataInd = searchForDataInStream(startBlockListSize, startBlockListSizeLen, endBlockListSize, endBlockListSizeLen, 2, spi_slave_message_buf, max_message_size);
    block_num_list_size_bytes[0] = spi_slave_message_buf[dataInd];
    block_num_list_size_bytes[1] = spi_slave_message_buf[dataInd+1];
    blockNumListSize = (spi_slave_message_buf[dataInd] << 8) + (spi_slave_message_buf[dataInd+1]); 
    printf("The size of the block list is: %d\n", blockNumListSize);
    slave.pop();
    //search the list of block numbers
    extractBlockList(blockNumListSize, blockNumListAsStr);
    for(int i = 0; i < blockNumListSize; i++){
      Serial.printf("%c", blockNumListAsStr[i]);
    }
    Serial.println();
    //initialize communication with the server
    initCommunication();
    //send <START_SIZE>[# blocks modified]<END_SIZE>
    sendTCPData(client, (uint8_t *)"---------------------", 10);
    sendTCPData(client, (uint8_t *)startSize, startSizeLen);
    sendTCPData(client, (uint8_t *)data_blocks_bytes, 2);
    sendTCPData(client, (uint8_t *)endSize, endSizeLen);
    //send <START_BLOCK_LIST_SIZE>[# characters in block list]<END_BLOCK_LIST_SIZE>
    sendTCPData(client, (uint8_t *)startBlockListSize, startBlockListSizeLen);
    sendTCPData(client, (uint8_t *)block_num_list_size_bytes, 2);
    sendTCPData(client, (uint8_t *)endBlockListSize, endBlockListSizeLen);
    //send <START_BLOCK_LIST>[block list]<END_BLOCK_LIST>
    sendTCPData(client, (uint8_t *)startBlockNums, startBlockNumsLen);
    sendTCPData(client, (uint8_t *)blockNumListAsStr, blockNumListSize);
    sendTCPData(client, (uint8_t *)endBlockNums, endBlockNumsLen);

  }

  //Step 3: send the memory to the server
  sendTCPData(client, (uint8_t *)startTransmission, startTransmissionLen);
  int calculatedMemSize;
  int usedMemSize;
  //accept memory blocks and send until there are no more
  int iter = 0;
  int offset = 0;
  while(dataBlocks > 0){
    Serial.printf("memory blocks remaining: %d\n\r", dataBlocks);
    calculatedMemSize = dataBlocks*1024;
    Serial.printf("calculated mem size is %d\n", calculatedMemSize);
    usedMemSize = (dataBlocks > 8) ? maxMemSize : dataBlocks*1024;
    Serial.printf("Grabbing transfer %d.\n", iter);

    dataInd = searchForDataInStream(startTransmission, startTransmissionLen, endTransmission, endTransmissionLen, usedMemSize, spi_slave_mem_block_buf, (MAX_BLOCKS*BLOCK_SIZE+max_message_size));
    Serial.printf("<start_transmission> found at %d\n", dataInd);
    //print memory; (note: very slow & can cause data loss if delay isnt increased on STM32) 
    // printMemReceived(&spi_slave_mem_block_buf[dataInd]);
    //send memory
    sendTCPData(client, (uint8_t *)&spi_slave_mem_block_buf[dataInd], usedMemSize);
    dataBlocks -= (dataBlocks >= 8 ? 8 : dataBlocks); //since unisgned: need this to prevent underflow
    Serial.printf("data blocks decremented to: %d\n", dataBlocks);
    slave.pop();
    offset = dataInd+1;
    iter++;
  }
  Serial.printf("All blocks sent.\n");
  sendTCPData(client, (uint8_t *)endTransmission, endTransmissionLen);

  //Step 4: receive the classification from the server: 
  int classification = getServerResponse();

  //Step 5: send classification to STM32:
  //load with something
  sendClassification(classification);

  //Step 6: close connection
  deinitCommunication();


}


void clearBuf(uint8_t *buf, int size){
  for(int i = 0; i < size; i++){
    buf[i] = 0;
  }
}


void printMemReceived(uint8_t* start){
  Serial.println("------------------------------------------------------------------");
  for(int i = 0; i < 8192; i++){
    printf("0x%02x\t", start[i]);
    if((i+1)%16 == 0){
      Serial.println();
    }
  }
  Serial.println("------------------------------------------------------------------");
}


//Connects to server; needs to be called before sending through TCP
void initCommunication(){
    //connect to the server
    Serial.print("Connecting to ");
    Serial.println(host);
    //try to connect to the host until connection succeeds
    while(!client.connect(host, port)) {
      Serial.println("Connection failed.");
      Serial.println("Waiting 3 seconds before retrying...");
      delay(3000);
    }
}


//closes tcp communication and resets variables
void deinitCommunication(){
  //close server and reset variables; prepare for another transmission
  Serial.println("Shutting down connection...");
  client.stop();
  Serial.print("Done.");
  transmissionEnded = false;
  transmissionStarted = false;
  bytesSent = 0;
}


//this function sends n bytes of a data block through TCP
void sendTCPData(WiFiClient client, uint8_t* buf, int n){
  //send the first n bytes in the buffer
  for(int i=0; i<n; i++){
    client.write(buf[i]);
  }

}


//gets the classification string from the server
int getServerResponse(){
  //value of -1 means there was an error in getting the classification
  int classification = -1;
  int maxloops = 0;
  //wait for the server's reply to become available -- up to 5 seconds
  while (!client.available() && maxloops < 5000)
  {
    maxloops++;
    delay(1); //delay 1 msec
  }
  //do we have data available?
  if (client.available() > 0)
  {
    //Then get the response
    String response = client.readStringUntil('\r');
    Serial.println("Received from server: " + response);

    //make sure its correct format <START_CLASSIFICATION>1<END_CLASSIFICATION>
    String start_classification = response.substring(0, 22);
    String raw_classification = response.substring(22, 23);
    Serial.println("classification received: " + raw_classification);
    String end_classification = response.substring(23);
    //convert classification to int
    classification = raw_classification.toInt();
  }
  //if not that means we timed out
  else
  {
    Serial.println("Error: client.available() timed out.");
  }
  return classification;
}


//sends the classification out through SPI back to the STM32
void sendClassification(int classification){
  //build the classification stream: <START_CLASSIFICATION>[ classification ]<END_CLASSIFICATION>
  String SPIResponse = String("----------------") + String(startClassification) + String(classification) + String(endClassification);
  //(print to console)
  Serial.print("Sending string to STM32: ");
  Serial.println(SPIResponse);
  Serial.println();
  //copy to TX buffer
  loadSPITXBuf(SPIResponse);
  Serial.println("The following buffer was sent to STM32 via SPI: ");
  for(int i = 0; i < BLOCK_SIZE; i++){
    Serial.printf("%u, ", spi_slave_classification_buf[i]);
  }
  Serial.println();
  if (slave.remained() == 0) {
    //this is where we get and send SPI data (full-duplex)
    slave.queue(spi_slave_rx_buf, spi_slave_classification_buf, BLOCK_SIZE);
  }

}



//loads the tx buffer with the payload to be sent
void loadSPITXBuf(String payload){
  for(int i = 0; i < payload.length(); i++){
    spi_slave_classification_buf[i] = (uint8_t) payload.charAt(i);
  }
}



//Returns position if s1 is substring of s2; -1 otherwise
int searchForSigInBuf(char* signal, int sizeOfSig, uint8_t* data, int dataSize)
{
    int M = sizeOfSig;
    int N = dataSize;
    int retval = -1;

    /* A loop to slide signal one by one while comparing */
    for (int i = 0; i <= N - M; i++) {
      int j;
      for (j = 0; j < M; j++){
        if (data[i + j] != (uint8_t) signal[j]){
          break;
        }
      }
      if (j == M){
          retval = i;
          break;
      }
    }
    return retval;
}



//resets tcp tx buffer with 0's
void clearTCPBuffer(int size){
  for(int i = 0; i < size; i++){
    tcp_tx_buf[i] = 0;
  }
}

// //resets spi tx buffer with 0's
// void clearSPIBuffer(int size){
//   for(int i = 0; i < size; i++){
//     spi_slave_tx_buf[i] = 7;
//   }
//   if (slave.remained() == 0) {
//     //this is where we get and send SPI data (full-duplex)
//     slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BLOCK_SIZE);
//   }
// }


int searchForDataInStream(char* sig1, int s1, char* sig2, int s2, int dataSize, uint8_t* buf, int bufSize){
  // Serial.printf("Looking for %s...\n", sig1);
  bool found = false;
  int dataInd = 0;

  clearBuf(buf, bufSize);

  while(true){
    if (slave.remained() == 0) {
      //we're looking for <SIGNAL>data<SIGNAL>
      slave.queue(buf, s1+s2+dataSize);
    }
    while(slave.available()){

      //use for debugging: 
      // Serial.println("SPI data received: ");
      // for(int i = 0; i < s1+s2+dataSize; i++){
      //   Serial.printf("%c\t", buf[i]);
      // }
      // Serial.println();

      int result = searchForSigInBuf(sig1, s1, buf, bufSize);
      if(result >= 0){
        // Serial.printf("%s found.\n", sig1);
        dataInd = result+s1;
        found = true;
        return dataInd;
      }
      if(!found){
        slave.pop();
      }
    }
  }
}


char* extractBlockList(int dataSize, char* resultBuf){
  Serial.printf("Extracting block list...\n");
  bool found = false;
  int dataInd = 0;

  while(true){
    if (slave.remained() == 0) {
      //we're looking for <SIGNAL>data<SIGNAL>
      slave.queue(spi_slave_block_list_buf, startBlockNumsLen + endBlockNumsLen + dataSize);
    }
    while(slave.available()){

      //use for debugging: 
      // Serial.println("SPI data received: ");
      // for(int i = 0; i < s1+s2+dataSize; i++){
      //   Serial.printf("%c\t", buf[i]);
      // }
      // Serial.println();

      int result = searchForSigInBuf(startBlockNums, startBlockNumsLen, spi_slave_block_list_buf, max_block_num_list);
      if(result >= 0){
        Serial.printf("Start Block Nums found.\n");
        dataInd = result+startBlockNumsLen;
        found = true;
        for(int i = 0; i < dataSize; i++){
          resultBuf[i] = spi_slave_block_list_buf[dataInd + i];
        }
        return resultBuf;
      }
      if(!found){
        slave.pop();
      }
    }
  }
}



// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓██▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▓▓▓▓██▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒▒▒▓▓▒▒▒▒▒▒▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    ░░░░░░  ░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░  ░░░░    ░░                      ░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░        ░░░░░░░░░░                                    ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒          ░░░░░░░░░░░░                                      ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░        ░░░░░░░░░░░░░░░░                                    ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░      ░░░░░░░░░░░░░░░░░░                                    ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░        ░░░░░░░░░░░░░░░░░░      ░░                            ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░    ░░░░░░░░░░░░▒▒░░░░░░░░    ░░    ░░                        ░░░░▒▒▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░▒▒░░░░░░░░▒▒▒▒░░░░░░        ░░                      ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    ░░                      ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░  ░░░░░░░░░░░░░░░░░░░░░░░░▒▒░░░░░░░░          ░░                ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░  ░░░░░░░░░░▒▒▒▒▒▒▒▒░░▒▒░░░░    ░░░░░░░░░░░░░░░░        ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒░░    ░░░░░░░░░░░░░░░░        ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░▒▒▒▒░░░░░░  ░░░░░░▒▒▓▓▒▒░░░░░░░░▒▒▒▒▒▒░░░░░░          ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░▒▒▒▒░░▒▒░░▒▒▒▒▒▒░░▒▒▓▓▒▒░░░░  ░░░░▒▒░░░░░░░░        ░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░▒▒▒▒▒▒▓▓██████▓▓▓▓▓▓▒▒▓▓░░░░    ░░░░░░▒▒▓▓▓▓▓▓▒▒░░  ░░  ░░░░░░░░▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░▓▓▓▓▓▓▒▒▒▒░░░░░░░░░░▒▒░░░░        ░░░░▒▒▓▓██▓▓██▓▓▒▒░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    ░░░░░░░░░░      ░░░░  ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░        ░░░░▒▒▒▒░░░░                              ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░▒▒░░░░░░░░      ░░▒▒░░▒▒▒▒░░░░  ░░░░░░░░                ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░▒▒░░░░░░░░░░    ░░░░▒▒▒▒▓▓▒▒▒▒▒▒░░░░░░░░░░                ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░▒▒░░░░▒▒░░░░░░    ░░▒▒▒▒▒▒██▒▒▒▒██▒▒░░░░░░░░              ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒▒▒▒▒░░▒▒▒▒░░▒▒▒▒░░░░░░░░░░░░▒▒▓▓▒▒▒▒░░░░░░░░░░  ▒▒░░░░            ░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓██▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░▒▒▒▒▒▒░░░░░░░░░░  ░░░░▒▒░░░░        ░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓████▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░▒▒░░░░░░    ░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓██▒▒▓▓▒▒▒▒▒▒▓▓▓▓▒▒▒▒░░░░░░░░▒▒▒▒▓▓▓▓▓▓▓▓▓▓▒▒░░░░░░░░▒▒▒▒░░░░░░░░░░░░░░░░░░▒▒▓▓░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓██▒▒▒▒▓▓▓▓▓▓▒▒▒▒░░░░░░▒▒▓▓▓▓██████████████▓▓░░░░░░▒▒░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓██▒▒▒▒▓▓▓▓▓▓▒▒▒▒░░░░░░▒▒▓▓██████████████████▓▓▒▒▒▒▒▒░░░░░░░░░░░░░░░░▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒██▒▒▓▓▓▓▓▓▓▓▓▓▒▒░░░░░░▓▓██████████████████████▓▓▓▓▒▒░░░░░░░░░░░░░░░░▒▒  ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒██▒▒▓▓▓▓▓▓▓▓▓▓▒▒░░░░▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▒▒▓▓██████▓▓▓▓▒▒░░░░░░░░░░░░░░░░▒▒░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒▓▓▓▓▓▓▓▓▓▓▒▒░░░░▒▒▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒░░░░░░▒▒▒▒▓▓▒▒░░░░░░░░░░░░░░░░░░  ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒▓▓▓▓▓▓▓▓▒▒▒▒░░▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒░░░░▓▓▒▒░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒▓▓▒▒▓▓▓▓▓▓▓▓▒▒░░░░▒▒▒▒▒▒▓▓▓▓▒▒▒▒░░░░░░░░▒▒▒▒░░░░▒▒▒▒░░░░░░░░░░░░░░  ░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░▒▒░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒▓▓▓▓▓▓▓▓▓▓▒▒▒▒▓▓▒▒▒▒▓▓▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓████▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▒▒▓▓▒▒▓▓▓▓▓▓▓▓▓▓████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒░░░░░░▒▒▒▒▒▒▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒▒▒▒▒▓▓▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒░░░░▒▒▒▒▒▒▒▒░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▒▒░░▒▒▒▒▒▒▓▓▓▓██▓▓▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░    ░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓
// ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▒▒▒▒▒▒▒▒▒▒▓▓▓▓██▓▓▒▒▓▓▓▓██▓▓████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░▒▒░░          ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓
// ▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░▒▒░░▒▒▒▒██▓▓▒▒▒▒▓▓▓▓████████▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒░░░░░░▒▒░░░░              ░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓
// ▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░▒▒██▒▒▒▒▓▓▓▓▓▓▓▓██████▓▓▓▓▓▓▒▒▒▒▒▒▒▒▒▒░░▒▒▒▒░░░░░░░░                  ░░▒▒▒▒▒▒▒▒▒▒▓▓▓▓
// ▒▒▒▒▒▒▒▒▒▒▒▒  ░░░░░░░░░░░░░░▒▒▒▒██▓▓▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒░░░░░░░░░░░░                    ▒▒▒▒▒▒▒▒▒▒▓▓▒▒
// ▒▒▒▒▒▒▒▒░░░░    ░░░░░░░░░░░░░░▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░                    ░░▒▒▒▒▒▒▒▒▒▒▓▓
// ▒▒▒▒░░░░░░░░░░  ░░░░░░░░░░░░░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░                      ░░░░▒▒▒▒▒▒▒▒
// ▒▒░░░░░░░░░░      ░░░░░░░░░░░░░░▓▓▓▓▒▒▒▒▒▒▓▓▒▒▓▓▓▓▓▓▓▓▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░                      ░░░░░░▒▒▒▒▒▒
// ▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░                        ░░░░░░░░▓▓▒▒
// ░░░░░░░░▒▒░░░░  ░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░                      ░░      ░░░░▓
