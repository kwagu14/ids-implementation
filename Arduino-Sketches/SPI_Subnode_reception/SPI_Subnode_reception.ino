#include "SPISlave.h"

void setup() {
  // put your setup code here, to run once:

  //rate of data transfer; this is for printing to console via uart. 
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  //This is a callback that gets executed when data is received from the master. 
  SPISlave.onData([](uint8_t * data, size_t len){
    //to synchronize data, keep going until we get a 1 (beginning of array)
    int index = 0;
    while(data[index] == 0){
      //skip
      Serial.println("Some data was skipped.");
      index++;
    }

    //then print out the data
    for(int i = 0; i<len-index; i++){
      Serial.printf("%u, ", data[index+i]);
    }
    Serial.println();
  });


  // this callback is invoked when a status has been received from the master.
  // The status register is a special register that both the slave and the master can write to and read from.
  // Can be used to exchange small data or status information
  // SPISlave.onStatus([](uint32_t data) {
  //     Serial.printf("Status: %u\n", data);
  //     SPISlave.setStatus(millis()); //set next status
  // });

  // // This callback is invoked when the master has read the status register
  // SPISlave.onStatusSent([]() {
  //     Serial.println("Status Sent.");
  // });

  // Setup SPI Slave registers and pins
  SPISlave.begin();
  // Additional setting to have MISO change on falling edge
  // SPI1C2 |= 1 << SPIC2MISODM_S;

}

void loop() {
  // put your main code here, to run repeatedly:
}
