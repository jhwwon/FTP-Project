/*********************************
 *	FTP Server	*
 *********************************/
//iter: iterative 모드를 의미 한번에 한 클라이언트씩 처리하는 프로그램
// 필요 헤더 선언
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/wait.h>

#define BUFSIZE 65535
#define MAXSOCK 64
#define MAXBACKLOG 4  //*서버가 한번에 클라이언트 접속을 4명까지 허용
//*한명이 처리되고 나면 나머지를 처리 iterative 방식이기 때문에 
#define MIN(a,b) (((a)<(b))?(a):(b)) //*버퍼 포인터 핸들링할 때 포인터가 버퍼의 경계를 넘어가면 이상한 행동으로 죽기 때문에 방지하기 위해 만든 마크로 
#define MAX(a,b) (((a)>(b))?(a):(b))

// 함수 프로토타입
void removeFrontSpaces(char *); //*commandline으로 입력받은 명령어 인자들을 받았을 때 앞에 front에 블랭크 문자들이 많은 것을 제거하는 함수  
void printMsg(char* formatString, ...); // 서버 메시지 기록 루틴
void removeFrontSpaces(char *);	// remove spaces 
int tokenize(char *buf, char *tokens[], int maxTokens);
void cmdHelp();			// help
void switchStdout(const char *newStream);
void revertStdout();
void cmd(char *tokens[]);
size_t cmdwithfile(char *tokens[],char txbuf[]);


// 전역변수 선언, *서버가 이용하고 있는 port번호 
int servport;


