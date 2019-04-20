#include<stdio.h>
#include "list_file.h"
int main(int argc , char **argv)
{
	struct FileNames gg=list_file(argv[1]);
	for(int i=0;i<gg.length;i++)
	{
		printf("%s\n",gg.names[i]);
	}
return 0;
}
