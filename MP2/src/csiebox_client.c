#include "csiebox_client.h"

#include "csiebox_common.h"
#include "connect.h"
#include <linux/inotify.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

static int parse_arg(csiebox_client* client, int argc, char** argv);
static int login(csiebox_client* client);

//read config file, and connect to server
void csiebox_client_init(
  csiebox_client** client, int argc, char** argv) {
  csiebox_client* tmp = (csiebox_client*)malloc(sizeof(csiebox_client));
  if (!tmp) {
    fprintf(stderr, "client malloc fail\n");
    return;
  }
  memset(tmp, 0, sizeof(csiebox_client));
  if (!parse_arg(tmp, argc, argv)) {
    fprintf(stderr, "Usage: %s [config file]\n", argv[0]);
    free(tmp);
    return;
  }
  int fd = client_start(tmp->arg.name, tmp->arg.server);
  if (fd < 0) {
    fprintf(stderr, "connect fail\n");
    free(tmp);
    return;
  }
  tmp->conn_fd = fd;
  *client = tmp;
}

//show how to use csiebox_protocol_meta header
//other headers is similar usage
//please check out include/common.h
//using .gitignore for example only for convenience  
int sampleFunction(csiebox_client* client){
  csiebox_protocol_meta req;
  memset(&req, 0, sizeof(req));
  req.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
  req.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
  req.message.header.req.datalen = sizeof(req) - sizeof(req.message.header);
  char path[] = "just/a/testing/path";
  req.message.body.pathlen = strlen(path);

  //just show how to use these function
  //Since there is no file at "just/a/testing/path"
  //I use ".gitignore" to replace with
  //In fact, it should be 
  //lstat(path, &req.message.body.stat);
  //md5_file(path, req.message.body.hash);
  lstat(".gitignore", &req.message.body.stat);
  md5_file(".gitignore", req.message.body.hash);


  //send pathlen to server so that server can know how many charachers it should receive
  //Please go to check the samplefunction in server
  if (!send_message(client->conn_fd, &req, sizeof(req))) {
    fprintf(stderr, "send fail\n");
    return 0;
  }

  //send path
  send_message(client->conn_fd, path, strlen(path));

  //receive csiebox_protocol_header from server
  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  if (recv_message(client->conn_fd, &header, sizeof(header))) {
    if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
        header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
        header.res.status == CSIEBOX_PROTOCOL_STATUS_OK) {
      printf("Receive OK from server\n");
      return 1;
    } else {
      return 0;
    }
  }
  return 0;
}
static __ino_t *in_table;
static int ujkl=0, in_num=0, longest_pathlen=-1, *wd, inotify_num=0,fd;
static char **pt_table, *longest_path, **inotify_path;

