#ifndef _LABLE_IF_H_
#define _LABLE_IF_H_

#include "Define.h"

#pragma pack(1)
enum _CPM_STREAM_TYPE
{
	STREAM_CHARS,			//字符流
	STREAM_BYTES,			//字节流
    STREAM_BCD,				//BCD码
    STREAM_BYTES_33,		//减0x33字节流，电能表专用
	STREAM_BCD_2,			//BCD码2
	STREAM_CHARS_EMERSON	//BCD码2
};
//LabelIF_DecodeByFormat返回结果lsd
typedef struct DECODE__RESULT_
{
	int Ret;			//解析结果（成功、失败）
	int DataResult;		//数据结果（正常、异常）
	int YCType;			//异常处理（丢弃、提交Agent）
	int BZType;			//非标准数据（提交so处理、不处理）
	int TCType;			//透明传输（提交透传队列，不提交）
}DECODE_RESULT, *PDECODE_RESULT;

typedef struct FORMAT__TIME_
{
    int TI;
    int M;
    int Y;
    int MO;
    int D;
    int H;
    int MI;
    int S;
}FORMAT_TIME, *PFORMAT_TIME;

typedef struct FORMAT__CHECK_
{
    int DS;       //数据起始位置
    int DE;       //数据结束位置
    int CS;       //校验码起始位置
    int CE;       //校验码结束位置
    int CL;       //校验码长度
    int T;        //校验码计算种类
    int C;        //校验码是否进行高低位变换
}FORMAT_CHECK, *PFORMAT_CHECK;

typedef struct FORMAT__HEAD_
{
    char DT[5];              //数据单位类型，如温度(01)、湿度(02)等
	int ST;                  //码流类型
	int T;                   //数据类型，如整形、浮点型、字符串等
	char BZ[2];				//是否标准数据lsd
	char TC[2];				//是否透明传输lsd
}FORMAT_HEAD, *PFORMAT_HEAD;

typedef struct FORMAT__PARA_
{
	int DataLen;        //数据长度
	int ChangeBytes;        //高低位变换
	int DataType;        //数值类型
	int StartBytes;        //起始位置
	int EndBytes;
    int RT;       //系数类型  0常量 1查表 2截取查表
    int RL;       //当系数需要截取时，截取长度
    int RST;      //系数数值类型
    float R;      //计算用系数,截取时为起始地址
    char DC[3];      //数据计算方法   直接截取/系数值/截取值*系数值
	int LenType;        //数据长度类型，0常规，1倒序
	int BitPos;		//按位取数值时的位数
}FORMAT_PARA, *PFORMAT_PARA;

typedef struct FORMAT_MAP_TABLE_
{
	float ValueX;
	float ValueY;
}FORMAT_MAP_TABLE, *PFORMAT_MAP_TABLE;


#pragma pack()

/******************************************************************************/
/*						 LabelIF   公共函数                                             */
/******************************************************************************/
void 	LabelIF_GetDeviceQueryPrivatePara(char* config_str, PDEVICE_INFO_POLL stDeviceInfoPoll);
void 	LabelIF_GetDeviceAttrQueryPrivatePara(char* config_str, PDEVICE_INFO_POLL stDeviceInfoPoll);
void 	LabelIF_GetDeviceActionPrivatePara(char* config_str, PDEVICE_INFO_ACT stDeviceInfoPoll);
void 	LabelIF_GetDeviceValueLimit(char* config_str, PDEVICE_INFO_POLL stDeviceInfoPoll);
//获取布防时间配置
void	LabelIF_GetDefenceDate(char *strTimeSet, DEVICE_INFO_POLL *hash_node);
//
int 	LabelIF_DecodeActionResponse(PDEVICE_INFO_ACT device_node, unsigned char* buf, int bufLen);
bool 	LabelIF_CheckRecvFormat(char* checkFormat, unsigned char* buf, int bufLen);
//
int 	LabelIF_EncodeFormat(char* format, char* self_id, char* szCmd, char* pInData, unsigned char* pOutBuf, int nBufSize);
int 	LabelIF_DecodeByFormat(PDEVICE_INFO_POLL pDevInfo, unsigned char* pInBuf, int inLen, PQUERY_INFO_CPM_EX stOutBuf);
int 	LabelIF_RangeCheck(PDEVICE_INFO_POLL pDevInfo, int vType, char* recValue, char* lastValue);
bool 	LabelIF_StandarCheck(int vType, char* value, char* standard);
/******************************************************************************/
/*						 私有函数                                             */
/******************************************************************************/
char* GetLable(char* format, char* lableName, int& len, char* outBuf);
char* GetCalLable(char* format, char* lableName, int sublableName, int& len, char* outBuf, unsigned char* tableType);

int GenerateTable(const char* split1, const char* split2, char* inData, PFORMAT_MAP_TABLE mapTable, int size);//L=1,C=1
int GetHeadValue(const char* split, char* Inbuf, PFORMAT_HEAD pFormatHead);
int GetCheckValue(const char* split, char* Inbuf, PFORMAT_CHECK pFormatCheck);
int GetValue(const char* Type, char* Inbuf, PFORMAT_PARA pFormatPara);
int GetValueTime(const char* Type, char* Inbuf, PFORMAT_TIME pFormatTime);
int GetValueByTable(unsigned char calType, float calValue, unsigned char tableType, void* curValue, PFORMAT_PARA rdFormat, FORMAT_HEAD headFormat, PFORMAT_MAP_TABLE mapTable, int mapSize, float* outValue);


int strindex(char *str,char *substr);
int define_sscanf(char *src, char *dst, char* outBuf);

int FormatID(char* format, char* id, char* outBuf, int nBufSize);
int FormatCMD(char* format, char* szCmd, char* outBuf, int nBufSize);
int FormatData(char* format, char* data, char* outBuf, int nBufSize);
int FormatCheck(char* format, char* outBuf, int nBufSize);
int DecodeByFormatEx(const char* D_Format, char* Comparison_table, char* devId,  unsigned char* pInBuf, int inLen, char* stOutBuf);

bool f_valueLimitCheck(int vType, char* value, char* standard, char* LastValue);


#endif
