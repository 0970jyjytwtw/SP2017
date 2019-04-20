#include<stdio.h>
#include<stdint.h>
#include "md5.h"
#include<string.h>
#include<fcntl.h>
int main(int argc,char **argv)
{
	
	uint8_t a[40],*path=argv[1];
	FILE *fp;
	fp=fopen(path,"rb");
	uint8_t qq[2000]={0},kk,buffer[2000];
	int nowlen=0,read_size;
	while((fread(&kk,1,1,fp)))
	{
		qq[nowlen]=kk;
		nowlen++;
		printf("ok\n");	
	}
	printf(" %d\n",nowlen);
	makemd5(a,qq,strlen(qq));
	printf("%s\n",a);
}
