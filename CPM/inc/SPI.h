#ifndef __SPI_API_H
#define __SPI_API_H

unsigned char OddEvenBitSet(unsigned char uchHighByte, unsigned char uchLowByte);
int SpiIORead(const int fd, unsigned char *pchWData, unsigned char *pchRData, int iLen);


#endif

