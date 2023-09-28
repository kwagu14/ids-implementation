#include "ESP32DMASPISlave.h"
#include <WiFi.h>
#include <WiFiMulti.h>

//SPI data structures
ESP32DMASPI::Slave slave;
static const uint32_t BUFFER_SIZE = 1024;
uint8_t* spi_slave_tx_buf;
uint8_t* spi_slave_rx_buf;

uint8_t tcp_tx_buf[BUFFER_SIZE];

//flags for detecting start and end of SPI transfer
bool transmissionStarted = false;
bool transmissionEnded = false;

//wifi data structures
//Port #-- must match with python server
const uint16_t port = 1337; 
//IP Address of my server
const char * host = "172.16.14.91"; 
// Use WiFiClient class to create TCP connections
WiFiClient client;
WiFiMulti wifiMulti;

//signals to help with parsing the data stream received through SPI & sent through TCP
String endTransmissionSignal = "<END_TRANSMISSION>";
String startTransmissionSignal = "<START_TRANSMISSION>";
char* rawStartTransmission = "<START_TRANSMISSION>";
char* rawEndTransmission = "<END_TRANSMISSION>----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
char* startClassification = "<START_CLASSIFICATION>";
char* endClassification = "<END_CLASSIFICATION>";

const int START_TRANSMISSION_SIZE = 20;
const int END_TRANSMISSION_SIZE = 18;
const int START_CLASSIFICATION_SIZE = 22;
const int END_CLASSIFICATION_SIZE = 20;
const int SPI_PAYLOAD_SIZE  = 43; // ------------> <START_CLASSIFICATION>0<END_CLASSIFICATION>

int maxBytes = 262144;
int bytesSent = 0;
int blockNum = 0;


void setup() {
  
  //configure serial communication
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  //allocate buffers for DMA transfer
  spi_slave_tx_buf = slave.allocDMABuffer(BUFFER_SIZE);
  spi_slave_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);
  //make sure they're cleared
  for(int i = 0; i < BUFFER_SIZE; i++){
    spi_slave_tx_buf[i] = 7;
  }

  //configure SPI
  slave.setDataMode(SPI_MODE0);
  slave.setMaxTransferSize(BUFFER_SIZE);
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

  //increase queue size by 256
  // slave.setQueueSize(256);

  //connect to the Wi-Fi network
  wifiMulti.addAP("LA-OAPA", "SpreadInvestigation");
  Serial.print("Waiting for WiFi... ");

  while(wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //pad the signals to the block number for TCP
  endTransmissionSignal = padString(endTransmissionSignal);
  startTransmissionSignal = padString(startTransmissionSignal);

}

void loop() { 


   /**************************** RECEIVE AND HANDLE INCOMMING SPI DATA ***********************************/

  //handle the transactions... first enter a search for <START_TRANSMISSION>
  searchForStartSignal();

  //then once data transmission has started, enter a search for <END_TRANSMISSION>
  receiveSPIData();
  Serial.printf("Number of bytes sent: %d\n", bytesSent);
  int classification = getServerResponse();
  Serial.printf("The classification received: %d\n", classification);
  //send the classification to STM32
  sendClassification(classification);
  if (slave.remained() == 0) {
    slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);
  }
  //once we get to this part, we're finished transmitting the memory. Now we need to wait for the response
  deinitCommunication();

}



/********************************************************** HELPER FUNCTIONS *******************************************************************/

//function to pad a signal; makes it easier for the server to process.
String padString(String str){
  int len = str.length();

  int padding = BUFFER_SIZE - len;
  for(int i = 0; i < padding; i++){
    str+="-";
  }
  return str;

}


//This function needs to be called before sending data to the server
void initCommunication(){
    //reset data management variables
    // blockNum = 0;
    //connect to the server
    Serial.print("Connecting to ");
    Serial.println(host);
    //try to connect to the host until connection succeeds
    while(!client.connect(host, port)) {
      Serial.println("Connection failed.");
      Serial.println("Waiting 3 seconds before retrying...");
      delay(3000);
    }
    //then send start signal to TCP server
    client.print(startTransmissionSignal);
}


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
  //allow this to finish before moving on to anything else
  // delay(1000);

}


//gets a response from the server
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


void sendClassification(int classification){
  //build the classification stream: <START_CLASSIFICATION>[ classification ]<END_CLASSIFICATION>
  String SPIResponse = String("----------------") + String(startClassification) + String(classification) + String(endClassification);
  //(print to console)
  Serial.print("The string to be sent: ");
  Serial.println(SPIResponse);
  Serial.println();
  //copy to TX buffer
  loadSPITXBuf(SPIResponse);
  Serial.println("The following buffer was sent to STM32 via SPI: ");
  for(int i = 0; i < BUFFER_SIZE; i++){
    Serial.printf("%u, ", spi_slave_tx_buf[i]);
  }
  Serial.println();

}



