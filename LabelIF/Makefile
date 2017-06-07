main :  ../lib/LabelIF.so 

../lib/LabelIF.so : 
	arm-linux-g++ -g -Wall -DDEBUG  -D_REENTRANT -shared -lpthread -I../inc -o../lib/LabelIF.so ./LabelIF.cpp ./Crc.cpp

strip:
	arm-linux-strip ../lib/LabelIF.so
		
clean:
	rm ../lib/LabelIF.so
