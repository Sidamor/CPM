#ifndef __INC_DES_H
#define	__INC_DES_H
/*
#pragma pack(1)
#pragma warning(disable:4103)	// warning C4103: file used #pragma pack to change alignment
*/
#define BYTE unsigned char
#define WORD unsigned char
typedef struct CBC_ST
{
	BYTE macKey[16];
	BYTE macBuf[8];
	BYTE macDest[8];
	BYTE macTemp[8];
	BYTE macCount;
} CBC_ST;

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

void vHardwareDES(BYTE  *pbKey, BYTE *pbIn, BYTE *pbOut, BYTE bOp);
void vHardware3DES(BYTE  *pbKey1, BYTE  *pbKey2, BYTE *pbIn, BYTE *pbOut, BYTE bOp);

void DES_CBC(BYTE temp[8], BYTE src[8], BYTE key[8], BYTE dest[8], BYTE op);
void TripleDES_CBC(BYTE temp[8], BYTE src[8], BYTE key[16], BYTE dest[8], BYTE op);
void TripleDES_ECB(BYTE src[8], BYTE key[16], BYTE dest[8], BYTE op);

void TDESCBC_Begin(CBC_ST *mac, BYTE op);
void TDESCBC_Data(BYTE *data, BYTE len);
BYTE TDESCBC_End(BYTE fix, BYTE *padTo);

void MAC_Begin(CBC_ST *mac);
void MAC_Data(BYTE  *data, WORD len);
void MAC_End(BYTE result[4], BYTE fix);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* __INC_DES_H */

