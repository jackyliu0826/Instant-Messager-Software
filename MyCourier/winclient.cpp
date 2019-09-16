#include "resource.h"
#pragma comment(lib,"WS2_32")
#include <winsock2.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define bzero(a,b) (memset((a),0,(b)))
#define SVR_ADDR "184.82.62.43"
#define SVR_PORT 26588
#define MAXDATASIZE 250
#define MAX_LOADSTRING 100
#define WM_SOCKET_N (WM_USER+1)


// Global Variables:
HINSTANCE hInst;				//应用程序句柄
TCHAR szMainTitle[MAX_LOADSTRING];		//主窗口标题
TCHAR szAppName[MAX_LOADSTRING];		//应用程序名字，主窗口类
TCHAR qid[11];					//登录时的用户ID
TCHAR qpwd[21];					//登录时的密码
BOOL logflag=FALSE;				//标示是否登录
unsigned int i;
SOCKET client;
sockaddr_in my_addr;
LPWSADATA wsaData=(LPWSADATA)malloc(sizeof(struct WSAData));//微软socket必备
char sndbuf[MAXDATASIZE];			//发送缓冲区
char rcvbuf[MAXDATASIZE];			//接受缓冲区
char msbbuf[MAXDATASIZE];			//提示框缓冲区
char opbuf[MAXDATASIZE];			//操作缓冲区
int numbytes;					//接受时显示字节数
int cxScreen,cyScreen;				//显示屏分辨率
int iFriend;					//好友个数
char aplid[11];					//
typedef struct _friends
{
	char id[11];
	char nick[31];
	BOOL butc;				//标记按钮是否被创建
	HWND but;				//按钮的窗口句柄
	BOOL chatc;				//标记聊天窗口是否被创建
	HWND chat;				//聊天窗口句柄
	long ctime;				//时间
	char op[50][100];			//聊天内容
	char fid[50][11];			//聊天记录的发起人
	int num;				//聊天纪录条数
	HWND hinput;				//录入框
	HWND hsend;				//发送按钮
	BOOL inputc;
	BOOL sendc;
	char tempbuf[100];			//缓存聊天结果
} friends;
friends flist[50];		

/*---Foward declarations of functions included in this code module:
-----LRESULT类型的都是窗口过程
-----函数名不以Proc结尾的是对话框
-----*/
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);	//主窗口
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);	//关于对话框
LRESULT CALLBACK	Login(HWND, UINT, WPARAM, LPARAM);	//登录对话框
LRESULT CALLBACK	LogsProc(HWND, UINT, WPARAM, LPARAM);	//登录状态窗口
LRESULT CALLBACK	Register(HWND, UINT, WPARAM, LPARAM);	//注册对话框
LRESULT CALLBACK	ChatProc(HWND, UINT, WPARAM, LPARAM);	//聊天窗口
LRESULT CALLBACK	AddFriend(HWND, UINT, WPARAM, LPARAM);	//添加好友窗口
LRESULT CALLBACK	ConAdd(HWND, UINT, WPARAM, LPARAM);	//添加好友窗口
void			myerror(char *str);



