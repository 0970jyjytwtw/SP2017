#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "boss.h"
int boss_has_assign=0;
int load_config_file(struct server_config *config, char *path)
{
    /* TODO finish your own config file parser */
    int fd = open(path,O_RDONLY), readnum,i,miner_num=0,mine_file_done=0,len=0,process=0,need_append=0,in_or_out=0;
    char buffer[4096],*mine_file;
    struct pipe_pair *pipes;
    pipes=(struct pipe_pair *)malloc(sizeof(struct pipe_pair)*9);
    while((readnum=read(fd,buffer,4096))>0)
    {
    	fprintf(stderr, "boss : read %d\n",readnum);
    	i=0;
    	need_append=0;
    	if(mine_file_done==0) //read mine file
    	{
    		if(process==0)
				{
					i+=6;
					process=1;
				}
				else need_append=1;
	    	for(i;i<readnum;i++)
	    	{
	    		if(buffer[i]=='\n')
					{
						if(need_append==1)
						{
							mine_file = (char *)realloc(mine_file ,sizeof(char)*(len+2));
							strncat(mine_file,buffer,i);
							mine_file[len]='\0';
							fprintf(stderr, "mine_file %s\n",mine_file);
							mine_file_done=1;
							len=0;
							i++;
							process=0;
						}
						else
						{		
							mine_file =(char *)malloc(sizeof(char) * (len+2));
							memcpy(mine_file,buffer+i-len,len);
							mine_file[len] = '\0';
							len=0;
							fprintf(stderr, "mine_file %s\n",mine_file);
							mine_file_done=1;
							process=0;
							i++;
						}
						break;
					}
	    		len++;
				}
				if(i>=readnum && process==1 && mine_file_done == 0)
				{
					if(need_append==1)
					{
						mine_file = (char *)realloc(mine_file, sizeof(char)*(len+2));
						strncat(mine_file,buffer,i);
						mine_file[len]='\0';
					}
					else
					{
						mine_file =(char *)malloc(sizeof(char) * (len+2));
						memcpy(mine_file,buffer+i-len,len);
						mine_file[len] = '\0';
					}
					continue;
				}			
			}
			if(mine_file_done != 0)//read miner's pipe path
			{	
				int skip=1;
				if(process==1) need_append=1;
				if(skip==1)
				{
					for(i;i<readnum;i++)
					{
						if(buffer[i]==' '||buffer[i]=='\n')
						{
							i++;
							process=1;
							skip=0;
							break;
						}
					}
				}
				while(i<readnum)
				{
					if(skip==1)
					{
						for(i;i<readnum;i++)
						{
							if(buffer[i]==' '||buffer[i]=='\n')
							{
								i++;
								process=1;
								skip=0;
								break;
							}
						}
					}
					if(i>=readnum) break;
					if(buffer[i]=='\n')
					{
						if(need_append==1)
						{
							pipes[miner_num].output_pipe = (char *)realloc(pipes[miner_num].output_pipe ,sizeof(char)*(len+2));
							strncat(pipes[miner_num].output_pipe,buffer,i);
							pipes[miner_num].output_pipe[len]='\0';	
							fprintf(stderr,"boss :1 num==%d output_pipe %s\n",miner_num,pipes[miner_num].output_pipe);
						}
						else
						{
							pipes[miner_num].output_pipe = (char *)malloc(sizeof(char )* (len+2));
							memcpy(pipes[miner_num].output_pipe, buffer+i-len, len);
							pipes[miner_num].output_pipe[len]='\0';			
							fprintf(stderr,"boss :1 num==%d output_pipe %s\n",miner_num,pipes[miner_num].output_pipe);
						}
						i++; len=0; miner_num++; process=0; in_or_out=0, skip=1;
						continue;	
					}
					else if(buffer[i]==' ')
					{
						if(need_append==1)
						{
							pipes[miner_num].input_pipe = (char *)realloc(pipes[miner_num].input_pipe, sizeof(char) * (len+2));
							strncat(pipes[miner_num].input_pipe,buffer,i);
							pipes[miner_num].input_pipe[len]='\0';
							fprintf(stderr,"boss :1 num==%d input_pipe %s\n",miner_num,pipes[miner_num].input_pipe);
						}
						else
						{
							pipes[miner_num].input_pipe = (char *)malloc(sizeof(char) * (len+2));
							memcpy(pipes[miner_num].input_pipe, buffer+i-len, len);
							pipes[miner_num].input_pipe[len]='\0';
							fprintf(stderr,"boss :1 num==%d input_pipe %s\n",miner_num,pipes[miner_num].input_pipe);
						}
						i++; len=0; process=0; in_or_out=1;
						continue;
					}
					i++;
					len++;
				}
				if(i>=readnum&&process==1)
				{
					if(in_or_out==0)
					{
						if(need_append==1)
						{
							pipes[miner_num].input_pipe = (char *)realloc(pipes[miner_num].input_pipe, sizeof(char) * (len+2));
							strncat(pipes[miner_num].input_pipe,buffer,i);
							pipes[miner_num].input_pipe[len]='\0';
						}
						else
						{
							pipes[miner_num].input_pipe = (char *)malloc(sizeof(char )* (len+2));
							memcpy(pipes[miner_num].input_pipe, buffer+i-len, len);
							pipes[miner_num].input_pipe[len]='\0';
						}
					}
					else
					{
						if(need_append==1)
						{
							pipes[miner_num].output_pipe = (char *)realloc(pipes[miner_num].output_pipe, sizeof(char) * (len+2));
							strncat(pipes[miner_num].output_pipe,buffer,i);
							pipes[miner_num].output_pipe[len]='\0';
						}
						else
						{
							pipes[miner_num].output_pipe = (char *)malloc(sizeof(char)* (len+2));
							memcpy(pipes[miner_num].output_pipe, buffer+i-len, len);
							pipes[miner_num].output_pipe[len]='\0';
						}
					}
				}
			}
		}
    config->mine_file  = mine_file;/* path to mine file */;
    config->pipes      = pipes;/* array of pipe pairs */;
    config->num_miners = miner_num;/* number of miners */;
    close(fd);
    return 0;
}
int first_assign=1;
int assign_jobs(int num_of_0, uint8_t *pre_treasure, int num_miners, struct fd_pair *client_fds, int treasure_len)
{
  /* TODO design your own (1) communication protocol (2) job assignment algorithm */
  jobs_header to_assign;
  memset(&to_assign, 0, sizeof(to_assign));
	to_assign.message.hd.result = HEADER_ASSIGN;
	int i;
  to_assign.message.body.datlen = treasure_len;
  to_assign.message.body.num_of_0 = num_of_0;
  to_assign.message.body.add = num_miners;
  for(i=0;i<num_miners;i++)
  {
  	to_assign.message.body.start = i;
  	write(client_fds[i].input_fd, &to_assign, sizeof(to_assign));
  	write(client_fds[i].input_fd, pre_treasure, to_assign.message.body.datlen);
  	fprintf(stderr, "boss : assign to %d, datlen %u, n0 %u, add %u, start %u\n",i,to_assign.message.body.datlen, to_assign.message.body.num_of_0, to_assign.message.body.add, to_assign.message.body.start);
	}
	return 1;
}
int handle_found(uint8_t **treasure, int *number_of_0, int num_miners, struct fd_pair *client_fds, char **miner_names, int *treasure_len, int i, uint8_t *treasure_md5)
{
	int num_of_0 = *number_of_0, j;
	result_header res_from_miner;
	memset(&res_from_miner, 0, sizeof(res_from_miner));
	complete_read_header(&res_from_miner, client_fds[i].output_fd, sizeof(res_from_miner) - sizeof(header)); //complete read header
	if(res_from_miner.message.body.num_of_0 == num_of_0) // if miner find correct mine
	{
		fprintf(stderr,"really find\n");
		header need_more;
		memset(&need_more, 0, sizeof(need_more));
		need_more.result = HEADER_SEND;
		
		*treasure = (uint8_t *)realloc(*treasure, res_from_miner.message.body.datlen + *treasure_len + 5);
		write(client_fds[i].input_fd, &need_more, sizeof(need_more));
		read(client_fds[i].output_fd, *treasure + *treasure_len, res_from_miner.message.body.datlen);
		printf("A %d-treasure discovered! %s\n",num_of_0, res_from_miner.message.body.md5);
		
		
		memcpy(treasure_md5, res_from_miner.message.body.md5, MD5_LEN);
		other_found_header notice_find; // notice other miner that a mine has been found.
		memset(&notice_find, 0, sizeof(notice_find));
		notice_find.message.hd.result = HEADER_OTHER_FOUND;
		memcpy(notice_find.message.body.md5, res_from_miner.message.body.md5, MD5_LEN);
		notice_find.message.body.namelen = strlen(miner_names[i]);
		notice_find.message.body.num_of_0 = num_of_0;
		for(j=0;j<num_miners;j++)
		{
			if(j!=i)
			{
				write(client_fds[j].input_fd, &notice_find, sizeof(notice_find));
				write(client_fds[j].input_fd, miner_names[i], notice_find.message.body.namelen);
			}
		}
		*number_of_0+=1;
		assign_jobs(*number_of_0, *treasure + *treasure_len , num_miners, client_fds, res_from_miner.message.body.datlen);
		*treasure_len += res_from_miner.message.body.datlen ;
		return 1;
	}
	else
	{
		fprintf(stderr,"not really find\n");
		return 0;
	}
}
int dump_time = 0, max_dump_time = 100, dump_num=0, need_quit = 0;
char **dump_path;

