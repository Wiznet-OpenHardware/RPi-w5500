## Hardware Connection

![RPI_pin_1](https://github.com/Wiznet-OpenHardware/RPi-w5500/blob/main/images/Driver/RPI_pin_1.jpg)

The Raspberry Pi GPIO used inside W5500 is as follows.

| I/O Function | Pin Name        | Description                |
| :----------: | --------------- | -------------------------- |
|     MOSI     | GPIO12 (BCM 10) | Connected to MOSI on W5500 |
|     MISO     | GPIO13 (BCM 9)  | Connected to MISO on W5500 |
|     SCLK     | GPIO14 (BCM 11) | Connected to SCLK on W5500 |
|     CEn      | GPIO10 (BCM 8)  | Connected to CSn on W5500  |
|    RESET     | GPIO5 (BCM 24)  | Connected to RSTn on W5500 |
|     INTR     | GPIO6 (BCM 25)  | Connected to INTn on W5500 |



## Raspberry Pi update

Update the list of packages on the system with the following command.

```
sudo apt-get update
```

![apt update](C:\Users\Louis\Downloads\img_all\apt update.jpg)



## Raspberry Pi Kernel Version

Check the Raspberry Pi kernel version (works on **ver 5.2** and above)

```
uname -r
```

![WiringPI_build](C:\Users\Louis\Downloads\img_all\WiringPI_build.jpg)



## Connecting the W5500 Driver

Download the dts overlay file via the wget command.

```
wget https://raw.githubusercontent.com/raspberrypi/linux/rpi-5.10.y/arch/arm/boot/dts/overlays/w5500-overlay.dts
```



Here's a look at the file contents. The MAC address can also be set in the dts file.

- Target : SPI 0 
- Interrupt : BCM 25
- Speed : 30Mb

```
vim w5500-overlay.dts
```

![w5500_dts](C:\Users\Louis\Downloads\img_all\Driver\w5500_dts.jpg)



## Putting the w5500 driver file into boot to apply it

Use the dtc command to generate the **"dtbo"** binary file as shown below.  (You can ignore any warning that come up)

```
dtc -I dts -O dtb -o w5500-driver.dtbo w5500-overlay.dts

sudo cp w5500-driver.dtbo /boot/overlays/
```



Open the **config.txt** file into the boot and add the following to it.

```
sudo vim /boot/config.txt

(+add) dtoverlay=w5500-driver
```



**Reboot** to apply the settings.

```
sudo reboot
```



After booting, you'll see that the network driver is enabled.

![enable_eth1](C:\Users\Louis\Downloads\img_all\Driver\enable_eth1.jpg)



## Static IP & DHCP setting

Open **/etc/dhcpcd.conf** for editing vim

```
sudo vim /etc/dhcpcd.conf
```

**Add the following lines** to the bottom of the file. If such lines already exist and are not commented out, remove them.

Replace the comments in brackets in the box below with the correct information. Interface will be *eth0* for Ethernet.

```
interface [INTERFACE]
static_routers=[ROUTER IP]
static domain_name_servers=[DNS IP]
static ip_address=[STATIC IP ADDRESS YOU WANT]/24
```

**Save the file** and **reboot**.