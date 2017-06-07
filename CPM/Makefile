BCSPROOT 	=  ..
PROJNAME 	=  CPM
INCLUDES 	=  ${BCSPROOT}/inc
PUBLIB 		=  ${BCSPROOT}/lib
SUBDIR 		=  $(BCSPROOT)/$(PROJNAME)
PRJSRC 		=  ${BCSPROOT}/${PROJNAME}/src
SUBINC 		=  ${BCSPROOT}/${PROJNAME}/inc
PUBLIBLIST 	=  $(PUBLIB)/ConnMgr.so $(PUBLIB)/ConnContainer.so $(PUBLIB)/ClsSocket.so $(PUBLIB)/ClsTcp.so $(PUBLIB)/ClsUdp.so  $(PUBLIB)/IniFile.so $(PUBLIB)/MemMgmt.so $(PUBLIB)/MsgCtlBase.so $(PUBLIB)/MsgMgrBase.so $(PUBLIB)/NetIfBase.so $(PUBLIB)/QueueMgr.so $(PUBLIB)/RouteMgr.so $(PUBLIB)/SeqRscMgr.so $(PUBLIB)/Shared.so $(PUBLIB)/ThreadPool.so $(PUBLIB)/Md5.so $(PUBLIB)/HardIF.so  $(PUBLIB)/LabelIF.so 

all : $(SUBDIR)/$(PROJNAME)

$(SUBDIR)/$(PROJNAME):
	arm-linux-g++  -Wall -DDEBUG -ldl -D_REENTRANT -lpthread -I$(INCLUDES) -I$(SUBINC) $(SUBDIR)/src/Transparent.cpp $(SUBDIR)/src/OnTimeCtrl.cpp $(SUBDIR)/src/DeviceIfAD.cpp $(SUBDIR)/src/DeviceIfBase.cpp $(SUBDIR)/src/DeviceIf485.cpp $(SUBDIR)/src/DevInfoContainer.cpp $(SUBDIR)/src/NetApp.cpp $(SUBDIR)/src/NetDataContainer.cpp $(SUBDIR)/src/GSMCtrl.cpp $(SUBDIR)/src/Util.cpp $(SUBDIR)/src/CodeUtil.cpp  $(SUBDIR)/src/Init.cpp $(SUBDIR)/src/Apm.cpp $(SUBDIR)/src/Analyser.cpp $(SUBDIR)/src/MsgCtl.cpp $(SUBDIR)/src/SqlCtl.cpp $(SUBDIR)/src/Crc.cpp $(SUBDIR)/src/main.cpp -o CPM ../lib/EmApi.a ../lib/SpiApi.a $(PUBLIBLIST) -L $(PUBLIB) -lsqlite3 
clean :
	rm -f CPM
