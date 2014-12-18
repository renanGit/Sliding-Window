all: client server


client: client.o send_recv.o
	g++ -g -o $@ $< send_recv.o
	
client.o: client.cpp send_recv.h
	g++ -g -c client.cpp

server: server.o send_recv.o
	g++ -g -o $@ $< send_recv.o
	
server.o: server.cpp send_recv.h
	g++ -g -c server.cpp

send_recv.o: send_recv.cpp
	g++ -g -c send_recv.cpp -lm

clean:
	rm -f client server *.o *~ core

