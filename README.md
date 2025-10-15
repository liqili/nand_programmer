# NANDO (NANDopen) programmer

## General
NANDO is open source NAND programmer based on STM32 processor. It supports parallel NAND and SPI flash programming.

PCB boards:

<img src="img/board.jpeg" width="800">

Application:

<img width="595" alt="image" src="https://github.com/user-attachments/assets/05663cc5-72a1-4f18-bc9c-c09771153af1" />


## Features
- USB interface
- PC client software for Linux & Windows.
- TSOP-48 socket adapter for NAND chip (compatible with TL866 adapter)
- TSOP-48 solder adapter for NAND chip
- 8 bit parallel NAND interface
- SPI interface
- 3.3V NAND power supply
- NAND read,write and erase support
- NAND read of chip ID support
- NAND read of bad blocks
- NAND bad block skip option
- NAND include spare area option
- Open KiCad PCB & Schematic
- Open source code
- Read & Write LEDs indication
- Extendable chip database
- Chip autodetection
- Firmware update
- Windows, MacOS,and Linux host application support

### Supported chips
#### Parallel NAND:
K9F2G08U0C, HY27US08121B, TC58NVG2S3E, F59L2G81A, MX30LF2G18AC and others.

See full list of supported chips [qt/nando_parallel_chip_db.csv](qt/nando_parallel_chip_db.csv)

#### SPI flash
AT45DB021D, MX25L8006E, W25Q16JV and others.

See full list of supported chips [qt/nando_spi_chip_db.csv](qt/nando_spi_chip_db.csv)

## Release binaries
You can build the release binaries using Github actions workflow.


### License
In general the sorce code, PCB and schematic are under GPLv3 license but with limitations of:

firmware/libs/spl/CMSIS/License.doc

firmware/libs/spl/STM32_USB-FS-Device_Driver/ - http://www.st.com/software_license_agreement_liberty_v2

firmware/usb_cdc - http://www.st.com/software_license_agreement_liberty_v2

## WiKi
Check [WiKi](https://github.com/bbogush/nand_programmer/wiki) page for more information.