int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     szCmdLine,
                     int       iCmdShow)
{
	MSG msg;					//消息处理用
	HWND hwMain,hwLogs;				//分别是主窗口和登录状态窗口
	WNDCLASS wndclass1,wndclass2,wndclass3;		//注册窗口用
	int idlen=0;					//id的长度
	hInst=hInstance;

	LoadString(hInst,IDS_APP_TITLE ,szMainTitle,MAX_LOADSTRING);	//从资源中读取主窗口标题
	LoadString(hInstance,IDS_APP_TITLE,szAppName,MAX_LOADSTRING);		//从资源中读取应用程序名称
	
	
	/*先注册主窗口*/
	wndclass1.style		=CS_HREDRAW | CS_VREDRAW;//水平尺寸、竖直尺寸改变时，重绘窗口
	wndclass1.lpfnWndProc	=(WNDPROC)WndProc;//设置窗口过程函数
	wndclass1.cbClsExtra	=0;
	wndclass1.cbWndExtra	=0;//这两个默认置0
	wndclass1.hInstance	=hInst;//应用程序实例句柄，默认
	wndclass1.hIcon		=LoadIcon(hInstance,(LPCTSTR)IDI_MAIN);//读图标
	wndclass1.hCursor	=LoadCursor(hInstance,(LPCTSTR)IDC_CUR1);//读鼠标
	wndclass1.hbrBackground	=(HBRUSH)GetStockObject(WHITE_BRUSH);//客户区刷为白色
	wndclass1.lpszMenuName	=(LPCSTR)IDR_MENU_MAIN;//菜单
	wndclass1.lpszClassName	=szAppName;//窗口类型名字
	RegisterClass(&wndclass1);

	/*注册登录信息显示窗口*/
	wndclass2.style		= CS_HREDRAW | CS_VREDRAW;
	wndclass2.lpfnWndProc	= (WNDPROC)LogsProc;
	wndclass2.cbClsExtra	= 0;
	wndclass2.cbWndExtra	= 0;
	wndclass2.hInstance	= hInstance;
	wndclass2.hIcon		= LoadIcon(hInstance,(LPCTSTR)IDI_MAIN);
	wndclass2.hCursor	= LoadCursor(hInstance,(LPCTSTR)IDC_CUR1);
	wndclass2.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass2.lpszMenuName	= NULL;
	wndclass2.lpszClassName	= TEXT ("LogStatue");
	RegisterClass(&wndclass2);



	/*注册聊天窗口类*/
	wndclass3.style		= CS_HREDRAW | CS_VREDRAW;
	wndclass3.lpfnWndProc	= (WNDPROC)ChatProc;
	wndclass3.cbClsExtra	= 0;
	wndclass3.cbWndExtra	= 0;
	wndclass3.hInstance	= hInstance;
	wndclass3.hIcon		= LoadIcon(hInstance,(LPCTSTR)IDI_MAIN);
	wndclass3.hCursor	= LoadCursor(hInstance,(LPCTSTR)IDC_CUR1);
	wndclass3.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass3.lpszMenuName	= NULL;
	wndclass3.lpszClassName	= TEXT ("ChatClass");
	RegisterClass(&wndclass3);	





	/*Socket准备工作*/
	int wsaret=WSAStartup(WINSOCK_VERSION,wsaData);
	if(wsaret)
	{
		myerror("WSAStartup");
		return -1;
	}
	
	bzero(&my_addr,sizeof(my_addr));
	my_addr.sin_family=AF_INET;//Address family Internet
	my_addr.sin_addr.s_addr=inet_addr(SVR_ADDR);//使用当前服务器配置的所有ip地址
	my_addr.sin_port=htons(SVR_PORT);//使用定义的端口通信



	/*获取显示屏分辨率*/
	cxScreen = GetSystemMetrics (SM_CXSCREEN) ;
	cyScreen = GetSystemMetrics (SM_CYSCREEN) ;
	
	/*直至登录成功为止*/
	while(!logflag)
	{
		DialogBox(hInst, (LPCTSTR)IDD_LOGIN, NULL, (DLGPROC)Login);
		hwLogs = CreateWindow(TEXT ("LogStatue"), TEXT ("正在登录"), WS_OVERLAPPEDWINDOW,
		  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
		ShowWindow(hwLogs, iCmdShow);
		UpdateWindow(hwLogs);
		DestroyWindow(hwLogs);
	}
	/*获取好友列表*/
	sprintf(sndbuf,"#getls");
	send(client,sndbuf,strlen(sndbuf),0);
	if ( (numbytes=recv(client,rcvbuf,MAXDATASIZE,0)) == -1 )
	{
		myerror("recv");
		return -1;
	}
	rcvbuf[numbytes]='\0';
	for(i=0;i<6;i++)
	      opbuf[i]=rcvbuf[i];
	opbuf[6]='\0';

	if ( strcmp(opbuf,"#frlst")!=0 )
	{
		myerror("getlist");
		sprintf(sndbuf,"#logot");
		send(client,sndbuf,strlen(sndbuf),0);
		Sleep(3000);
		return -1;
	}
	sscanf(rcvbuf,"#frlst#%d",&iFriend);		//读取好友数目
	sprintf(opbuf,"%d",iFriend);
	idlen=8+strlen(opbuf);
	for(i=0;i<iFriend;i++)
	{
		sscanf(rcvbuf+idlen,"%[0-9]",flist[i].id);
		flist[i].nick[0]='\0';
		flist[i].butc=FALSE;			//没有创建按钮窗口
		flist[i].chatc=FALSE;
		flist[i].inputc=FALSE;
		flist[i].sendc=FALSE;
		flist[i].num=0;
		flist[i].chat=0;			//初始化窗口句柄
		flist[i].tempbuf[0]='\0';
		idlen=idlen+strlen(flist[i].id)+1;
	}



	
	
	hwMain=CreateWindow(szAppName,			//窗口类名称
			    szMainTitle,		//窗口标题
			    WS_OVERLAPPEDWINDOW| WS_VSCROLL,	//窗口风格
			    128,			//初始x坐标
			    0,				//初始y坐标
			    256,			//初始x方向尺寸
			    512,			//初始y方向尺寸
			    NULL,			//父窗口句柄
			    NULL,			//窗口菜单句柄
			    hInstance,			//程序实例句柄
			    NULL);			//创建参数



	ShowWindow(hwMain,iCmdShow);
	UpdateWindow(hwMain);


	while(GetMessage(&msg,NULL,0,0))		//开始处理消息
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

/*主窗口过程*/
LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;				//有效矩形
	static int  cxChar, cxCaps, cyChar,cxClient,cyClient;
	TEXTMETRIC  tm ;
	int wmId, wmEvent;
	SCROLLINFO  si ;			//滚动条信息
	int i, x, y, iVertPos,iPaintBeg, iPaintEnd ,j;
	WORD wEvent,wError;			//处理socket信息用
	char fid[11],oid[11];			//源id，目标id
	int idlen;


	switch(message)
	{
		case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hwnd, (DLGPROC)About);
				   break;
				case IDM_ADDFRIEND:
				   DialogBox(hInst, (LPCTSTR)IDD_ADDFRIEND, hwnd, (DLGPROC)AddFriend);
				   break;
				case IDM_EXIT:
				   DestroyWindow(hwnd);
				   break;
				default:
				{
					for(i=0;i<iFriend;i++)
					{
						if(wmId==i&&flist[i].chatc==FALSE)
						{
							flist[i].chat=CreateWindow(TEXT ("ChatClass"),flist[i].id,WS_OVERLAPPEDWINDOW| WS_VSCROLL,CW_USEDEFAULT,CW_USEDEFAULT,60*cxChar,62*cxChar,NULL,NULL,hInst,NULL);	
							ShowWindow(flist[i].chat,SW_SHOWNORMAL);
							UpdateWindow(flist[i].chat);
							flist[i].chatc=TRUE;
							break;
						}
					}
					return DefWindowProc(hwnd, message, wParam, lParam);
				}
			}
			break;
		case WM_CREATE:
			hdc = GetDC (hwnd) ;
          
			GetTextMetrics (hdc, &tm) ;
			cxChar = tm.tmAveCharWidth ;
			cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2 ;
			cyChar = tm.tmHeight + tm.tmExternalLeading ;
          
			ReleaseDC (hwnd, hdc) ;
			//设置非阻塞;并且在适当的时候发消息
			if(SOCKET_ERROR == WSAAsyncSelect(client,hwnd,WM_SOCKET_N,FD_READ))
			{
				sprintf(opbuf,"WSASelect #%i\n",WSAGetLastError());
				myerror(opbuf);
				closesocket(client);
				WSACleanup();
				exit(-1);
			}

			break;
		case WM_SIZE:
			cxClient = LOWORD (lParam) ;
			cyClient = HIWORD (lParam) ;

			// Set vertical scroll bar range and page size

			si.cbSize = sizeof (si) ;
			si.fMask  = SIF_RANGE | SIF_PAGE|SIF_DISABLENOSCROLL ;
			si.nMin   = 0 ;
			si.nMax   = iFriend - 1 ;
			si.nPage  = cyClient / cyChar/2 ;
			SetScrollInfo (hwnd, SB_VERT, &si, TRUE) ;
		case WM_VSCROLL:
			       // Get all the vertial scroll bar information

			si.cbSize = sizeof (si) ;
			si.fMask  = SIF_ALL ;
			GetScrollInfo (hwnd, SB_VERT, &si) ;

			// Save the position for comparison later on

			iVertPos = si.nPos ;

			switch (LOWORD (wParam))
			{
			case SB_TOP:
				si.nPos = si.nMin ;
				break ;
               
			case SB_BOTTOM:
				si.nPos = si.nMax ;
				break ;
               
			case SB_LINEUP:
				si.nPos -= 1 ;
				break ;
               
			case SB_LINEDOWN:
				si.nPos += 1 ;
				break ;
               
			case SB_PAGEUP:
				si.nPos -= si.nPage ;
				break ;
               
			case SB_PAGEDOWN:
				si.nPos += si.nPage ;
				break ;
               
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos ;
				break ;
               
			default:
				break ;         
			}
			// Set the position and then retrieve it;  Due to adjustments
			//   by Windows it may not be the same as the value set.
			si.fMask = SIF_POS ;
			SetScrollInfo (hwnd, SB_VERT, &si, TRUE) ;
			GetScrollInfo (hwnd, SB_VERT, &si) ;
			// If the position has changed, scroll the window and update it

			if (si.nPos != iVertPos)
			{                    
				ScrollWindow (hwnd, 0, 2*cyChar * (iVertPos - si.nPos), NULL, NULL) ;
				UpdateWindow (hwnd) ;
			}
			return 0 ;
          
		case WM_PAINT:
			for(i=0;i<iFriend;i++)
			{
				if(flist[i].butc==TRUE)
				{
					DestroyWindow(flist[i].but);
					flist[i].butc=FALSE;
				}
			}
			hdc=BeginPaint(hwnd,&ps);

			// Get vertical scroll bar position

			si.cbSize = sizeof (si) ;
			si.fMask  = SIF_POS ;
			GetScrollInfo (hwnd, SB_VERT, &si) ;
			iVertPos = si.nPos ;
			iPaintBeg = max (0,iVertPos+ps.rcPaint.top/cyChar/2);
			iPaintEnd = min (iFriend - 1,iVertPos+ps.rcPaint.bottom/cyChar/2);
			for(i=iPaintBeg;i<=iPaintEnd;i++)
			{
				x=0;
				y=2*cyChar*(i-iVertPos);
				flist[i].but=CreateWindow(TEXT("button"),TEXT(flist[i].id),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,x,y,20*cxChar,7*cyChar/4,hwnd,(HMENU)i,hInst,NULL);
				flist[i].butc=TRUE;
				UpdateWindow(flist[i].but);
				//TextOut(hdc,x,y,flist[i].id,strlen(flist[i].id));
			}
			EndPaint(hwnd,&ps);
			break;
		case WM_DESTROY:
			sprintf(sndbuf,"#logot");
			send(client,sndbuf,strlen(sndbuf),0);
			Sleep(3000);
			PostQuitMessage(0);
			break;
		case WM_SOCKET_N:
			Sleep(100);
			wEvent=WSAGETSELECTEVENT (lParam);
			wError=WSAGETSELECTERROR (lParam);
			if(wError)
			{
				sprintf(opbuf,"socket error #%i\n",wError);
				myerror(opbuf);
				closesocket(client);
				WSACleanup();
				exit(-1);
			}
			numbytes=recv(client,rcvbuf,MAXDATASIZE,0);
			rcvbuf[numbytes]='\0';
			for(i=0;i<6;i++)
			      opbuf[i]=rcvbuf[i];
			opbuf[6]='\0';
			if(strcmp(opbuf,"#chatf")==0)
			{
				sscanf(rcvbuf,"#chatf#%[0-9]#%[0-9]#%[^#]",fid,oid,opbuf);
				if(strcmp(qid,oid)==0)
				{
					for(i=0;i<iFriend;i++)
					{
						if(strcmp(flist[i].id,fid)==0)
						{
							if(flist[i].num==50)
							{
								for(j=0;j<49;j++)
									strcpy(flist[i].op[j],flist[i].op[j+1]);
								flist[i].num--;
							}
							strcpy( flist[i].op[(flist[i].num) ] ,opbuf );
							strcpy( flist[i].fid[(flist[i].num)] ,fid   );
							flist[i].num++;
							if(flist[i].chatc==TRUE)
							{
								InvalidateRect(flist[i].chat,NULL,TRUE);
								UpdateWindow(flist[i].chat);
								break;
							}
							if(flist[i].chatc==FALSE)
							{
								flist[i].chat=CreateWindow(TEXT ("ChatClass"),flist[i].id,WS_OVERLAPPEDWINDOW| WS_VSCROLL,CW_USEDEFAULT,CW_USEDEFAULT,60*cxChar,62*cxChar,NULL,NULL,hInst,NULL);	
								ShowWindow(flist[i].chat,SW_SHOWNORMAL);
								UpdateWindow(flist[i].chat);
								flist[i].chatc=TRUE;
								break;
							}
						}

					}
				}
				else if(strcmp(qid,fid)==0)
				{
					for(i=0;i<iFriend;i++)
					{
						if(strcmp(flist[i].id,oid)==0)
						{
							if(flist[i].num==50)
							{
								for(j=0;j<49;j++)
									strcpy(flist[i].op[j],flist[i].op[j+1]);
								flist[i].num--;
							}
							strcpy( flist[i].op[(flist[i].num) ] ,opbuf );
							strcpy( flist[i].fid[(flist[i].num)] ,qid   );
							flist[i].num++;
							if(flist[i].chatc==TRUE)
							{
								InvalidateRect(flist[i].chat,NULL,TRUE);
								UpdateWindow(flist[i].chat);
								break;
							}
							if(flist[i].chatc==FALSE)
							{
								flist[i].chat=CreateWindow(TEXT ("ChatClass"),flist[i].id,WS_OVERLAPPEDWINDOW| WS_VSCROLL,CW_USEDEFAULT,CW_USEDEFAULT,60*cxChar,62*cxChar,NULL,NULL,hInst,NULL);	
								ShowWindow(flist[i].chat,SW_SHOWNORMAL);
								UpdateWindow(flist[i].chat);
								flist[i].chatc=TRUE;
								break;
							}
						}

					}
				}
			}
			else if(strcmp(opbuf,"#aplff")==0)
			{
				sscanf(rcvbuf,"#aplff#%s",aplid);
				DialogBox(hInst, (LPCTSTR)IDD_CONADD, hwnd, (DLGPROC)ConAdd);				
			}
			else if(strcmp(opbuf,"#conff")==0)
			{
					sprintf(sndbuf,"#getls");
					send(client,sndbuf,strlen(sndbuf),0);				
			}
			else if (strcmp(opbuf,"#frlst")==0 )
			{
				sscanf(rcvbuf,"#frlst#%d",&iFriend);		//读取好友数目
				sprintf(opbuf,"%d",iFriend);
				idlen=8+strlen(opbuf);
				for(i=0;i<iFriend;i++)
				{
					sscanf(rcvbuf+idlen,"%[0-9]",flist[i].id);
					flist[i].nick[0]='\0';
					flist[i].butc=FALSE;			//没有创建按钮窗口
					flist[i].chatc=FALSE;
					flist[i].inputc=FALSE;
					flist[i].sendc=FALSE;
					flist[i].num=0;
					flist[i].chat=0;			//初始化窗口句柄
					flist[i].tempbuf[0]='\0';
					idlen=idlen+strlen(flist[i].id)+1;
				}
				InvalidateRect(hwnd,NULL,TRUE);
				UpdateWindow(hwnd);
			}
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

/*关于对话框过程*/
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			if ( LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
	return FALSE;
}


/*登录窗口过程*/
LRESULT CALLBACK LogsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int logsteps=0;
	PAINTSTRUCT ps;
	HDC hdc;
	UINT texth;

	switch (message) 
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			RECT rt;
			/*调用socket()并检查*/
			if(logsteps<1)
			{
				client=socket(AF_INET,SOCK_STREAM,0);
				if(client==INVALID_SOCKET)
				{
					myerror("socket");
					return -1;
				}
				logsteps++;
			}
			GetClientRect(hWnd, &rt);
			sprintf(msbbuf,"正在连接到 %s:%d",inet_ntoa(my_addr.sin_addr),ntohs(my_addr.sin_port));
			texth=DrawText(hdc, msbbuf, strlen(msbbuf), &rt, DT_CENTER);
			rt.top+=texth;
			if(logsteps<2)
			{
				if( connect(client,(sockaddr *)&my_addr,sizeof(sockaddr)) == -1 )
				{
					closesocket(client);
					myerror("connect");
					return -1;
				}
				logsteps++;
			}
			sprintf(msbbuf,"已连接.",inet_ntoa(my_addr.sin_addr),ntohs(my_addr.sin_port));
			texth=DrawText(hdc, msbbuf, strlen(msbbuf), &rt, DT_CENTER);
			rt.top+=texth;
			sprintf(msbbuf,"验证用户名与密码……");
			texth=DrawText(hdc, msbbuf, strlen(msbbuf), &rt, DT_CENTER);
			rt.top+=texth;
			if(logsteps<3)
			{
				sprintf(sndbuf,"#login#%s#%s",qid,qpwd);
				//MessageBox(NULL,sndbuf,"q",0);
				send(client,sndbuf,strlen(sndbuf),0);
				logsteps++;
			}
			if(logsteps<4)
			{
				if ( (numbytes=recv(client,rcvbuf,MAXDATASIZE,0)) == -1 )
				{
					myerror("recv");
					return -1;
				}
				rcvbuf[numbytes]='\0';
				logsteps++;
			}
			MessageBox(hWnd,rcvbuf,"信息",MB_OK);
			if(strcmp(rcvbuf,"#logst#1")==0)
			{
				logflag=TRUE;
			}
			else if(strcmp(rcvbuf,"#logst#0#pass")==0)
			{
				logflag=FALSE;
				closesocket(client);
			}
			//DrawText(hdc, qpwd, strlen(qpwd), &rt, DT_CENTER);
			EndPaint(hWnd, &ps);
			break;
		case WM_DESTROY:
			logsteps=0;
			//PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return FALSE;
}

