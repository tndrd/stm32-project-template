# Linear Actuator Testbench

This repository contains firmware for the STM32F407DISCOVERY board designed to perform HJL drive identification via CAN bus communication.
The board is controlled through Ethernet connectivity using specialized software.

---

## Build Instructions

### Prerequisites

- **IAR Embedded Workbench for ARM** version 9.30.1 or compatible
- Git for version control and repository management

### Dependencies

This project requires the **stm32++ framework**. Clone it into your workspace:

```bash
git clone https://github.com/vovaok/stm32-plus-plus.git
```

### Building the Project

1. Open IAR Embedded Workbench
2. Import the project by selecting `File → Open → Workspace` and navigating to the project directory
3. Ensure the include paths point correctly to the stm32++ framework dependencies
4. Build the project by selecting `Project → Rebuild All`
5. Choose ST-LINK debugger in the project options
6. Download the firmware by clicking `Download & Debug`, wait for completion
7. Stop debugging

---

## Setup Instructions

### Hardware Requirements

#### Primary Configuration
- STM32F4DISCOVERY development board
- STM32DIS-BB expansion shield
- CAN transceiver IC (e.g., SN65HVD230 or similar)
- USB-Ethernet adapter (if board doesn't have native Ethernet)

#### Alternative Configuration
- [STM32F407VET6 development board](https://sl.aliexpress.ru/p?key=wDSrVFQ)
- USB-Ethernet adapter (if required)

### Hardware Connections

1. Connect the CAN transceiver to the appropriate pins on the discovery board/expansion shield
2. Ensure proper termination resistors are installed on the CAN bus
3. Connect the HJL drive to the CAN bus interface
4. Connect Ethernet cable directly or via USB-Ethernet adapter, configure static IP address (typically 192.168.0.1)
5. Provide appropriate power supply to both the board and the drive

### Software Requirements

- **ONBexplorer** GUI application ([source code](https://github.com/vovaok/ONBexplorer.git))
- **Components5 library** ([source code](https://github.com/vovaok/components5.git))
- **Qt Framework** version 5.15.2

### Building the GUI Software

1. Clone both repositories:
   ```bash
   git clone https://github.com/vovaok/ONBexplorer.git
   git clone https://github.com/vovaok/components5.git
   ```

2. Open the components5 project in Qt Creator
3. Build the library using Qt 5.15.2
4. Open the ONBexplorer project in Qt Creator
5. Ensure the components5 library is properly linked in the project configuration
6. Build the project using Qt 5.15.2

---

## Usage Guide

### Initial Setup

1. Verify all hardware connections
2. Power on the HJL drive and the STM32 board
3. Ensure the CAN bus is properly terminated (typically 120Ω at both ends)

### Operation Procedure

1. **Launch ONBexplorer** application
   - The software should automatically detect and connect to the board

2. **Initialize the Drive**
   - In the parameter list (right panel), locate and click the `reset drive` button
   - Select `drive` from the device list (top left corner)
   - Change the `enable` parameter to `true`

3. **Configure Analysis**
   - Select `Analyzer` from the device list
   - Enter the desired test parameters:
     - Start and stop frequency
	 - Chirp amplitude
     - Test duration
     - Input signal

4. **Execute Test**
   - Click the `Start/stop` button to begin the identification process
   - Monitor the progress through the GUI interface
   - Results will be displayed during the test
