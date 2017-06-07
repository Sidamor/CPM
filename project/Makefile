BCSPROOT 	=  ..
PROJNAME 	=  CPM
INCLUDES 	=  ${BCSPROOT}/inc
PUBLIB 		=  ${BCSPROOT}/lib
SUBDIR 		=  $(BCSPROOT)/$(PROJNAME)
PRJSRC 		=  ${BCSPROOT}/${PROJNAME}/src
SUBINC 		=  ${BCSPROOT}/${PROJNAME}/inc
PUBLIBLIST 	=  $(PUBLIB)/ConnMgr.so $(PUBLIB)/ConnContainer.so $(PUBLIB)/ClsSocket.so $(PUBLIB)/ClsTcp.so $(PUBLIB)/ClsUdp.so  $(PUBLIB)/IniFile.so $(PUBLIB)/MemMgmt.so $(PUBLIB)/MsgCtlBase.so $(PUBLIB)/MsgMgrBase.so $(PUBLIB)/NetIfBase.so $(PUBLIB)/QueueMgr.so $(PUBLIB)/RouteMgr.so $(PUBLIB)/SeqRscMgr.so $(PUBLIB)/Shared.so $(PUBLIB)/ThreadPool.so $(PUBLIB)/Md5.so 

all : $(SUBDIR)/$(PROJNAME)

$(SUBDIR)/$(PROJNAME):
	arm-linux-g++ $(DEBUG_FLAGS) -Wall  -DDEBUG -D_REENTRANT -lpthread -I$(INCLUDES) -I$(SUBINC) -o CPM $(SUBDIR)/src/Init.cpp $(SUBDIR)/src/BestLable.cpp $(SUBDIR)/src/CenterCtl.cpp $(SUBDIR)/src/Apm.cpp $(SUBDIR)/src/Analyser.cpp $(SUBDIR)/src/MsgCtl.cpp $(SUBDIR)/src/SqlCtl.cpp $(SUBDIR)/src/Crc.cpp $(SUBDIR)/src/main.cpp $(PUBLIBLIST) -L $(PUBLIB)/ -lsqlite3
	#arm-linux-g++ $(DEBUG_FLAGS) -Wall  -DDEBUG -D_REENTRANT -lpthread -I$(INCLUDES) -I$(SUBINC) -o $(PROJNAME) $(SUBDIR)/src/DoorManager.cpp  $(SUBDIR)/src/HashTable.cpp  $(SUBDIR)/src/Init.cpp $(SUBDIR)/src/ReaderCtl.cpp $(SUBDIR)/src/Apm.cpp $(SUBDIR)/src/SerialBase.cpp $(SUBDIR)/src/MsgCtl.cpp $(SUBDIR)/src/comm.cpp $(SUBDIR)/src/main.cpp $(PUBLIBLIST) 
clean :
	rm -f CPM
