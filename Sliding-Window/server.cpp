#include "send_recv.h"

int main(int argc, char *argv[]){
	if(argc != 3) { 
		fprintf(stderr,"Usage: %s <port> <file name>\n", argv[0]);
		return 1;
	} 
	int portno = atoi(argv[1]);
	char* file_name = argv[2];
	
	receiver(portno, file_name);
	
  	return 0;
}
