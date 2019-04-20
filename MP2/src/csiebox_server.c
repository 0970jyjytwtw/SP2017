#include "csiebox_server.h"

#include "csiebox_common.h"
#include "connect.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
static int parse_arg(csiebox_server* server, int argc, char** argv);
static void handle_request(csiebox_server* server, int conn_fd);
static int get_account_info(
  csiebox_server* server,  const char* user, csiebox_account_info* info);
static void login(
  csiebox_server* server, int conn_fd, csiebox_protocol_login* login);
static void logout(csiebox_server* server, int conn_fd);
static char* get_user_homedir(
  csiebox_server* server, csiebox_client_info* info);

#define DIR_S_FLAG (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)//permission you can use to create new file
#define REG_S_FLAG (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)//permission you can use to create new directory

//read config file, and start to listen
void csiebox_server_init(
  csiebox_server** server, int argc, char** argv) {
  csiebox_server* tmp = (csiebox_server*)malloc(sizeof(csiebox_server));
  if (!tmp) {
    fprintf(stderr, "server malloc fail\n");
    return;
  }
  memset(tmp, 0, sizeof(csiebox_server));
  if (!parse_arg(tmp, argc, argv)) {
    fprintf(stderr, "Usage: %s [config file]\n", argv[0]);
    free(tmp);
    return;
  }
  int fd = server_start();
  if (fd < 0) {
    fprintf(stderr, "server fail\n");
    free(tmp);
    return;
  }
  tmp->client = (csiebox_client_info**)
      malloc(sizeof(csiebox_client_info*) * getdtablesize());
  if (!tmp->client) {
    fprintf(stderr, "client list malloc fail\n");
    close(fd);
    free(tmp);
    return;
  }
  memset(tmp->client, 0, sizeof(csiebox_client_info*) * getdtablesize());
  tmp->listen_fd = fd;
  *server = tmp;
}

//wait client to connect and handle requests from connected socket fd
int csiebox_server_run(csiebox_server* server) {
  int conn_fd, conn_len;
  struct sockaddr_in addr;
  while (1) {
    memset(&addr, 0, sizeof(addr));
    conn_len = 0;
    // waiting client connect
    conn_fd = accept(
      server->listen_fd, (struct sockaddr*)&addr, (socklen_t*)&conn_len);
    if (conn_fd < 0) {
      if (errno == ENFILE) {
          fprintf(stderr, "out of file descriptor table\n");
          continue;
        } else if (errno == EAGAIN || errno == EINTR) {
          continue;
        } else {
          fprintf(stderr, "accept err\n");
          fprintf(stderr, "code: %s\n", strerror(errno));
          break;
        }
    }
    // handle request from connected socket fd
    handle_request(server, conn_fd);
  }
  return 1;
}

void csiebox_server_destroy(csiebox_server** server) {
  csiebox_server* tmp = *server;
  *server = 0;
  if (!tmp) {
    return;
  }
  close(tmp->listen_fd);
  int i = getdtablesize() - 1;
  for (; i >= 0; --i) {
    if (tmp->client[i]) {
      free(tmp->client[i]);
    }
  }
  free(tmp->client);
  free(tmp);
}