int main(int argc, char * argv[])
{
	int servsock, clisock;	//서버 소켓, 통신용 소켓 변수
	int cliaddrlen;		// Client Address 구조체 길이 저장 변수
	struct sockaddr_in servaddr, cliaddr;
	long len;
	
	char rxbuf[BUFSIZE] = {0,};	// 수신 저장
	char txbuf[BUFSIZE] = {0,};	// 송신 저장
	char msg[BUFSIZE] = {0,};	// 송신 메시지 저장
	char cpath[BUFSIZE] = {0,};	//고정 경로 저장, *파일경로 알기위해 필요함

	if(argc < 2) { //자기 서버 실행파일 이름하고 포트번호 
		printf("Usage : %s <Port#>\n", argv[0]);
		exit(1);
	}

	servport=atoi(argv[1]);

	// 소켓 생성
	if((servsock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("sock() error");
		exit(1);
	}

	// 서버 소켓 주소 구조체 정보
	memset(&servaddr, 0x00, sizeof(servaddr));  //*성공하면 servaddr IP주소 구조체를 0으로 초기화 
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //*서버니까 어떤 addr에 접속해도 허용하겠다라는 의미
	servaddr.sin_port = htons(servport); //*host to network short int로 

	// 서버 소켓번호와 서버 주소, 서버 포트 바인딩
	if(bind(servsock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind() error");
		exit(1);
	}
	printf("Waiting for connection! \n");	
	listen(servsock, MAXBACKLOG);	//클라이언트 접속 4명 까지 요청 대기

	cliaddrlen = sizeof(cliaddr);
	
	while(1) {
		// 연결 요청 수락
		if((clisock = accept(servsock, (struct sockaddr *)&cliaddr, &cliaddrlen)) < 0) {
			perror("accept() error");
			exit(1);
		}
	
		printf("Waiting for commands! \n");
		
		while(1) {  //*데이터 수신
			len=recv(clisock, rxbuf, BUFSIZE, 0);
			if(len == -1) {
      				perror("recv() 에러");
					exit(1);
      		 }
     		if(len == 0)  { // 클라이언트 연결 단절 확인
       	 		printf("클라이언트 단절: s= %d\n", clisock);
	    		close(clisock);
				break;
			}
			else {
				char *tokens[10];
				char *endptr = (char*)rxbuf + MIN(strlen(rxbuf), BUFSIZE - 1); //*inbuf의 끝이 어딘가를 나타내기 위해서, 경계넘어가면 에러나니까
				char *cmdptr = strtok(rxbuf, " \n"); //inbuf에서 읽어들어옴, cmdptr은 클라이언트 입력 명령을 저장하고 있음, txbuf에 원래 데이터 그대로 있음
		
				char *argptr = rxbuf + strlen(cmdptr) +1; //* ls다음에 -l, -al이 있다면 argptr이 arg를 가리킴 없
				
				if (argptr > endptr || *argptr == '\0') {
					argptr = NULL;
				}else
				{
					tokenize(argptr,tokens,10);
				}
				
				// ls 명령,  서버쪽 현재 디렉터리를 읽어서 클라이언트로 전송
				if(strncmp(rxbuf, "ls",2)==0) {
					printf("ls started! \n");
					//  이부분을 구현하세요
					tokens[0]="ls";
					cmdwithfile(tokens,txbuf);
					send(clisock,txbuf,strlen(txbuf),0);
				}
				// cd 명령,  바뀐 경로명을 전달 서버는 바뀐 경로에 머물러야 함
				else if(strncmp(rxbuf, "cd",2)==0) {
					printf("cd started! \n");
					//  이부분을 구현하세요

				}
				// get 명령,  해당 파일 이름을 가지고 찾아서 그 파일을 클라이언트에 전송
				else if(strncmp(rxbuf, "get",3)==0) {
					printf("GET started! \n");
					//  이부분을 구현하세요
					// tokens[0]="get"; tokens[1]="filename";
					open("short.txt", O_RDONLY);
					send() // 상태 OK FAIL
					read(txbuf)
					send(txbuf)
					close(clisock)
					printf(""// 상태 출력 
				}
				// put 명령, 클라이언트가 서버에 파일 전송하면 서버는 받아서 현재 디렉토리 폴더내에 이 파일이름으로 저장
				else if(strncmp(rxbuf, "put",3)==0) {
					printf("PUT started! \n");
					//  이부분을 구현하세요

				}
				// quit 명령 
				else if(strncmp(rxbuf, "quit",4)==0) {
					printf("Quit: a client out \n");
					close(clisock);
					memset(rxbuf, 0x00, sizeof(rxbuf));
					break;
				}
				// 허용 안되는 명령
				else 	printf("Illegal command \n");
			}
		}
	}
	
	close(servsock);
	return 0;
}

/////////////////////////////////////////////////////////////
/*help 명령 함수*/
void cmdHelp() {
	printf("-----------------------------------------------------------\n");
	printf("| ls  [option] : list server directory		|\n");
	printf("| cls [option] : list client directory    		|\n");
	printf("| cd  [option] : Change server directory		|\n");
	printf("| ccd [option] : Change client directory		|\n");
	printf("| get filename : Download a file from the server	|\n");
	printf("| put filename: Upload a file from a client	|\n");
	printf("| quit : exit 					|\n");
	printf("-----------------------------------------------------------\n");
}

// 필요한 명령 함수들을 여기에 작성하세요

void removeFrontSpaces(char *buf)
{ 
    int i1=0;
	int i2=0;
	
    while (buf[i1] == ' ') {
		i1++;
	}
	while (buf[i1]) {
		buf[i2]=buf[i1];
		i1++; i2++;
	}
	buf[i2]='\0';
}

// "ls -al" ==> "ls","-al",NULL
int tokenize(char *buf, char *tokens[], int maxTokens)
{
	int t_count = 1;
	char *token;
	token = strtok(buf, " \n");

	while (token != NULL && t_count < maxTokens)
	{
		tokens[t_count++] = token;
		token = strtok(NULL, " \n");
	}

	tokens[t_count] = NULL;
	return t_count;
}

static int fd;
static fpos_t pos;

void switchStdout(const char *newStream)
{
  fflush(stdout);
  fgetpos(stdout, &pos);
  fd = dup(fileno(stdout));
  freopen(newStream, "w", stdout);
}

void revertStdout()
{
  fflush(stdout);
  dup2(fd, fileno(stdout));
  close(fd);
  clearerr(stdout);
  fsetpos(stdout, &pos);
}


void cmd(char *tokens[])
{
	pid_t pid;
	int status;

	if ((pid = fork()) == 0)
	{
		execvp(tokens[0], tokens);	
	}
	wait(&status);
}

size_t cmdwithfile(char *tokens[],char txbuf[])
{
	
	switchStdout("result.txt");
	cmd(tokens);
	revertStdout();
	
	int fd=open("result.txt",O_RDONLY);
	size_t size=0;
	size=read(fd,txbuf,BUFSIZE);
	txbuf[size]='\0';
	close(fd);
	
	return size;
}

