#include "ESP32DMASPISlave.h"
#include <WiFi.h>
#include <WiFiMulti.h>


//SPI data structures
ESP32DMASPI::Slave slave;
static const uint32_t BLOCK_SIZE = 1024;
static const uint32_t MAX_BLOCKS = 8;
//for sending things to STM32
uint8_t* spi_slave_classification_buf;
//for receiving memory blocks through SPI
uint8_t* spi_slave_mem_block_buf;
//this is for receiving signals
uint8_t* spi_slave_message_buf;
//for receiving the number of blocks
uint8_t* numBlocksToReceive;
uint8_t* spi_slave_block_list_buf;
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
const char * host = "172.16.14.97"; 
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

  //allocate buffers for DMA transfer
  spi_slave_classification_buf = slave.allocDMABuffer(BLOCK_SIZE);
  spi_slave_mem_block_buf = slave.allocDMABuffer(MAX_BLOCKS*BLOCK_SIZE+max_message_size);
  spi_slave_message_buf = slave.allocDMABuffer(max_message_size);
  spi_slave_block_list_buf = slave.allocDMABuffer(max_block_num_list);
  numBlocksToReceive = slave.allocDMABuffer(1);

  //configure SPI
  slave.setDataMode(SPI_MODE0);
  slave.setMaxTransferSize(BLOCK_SIZE*8);
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
  wifiMulti.addAP("LA-OAPA", "ExtraordinaryRover");
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
  //search for the number of blocks
  int dataInd = searchForDataInStream(startSize, startSizeLen, endSize, endSizeLen, 1, spi_slave_message_buf, max_message_size);
  int dataBlocks = spi_slave_message_buf[dataInd];
  Serial.printf("The number of data blocks is: %d\n", dataBlocks);
  int memSize = dataBlocks*1024;
  slave.pop();
  //search for the size of the block number list
  dataInd = searchForDataInStream(startBlockListSize, startBlockListSizeLen, endBlockListSize, endBlockListSizeLen, 2, spi_slave_message_buf, max_message_size);
  int blockNumListSize = spi_slave_message_buf[dataInd];
  Serial.printf("The size of the block list is: %d\n", blockNumListSize);
  slave.pop();
  //get the list of block numbers
  extractBlockList(blockNumListSize, blockNumListAsStr);
  for(int i = 0; i < blockNumListSize; i++){
    Serial.printf("%c", blockNumListAsStr[i]);
  }

  //search for the blocks
  dataInd = searchForDataInStream(startTransmission, startTransmissionLen, endTransmission, endTransmissionLen, memSize, spi_slave_mem_block_buf, MAX_BLOCKS*BLOCK_SIZE+max_message_size);

  // For debugging
  // Serial.println("The first data block found: \n");
  // Serial.println("======================================================================");
  // for(int i = 0; i<1024; i++){
  //   Serial.printf("0x%02x\t", spi_slave_mem_block_buf[i+dataInd]);
  //   if((i+1)%16 == 0){
  //     Serial.printf("\n");
  //   }
  // }
  // Serial.println("The second data block found: \n");
  // Serial.println("======================================================================");
  // for(int i = 1024; i<2048; i++){
  //   Serial.printf("0x%02x\t", spi_slave_mem_block_buf[i+dataInd]);
  //   if((i+1)%16 == 0){
  //     Serial.printf("\n");
  //   }
  // }
  // Serial.println("The third data block found: \n");
  // Serial.println("======================================================================");
  // for(int i = 2048; i<3072; i++){
  //   Serial.printf("0x%02x\t", spi_slave_mem_block_buf[i+dataInd]);
  //   if((i+1)%16 == 0){
  //     Serial.printf("\n");
  //   }
  // }
  // Serial.println("The fourth data block found: \n");
  // Serial.println("======================================================================");
  // for(int i = 3072; i<4096; i++){
  //   Serial.printf("0x%02x\t", spi_slave_mem_block_buf[i+dataInd]);
  //   if((i+1)%16 == 0){
  //     Serial.printf("\n");
  //   }
  // }
  // Serial.println("The fifth data block found: \n");
  // Serial.println("======================================================================");
  // for(int i = 4096; i<5120; i++){
  //   Serial.printf("0x%02x\t", spi_slave_mem_block_buf[i+dataInd]);
  //   if((i+1)%16 == 0){
  //     Serial.printf("\n");
  //   }
  // }

  //send the number of blocks and the block list to the server

  //send the memory to the server 1024 bytes at a time

  //receive the classification from the server

  //send the classification back to the STM32


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
  //wait for the server's reply to become available -- up to 3 seconds
  while (!client.available() && maxloops < 3000)
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
    Serial.println("Raw classification: " + raw_classification);
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


// //sends the classification out through SPI back to the STM32
// void sendClassification(int classification){
//   //build the classification stream: <START_CLASSIFICATION>[ classification ]<END_CLASSIFICATION>
//   String SPIResponse = String("----------------") + String(startClassification) + String(classification) + String(endClassification);
//   //(print to console)
//   Serial.print("The string to be sent: ");
//   Serial.println(SPIResponse);
//   Serial.println();
//   //copy to TX buffer
//   loadSPITXBuf(SPIResponse);
//   Serial.println("The following buffer was sent to STM32 via SPI: ");
//   for(int i = 0; i < BLOCK_SIZE; i++){
//     Serial.printf("%u, ", spi_slave_tx_buf[i]);
//   }
//   Serial.println();

// }



// //loads the tx buffer with the payload to be sent
// void loadSPITXBuf(String payload){
//   for(int i = 0; i < payload.length(); i++){
//     spi_slave_tx_buf[i] = (uint8_t) payload.charAt(i);
//   }
// }



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
  Serial.printf("Looking for %s...\n", sig1);
  bool found = false;
  int dataInd = 0;

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
        Serial.printf("%s found.\n", sig1);
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
