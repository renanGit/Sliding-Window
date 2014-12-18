#include "send_recv.h"

int main(int argc, char* argv[]){

	if(argc != 4) {
		fprintf(stderr, "Usage: %s <receiver ip> <port> <file name>\n", argv[0]);
		return 1;
	}
	char* recv_ip = argv[1];
	int recv_port = atoi(argv[2]);
	char* file_name = argv[3];
	
	sender(recv_ip, recv_port, file_name);

	return 0;
}