void trav_dir(char *normal_path, csiebox_client* client, int layer)
{
	char el_path[410]={0},now_path[410]={0};
	if(strlen(normal_path) > 0)sprintf(now_path,"%s/%s", client->arg.path, normal_path);	
	else strcpy(now_path, client->arg.path);
	DIR *dirp;
	dirp=opendir(now_path);
	if(dirp==NULL) return;
	struct dirent *dp;
	dp=(struct dirent *)malloc(sizeof(struct dirent));
	fprintf(stderr, "now travel DIR -> %s\n",now_path);
	while((dp=readdir(dirp))!=NULL)
	{
		if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0) continue;		
		csiebox_protocol_meta req;
  	memset(&req, 0, sizeof(req));
  	
 		req.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
 		req.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
	  req.message.header.req.datalen = sizeof(req) - sizeof(req.message.header);
	  
	  
		sprintf(el_path,"%s/%s",now_path,dp->d_name);
		lstat(el_path, &req.message.body.stat);
		char next_path[410]={0};
    if(strlen(normal_path)>0) sprintf(next_path, "%s/%s", normal_path, dp->d_name);
    else strcpy(next_path, dp->d_name);
    req.message.body.pathlen=strlen(next_path);
    if(longest_pathlen < layer + 1)
    {
    	longest_pathlen = layer + 1;
			strcpy(longest_path, next_path); 
		}
		if(S_ISDIR(req.message.body.stat.st_mode))
		{
			fprintf(stderr, "now deal with -> %s is a DIR\n", next_path);
		  if (!send_message(client->conn_fd, &req, sizeof(req))) {
  			fprintf(stderr, "send fail\n");
  			return ;
			}
			send_message(client->conn_fd, next_path, strlen(next_path));
			wd[inotify_num] = inotify_add_watch(fd, el_path, IN_CREATE | IN_DELETE | IN_ATTRIB | IN_MODIFY);
			inotify_path[inotify_num] = (char *)malloc(strlen(next_path)+9);
			strcpy(inotify_path[inotify_num], next_path);
			fprintf(stderr, "path = %s, wd = %d",next_path, wd[inotify_num]);
			inotify_num++;		
			
			csiebox_protocol_header header;
			memset(&header, 0, sizeof(header));
			if (recv_message(client->conn_fd, &header, sizeof(header))) 
			{
				if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
	      header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
	      header.res.status == CSIEBOX_PROTOCOL_STATUS_MORE) 
				{
					fprintf(stderr, "recv MORE\n");
		    	csiebox_protocol_file file;
					memset(&file, 0, sizeof(file));
					file.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
 					file.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
	  			file.message.header.req.datalen = sizeof(file) - sizeof(file.message.header);
	  			file.message.body.datalen=0;
	  			if(!send_message(client->conn_fd, &file, sizeof(file))){
	  				fprintf(stderr, "send fail\n");
  					return ;
					}
					csiebox_protocol_header res;
					memset(&res, 0, sizeof(res));
					recv_message(client->conn_fd, &res, sizeof(res));
					if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
							res.res.op == CSIEBOX_PROTOCOL_OP_SYNC_FILE &&
							res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
					{
						fprintf(stderr, "sync file success\n");
					}		
	    	}
    	}
			
    	trav_dir(next_path, client, layer+1);
		}
		
		else if(S_ISLNK(req.message.body.stat.st_mode))
		{			
			fprintf(stderr, "now deal with -> %s is LNK\n",next_path);
    	pt_table[in_num]=(char *)malloc(strlen(next_path)+2);
    	strcpy(pt_table[in_num],next_path);
			in_table[in_num]=req.message.body.stat.st_ino;
			in_num++;
			char lnk_content[402]={0};
			readlink(el_path, lnk_content, 402);
			MD5_CTX ctx;
			MD5Init(&ctx);
			MD5Update(&ctx, (const uint8_t*)lnk_content, strlen(lnk_content));
			MD5Final(req.message.body.hash, &ctx);
			
			if (!send_message(client->conn_fd, &req, sizeof(req))) {
  			fprintf(stderr, "send fail\n");
  			return ;
			}
			send_message(client->conn_fd, next_path, strlen(next_path));
			
			csiebox_protocol_header header;
			memset(&header, 0, sizeof(header));
			if (recv_message(client->conn_fd, &header, sizeof(header))) 
			{
				if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
	      header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
	      header.res.status == CSIEBOX_PROTOCOL_STATUS_MORE) 
				{
		    	csiebox_protocol_file file;
					memset(&file, 0, sizeof(file));
					file.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
 					file.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
	  			file.message.header.req.datalen = sizeof(file) - sizeof(file.message.header);
	  			file.message.body.datalen=strlen(lnk_content);
	  			if(!send_message(client->conn_fd, &file, sizeof(file))){
	  				fprintf(stderr, "send fail\n");
  					return ;
					}
					send_message(client->conn_fd, lnk_content, strlen(lnk_content));
					csiebox_protocol_header res;
					memset(&res, 0, sizeof(res));
					recv_message(client->conn_fd, &res, sizeof(res));
					if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
							res.res.op == CSIEBOX_PROTOCOL_OP_SYNC_FILE &&
							res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
					{
						fprintf(stderr, "sync file success\n");
					}	
	    	}	
    	}
		}
		else
		{
    	int judhardln=0;
			fprintf(stderr, "now deal with -> %s is REG\n",next_path);
			if(req.message.body.stat.st_nlink>1)
			{
				for(int i=0;i<in_num;i++)
				{
					if(req.message.body.stat.st_ino == in_table[i])
					{
						judhardln=1;
						csiebox_protocol_hardlink hkreq;
						memset(&hkreq, 0, sizeof(hkreq));
						hkreq.message.header.req.magic =  CSIEBOX_PROTOCOL_MAGIC_REQ;
						hkreq.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK;
						hkreq.message.header.req.datalen = sizeof(hkreq) - sizeof(hkreq.message.header);
						hkreq.message.body.srclen = strlen(next_path);
						hkreq.message.body.targetlen = strlen(pt_table[i]);
						if(!send_message(client->conn_fd, &hkreq, sizeof(hkreq))){
	  					fprintf(stderr, "send fail\n");
  						return ;
						}
						send_message(client->conn_fd, next_path, strlen(next_path));
						send_message(client->conn_fd, pt_table[i], strlen(pt_table[i]));
						csiebox_protocol_header res;
						memset(&res, 0, sizeof(res));
						recv_message(client->conn_fd, &res, sizeof(res));
						if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
								res.res.op == CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK &&
								res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
						{
							fprintf(stderr, "sync hardlink success\n");
						}	
						break;
					}
				}
			}
			if(judhardln==0)
			{
	    	pt_table[in_num]=(char *)malloc(strlen(next_path)+2);
	    	strcpy(pt_table[in_num],next_path);
				in_table[in_num]=req.message.body.stat.st_ino;
				in_num++;
				char file_content[4096]={0};
				FILE *fp;
				fp=fopen(el_path, "rb");
				if(fp==NULL)
				{
					fprintf(stderr,"open fail\n");
					return ;
				}
				fread(file_content, 1, 4096, fp);
				
				MD5_CTX ctx;
				MD5Init(&ctx);
				MD5Update(&ctx, (const uint8_t*)file_content, strlen(file_content));
				MD5Final(req.message.body.hash, &ctx);
				fprintf(stderr, "file content = %s ,file_len %d\n",file_content, strlen(file_content));
				if (!send_message(client->conn_fd, &req, sizeof(req))) {
	  			fprintf(stderr, "send fail\n");
	  			return ;
				}
				send_message(client->conn_fd, next_path, strlen(next_path));
				csiebox_protocol_header header;
				if (recv_message(client->conn_fd, &header, sizeof(header))) 
				{
					if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
		      header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
		      header.res.status == CSIEBOX_PROTOCOL_STATUS_MORE) 
					{
			    	csiebox_protocol_file file;
						memset(&file, 0, sizeof(file));
						file.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
	 					file.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
		  			file.message.header.req.datalen = sizeof(file) - sizeof(file.message.header);
		  			file.message.body.datalen=strlen(file_content);
		  			if(!send_message(client->conn_fd, &file, sizeof(file))){
		  				fprintf(stderr, "send fail\n");
	  					return ;
						}
						send_message(client->conn_fd, file_content, strlen(file_content));
						csiebox_protocol_header res;
						memset(&res, 0, sizeof(res));
						recv_message(client->conn_fd, &res, sizeof(res));
						if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
								res.res.op == CSIEBOX_PROTOCOL_OP_SYNC_FILE &&
								res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
						{
							fprintf(stderr, "sync file success\n");
						}	
		    	}
	    	}
			}
		}
	}		
}
//this is where client sends request, you sould write your code here
int csiebox_client_run(csiebox_client* client) {
  if (!login(client)) {
    fprintf(stderr, "login fail\n");
    return 0;
  }
  fprintf(stderr, "login success\n");
  
	
  //This is a sample function showing how to send data using defined header in common.h
  //You can remove it after you understand
  //sampleFunction(client);
  
  //====================
  //        TODO
  //====================
  static int initial=0;
  if(ujkl==0)
  {
		in_table=(__ino_t *)malloc(302); 
		pt_table=(char **)malloc(302*sizeof(char*));
		longest_path=(char *)malloc(302);
		wd=(int *)malloc(302*sizeof(int));
		inotify_path = (char **)malloc(302*sizeof(char *));
		fd = inotify_init();
		ujkl++;
  }
  if(initial==0)
  {
  	initial++;
  	trav_dir("", client, 0);
  	char tmp[320]={0};
  	strcpy(tmp, "cdir/");
  	strcat(tmp, longest_path);
  	strcpy(longest_path, tmp);
  	FILE *fp;
  	fp = fopen("../cdir/longestPath.txt", "wb");
  	fwrite(longest_path, 1, strlen(longest_path), fp);
  	fclose(fp);

  	csiebox_protocol_meta meta;
  	memset(&meta, 0, sizeof(meta));
  	meta.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
  	meta.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
  	meta.message.header.req.datalen = sizeof(meta) - sizeof(meta.message.header);
		lstat("../cdir/longestPath.txt", &meta.message.body.stat);
		meta.message.body.pathlen = strlen("longestPath.txt");
		if (!send_message(client->conn_fd, &meta, sizeof(meta))) {
  			fprintf(stderr, "send fail\n");
  			return 0;
		}
		send_message(client->conn_fd, "longestPath.txt", strlen("longestPath.txt"));
		csiebox_protocol_header header;
		memset(&header, 0, sizeof(header));
		if (recv_message(client->conn_fd, &header, sizeof(header))) 
		{
			if(header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
		      header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
		      header.res.status == CSIEBOX_PROTOCOL_STATUS_MORE)
		  {
		  	csiebox_protocol_file file;
		  	file.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
		  	file.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
		  	file.message.header.req.datalen = sizeof(file) - sizeof(file.message.header);
		  	file.message.body.datalen = strlen(longest_path);
		  	if(!send_message(client->conn_fd, &file, sizeof(file)))
		  	{
		  		fprintf(stderr,"send fail\n");
		  		return 0;
				}
				send_message(client->conn_fd, longest_path, strlen(longest_path));
		  	csiebox_protocol_header res;
				memset(&res, 0, sizeof(res));
				recv_message(client->conn_fd, &res, sizeof(res));
				if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
						res.res.op == CSIEBOX_PROTOCOL_OP_SYNC_FILE &&
						res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
				{
					fprintf(stderr, "sync file success\n");
				}	
			}
		}
	}
	wd[inotify_num] = inotify_add_watch(fd, "../cdir", IN_CREATE | IN_DELETE | IN_ATTRIB | IN_MODIFY);
	inotify_path[inotify_num] = (char *)malloc(9);
	memset(inotify_path[inotify_num], 0, 9);
	strcpy(inotify_path[inotify_num], "");
	inotify_num++;
	char buffer[EVENT_BUF_LEN];
	memset(buffer, 0, EVENT_BUF_LEN);
	int length;
  while ((length = read(fd, buffer, EVENT_BUF_LEN)) > 0) {
    int i = 0;
    while (i < length) {
      struct inotify_event* event = (struct inotify_event*)&buffer[i];
      printf("event: (%d, %d, %s)\ntype: ", event->wd, strlen(event->name), event->name);
			int wd2index,judwd2index=0;
	  	if(event->wd == wd[event->wd-1])
			{
				wd2index = event->wd-1;
				judwd2index = 1;
			}
		  else
			{
				for(int i=0; i<inotify_num; i++)
				{
					if(wd[inotify_num] == event->wd)
					{
						wd2index=i;
						judwd2index = 1;
						break;
					}	
			  }
			}
			if(judwd2index == 0)
			{
				i += EVENT_SIZE + event->len;
				continue;
			}
      if (event->mask & IN_CREATE || event->mask & IN_MODIFY) 
			{
				csiebox_protocol_meta meta;
		  	memset(&meta, 0, sizeof(meta));
		  	meta.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
		  	meta.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
		  	meta.message.header.req.datalen = sizeof(meta) - sizeof(meta.message.header);
		  	char now_path[340]={0}, send_path[340]={0};
		  	
				sprintf(now_path, "%s/%s/%s", "../cdir", inotify_path[wd2index], event->name);
		  	if(strlen(inotify_path[event->wd-1]) > 0) sprintf(send_path, "%s/%s", inotify_path[wd2index], event->name);
		  	else sprintf(send_path, "%s", event->name);
				meta.message.body.pathlen = strlen(send_path);
				if(lstat(now_path, &meta.message.body.stat)==-1)
				{
					i += EVENT_SIZE + event->len; continue;
				}
				if(S_ISDIR(meta.message.body.stat.st_mode))
				{
					wd[inotify_num] = inotify_add_watch(fd, now_path, IN_CREATE | IN_DELETE | IN_ATTRIB | IN_MODIFY);
					inotify_path[inotify_num] = (char *)malloc(strlen(send_path)+5);
					strcpy(inotify_path[inotify_num], send_path);
					inotify_num++;
					if(send_message(client->conn_fd, &meta, sizeof(meta)))
					{
						send_message(client->conn_fd, send_path, strlen(send_path));
						csiebox_protocol_header header;
						memset(&header, 0, sizeof(header));
						if (recv_message(client->conn_fd, &header, sizeof(header))) 
						{
							if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
	     				header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
	      			header.res.status == CSIEBOX_PROTOCOL_STATUS_MORE) 
							{
					    	csiebox_protocol_file file;
								memset(&file, 0, sizeof(file));
								file.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
			 					file.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
				  			file.message.header.req.datalen = sizeof(file) - sizeof(file.message.header);
				  			file.message.body.datalen=0;
				  			if(!send_message(client->conn_fd, &file, sizeof(file))){
				  				fprintf(stderr, "send fail\n");
								}
								csiebox_protocol_header res;
								memset(&res, 0, sizeof(res));
								recv_message(client->conn_fd, &res, sizeof(res));
								if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
										res.res.op == CSIEBOX_PROTOCOL_OP_SYNC_FILE &&
										res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
								{
									fprintf(stderr, "sync file success\n");
								}	
	    				}
    				}
					}
				}
				else if(S_ISLNK(meta.message.body.stat.st_mode))
				{
					char lnk_content[402]={0};
					if(readlink(now_path, lnk_content, 402)>=0)
					{
						MD5_CTX ctx;
						MD5Init(&ctx);
						MD5Update(&ctx, (const uint8_t*)lnk_content, strlen(lnk_content));
						MD5Final(meta.message.body.hash, &ctx);
						if(send_message(client->conn_fd, &meta, sizeof(meta)))
						{
							send_message(client->conn_fd, send_path, strlen(send_path));
							csiebox_protocol_header header;
							memset(&header, 0, sizeof(header));
							if (recv_message(client->conn_fd, &header, sizeof(header))) 
							{
								if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
		     				header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
		      			header.res.status == CSIEBOX_PROTOCOL_STATUS_MORE) 
								{
						    	csiebox_protocol_file file;
									memset(&file, 0, sizeof(file));
									file.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
				 					file.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
					  			file.message.header.req.datalen = sizeof(file) - sizeof(file.message.header);
					  			file.message.body.datalen=strlen(lnk_content);
					  			if(!send_message(client->conn_fd, &file, sizeof(file))){
					  				fprintf(stderr, "send fail\n");
									}
									send_message(client->conn_fd, lnk_content, strlen(lnk_content));
									csiebox_protocol_header res;
									memset(&res, 0, sizeof(res));
									recv_message(client->conn_fd, &res, sizeof(res));
									if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
											res.res.op == CSIEBOX_PROTOCOL_OP_SYNC_FILE &&
											res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
									{
										fprintf(stderr, "sync file success\n");
									}	
		    				}
	    				}
	    			}
					}
				}
				else
				{
					char file_content[4096]={0};
					FILE *fp;
					fp=fopen(now_path, "rb");
					if(fp==NULL)
					{
						fprintf(stderr,"open fail\n");
						i += EVENT_SIZE + event->len; continue;
					}
					fread(file_content, 1, 4096, fp);			
					MD5_CTX ctx;
					MD5Init(&ctx);
					MD5Update(&ctx, (const uint8_t*)file_content, strlen(file_content));
					MD5Final(meta.message.body.hash, &ctx);
					fprintf(stderr, "file content = %s ,file_len %d\n",file_content, strlen(file_content));
					if (!send_message(client->conn_fd, &meta, sizeof(meta))) {
		  			fprintf(stderr, "send fail\n");
					}
					send_message(client->conn_fd, send_path, strlen(send_path));
					csiebox_protocol_header header;
					if (recv_message(client->conn_fd, &header, sizeof(header))) 
					{
						if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
			      header.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
			      header.res.status == CSIEBOX_PROTOCOL_STATUS_MORE) 
						{
				    	csiebox_protocol_file file;
							memset(&file, 0, sizeof(file));
							file.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
		 					file.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
			  			file.message.header.req.datalen = sizeof(file) - sizeof(file.message.header);
			  			file.message.body.datalen=strlen(file_content);
			  			if(!send_message(client->conn_fd, &file, sizeof(file))){
			  				fprintf(stderr, "send fail\n");
							}
							send_message(client->conn_fd, file_content, strlen(file_content));
							csiebox_protocol_header res;
							memset(&res, 0, sizeof(res));
							recv_message(client->conn_fd, &res, sizeof(res));
							if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
									res.res.op == CSIEBOX_PROTOCOL_OP_SYNC_FILE &&
									res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
							{
								fprintf(stderr, "sync file success\n");
							}	
			    	}
		    	}				
				}
      }
      if (event->mask & IN_DELETE) 
			{
				char send_path[340]={0};
				if(strlen(inotify_path[wd2index])>0)sprintf(send_path, "%s/%s", inotify_path[wd2index], event->name);	
				else strcpy(send_path, event->name);
				csiebox_protocol_rm rm;
				memset(&rm, 0, sizeof(rm));
				rm.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
				rm.message.header.req.op = CSIEBOX_PROTOCOL_OP_RM;
				rm.message.header.req.datalen = sizeof(rm) - sizeof(rm.message.header);
				rm.message.body.pathlen = strlen(send_path);
				send_message(client->conn_fd, &rm, sizeof(rm));
				send_message(client->conn_fd, send_path, strlen(send_path));
				csiebox_protocol_header res;
				memset(&res, 0, sizeof(res));
				recv_message(client->conn_fd, &res, sizeof(res));
				if(res.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
						res.res.op == CSIEBOX_PROTOCOL_OP_RM &&
						res.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
				{
					fprintf(stderr, "rm success\n");
				}
					
      }
      if (event->mask & IN_ATTRIB) 
			{
        csiebox_protocol_meta meta;
		  	memset(&meta, 0, sizeof(meta));
		  	meta.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
		  	meta.message.header.req.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
		  	meta.message.header.req.datalen = sizeof(meta) - sizeof(meta.message.header);
		  	char now_path[340]={0}, send_path[340]={0};
		  	sprintf(now_path, "%s/%s/%s","../cdir",inotify_path[wd2index], event->name);
		  	if(strlen(inotify_path[wd2index]) > 0) sprintf(send_path, "%s/%s", inotify_path[wd2index], event->name);	
		  	else sprintf(send_path, "%s", event->name);
				meta.message.body.pathlen = strlen(send_path);
				lstat(now_path, &meta.message.body.stat);
				if(S_ISREG(meta.message.body.stat.st_mode))
				{
					char file_content[4096]={0};
					FILE *fp;
					fp=fopen(now_path, "rb");
					if(fp==NULL)
					{
						fprintf(stderr,"open fail\n");
						i += EVENT_SIZE + event->len; continue;
					}
					fread(file_content, 1, 4096, fp);			
					MD5_CTX ctx;
					MD5Init(&ctx);
					MD5Update(&ctx, (const uint8_t*)file_content, strlen(file_content));
					MD5Final(meta.message.body.hash, &ctx);
					fprintf(stderr, "file content = %s ,file_len %d\n",file_content, strlen(file_content));
				}
				send_message(client->conn_fd, &meta, sizeof(meta));
				send_message(client->conn_fd, send_path, strlen(send_path));
				csiebox_protocol_header fay;
				memset(&fay,0,sizeof(fay));
				recv_message(client->conn_fd, &fay, sizeof(fay));
				if(fay.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES&&
						fay.res.op == CSIEBOX_PROTOCOL_OP_SYNC_META &&
						fay.res.status ==  CSIEBOX_PROTOCOL_STATUS_OK)
				{
					fprintf(stderr, "ch success\n");
				}
      }
      i += EVENT_SIZE + event->len;
      
    }
    memset(buffer, 0, EVENT_BUF_LEN);
  }
  
  return 1;
}

