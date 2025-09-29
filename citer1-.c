/*********************************
 *	FTP Client	*
 *********************************/

/*필요 헤더 선언*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>

#define BUFSIZE 65535
#define MAXSOCK 128
#define IPNum "127.0.0.1" //Server IP 주소, 루프백 주소 IP주소 입력안해도됨

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// 함수 프로토타입(Prototype) 
void removeFrontSpaces(char *);	// remove spaces 
int tokenize(char *buf, char *tokens[], int maxTokens);
void cmdHelp();			// help
void switchStdout(const char *newStream);
void revertStdout();
void cmd(char *tokens[]);
size_t cmdwithfile(char *tokens[],char txbuf[]);

int main(int argc, char * argv[])
{
	int sock;			//소켓디스크립터 저장 int형 변수
	int fsize;			// 파일 크기
	char charbuf[1]={0}; //전송할 때 테스트용
	int flag;
	
	struct sockaddr_in ServerAddr;	//bind 시킬 구조체 변수
	char inbuf[BUFSIZE] = {0,};	//명령 라인 저장용 배열
	char txbuf[BUFSIZE] = {0,};	//명령 라인 전송용 배열
	char rxbuf[BUFSIZE] = {0,};	//데이터 받을 char형 배열 서버로부터 데이터 받을 때
	char cpath[BUFSIZE] = {0,};	//현재 경로 저장 char형 배열
	char *tokens[10];
	
	// 사용법
	if(argc < 2) { //*argc2개보다 작으면 에러, 클라이언트 프로그램 이름이랑 서버 포트 번호 입력
		printf("Usage : %s <PortNum>\n", argv[0]);
		exit(1);
	}

	// 소켓 생성 
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("sock() error");
		exit(1);
	}

	//  소켓 주소 구조체 정보 저장*/
	memset(&ServerAddr, 0x00, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr(IPNum);
	ServerAddr.sin_port = htons(atoi(argv[1]));

	if(connect(sock, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) < 0) {
		perror("connect() error");
		exit(1);
	}
	
	printf("FTP Server connected!\n");
	
	while(1) {
		printf("ftp>> ");
		fgets(inbuf, BUFSIZE, stdin); //키보드 입력 저장
		removeFrontSpaces(inbuf); //*블랭크 문자 제거해서 inbuf에 넣음
		if (strlen(inbuf)==1) 
			continue; 
		strncpy(txbuf, inbuf, BUFSIZE); //*키보드 입력된 내용을 두군데에 저장, 두 buf변수를 임의로 사용해서 구현하면 됨 이걸 이용해서 구현 
		
		char *endptr = (char*)inbuf + MIN(strlen(inbuf), BUFSIZE - 1); //*inbuf의 끝이 어딘가를 나타내기 위해서, 경계넘어가면 에러나니까
		char *cmdptr = strtok(inbuf, " \n"); //inbuf에서 읽어들어옴, cmdptr은 클라이언트 입력 명령을 저장하고 있음, txbuf에 원래 데이터 그대로 있음
		
		char *argptr = inbuf + strlen(cmdptr) +1; //* ls다음에 -l, -al이 있다면 argptr이 arg를 가리킴 없다면 NULL을 리턴
		if (argptr > endptr || *argptr == '\0') {
			argptr = NULL;
		}else
		{
			tokenize(argptr,tokens,10);
		}
		//printf("Inbuf argptr = %s \n", argptr);
		
		// cls 명령, 자기 local ls 명령이므로 서버랑 통신이 필요없음 
		if(strcmp(cmdptr, "cls")==0) {
			printf("cls started \n");
		// 이부분을 구현하세요 //*내 클라이언트 사이드에 ls의 내용을 리스트 디렉토리 한것을 display하면    		
			tokens[0]="ls";
			cmd(tokens);
			printf("cls END\n");
		}
		
		// ccd 명령,  서버랑 통신 필요없음
		else if(strcmp(cmdptr, "ccd")==0) {
			printf("ccd started \n");
			// 이부분을 구현하세요
			tokens[0] = "cd";
			cmd(tokens);
		}
		
		// ls 명령, 서버로 ls명령을 보내줘야 함 서버 ls 명령 내용을 클라이언트로 보내줘야 함 출력
		else if(strcmp(cmdptr, "ls")==0) {
			printf("ls started\n");
			// 이부분을 구현하세요
			send(sock,cmdptr,strlen(cmdptr),0);
			int len=recv(sock,rxbuf,BUFSIZE,0);
			rxbuf[len]='\0';
			printf("%s\n", rxbuf);
		}
		
		// cd 명령, 서버에 보내주고 서버는 이걸 받아서 자기 디렉토리를 change하고 경로를 이쪽으로 보내줘야 함
		else if(strcmp(cmdptr, "cd")==0) {
			printf("cd started \n");
			// 이부분을 구현하세요
			tokens[0] = "cd";
			cmdwithfile(tokens, txbuf);
			printf("---------%s---------\n", txbuf);
		}
		
		// get 명령, 서버로부터 해당 파일을 가져오는 것
		else if(strcmp(cmdptr, "get")==0) {
			printf("get started \n");
			// 이부분을 구현하세요
			sprintf(txbuf, "get %s", char *tokens[2]); //"get filename"
			send(sock,txbuf,strlen(txbuf),0); 
			recv() // 상태 OK FAIL
			recv(sock,rxbuf,BUFSIZE,0);    // rxbuf
			open("short.txt", O_WRONLY|O_CREAT, 0666);      // open write close
			write(sock, char *tokens[2],  );  //전송할 데이터를 가지고 있는 버퍼의 포인터,내 디렉토리에 파일을 써줘야함
			close(sock);
			printf("다운로드 완료\n"); //상태 출력 
		}
			
		// put 명령, 파일네임에 해당하는 파일을 서버로 보내서 서버의 현재 디렉토리에 저장시키는 것
		else if(strcmp(cmdptr, "put")==0) {
			printf("put started \n");
			// 이부분을 구현하세요
		}
		
		// help 명령
		else if(strcmp(cmdptr, "help")==0) {
			cmdHelp();
		}

		// quit 명령 
		else if(strcmp(cmdptr, "quit")==0) {
			send(sock, txbuf, BUFSIZE, 0);
			printf("종료 되었습니다\n");
			close(sock);
			break;
		}
		else printf("CMD error: Type 'help'\n");
	}	
	return 0;
}


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

