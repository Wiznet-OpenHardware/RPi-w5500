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
#include "dhcp.h"
#include "dns.h"

/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
//* Debugging Message Printout enable *//
#define _MAIN_DEBUG_

#define DATA_BUF_SIZE			2048

//* Demo Version *//
#define VER_H		1
#define VER_L		00

/* Socket */
#define SOCKET_DHCP 0
#define SOCKET_DNS 1

/* Retry count */
#define DHCP_RETRY_COUNT 5
#define DNS_RETRY_COUNT 5

//* SPI Conf *//
#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_MODE SPI_MODE_0
#define SPI_BITS_PER_WORD 8
#define SPI_SPEED 5 * 1000 * 1000

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
//*  Buffer Definition for LOOPBACK *//
uint8_t gDATABUF[DATA_BUF_SIZE];

//* Network Configuration *//
wiz_NetInfo gWIZNETINFO = { .mac = {0x00, 0x08, 0xdc, 0x12, 0x34, 0x56},
			    .ip = {192, 168, 11, 2},
			    .sn = {255, 255, 255, 0},
			    .gw = {192, 168, 11, 1},
			    .dns = {8, 8, 8, 8},
			    .dhcp = NETINFO_DHCP };

//* GPIO *//
struct gpiod_chip *chip;
struct gpiod_line *reset_pin, *cs_pin;

//* GPIO pin number *//
int CS_22 = 22;
int RST_24 = 24; 
int INT_25 = 25; 

/* DHCP */
static uint8_t g_dhcp_get_ip_flag = 0;

/* DNS */
static uint8_t g_dns_target_domain[] = "www.wiznet.io";
static uint8_t g_dns_target_ip[4] = {
    0,
};
static uint8_t g_dns_get_ip_flag = 0;

/* Timer */
static volatile uint16_t g_msec_cnt = 0;

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

static void wizchip_dhcp_init(void);
static void wizchip_dhcp_assign(void);
static void wizchip_dhcp_conflict(void);
/**
 * ----------------------------------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------------------------------------
 */
