#include "send_recv.h"

void syserr(const char* msg) { perror(msg); exit(-1); }

typedef struct pkt{
	unsigned char buffer[1024];
	timeval t;
	unsigned int seq_no;
	unsigned int file_rem;	
	unsigned int chk_sum;
}Packet;


// Note: we can change this to be uint32_t and uint16_t to have normal chksum
// But we like to keep it simple.
unsigned int checksum(void *pkt, int size){
	unsigned int sum = 0;
	unsigned char* temp_pkt = (unsigned char*)pkt;
	
	for(int i = 0; i < size; i++){
		sum += temp_pkt[i];
		sum = (sum>>8) + (sum&0xff);
	}
	return ~sum;
}

int sender(char *recv_ip, int recv_port, char* file_name){
	
	/* File + Sanity Chk */
	int file_size, total_size;
	FILE *fp = fopen(file_name, "rb");
	if(fp == NULL){ perror("FP NULL: "); return -1; }
	//Get file length
	fseek(fp, 0, SEEK_END);
	total_size = file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if(recv_ip == 0 || recv_port <= 0){ syserr("Param err\n"); }
	/* File + Sanity Chk */
	
	/* Prepare Socket */
	int sockfd, n;
	struct hostent* addrof_recv;
	struct sockaddr_in serv_addr;
	socklen_t addrlen;
	
	addrof_recv = gethostbyname(recv_ip);
	if(!addrof_recv){ printf("ERROR: no such host: %s\n", recv_ip); return -1; }
	
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd < 0){ syserr("Can't open socket"); }

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = *((struct in_addr*)addrof_recv->h_addr);
	serv_addr.sin_port = htons(recv_port);
	addrlen = sizeof(serv_addr);
	/* Prepare Socket */
	
	/* Prepare Select + GBN */
	fd_set set;
	unsigned int base = 0, next = 0, recv_iter = 0, send_iter = 0, acked = (unsigned int)ceil(file_size/1024 + .01) ;
	double rtt = 0.0;
	timeval timeout;
	timeout.tv_sec = 0; timeout.tv_usec = 10000;
	int sel_result, amo_recv = 0;
	FD_ZERO(&set);				// reset flags
	FD_SET(sockfd, &set);
	
	static const unsigned short int WINDOWSIZE = 100;
	unsigned char** window = (unsigned char**)malloc(WINDOWSIZE * sizeof(Packet));
	memset(window, 0, WINDOWSIZE*sizeof(Packet));	// clean the window
	unsigned char buffer[1025];
	/* Prepare Select + GBN */
	
	printf("***** Sending *****\n");
	printf("Sending File: %s\n", file_name);
	printf("File size %.2f Kb\n", (double)file_size/1024);
	
	// We first send all the packets until size of window or if less send acked
	// Small description: Make the pkt, save pkt in window, send pkt, increase next, 
	// if next is the size of the window then send_iter becomes one (I'll discribe this in the algo_of_window.txt)
	int temp = (acked > 100) ? 100 : acked;
	for(int send = 0; send < temp; send++){
		int index = next - (WINDOWSIZE * send_iter);	
		timeval time;
		
		memset(buffer, 0, sizeof(buffer));
		int amo_read = fread(buffer, sizeof(unsigned char), sizeof(buffer)-1, fp);
		buffer[amo_read] = '\0';
		gettimeofday(&time, 0);
		
		Packet *value = (Packet*)malloc(sizeof(Packet));

		value->seq_no = next;
		value->file_rem = file_size;
		value->t = time;
		memcpy(value->buffer, buffer, sizeof(buffer));
		value->chk_sum = checksum((void*)value, sizeof(Packet)-sizeof(unsigned int));
		
		window[index] = (unsigned char*)value;

		unsigned char* pkt_ar = reinterpret_cast<unsigned char*>(value);
		unsigned char pkt_array[sizeof(Packet)];
		memcpy(pkt_array, pkt_ar, sizeof(Packet));
		
		n = sendto(sockfd, pkt_array, sizeof(Packet), 0, (struct sockaddr*)&serv_addr, addrlen);
		if(n < 0){ syserr("Can't send"); }

		file_size = (file_size - amo_read < 0) ? file_size : file_size - amo_read;
		next++;
		if(next >= WINDOWSIZE*(send_iter+1)){ send_iter++; }
	}
	
	while(acked > 0){
		// select will wait for an event (event: (var > 0) incoming pkt, (var == 0) timeout, (var < 0) err)
		sel_result = select(sockfd+1, &set, 0, 0, &timeout);
		
		if (sel_result < 0){ return -1; }
		else if (sel_result > 0) {
			if(FD_ISSET(sockfd, &set)){
				// receive a pkt
				
				int index = base - (WINDOWSIZE * recv_iter);
				
				timeval t_recv;
				Packet *p_r, *p_s = reinterpret_cast<Packet*>(window[index]);
				unsigned char pkt_array[sizeof(Packet)];
				
				n = recvfrom(sockfd, pkt_array, sizeof(Packet), 0, (struct sockaddr*)&serv_addr, &addrlen);
			  	if(n < 0){ syserr("Can't receive"); }
				
				p_r = reinterpret_cast<Packet*>(pkt_array);
			  	
			  	// seq_no is correct and chk_sum needs to be correct to enter
			  	if(p_r->seq_no == p_s->seq_no){
			  		if(p_r->chk_sum == p_s->chk_sum){
			  			gettimeofday(&t_recv, 0);
			  			
			  			// used to get the throughput
						rtt += ((double)t_recv.tv_sec + (double)t_recv.tv_usec/1000000.0) - ((double)p_s->t.tv_sec + (double)p_s->t.tv_usec/1000000.0);
			  			base++;
			  			if(base >= WINDOWSIZE*(recv_iter+1)){ recv_iter++; }
			  			
			  			acked--;
			  			amo_recv++;
			  			
			  			// when amount received is 100 we received all packets in the window
			  			// now we need to send the next batch/portion/remainder of the file
			  			if(amo_recv >= WINDOWSIZE){
			  				// reset the time sense we received everything
			  				timeout.tv_usec = 10000;
			  				temp = (acked > 100) ? 100 : acked;
			  				memset(window, 0, temp * sizeof(Packet));
				  			for(int send = 0; send < temp; send++){
					  			
					  			int index = next - (WINDOWSIZE * send_iter);
								timeval time;
								
								memset(buffer, 0, sizeof(buffer));
								int amo_read = fread(buffer, sizeof(unsigned char), sizeof(buffer)-1, fp);
								buffer[amo_read] = '\0';
								gettimeofday(&time, 0);
		
								Packet *value = (Packet*)malloc(sizeof(Packet));

								value->seq_no = next;
								value->file_rem = file_size;
								value->t = time;
								memcpy(value->buffer, buffer, sizeof(buffer));
								value->chk_sum = checksum((void*)value, sizeof(Packet)-sizeof(unsigned int));
		
								window[index] = (unsigned char*)value;

								unsigned char* pkt_ar = reinterpret_cast<unsigned char*>(value);
								unsigned char pkt_array[sizeof(Packet)];
								memcpy(pkt_array, pkt_ar, sizeof(Packet));
		
								n = sendto(sockfd, pkt_array, sizeof(Packet), 0, (struct sockaddr*)&serv_addr, addrlen);
								if(n < 0){ syserr("Can't send"); }

								file_size = (file_size - amo_read < 0) ? file_size : file_size - amo_read;
								next++;
								if(next >= WINDOWSIZE*(send_iter+1)){ send_iter++; }
								
							}
							amo_recv = 0;
						}// end if amo_recv
			  		}// end if chk_sum
			  	}// end if seq_no
			}
		}
		else{
			//	time out
			timeout.tv_usec = 10000;
			int index, temp_iter = recv_iter;
			Packet *p;
			
			for(int i = base; i < next; i++){
				index = i - (WINDOWSIZE * temp_iter);
				if(i >= WINDOWSIZE*(temp_iter+1)){ temp_iter++; }
				
				unsigned char pkt_array[sizeof(Packet)];
				memcpy(pkt_array, window[index], sizeof(Packet));
				
				n = sendto(sockfd, pkt_array, sizeof(Packet), 0, (struct sockaddr*)&serv_addr, addrlen);
				if(n < 0) syserr("Can't send");
			}
			
		}
		FD_ZERO(&set);				// reset flags
		FD_SET(sockfd, &set);
	}
	
	printf("Throughput: %.2f bits/sec\n", (total_size*8)/rtt);
	printf("***** Closing Connection *****\n");
	fclose(fp);
	close(sockfd);
	free(window);
	return 0;
}


