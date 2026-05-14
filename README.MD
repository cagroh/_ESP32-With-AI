\# GR-WM-01



This is my project for a ESP32 AI-powered Water Meter Reader



\## Description

This project comprises of a master device (the MasterBox, or MB) and several Slave Data Collector (SDC) devices that can connect to the mastebox via a LoRa radio, RS485 or RS232.

### Configuration
The MasterBox and the Slaves uses a proprietary technology to recognize and pair with any additional slave boxes, requiring the user just to make physical contact between any MB and SDC boxes to get them working under the same context. 

## The MasterBox
the MB all stores, processes,and displaye the consolidated data to the user.
The MB has a color, graphical display to display the data, and a buzzer to sound any alarm when crytical conditions are reported by any of the SDCs.

## The SDCs
There are several SDC configurations, each one for a typical application:
- SDC-CAM: this device uses an ESP32-CAM for taking pictures of a particular analogical device, like water meters, watt metters, pressure gauges, etc. and forwards those pictures to a MD using LoRa. The MB in its turn conveys those pictures to be processed in the cloud by a server running Python and Open-CV
- SDC-SONO: this device is installed on a reservatoire cover and uses an ESP32 with a sonar used to measure the depth of liquid reservatories up to 4m deep/high. It forwards the readings to a MB using LoRa.
- SDC-THP: this device uses an ESP32 to take constant mesures of tempretature, humidity and pressure of an ambient and send these data to a MB using LoRa.
- Other SDC will come.



\## Getting Started

WIP



\### Prerequisites

The project uses:

\- Arduino IDE

\- ESP32-specific libraries

\- ESP32-CAM libraries

\- Open-MV libraries



\### Installation
git clone https://github.com/cagroh/_ESP32-With-AI
WIP



\## Usage

WIP



\## Contributing

Guidelines for developers who want to contribute. Please read `CONTRIBUTING.md` for details on our code of conduct.



\## License

This project is licensed under the MIT License - see the \[LICENSE.md](LICENSE.md) file for details.



\## Contact

cesar.groh@gmail.com