//read config file
static int parse_arg(csiebox_server* server, int argc, char** argv) {
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
  int accept_config_total = 2;
  int accept_config[2] = {0, 0};
  while ((keylen = getdelim(&key, &keysize, '=', file) - 1) > 0) {
    key[keylen] = '\0';
    vallen = getline(&val, &valsize, file) - 1;
    val[vallen] = '\0';
    fprintf(stderr, "config (%d, %s)=(%d, %s)\n", keylen, key, vallen, val);
    if (strcmp("path", key) == 0) {
      if (vallen <= sizeof(server->arg.path)) {
        strncpy(server->arg.path, val, vallen);
        accept_config[0] = 1;
      }
    } else if (strcmp("account_path", key) == 0) {
      if (vallen <= sizeof(server->arg.account_path)) {
        strncpy(server->arg.account_path, val, vallen);
        accept_config[1] = 1;
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

//It is a sample function
//you may remove it after understanding
int sampleFunction(int conn_fd, csiebox_protocol_meta* meta) {
  
  printf("In sampleFunction:\n");
  uint8_t hash[MD5_DIGEST_LENGTH];
  memset(&hash, 0, sizeof(hash));
  md5_file(".gitignore", hash);
  printf("pathlen: %d\n", meta->message.body.pathlen);
  if (memcmp(hash, meta->message.body.hash, sizeof(hash)) == 0) {
    printf("hashes are equal!\n");
  }

  //use the pathlen from client to recv path 
  char buf[400];
  memset(buf, 0, sizeof(buf));
  recv_message(conn_fd, buf, meta->message.body.pathlen);
  printf("This is the path from client:%s\n", buf);

  //send OK to client
  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  header.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
  header.res.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
  header.res.datalen = 0;
  header.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
  if(!send_message(conn_fd, &header, sizeof(header))){
    fprintf(stderr, "send fail\n");
    return 0;
  }

  return 1;
}
void remove_dir(char *path)
{
	DIR *dirp;
	dirp=opendir(path);
	if(dirp==NULL) return;
	char el_path[410];
	memset(el_path,0,410);
	struct dirent *dp;
	dp=(struct dirent *)malloc(sizeof(struct dirent));
	while((dp=readdir(dirp))!=NULL)
	{
		if(strcmp(dp->d_name,".")==0 || strcmp(dp->d_name,"..")==0) continue;
		sprintf(el_path,"%s/%s",path,dp->d_name);
		struct stat fst;
		lstat(el_path, &fst);
		if(S_ISDIR(fst.st_mode))
		{
			remove_dir(el_path);
			rmdir(el_path);
		}
		else if(S_ISLNK(fst.st_mode) || S_ISREG(fst.st_mode))
		{
			unlink(el_path);
		}
	}
	free(dp);
	closedir(dirp);
	rmdir(path);
	return ;
}

//this is where the server handle requests, you should write your code here
static void handle_request(csiebox_server* server, int conn_fd) {
  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  while (recv_message(conn_fd, &header, sizeof(header))) {
    if (header.req.magic != CSIEBOX_PROTOCOL_MAGIC_REQ) {
      continue;
    }
    static char *server_path;
    static int p=0;
    static __mode_t file_type;
    static struct timespec *time;
    if(p==0)
    {
    	fprintf(stderr, "malloc server_path\n");
    	server_path=(char *)malloc(410);
    	time=(struct timespec *)malloc(sizeof(struct timespec)*2);
    	p++;
		}
    switch (header.req.op) {
      case CSIEBOX_PROTOCOL_OP_LOGIN:
        fprintf(stderr, "login\n");
        csiebox_protocol_login req;
        if (complete_message_with_header(conn_fd, &header, &req)) {
          login(server, conn_fd, &req);
        }
        break;
      case CSIEBOX_PROTOCOL_OP_SYNC_META:
        fprintf(stderr, "sync meta\n");
        csiebox_protocol_meta meta;
        if (complete_message_with_header(conn_fd, &header, &meta)) {
          
        //This is a sample function showing how to send data using defined header in common.h
        //You can remove it after you understand
      //  sampleFunction(conn_fd, &meta);

          //====================
          //        TODO
          //====================
          time[0].tv_sec=meta.message.body.stat.st_atime;
          time[1].tv_sec=meta.message.body.stat.st_mtime;
          time[0].tv_nsec = 60;
          time[1].tv_nsec = 60;
	        char *file_path;
	        csiebox_protocol_header res_header;
	        memset(&res_header, 0, sizeof(res_header));
	        res_header.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
  				res_header.res.op = CSIEBOX_PROTOCOL_OP_SYNC_META;
  				res_header.res.datalen = 0;
  				file_type=meta.message.body.stat.st_mode;
	        file_path = (char *)malloc( sizeof(char) * (meta.message.body.pathlen + 1));
	        memset(file_path, 0, sizeof(char) * (meta.message.body.pathlen + 1));
	        if(!recv_message(conn_fd, file_path, meta.message.body.pathlen))
					{
						res_header.res.status = CSIEBOX_PROTOCOL_STATUS_FAIL;
						fprintf(stderr, "recv fail: %s , %d\n",file_path, meta.message.body.pathlen);
					}
	        else
	        {
	        	uint8_t hash[MD5_DIGEST_LENGTH];
	  				memset(&hash, 0, sizeof(hash));
	  				memset(server_path,0,410);
	  				sprintf(server_path, "%s/%s/%s", server->arg.path, server->client[conn_fd]->account.user, file_path);
						fprintf(stderr ,"server_path -> %s\n", server_path);
						chmod(server_path ,meta.message.body.stat.st_mode);
	  				if(S_ISREG(file_type))
	  				{	
							struct stat server_st;
	  					if(lstat(server_path, &server_st)==-1)
	  					{
	  						fprintf(stderr ,"no such file in server\n");
	  						res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
							}
							else if(!S_ISREG(server_st.st_mode))
							{
								fprintf(stderr ,"file type does not match\n");
								if(S_ISDIR(server_st.st_mode)) remove_dir(server_path);
								else remove(server_path);
								res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
							}
		  				else if(!md5_file(server_path, hash))
		  				{
		  					fprintf(stderr, "md5 fail\n");
		  					res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
							}
		  				else if(memcmp(meta.message.body.hash, hash, MD5_DIGEST_LENGTH)==0)
							{
								fprintf(stderr, "OK\n");
		  					res_header.res.status = CSIEBOX_PROTOCOL_STATUS_OK; 	
								utimensat(AT_FDCWD, server_path, time, AT_SYMLINK_NOFOLLOW);					
							}
							else
							{
								fprintf(stderr, "hash not OK\n");
								res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;			
							}
						}
						else if(S_ISDIR(file_type))
						{
							struct stat server_st;
							fprintf(stderr ,"server_path -> %s\n", server_path);
	  					if(lstat(server_path, &server_st)==-1)
	  					{
	  						fprintf(stderr ,"no such file in server\n");
	  						res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
							}
							else if(!S_ISDIR(server_st.st_mode))
							{
								fprintf(stderr ,"file type does not match\n");
								remove(server_path);
								res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
							}
							else
							{
								DIR *dp;
								dp=opendir(server_path);
								if(dp==NULL)
								{
									fprintf(stderr ,"no such file in server\n");
									res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
								}
								else
								{
									fprintf(stderr ,"OK\n");
									res_header.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
									utimensat(AT_FDCWD, server_path, time, AT_SYMLINK_NOFOLLOW);
								}
								closedir(dp);
							}
						}
						else if(S_ISLNK(file_type))
						{
							struct stat server_st;
	  					if(lstat(server_path, &server_st)==-1)
	  					{
	  						res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
							}
							else if(!S_ISLNK(server_st.st_mode))
							{
								remove(server_path);
								res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
							}
							else 
							{
								char sym_link[402]={0};
								if(readlink(server_path, sym_link,402)>=0)
								{
									MD5_CTX ctx;
								  MD5Init(&ctx);
									MD5Update(&ctx, (const uint8_t*)sym_link, strlen(sym_link));
								  MD5Final(hash, &ctx);
								  if(memcmp(hash, meta.message.body.hash, MD5_DIGEST_LENGTH)==0)
								  {
								  	fprintf(stderr ,"OK\n");
								  	res_header.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
										utimensat(AT_FDCWD, server_path, time, AT_SYMLINK_NOFOLLOW);
									}
									else
									{
										fprintf(stderr ,"md5 fail\n");
										res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
									}
								}
								else
								{
									res_header.res.status = CSIEBOX_PROTOCOL_STATUS_MORE;
									fprintf(stderr ,"no such file in server\n");
								}
							}
						}
						else
						{
							res_header.res.status = CSIEBOX_PROTOCOL_STATUS_FAIL;
							fprintf(stderr, "not symbolic link or dir or regular file\n");
						}
					}
					free(file_path);
					if(!send_message(conn_fd, &res_header, sizeof(res_header))){
    				fprintf(stderr, "send fail\n");
  				}
        }
        else fprintf(stderr, "recv meta fail\n");
        break;
      case CSIEBOX_PROTOCOL_OP_SYNC_FILE:
        fprintf(stderr, "sync file\n");
        csiebox_protocol_file file;
        if (complete_message_with_header(conn_fd, &header, &file)) {
          //====================
          //        TODO
          //====================
          remove(server_path);
        	if(S_ISREG(file_type))
  				{
	  				char *data_content;
	  				data_content=(char *)malloc(file.message.body.datalen+2);
	  				memset(data_content,0,file.message.body.datalen+2);
	  				recv_message(conn_fd, data_content, file.message.body.datalen);
	  				FILE *target;
	  				target=fopen(server_path, "wb");
	  				csiebox_protocol_header resheader;
						memset(&header, 0, sizeof(header));
						resheader.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
						resheader.res.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
						resheader.res.datalen = 0;
	  				if(target == NULL)
	  				{
	  					fprintf(stderr, "open fail\n");  	
							resheader.res.status = CSIEBOX_PROTOCOL_STATUS_FAIL;			
						}
						else
						{
		  				fwrite(data_content, sizeof(char), file.message.body.datalen, target);
		  				resheader.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
		  				fclose(target);
		  			}
	  				free(data_content);
	  				chmod(server_path ,file_type);
	  				utimensat(AT_FDCWD, server_path, time, AT_SYMLINK_NOFOLLOW);
	  				send_message(conn_fd, &resheader, sizeof(resheader));
					}
					else if(S_ISDIR(file_type))
					{					
						mkdir(server_path, DIR_S_FLAG);
						csiebox_protocol_header resheader;
						memset(&header, 0, sizeof(header));
						resheader.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
						resheader.res.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
						resheader.res.datalen = 0;
						resheader.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
						chmod(server_path ,file_type);
						utimensat(AT_FDCWD, server_path, time, AT_SYMLINK_NOFOLLOW);
						send_message(conn_fd, &resheader, sizeof(resheader));
					}
					else if(S_ISLNK(file_type))
					{
						char *data_content;
	  				data_content=(char *)malloc(file.message.body.datalen+2);
	  				memset(data_content,0,file.message.body.datalen+2);
	  				recv_message(conn_fd, data_content, file.message.body.datalen);
	  				symlink(data_content, server_path);
	  				free(data_content);
	  				csiebox_protocol_header resheader;
						memset(&header, 0, sizeof(header));
						resheader.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
						resheader.res.op = CSIEBOX_PROTOCOL_OP_SYNC_FILE;
						resheader.res.datalen = 0;
						resheader.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
						chmod(server_path ,file_type);
						utimensat(AT_FDCWD, server_path, time, AT_SYMLINK_NOFOLLOW);
						send_message(conn_fd, &resheader, sizeof(resheader));
					}
        }
        break;
      case CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK:
        fprintf(stderr, "sync hardlink\n");
        csiebox_protocol_hardlink hardlink;
        if (complete_message_with_header(conn_fd, &header, &hardlink)) {
          //====================
          //        TODO
          //====================
          struct stat st;
          char *tar_path, *src_path, *tmp_path;
        	tmp_path=(char *)malloc(302);
        	tar_path=(char *)malloc(410);
        	src_path=(char *)malloc(410);
        	memset(tmp_path,0,302);
          recv_message(conn_fd, tmp_path, hardlink.message.body.srclen);
          sprintf(src_path,"%s/%s/%s",server->arg.path, server->client[conn_fd]->account.user, tmp_path);
        	csiebox_protocol_header resheader;
					memset(&header, 0, sizeof(header));
					resheader.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
					resheader.res.op = CSIEBOX_PROTOCOL_OP_SYNC_HARDLINK;
					resheader.res.datalen = 0;
					resheader.res.status = CSIEBOX_PROTOCOL_STATUS_FAIL;
          memset(tmp_path,0,302);
          recv_message(conn_fd, tmp_path, hardlink.message.body.targetlen);
          sprintf(tar_path,"%s/%s/%s",server->arg.path, server->client[conn_fd]->account.user, tmp_path);
          free(tmp_path);
          if(lstat(tar_path, &st)==0)
          {
          	link(tar_path,src_path);
          	resheader.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
					}
					else if(lstat(src_path, &st)==0)
					{
						link(src_path,tar_path);
						resheader.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
					}
					send_message(conn_fd, &resheader, sizeof(resheader));
        }
        break;
      case CSIEBOX_PROTOCOL_OP_SYNC_END:
        fprintf(stderr, "sync end\n");
        csiebox_protocol_header end;
          //====================
          //        TODO
          //====================
        break;
      case CSIEBOX_PROTOCOL_OP_RM:
        fprintf(stderr, "rm\n");
        csiebox_protocol_rm rm;
        if (complete_message_with_header(conn_fd, &header, &rm)) {
          //====================
          //        TODO
          //====================
          char *rm_path,*srm_path;
          rm_path=(char *)malloc(rm.message.body.pathlen+1);
          srm_path=(char *)malloc(rm.message.body.pathlen+110);
          memset(rm_path,0,rm.message.body.pathlen+1);
          memset(srm_path,0,rm.message.body.pathlen+110);
          recv_message(conn_fd,rm_path,rm.message.body.pathlen);
          sprintf(srm_path,"%s/%s/%s", server->arg.path, server->client[conn_fd]->account.user, rm_path);
          fprintf(stderr, "rm path in server = %s\n", srm_path);
          free(rm_path);
          struct stat rm_st;
          csiebox_protocol_header resheader;
					memset(&header, 0, sizeof(header));
					resheader.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
					resheader.res.op = CSIEBOX_PROTOCOL_OP_RM;
					resheader.res.datalen = 0;
					resheader.res.status = CSIEBOX_PROTOCOL_STATUS_FAIL;
          if(lstat(srm_path,&rm_st)==0)
          {
	          if(S_ISDIR(rm_st.st_mode))
	          {
	          	remove_dir(srm_path);
	          	resheader.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
						}
						else if(S_ISREG(rm_st.st_mode) || S_ISLNK(rm_st.st_mode))
						{
							unlink(srm_path);
							resheader.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
						}
					}
					free(srm_path);
					send_message(conn_fd, &resheader, sizeof(resheader));
        }
        break;
      default:
        fprintf(stderr, "unknown op %x\n", header.req.op);
        break;
    }
  }
  fprintf(stderr, "end of connection\n");
  logout(server, conn_fd);
}

//open account file to get account information
static int get_account_info(
  csiebox_server* server,  const char* user, csiebox_account_info* info) {
  FILE* file = fopen(server->arg.account_path, "r");
  if (!file) {
    return 0;
  }
  size_t buflen = 100;
  char* buf = (char*)malloc(sizeof(char) * buflen);
  memset(buf, 0, buflen);
  ssize_t len;
  int ret = 0;
  int line = 0;
  while ((len = getline(&buf, &buflen, file) - 1) > 0) {
    ++line;
    buf[len] = '\0';
    char* u = strtok(buf, ",");
    if (!u) {
      fprintf(stderr, "illegal form in account file, line %d\n", line);
      continue;
    }
    if (strcmp(user, u) == 0) {
      memcpy(info->user, user, strlen(user));
      char* passwd = strtok(NULL, ",");
      if (!passwd) {
        fprintf(stderr, "illegal form in account file, line %d\n", line);
        continue;
      }
      md5(passwd, strlen(passwd), info->passwd_hash);
      ret = 1;
      break;
    }
  }
  free(buf);
  fclose(file);
  return ret;
}

//handle the login request from client
static void login(
  csiebox_server* server, int conn_fd, csiebox_protocol_login* login) {
  int succ = 1;
  csiebox_client_info* info =
    (csiebox_client_info*)malloc(sizeof(csiebox_client_info));
  memset(info, 0, sizeof(csiebox_client_info));
  if (!get_account_info(server, login->message.body.user, &(info->account))) {
    fprintf(stderr, "cannot find account\n");
    succ = 0;
  }
  if (succ &&
      memcmp(login->message.body.passwd_hash,
             info->account.passwd_hash,
             MD5_DIGEST_LENGTH) != 0) {
    fprintf(stderr, "passwd miss match\n");
    succ = 0;
  }

  csiebox_protocol_header header;
  memset(&header, 0, sizeof(header));
  header.res.magic = CSIEBOX_PROTOCOL_MAGIC_RES;
  header.res.op = CSIEBOX_PROTOCOL_OP_LOGIN;
  header.res.datalen = 0;
  if (succ) {
    if (server->client[conn_fd]) {
      free(server->client[conn_fd]);
    }
    info->conn_fd = conn_fd;
    server->client[conn_fd] = info;
    header.res.status = CSIEBOX_PROTOCOL_STATUS_OK;
    header.res.client_id = info->conn_fd;
    char* homedir = get_user_homedir(server, info);
    mkdir(homedir, DIR_S_FLAG);
    free(homedir);
  } else {
    header.res.status = CSIEBOX_PROTOCOL_STATUS_FAIL;
    free(info);
  }
  send_message(conn_fd, &header, sizeof(header));
}

static void logout(csiebox_server* server, int conn_fd) {
  free(server->client[conn_fd]);
  server->client[conn_fd] = 0;
  close(conn_fd);
}

static char* get_user_homedir(
  csiebox_server* server, csiebox_client_info* info) {
  char* ret = (char*)malloc(sizeof(char) * PATH_MAX);
  memset(ret, 0, PATH_MAX);
  sprintf(ret, "%s/%s", server->arg.path, info->account.user);
  return ret;
}

