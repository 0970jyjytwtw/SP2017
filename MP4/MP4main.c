#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h> 
#include <errno.h>
#include "cJSON.h"


struct User {
	char name[33];
	unsigned int age;
	char gender[7];
	char introduction[1025];
};

typedef struct  dfsfewgw{
	struct  dfsfewgw *pre,*next;
	int value;
} list;



#define MAX_FD 1025
#define BUFFER_SIZE 6144
char buffer[MAX_FD][BUFFER_SIZE+1], *ptr_to_buffer[MAX_FD], try_match[]="\"try_match\"", send_message[]="\"send_message\"", quit[]="\"quit\"",user_def[]="struct User {char name[33];unsigned int age;char gender[7];char introduction[1025];};";
struct User *user_array[MAX_FD];
int connect_who[MAX_FD]={0}, size=0, in_wait_queue[MAX_FD]={0};
list *front=NULL,*end=NULL;
char *filter_function_array[MAX_FD];

void insert(int value)
{
	if(size==0)
	{
		list *tmp=(list *)malloc(sizeof(list));
		tmp->pre = NULL; tmp->value=value; tmp->next = NULL;
		front = tmp;
		end=tmp;
	}
	else
	{
		list *tmp=(list *)malloc(sizeof(list));
		tmp->pre = end; tmp->value=value; tmp->next = NULL;
		end->next=tmp;
		end=tmp;
	}
	size++;
	return;
}
void del(list *target)
{
	if(target->next== NULL&&target->pre==NULL)
	{
		free(front); front=NULL; end=NULL;
	}
	else if(target->pre== NULL)
	{
		list *tmp=front;
		front=front->next;
		front->pre=NULL;
		free(tmp);
	}
	else if(target->next==NULL)
	{
		end=target->pre;
		target->pre->next=NULL; free(target);
	}
	else
	{
		list *pre=target->pre, *next=target->next;
		free(target); pre->next=next; next->pre=pre;
	}
	size--;
	return;
}
char * string_GetObjectItem(char * name, cJSON *JSON_DATA)
{
	cJSON *tmp;
	tmp=cJSON_GetObjectItem(JSON_DATA, name);
	char *ggg=cJSON_PrintUnformatted(tmp);
	int len=strlen(ggg);
	char *res=(char *)malloc(len+1);
	strcpy(res, ggg);
	res[len]=0;
	return res;
}

void load_data_to_user(struct User *info, cJSON *JSON_DATA)
{
	char *age=string_GetObjectItem("age",JSON_DATA), *gender=string_GetObjectItem("gender",JSON_DATA);
	char *intro=string_GetObjectItem("introduction",JSON_DATA), *name=string_GetObjectItem("name",JSON_DATA);
	fprintf(stderr,"name=%s, age=%s, intro=%s, gender=%s\n",name,age,intro,gender);
	info->age=atoi(age); strncpy(info->gender, gender+1, strlen(gender)-2); strncpy(info->introduction, intro+1, strlen(intro)-2);
	strncpy(info->name, name+1, strlen(name)-2);
	free(age); free(gender); free(intro); free(name);
	fprintf(stderr,"user info name=%s, age=%u, intro=%s, gender=%s\n",info->name,info->age,info->introduction,info->gender);
	return;
}

