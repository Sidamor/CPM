main :  ../lib/HardIF.so 

../lib/HardIF.so : 
	arm-linux-g++ -g -Wall -DDEBUG  -D_REENTRANT -shared -lpthread -I../CPM/inc -I./inc -I../inc -I./  -o../lib/HardIF.so ./HardIF.cpp ./src/Sys485.cpp ./src/ADProc.cpp ./src/ADCBase.cpp ./src/GSMBaseSiemens.cpp ./src/AudioPlay.cpp ./src/LightCtrl.cpp ./src/Util.cpp ./src/CodeUtil.cpp ../lib/IniFile.so ../lib/Shared.so ../lib/ThreadPool.so ../lib/MsgMgrBase.so ../lib/MemMgmt.so  ../lib/EmApi.a ../lib/SpiApi.a

strip:
	arm-linux-strip ../lib/HardIF.so
		
clean:
	rm ../lib/HardIF.so
