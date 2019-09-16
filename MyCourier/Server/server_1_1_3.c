#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mysql.h>
#define bzero(a,b) (memset((a),0,(b))
#define MYPORT 26588
#define MAXSOCKED 10
#define MAXDATASIZE 1000
#define DEBUG
volatile sig_atomic_t _running = 1; 
/*ά����ǰ�����û��б� Maintains current online usrers*/
typedef struct _onusers
{
	
	char id[11];//�û�id     User id
	pid_t pid;//�ӽ���pid    Child process pid
	int fw;//д�Ĺܵ�        Write pipe
	int fr;//���Ĺܵ�        Read pipe
	int code;
	struct _onusers *next;
} onusers;
onusers *head,*p1,*p2;
int fpipe[50][2][2];//�������50λ�û�����         Maximum 50 users online
int pipelen;//�ܵ��е��ļ�����                     File lenth for pipe
int reg(char *addr,int client);
int checkreg(char *addr,char *nick,char *pwd ,char *ccode,int client);
/*mysqlʹ�õĲ���*/
MYSQL mmysql;
MYSQL *mysql;
MYSQL_RES *sqlres;
char *sqlserver="localhost";
char *sqluser="icq";
char *sqlpwd="icqsql";
char *sqldb="icq";
char sqlbuf[100];
unsigned int check_statues;
FILE *fsql;
MYSQL_ROW sqlrow;
int wrtsta;		//д���ַ����ĵ�ǰλ          Current index in the write string

/***�Զ��庯����ÿ�δ��ļ���д�룬�ر�***//*Every time, open a file, write in it, and the close it*/
int printl(const char *format,...)
{
	FILE *flog;
	int iReturn;
	va_list pArgs;
	if( (flog=fopen("/home/myserver/error1_1_3.log","a")) == NULL)
		return 0;
	va_start (pArgs,format);
	iReturn = vfprintf(flog,format,pArgs);
	va_end (pArgs);
	fclose(flog);
	return iReturn;
}


void sigterm_handler(int arg)
{
	if(arg==SIGTERM)		
		_running=0;
	else if(arg==SIGCHLD)//����������ӽ��� handle dead child process
	{
		pid_t pid;
		pid=wait(NULL);
		for(p1=head;p1->next;p1=p1->next)
		{
			p2=p1->next;
			if(p2->pid==pid)
			{
				fpipe[p2->code][0][0]=0;
				fpipe[p2->code][0][1]=0;
				fpipe[p2->code][1][0]=0;
				fpipe[p2->code][1][1]=0;
				close(p2->fr);
				close(p2->fw);
				p1->next=p2->next;
				free(p2);
				break;
			}
		}
#ifdef DEBUG
		time_t now;
		FILE *fchild;
		time(&now);
		fchild=fopen("/home/myserver/child.log","a");
		fprintf(fchild,"%d end at %s",pid,ctime(&now));
		fclose(fchild);
#endif
	}
}