void get_func_name(char *fn, int fd)
{
	fn[0]='f';fn[1]='f';
	int digit=0;
	fn[2]=fd/1000+'0'; fd%=1000;
	fn[3]=fd/100+'0'; fd%=100;
	fn[4]=fd/10+'0'; fd%=10;
	fn[5]=fd+'0';
	fn[6]=0;

	return;
}
void match(int fd, char *func_name, int from_here)
{
	if(size==0)
	{
		int res_fd=open("res_from_child", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		int res=0; //not match
		write(res_fd, &res, sizeof(res));
		close(res_fd);
		return;
	}
	else
	{
		char func1[10],lib_name[20];
		list *visit=front;
		int vis_fd=front->value, match_fd=-1;
		for(int i=0;i<=from_here;i++)
		{
			visit=visit->next;
		}
		vis_fd=visit->value;
		fprintf(stderr,"fuck\n");
		int res_fd=open("res_from_child", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		for(int i=from_here+1;i<size;i++)
		{
			fprintf(stderr, "try %s(fd=%d)\n", user_array[vis_fd]->name, vis_fd);
			lseek(res_fd, 0, SEEK_SET);
			int res[2]={1,i}; //now match
			write(res_fd, res, sizeof(int)*2);
			int res1=0,res2=0;
			get_func_name(func1, vis_fd);
			sprintf(lib_name, "./%s.so", func1);
			void* handle = dlopen(lib_name, RTLD_LAZY);
	    assert(NULL != handle);
	    dlerror();
	    int (*filter)(struct User) = (int (*)(struct User)) dlsym(handle, "filter_function");
	    const char *dlsym_error = dlerror();
	    if (dlsym_error)
	    {
	        fprintf(stderr, "Cannot load symbol 'multiple': %s\n", dlsym_error);
	        dlclose(handle);
	        return ;
	    }
	    res1=filter(*(user_array[fd]));
	    dlclose(handle);
	    
	    sprintf(lib_name, "./%s.so", func_name);
	    handle = dlopen(lib_name, RTLD_LAZY);
	    assert(NULL != handle);
	    dlerror();
	    int (*filter22)(struct User) = (int (*)(struct User)) dlsym(handle, "filter_function");
	    dlsym_error = dlerror();
	    if (dlsym_error)
	    {
	        fprintf(stderr, "Cannot load symbol 'multiple': %s\n", dlsym_error);
	        dlclose(handle);
	        return ;
	    }
	    res2=filter22(*(user_array[vis_fd]));
	    dlclose(handle);
	    
	    if(res1&&res2)
	    {
	    	match_fd=vis_fd;
	    	break;
			}
			visit=visit->next;
			vis_fd=visit->value;
		}
		if(match_fd!=-1) //match success
		{
			lseek(res_fd, 0, SEEK_SET);
			int res[2]={2,match_fd}; //match success
			write(res_fd, res, sizeof(int)*2);
			close(res_fd);
		}
		else
		{
			lseek(res_fd, 0, SEEK_SET);
			int res=0; //not match
			write(res_fd, &res, sizeof(res));
			close(res_fd);
		}
		return;
	}
}

void send_try_match(int socket_fd)
{
	cJSON *tmp=cJSON_CreateObject();
	cJSON_AddStringToObject(tmp,"cmd","try_match");
	char *ccc=cJSON_PrintUnformatted(tmp);
	char output[30]={0};
	sprintf(output, "%s\n", ccc);
	send(socket_fd, output, strlen(output), 0);
	cJSON_Delete(tmp);
	return;
}

void send_matched(struct User *info, int socket_fd, char *filter_f, int flen)
{
	cJSON *tmp=cJSON_CreateObject();
	cJSON_AddStringToObject(tmp,"cmd","matched");
	cJSON_AddStringToObject(tmp,"name",info->name);
	cJSON_AddNumberToObject(tmp, "age", info->age);
	cJSON_AddStringToObject(tmp,"introduction",info->introduction);
	cJSON_AddStringToObject(tmp,"gender",info->gender);
	filter_f[flen-1]=0;
	cJSON_AddStringToObject(tmp,"filter_function",filter_f+1);
	filter_f[flen-1]='\"';
	char *ccc=cJSON_PrintUnformatted(tmp);
	int len=strlen(ccc)+2;
	char *out=(char *)malloc(len*sizeof(char));
	memset(out, 0, len);
	sprintf(out, "%s\n", ccc);
	send(socket_fd, out, len-1, 0);
	cJSON_Delete(tmp);
	free(out);
	return;
}


int main(int argc, char **argv)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);
	
	// 利用 struct sockaddr_in 設定伺服器"自己"的位置
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr)); // 清零初始化，不可省略
	server_addr.sin_family = PF_INET;              // 位置類型是網際網路位置
	server_addr.sin_addr.s_addr = INADDR_ANY;      // INADDR_ANY 是特殊的IP位置，表示接受所有外來的連線
	server_addr.sin_port = htons(atoi(argv[1]));          
	
	// 使用 bind() 將伺服socket"綁定"到特定 IP 位置
	int retval = bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
	if(retval==-1)
	{
		printf("socket fail\n");
		return 0;
	}
	
	// 呼叫 listen() 告知作業系統這是一個伺服socket
	retval = listen(sockfd, 5);
	assert(!retval);
	
	fd_set readset;
	fd_set working_readset;
	FD_ZERO(&readset);

	// 將 socket 檔案描述子放進 readset
	FD_SET(sockfd, &readset);
	for(int i=0; i<MAX_FD;i++) ptr_to_buffer[i]=buffer[i];
	while (1)
	{
    memcpy(&working_readset, &readset, sizeof(fd_set));
    retval = select(MAX_FD, &working_readset, NULL, NULL, NULL);

    if (retval < 0) // 發生錯誤
    {
      perror("select() went wrong");
      exit(errno);
    }

    if (retval == 0) continue; // 排除沒有事件的情形
        
    for (int fd = 0; fd < MAX_FD; fd += 1) // 用迴圈列舉描述子
    {
      // 排除沒有事件的描述子
      if (!FD_ISSET(fd, &working_readset))  continue;
      // 分成兩個情形：接受新連線用的 socket 和資料傳輸用的 socket
      if (fd == sockfd)
      {
        // sockfd 有事件，表示有新連線
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(fd, (struct sockaddr*) &client_addr, &addrlen);
        if (client_fd >= 0) FD_SET(client_fd, &readset); // 加入新創的描述子，用於和客戶端連線    
      }
      else
      {
        // 這裏的描述子來自 accept() 回傳值，用於和客戶端連線
        ssize_t sz;
        sz = recv(fd, ptr_to_buffer[fd], BUFFER_SIZE - (ptr_to_buffer[fd] - buffer[fd]), 0); // 接收資料

        if (sz == 0) // recv() 回傳值?零表示客戶端已關閉連線
        {
            if(connect_who[fd]!=0)
						{
							cJSON *data_json;
							data_json=cJSON_CreateObject();
							cJSON_AddStringToObject(data_json, "cmd", "other_side_quit");
							char *cccd=cJSON_PrintUnformatted(data_json);
							int len=strlen(cccd);
							char *out=(char *)malloc(len+2);
							sprintf(out, "%s\n",cccd);
							send(connect_who[fd], out, len+1, 0); 
							free(out);
							connect_who[connect_who[fd]]=0;
							connect_who[fd]=0;
							cJSON_Delete(data_json);
						}
						fprintf(stderr, "%s (fd = %d) close socket, queue size=%d\n", user_array[fd]->name, fd, size);
						
						char ffff[15],path[20]={0};
						get_func_name(ffff,fd);
						sprintf(path,"%s.so",ffff);
						remove(path);
						if(in_wait_queue[fd]==1)
						{
							list *visit=front;
							for(int i=0;i<size;i++)
							{
								if(visit->value==fd)
								{
									del(visit);
									fprintf(stderr, "%s (fd = %d) delete from queue\n", user_array[fd]->name, fd);
									break;
								}
								visit=visit->next;
							}
							in_wait_queue[fd]==0;
						}
						free(filter_function_array[fd]);
						free(user_array[fd]);
            close(fd);
            FD_CLR(fd, &readset);
        }
        else if (sz < 0) // 發生錯誤
        {
            /* 進行錯誤處理
               ...略...  */
        }
        else // sz > 0，表示有新資料讀入
        {
      		ptr_to_buffer[fd]+=sz;
        	*(ptr_to_buffer[fd])=0;
        	if(*(ptr_to_buffer[fd]-1)=='\n')
        	{
        		fprintf(stderr, "json = %s",buffer[fd]);
        		*(ptr_to_buffer[fd]-1)=0;
        		ptr_to_buffer[fd]=buffer[fd];
        		cJSON *data_json = cJSON_Parse(buffer[fd]);
        		fprintf(stderr,"get command ->");
        		char *command = string_GetObjectItem("cmd", data_json);
        		fprintf(stderr,"command = %s\n",command);
        		if(strcmp(command, try_match)==0)
        		{
        			send_try_match(fd);
        			struct User *info=(struct User *)malloc(sizeof(struct User));
        			load_data_to_user(info, data_json);
        			user_array[fd]=info;
        			filter_function_array[fd]=string_GetObjectItem("filter_function", data_json);
							char func_name[10],syscmd[100];
        			get_func_name(func_name, fd);
        			sprintf(syscmd, "%s.c",func_name);
        			int len=strlen(filter_function_array[fd]), func_d=open(syscmd, O_WRONLY|O_CREAT|O_TRUNC, 0666), from_here=-1;
        			write(func_d, user_def, strlen(user_def));
        			write(func_d, filter_function_array[fd]+1, len-2);
        			close(func_d);
        			sprintf(syscmd, "gcc -fPIC -O2 -std=c11 %s.c -shared -o %s.so", func_name, func_name);
        			system(syscmd);
        			while(1)
        			{
	        			int pid=fork(), jud_out=0;
	        			if(pid==0)//child process
	        			{
	        				match(fd, func_name, from_here);
	        				return 0;
								}
								else if(pid>0)//parent process
								{
									int stloc;
									waitpid(pid, &stloc, 0);
									int res_fd = open("res_from_child", O_RDONLY), res1;
									read(res_fd, &res1, sizeof(res1));
									if(res1==0) //not match
									{
										fprintf(stderr,"%s (fd=%d) not match\n",user_array[fd]->name, fd);
										insert(fd);
										in_wait_queue[fd]=1;
										close(res_fd);
										break;
									}
									else if(res1==1) //crash
									{
										read(res_fd, &from_here, sizeof(from_here));
										fprintf(stderr,"%s filter function crash from_here = %d,size = %d\n",user_array[fd]->name,from_here,size);
										close(res_fd);
										if(from_here==size-1)
										{
											fprintf(stderr,"%s crash at end\n",user_array[fd]->name);
											insert(fd);
											in_wait_queue[fd]=1;
											break;
										}
										else continue;
									}
									else //matched
									{
										int res2;
										read(res_fd, &res2, sizeof(res2));
										close(res_fd);
										fprintf(stderr,"%s (fd=%d) and %s (fd=%d) match\n",user_array[fd]->name, fd, user_array[res2]->name, res2);
										connect_who[res2]=fd; connect_who[fd]=res2;
										send_matched(user_array[res2], fd, filter_function_array[res2], strlen(filter_function_array[res2]));
										send_matched(user_array[fd], res2, filter_function_array[fd], len);
										list *now_vis=front;
										for(int i=0;i<size;i++)
										{
											if(now_vis->value == res2)
											{
												del(now_vis);
												fprintf(stderr,"%s(fd=%d) delete from wait queue\n", user_array[res2]->name, res2);
												break;
											}
											now_vis = now_vis->next;
										}
										in_wait_queue[res2]=0;
										break;
									}
								}
							}
						}
						else if(strcmp(command, send_message)==0)
						{
							char *cccd=cJSON_PrintUnformatted(data_json);
							int len=strlen(cccd);
							char *out=(char *)malloc(len+2);
							sprintf(out, "%s\n",cccd);
							send(fd, out, len+1, 0); free(out);
							cJSON_DeleteItemFromObject(data_json, "cmd");
							cJSON_AddStringToObject(data_json, "cmd", "receive_message");
							cccd=cJSON_PrintUnformatted(data_json);
							len=strlen(cccd);
							out=(char *)malloc(len+2);
							sprintf(out, "%s\n",cccd);
							send(connect_who[fd], out, len+1, 0); free(out);
						}
						else
						{
							char *cccd=cJSON_PrintUnformatted(data_json);
							int len=strlen(cccd);
							char *out=(char *)malloc(len+2);
							sprintf(out, "%s\n",cccd);
							fprintf(stderr,"%s quit\n",user_array[fd]->name);
							send(fd, out, len+1, 0); free(out);
							if(connect_who[fd]!=0)
							{
								cJSON_DeleteItemFromObject(data_json, "cmd");
								cJSON_AddStringToObject(data_json, "cmd", "other_side_quit");
								cccd=cJSON_PrintUnformatted(data_json);
								len=strlen(cccd);
								out=(char *)malloc(len+2);
								sprintf(out, "%s\n",cccd);
								send(connect_who[fd], out, len+1, 0); free(out);
								connect_who[connect_who[fd]]=0;
								connect_who[fd]=0;
							}
							if(in_wait_queue[fd]==1)
							{
								list *now_vis=front;
								for(int i=0;i<size;i++)//remove from wait list
								{
									if(now_vis->value == fd)
									{
										del(now_vis);
										fprintf(stderr,"%s(fd=%d) delete from wait queue\n", user_array[fd]->name, fd);
										break;
									}
									now_vis = now_vis->next;
								}
								in_wait_queue[fd]==0;
							}
							else fprintf(stderr, "%s not in wait queue\n",user_array[fd]->name);
						}
						cJSON_Delete(data_json);
						free(command);
					}
        }
      }
    }
	}
	// 結束程式前記得關閉 sockfd
	close(sockfd);
}
