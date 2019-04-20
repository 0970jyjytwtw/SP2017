#include<stdio.h>
#include "list_file.h"
#include "md5.h"
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<ctype.h>
int compare(const void *a , const void *b)
{
	return(strcmp(*(char **)a,*(char **)b));
}
void status(char *path)
{
	struct FileNames dir_info=list_file(path);
	if(dir_info.length <0)
	{
		free_file_names(dir_info);
		return;
	}
	else if(dir_info.length==0)
	{
		printf("[new_file]\n[modified]\n[copied]\n");
	}
	else
	{
		qsort((void **)dir_info.names,dir_info.length,sizeof(dir_info.names[0]),compare);
		int recordexist=0;
		for(int i = 0; i<dir_info.length;i++)
		{
			if(strcmp(dir_info.names[i],".loser_record")==0)
			{
				recordexist=1;
				break;
			}
		}
		if(recordexist==0) //record doesn't exist
		{		 	//output all filename
			printf("[new_file]\n");
			for(int i=0;i<dir_info.length;i++)
			{
				 printf("%s\n",dir_info.names[i]);
			}
			printf("[modified]\n[copied]\n");	
		}
		else
		{
			uint8_t dir_md5[dir_info.length][40];
			for(int i=0;i<dir_info.length;i++) // calculate md5
			{
				//get filepath
				int path_len=strlen(path),file_namelen=strlen(dir_info.names[i]),bufferlen=2048,nowlen=0;
				char *filepath;
				if(strcmp(dir_info.names[i],".loser_record")==0) continue;
				filepath=(char *)malloc(sizeof(char)*(path_len+file_namelen+5));
				strcpy(filepath,path);
				if(filepath[path_len-1]!='/')
				{
					filepath[path_len]='/'; filepath[path_len+1]='\0';				
				}
				strcat(filepath,dir_info.names[i]);
				//get file's content
				FILE *fp;
				fp=fopen(filepath,"rb");
				if(fp==NULL) continue;
				char *file_content=(char *)malloc(sizeof(char)*bufferlen),tmpc;
				while(fread(&tmpc,sizeof(char),1,fp))
				{
					file_content[nowlen]=tmpc;
					nowlen++;
					if(nowlen>=bufferlen-1)
					{
						bufferlen*=2;
						file_content=(char * )realloc(file_content , bufferlen);
					}
				}
				makemd5(dir_md5[i],file_content,nowlen);
		//		printf("%s %s\n",dir_info.names[i],dir_md5[i]);
				fclose(fp);
				free(filepath);
			//	free(file_content);
			}
			//get recordpath
			FILE *record; int path_len=strlen(path);
			char *recordpath=(char *)malloc(sizeof(char)*(path_len+16));
			strcpy(recordpath,path);
			if(recordpath[path_len-1]!='/')
			{
				recordpath[path_len]='/'; recordpath[path_len+1]='\0';
			}
			strcat(recordpath,".loser_record");
			record=fopen(recordpath,"r+");
			if(record==NULL)  return;
			//find (MD5)
			fseek(record,sizeof(char)*-2,SEEK_END);
		//	printf("seek\n");
			do
			{
				char tmpc;
				tmpc=fgetc(record);
				if(tmpc==')')
				{
					fseek(record,sizeof(char),SEEK_CUR);
					break;
				} 
			}while(fseek(record,sizeof(char)*-2,SEEK_CUR)==0);
			char tmpc,modified[dir_info.length][256],newfile[dir_info.length][256],copy[dir_info.length][256*2+4],oldfile[dir_info.length][256],oldmd5[dir_info.length][34];
			int modified_num=0,newfile_num=0,copy_num=0,oldfile_num=0,tmplen=0;
			while(fscanf(record,"%s%s",oldfile[oldfile_num],oldmd5[oldfile_num])!=EOF)//get oldfile's name and md5
			{
					oldfile_num++;
			}
		//	for(int i=0;i<oldfile_num;i++) printf("old=> %s %s\n",oldfile[i],oldmd5[i]);
			fclose(record);
			free(recordpath);
			//find old file
			int isold[dir_info.length];
			for(int i=0;i<dir_info.length;i++) isold[i]=0;
			for(int i=0;i<dir_info.length;i++)
			{
				for(int j=0;j<oldfile_num;j++)
				{
					if(strcmp(oldfile[j],dir_info.names[i])==0)
					{
						isold[i]=1;
						break;
					}
				}
			}
			for(int i=0;i<dir_info.length;i++)
			{
				int judnew=1,judcopy=0;
				if(strcmp(dir_info.names[i],".loser_record")==0) continue;
				char newmd5[40];
				strcpy(newmd5,dir_md5[i]);
				for(int j=0;j<oldfile_num;j++)
				{
					int name_cmp=strcmp(oldfile[j],dir_info.names[i]),md5_cmp=strcmp(oldmd5[j],newmd5);
					if(name_cmp==0 && md5_cmp==0)
					{
						judnew=0;
						break;
					}
					else if(name_cmp==0 && md5_cmp!=0)
					{
						judnew=0;
						strcpy(modified[modified_num],oldfile[j]);
						modified_num++;
					}
					else if(name_cmp!=0&&md5_cmp==0 && judcopy==0 && isold[i]==0)
					{
						judnew=0; judcopy=1;
						sprintf(copy[copy_num],"%s => %s",oldfile[j],dir_info.names[i]);
						copy_num++;
					}
				}
				if(judnew==1)
				{
					strcpy(newfile[newfile_num],dir_info.names[i]);
					newfile_num++;
				}
			}
			printf("[new_file]\n");
			for(int i=0;i<newfile_num;i++) printf("%s\n",newfile[i]);
			printf("[modified]\n");
			for(int i=0;i<modified_num;i++) printf("%s\n",modified[i]);
			printf("[copied]\n");
			for(int i=0;i<copy_num;i++) printf("%s\n",copy[i]);
		}
	}
	free_file_names(dir_info);
	return;
}
void commit(char *path)
{	
	struct FileNames dir_info=list_file(path);
	if(dir_info.length <=0) return;
	else
	{
		qsort((void **)dir_info.names,dir_info.length,sizeof(dir_info.names[0]),compare);
		int recordexist=0;
		for(int i = 0; i<dir_info.length;i++)
		{
			if(strcmp(dir_info.names[i],".loser_record")==0)
			{
				recordexist=1;
				break;
			}
		}
		if(recordexist==0) //record doesn't exist
		{		 	//all filename new
			//get recordpath
			FILE *record; int path_len=strlen(path);
			char *recordpath=(char *)malloc(sizeof(char)*(path_len+16));
			strcpy(recordpath,path);
			if(recordpath[path_len-1]!='/')
			{
				recordpath[path_len]='/'; recordpath[path_len+1]='\0';
			}
			strcat(recordpath,".loser_record");
			record=fopen(recordpath,"a");
			fprintf(record,"# commit 1\n[new_file]\n");
			for(int i=0;i<dir_info.length;i++) fprintf(record,"%s\n",dir_info.names[i]);
			fprintf(record,"[modified]\n[copied]\n(MD5)\n");	
			for(int i=0;i<dir_info.length;i++) // calculate md5
			{
				//get filepath
				int file_namelen=strlen(dir_info.names[i]),bufferlen=2048,nowlen=0;
				char *filepath;
				if(strcmp(dir_info.names[i],".loser_record")==0) continue;
				filepath=(char *)malloc(sizeof(char)*(path_len+file_namelen+5));
				strcpy(filepath,path);
				if(filepath[path_len-1]!='/')
				{
					filepath[path_len]='/'; filepath[path_len+1]='\0';				
				}
				strcat(filepath,dir_info.names[i]);
				FILE *fp;
			//	printf("%s\n",filepath);
				fp=fopen(filepath,"r+");
				if(fp==NULL) continue;
				char *file_content=(char *)malloc(sizeof(char)*bufferlen),tmpc;
				//get file content
				while(fread(&tmpc,sizeof(char),1,fp))
				{
					file_content[nowlen]=tmpc;
					nowlen++;
					if(nowlen>=bufferlen-1)
					{
						bufferlen*=2;
						file_content=(char * )realloc(file_content , bufferlen);
					}
				}
				uint8_t nowmd5[40];
				makemd5(nowmd5,file_content,nowlen);

			//	printf("%d\n%s\n%s\n",nowlen,file_content,nowmd5);
				fprintf(record,"%s %s\n",dir_info.names[i],nowmd5);
				fclose(fp);
				//free(file_content);
				free(filepath);
			}
			fclose(record);
			free(recordpath);
		}
		else
		{
			uint8_t dir_md5[dir_info.length][40];
			for(int i=0;i<dir_info.length;i++) // calculate md5
			{
				int path_len=strlen(path),file_namelen=strlen(dir_info.names[i]),bufferlen=2048,nowlen=0;
				char *filepath;
				if(strcmp(dir_info.names[i],".loser_record")==0) continue;
				//find filepath
				filepath=(char *)malloc(sizeof(char)*(path_len+file_namelen+5));
				strcpy(filepath,path);
				if(filepath[path_len-1]!='/')
				{
					filepath[path_len]='/'; filepath[path_len+1]='\0';				
				}
				strcat(filepath,dir_info.names[i]);
				FILE *fp;
				fp=fopen(filepath,"rb");
				if(fp==NULL) continue;
				char *file_content=(char *)malloc(sizeof(char)*bufferlen),tmpc;
				while(fread(&tmpc,sizeof(char),1,fp))
				{
					file_content[nowlen]=tmpc;
					nowlen++;
					if(nowlen>=bufferlen-1)
					{
						bufferlen*=2;
						file_content=(char * )realloc(file_content , bufferlen);
					}
				}
				makemd5(dir_md5[i],file_content,nowlen);
				fclose(fp);
		//		free(file_content);
				free(filepath);
			}
			//get recorfpath
			FILE *record; int path_len=strlen(path);
			char *recordpath=(char *)malloc(sizeof(char)*(path_len+16));
			strcpy(recordpath,path);
			if(recordpath[path_len-1]!='/')
			{
				recordpath[path_len]='/'; recordpath[path_len+1]='\0';
			}
			strcat(recordpath,".loser_record");
			record=fopen(recordpath,"r+");
			if(record==NULL) return;
			fseek(record,sizeof(char)*-2,SEEK_END);
			//find (MD5)
			while(fseek(record,sizeof(char)*-2,SEEK_CUR)==0)
			{
				char tmpc;
				tmpc=fgetc(record);
				if(tmpc==')')
				{
					fseek(record,sizeof(char),SEEK_CUR);
					break;
				} 
			}
			char modified[dir_info.length][256],newfile[dir_info.length][256],copy[dir_info.length][256*2+4],oldfile[dir_info.length][256],oldmd5[dir_info.length][34];
			int modified_num=0,newfile_num=0,copy_num=0,oldfile_num=0;
			while(fscanf(record,"%s%s",oldfile[oldfile_num],oldmd5[oldfile_num])!=EOF) oldfile_num++;
			fseek(record,sizeof(char)*-2,SEEK_END);
			//find # commit ?
			int commit_time;
			while(fseek(record,sizeof(char)*-2,SEEK_CUR)==0)
			{
				char tmpc;
				tmpc=fgetc(record);
				if(tmpc=='#')
				{
					fseek(record,sizeof(char)*8,SEEK_CUR);
					fscanf(record,"%d",&commit_time);
					break;
				} 
			}
			fclose(record);
			//find old file
			int isold[dir_info.length];
			for(int i=0;i<dir_info.length;i++) isold[i]=0;
			for(int i=0;i<dir_info.length;i++)
			{
				for(int j=0;j<oldfile_num;j++)
				{
					if(strcmp(oldfile[j],dir_info.names[i])==0)
					{
						isold[i]=1;
						break;
					}
				}
			}
			for(int i=0;i<dir_info.length;i++)
			{
				int judnew=1,judcopy=0;
				if(strcmp(dir_info.names[i],".loser_record")==0) continue;
				char newmd5[40];
				strcpy(newmd5,dir_md5[i]);
				for(int j=0;j<oldfile_num;j++)
				{
					int name_cmp=strcmp(oldfile[j],dir_info.names[i]),md5_cmp=strcmp(oldmd5[j],newmd5);
					if(name_cmp==0 && md5_cmp==0)
					{
						judnew=0;
						break;
					}
					else if(name_cmp==0 && md5_cmp!=0)
					{
						judnew=0;
						strcpy(modified[modified_num],oldfile[j]);
						modified_num++;
					}
					else if(name_cmp!=0&&md5_cmp==0 && judcopy==0 &&isold[i]==0)
					{
						judnew=0; judcopy=1;
						sprintf(copy[copy_num],"%s => %s",oldfile[j],dir_info.names[i]);
						copy_num++;
					}
				}
				if(judnew==1)
				{
					strcpy(newfile[newfile_num],dir_info.names[i]);
					newfile_num++;
				}
			}
			if(newfile_num!=0 || modified_num!=0 || copy_num!=0)
			{
				record = fopen(recordpath,"a");
				fprintf(record,"\n# commit %d\n[new_file]\n",commit_time+1);
				for(int i=0;i<newfile_num;i++) fprintf(record,"%s\n",newfile[i]);
				fprintf(record,"[modified]\n");
				for(int i=0;i<modified_num;i++) fprintf(record,"%s\n",modified[i]);
				fprintf(record,"[copied]\n");
				for(int i=0;i<copy_num;i++) fprintf(record,"%s\n",copy[i]);
				fprintf(record,"(MD5)\n");
				for(int i=0;i<dir_info.length;i++)
				{
					if(strcmp(dir_info.names[i],".loser_record")==0) continue;
					else
					{
						fprintf(record,"%s %s\n",dir_info.names[i],dir_md5[i]);
					}
				}
				fclose(record);
			}
			free(recordpath);
		}
	}
	free_file_names(dir_info);
	return;
}
void logloser(char *path, int output_num)
{
	struct FileNames dir_info=list_file(path);
	if(dir_info.length<=0) return;
	else 
	{
		int recordexist=0;
		for(int i=0;i<dir_info.length;i++)
		{
			if(strcmp(dir_info.names[i],".loser_record")==0)
			{
				recordexist=1; break;
			}
		}
		if(recordexist==1)
		{
			FILE *record; int path_len=strlen(path);
			char *recordpath=(char *)malloc(sizeof(char)*(path_len+16));
			strcpy(recordpath,path);
			if(recordpath[path_len-1]!='/')
			{
				recordpath[path_len]='/'; recordpath[path_len+1]='\0'; 
			}
			strcat(recordpath,".loser_record");
			record=fopen(recordpath,"r+");
			free(recordpath);
			//output
			int judout=0,previouslen=0;
			for(int i=1;i<=output_num;i++)
			{
			//	printf("~~onlyfor %d\n",i);
				fseek(record,sizeof(char)*(-1*previouslen-1),SEEK_END);
				if(judout==1) break;
				if(i!=1) printf("\n");
				do
				{
					previouslen++;
					char tmpc;
					tmpc=fgetc(record);
					if(tmpc=='#')
					{
						printf("#"); break;
					} 
				}while(fseek(record,sizeof(char)*-2,SEEK_CUR)==0);
				char tmps[9],pre_char='o'; int tmpd;
				fscanf(record,"%s%d",tmps,&tmpd);
				if(tmpd==1) judout=1;
				printf(" commit %d",tmpd);
				while(tmps[0]=fgetc(record))
				{
					if(feof(record))
					{
				//		printf("~~~in the end ~~~~\n");
						break;
					}
					if(pre_char=='\n' && tmps[0]=='\n') break;
					printf("%c",tmps[0]);
					pre_char=tmps[0];
				}
			}
			fclose(record);
		}
	}
	free_file_names(dir_info);
	return;
}
int main(int argc , char **argv)
{
	if(argc<3||argc>4) return 0;
	if(strcmp(argv[1],"status")==0)
	{
		status(argv[2]);
	}
	else if(strcmp(argv[1],"commit")==0) commit(argv[2]);
	else if(strcmp(argv[1],"log")==0)
	{
		int num_len=strlen(argv[2]),num=0,tui=1;
		for(int i=num_len-1;i>=0;i--)
		{
			num+=(argv[2][i]-'0')*tui;
			tui*=10;
		}
		logloser(argv[3],num);
	}
	else
	{
		FILE *fp;
		char *con_path;
		if(argc == 3) 
		{
			con_path=(char *)malloc(sizeof(char)*(strlen(argv[2])+30));
			strcpy(con_path,argv[2]);
		}
		else if(argc == 4) 
		{
			con_path=(char *)malloc(sizeof(char)*(strlen(argv[3])+30));
			strcpy(con_path,argv[3]);
		}
		int path_len=strlen(con_path);
		if(con_path[path_len-1]!='/')
		{
			con_path[path_len]='/'; con_path[path_len+1]='\0';
		}
		strcat(con_path,".loser_config");
		fp=fopen(con_path,"r+");
		if(fp == NULL)
		{
		//	printf("%s\nNO\n",con_path);
			return 0;
		}
		else
		{
		//	printf("in config\n");
			char nick_name[260],laji[2],command[12];
			while(fscanf(fp,"%s%s%s",nick_name,laji,command)!=EOF )
			{
				if(strcmp(nick_name,argv[1])==0)
				{
					if(command[0]=='c') commit(argv[2]);
					else if(command[0]=='s') status(argv[2]);
					else if(command[0]=='l')
					{		
						int num_len=strlen(argv[2]),num=0,tui=1;
						for(int i=num_len-1;i>=0;i--)
						{
							num+=(argv[2][i]-'0')*tui;
							tui*=10;
						}
						logloser(argv[3],num);
					}
					break;
				}
			}
		}
		free(con_path);
		fclose(fp);
	}
	return 0;
}
