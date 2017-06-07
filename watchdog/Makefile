BCSPROOT 	=  ..
PROJNAME 	=  watchdog
SUBDIR 		=  $(BCSPROOT)/$(PROJNAME)
all : $(SUBDIR)/$(PROJNAME)

$(SUBDIR)/$(PROJNAME):
	arm-linux-gcc  -Wall -D_REENTRANT -DDEBUG  $(SUBDIR)/watchdog.c -o watchdog
strip :
	arm-linux-strip watchdog
clean :
	rm -f watchdog
