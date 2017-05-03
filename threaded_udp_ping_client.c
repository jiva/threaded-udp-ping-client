/*
	Name:		Farhan Jiva
	Date:		2-15-09
	Course:		CSCI4760
	Description:	A multi-threaded UDP ping client
*/

#include <stdio.h>			/* printf() sprintf() sscanf() */
#include <stdlib.h>			/* exit() atoi() */
#include <sys/socket.h>		/* socket() connect() send() recv() */
#include <arpa/inet.h>		/* sockaddr_in inet_addr() htons() */
#include <string.h>			/* memset() strncmp() strlen() */
#include <sys/time.h>		/* gettimeofday() */
#include <pthread.h>		/* pthread_create() */
#include <unistd.h>			/* sleep() */

int sock;					/* The socket descriptor */
int minRTT = 1000000;		/* Minimum RTT */
int maxRTT = 0;				/* Maximum RTT */
int avgRTT = 0;				/* Average RTT */
int pcount = 0;				/* Keep count on lost packets */
int times[10][2];			/* Keep track of packet send/recv times */

/* A function which waits for replies from the ping server */
static void *getReplies(void *vptr_args)
{
	while(1)
	{
		/* Time struct */
		struct timeval tv;
		
		/* Parsed sequence number */
		int seq;
		
		/* Time in usec */
		int tusec;
		
		/* Round-trip-time */
		int RTT;
		
		/* Buffer to recv() */
		char readBuf[1024] = {0};
		
		/* Receiving data from ping server */
		recv(sock, readBuf, 1024, 0);
		
		/* Current time in microseconds since the Epoch */
		gettimeofday(&tv, NULL);
		
		/* Set time variable */
		tusec = tv.tv_usec;
		
		/* Increment packet counter */
		pcount++;
		
		/* Parse reply */
		sscanf(readBuf, "PING %d", &seq);
		
		/* Track time in array */
		times[seq][1] = tusec;
		
		/* Calculate RTT */
		RTT = times[seq][1] - times[seq][0];
		
		/* Check if time value has rolled */
		if(RTT < 0)
		{
			RTT = 1000000 - times[seq][0] + times[seq][1];
		}
		
		/* Print reply info */
		printf("Reply from server: time = %d 탎ec\n", RTT);
		
		/* Set ping statistics */
		if(RTT < minRTT)
		{
			minRTT = RTT;
		}
		if(RTT > maxRTT)
		{
			maxRTT = RTT;
		}
		avgRTT += RTT;
	}
}

/* The main function */
int main(int argc, char *argv[])
{
	struct sockaddr_in servAddr;			/* Ping server address */
	unsigned short servPort;				/* Ping server port */
	char *servIP;							/* Ping server IP (dotted quad) */
	int seq;								/* Sequence number */
	struct timeval tv;						/* Time struct */
	time_t tusec;							/* Time in microseconds */
	pthread_t thread;						/* Thread which will wait for replies */
	
	/* Checking number of arguments */
	if(argc != 3)
	{
		printf("Usage:	$ %s <IP> <port>\n", argv[0]);
		exit(-1);
	}
	
	/* Checking if localhost and set IP */
	if(strncmp(argv[1], "localhost", 9) == 0)
	{
		/* Set IP */
		servIP = "127.0.0.1";
	}
	else
	{
		/* Set IP */
		servIP = argv[1];
	}
	
	/* Set port */
	servPort = atoi(argv[2]);

	/* Create a UDP socket */
	if((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		perror("socket()");
		exit(-1);
	}
	
	/* Construct the server address structure */
	memset(&servAddr, 0, sizeof(servAddr));				/* Zero out structure */
	servAddr.sin_family			= AF_INET;				/* Internet address family */
	servAddr.sin_addr.s_addr	= inet_addr(servIP);	/* Server IP address */
	servAddr.sin_port			= htons(servPort);		/* Server port */
	
	/* Establish a connection with the ping server */
	if(connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
	{
		perror("connect()");
		exit(-1);
	}
	
	/* Execute reply-waiting thread */
	if(pthread_create(&thread, NULL, getReplies, NULL) != 0)
	{
		printf("pthread_create() failed\n");
		close(sock);
		exit(-1);
	}
	
	/* Intro message */
	printf("Pinging %s:\n", argv[1]);
	
	/* Send/Recv packets */
	for(seq = 0; seq < 10; seq++)
	{
		/* Packet to be sent */
		char packet[1024];
		
		/* Current time in microseconds since the Epoch */
		gettimeofday(&tv, NULL);
		
		/* Set time variable */
		tusec = tv.tv_usec;
		
		/* Craft the packet */
		sprintf(packet, "PING %d %ld \r\n", seq, tusec);
		
		/* Track time in array */
		times[seq][0] = tusec;
		
		/* Send the packet */
		if(send(sock, packet, strlen(packet), 0) < 0)
		{
			perror("send()");
			exit(-1);
		}
		
		/* Sleep for one second */
		sleep(1);
	}
	
	/* Display ping statistics */
	printf("\nPing statistics for %s:\n", argv[1]);
	printf("\tPackets: Sent = 10, Received = %d, Lost = %d (%d%% loss)\n", pcount, 10 - pcount, ((10-pcount)*100/10));
	printf("Approximate round trip times in micro-seconds:\n");
	printf("\tMinimum = %d 탎ec, Maximum = %d 탎ec, Average = %d 탎ec\n\n", minRTT, maxRTT, avgRTT/pcount);
	
	exit(0);
}