/*登录对话框过程*/
LRESULT CALLBACK Login(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				GetDlgItemText(hDlg,IDC_EDIT1,qid,11);
				GetDlgItemText(hDlg,IDC_EDIT2,qpwd,21);
				for(i=0;i<strlen(qid);i++)
				{
					if(qid[i]<'0'||qid[i]>'9')
					{
						MessageBox(hDlg,"ID非法！","警告",MB_OK);
						return TRUE;
					}
				}
			/*	if(strlen(qpwd)<6)
				{
						MessageBox(hDlg,"密码过短！","警告",MB_OK);
						return TRUE;
				}*/
				for(i=0;i<strlen(qpwd);i++)
				{
					if(!( (qpwd[i]>='a'&&qpwd[i]<='z') || (qpwd[i]>='A'&&qpwd[i]<='Z') || (qpwd[i]=='_') ))
					{
						MessageBox(hDlg,"密码非法！","警告",MB_OK);
						return TRUE;
					}
				}
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			if (LOWORD(wParam) == IDCANCEL) 
			{
				WSACleanup();
				exit(0);
			}
			if (LOWORD(wParam) == IDC_BUTTON1) 
			{
				DialogBox(hInst, (LPCTSTR)IDD_REGISTER, hDlg, (DLGPROC)Register);
			}
			break;
	}
    return FALSE;
}