int handle_command(char *cmd, uint8_t *treasure, int num_of_0, int num_miners, struct fd_pair *client_fds, int *treasure_len, uint8_t *treasure_md5)
{
    /* TODO parse user commands here */
      /* command string */;
		fprintf(stderr, "boss handle %s\n",cmd);
    if (strcmp(cmd, "status") == 0)
    {
        /* TODO show status */  
				if(num_of_0 == 1) printf("best 0-treasure in 0 bytes\n");
        else printf("best %d-treasure %s in %d bytes\n", num_of_0-1, treasure_md5, *treasure_len);
        if(boss_has_assign==0) return 1;
        header header_status;
        memset(&header_status, 0, sizeof(header_status));
        header_status.result = HEADER_STATUS;
        int i;
        for(i=0; i<num_miners; i++)
				{
					write(client_fds[i].input_fd, &header_status, sizeof(header_status));
				}
        return 1;
    }
    else if (strcmp(cmd, "dump") == 0)
    {
      /* TODO write best n-treasure to specified file */
      char path[4100]={0};
      scanf("%s",path);
      fprintf(stderr,"dump to %s\n",path);
     /* uint8_t ggg[MD5_LEN]={0};
      makemd5(ggg, treasure, *treasure_len);
      fprintf(stderr,"md5 %s\n",ggg);*/
      /*int wfd = open(path, O_WRONLY|O_TRUNC|O_CREAT, 0644);
      write(wfd, treasure, *treasure_len);*/
      int pathlen=strlen(path), wfd = open(path, O_WRONLY|O_TRUNC|O_CREAT, 0644), ntowrite = *treasure_len, nowwrite,gg=0, now_dump_time = dump_time;
      if(num_of_0 == 1)
			{
				close(wfd);
				return 2;
			}
			dump_time++;
      dump_num++;
      if(dump_time >=max_dump_time)
			{
				max_dump_time*=2;
				dump_path = (char **)realloc(dump_path,sizeof(char *) * max_dump_time);
			}
  		fd_set readset;
  		fd_set writeset;
	    fd_set working_readset;
	    uint8_t *ptr = treasure;
	    struct timeval timeout;
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 5000;
			
			int i,j,write_once = (ntowrite > 1024) ?1024:ntowrite;
	    FD_ZERO(&readset);
	    FD_SET(STDIN_FILENO, &readset);
	    FD_ZERO(&writeset);
	    FD_SET(wfd, &writeset);
	    fprintf(stderr,"dump into while, write %d\n",ntowrite);
	    while(ntowrite > 0 )
	    {	
				memcpy(&working_readset, &writeset, sizeof(readset));
				timeout.tv_sec = 0;
	   		timeout.tv_usec = 500;	  
				//fprintf(stderr,"start select\n");  
	   		select(wfd+1,NULL,&working_readset,NULL,&timeout);
	   	//	fprintf(stderr,"finish select\n");
	    	if(FD_ISSET(wfd, &working_readset) && gg <= 5)
	    	{
					gg++;
	    //		fprintf(stderr,"try to write\n");
					nowwrite = write(wfd, ptr, write_once);
					if(nowwrite>0)
					{
		    		fprintf(stderr, "dump write %d\n",nowwrite);	
		    		ptr+=nowwrite;
		    		ntowrite -= nowwrite;
		    	}
				}
				else
				{
					gg=0;
					//fprintf(stderr,"dump select\n");
					memcpy(&working_readset, &readset, sizeof(readset));
					timeout.tv_sec = 0;
	   			timeout.tv_usec = 500;
					select(STDIN_FILENO+1, &working_readset, NULL, NULL, &timeout);
					if(FD_ISSET(STDIN_FILENO, &working_readset))
					{
						fprintf(stderr,"there's another command during dump\n'");
						char command[10]={0};
						scanf("%s" ,command);
						handle_command(command,treasure, num_of_0, num_miners, client_fds, treasure_len, treasure_md5);
						for(i=now_dump_time+1; i<dump_time;i++)
						{
							if(strcmp(dump_path[i], path) == 0)
							{
								ntowrite = 0;
								break;
							}
						}
					}
				}
			}
			dump_num--; 
			if(dump_num > 0)
			{
				dump_path[now_dump_time] = (char *)malloc(pathlen+2);
				strcpy(dump_path[now_dump_time], path); dump_path[now_dump_time][pathlen] = '\0';
			}
			else
			{
				for(i=1;i<dump_time;i++) free(dump_path[i]);
				dump_time = 0;
			}
			close(wfd);
			return 2;
    }
    else
    {
      assert(strcmp(cmd, "quit")==0);
      /* TODO tell clients to cease their jobs and exit normally */
      need_quit=1;
      return 3;
    }
}

