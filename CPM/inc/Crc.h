#ifndef _CRC_H
#define _CRC_H

#include "Shared.h"
#include "Define.h"

unsigned short int GetCrc16(const BYTE* pData, int nLength);
bool CheckCrc16( const BYTE* pData, int nLength, unsigned short int check_data );


#endif
