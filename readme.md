## /* update */

sudo apt-get update

## /* get WiringPi */

git clone https://github.com/WiringPi/WiringPi.git

cd WiringPi

./build



## /* Enable the SPI Module */

sudo raspi-config

"Interfacing Options" -> "SPI"



## /* get w5500 loopback source */

git clone https://github.com/Wiznet-OpenHardware/RPi-w5500.git

cd RPi-w5500



## /* build & run */

make

./w5x00_loopback

make clean