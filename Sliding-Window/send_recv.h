#ifndef S_N_R
#define S_N_R

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <string>
#include <iostream>
#include <fstream>
using std::string;
using namespace std;

int sender(char *recv_ip, int recv_port, char* file_name);
int receiver(unsigned int portno, char* file_name);

#endif
