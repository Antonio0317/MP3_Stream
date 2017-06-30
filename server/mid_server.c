#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include "wavFile.h"

void streaming_wav(int client_fd);
long accept_clone(int ,struct sockaddr *, socklen_t *, int ,int *);

int main(void)
{
	struct sockaddr_in server_addr, client_addr;
	int server_fd, client_fd;
	socklen_t client_addr_len;
	int pid;

    //db
    if(fork() == 0){
        system("python server.py");
        return 0;
    }

	//create socket
	server_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (server_fd < 0){
		perror("socket error : ");
		exit(1);
	}

	// option
	int enable = 1;
	if( setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0 )
		perror("setsockopt(SO_REUSEADDR) failed");

	//bind
	memset(&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(19999);
	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
		perror("bind error : ");
		exit(1);
	}

	//listen
	if (listen(server_fd, 5) < 0){
		perror("listen error : ");
		exit(1);
	}

	/* test */
	printf("Server running ...\n");
	/* test */

	client_addr_len = sizeof(client_addr);

	printf("pid : %d\n", getpid());
	while(1){
		pid = accept_clone(server_fd, (struct sockaddr*)&client_addr, &client_addr_len, 0, &client_fd);
		if(pid < 0){
			fprintf(stderr, "accept_clone error\n");
		}
		else if(pid == 0){
			streaming_wav(client_fd);
			return 0;
		}
		else{
			close(client_fd);
		}
		/*
		//accept
		client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
		if (client_fd < 0){
		printf("accept error\n");
		}else{
		printf("connect success");
		streaming_wav(client_fd);
		}
		 */
	}
}


void streaming_wav(int client_fd){
	int fd = -1;                     /* Wav 파일을 위한 파일 디스크립터 */
	int rc, buf_size, dir;
	int channels, format;               /* 오디오 장치 설정을 위한 채널과 포맷 */ 
	long loops, count;
	unsigned int val;
	char buffer[BUFSIZ];               /* 오디오 출력을 위한 데이터 버퍼 */
	char wav_path[512];					/* filepath from DB */

	wavHeader wavheader;
	/* ALSA 장치 설정을 위한 구조체 */
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

	/* TODO : make socket to connect with DB? */
	struct sockaddr_in db_addr;
	int db_fd;
	socklen_t db_addr_len;

	//create socket
	db_fd = socket(PF_INET, SOCK_STREAM, 0); 
	if (db_fd < 0){ 
		perror("socket error : ");
		exit(1);
	}   

	memset(&db_addr, 0x00, sizeof(db_addr));
	db_addr.sin_family = AF_INET;
	db_addr.sin_addr.s_addr = inet_addr("127.0.0.2");
	db_addr.sin_port = htons(19998);
	db_addr_len = sizeof(db_addr);

	if (connect(db_fd, (struct sockaddr *)&db_addr, db_addr_len) < 0)
	{   
		perror("Connect error: ");
		exit(0);
	}   
    printf("db connection\n");

	while(1) {
		/* TODO : first get query from client */
		char query[BUFSIZ]; /* query for DB */
		if((count = read(client_fd, query, BUFSIZ) < 0) ){
            fprintf(stderr, "client fd read error\n");
		}
        printf("client read : %s\n", query);


		write(db_fd, query, BUFSIZ);
        memset(query, 0x00, sizeof(query));
		if((count = read(db_fd, query, 1) > 0) ){
		}
		printf("query : %s\n", query);
		if(strcmp("1", query) == 0){
			printf("play!!\n");
			write(client_fd, "1", 1);
		}else if(strcmp("0", query) == 0){
			printf("down!!\n");
			write(client_fd, "0", 1);
		}
		else{
			printf("query error\n");
		}
		memset(query, 0x00, sizeof(query));
		if((count = read(db_fd, query, BUFSIZ) < 0) ){
			printf("db read error\n");
		}
        printf("db request : %s\n", query);
		/* TODO : connect with DB finish */



		/* Wav 파일 열기 */
		//if((fd = open(query, O_RDONLY)) == -1) {
		if((fd = open(query, O_RDONLY)) == -1) {
			printf("Could not open the specified wave file : %s\n", query);
			return;
		}

		/* Wav 파일로 부터 헤더 읽어오기 */
		if((count = read(fd, &wavheader, sizeof(wavHeader))) < 1) {
			printf("Could not read wave data\n");
			return;
		}

		write(client_fd, (char *)&wavheader, sizeof(wavheader));
		while(read(fd, buffer, BUFSIZ) > 0){
			/* TODO : async read */
			if(recv(client_fd, buffer, BUFSIZ, MSG_DONTWAIT) != -1) {
				continue;
			}
			if(write(client_fd, buffer, BUFSIZ) < 0){
				printf("client closed\n");
			}
		}
	}
	close(db_fd);
	close(client_fd);
	}

	long accept_clone(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags, int *cli_fd)
	{
		return syscall(399, fd, addr, addrlen, flags, cli_fd);
	//	*cli_fd = accept(fd, addr, addrlen);
	//	return fork();
	}