//loads the tx buffer with the payload to be sent
void loadSPITXBuf(String payload){
  for(int i = 0; i < payload.length(); i++){
    spi_slave_tx_buf[i] = (uint8_t) payload.charAt(i);
  }
}



//Returns position if s1 is substring of s2; -1 otherwise
int searchForSig(char* signal, int sizeOfSig, uint8_t* data, int dataSize)
{
    int M = sizeOfSig;
    int N = dataSize;
    int retval = -1;

 
    /* A loop to slide signal one by one while comparing */
    for (int i = 0; i <= N - M; i++) {
      int j;
 
        //for current index i, check for signal match
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

//resets spi tx buffer with 0's
void clearSPIBuffer(int size){
  for(int i = 0; i < size; i++){
    spi_slave_tx_buf[i] = 7;
  }
  if (slave.remained() == 0) {
    //this is where we get and send SPI data (full-duplex)
    slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);
  }
}

void searchForStartSignal(){
  Serial.println("Looking for <START_TRANSMISSION>...");
  while(true){
    if (slave.remained() == 0) {
      //this is where we get and send SPI data (full-duplex)
      slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);
    }
    while(slave.available()){
      //Is the signal contained in the data?
      int result = searchForSig(rawStartTransmission, START_TRANSMISSION_SIZE, spi_slave_rx_buf, BUFFER_SIZE);
      if(result >= 0){
        Serial.println("<START_TRANSMISSION> found.");
        //if so, then set transmissionStarted so the loop exits
        transmissionStarted = true;
        //initialize TCP communication
        initCommunication();
        //grab the data behind the signal
        int dataIndex = result + START_TRANSMISSION_SIZE;
        int sizeOfData = (BUFFER_SIZE - dataIndex);
        for(int i = dataIndex; i < BUFFER_SIZE; i++){
          tcp_tx_buf[i-dataIndex] = spi_slave_rx_buf[i]; 
          //print out this data received from stm32
          //  Serial.printf("0x%02x\t", spi_slave_rx_buf[i]);

          if((i+1)%16 == 0){
            Serial.println();
          }
        }
        if(sizeOfData > 0){
          //print the data being sent to server: 
          // for(int i = 0; i < sizeOfData; i++){
          //   Serial.printf("0x%02x\t", tcp_tx_buf[i]);
          //   if((i+1)%16==0){
          //     Serial.println();
          //   }
          // }
          //send that to the server
          sendTCPData(client, tcp_tx_buf, sizeOfData);
        }
        //clear buffer after sending
        clearTCPBuffer(BUFFER_SIZE);
        //increment bytes sent
        bytesSent += sizeOfData;

      }
      //whether signal was found or not, we have to pop the SPI transaction and get a new one
      slave.pop();
      if(transmissionStarted){
        return;
      }

    }
  }
}


void receiveSPIData(){
  int result;

  while(true){

    if (slave.remained() == 0) {
      //we have 256 buffers to get
      slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);
      // for(int i = 0; i < 256; i++){
      //   slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);
      // }
    }

    while(slave.available()){
      clearSPIBuffer(BUFFER_SIZE);
      //is the end transmission signal contained in the data?
      result = searchForSig(rawEndTransmission, BUFFER_SIZE, spi_slave_rx_buf, BUFFER_SIZE);
      if(result >= 0){
        //if so, grab the data before the signal
        int sizeOfData = result;
        Serial.printf("Size of data: %d\n", sizeOfData);
        for(int i = 0; i < sizeOfData; i++){
          tcp_tx_buf[i] = spi_slave_rx_buf[i];
          //print the data received from the stm32 
          Serial.printf("0x%02x\t", spi_slave_rx_buf[i]);

          if((i+1)%16 == 0){
            Serial.println();
          }

        }
        if(sizeOfData != 0){
          // Serial.println("Sending data: ");
          // //print the buffer received: 
          // Serial.println("===========================================");
          // for(int i = 0; i < sizeOfData; i++){
          //   Serial.printf("0x%02x\t", tcp_tx_buf[i]);
          //   if((i+1)%16==0){
          //     Serial.println();
          //   }
          // }
          //send that to the server
          sendTCPData(client, tcp_tx_buf, sizeOfData);
          //increment bytes sent
          bytesSent += sizeOfData;
        }

        //send the padded end transmission signal
        client.print(endTransmissionSignal);
        //set the transmissionEnded variable
        transmissionEnded = true;
        Serial.println("<END_TRANSMISSION> found.");
      }else{
        //handle data normally by sending to server
        //print the buffer received: 
        for(int i=0; i<BUFFER_SIZE; i++){
          Serial.printf("0x%02x\t", spi_slave_rx_buf[i]);
          if((i+1)%16==0){
            Serial.println();
          }
        }
        //send to the server
        sendTCPData(client, spi_slave_rx_buf, BUFFER_SIZE);
        bytesSent += BUFFER_SIZE;
      }
      clearTCPBuffer(BUFFER_SIZE);
      slave.pop();

    }
    if(transmissionEnded){
      return;
    }
  }
}
