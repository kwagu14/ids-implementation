#include "ESP32DMASPISlave.h"
#include <WiFi.h>
#include <WiFiMulti.h>

//SPI data structures
ESP32DMASPI::Slave slave;
static const uint32_t BUFFER_SIZE = 1024;
uint8_t* spi_slave_tx_buf;
uint8_t* spi_slave_rx_buf;

//edge detection data structures
int ssPinState = 1;
int lastSSPinState =1;
//flags for detecting start and end of SPI transfer
bool endTransmission = false;
bool startTransmission = false;

//wifi data structures
//Port #-- must match with python server
const uint16_t port = 1337; 
//IP Address of my server
const char * host = "192.168.0.94"; 
// Use WiFiClient class to create TCP connections
WiFiClient client;
WiFiMulti wifiMulti;

//signals to be sent to server to help with parsing the TCP stream
String endTransmissionSignal = "<END_TRANSMISSION>";
String startTransmissionSignal = "<START_TRANSMISSION>";
String startBlockSignal = "<START_DATA_BLOCK>";
String endBlockSignal = "<END_DATA_BLOCK>";


void setup() {
  
  //configure serial communication
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  //allocate buffers for DMA transfer
  spi_slave_tx_buf = slave.allocDMABuffer(BUFFER_SIZE);
  spi_slave_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);

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

  //connect to the Wi-Fi network
  wifiMulti.addAP("itonyamovie", "handlog23");
  Serial.print("Waiting for WiFi... ");

  while(wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //set CS pin as input so we can implement edge detection
  pinMode(SS, INPUT);

  //pad the signals
  endTransmissionSignal = padString(endTransmissionSignal);
  startTransmissionSignal = padString(startTransmissionSignal);
  endBlockSignal = padString(endBlockSignal);
  startBlockSignal = padString(startBlockSignal);

  //print them
  Serial.println("=============== SIGNALS ==================");
  Serial.print("Start Transmission: ");
  Serial.println(startTransmissionSignal);
  Serial.print("End Transmission: ");
  Serial.println(endTransmissionSignal);
  Serial.print("Start Block: ");
  Serial.println(startBlockSignal);
  Serial.print("End Block: ");
  Serial.println(endBlockSignal);
  Serial.println("==========================================");

}

void loop() { 

  //edge detection; update with new state upon each iteration
  ssPinState = digitalRead(SS);
  //pin state has changed from HIGH --> LOW or from LOW --> HIGH if this condition is met
  if(ssPinState != lastSSPinState){
    if(ssPinState == HIGH){
      //data transfer has just completed
      Serial.println("Memory transfer complete. Sending end transmission signal to server.");
      //set flag
      endTransmission = true;
    }else{
      //data transfer has just started. 
      Serial.println("Memory transfer has started. Sending signal to server.");
      //set the flag
      startTransmission = true;
    }
  }


  //before we do anything with SPI, see if the transmission has started 
  if(startTransmission){
    //connect to the server
    Serial.print("Connecting to ");
    Serial.println(host);
    //try to connect to the host until connection succeeds
    while(!client.connect(host, port)) {
      Serial.println("Connection failed.");
      Serial.println("Waiting 3 seconds before retrying...");
      delay(3000);
    }
    //then send start signal to server
    client.print(startTransmissionSignal);
    //reset the signal
    startTransmission = false;
  }

  // if there is no SPI transaction in queue, add transaction
  if (slave.remained() == 0) {
      //this is where we get SPI data
      slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);
  }

  //handle the transactions
  while (slave.available()) {
    
    //Only accept & process data if CS pin is LOW
    if(!endTransmission){
      //print the buffer
      Serial.println("Buffer Recieved from SPI:");
      Serial.println("===========================================");
      for(int i=0; i<BUFFER_SIZE; i++){
        Serial.printf("%x ", spi_slave_rx_buf[i]);
      }
      Serial.println();
      Serial.println("===========================================");

      //then send each byte of binary data to the server if client still connected
      if(client.connected()){
        //send start data block signal
        Serial.println("SPI data available; sending start block to server.");
        client.print(startBlockSignal);
        //followed by all bytes
        for(int i=0; i<BUFFER_SIZE; i++){
          client.write(spi_slave_rx_buf[i]);
        }
        //send end data block signal
        Serial.println("Transfer finished; sending end block to server.");
        client.print(endBlockSignal);
      }else{
        Serial.println("Error. Client disconnected.");
        break;
      }

    }

    //pop the SPI transaction so a new one can be handled
    slave.pop();
  }

  //check if we need to send the endTransmission signal
  if(endTransmission){
    //send the signal
    client.print(endTransmissionSignal);
    //disconnect from server
    client.stop();
    //reset flag
    endTransmission = false;
  }
  //update the SS pin state for edge detection
  lastSSPinState = ssPinState;

}


//function to pad the string with the correct number of characters
String padString(String str){
  int len = str.length();

  int padding = BUFFER_SIZE - len;
  for(int i = 0; i < padding; i++){
    str+="-";
  }
  return str;

}
