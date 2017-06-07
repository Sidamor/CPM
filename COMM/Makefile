main :  ../lib/ConnMgr.so ../lib/ConnContainer.so ../lib/ClsSocket.so ../lib/ClsTcp.so ../lib/ClsUdp.so ../lib/IniFile.so ../lib/Md5.so ../lib/MemMgmt.so ../lib/MsgCtlBase.so ../lib/MsgMgrBase.so ../lib/NetIfBase.so ../lib/QueueMgr.so ../lib/RouteMgr.so ../lib/SeqRscMgr.so ../lib/Shared.so ../lib/ThreadPool.so	

../lib/ConnMgr.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/ConnMgr.so ./src/ConnMgr.cpp 

../lib/ConnContainer.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/ConnContainer.so ./src/ConnContainer.cpp 
	
../lib/ClsSocket.so :  
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/ClsSocket.so ./src/ClsSocket.cpp

../lib/ClsTcp.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/ClsTcp.so ./src/ClsTcp.cpp 

../lib/ClsUdp.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/ClsUdp.so ./src/ClsUdp.cpp 
	
../lib/IniFile.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/IniFile.so ./src/IniFile.cpp 

../lib/Md5.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/Md5.so ./src/Md5.cpp

../lib/MemMgmt.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/MemMgmt.so ./src/MemMgmt.cpp 

../lib/MsgCtlBase.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/MsgCtlBase.so ./src/MsgCtlBase.cpp 

../lib/MsgMgrBase.so :
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o ../lib/MsgMgrBase.so ./src/MsgMgrBase.cpp  
		
../lib/NetIfBase.so :  
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/NetIfBase.so ./src/NetIfBase.cpp 
	
../lib/QueueMgr.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/QueueMgr.so ./src/QueueMgr.cpp

../lib/RouteMgr.so: 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/RouteMgr.so ./src/RouteMgr.cpp
	
../lib/SeqRscMgr.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/SeqRscMgr.so  ./src/SeqRscMgr.cpp

../lib/Shared.so :
	arm-linux-g++ -g -DDEBUG -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/Shared.so ./src/Shared.cpp 

../lib/ThreadPool.so : 
	arm-linux-g++ -g -Wall -D_REENTRANT -shared -lpthread -L../lib -I../inc -o../lib/ThreadPool.so ./src/ThreadPool.cpp

strip:
	arm-linux-strip ../lib/*.so
		
clean:
	rm ../lib/*.so

	
