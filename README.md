# ids-implementation

This repo contains code for memory-based host-based intrusion detection system for IoT devices. 

The IDS is being implemented on an STM32L562QE using the STM32L562E-DK development package. To add wifi capability to the device, we are using an ESP32 WiFi-module which communicates with the STM32 MCU via UART and SPI. This Repo is organized like so: 

* Classification-Server-Scripts: Contains Ramya's python scripts for the CNN model / WAV creation. Also contains some server-side network code that receives memory dumps from the MCU and sends classifications back to the MCU.

* STM32-Examples: contains individual pieces of this project that I developed one-by-one.  It is difficult to develop large projects in the STM32Cube IDE, so I am creating these small examples which will get coalesced into the final product.

* Arduino-Sketches: This contains the code that was written for the ESP32. Currently, the only file actively being used in this project is the SPI_TCP_CLIENT.ino. The others are older snippets that I experimented with. 

* IDS-Code: will contain the final STM32 IDS project once I am able to combine everything. 