int main(int argc, char **argv)
{
    /* sanity check on arguments */
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s CONFIG_FILE\n", argv[0]);
        exit(1);
    }
		dump_path = (char **)malloc(sizeof(char *) * max_dump_time);
    /* load config file */
    struct server_config config;
    load_config_file(&config, argv[1]);
		fprintf(stderr, "boss : load config success\n");
    /* open the named pipes */
    struct fd_pair client_fds[config.num_miners];

    for (int ind = 0; ind < config.num_miners; ind += 1)
    {
        struct fd_pair *fd_ptr = &client_fds[ind];
        struct pipe_pair *pipe_ptr = &config.pipes[ind];
				fprintf(stderr,"boss:1 now open %s\n", pipe_ptr->input_pipe);
        fd_ptr->input_fd = open(pipe_ptr->input_pipe, O_WRONLY);
        assert (fd_ptr->input_fd >= 0);
				fprintf(stderr,"boss:2 now open %s\n", pipe_ptr->output_pipe);
        fd_ptr->output_fd = open(pipe_ptr->output_pipe, O_RDONLY);
        assert (fd_ptr->output_fd >= 0);
    }
    fprintf(stderr, "boss : open success\n");
		char **miner_names;
		miner_names =(char **) malloc(sizeof(char *));
    /* initialize data for select() */
    int maxfd=0,i,j, treasure_len = 0;
    fd_set readset;
    fd_set working_readset;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;

    FD_ZERO(&readset);
    FD_SET(STDIN_FILENO, &readset);
		int namelen;
    // TODO add input pipes to readset, setup maxfd
		for(i=0; i<config.num_miners; i++)
		{
			if(maxfd < client_fds[i].output_fd) maxfd= client_fds[i].output_fd;
			FD_SET(client_fds[i].output_fd, &readset);
			header start;
			memset(&start, 0, sizeof(start));
			start.result = HEADER_SERVER_START;
			write(client_fds[i].input_fd, &start, sizeof(start));
			read(client_fds[i].output_fd, &namelen, sizeof(namelen));
			miner_names[i] = (char *)malloc(namelen + 1);
			memset(miner_names[i], 0, namelen + 1);
			read(client_fds[i].output_fd, miner_names[i], namelen);
			fprintf(stderr, "told %s start\n",miner_names[i]);
		}
		fprintf(stderr, "boss : start success\n");
		maxfd++;
    /* assign jobs to clients */
		struct stat bin_stat;
		lstat(config.mine_file, &bin_stat);
		uint8_t *treasure = (uint8_t *)malloc( bin_stat.st_size +6), treasure_md5[MD5_LEN]={0};
		memset(treasure, 0, bin_stat.st_size +6);
		char *tmp_ptr=treasure;
		fd_set read_mine;
		int bin_fd=open(config.mine_file, O_RDONLY|O_NONBLOCK), not_find_num=0, num_of_0=1, read_once=(bin_stat.st_size > 4096)?4096:bin_stat.st_size, number_to_read=bin_stat.st_size, has_read;
		while(number_to_read>0) //read mine.bin
		{
			has_read=read(bin_fd, tmp_ptr, read_once);
			if(has_read>0)
			{
				number_to_read-=has_read;
				tmp_ptr+=has_read;
			}
			else
			{
				FD_ZERO(&read_mine);
				FD_SET(STDIN_FILENO,&read_mine);
				timeout.tv_sec = 0;
    		timeout.tv_usec = 500;
    		select(STDIN_FILENO+1,&read_mine,NULL,NULL,&timeout);
    		if(FD_ISSET(STDIN_FILENO, &read_mine))
    		{
    			char command[10] = {0};
          scanf("%s", command);
          handle_command(command, treasure, num_of_0, config.num_miners, client_fds, &treasure_len, treasure_md5);
          if(need_quit == 1)
          {
          	header boss_rest;
          	memset(&boss_rest, 0, sizeof(boss_rest));
          	boss_rest.result = HEADER_QUIT;
          	for(j=0; j<config.num_miners;j++) write(client_fds[j].input_fd, &boss_rest, sizeof(boss_rest));
          	fprintf(stderr,"quit\n");
          	for(j=0; j<config.num_miners;j++)
						{
							close(client_fds[j].input_fd);
							close(client_fds[j].output_fd);
						}
				    /* TODO close file descriptors */
				
				    return 0;
					}
				}
			}
		}
		treasure_len = bin_stat.st_size;
    assign_jobs(num_of_0, treasure, config.num_miners, client_fds, treasure_len);		
    boss_has_assign = 1;
    header res;
    while (need_quit == 0)
    {
        memcpy(&working_readset, &readset, sizeof(readset)); // why we need memcpy() here?
        select(maxfd, &working_readset, NULL, NULL, &timeout);
				timeout.tv_sec = 0;
    		timeout.tv_usec = 5000;
        if (FD_ISSET(STDIN_FILENO, &working_readset))
        {
          /* TODO handle user input here */
          char command[10] = {0};
          scanf("%s", command);
          handle_command(command, treasure, num_of_0, config.num_miners, client_fds, &treasure_len, treasure_md5);
          if(need_quit == 1)
          {
          	header boss_rest;
          	memset(&boss_rest, 0, sizeof(boss_rest));
          	boss_rest.result = HEADER_QUIT;
          	for(j=0; j<config.num_miners;j++) write(client_fds[j].input_fd, &boss_rest, sizeof(boss_rest));
          	fprintf(stderr,"quit\n");
          	break;
					}
        }
        for(i=0;i<config.num_miners;i++)
        {
        	if(FD_ISSET(client_fds[i].output_fd, &working_readset))
        	{
        		memset(&res, 0, sizeof(res));
        		int readnum = read(client_fds[i].output_fd, &res, sizeof(res));
        		fprintf(stderr, "message from %d\n",i);
        		if(readnum>0)
        		{
        			switch(res.result)
        			{
        				case HEADER_TREASURE_FOUND:
        					handle_found(&treasure, &num_of_0, config.num_miners, client_fds, miner_names, &treasure_len, i, treasure_md5);
        					
        						/*uint8_t ujkj[MD5_LEN]={0};
        						makemd5(ujkj, treasure, treasure_len);
        						fprintf(stderr, "change md5  %s\n",ujkj);*/
									
									break;
							}
						}
					}
				}
        /* TODO check if any client send me some message
           you may re-assign new jobs to clients
        if (FD_ISSET(...))
        {
           ...
        }
        */
    }
		for(j=0; j<config.num_miners;j++)
		{
			close(client_fds[j].input_fd);
			close(client_fds[j].output_fd);
		}
    /* TODO close file descriptors */

    return 0;
}
