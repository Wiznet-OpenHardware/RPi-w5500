/**
 * Copyright (c) 2023 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * ----------------------------------------------------------------------------------------------------
 * Includes
 * ----------------------------------------------------------------------------------------------------
 */
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <gpiod.h>

#include "wizchip_conf.h"
#include "loopback.h"

/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
//* Debugging Message Printout enable *//
#define _MAIN_DEBUG_

//* Demo Version *//
#define VER_H		1
#define VER_L		00

//* WIZnet Chip Socket number *//
#define SOCK_TCPS       0
#define SOCK_UDPS       1

//* Port number *//
#define PORT_TCPS		5100
#define PORT_UDPS       5200

//* SPI Conf *//
#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_MODE SPI_MODE_0
#define SPI_BITS_PER_WORD 8
#define SPI_SPEED 10 * 1000 * 1000

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
//*  Buffer Definition for LOOPBACK *//
uint8_t gDATABUF[DATA_BUF_SIZE];

//* Network Configuration *//
wiz_NetInfo gWIZNETINFO = {
	.mac = {0x00, 0x08, 0xdc, 0x12, 0x34, 0x56},
	.ip = {192, 168, 11, 2},
	.sn = {255, 255, 255, 0},
	.gw = {192, 168, 11, 1},
	.dns = {8, 8, 8, 8},
	.dhcp = NETINFO_STATIC
};

//* For TCP client echechok examples; destination network info *//
uint8_t destip[4] = {192, 168, 11, 74};
uint16_t destport = 5000;

//* GPIO *//
struct gpiod_chip *chip;
struct gpiod_line *reset_pin, *cs_pin;

//* GPIO pin number *//
int CS_22 = 22;
int RST_24 = 24; 
int INT_25 = 25; 

/* global value */
uint8_t ret;
uint8_t g_flag;
int spi_fd;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* SPI */
static void spi_init(void);
static void spi_transfer(int spi_fd, uint8_t *tx_buffer, size_t len);
static void spi_receiver(int spi_fd, uint8_t *rx_buffer, size_t len);
static void spi_close(void);

/* GPIO */
static void setupGPIO(void);
static void cleanupGPIO(void);

/* WIZnet chip */
static void wizchip_reset(void);
static void wizchip_select(void);
static void wizchip_deselect(void);
static uint8_t wizchip_read(void);
static void wizchip_write(uint8_t wb);

static void wizchip_check(void);
static void wizchip_initialize(void);
static void Net_Conf(wiz_NetInfo netinfo);
static void Display_Net_Conf();

/**
 * ----------------------------------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------------------------------------
 */
int main( void )
{
	int fd;
	uint8_t tmp;
	int32_t echoback_ret;
	
	printf( "\r\nRaspberry  - W5x00 Loopback\r\n" );

	//* Initialize *//
	setupGPIO();
	spi_init();

	//* W5500 reset *//
	wizchip_reset();

	//* Ethernet chip Initialize *//
	wizchip_initialize();

	//* Ethernet chip version check *//
	wizchip_check();

	//* Network init *//
	Net_Conf(gWIZNETINFO);

#ifdef _MAIN_DEBUG_
	uint8_t tmpstr[6] = {0,};

	ctlwizchip(CW_GET_ID,(void*)tmpstr);

	printf("\r\n=======================================\r\n");
	printf(" WIZnet RPi device [%s] -- ver %d.%.2d\r\n", tmpstr, VER_H, VER_L);
	printf("=======================================\r\n");

	Display_Net_Conf(); // Print out the network information
#endif

	g_flag = 0;

	while ( 1 )
	{
		/* Loopback Test: TCP Server and UDP */
		{
			echoback_ret = loopback_tcps(SOCK_TCPS, gDATABUF, PORT_TCPS);
			//echoback_ret = loopback_udps(SOCK_UDPS, gDATABUF, PORT_UDPS);
			//echoback_ret = loopback_tcpc(SOCK_TCPS, gDATABUF, destip, destport);

			if(echoback_ret < 0)
			{
				printf("echoback ret: %ld\r\n", echoback_ret); // Socket Error code
				break;
			}
		}
	}

	cleanupGPIO();

	return(0);
}

