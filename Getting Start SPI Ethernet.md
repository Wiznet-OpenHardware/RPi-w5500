## Hardware Connection

![RPI_pin](C:\Users\Louis\Downloads\img_all\RPI_pin.jpg)

The Raspberry Pi GPIO used inside W5500 is as follows.

| I/O Function | Pin Name        | Description                |
| :----------: | --------------- | -------------------------- |
|     MOSI     | GPIO12 (BCM 10) | Connected to MOSI on W5500 |
|     MISO     | GPIO13 (BCM 9)  | Connected to MISO on W5500 |
|     SCLK     | GPIO14 (BCM 11) | Connected to SCLK on W5500 |
|     CEn      | GPIO3 (BCM 8)   | Connected to CSn on W5500  |
|    RESET     | GPIO5 (BCM 24)  | Connected to RSTn on W5500 |
|     INTR     | GPIO6 (BCM 25)  | Connected to INTn on W5500 |



## Raspberry Pi update

Update the list of packages on the system with the following command.

```
sudo apt-get update
```

![apt update](C:\Users\Louis\Downloads\img_all\apt update.jpg)



## Build WiringPi

**WiringPi** is **PRE-INSTALLED** with standard Raspbian systems. To update or install on a Raspbian-Lite system:

```
git clone https://github.com/WiringPi/WiringPi.git

cd WiringPi

./build
```

![WiringPI_build](C:\Users\Louis\Downloads\img_all\WiringPI_build.jpg)

First check that "wiringPi" is not already installed. In a terminal, run:

```
gpio -v
```

![GPIO_version](C:\Users\Louis\Downloads\img_all\GPIO_version.jpg)



## Enable the SPI Module

Enter raspi-config in the terminal to open the serial port I2C (refer to the configuration to open ssh Interface Options:

```
sudo raspi-config
```

![raspi-config](C:\Users\Louis\Downloads\img_all\raspi-config.jpg)

**"Interfacing Options" -> "SPI" enable**

![RPi_SPI](C:\Users\Louis\Downloads\img_all\RPi_SPI.jpg)

Test that the SPI is enabled:

```
ls -l /dev/spidev*
```

![spi_enable](C:\Users\Louis\Downloads\img_all\spi_enable.jpg)

## get w5500 loopback source

Get the W5500 code:

```
git clone https://github.com/Wiznet-OpenHardware/RPi-w5500.git

cd RPi-w5500
```



## build & run

**"make"** command to compile it. This will create an executable called W5x00_loopback.

```
make
```

![RPI_W5500_make](C:\Users\Louis\Downloads\img_all\RPI_W5500_make.jpg)

When you run the executable, you can run the **Loopback** code as shown below.

```
./w5x00_loopback
```

![RPI_loopback_run](C:\Users\Louis\Downloads\img_all\RPI_loopback_run.jpg)

When you run the executable, you can run the **DHCP & DNS** code as shown below.

```
./w5x00_dhcp_dns
```

![RPI_dhcpdns_run](C:\Users\Louis\Downloads\img_all\RPI_dhcpdns_run.jpg)