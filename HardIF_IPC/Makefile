main :  ../lib/HardIF.so 

../lib/HardIF.so : 
	g++ -g -m32 -Wall -DDEBUG  -D_REENTRANT -shared -lpthread -I../CPM/inc -I./inc -I../inc -I./  -o../lib/HardIF.so ./HardIF.cpp ./src/Sys485.cpp ./src/LightCtrl.cpp ./src/GSMBaseSiemens.cpp ./src/AudioPlay.cpp ./src/Util.cpp ./src/CodeUtil.cpp ../lib/IniFile.so ../lib/Shared.so ../lib/ThreadPool.so ../lib/MsgMgrBase.so ../lib/MemMgmt.so 
	
clean:
	rm ../lib/HardIF.so