static void spi_init(void)
{
	/*
	 * spi fd open
	 */
	spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("Error opening SPI device");
        exit(EXIT_FAILURE);
    }

	/*
	 * spi mode
	 */
    uint8_t mode = SPI_MODE;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("Error can't set spi mode");
        exit(EXIT_FAILURE);
    }
	if (ioctl(spi_fd, SPI_IOC_RD_MODE, &mode) == -1) {
        perror("Error can't set spi mode");
        exit(EXIT_FAILURE);
    }

	/*
	 * bits per word
	 */
    uint8_t bits = SPI_BITS_PER_WORD;
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("Error setting SPI bits per word");
        exit(EXIT_FAILURE);
    }
	if (ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits) == -1) {
        perror("Error setting SPI bits per word");
        exit(EXIT_FAILURE);
    }

	/*
	 * max speed hz
	 */
    uint32_t speed = SPI_SPEED;
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("Error setting SPI speed");
        exit(EXIT_FAILURE);
    }
	if (ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
        perror("Error setting SPI speed");
        exit(EXIT_FAILURE);
    }
}

static void spi_transfer(int spi_fd, uint8_t *tx_buffer, size_t len)
{
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx_buffer,
		.len = len,
		.speed_hz = SPI_SPEED,
		.bits_per_word = SPI_BITS_PER_WORD,
	};

	if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) == -1) {
        perror("Error during SPI transfer");
        exit(EXIT_FAILURE);
    }
}

static void spi_receiver(int spi_fd, uint8_t *rx_buffer, size_t len)
{
	struct spi_ioc_transfer tr = {
		.rx_buf = (unsigned long)rx_buffer,
		.len = len,
		.speed_hz = SPI_SPEED,
		.bits_per_word = SPI_BITS_PER_WORD,
	};

	if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) == -1) {
        perror("Error during SPI transfer");
        exit(EXIT_FAILURE);
    }
}

static void setupGPIO(void)
{
    chip = gpiod_chip_open("/dev/gpiochip4"); // Replace 0 with the appropriate chip number

	if (!chip) {
        perror("Open chip failed");
    }

	reset_pin = gpiod_chip_get_line(chip, RST_24);
	cs_pin = gpiod_chip_get_line(chip, CS_22);

    if (!reset_pin)
	{
        perror("Get line failed");
        gpiod_chip_close(chip);
    }

    // Request the lines for output
    if (gpiod_line_request_output(reset_pin, "example", 0) < 0 || gpiod_line_request_output(cs_pin, "example", 0)) 
	{
        perror("Request line as output failed");
        gpiod_line_release(reset_pin);
        gpiod_line_release(cs_pin);
        gpiod_chip_close(chip);
    }
}

static void cleanupGPIO(void)
{
    gpiod_line_release(reset_pin);
    gpiod_line_release(cs_pin);
    gpiod_chip_close(chip);
}

static void wizchip_reset(void)
{
    gpiod_line_set_value(reset_pin, 0);
	usleep(100000);
	gpiod_line_set_value(reset_pin, 1);
	usleep(100000);
}

static void wizchip_select(void)
{
	//printf(">> SPI select\r\n");
	gpiod_line_set_value(cs_pin, 0);
}

static void wizchip_deselect(void)
{
	//printf("<< SPI deselect\r\n");
	gpiod_line_set_value(cs_pin, 1);
}

static uint8_t wizchip_read(void)
{
	uint8_t rb;

	spi_receiver(spi_fd, &rb, 1);
	//printf("<<SPI read:0x%02x\r\n", rb);
	// printf("read ret : %d\r\n", ret);

	return rb;
}

