#include "ESP32DMASPISlave.h"

#define MOSIPIN 13
#define MISOPIN 12
#define CSPIN 15
#define CLKPIN 14

ESP32DMASPI::Slave slave;
static const uint32_t BUFFER_SIZE = 32;
uint8_t* spi_slave_tx_buf;
uint8_t* spi_slave_rx_buf;

void setup() {
  // put your setup code here, to run once:

  spi_slave_tx_buf = slave.allocDMABuffer(BUFFER_SIZE);
  spi_slave_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);

  //this may need to be changed to mode 1 or 3
  slave.setDataMode(SPI_MODE0);
  slave.setMaxTransferSize(BUFFER_SIZE);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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

  //VSPI, CLK, MISO, MOSI, SS
  slave.begin(VSPI, SCK, MISO, MOSI, SS);

}

void loop() {

  //data is available from master
  // available() returns size of results of transaction,
  // and buffer is automatically updated
   // if there is no transaction in queue, add transaction
  if (slave.remained() == 0) {
      slave.queue(spi_slave_rx_buf, BUFFER_SIZE);
  }

  // do something with received data: spi_slave_rx_buf
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
      Serial.printf("%u ", spi_slave_rx_buf[i]);
    }
    Serial.printf("\n");
    slave.pop();
    delay(1000);


}
