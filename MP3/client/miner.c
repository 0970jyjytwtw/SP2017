#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "boss.h"
void if_find_treasure(header *res, int input_fd, int output_fd, uint8_t *append, result_header *result_of_find, jobs_header *jobs, int *has_read, int *end_find, int num_of_append, uint8_t *md5)
{
	memset(res, 0, sizeof(*res));
	read(input_fd, res, sizeof(*res));
	if(res->result == HEADER_SEND)
	{
		write(output_fd, append, result_of_find->message.body.datlen);
		printf("I win a %d-treasure! %s\n",jobs->message.body.num_of_0, result_of_find->message.body.md5);
	}
	else if(res->result == HEADER_STATUS)
	{
		fprintf(stderr,"treasure len %d , append len %d\n",jobs->message.body.datlen, num_of_append );
		printf("I'm working on %s\n", md5);
		if_find_treasure(res,input_fd, output_fd, append, result_of_find, jobs, has_read, end_find, num_of_append, md5);
	}
	else
	{
		fprintf(stderr, "no need\n");
		*has_read = 1;
		*end_find = 1;	
	}
	return;
}
int main(int argc, char **argv)
{
  /* parse arguments */
  if (argc != 4)
  {
      fprintf(stderr, "Usage: %s CONFIG_FILE\n", argv[0]);
      exit(1);
  }
  jobs_header sdo;
  other_found_header ijkm;
  char *name = argv[1];
  char *input_pipe = argv[2];
  char *output_pipe = argv[3];
  /* create named pipes */
  int ret;
  ret = mkfifo(input_pipe, 0644);
  assert (!ret);

  ret = mkfifo(output_pipe, 0644);
  assert (!ret);

  /* open pipes */
  int input_fd = open(input_pipe, O_RDONLY);
  assert (input_fd >= 0);

  int output_fd = open(output_pipe, O_WRONLY), has_read=0;
  assert (output_fd >= 0);
	struct timeval tv;
  fd_set readset;
	int first_assign=1, max_append=15, pre_treasure_len;
  uint8_t *treasure;
  /* TODO write your own (1) communication protocol (2) computation algorithm */
  header res;
  uint8_t md5[MD5_LEN] = {0};
  fprintf(stderr,"client : start read\n");
  while(1)
  {
		if(has_read == 0)
		{
			memset(&res, 0, sizeof(res));
			int read_num=0;
  		while(1)
  		{
  			read_num = read(input_fd, &res, sizeof(header));
  			if(read_num > 0)
				{
					fprintf(stderr, "client : read success\n");
					break;
				}
			}
  	}
  	else has_read = 0;
  	int i,j,judans;
 		switch(res.result)
 		{
 			case HEADER_SERVER_START:
				printf("BOSS is mindful.\n");
				int namelen = strlen(name);
				write(output_fd, &namelen, sizeof(namelen));
				write(output_fd, name, strlen(name));
				break;
			case HEADER_ASSIGN:
				fprintf(stderr, "HEADER_ASSIGN\n");
				jobs_header jobs;	
				memset(&jobs, 0, sizeof(jobs));
				complete_read_header(&jobs, input_fd, sizeof(jobs)-sizeof(header));
				fprintf(stderr, "start %u, add %u, datlen %u, n0 %u\n",jobs.message.body.start, jobs.message.body.add, jobs.message.body.datlen, jobs.message.body.num_of_0);
				if(first_assign == 1)// when boss fisrt assign work ,it send entirely treasure
				{
					treasure=(char *)malloc(jobs.message.body.datlen + max_append);
					read(input_fd, treasure, jobs.message.body.datlen);
					pre_treasure_len = jobs.message.body.datlen;
					first_assign=0;
				}
				else
				{
					treasure=(char *)realloc(treasure, pre_treasure_len +jobs.message.body.datlen + max_append);
					read(input_fd, treasure+pre_treasure_len, jobs.message.body.datlen);
					pre_treasure_len+=jobs.message.body.datlen;
				}
				int num_of_append=1,end_find=0;	
				treasure[pre_treasure_len] = 0;
				while(end_find==0)
				{
					fprintf(stderr,"append_num %d\n",num_of_append);
					int start_next=0;
					uint8_t *append = (uint8_t *)malloc(num_of_append);
					memset(append, 0, num_of_append);
					if(num_of_append > max_append) {max_append*=2;treasure = (char *)realloc(treasure, pre_treasure_len + max_append);}
					append[num_of_append-1]+=jobs.message.body.start;				
					result_header result_of_find;
					memset(&result_of_find, 0, sizeof(result_of_find));
					result_of_find.message.body.datlen = 0;
			
					while(start_next==0)
					{
						i=20000;	
						//fprintf(stderr, "%d\n",i);
						while(i--)
						{
							memcpy(treasure + pre_treasure_len, append, num_of_append);
							makemd5(md5, (uint8_t *)treasure, pre_treasure_len + num_of_append);
							memcpy(result_of_find.message.body.md5, md5, MD5_LEN);
							judans=1;
							for(j=0;j<jobs.message.body.num_of_0;j++)
							{
								if(result_of_find.message.body.md5[j]!='0')
								{
									judans=0;
									break;
								}
							}
							if(judans==1)
							{
								if(result_of_find.message.body.md5[jobs.message.body.num_of_0]=='0') judans=0;
							}
							if(judans==1)
							{
								fprintf(stderr, "may find a treasure\n");
								result_of_find.message.hd.result = HEADER_TREASURE_FOUND;
								result_of_find.message.body.num_of_0 = jobs.message.body.num_of_0;
								result_of_find.message.body.datlen = num_of_append;
								write(output_fd, &result_of_find, sizeof(result_of_find));
								if_find_treasure(&res,input_fd, output_fd, append, &result_of_find, &jobs, &has_read, &end_find, num_of_append, md5);
								end_find = 1;
								break;
							}
							else
							{
								j=num_of_append-1;
								uint8_t tmp_append;
								int next_add=jobs.message.body.add, tmp_add;
								for(j;j>=0;j--)
								{
									if(append[j]+next_add > 255)
									{
										if(j==0)
										{
											start_next = 1;
											break;
										}
										tmp_append = append[j]; tmp_add = next_add;
										append[j] = (tmp_append + tmp_add) % 256;
										next_add = (tmp_append + tmp_add) / 256;	
									}	
									else
									{
										append[j] += (uint8_t)next_add;
										break;
									}
								}
							/*	fprintf(stderr,"append :");	
								for(j=0;j<num_of_append;j++) fprintf(stderr,"%u ",append[j]);	
								fprintf(stderr,"\n");	*/
							}
							if(end_find == 1 || start_next == 1) break;
						}
						if(end_find == 1 ) break;
						tv.tv_sec = 0;
	   				tv.tv_usec = 5000;
	    			FD_ZERO(&readset);
						FD_SET(input_fd, &readset);
						select(input_fd+1, &readset, NULL, NULL, &tv);
						if(FD_ISSET(input_fd, &readset))
						{
							memset(&res, 0, sizeof(res));
							int read_number = read(input_fd, &res, sizeof(res));
							if(res.result == HEADER_STATUS)
							{
								fprintf(stderr,"treasure len %d , append len %d\n",jobs.message.body.datlen, num_of_append );
								printf("I'm working on %s\n", md5);
							}
							else if(read_number>0)
							{
								fprintf(stderr,"message is not status\n");
								has_read = 1;
								end_find = 1;
								break;
							}
						}
					}
					num_of_append++;
					free(append);
				}
				break;
			case HEADER_OTHER_FOUND:
				fprintf(stderr, "HEADER_OTHER_FOUND\n");
				other_found_header print_other;		
				memset(&print_other, 0, sizeof(print_other));
				complete_read_header(&print_other, input_fd, sizeof(print_other) - sizeof(header));
				fprintf(stderr,"print other md5 %s, n0 %u, namelen, %u\n",print_other.message.body.md5, print_other.message.body.num_of_0, print_other.message.body.namelen);
				char *other_name = (char *)malloc(print_other.message.body.namelen+2);
				memset(other_name, 0, print_other.message.body.namelen+2);
				read(input_fd, other_name, print_other.message.body.namelen);
				printf("%s wins a %d-treasure! %s\n",other_name, print_other.message.body.num_of_0, print_other.message.body.md5);
				free(other_name);
				break;
			case HEADER_STATUS:
				fprintf(stderr, "HEADER_STATUS\n");
				printf("I'm working on %s\n", md5);
				break;
			case HEADER_QUIT:
				fprintf(stderr, "HEADER_QUIT\n");
				first_assign=1;
				printf("BOSS is at rest.\n");
				break;
		}
	}
  /* To prevent from blocked read on input_fd, try select() or non-blocking I/O */

  return 0;
}