void csiebox_client_destroy(csiebox_client** client) {
  csiebox_client* tmp = *client;
  *client = 0;
  if (!tmp) {
    return;
  }
  close(tmp->conn_fd);
  free(tmp);
}

//read config file
static int parse_arg(csiebox_client* client, int argc, char** argv) {
  if (argc != 2) {
    return 0;
  }
  FILE* file = fopen(argv[1], "r");
  if (!file) {
    return 0;
  }
  fprintf(stderr, "reading config...\n");
  size_t keysize = 20, valsize = 20;
  char* key = (char*)malloc(sizeof(char) * keysize);
  char* val = (char*)malloc(sizeof(char) * valsize);
  ssize_t keylen, vallen;
  int accept_config_total = 5;
  int accept_config[5] = {0, 0, 0, 0, 0};
  while ((keylen = getdelim(&key, &keysize, '=', file) - 1) > 0) {
    key[keylen] = '\0';
    vallen = getline(&val, &valsize, file) - 1;
    val[vallen] = '\0';
    fprintf(stderr, "config (%d, %s)=(%d, %s)\n", keylen, key, vallen, val);
    if (strcmp("name", key) == 0) {
      if (vallen <= sizeof(client->arg.name)) {
        strncpy(client->arg.name, val, vallen);
        accept_config[0] = 1;
      }
    } else if (strcmp("server", key) == 0) {
      if (vallen <= sizeof(client->arg.server)) {
        strncpy(client->arg.server, val, vallen);
        accept_config[1] = 1;
      }
    } else if (strcmp("user", key) == 0) {
      if (vallen <= sizeof(client->arg.user)) {
        strncpy(client->arg.user, val, vallen);
        accept_config[2] = 1;
      }
    } else if (strcmp("passwd", key) == 0) {
      if (vallen <= sizeof(client->arg.passwd)) {
        strncpy(client->arg.passwd, val, vallen);
        accept_config[3] = 1;
      }
    } else if (strcmp("path", key) == 0) {
      if (vallen <= sizeof(client->arg.path)) {
        strncpy(client->arg.path, val, vallen);
        accept_config[4] = 1;
      }
    }
  }
  free(key);
  free(val);
  fclose(file);
  int i, test = 1;
  for (i = 0; i < accept_config_total; ++i) {
    test = test & accept_config[i];
  }
  if (!test) {
    fprintf(stderr, "config error\n");
    return 0;
  }
  return 1;
}

static int login(csiebox_client* client) {
  csiebox_protocol_login req;
  memset(&req, 0, sizeof(req));
  req.message.header.req.magic = CSIEBOX_PROTOCOL_MAGIC_REQ;
  req.message.header.req.op = CSIEBOX_PROTOCOL_OP_LOGIN;
  req.message.header.req.datalen = sizeof(req) - sizeof(req.message.header);
  memcpy(req.message.body.user, client->arg.user, strlen(client->arg.user));
  md5(client->arg.passwd,
      strlen(client->arg.passwd),
      req.message.body.passwd_hash);
  if (!send_message(client->conn_fd, &req, sizeof(req))) {
    fprintf(stderr, "send fail\n");
    return 0;
  }
  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  if (recv_message(client->conn_fd, &header, sizeof(header))) {
    if (header.res.magic == CSIEBOX_PROTOCOL_MAGIC_RES &&
        header.res.op == CSIEBOX_PROTOCOL_OP_LOGIN &&
        header.res.status == CSIEBOX_PROTOCOL_STATUS_OK) {
      client->client_id = header.res.client_id;
      return 1;
    } else {
      return 0;
    }
  }
  return 0;
}
