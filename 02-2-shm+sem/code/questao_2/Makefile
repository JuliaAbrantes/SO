# APRENDER A FAZER ISSO
#	https://makefiletutorial.com

CXXFLAGS = -Wall -ggdb -pthread

LDFLAGS = -lpthread -lm

TARGETS = client server

all:	$(TARGETS)

client:	memoryHandler.o myApp.o delays.o process.o utils.o
		g++ -o $@ $^ $(LDFLAGS)
		echo "client"

server:	memoryHandler.o myApp.o delays.o process.o utils.o
		g++ -o $@ $^ $(LDFLAGS)
		echo "server"

memoryHandler.o myApp.o :	%.o:	%.h fifo.h delays.h
		g++ -o $@ $^ $(LDFLAGS)
		echo "mem handler"

delay.o:	%.o:	%.h
		echo "delay"

utils.o process.o:	%.o:	%.h dbc.h utils.h
		echo "utils e process"

clean:
		rm -f *.o *~
		echo "cleaning"

cleanall:	clean
		rm -f $(TARGETS)
		echo "cleaning all "