int main(void)
{
	int server;//socket()�����ķ���ֵ                      returned value of socket()
	struct sockaddr_in my_addr;//��������ַ�Ľṹ��         
	pid_t pc,ppc=1;//����pid�������ػ��������ӽ���ʱʹ��
	int i,fd;
	FILE *fp;
	time_t now;
	char rcvbuf[MAXDATASIZE];//���ڽ��յ��ַ���            string to be used to recieve
	char sndbuf[MAXDATASIZE];//���ڷ��͵��ַ���            string to be used to send
	char opbuf[MAXDATASIZE];//���ڲ������ַ���             Intermediate string
	int numbytes;//���յ����ֽ���
	char qid[11];//�û��ʺ�                                My id   
	char oid[11];//�Է��˺�                                Other's id
	int qpwd[21];//����                                    My password
	int logflag=0;
	int numfriends;//������Ŀ                              Number of friends
	int thispipe;//��ǰ����Ĺܵ�                          Current pipe
	fd_set pset;//������I/O�����ļ���                      For select() (parent process)
	fd_set sset;//�ӽ���I/O�����ļ���                      For select() (child process)
	int childw,childr;//�ӽ��̵Ķ�д�ܵ�
	int maxf;//����ļ�����
	char nick[31]={0};
	char cfcode[33]={0};
	char pwd[21]={0};
	char mailbox[51]={0};
	
	



	


	/*����socketaddr_in�ṹ(���������׽���)*/
	memset(&my_addr,0,sizeof(my_addr));//��ȫ������
	my_addr.sin_family=AF_INET;//Address family Internet
	my_addr.sin_addr.s_addr=htonl(INADDR_ANY);//ʹ�õ�ǰ���������õ�����ip��ַ
	my_addr.sin_port=htons(MYPORT);//ʹ�ö���Ķ˿�ͨ��


	/*����ͻ����׽���*/
 	int client;
	struct sockaddr_in from_addr;
	int from_len=sizeof(from_addr);
	memset(&from_addr,0,sizeof(from_addr));


	/*��ʼ������*/
	head=(onusers *)malloc(sizeof(onusers));
	sprintf(head->id,"0");
	head->pid=0;
	head->fw=0;
	head->fr=0;
	head->next=NULL;
	head->code=-1;

	/*��һ���ļ���׼��д��*/
	time(&now);
	printl("Program start at %s",ctime(&now));

	/*����socket()�����*/
	server=socket(AF_INET,SOCK_STREAM,0);
	if(server==-1)
	{
		printl("Socket error\n\n");
		return 0;
	}

	/*����bind()*/
	if( bind(server,(struct sockaddr*)&my_addr,sizeof(my_addr)) != 0 )
	{
		printl("Bind error\n\n");
		return 0;
	}

	/*����listen()*/
	if( listen(server,10) != 0 )
	{
		printl("Listen error\n\n");
		return 0;
	}
  
	if(ppc>0)
		printl("Server started with port %d\n",MYPORT);


	/* �����ػ����̣�����������*/
	pc = fork(); //��һ�� ����
	if(pc<0)
	{
		printl("error fork\n\n");
		exit(1);
	}
	if(pc>0)
	{
#ifdef DEBUG
		printf("Child is %d.Now suiside.\n",pc);
#endif
		exit(0);
	}	
	setsid(); //�ڶ���
	chdir("/"); //������
	umask(0); //���Ĳ�
	for(i=0;i<3;i++) //���岽	
	      close(i);
	signal(SIGTERM, sigterm_handler); 
	signal(SIGHUP, sigterm_handler); 
	signal(SIGCHLD,sigterm_handler);

	/*��ʼ��MySQL��*/
	mysql_init(&mmysql);
	/*if( mysql == NULL);
	{
		printl("MySQL init error\n");
		exit(-1);
	}*/	

	/*����socket���*/
	/*��ʼ����ѭ���������û�����*/

	while( _running )
	{
		FD_ZERO (&pset);
		FD_SET(server,&pset);
		maxf=server;
		for(p1=head->next;p1;p1=p1->next)
		{
			FD_SET(p1->fr,&pset);
			maxf=maxf>(p1->fr)?maxf:(p1->fr);
		}
		maxf++;
		printl("form p:\n");
		for(p1=head;p1!=NULL;p1=p1->next)
			printl("id=%s,code=%d,idaddr=%x",p1->id,p1->code,p1->id);
		printl("\n");
		printl("Parent select\n");
		if(select(maxf,&pset,NULL,NULL,NULL)<-1)
		{
			printl("select error\n\n");
			return -1;
		}
		if(FD_ISSET(server,&pset))
		{
			printl("select return caz socket writable\n");
			/*��һ�����еĹܵ�*/
			for(i=0;i<50;i++)
			{
				if(fpipe[i][0][0]==0 && fpipe[i][0][1]==0 && fpipe[i][1][0]==0 && fpipe[i][1][1]==0 )
				{
					thispipe=i;
					if(i==49)
					      i--;
					break;
				}

			}
			if(i==50)
			{
				printl("connection max\n");
				continue;
			}
			if(pipe(fpipe[thispipe][0])==-1)
			{
				printl("pipe error\n");
				exit(-1);
			}
			if(pipe(fpipe[thispipe][1])==-1)
			{
				printl("pipe error\n");
				exit(-1);
			}
			printl("This = %d,ready to accept\n",thispipe);
			client=accept(server,(struct sockaddr*)&from_addr,&from_len);
			/*�����ӽ��̣���Ϊ���ǲ���������*/
			ppc=fork();
			if(ppc<0)
			{
				printl("error fork\n\n");
				exit(1);
			}
			else if(ppc==0)
			{      
				printl("child start\n");
				printl("client=%d,thispipe=%d,fpipe[thispipe][1][1]=%d,sndbuf=%s,sndbufaddr=%x\n",client,thispipe,fpipe[thispipe][1][1],sndbuf,sndbuf);
				/*�ӽ��̱��*/
				close(server);//�رռ����˿�
				/*�رղ������Լ��Ĺܵ�*/
				for(p1=head->next;p1;p1=p1->next)
				{
					close(p1->fw);
					close(p1->fr);
				}
				/*�����븸���̽����Ĺܵ�*/
				childw=fpipe[thispipe][1][1];				
				childr=fpipe[thispipe][0][0];
				close(fpipe[thispipe][0][1]);
				close(fpipe[thispipe][1][0]);
				/*�������*/
				for(p1=head;p1;)
				{
					p2=p1->next;
					free(p1);
					p1=p2;
				}


				/*�Ƚ����û�ָ�������ǵ�½��ע�ᣬ��������*/
				if ( (numbytes=recv(client,rcvbuf,MAXDATASIZE,0)) == -1 )
				{
					printl("recv error\n\n");
					return -1;
				}
				rcvbuf[numbytes]='\0';
				time(&now);
				printl("recv:%s at %s\n",rcvbuf,ctime(&now));

				/*�ͻ��˷�����Ϣ��ǰ��λͳһΪָ���룬���Կ�����ǰ��λ�������ж�*/
				for(i=0;i<6;i++)
				      opbuf[i]=rcvbuf[i];
				opbuf[6]='\0';


				/*��¼ָ��*/
				if ( strcmp(opbuf,"#login")==0 )
				{
					/*��ȡ�û��������û���������*/
					sscanf(rcvbuf,"#login#%[0-9]#%s",qid,qpwd);					
					for(p1=head->next;p1;p1=p1->next)
					{
						if(strcpy(p1->id,qid)==0)
						{
							sprintf(sndbuf,"#logst#0#already");
							send(client,sndbuf,strlen(sndbuf),0);
							exit(0);
						}
					}					
					
					
					/*��������mysql������*/
					/*����*/
					if (!mysql_real_connect(&mmysql, sqlserver,sqluser, sqlpwd, sqldb, 0, NULL, 0)) 
					{
						printl("MySQL connect:");
						printl("%s\n", mysql_error(&mmysql));
						exit(-1);
					}

					/*��MySQL�в����Ƿ�ƥ��*/				
					sprintf(sqlbuf,"select * from user where id='%s'&&password=md5('%s')",qid,qpwd);
					if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf))) 
					{
						printl("%s\n", mysql_error(&mmysql));
						exit(-1);
					}
					sqlres=mysql_store_result(&mmysql);

					/*����鵽һ��,˵��ƥ��*/
					check_statues=mysql_num_rows(sqlres);
					mysql_free_result(sqlres);



					/*��ͻ��˷��͵�½�ɹ�����Ϣ�������*/
					if(check_statues==1)
					{
	      					sprintf(sndbuf,"#logst#1");
						logflag=1;
					}
					/*�����û���¼ʧ��*/
					else
	      					sprintf(sndbuf,"#logst#0#pass");
					send(client,sndbuf,strlen(sndbuf),0);

					/*�Ͽ����ӣ��ͷŽ��*/
					mysql_close(&mmysql);
				}
				else if( strcmp(opbuf,"#sdcfm")==0 )
				{
					sscanf(rcvbuf,"#sdcfm#%s",opbuf);
					reg(opbuf,client);

				}
				else if( strcmp(opbuf,"#regst")==0 )
				{
					sscanf(rcvbuf,"#regst#%s#%s#%s#%s",mailbox,pwd,nick,cfcode);
					checkreg(mailbox,nick,pwd,cfcode,client);

				}
				else
				{
					sprintf(sndbuf,"#error#%s:command not found",rcvbuf);
					send(client,sndbuf,strlen(sndbuf),0);
				}


				/*��һ�׶�ָ����������û�е�½���Ͽ�����*/
				if(logflag==0)
				{
					close(childr);
					close(childw);
					exit(0);
				}
				
				/*���߸����������˺�*/
				sprintf(sndbuf,"#setid#%s",qid);
				printl("child send %s in %d \n",sndbuf,childw);
				write(childw,sndbuf,strlen(sndbuf));
				printl("Child sent\n");
				
				FD_ZERO (&sset);

				/*��ʼѭ��*/
				while(logflag==1)
				{
					FD_ZERO (&sset);					
					FD_SET(client,&sset);
					maxf=client;
					FD_SET(childr,&sset);
					maxf=maxf>childr?maxf:childr;
					maxf++;
					printl("Child %s select \n",qid);
					if(select(maxf,&sset,NULL,NULL,NULL)<0)
					{
						printl("child select error\n");
						close(childr);
						close(childw);						
						exit(-1);
					}
					if(FD_ISSET(client,&sset))
					{
						printl("child select return caz socket\n");
						if ( (numbytes=recv(client,rcvbuf,MAXDATASIZE,0)) == -1 )
						{
							printl("recv error\n\n");
							close(childr);
							close(childw);
							return -1;
						}
						rcvbuf[numbytes]='\0';
						if(numbytes==0)
						{
							printl("child error return socket\n");
							close(client);
							logflag=0;
						}
						time(&now);
						printl("recv:%s at %s\n",rcvbuf,ctime(&now));			
						for(i=0;i<6;i++)
						      opbuf[i]=rcvbuf[i];
						opbuf[6]='\0';



						/*��ȡ�����б�*/
						if( strcmp(opbuf,"#getls")==0)
						{
							/*��������mysql������*/
							/*����*/
							if (!mysql_real_connect(&mmysql, sqlserver,sqluser, sqlpwd, sqldb, 0, NULL, 0)) 
							{
								printl("%s\n", mysql_error(&mmysql));
								exit(-1);
							}
							printl("mysql connected\n");

							/*��ѯ�����б�*/
							/*�����б������ݿ���ͳһ���û�ID_friend����*/
							sprintf(sqlbuf,"select * from %s_friend",qid);
							if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf))) 
							{
								printl("%s\n", mysql_error(&mmysql));
								exit(1);
							}
							sqlres=mysql_store_result(&mmysql);
							numfriends=mysql_num_rows(sqlres);

							/*�����д��sndbuf��*/
							sprintf(sndbuf,"#frlst#%d",numfriends);
							wrtsta=strlen(sndbuf);
							for(i=0;i<numfriends;i++)
							{
								sqlrow=mysql_fetch_row(sqlres);//��һ��
								sprintf(sndbuf+wrtsta,"#%s",sqlrow[0]);//�ں��ʵ�λ��д������
								wrtsta+=strlen(sqlrow[0])+1;
							}
							/*�ͷŽ�����Ͽ�����*/
							mysql_free_result(sqlres);
							send(client,sndbuf,strlen(sndbuf),0);				

							mysql_close(&mmysql);

						}
						else if(strcmp(opbuf,"#logot")==0)
						{
							logflag=0;
							printl("Child exit\n");
							close(client);
							close(childr);
							close(childw);
							exit(0);
						}
						else if(strcmp(opbuf,"#chatt")==0)
						{
							sscanf(rcvbuf,"#chatt#%[0-9]#%[^#]",oid,opbuf);
							printl("%s to %s :%s",qid,oid,opbuf);
							strcpy(sndbuf,rcvbuf);
							write(childw,sndbuf,strlen(sndbuf));
						}
						else if(strcmp(opbuf,"#aplfr")==0)// �������
						{
							strcpy(sndbuf,rcvbuf);
							write(childw,sndbuf,strlen(sndbuf));
						}
						else if(strcmp(opbuf,"#confr")==0)
						{
							strcpy(sndbuf,rcvbuf);
							write(childw,sndbuf,strlen(sndbuf));
						}
						else
						{
							sprintf(sndbuf,"#error#%s:command not found",rcvbuf);
							send(client,sndbuf,strlen(sndbuf),0);
						}
					}
					else if(FD_ISSET(childr,&sset))
					{
						printl("Child return caz pipe\n");
						pipelen=read(childr,rcvbuf,MAXDATASIZE);
						rcvbuf[pipelen]='\0';
						if(pipelen==0)
						{
							printl("child error return pipe\n");
							close(client);
							logflag=0;
						}
						printl("Child read %s via pipe %d.\n",rcvbuf,childr);
						for(i=0;i<6;i++)
						      opbuf[i]=rcvbuf[i];
						opbuf[6]='\0';
						if( strcmp(opbuf,"#chatf")==0)
						{
							strcpy(sndbuf,rcvbuf);
							send(client,sndbuf,strlen(sndbuf),0);
						}
						if( strcmp(opbuf,"#aplff")==0)
						{
							strcpy(sndbuf,rcvbuf);
							send(client,sndbuf,strlen(sndbuf),0);
						}
						if( strcmp(opbuf,"#conff")==0)
						{
							strcpy(sndbuf,rcvbuf);
							send(client,sndbuf,strlen(sndbuf),0);
						}

					}
				}
				close(client);
				exit(0);
			}
			else if(ppc>0)
			{
				/*�����̳������*/
				/*�ȹر���ͻ��˽������׽���*/
				close(client);
				/*�������½�һ��*/
				printl("parent add form\n");
				for(p1=head;p1->next;p1=p1->next);
				p1->next=(onusers *)malloc(sizeof(onusers));
				memset(p1->next,0,sizeof(onusers));
				p1->next->pid=ppc;
				p1->next->fw=fpipe[thispipe][0][1];
				p1->next->fr=fpipe[thispipe][1][0];
				close(fpipe[thispipe][1][1]);
				close(fpipe[thispipe][0][0]);
				p1->next->code=thispipe;
				p1->next->next=NULL;
				printl("form p:\n");
				for(p1=head;p1;p1=p1->next)
					printl("id=%s,code=%d",p1->id,p1->code);
				printl("\n");
			}
		}
		for(p1=head->next;p1;p1=p1->next)
		{
			if(FD_ISSET(p1->fr,&pset))
			{
				printl("parent select caz pipe\n");
				pipelen=read(p1->fr,rcvbuf,MAXDATASIZE);
				rcvbuf[pipelen]='\0';
				if(pipelen==0)
				{
					printl("parent error return pipe\n");
					sleep(10);
				}				
				for(i=0;i<6;i++)
				      opbuf[i]=rcvbuf[i];
				opbuf[6]='\0';
				if( strcmp(opbuf,"#setid")==0)
				{
					sscanf(rcvbuf,"#setid#%[0-9]",opbuf);
					strcpy(p1->id,opbuf);
					printl("parent resv:%s",rcvbuf);
				}
				else if( strcmp(opbuf,"#chatt")==0)
				{
					sscanf(rcvbuf,"#chatt#%[0-9]#%[^#]",oid,opbuf);
					sprintf(sndbuf,"#chatf#%s#%s#%s",p1->id,oid,opbuf);
					printl("oid=%s,opbuf=%s,sndbuf=%s\n",oid,opbuf,sndbuf);
					for(p2=head->next;p2;p2=p2->next)
					{
						if(strcmp(p2->id,oid)==0)
						{	
							printl("pipe found,code=%d\n",p2->code);
							write(p2->fw,sndbuf,strlen(sndbuf));
							write(p1->fw,sndbuf,strlen(sndbuf));
						}
					}
				}
				else if( strcmp(opbuf,"#aplfr")==0)
				{
					printl("parent resv:%s\n",rcvbuf);
					sscanf(rcvbuf,"#aplfr#%[0-9]",oid);
					sprintf(sndbuf,"#aplff#%s",p1->id);
					printl("oid=%s,opbuf=%s,sndbuf=%s\n",oid,opbuf,sndbuf);
					for(p2=head->next;p2;p2=p2->next)
					{
						if(strcmp(p2->id,oid)==0)
						{	
							printl("pipe found,code=%d\n",p2->code);
							write(p2->fw,sndbuf,strlen(sndbuf));
						}
					}
				}
				else if( strcmp(opbuf,"#confr")==0)
				{
					printl("parent resv:%s\n",rcvbuf);
					sscanf(rcvbuf,"#confr#%[0-9]",oid);
					sprintf(sndbuf,"#conff#%s",p1->id);
					printl("oid=%s,opbuf=%s,sndbuf=%s\n",oid,opbuf,sndbuf);


					if (!mysql_real_connect(&mmysql, sqlserver,sqluser, sqlpwd, sqldb, 0, NULL, 0)) 
					{
						printl( "mysql:%s\n", mysql_error(&mmysql));
						exit(1);
					}

					//printf("connected\n");	
					sprintf(sqlbuf,"insert into %s_friend values(\'%s\')",oid,p1->id);
					if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf))) 
					{
						printl( "%s\n", mysql_error(&mmysql));
						exit(1);
					}
					sqlres=mysql_store_result(&mmysql);
					mysql_free_result(sqlres);
					mysql_close(&mmysql);									
					// �ڱ��%s_friend�����һ��%s,��һ��%s���ַ���oid���ڶ����ַ�����p1->id



					for(p2=head->next;p2;p2=p2->next)
					{
						if(strcmp(p2->id,oid)==0)
						{	
							printl("pipe found,code=%d\n",p2->code);
							write(p2->fw,sndbuf,strlen(sndbuf));
						}
					}

				}				



			}
		}
	}
}
int reg(char *addr,int client)
{
	int mail;
	struct sockaddr_in m_addr;
	char mailbuf[500],ccode[33];
	int numbytes;
	memset(&m_addr,0,sizeof(m_addr));
	m_addr.sin_family=AF_INET;//Address family Internet
	m_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	m_addr.sin_port=htons(25);
	int flag;
	long times;
	char sndbuf[MAXDATASIZE];


	/*�����￪ʼ��*/
	/*���ӷ�����*/
	if (!mysql_real_connect(&mmysql, sqlserver,sqluser, sqlpwd, sqldb, 0, NULL, 0)) 
	{
		sprintf(sndbuf,"#conff#error_mysql_opera");
		send(client,sndbuf,strlen(sndbuf),0);
		printl("%s\n", mysql_error(&mmysql));
		exit(1);
	}

	sprintf(sqlbuf,"select unix_timestamp(now())-unix_timestamp(time) from confirm where email=\'%s\'",addr);
	if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
	{		
		sprintf(sndbuf,"#conff#error_mysql_opera");
		send(client,sndbuf,strlen(sndbuf),0);
		printl("%s\n", mysql_error(&mmysql));
		exit(1);
	}

	sqlres=mysql_store_result(&mmysql);//��ý��
	flag=mysql_num_rows(sqlres);//�˺������ر�������
	mysql_free_result(sqlres);
	/*	���ڣ�����Ƿ����������;
		��û�����˳�;
		�����ˣ���config����е�config�����ַ�����; */
	if(flag==1)
	{
		sqlrow=mysql_fetch_row(sqlres);//SQLrow��Ϊ��һ��
		strcpy(sqlbuf,sqlrow[0]);
		sscanf(sqlbuf,"%d",&times);
		if(times<=180)
		{
			sprintf(sndbuf,"#conff#less_than_three_minutes");
			send(client,sndbuf,strlen(sndbuf),0);
			mysql_close(&mmysql);
		}
		else
		{
			sprintf(sqlbuf,"select confirm from confirm where email=\'%s\'",addr);
			if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
			{		
				sprintf(sndbuf,"#conff#error_mysql_opera");
				send(client,sndbuf,strlen(sndbuf),0);
				printl("%s\n", mysql_error(&mmysql));
				exit(1);
			}
			sqlres=mysql_store_result(&mmysql);//��ý��
			sqlrow=mysql_fetch_row(sqlres);
			strcpy(ccode,sqlrow[0]);
		}
	}
	if(flag==0)
	{
		sprintf(sqlbuf,"insert into confirm values(\'%s\',md5(now()),NULL)",addr);
		if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
		{		
			sprintf(sndbuf,"#conff#error_mysql_opera");
			send(client,sndbuf,strlen(sndbuf),0);
			printl("%s\n", mysql_error(&mmysql));
			exit(1);
		}
	
	
		sprintf(sqlbuf,"select time from confirm where email=\'%s\'",addr);
		if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
		{		
			sprintf(sndbuf,"#conff#error_mysql_opera");
			send(client,sndbuf,strlen(sndbuf),0);
			printl("%s\n", mysql_error(&mmysql));
			exit(1);
		}

		sqlres=mysql_store_result(&mmysql);//��ý��
		flag=mysql_num_rows(sqlres);//�˺������ر�������
		mysql_free_result(sqlres);
		if(flag!=1)
		{
			sprintf(sndbuf,"#conff#error_mysql_opera");
			send(client,sndbuf,strlen(sndbuf),0);
			printl("insert error: %s\n", mysql_error(&mmysql));
			exit(1);
		}
		else
		{
			sprintf(sqlbuf,"select confirm from confirm where email=\'%s\'",addr);
			if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
			{		
				sprintf(sndbuf,"#conff#error_mysql_opera");
				send(client,sndbuf,strlen(sndbuf),0);
				printl("%s\n", mysql_error(&mmysql));
				exit(1);
			}
			sqlres=mysql_store_result(&mmysql);//��ý��
			sqlrow=mysql_fetch_row(sqlres);
			strcpy(ccode,sqlrow[0]);
			mysql_free_result(sqlres);
		}
		mysql_close(&mmysql);


	}



	
	/*�����Ϸ�����*/
	mail=socket(AF_INET,SOCK_STREAM,0);
	if(mail==-1)
	{
		sprintf(sndbuf,"#conff#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("socket");
		return -1;
	}
	if( connect(mail,(struct sockaddr *)&m_addr,sizeof(struct sockaddr)) == -1 )
	{
		sprintf(sndbuf,"#conff#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		close(mail);
		printf("connect");
		return -1;
	}
	



	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#conff#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);


	/*����ehlo localhost*/
	sprintf(mailbuf,"ehlo localhost\n");
	printf("send: %s\n",mailbuf);
	send(mail,mailbuf,strlen(mailbuf),0);

	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#conff#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	printf("recved:%d\n",numbytes);
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);//�յ��ܶණ��
	
	sprintf(mailbuf,"mail from: \"auto-noreply\"<auto-noreply@[184.82.62.43]>\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s",mailbuf);
	printf("\n");

	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#conff#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);               //chucuo

	sprintf(mailbuf,"rcpt to: <%s>\n",addr);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s",mailbuf);
	printf("\n");

	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#conff#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);		
		printf("recv");
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);               //chucuo

	sprintf(mailbuf,"data\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s",mailbuf);
	printf("\n");

	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		printf("recv");
		sprintf(sndbuf,"#conff#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);

	sprintf(mailbuf, "from: auto-noreply@[184.82.62.43]\nto: %s\nsubject: Register letter\n",addr);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);
	printf("\n");	


	sprintf(mailbuf,"Dear %s:\n",addr);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);


	sprintf(mailbuf,"Thank you for registering Sky Talking. We will try our best to supply you the most considerate service. \n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);

	sprintf(mailbuf,"Please copy the code to the program to confirm your registration.\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);

	sprintf(mailbuf,"%s\n",ccode);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);

	sprintf(mailbuf,"                                                          Sincerely Sky Talking \n\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);	

	sprintf(mailbuf,".\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);



	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		printl("recv");
		sprintf(sndbuf,"#conff#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);

	sprintf(mailbuf,"quit\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);
	printf("\n");
	sprintf(sndbuf,"#confs");
	send(client,sndbuf,strlen(sndbuf),0);
	return 0;
}
int checkreg(char *addr,char *nick,char *pwd ,char *ccode,int client)
{
	int mail;
	struct sockaddr_in m_addr;
	char mailbuf[500];
	int numbytes;
	memset(&m_addr,0,sizeof(m_addr));
	m_addr.sin_family=AF_INET;//Address family Internet
	m_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	m_addr.sin_port=htons(25);
	char sndbuf[MAXDATASIZE];
	int i;


	//���confirm����ַ����֤���Ƿ�ƥ��
	//����ƥ�䣬���û�����#rgstf#not_match
	//��ƥ�䣬��user������ҵ�10���ϵ�δ��ʹ�õ�id����user������һ������û�����#rgsts#�û�id��user�Ľṹ�Ѿ�����
	sprintf(sqlbuf,"select * from confirm where email=\'%s\' and confirm=\'%s\'",addr,ccode);
	if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
	{		
		sprintf(sndbuf,"#rgstf#error_mysql_error");
		send(client,sndbuf,strlen(sndbuf),0);
		printl("%s\n", mysql_error(&mmysql));
		exit(1);
	}
	sqlres=mysql_store_result(&mmysql);//��ý��
	sqlrow=mysql_fetch_row(sqlres);
	mysql_free_result(sqlres);	
	if(sqlrow==0)
	{
		printl("#rgstf#not_match");
		mysql_close(&mmysql);
		exit(1);
	}
	else
	{
		sprintf(sqlbuf,"delete from confirm where email=\'%s\' and confirm=\'%s\'",addr,ccode);
		if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
		{		
			sprintf(sndbuf,"#rgstf#error_mysql_error");
			send(client,sndbuf,strlen(sndbuf),0);
			printl("%s\n", mysql_error(&mmysql));
			exit(1);
		}
	}
	for(i=10;;i++)
	{
		sprintf(sqlbuf,"select * from user where id=\'%d\'",i);
		if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
		{		
			sprintf(sndbuf,"#rgstf#error_mysql_error");
			send(client,sndbuf,strlen(sndbuf),0);
			printl("%s\n", mysql_error(&mmysql));
			exit(1);
		}
		sqlres=mysql_store_result(&mmysql);//��ý��

		if(mysql_num_rows(sqlres)==1)
		{
			mysql_free_result(sqlres);
			continue;
		}	      
		else
		{
			sprintf(sqlbuf,"insert into user values(\'%d\',now(),md5(\'%s\'),\'%s\',\'%s\',\'NULL\')",i,pwd,addr,nick);
			if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
			{		
				sprintf(sndbuf,"#rgstf#error_mysql_error");
				send(client,sndbuf,strlen(sndbuf),0);
				printl("%s\n", mysql_error(&mmysql));
				exit(1);
			}
			break;
		}
	}
	sprintf(sndbuf,"#rgsts#%d",i);
	send(client,sndbuf,strlen(sndbuf),0);
	mysql_close(&mmysql);	

	
	/*�����Ϸ�����*/
	mail=socket(AF_INET,SOCK_STREAM,0);
	if(mail==-1)
	{
		sprintf(sndbuf,"#rgstf#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("socket");
		return -1;
	}
	if( connect(mail,(struct sockaddr *)&m_addr,sizeof(struct sockaddr)) == -1 )
	{
		sprintf(sndbuf,"#rgstf#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		close(mail);
		printf("connect");
		return -1;
	}
	



	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#rgstf#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);


	/*����ehlo localhost*/
	sprintf(mailbuf,"ehlo localhost\n");
	printf("send: %s\n",mailbuf);
	send(mail,mailbuf,strlen(mailbuf),0);

	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#rgstf#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	printf("recved:%d\n",numbytes);
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);//�յ��ܶණ��
	
	sprintf(mailbuf,"mail from: \"auto-noreply\"<auto-noreply@[184.82.62.43]>\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s",mailbuf);
	printf("\n");

	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#rgstf#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);               //chucuo

	sprintf(mailbuf,"rcpt to: <%s>\n",addr);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s",mailbuf);
	printf("\n");

	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#rgstf#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);               //chucuo

	sprintf(mailbuf,"data\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s",mailbuf);
	printf("\n");

	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#rgstf#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);

	sprintf(mailbuf, "from: auto-noreply@[184.82.62.43]\nto: %s\nsubject: Register success\n",addr);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);
	printf("\n");	





	sprintf(mailbuf,"Dear %s:\n",addr);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);


	sprintf(mailbuf,"Thank you for registering Sky Talking. We will try our best to supply you the most considerate service. \n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);

	sprintf(mailbuf,"Your information of the registration is as follows:\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);

	sprintf(mailbuf,"Your id:%s\n",i);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);

	sprintf(mailbuf,"Your nick name :%s\n",nick);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);

	sprintf(mailbuf,"Your password :%s\n",pwd);
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);

	sprintf(mailbuf,"                                                          Sincerely Sky Talking \n\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);	

	sprintf(mailbuf,".\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);



	if ( (numbytes=recv(mail,mailbuf,500,0)) == -1 )
	{
		sprintf(sndbuf,"#rgstf#error_mail_send");
		send(client,sndbuf,strlen(sndbuf),0);
		printf("recv");
		return -1;
	}
	mailbuf[numbytes]='\0';
	printf("%s",mailbuf);

	sprintf(mailbuf,"quit\n");
	send(mail,mailbuf,strlen(mailbuf),0);
	printf("%s\n",mailbuf);
	printf("\n");
	return 0;
}




