#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int main(int argc, char *argv[])
{
	int charsetlen=strlen(argv[1]),charoc[300]={0};
	for(int i=0;i<charsetlen;i++) charoc[argv[1][i]]=1;
	FILE *fp;
	if(argc == 2) fp=stdin;
	else
	{
		fp = fopen(argv[2],"r+");
		if(fp == NULL)
		{ 
			fprintf(stderr, "error\n");
			exit(0);
		}
	}
	char qq;
	int ans=0;
	while((qq=fgetc(fp)))
	{
		if(charoc[qq]!=0) ans++;
		if(qq=='\n')
		{
			printf("%d\n",ans);
			ans=0;
		}
		if(feof(fp)) break;
	}
	return 0;
}