static void wizchip_write(uint8_t wb)
{
	//printf(">>SPI write before:0x%02x\r\n", wb);
	spi_transfer(spi_fd, &wb, 1);
	// printf("read ret : %d\r\n", ret);
}

static void wizchip_check(void)
{
	if (getVERSIONR() != 0x04)	printf(" ACCESS ERR : VERSION != 0x04, read value = 0x%02x\n", getVERSIONR());
}

static void wizchip_initialize(void)
{
	uint8_t tmp;
	uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2}};

    /* Deselect the FLASH : chip select high */
    wizchip_deselect();

    /* CS function register */
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);

    /* SPI function register */
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

	if(ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1)
	{
		printf("WIZCHIP Initialized fail.\r\n");
		while(1);

	}

	do
	{
		if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)
		{
			printf("Unknown PHY Link status.\r\n");
		}
	} while (tmp == PHY_LINK_OFF);

}

static void Net_Conf(wiz_NetInfo netinfo)
{
#ifdef _MAIN_DEBUG_
	/*set nNetwork Information */
	// printf("\r\n[DEBUG]MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", netinfo.mac[0], netinfo.mac[1], netinfo.mac[2], netinfo.mac[3], netinfo.mac[4], netinfo.mac[5]);
	// printf("[DEBUG]IP: %d.%d.%d.%d\r\n", netinfo.ip[0], netinfo.ip[1], netinfo.ip[2], netinfo.ip[3]);
	// printf("[DEBUG]GW: %d.%d.%d.%d\r\n", netinfo.gw[0], netinfo.gw[1], netinfo.gw[2], netinfo.gw[3]);
	// printf("[DEBUG]SN: %d.%d.%d.%d\r\n", netinfo.sn[0], netinfo.sn[1], netinfo.sn[2], netinfo.sn[3]);
	// printf("[DEBUG]DNS: %d.%d.%d.%d\r\n", netinfo.dns[0], netinfo.dns[1], netinfo.dns[2], netinfo.dns[3]);
#endif
	ctlnetwork(CN_SET_NETINFO, (void*) &netinfo);
}

static void Display_Net_Conf()
{
	uint8_t tmpstr[6] = {0,};
	wiz_NetInfo WIZNETINFO;

	ctlnetwork(CN_GET_NETINFO, (void*) &WIZNETINFO);
	ctlwizchip(CW_GET_ID,(void*)tmpstr);

	//* Display Network Information *//
	if(WIZNETINFO.dhcp == NETINFO_DHCP) printf("\r\n======= %s NET CONF : DHCP =======\r\n",(char*)tmpstr);
		else printf("\r\n======= %s NET CONF : Static =======\r\n",(char*)tmpstr);

	printf("\r\n=======================================\r\n");
	printf("MAC		: %02X:%02X:%02X:%02X:%02X:%02X\r\n", WIZNETINFO.mac[0], WIZNETINFO.mac[1], WIZNETINFO.mac[2], WIZNETINFO.mac[3], WIZNETINFO.mac[4], WIZNETINFO.mac[5]);
	printf("IP		: %d.%d.%d.%d\r\n", WIZNETINFO.ip[0], WIZNETINFO.ip[1], WIZNETINFO.ip[2], WIZNETINFO.ip[3]);
	printf("GW		: %d.%d.%d.%d\r\n", WIZNETINFO.gw[0], WIZNETINFO.gw[1], WIZNETINFO.gw[2], WIZNETINFO.gw[3]);
	printf("SN		: %d.%d.%d.%d\r\n", WIZNETINFO.sn[0], WIZNETINFO.sn[1], WIZNETINFO.sn[2], WIZNETINFO.sn[3]);
	printf("DNS		: %d.%d.%d.%d\r\n", WIZNETINFO.dns[0], WIZNETINFO.dns[1], WIZNETINFO.dns[2], WIZNETINFO.dns[3]);
	printf("=======================================\r\n");
}
