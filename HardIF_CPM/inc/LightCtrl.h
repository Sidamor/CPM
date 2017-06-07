#ifndef _LIGHT_CTRL_H
#define _LIGHT_CTRL_H

#include "ADCBase.h"

#define LIGHT_OFF        (0)
#define LIGHT_ON         (1)


class CLightCtrl
{
public:
	CLightCtrl();
	virtual ~CLightCtrl();

	bool Initialize(int hGPIO, CADCBase* pA, CADCBase* pB);
public:
	static void BackLightSet( int portNum, int status );
	static void AllLightOFF();
	static void AllLightON();
	static void GSMLightOFF();
	static void GSMLightON();
	static void DBGLightOFF();
	static void DBGLightON();
	static void PLALightOFF();
	static void PLALightON();
public:
	static int GetGD0();
};

#endif
