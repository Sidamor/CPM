main :  ../lib/SOIF.so 

../lib/SOIF.so : 
	arm-linux-g++ -g -Wall -DDEBUG  -D_REENTRANT -shared -lpthread -I../CPM/inc -I../inc -I./  -o../lib/SOIF.so ./SOIF.cpp ../lib/IniFile.so ../lib/Shared.so ../lib/ThreadPool.so ../lib/MsgMgrBase.so ../lib/MemMgmt.so  ../lib/HardIF.so

strip:
	arm-linux-strip ../lib/SOIF.so
		
clean:
	rm ../lib/SOIF.so
