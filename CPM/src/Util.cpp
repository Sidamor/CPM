#include <string.h>
#include <stdio.h>
#include "Util.h"

int		speed_arr[] = {B115200,	B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300,};
int name_arr[] = {115200, 57600, 38400,  19200,  9600,  4800,  2400,  1200,  300,};
int BAUDRATE(int speed)
{
	int i;
	int ret = 0;
	for( i=0; i < (int)(sizeof(speed_arr)/sizeof(int)); i++)
	{
		if(speed == name_arr[i])
		{
			ret = speed_arr[i];
			break;
		}
	}
	return ret;
}

// 可打印字符串转换为字节数据
// 如："C8329BFD0E01" --> {0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01}
// 输入: pSrc - 源字符串指针
//       nSrcLength - 源字符串长度
// 输出: pDst - 目标数据指针
// 返回: 目标数据长度
int gsmString2Bytes(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	for (int i = 0; i < nSrcLength; i += 2)
	{
		// 输出高4位
		if ((*pSrc >= '0') && (*pSrc <= '9'))
		{
			*pDst = (*pSrc - '0') << 4;
		}
		else
		{
			*pDst = (*pSrc - 'A' + 10) << 4;
		}

		pSrc++;

		// 输出低4位
		if ((*pSrc>='0') && (*pSrc<='9'))
		{
			*pDst |= *pSrc - '0';
		}
		else
		{
			*pDst |= *pSrc - 'A' + 10;
		}

		pSrc++;
		pDst++;
	}

	// 返回目标数据长度
	return (nSrcLength / 2);
}

// 字节数据转换为可打印字符串
// 如：{0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01} --> "C8329BFD0E01" 
// 输入: pSrc - 源数据指针
//       nSrcLength - 源数据长度
// 输出: pDst - 目标字符串指针
// 返回: 目标字符串长度
int gsmBytes2String(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	const char tab[]="0123456789ABCDEF";    // 0x0-0xf的字符查找表

	for (int i = 0; i < nSrcLength; i++)
	{
		*pDst++ = tab[*pSrc >> 4];        // 输出高4位
		*pDst++ = tab[*pSrc & 0x0f];    // 输出低4位
		pSrc++;
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return (nSrcLength * 2);
}

// 7bit编码
// 输入: pSrc - 源字符串指针
//       nSrcLength - 源字符串长度
// 输出: pDst - 目标编码串指针
// 返回: 目标编码串长度
int gsmEncode7bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int nSrc;        // 源字符串的计数值
	int nDst;        // 目标编码串的计数值
	int nChar;        // 当前正在处理的组内字符字节的序号，范围是0-7
	unsigned char nLeft;    // 上一字节残余的数据

	// 计数值初始化
	nSrc = 0;
	nDst = 0;

	// 将源串每8个字节分为一组，压缩成7个字节
	// 循环该处理过程，直至源串被处理完
	// 如果分组不到8字节，也能正确处理
	while (nSrc < nSrcLength)
	{
		// 取源字符串的计数值的最低3位
		nChar = nSrc & 7;

		// 处理源串的每个字节
		if(nChar == 0)
		{
			// 组内第一个字节，只是保存起来，待处理下一个字节时使用
			nLeft = *pSrc;
		}
		else
		{
			// 组内其它字节，将其右边部分与残余数据相加，得到一个目标编码字节
			*pDst = (*pSrc << (8-nChar)) | nLeft;

			// 将该字节剩下的左边部分，作为残余数据保存起来
			nLeft = *pSrc >> nChar;

			// 修改目标串的指针和计数值
			pDst++;
			nDst++;
		}

		// 修改源串的指针和计数值
		pSrc++;
		nSrc++;
	}

	// 返回目标串长度
	return nDst;
}

// 7bit解码
// 输入: pSrc - 源编码串指针
//       nSrcLength - 源编码串长度
// 输出: pDst - 目标字符串指针
// 返回: 目标字符串长度
int gsmDecode7bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	int nSrc;        // 源字符串的计数值
	int nDst;        // 目标解码串的计数值
	int nByte;        // 当前正在处理的组内字节的序号，范围是0-6
	unsigned char nLeft;    // 上一字节残余的数据

	// 计数值初始化
	nSrc = 0;
	nDst = 0;

	// 组内字节序号和残余数据初始化
	nByte = 0;
	nLeft = 0;

	// 将源数据每7个字节分为一组，解压缩成8个字节
	// 循环该处理过程，直至源数据被处理完
	// 如果分组不到7字节，也能正确处理
	while(nSrc<nSrcLength)
	{
		// 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
		*pDst = ((*pSrc << nByte) | nLeft) & 0x7f;

		// 将该字节剩下的左边部分，作为残余数据保存起来
		nLeft = *pSrc >> (7-nByte);

		// 修改目标串的指针和计数值
		pDst++;
		nDst++;

		// 修改字节计数值
		nByte++;

		// 到了一组的最后一个字节
		if(nByte == 7)
		{
			// 额外得到一个目标解码字节
			*pDst = nLeft;

			// 修改目标串的指针和计数值
			pDst++;
			nDst++;

			// 组内字节序号和残余数据初始化
			nByte = 0;
			nLeft = 0;
		}

		// 修改源串的指针和计数值
		pSrc++;
		nSrc++;
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标串长度
	return nDst;
}