int main( void )
{
	int fd;
	uint8_t tmp;
	uint8_t dhcp_retry = 0;
    uint8_t dns_retry = 0;

	printf( "\r\nRaspberry Pi - W5x00 DHCP & DNS\r\n" );

	//* Initialize *//
	setupGPIO();
	spi_init();

	//* W5500 reset *//
	wizchip_reset();

	//* Ethernet chip Initialize *//
	wizchip_initialize();

	//* Ethernet chip version check *//
	wizchip_check();
#ifdef _MAIN_DEBUG_
	uint8_t tmpstr[6] = {0,};

	ctlwizchip(CW_GET_ID,(void*)tmpstr);

	printf("\r\n=======================================\r\n");
	printf(" WIZnet RPi [%s] -- ver %d.%.2d", tmpstr, VER_H, VER_L);
	printf("\r\n=======================================\r\n");		

#endif

	//* Network init *//
	if (gWIZNETINFO.dhcp == NETINFO_DHCP) // DHCP
    {
        wizchip_dhcp_init();
    }
    else 
    {
        Net_Conf(gWIZNETINFO); // static

		Display_Net_Conf(); // Print out the network information
        /* Get network information */
    }
	DNS_init(SOCKET_DNS, gDATABUF);

	g_flag = 0;
	while ( 1 )
	{
		/* Assigned IP through DHCP */
		if (gWIZNETINFO.dhcp == NETINFO_DHCP)
		{
			g_flag = DHCP_run();

            if (g_flag == DHCP_IP_LEASED)
            {
                if (g_dhcp_get_ip_flag == 0)
                {
                    printf(" DHCP success\n");

                    g_dhcp_get_ip_flag = 1;
                }
            }
            else if (g_flag == DHCP_FAILED)
            {
                g_dhcp_get_ip_flag = 0;
                dhcp_retry++;

                if (dhcp_retry <= DHCP_RETRY_COUNT)
                {
                    printf(" DHCP timeout occurred and retry %d\n", dhcp_retry);
                }
            }

			if (dhcp_retry > DHCP_RETRY_COUNT)
            {
                printf(" DHCP failed\n");

                DHCP_stop();

                while (1)
                    ;
            }
			usleep(1000000);; // wait for 1 second
		}

		if ((g_dns_get_ip_flag == 0) && (g_flag == DHCP_IP_LEASED))
		{
			 while (1)
            {
				if (DNS_run(gWIZNETINFO.dns, g_dns_target_domain, g_dns_target_ip) > 0)
				{
                    printf(" DNS success\n");
                    printf(" Target domain : %s\n", g_dns_target_domain);
                    printf(" IP of target domain : %d.%d.%d.%d\n", g_dns_target_ip[0], g_dns_target_ip[1], g_dns_target_ip[2], g_dns_target_ip[3]);
					printf("=======================================\r\n");

                    g_dns_get_ip_flag = 1;

                    break;
                }
				else
				{
					printf("DNS Fail...\r\n");
					dns_retry++;

					if (dns_retry <= DNS_RETRY_COUNT)
					{
						printf(" DNS timeout occurred and retry %d\n", dns_retry);
					}
				}
				if (dns_retry > DNS_RETRY_COUNT)
				{
					printf(" DNS failed\n");

					while (1)
						;
				}
				usleep(1000000); // wait for 1 second
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
#ifdef _MAIN_DEBUG_
	printf("[DEBUG]SPI initialize Done\r\n");
#endif
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
#ifdef _MAIN_DEBUG_
	printf("[DEBUG]GPIO initialize Done\r\n");
#endif
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

	return rb;
}

static void wizchip_write(uint8_t wb)
{
	//printf(">>SPI write before:0x%02x\r\n", wb);
	spi_transfer(spi_fd, &wb, 1);
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

	//* Ethernet chip version check *//
	wizchip_check();

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
	/*set nNetwork Information */
	// printf("\r\n[DEBUG]MAC	: %02X:%02X:%02X:%02X:%02X:%02X\r\n", netinfo.mac[0], netinfo.mac[1], netinfo.mac[2], netinfo.mac[3], netinfo.mac[4], netinfo.mac[5]);
	// printf("[DEBUG]IP		: %d.%d.%d.%d\r\n", netinfo.ip[0], netinfo.ip[1], netinfo.ip[2], netinfo.ip[3]);
	// printf("[DEBUG]GW		: %d.%d.%d.%d\r\n", netinfo.gw[0], netinfo.gw[1], netinfo.gw[2], netinfo.gw[3]);
	// printf("[DEBUG]SN		: %d.%d.%d.%d\r\n", netinfo.sn[0], netinfo.sn[1], netinfo.sn[2], netinfo.sn[3]);
	// printf("[DEBUG]DNS		: %d.%d.%d.%d\r\n", netinfo.dns[0], netinfo.dns[1], netinfo.dns[2], netinfo.dns[3]);
	ctlnetwork(CN_SET_NETINFO, (void*) &netinfo);
}

static void Display_Net_Conf()
{
	uint8_t tmpstr[6] = {0,};
	wiz_NetInfo WIZNETINFO;

	ctlnetwork(CN_GET_NETINFO, (void*) &WIZNETINFO);
	ctlwizchip(CW_GET_ID,(void*)tmpstr);

	//* Display Network Information *//
	if(WIZNETINFO.dhcp == NETINFO_DHCP)
	{
		printf("\r\n====== %s NETWORK CONF : DHCP ======\r\n",(char*)tmpstr);
	} 
	else
	{
		printf("\r\n===== %s NETWORK CONF : Static =====\r\n",(char*)tmpstr);
	} 
	printf("\r\n=======================================\r\n");
	printf("MAC		: %02X:%02X:%02X:%02X:%02X:%02X\r\n", WIZNETINFO.mac[0], WIZNETINFO.mac[1], WIZNETINFO.mac[2], WIZNETINFO.mac[3], WIZNETINFO.mac[4], WIZNETINFO.mac[5]);
	printf("IP		: %d.%d.%d.%d\r\n", WIZNETINFO.ip[0], WIZNETINFO.ip[1], WIZNETINFO.ip[2], WIZNETINFO.ip[3]);
	printf("GW		: %d.%d.%d.%d\r\n", WIZNETINFO.gw[0], WIZNETINFO.gw[1], WIZNETINFO.gw[2], WIZNETINFO.gw[3]);
	printf("SN		: %d.%d.%d.%d\r\n", WIZNETINFO.sn[0], WIZNETINFO.sn[1], WIZNETINFO.sn[2], WIZNETINFO.sn[3]);
	printf("DNS		: %d.%d.%d.%d\r\n", WIZNETINFO.dns[0], WIZNETINFO.dns[1], WIZNETINFO.dns[2], WIZNETINFO.dns[3]);
	printf("=======================================\r\n");
}

/* DHCP */
static void wizchip_dhcp_init(void)
{
    DHCP_init(SOCKET_DHCP, gDATABUF);

    reg_dhcp_cbfunc(wizchip_dhcp_assign, wizchip_dhcp_assign, wizchip_dhcp_conflict);
}

static void wizchip_dhcp_assign(void)
{
    getIPfromDHCP(gWIZNETINFO.ip);
    getGWfromDHCP(gWIZNETINFO.gw);
    getSNfromDHCP(gWIZNETINFO.sn);
    getDNSfromDHCP(gWIZNETINFO.dns);

    gWIZNETINFO.dhcp = NETINFO_DHCP;

    /* Network initialize */
    Net_Conf(gWIZNETINFO); // apply from DHCP

    Display_Net_Conf();
    printf(" DHCP leased time : %ld seconds\n", getDHCPLeasetime());
}

static void wizchip_dhcp_conflict(void)
{
    printf(" Conflict IP from DHCP\n");

    // halt or reset or any...
    while (1)
        ; // this example is halt.
}
