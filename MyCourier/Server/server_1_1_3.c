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
/*维护当前在线用户列表 Maintains current online usrers*/
typedef struct _onusers
{
	
	char id[11];//用户id     User id
	pid_t pid;//子进程pid    Child process pid
	int fw;//写的管道        Write pipe
	int fr;//读的管道        Read pipe
	int code;
	struct _onusers *next;
} onusers;
onusers *head,*p1,*p2;
int fpipe[50][2][2];//最多允许50位用户连接         Maximum 50 users online
int pipelen;//管道中的文件长度                     File lenth for pipe
int reg(char *addr,int client);
int checkreg(char *addr,char *nick,char *pwd ,char *ccode,int client);
/*mysql使用的参数*/
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
int wrtsta;		//写入字符串的当前位          Current index in the write string

/***自定义函数，每次打开文件，写入，关闭***//*Every time, open a file, write in it, and the close it*/
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
	else if(arg==SIGCHLD)//处理结束的子进程 handle dead child process
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
	int server;//socket()函数的返回值                      returned value of socket()
	struct sockaddr_in my_addr;//服务器地址的结构体         
	pid_t pc,ppc=1;//进程pid，创建守护进程与子进程时使用
	int i,fd;
	FILE *fp;
	time_t now;
	char rcvbuf[MAXDATASIZE];//用于接收的字符串            string to be used to recieve
	char sndbuf[MAXDATASIZE];//用于发送的字符串            string to be used to send
	char opbuf[MAXDATASIZE];//用于操作的字符串             Intermediate string
	int numbytes;//接收到的字节数
	char qid[11];//用户帐号                                My id   
	char oid[11];//对方账号                                Other's id
	int qpwd[21];//密码                                    My password
	int logflag=0;
	int numfriends;//好友数目                              Number of friends
	int thispipe;//当前处理的管道                          Current pipe
	fd_set pset;//父进程I/O复用文件集                      For select() (parent process)
	fd_set sset;//子进程I/O复用文件集                      For select() (child process)
	int childw,childr;//子进程的读写管道
	int maxf;//最大文件描述
	char nick[31]={0};
	char cfcode[33]={0};
	char pwd[21]={0};
	char mailbox[51]={0};
	
	



	


	/*定义socketaddr_in结构(服务器端套接字)*/
	memset(&my_addr,0,sizeof(my_addr));//先全部置零
	my_addr.sin_family=AF_INET;//Address family Internet
	my_addr.sin_addr.s_addr=htonl(INADDR_ANY);//使用当前服务器配置的所有ip地址
	my_addr.sin_port=htons(MYPORT);//使用定义的端口通信


	/*定义客户端套接字*/
 	int client;
	struct sockaddr_in from_addr;
	int from_len=sizeof(from_addr);
	memset(&from_addr,0,sizeof(from_addr));


	/*初始化链表*/
	head=(onusers *)malloc(sizeof(onusers));
	sprintf(head->id,"0");
	head->pid=0;
	head->fw=0;
	head->fr=0;
	head->next=NULL;
	head->code=-1;

	/*打开一个文件，准备写入*/
	time(&now);
	printl("Program start at %s",ctime(&now));

	/*调用socket()并检查*/
	server=socket(AF_INET,SOCK_STREAM,0);
	if(server==-1)
	{
		printl("Socket error\n\n");
		return 0;
	}

	/*调用bind()*/
	if( bind(server,(struct sockaddr*)&my_addr,sizeof(my_addr)) != 0 )
	{
		printl("Bind error\n\n");
		return 0;
	}

	/*调用listen()*/
	if( listen(server,10) != 0 )
	{
		printl("Listen error\n\n");
		return 0;
	}
  
	if(ppc>0)
		printl("Server started with port %d\n",MYPORT);


	/* 创建守护进程，结束父进程*/
	pc = fork(); //第一步 　　
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
	setsid(); //第二步
	chdir("/"); //第三步
	umask(0); //第四步
	for(i=0;i<3;i++) //第五步	
	      close(i);
	signal(SIGTERM, sigterm_handler); 
	signal(SIGHUP, sigterm_handler); 
	signal(SIGCHLD,sigterm_handler);

	/*初始化MySQL库*/
	mysql_init(&mmysql);
	/*if( mysql == NULL);
	{
		printl("MySQL init error\n");
		exit(-1);
	}*/	

	/*继续socket编程*/
	/*开始无限循环，接受用户请求*/

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
			/*找一个空闲的管道*/
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
			/*创建子进程，因为这是并发服务器*/
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
				/*子进程编程*/
				close(server);//关闭监听端口
				/*关闭不属于自己的管道*/
				for(p1=head->next;p1;p1=p1->next)
				{
					close(p1->fw);
					close(p1->fr);
				}
				/*定义与父进程交流的管道*/
				childw=fpipe[thispipe][1][1];				
				childr=fpipe[thispipe][0][0];
				close(fpipe[thispipe][0][1]);
				close(fpipe[thispipe][1][0]);
				/*清除链表*/
				for(p1=head;p1;)
				{
					p2=p1->next;
					free(p1);
					p1=p2;
				}


				/*先接受用户指令，这可以是登陆，注册，忘记密码*/
				if ( (numbytes=recv(client,rcvbuf,MAXDATASIZE,0)) == -1 )
				{
					printl("recv error\n\n");
					return -1;
				}
				rcvbuf[numbytes]='\0';
				time(&now);
				printl("recv:%s at %s\n",rcvbuf,ctime(&now));

				/*客户端发送消息的前六位统一为指令码，所以拷贝走前六位，进行判断*/
				for(i=0;i<6;i++)
				      opbuf[i]=rcvbuf[i];
				opbuf[6]='\0';


				/*登录指令*/
				if ( strcmp(opbuf,"#login")==0 )
				{
					/*读取用户发来的用户名与密码*/
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
					
					
					/*以下连接mysql服务器*/
					/*连接*/
					if (!mysql_real_connect(&mmysql, sqlserver,sqluser, sqlpwd, sqldb, 0, NULL, 0)) 
					{
						printl("MySQL connect:");
						printl("%s\n", mysql_error(&mmysql));
						exit(-1);
					}

					/*在MySQL中查找是否匹配*/				
					sprintf(sqlbuf,"select * from user where id='%s'&&password=md5('%s')",qid,qpwd);
					if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf))) 
					{
						printl("%s\n", mysql_error(&mmysql));
						exit(-1);
					}
					sqlres=mysql_store_result(&mmysql);

					/*如果查到一行,说明匹配*/
					check_statues=mysql_num_rows(sqlres);
					mysql_free_result(sqlres);



					/*向客户端发送登陆成功的消息，并标记*/
					if(check_statues==1)
					{
	      					sprintf(sndbuf,"#logst#1");
						logflag=1;
					}
					/*提醒用户登录失败*/
					else
	      					sprintf(sndbuf,"#logst#0#pass");
					send(client,sndbuf,strlen(sndbuf),0);

					/*断开连接，释放结果*/
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


				/*第一阶段指令结束，如果没有登陆，断开连接*/
				if(logflag==0)
				{
					close(childr);
					close(childw);
					exit(0);
				}
				
				/*告诉父进程它的账号*/
				sprintf(sndbuf,"#setid#%s",qid);
				printl("child send %s in %d \n",sndbuf,childw);
				write(childw,sndbuf,strlen(sndbuf));
				printl("Child sent\n");
				
				FD_ZERO (&sset);

				/*开始循环*/
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



						/*获取好友列表*/
						if( strcmp(opbuf,"#getls")==0)
						{
							/*以下连接mysql服务器*/
							/*连接*/
							if (!mysql_real_connect(&mmysql, sqlserver,sqluser, sqlpwd, sqldb, 0, NULL, 0)) 
							{
								printl("%s\n", mysql_error(&mmysql));
								exit(-1);
							}
							printl("mysql connected\n");

							/*查询好友列表*/
							/*好友列表在数据库中统一以用户ID_friend命名*/
							sprintf(sqlbuf,"select * from %s_friend",qid);
							if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf))) 
							{
								printl("%s\n", mysql_error(&mmysql));
								exit(1);
							}
							sqlres=mysql_store_result(&mmysql);
							numfriends=mysql_num_rows(sqlres);

							/*将结果写在sndbuf里*/
							sprintf(sndbuf,"#frlst#%d",numfriends);
							wrtsta=strlen(sndbuf);
							for(i=0;i<numfriends;i++)
							{
								sqlrow=mysql_fetch_row(sqlres);//下一行
								sprintf(sndbuf+wrtsta,"#%s",sqlrow[0]);//在合适的位置写入数据
								wrtsta+=strlen(sqlrow[0])+1;
							}
							/*释放结果，断开连接*/
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
						else if(strcmp(opbuf,"#aplfr")==0)// 申请好友
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
				/*父进程程序设计*/
				/*先关闭与客户端交流的套接字*/
				close(client);
				/*链表中新建一项*/
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
					// 在表格%s_friend中添加一项%s,第一个%s是字符串oid，第二个字符串是p1->id



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


	/*从这里开始用*/
	/*连接服务器*/
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

	sqlres=mysql_store_result(&mmysql);//获得结果
	flag=mysql_num_rows(sqlres);//此函数返回表格的行数
	mysql_free_result(sqlres);
	/*	若在，检查是否过了三分钟;
		若没过，退出;
		若过了，把config表格中的config读到字符串中; */
	if(flag==1)
	{
		sqlrow=mysql_fetch_row(sqlres);//SQLrow变为下一行
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
			sqlres=mysql_store_result(&mmysql);//获得结果
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

		sqlres=mysql_store_result(&mmysql);//获得结果
		flag=mysql_num_rows(sqlres);//此函数返回表格的行数
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
			sqlres=mysql_store_result(&mmysql);//获得结果
			sqlrow=mysql_fetch_row(sqlres);
			strcpy(ccode,sqlrow[0]);
			mysql_free_result(sqlres);
		}
		mysql_close(&mmysql);


	}



	
	/*连接上服务器*/
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


	/*输入ehlo localhost*/
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
	printf("%s",mailbuf);//收到很多东西
	
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


	//检查confirm表格地址与验证码是否匹配
	//若不匹配，给用户发送#rgstf#not_match
	//若匹配，在user表格中找到10以上的未被使用的id，在user表格添加一项，并给用户发送#rgsts#用户id，user的结构已经给出
	sprintf(sqlbuf,"select * from confirm where email=\'%s\' and confirm=\'%s\'",addr,ccode);
	if (mysql_real_query(&mmysql, sqlbuf,strlen(sqlbuf)))
	{		
		sprintf(sndbuf,"#rgstf#error_mysql_error");
		send(client,sndbuf,strlen(sndbuf),0);
		printl("%s\n", mysql_error(&mmysql));
		exit(1);
	}
	sqlres=mysql_store_result(&mmysql);//获得结果
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
		sqlres=mysql_store_result(&mmysql);//获得结果

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

	
	/*连接上服务器*/
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


	/*输入ehlo localhost*/
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
	printf("%s",mailbuf);//收到很多东西
	
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




