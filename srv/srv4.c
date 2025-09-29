/*********************************
 *	FTP Server	*
 *********************************/

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
#include <sys/stat.h>
#include <pthread.h>

#define BUFSIZE 65535
#define MAXSOCK 64
#define MAXBACKLOG 4
#define MAXCLIENT 4

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// 함수 프로토타입
void removeFrontSpaces(char *);
void printMsg(char *formatString, ...); // 서버 메시지 기록 루틴
void removeFrontSpaces(char *);         // remove spaces
int tokenize(char *buf, char *tokens[], int maxTokens);
void cmdHelp(); // help
void switchStdout(const char *newStream);
void revertStdout();
void cmd(char *tokens[]);
size_t cmdwithfile(char *tokens[], char txbuf[]);
void cmdLs(int sock, char *options[]);
void cmdCd(int sock, char *options[]);
void cmdGet(int sock, char filename[]);
void cmdPut(int sock, char filename[]);
void *ftpsrv(void *data);

// 전역변수 선언
int servport;
pthread_t pthread[MAXCLIENT] = {
    0,
};

struct thread_data
{
    int clisock;
    int idx;
    char path[100];
};

int main(int argc, char *argv[])
{
    int servsock;   //서버 소켓, 통신용 소켓 변수
    int clisock;    //서버 소켓, 통신용 소켓 변수
    int cliaddrlen; // Client Address 구조체 길이 저장 변수
    struct sockaddr_in servaddr, cliaddr;
    int pid;

    if (argc < 2)
    {
        printf("Usage : %s <Port#>\n", argv[0]);
        exit(1);
    }

    servport = atoi(argv[1]);

    // 소켓 생성
    if ((servsock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("sock() error");
        exit(1);
    }

    // 서버 소켓 주소 구조체 정보
    memset(&servaddr, 0x00, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(servport);

    // 서버 소켓번호와 서버 주소, 서버 포트 바인딩
    if (bind(servsock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind() error");
        exit(1);
    }
    printf("Waiting for connection! \n");

    listen(servsock, MAXBACKLOG); //클라이언트 접속 4명 까지 요청 대기

    while (1)
    {
        // 연결 요청 수락
        cliaddrlen = sizeof(cliaddr);

        if ((clisock = accept(servsock, (struct sockaddr *)&cliaddr, &cliaddrlen)) < 0)
        {
            perror("accept() error");
            printf("Waiting %d\n", clisock);
            exit(1);
        }

        printf("Waiting for commands! %d\n", clisock);

        struct thread_data td;

        for (int i = 0; i < MAXCLIENT; i++)
        {
            if (pthread[i] == 0)
            {
                td.clisock = clisock;
                td.idx = i;
                getcwd(td.path, 256);

                pthread_create(&pthread[i], NULL, ftpsrv, (void *)&td);
                break;
            }
        }
    }

    return 0;
}

//////////////////
void *ftpsrv(void *data)
{
    struct thread_data td;
	struct thread_data *ptd=(struct thread_data *)data;
	
	td.clisock=ptd->clisock;
	td.idx=ptd->idx;
	strcpy(td.path,ptd->path);
	
    long len;

    char rxbuf[BUFSIZE] = {
        0,
    }; // 수신 저장
    char txbuf[BUFSIZE] = {
        0,
    }; // 송신 저장
    char msg[BUFSIZE] = {
        0,
    }; // 송신 메시지 저장
    char cpath[BUFSIZE] = {
        0,
    }; //고정 경로 저장

    int clisock = td.clisock;
    int index = td.idx;

    for (;;)
    {
        len = recv(clisock, rxbuf, 100, 0);
        if (len == -1)
        {
            perror("recv() 에러");
            exit(1);
        }
        if (len == 0)
        { // 클라이언트 연결 단절 확인
            printf("클라이언트 단절: s= %d\n", clisock);
            close(clisock);
            pthread[index] = 0;
            break;
        }
        else
        {
            rxbuf[len] = '\0';
            //				printf("received[%s]",rxbuf);
            char *tokens[10];
            char *endptr = (char *)rxbuf + MIN(strlen(rxbuf), BUFSIZE - 1);
            char *cmdptr = strtok(rxbuf, " \n");
            char *argptr = rxbuf + strlen(cmdptr) + 1;

            if (argptr > endptr || *argptr == '\0')
            {
                argptr = NULL;
            }
            else
            {
                tokenize(argptr, tokens, 10);
            }

            if (strcmp(cmdptr, "ls") == 0)
			{        
                printf("ls started! \n");
                chdir(td.path);
                cmdLs(clisock, tokens);
            }
            else if (strcmp(cmdptr, "cd") == 0)
            {
                printf("cd started! \n");
                chdir(td.path);
                cmdCd(clisock, tokens);
                getcwd(td.path, 256);
            }
            // get 명령
            else if (strcmp(cmdptr, "get") == 0)
            {
                printf("GET started! \n");
                chdir(td.path);
                cmdGet(clisock, tokens[1]);
            }
            // put 명령
            else if (strcmp(cmdptr, "put") == 0)
            {
                printf("PUT started! \n");
                chdir(td.path);
                cmdPut(clisock, tokens[1]);
            }
            // quit 명령
            else if (strcmp(rxbuf, "quit") == 0)
            {
                printf("Quit: a client out \n");
                close(clisock);
                pthread[index] = 0;
                break;
            }
            // 허용 안되는 명령
            else
                printf("Illegal command \n");
        }
    }
}

/////////////////////////////////////////////////////////////
/*help 명령 함수*/
void cmdHelp()
{
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
void cmdLs(int sock, char *options[])
{
    char txbuf[BUFSIZE];
    options[0] = "ls";
    cmdwithfile(options, txbuf);
    send(sock, txbuf, strlen(txbuf), 0);
}

void cmdCd(int sock, char *options[])
{
    chdir(options[1]);
    char buf[256];
    printf("%s\n", getcwd(buf, 256));
    send(sock, buf, 100, 0);
}

void cmdGet(int sock, char filename[])
{
    // tokens[0]="get"; tokens[1]="filename";
    char txbuf[BUFSIZE];
    int fd = open(filename, O_RDONLY);
    if (fd > 0)
    {
        send(sock, "OOOO", 4, 0);

        struct stat st;
        stat(filename, &st);
        int count = (int)((float)st.st_size / (BUFSIZE + 1) + 1);
        printf("file size=%ld,count=%d\n", st.st_size, count);
        send(sock, &count, sizeof(count), 0); // 상태 OK FAIL

        for (int i = 0; i < count; i++)
        {
            int len = read(fd, txbuf, BUFSIZE);
            send(sock, txbuf, len, 0);
        }
        close(fd);
    }
    else
        send(sock, "XXXX", 4, 0);
}

void cmdPut(int sock, char filename[])
{
    char txbuf[BUFSIZE];

    int count;
    recv(sock, &count, sizeof(count), 0);
    printf("count= %d\n", count);
    int fd = open(filename, O_WRONLY | O_CREAT, 0666);

    for (int i = 0; i < count; i++)
    {
        int len = recv(sock, txbuf, BUFSIZE, 0);
        write(fd, txbuf, len);
    }
    close(fd);
    printf("완료\n");
}

void removeFrontSpaces(char *buf)
{
    int i1 = 0;
    int i2 = 0;

    while (buf[i1] == ' ')
    {
        i1++;
    }
    while (buf[i1])
    {
        buf[i2] = buf[i1];
        i1++;
        i2++;
    }
    buf[i2] = '\0';
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

    if (tokens[1] != NULL && tokens[1][0] == '\0')
    {
        tokens[1] = NULL;
        printf("cmd %s\n", tokens[0]);
    }

    if (tokens[1] != NULL && tokens[1][0] != '\0')
        printf("cmd %s %s\n", tokens[0], tokens[1]);

    if ((pid = fork()) == 0)
    {
        execvp(tokens[0], tokens);
    }
    wait(&status);
}

size_t cmdwithfile(char *tokens[], char txbuf[])
{

    switchStdout("result.txt");
    cmd(tokens);
    revertStdout();

    int fd = open("result.txt", O_RDONLY);
    size_t size = 0;
    size = read(fd, txbuf, BUFSIZE);
    txbuf[size] = '\0';
    close(fd);

    return size;
}