int receiver(unsigned int portno, char* file_name){
	printf("***** Receiving *****\n");
	printf("Writting File: %s\n", file_name);
	
	/* Prepare File + Keep Recv alive for final pkts */
	int file_size = 1, write_size = 0, done = 0, diff = 0, sel_result, amo_wrote, totalSize, isFirst = 1;
		/* Keep Recv alive for final pkts */
	timeval timeout;
	fd_set set;
		/* Keep Recv alive for final pkts */
	FILE *fp = fopen(file_name, "w+b");
	if(fp == NULL){ perror("FP NULL: "); return -1; }
	/* Prepare File + Keep Recv alive for final pkts */
	
	/*Socket Prepare*/
	int sockfd, n, seq_no_expect = 0;
	unsigned char pkt_array[sizeof(Packet)];
	Packet *p;
	struct sockaddr_in addr_self, addr_recv;
	socklen_t addrlen;	
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){ syserr("Can't open socket"); }
	
	memset(&addr_self, 0, sizeof(addr_self));
	addr_self.sin_family = AF_INET;
	addr_self.sin_addr.s_addr = INADDR_ANY;
	addr_self.sin_port = htons(portno);
	addrlen = sizeof(addr_recv);
	
	if(bind(sockfd, (struct sockaddr*)&addr_self, sizeof(addr_self)) < 0){ syserr("Can't Bind"); }
	/*Socket Prepare*/
	
	do{
		n = recvfrom(sockfd, pkt_array, sizeof(Packet), 0, (struct sockaddr*)&addr_recv, &addrlen);
		if(n < 0){ syserr("Can't receive from client"); }
		
		p = reinterpret_cast<Packet*>(pkt_array);
		
		// Seq_no are received inorder or pkt that were lost when heading to the sender
		if(seq_no_expect >= p->seq_no){
			// Omit the chksum when adding
			if( p->chk_sum && ~checksum(pkt_array, sizeof(Packet)-sizeof(unsigned int)) ){
				if(seq_no_expect == p->seq_no){
					seq_no_expect++;
					
					file_size = p->file_rem;
					write_size = (file_size - 1024 <= 0) ? file_size : 1024;
					
					amo_wrote = fwrite(p->buffer, sizeof(unsigned char), write_size, fp);
					if(isFirst){ totalSize = file_size; isFirst = 0;}
					file_size -= amo_wrote;
				}
				
				if(seq_no_expect > p->seq_no){ diff = seq_no_expect - p->seq_no; }
				n = sendto(sockfd, pkt_array, sizeof(Packet), 0, (struct sockaddr*)&addr_recv, addrlen);
				if(n < 0){ syserr("Can't send"); }
			}
		}
		
		// When file size == 0 make receiver wait 1 sec to see if we have pkt that were lost in transit
		if(file_size == 0 && diff <= 1){
			FD_ZERO(&set);				// reset flags
			FD_SET(sockfd, &set);
			timeout.tv_sec = 1; timeout.tv_usec = 0;
			sel_result = select(sockfd+1, &set, 0, 0, &timeout);
			
			if (sel_result < 0){ return -1; }
			else if (sel_result == 0) { done = 1; }
		}
		
	}while(!done);
	
	printf("Transfer Size: %.2f Kb\nCompleted\n", (double)totalSize/1024);
	printf("***** Closing Connection *****\n");
	close(sockfd);
	fclose(fp);
	return 0;
}



