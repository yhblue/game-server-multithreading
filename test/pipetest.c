#include <stdio.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

struct close_req
{
	int id;
};

struct send_data_req
{
	int id;               //4
	int size;            //4
	char *buffer;	    //8   max 120
};

#pragma pack(1) 
struct package_close
{
	uint8_t type;
	uint8_t len;
	uint8_t buffer[6];
	struct close_req close;	
};

struct package_send_data
{
	uint8_t type;
	uint8_t len;
//	uint8_t buffer[6];        //8
	struct send_data_req data;//16
};

#pragma pack()

void main(void)
{
		printf("sizeof(package_send_data) = %d\n",sizeof(struct package_send_data));
}