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
#include <wiringPi.h>
#include <wiringPiSPI.h>
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

//* Socket number definition for Examples *//
#define SOCK_TCPS       0
#define SOCK_UDPS       1

//* Port number definition for Examples *//
#define PORT_TCPS		5100
#define PORT_UDPS       5200


#define RDY		1
#define CS		10
#define RST		5
#define PWR		6

//* SPI Channel *//
#define CHANNEL		0

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
			    .dhcp = NETINFO_STATIC };

//* For TCP client echechok examples; destination network info *//
uint8_t destip[4] = {192, 168, 11, 74};
uint16_t destport = 5000;

/* global value */
uint8_t ret;
unsigned char buffer[100];
uint8_t g_flag;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
static void wizchip_reset(void);
static void wizchip_select(void);
static void wizchip_deselect(void);
static uint8_t wizchip_read(void);
static void wizchip_write(uint8_t wb);
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
	uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2}};
	uint8_t tmp;
	int32_t echoback_ret;
	uint8_t chip_ver;

	printf( "\r\nRaspberry Pi wiringPi SPI test program\r\n" );
	
	if ( wiringPiSetup() == -1 )
	{
		printf("Not wiringPi setup");
		exit( 1 );
	}

	//* W5500 reset *//
	wizchip_reset();

	//* SPI init *//
	pinMode(CS, OUTPUT);
	fd = wiringPiSPISetup(CHANNEL, 8000000);
	//printf( "fd:%d\n", fd);

	//* W5500 Init *//
	reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
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
			return 0;
		}
	} while (tmp == PHY_LINK_OFF);

	//* Network init */
	Net_Conf(gWIZNETINFO);

	//* Ethernet chip version check*//
	chip_ver = getVERSIONR();
	printf("Ethernet Chip version 0x%02x", chip_ver); 

#ifdef _MAIN_DEBUG_
	uint8_t tmpstr[6] = {0,};

	ctlwizchip(CW_GET_ID,(void*)tmpstr);

	printf("\r\n=======================================\r\n");
	printf(" WIZnet RPi device%s -- ver %d.%.2d\r\n", tmpstr, VER_H, VER_L);
	printf("=======================================\r\n");
	printf(">> W5500 chip based Loopback\r\n");
	printf("=======================================\r\n");
	printf("Ethernet Chip version 0x%02x", chip_ver); 

	Display_Net_Conf(); // Print out the network information
#endif

#if 0
	wiz_NetTimeout gtimeout, stimeout;
	stimeout.retry_cnt = 8;
	stimeout.time_100us = 2000;
	wizchip_settimeout(&stimeout);
	wizchip_gettimeout(&gtimeout);
	//printf("gtimeout.retry_cnt:%d time_100us:%d\r\n", gtimeout.retry_cnt, gtimeout.time_100us);
#endif

	g_flag = 0;

	while ( 1 )
	{
		/* Loopback Test: TCP Server and UDP */
		{
			loopback_tcps(SOCK_TCPS, gDATABUF, PORT_TCPS);
			//loopback_udps(SOCK_UDPS, gDATABUF, PORT_UDPS);
			//echoback_ret = loopback_tcpc(SOCK_TCPS, gDATABUF, destip, destport);

			//if(echoback_ret < 0) printf("echoback ret: %ld\r\n", echoback_ret); // TCP Socket Error code
		}
	}
 
	return(0);
}

void wizchip_reset(void)
{
	pinMode(RST, OUTPUT);
	digitalWrite(RST, 0);
	delay(100);
	digitalWrite(RST, 1);
	delay(100);
}

static void wizchip_select(void)
{
	//printf(">> SPI select\r\n");
	digitalWrite(CS, 0);
}

static void wizchip_deselect(void)
{
	//printf("<< SPI deselect\r\n");
	digitalWrite(CS, 1);
}

static uint8_t wizchip_read(void)
{
	uint8_t rb;
	
	ret = wiringPiSPIDataRW(CHANNEL, &rb, 1);
	// printf("SPI read:0x%02x\r\n", rb);
	// printf("read ret : %d\r\n", ret);

	return rb;
}

static void wizchip_write(uint8_t wb)
{
	//printf("SPI write before:0x%02x\r\n", wb);
	ret = wiringPiSPIDataRW(CHANNEL, &wb, 1);
	// printf("read ret : %d\r\n", ret);
}

static void Net_Conf(wiz_NetInfo netinfo)
{
#ifdef _MAIN_DEBUG_
	/*set nNetwork Information */
	printf("\r\n[DEBUG]MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", netinfo.mac[0], netinfo.mac[1], netinfo.mac[2], netinfo.mac[3], netinfo.mac[4], netinfo.mac[5]);
	printf("[DEBUG]IP: %d.%d.%d.%d\r\n", netinfo.ip[0], netinfo.ip[1], netinfo.ip[2], netinfo.ip[3]);
	printf("[DEBUG]GW: %d.%d.%d.%d\r\n", netinfo.gw[0], netinfo.gw[1], netinfo.gw[2], netinfo.gw[3]);
	printf("[DEBUG]SN: %d.%d.%d.%d\r\n", netinfo.sn[0], netinfo.sn[1], netinfo.sn[2], netinfo.sn[3]);
	printf("[DEBUG]DNS: %d.%d.%d.%d\r\n", netinfo.dns[0], netinfo.dns[1], netinfo.dns[2], netinfo.dns[3]);
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
	if(WIZNETINFO.dhcp == NETINFO_DHCP) printf("\r\n===== %s NET CONF : DHCP =====\r\n",(char*)tmpstr);
		else printf("\r\n===== %s NET CONF : Static =====\r\n",(char*)tmpstr);

	printf("\r\nMAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", WIZNETINFO.mac[0], WIZNETINFO.mac[1], WIZNETINFO.mac[2], WIZNETINFO.mac[3], WIZNETINFO.mac[4], WIZNETINFO.mac[5]);
	printf("IP: %d.%d.%d.%d\r\n", WIZNETINFO.ip[0], WIZNETINFO.ip[1], WIZNETINFO.ip[2], WIZNETINFO.ip[3]);
	printf("GW: %d.%d.%d.%d\r\n", WIZNETINFO.gw[0], WIZNETINFO.gw[1], WIZNETINFO.gw[2], WIZNETINFO.gw[3]);
	printf("SN: %d.%d.%d.%d\r\n", WIZNETINFO.sn[0], WIZNETINFO.sn[1], WIZNETINFO.sn[2], WIZNETINFO.sn[3]);
	printf("DNS: %d.%d.%d.%d\r\n", WIZNETINFO.dns[0], WIZNETINFO.dns[1], WIZNETINFO.dns[2], WIZNETINFO.dns[3]);
}