// 8bit编码
// 输入: pSrc - 源字符串指针
//       nSrcLength - 源字符串长度
// 输出: pDst - 目标编码串指针
// 返回: 目标编码串长度
int gsmEncode8bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	// 简单复制
	memcpy(pDst, pSrc, nSrcLength);

	return nSrcLength;
}

// 8bit解码
// 输入: pSrc - 源编码串指针
//       nSrcLength -  源编码串长度
// 输出: pDst -  目标字符串指针
// 返回: 目标字符串长度
int gsmDecode8bit(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	// 简单复制
	memcpy(pDst, pSrc, nSrcLength);

	// 输出字符串加个结束符
	*pDst = '\0';

	return nSrcLength;
}

// 正常顺序的字符串转换为两两颠倒的字符串，若长度为奇数，补'F'凑成偶数
// 如："8613851872468" --> "683158812764F8"
// 输入: pSrc - 源字符串指针
//       nSrcLength - 源字符串长度
// 输出: pDst - 目标字符串指针
// 返回: 目标字符串长度
int gsmInvertNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;        // 目标字符串长度
	char ch;            // 用于保存一个字符

	// 复制串长度
	nDstLength = nSrcLength;

	// 两两颠倒
	for(int i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;        // 保存先出现的字符
		*pDst++ = *pSrc++;    // 复制后出现的字符
		*pDst++ = ch;        // 复制先出现的字符
	}

	// 源串长度是奇数吗？
	if(nSrcLength & 1)
	{
		*(pDst-2) = 'F';    // 补'F'
		nDstLength++;        // 目标串长度加1
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nDstLength;
}

// 两两颠倒的字符串转换为正常顺序的字符串
// 如："683158812764F8" --> "8613851872468"
// 输入: pSrc - 源字符串指针
//       nSrcLength - 源字符串长度
// 输出: pDst - 目标字符串指针
// 返回: 目标字符串长度
int gsmSerializeNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;        // 目标字符串长度
	char ch;            // 用于保存一个字符

	// 复制串长度
	nDstLength = nSrcLength;

	// 两两颠倒
	for(int i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;        // 保存先出现的字符
		*pDst++ = *pSrc++;    // 复制后出现的字符
		*pDst++ = ch;        // 复制先出现的字符
	}

	// 最后的字符是'F'吗？
	if(*(pDst-1) == 'F')
	{
		pDst--;
		nDstLength--;        // 目标字符串长度减1
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nDstLength;
}



void timet2chars(time_t t_time, char* pTime)
{
	int year=0,mon=0,day=0,hour=0,min=0,sec=0;
	struct tm *m_tm;
	m_tm = localtime(&t_time);
	year = m_tm->tm_year+1900;
	mon = m_tm->tm_mon+1;
	day = m_tm->tm_mday;
	hour = m_tm->tm_hour;
	min = m_tm->tm_min;
	sec = m_tm->tm_sec;
	sprintf(pTime, "%04d-%02d-%02d %02d:%02d:%02d", year, mon, day, hour, min, sec);
}

void QuickSortRun(float* pData,int left,int right) 
{ 
    int i,j; 
    float middle,iTemp; 
    i = left; 
    j = right; 
    middle = pData[(left+right)/2]; //求中间值 
    do{ 
      while((pData[i]<middle) && (i<right))//从左扫描大于中值的数 
        i++;      
      while((pData[j]>middle) && (j>left))//从右扫描大于中值的数 
        j--; 
      if(i<=j)//找到了一对值 
      { 
        //交换 
        iTemp = pData[i]; 
        pData[i] = pData[j]; 
        pData[j] = iTemp; 
        i++; 
        j--; 
      } 
    }while(i<=j);//如果两边扫描的下标交错，就停止（完成一次） 

    //当左边部分有值(left<j)，递归左半边 
    if(left<j) 
      QuickSortRun(pData,left,j); 
    //当右边部分有值(right>i)，递归右半边 
    if(right>i) 
      QuickSortRun(pData,i,right); 
} 

void QuickSort(float* pData,int Count) 
{ 
    QuickSortRun(pData,0,Count-1); 
} 

