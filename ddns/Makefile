PROJNAME 	  =  DDNS
SUBINCLUDES =  ../inc
PUBLIB 		  =  ../lib
INCLUDES 	  =  ./inc
SUBDIR 		  =  ./src
PUBLIBLIST 	= $(PUBLIB)/ClsSocket.so $(PUBLIB)/ClsUdp.so  $(PUBLIB)/IniFile.so $(PUBLIB)/MemMgmt.so $(PUBLIB)/MsgMgrBase.so $(PUBLIB)/QueueMgr.so $(PUBLIB)/RouteMgr.so $(PUBLIB)/SeqRscMgr.so $(PUBLIB)/ThreadPool.so $(PUBLIB)/Md5.so 


all : $(SUBDIR)/$(PROJNAME)
$(SUBDIR)/$(PROJNAME):
	arm-linux-g++ $(DEBUG_FLAGS) -Wall  -DDEBUG -D_REENTRANT -lpthread -I$(INCLUDES) -I$(SUBINCLUDES) $(SUBDIR)/DDNSClient.cpp  $(SUBDIR)/Init.cpp $(SUBDIR)/main.cpp -o DDNS $(PUBLIBLIST)
	#arm-linux-g++ $(DEBUG_FLAGS) -Wall  -D_REENTRANT -lpthread -I$(INCLUDES) -I$(SUBINCLUDES) $(SUBDIR)/main.cpp -o DDNS
	
strip:
	arm-linux-strip DDNS
	
clean :
	rm -f DDNS
	