/*注册对话框过程*/
LRESULT CALLBACK Register(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int charnum=0;              //charnum代表已经检查的字符个数
	int atnum=0;                //atnum代表@的个数
	char mailbox[51]={0};
	char mailconf[8]={'\0'};         //回收服务器对邮箱的正确性的恢复
	char confs[8]="#confs";                
	char content[120]={'\0'};             
	char notice[]="NOTICE!";         //提示匡标题
	char nick[31]={0};
	char cfcode[33]={0};
	char pwd[21]={0};
	char pwd1[21]={0};
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_BUTTON1) 
			{
				client=socket(AF_INET,SOCK_STREAM,0);
				if(client==INVALID_SOCKET)
				{
					myerror("socket");
					return -1;
				}
				if( connect(client,(sockaddr *)&my_addr,sizeof(sockaddr)) == -1 )
				{
					closesocket(client);
					myerror("connect");
					return -1;
				}
				GetDlgItemText(hDlg,IDC_EDIT1,mailbox,50);
				do
				{
					if(mailbox[charnum]=='\0')
					{
						if(mailbox[charnum-1]=='@')//结尾为@
						{
							atnum=2;
						}
						break;
					}
					if(mailbox[charnum]=='@')
					{
						if(charnum==0)
						{
							atnum=2;//开头为@
							break;
						}
						atnum++;
						if(atnum>1)
						{
							break;
						}
					}
					charnum++;	
				}
				while(charnum<50);
				if(atnum!=1)
				{
					MessageBox (NULL, TEXT("邮箱输入非法"), TEXT("提示"),0);
					return TRUE;
				}
				else
				{
					sprintf(sndbuf,"#sdcfm#%s",mailbox);
					send(client,sndbuf,strlen(sndbuf),0);
					recv(client,rcvbuf,50,0);
					closesocket(client);
					strncpy(mailconf,rcvbuf,6);
					if(strcmp(confs,mailconf)==0)
					{
						sprintf(content,"SUCCEED!Please check your mailbox!");
						MessageBox (NULL, content, notice,0);
						return TRUE;
					}
					else
					{
						sprintf(content,"FAILURE!Reason:%s",rcvbuf+7);
						MessageBox (NULL, content, notice,0);
						return TRUE;
					}
				}
				/*在这里编辑按下发送验证码后的操作
				如果邮箱为空或非法，则弹出提示对话框并return TRUE
				否则，向服务器发送#sdcfm#邮箱
				若服务器返回#confs，则弹出提示对话框"发送成功"并return TRUE
				若服务器返回#conff#错误原因,则弹出提示对话框"发送错误：错误原因"并return TRUE
				*/
				closesocket(client);
			}
			if (LOWORD(wParam) == IDOK) 
			{
				client=socket(AF_INET,SOCK_STREAM,0);
				if(client==INVALID_SOCKET)
				{
					myerror("socket");
					return -1;
				}
				if( connect(client,(sockaddr *)&my_addr,sizeof(sockaddr)) == -1 )
				{
					closesocket(client);
					myerror("connect");
					return -1;
				}
				GetDlgItemText(hDlg,IDC_EDIT1,mailbox,50);
				GetDlgItemText(hDlg,IDC_EDIT2,nick,30);
				GetDlgItemText(hDlg,IDC_EDIT3,cfcode,32);
				GetDlgItemText(hDlg,IDC_EDIT4,pwd,20);
				GetDlgItemText(hDlg,IDC_EDIT5,pwd1,20);
				if(strlen(mailbox)==0)
				{
					sprintf(content,"Please input your mail address!");
					MessageBox (NULL, content, notice,0);
				}
				else if(strlen(nick)==0)
				{
					sprintf(content,"Please input your nickname!");
					MessageBox (NULL, content, notice,0);
				}
				else if(strlen(cfcode)==0)
				{
					sprintf(content,"Please input your identifying code!");
					MessageBox (NULL, content, notice,0);
				}
				else if(strlen(pwd)==0)
				{
					sprintf(content,"Please input your identifying code!");
					MessageBox (NULL, content, notice,0);
				}
				else if(strlen(pwd1)==0)
				{
					sprintf(content,"Please input your password!");
					MessageBox (NULL, content, notice,0);
				}
				else if(strcmp(pwd,pwd1)!=0)
				{
					sprintf(content,"Passwords don't match!");
					MessageBox (NULL, content, notice,0);
				}
				else
				{
					sprintf(sndbuf,"#regst#%s#%s#%s#%s",mailbox,pwd,nick,cfcode);
					send(client,sndbuf,strlen(sndbuf),0);
					recv(client,rcvbuf,50,0);

					strncpy(mailconf,rcvbuf,7);
					sprintf(confs,"#rgsts#");
					if(strcmp(confs,mailconf)==0)
					{
						sprintf(content,"SUCCEED!Your ID:%s",rcvbuf+7);
						MessageBox (NULL, content, notice,0);
						EndDialog  (hDlg, LOWORD(wParam));
					}
					else
					{
						sprintf(confs,"#rgstf#");
						if(strcmp(confs,mailconf)==0)
						{
							sprintf(content,"FAILURE!Reason:%s",rcvbuf+7);
							MessageBox (NULL, content, notice,0);
							return true;
						}
						else
						{
							myerror("mail");
							return true;
						}

					}
				}


				/*在这里编辑按下注册后的操作
				如果三项任一项为空或非法，则弹出提示对话框并return TRUE
				否则，向服务器发送#regst#邮箱#昵称#验证码
				若服务器返回#rgsts#用户id，则弹出提示“注册成功，您的id是……，并使用EndDialog(hDlg, LOWORD(wParam))关闭对话框”
				若服务器返回#rgstf#错误原因,则弹出提示对话框"错误原因"并return TRUE

				*/

				return TRUE;
			}
			if (LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
			}
			if (LOWORD(wParam) == IDC_BUTTON1) 
			{
				DialogBox(hInst, (LPCTSTR)IDD_REGISTER, hDlg, (DLGPROC)Register);
			}
			break;
	}
    return FALSE;
}


