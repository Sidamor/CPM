#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <signal.h>

#include "Shared.h"
#include "SPI.h"
#include "SPI_Define.h"

#define GetBitValue(iByte, iBit)	((iByte>> iBit) & (0x01))


//Spi ∂¡
int SpiIORead(const int fd, unsigned char *pchWData, unsigned char *pchRData, int iLen)
{
	struct spi_ioc_transfer	xfer[2];
	int status;
    //unsigned char ucRecv[64] = "";
    //int i = 0;
	
	memset(xfer, 0, sizeof (xfer));

/*
    DBG(("SendBuf:");
	for (i = 0; i < iLen; i+=2) {
		if (!(i % 8))
			puts("");
		DBG(("[%.2X][%.2X] ", pchWData[i], pchWData[i+1]);
	}
	puts("");*/

 	xfer[0].tx_buf = (__u64) pchWData;
	xfer[0].len = iLen;
    xfer[0].cs_change = 0;
 
	xfer[0].rx_buf = (__u64) pchRData;
	xfer[0].len = iLen;
	xfer[0].cs_change = 0;   
    
    status = ioctl(fd, SPI_IOC_MESSAGE(1), xfer);
	if (status < 0) 
    {
		perror("SPI_IOC_MESSAGE");
		return fail;
	}
    /*
    DBG(("RecvBuf:");
	for (i = 0; i < iLen; i+=2) {
		if (!(i % 8))
			puts("");
		DBG(("[%.2X][%.2X] ", ucRecv[i], ucRecv[i+1]);
	}
	puts("");*/


/*
    //∆Ê≈º–£—È
    if (false == OddEventBitCheck(pchRecvData[1], pchRecvData[0]))
    {
		DBG(("∆Ê≈º–£—È ß∞‹[0x%02x][0x%02x]\n", pchRecvData[1], pchRecvData[0]);
        return fail;
    }

	
	DBG(("response(%2d, %2d): ", iRecvLen, status);
	for (bp = pchRecvData; iRecvLen > 0; iRecvLen--)
	DBG((" %02x", *bp++);
	DBG(("\n");
    */
	return succ;
}

unsigned char OddEvenBitSet(unsigned char uchHighByte, unsigned char uchLowByte)
{
	//bit5±£¡Ù
	int iTotal = 0;
    unsigned char uchReturnByte = uchHighByte;
    //unsigned char uchOldByte = uchHighByte;
    iTotal = GetBitValue(uchHighByte, 0) + 
        	 GetBitValue(uchHighByte, 1) +
        	 GetBitValue(uchHighByte, 2) +
        	 GetBitValue(uchHighByte, 3) +
        	 GetBitValue(uchHighByte, 4) +
        	 GetBitValue(uchHighByte, 6) +
        	 GetBitValue(uchLowByte, 0) + 
        	 GetBitValue(uchLowByte, 1) +
        	 GetBitValue(uchLowByte, 2) +
        	 GetBitValue(uchLowByte, 3) +
        	 GetBitValue(uchLowByte, 4) +
        	 GetBitValue(uchLowByte, 5) +
        	 GetBitValue(uchLowByte, 6) +
        	 GetBitValue(uchLowByte, 7);

    //DBG(("[OddEvenBitSet]-- [%d], [%d]\n", iTotal, iTotal%2);
    iTotal = iTotal%2;
    
    uchReturnByte = uchReturnByte | (iTotal << 7);
    //DBG(("InputValue[0x%02x], OutputValue[0x%02x]\n", uchOldByte, uchReturnByte);
    return uchReturnByte;
	
}

static int OddEventBitCheck(unsigned char uchHighByte, unsigned char uchLowByte)
{
	int iTotal = 0;
    
    iTotal = GetBitValue(uchHighByte, 0) + 
        	 GetBitValue(uchHighByte, 1) +
        	 GetBitValue(uchHighByte, 2) +
        	 GetBitValue(uchHighByte, 3) +
        	 GetBitValue(uchHighByte, 4) +
        	 GetBitValue(uchHighByte, 6) +
        	 GetBitValue(uchLowByte, 0) + 
        	 GetBitValue(uchLowByte, 1) +
        	 GetBitValue(uchLowByte, 2) +
        	 GetBitValue(uchLowByte, 3) +
        	 GetBitValue(uchLowByte, 4) +
        	 GetBitValue(uchLowByte, 5) +
        	 GetBitValue(uchLowByte, 6) +
        	 GetBitValue(uchLowByte, 7);
    
    DBG(("[%s]-- [%d], [%d]\n", __FUNCTION__, iTotal, iTotal%2));

    iTotal = iTotal%2;
    if (iTotal != GetBitValue(uchHighByte, 7))
    {
		return false;
    }
    
	return true;
}


