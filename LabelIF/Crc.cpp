#include "Crc.h"

#define CRC_CONSTANT 0xa001
unsigned short int crc_result=0;
void crc_check (unsigned char crc_data) // crc_data is the nummber of check
{
     unsigned char xor_flag=1;
     unsigned char m;
     unsigned int crc_num;
     crc_result^=crc_data;
     crc_num=crc_result;
     crc_num&=0x0001;
     for (m=0;m<8;m++)
     {
         if (crc_num) xor_flag=1;
         else xor_flag=0;
         crc_result>>=1;
         if (xor_flag) crc_result^=CRC_CONSTANT;
         crc_num=crc_result;
         crc_num&=0x0001;
     }
}


// 计算给定长度数据的16位CRC。
unsigned short int GetCrc16(const BYTE* pData, int nLength)
{
    crc_result = 0xffff;    // 初始化
    
    while(nLength>0)
    {
        crc_check ( *pData );
        nLength--;
        pData++;
    }
    
    return crc_result;    // 取反
}

bool CheckCrc16( const BYTE* pData, int nLength, unsigned short int check_data )
{
    unsigned short int calc_data = GetCrc16( pData, nLength );
    check_data = ( check_data << 8 ) + ( check_data >> 8 );
    if( check_data == calc_data )
    {
        return true;
    }
    else
    {
        DBG(("Crc check error!  check_data:[%x]   calc_data:[%x]\n", check_data, calc_data));
        return false;
    }
}