/*聊天窗口过程*/
LRESULT CALLBACK ChatProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;				//有效矩形
	static int  cxChar, cxCaps, cyChar,cxClient,cyClient;
	TEXTMETRIC  tm ;
	int wmId, wmEvent;
	SCROLLINFO  si ;			//滚动条信息
	int i, x, y, iVertPos,iPaintBeg, iPaintEnd ;
	char curid[11];				//当前id
	int curnum=-1;				//当前顺序数目
	for(i=0;i<iFriend;i++)
	{
		if(flist[i].chat==hwnd)
			curnum=i;
	}
	if(curnum!=-1)
	{
		strcpy(curid,flist[curnum].id);
	}


	switch(message)
	{
		case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
			switch (wmId)
			{
				case 2:
					if(GetWindowTextLength(flist[curnum].hinput)>=100)
						myerror("You have inputed over 100 characters\n");
					else if(GetWindowTextLength(flist[curnum].hinput)==0)
						myerror("Please input before send\n");	
					else
					{
						GetWindowText(flist[curnum].hinput,opbuf,100);
						flist[curnum].tempbuf[0]='\0';
						SetWindowText(flist[curnum].hinput,"\0");
						sprintf(sndbuf,"#chatt#%s#%s",flist[curnum].id,opbuf);
						send(client,sndbuf,strlen(sndbuf),0);
					}
					break;
				default:
				{
					return DefWindowProc(hwnd, message, wParam, lParam);
				}
			}
			break;
		case WM_CREATE:
			hdc = GetDC (hwnd) ;
          
			GetTextMetrics (hdc, &tm) ;
			cxChar = tm.tmAveCharWidth ;
			cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2 ;
			cyChar = tm.tmHeight + tm.tmExternalLeading ;
          
			ReleaseDC (hwnd, hdc) ;

			break;
		case WM_SIZE:
			cxClient = LOWORD (lParam) ;
			cyClient = HIWORD (lParam)-3*cyChar-7*cyChar/4 ;

			// Set vertical scroll bar range and page size

			si.cbSize = sizeof (si) ;
			si.fMask  = SIF_RANGE | SIF_PAGE|SIF_DISABLENOSCROLL ;
			si.nMin   = 0 ;
			si.nMax   = max(flist[curnum].num - 1,0) ;
			si.nPage  = cyClient / cyChar/3 ;
			SetScrollInfo (hwnd, SB_VERT, &si, TRUE) ;
		case WM_VSCROLL:
			       // Get all the vertial scroll bar information

			si.cbSize = sizeof (si) ;
			si.fMask  = SIF_ALL ;
			GetScrollInfo (hwnd, SB_VERT, &si) ;

			// Save the position for comparison later on

			iVertPos = si.nPos ;

			switch (LOWORD (wParam))
			{
			case SB_TOP:
				si.nPos = si.nMin ;
				break ;
               
			case SB_BOTTOM:
				si.nPos = si.nMax ;
				break ;
               
			case SB_LINEUP:
				si.nPos -= 1 ;
				break ;
               
			case SB_LINEDOWN:
				si.nPos += 1 ;
				break ;
               
			case SB_PAGEUP:
				si.nPos -= si.nPage ;
				break ;
               
			case SB_PAGEDOWN:
				si.nPos += si.nPage ;
				break ;
               
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos ;
				break ;
               
			default:
				break ;         
			}
			// Set the position and then retrieve it;  Due to adjustments
			//   by Windows it may not be the same as the value set.
			si.fMask = SIF_POS ;
			SetScrollInfo (hwnd, SB_VERT, &si, TRUE) ;
			GetScrollInfo (hwnd, SB_VERT, &si) ;
			// If the position has changed, scroll the window and update it

			if (si.nPos != iVertPos)
			{                    
				ScrollWindow (hwnd, 0, 3*cyChar * (iVertPos - si.nPos), NULL, NULL) ;
				UpdateWindow (hwnd) ;
			}
			return 0 ;
          
		case WM_PAINT:
			if(GetWindowTextLength(flist[curnum].hinput)>0)
			{
				GetWindowText(flist[curnum].hinput,flist[curnum].tempbuf,100);
			}
			
			si.cbSize = sizeof (si) ;
			si.fMask  = SIF_RANGE | SIF_PAGE|SIF_DISABLENOSCROLL ;
			si.nMin   = 0 ;
			si.nMax   = max(flist[curnum].num - 1,0) ;
			si.nPage  = cyClient / cyChar/3 ;
			SetScrollInfo (hwnd, SB_VERT, &si, TRUE) ;
			hdc=BeginPaint(hwnd,&ps);

			// Get vertical scroll bar position

			si.cbSize = sizeof (si) ;
			si.fMask  = SIF_POS ;
			GetScrollInfo (hwnd, SB_VERT, &si) ;
			iVertPos = si.nPos ;
			iPaintBeg = max (0,iVertPos+ps.rcPaint.top/cyChar/3);
			iPaintEnd = min (flist[curnum].num - 1,iVertPos+(ps.rcPaint.bottom-19*cyChar/3)/cyChar/3);
			
			if(flist[curnum].inputc=TRUE)
			{
				DestroyWindow(flist[curnum].hinput);
				flist[curnum].inputc=FALSE;
			}
			if(flist[curnum].sendc=TRUE)
			{
				DestroyWindow(flist[curnum].hsend);
				flist[curnum].sendc=FALSE;
			}
			if(flist[curnum].num)
			{
				for(i=iPaintBeg;i<flist[curnum].num;i++)
				{
					x=0;
					y=3*cyChar*(i-iVertPos);
					if(y>=cyClient)
						break;
					TextOut(hdc,x,y,flist[curnum].fid[i],strlen(flist[curnum].fid[i]));
					TextOut(hdc,x,y+cyChar,flist[curnum].op[i],strlen(flist[curnum].op[i]));
				}

			}
			EndPaint(hwnd,&ps);
			flist[curnum].hinput=CreateWindow(TEXT ("edit"),NULL,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_MULTILINE,0,cyClient,60*cxChar,3*cyChar,hwnd,(HMENU)1,hInst,NULL);
			flist[curnum].hsend =CreateWindow(TEXT("button"),TEXT("Send"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,0,cyClient+3*cyChar,10*cxChar,7*cyChar/4,hwnd,(HMENU)2,hInst,NULL);
			if(flist[curnum].tempbuf[0]!='\0')
			{
				SetWindowText(flist[curnum].hinput,flist[curnum].tempbuf);
				flist[curnum].tempbuf[0]='\0';
			}
			UpdateWindow(flist[curnum].hinput);
			UpdateWindow(flist[curnum].hsend);
			break;
		case WM_DESTROY:
			flist[curnum].chatc=FALSE;
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}




void myerror(char *str)
{
	MessageBox(NULL,TEXT (str),TEXT ("error"),0);
	return;
}



LRESULT CALLBACK AddFriend(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char AFID[11];
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				GetDlgItemText(hDlg,IDC_EDIT1,AFID,10);
				for(i=0;i<10;i++)
				{
					if(i==strlen(AFID))
					{
						break;
					}
					if(*(AFID+i)>='0' && *(AFID+i)<='9')
					{
						continue;
					}
					else
					{
						myerror("ID中有非法字符!");
						return TRUE;
					}

				}
				sprintf(sndbuf,"#aplfr#%s",AFID);
				send(client,sndbuf,strlen(sndbuf),0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			if ( LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
	return FALSE;
}

LRESULT CALLBACK ConAdd(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				sprintf(sndbuf,"#confr#%s",aplid);
				send(client,sndbuf,strlen(sndbuf),0);
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			if ( LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
	return FALSE;
}



