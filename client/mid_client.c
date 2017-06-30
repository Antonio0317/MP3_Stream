#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include "wavFile.h"

#define EXIT_WAV 1
#define PLAY_WAV 2
#define PAUSE_WAV 3
#define UNPAUSE_WAV 4

wavHeader wavheader;
int client_sockfd;
int sig_check;
int play_wav_chk = 0;
int rec_fd;
char query[BUFSIZ]; /* for query */
char *IP;
int is_playing = 0;

void signal_handler(int signo) {
    int fd;
    char buf[500];
    int pid;
    memset(buf, 0x00, sizeof(buf));
    memset(query, 0x00, sizeof(buf));
	/* TODO : get input from GOOGLE SPEECH here? */
	/* 
	   get_speech(); 
	 */
    //system("mplayer ask.mp3");
    pid = fork();
    if(pid == 0){
        system("mpg123 ask.mp3");
        exit(1);
    }
    sleep(1);
    system("python record_transcribe.py record.flac");
    fd = open("text", O_RDONLY);
    fd = read(fd, buf, sizeof(buf));
    strcpy(query, buf);
    close(fd);
    system("rm -f text");

//    buf[strlen(buf)] = '\n';
       printf("%s\n", buf);

    int order;
    if(strcmp(buf, "일시 정지") == 0 || strcmp(buf, "일시정지") == 0 || strcmp(buf, "일 시 정 지") == 0){
        order = PAUSE_WAV;
    }
    else if(strcmp(buf, "다시틀어줘") == 0 || strcmp(buf, "다시 틀어줘") == 0 || 
            strcmp(buf, "다 시 틀 어 줘") == 0 || strcmp(buf, "다시 틀어 줘") == 0 || 
            strcmp(buf, "다시 틀 어 줘") == 0){
        order = UNPAUSE_WAV;
    }
    else if(strcmp(buf, "정지") == 0 || strcmp(buf, "정 지") == 0){
        order = EXIT_WAV;
    }
    else if(strcmp(buf, "종료") == 0 || strcmp(buf, "종 료") == 0 || strcmp(buf, "exit") == 0 ||
            strcmp(buf, "종묘") == 0 || strcmp(buf, "종 묘") == 0){
        exit(1);
    }
    else{
        if(sig_check == 1)
            sig_check = 3;
        order = PLAY_WAV;
    }

    switch(order){
        case EXIT_WAV:
            sig_check = 4;
            break;
        case PLAY_WAV:
//            strcpy(query, "Psy new fase");
//            strcpy(query, "싸이 뉴페이스");
 //           printf("%s\n", query);
            if(is_playing == 1)
                sig_check = 3;
            play_wav_chk = 1;
            break;
        case PAUSE_WAV:
            sig_check = 1;
            break;
        case UNPAUSE_WAV:
            sig_check = 0;
            break;
        case 5:
            break;
        default:
            exit(1);
    }
}

int main(int argc, char **argv)
{

    /*
	int client_len;

	struct sockaddr_in client_addr;

    //server connect
	if((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error : ");
	}
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(argv[1]);
	client_addr.sin_port = htons(19999);

	client_len = sizeof(client_addr);

	if (connect(client_sockfd, (struct sockaddr *)&client_addr, client_len) < 0)
	{
		perror("Connect error: ");
		exit(0);
	}
    printf("server connect\n");
    */
    IP = argv[1];

    //signal
	struct sigaction act_new;
	struct sigaction act_old;

	act_new.sa_handler = signal_handler;
	sigemptyset(&act_new.sa_mask);

	sigaction(SIGINT, &act_new, &act_old);

	while(1){
        if(play_wav_chk == 1){
            play_wav_chk = 0;
            printf("play_wav()\n");
            play_wav();
        }
    }

	return 0;
}

int play_wav() {   
	int rc, buf_size, dir;
	int channels, format;                                      /* 오디오 장치 설정을 위한 채널과 포맷 */
	long loops, count;
	unsigned int val;
	char *buffer;                                                   /* 오디오 출력을 위한 데이터 버퍼 */
    int client_len;
    char tmp[10];

	struct sockaddr_in client_addr;

    is_playing = 1;
    sig_check = 0;
    close(client_sockfd);
    //server connect
	if((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error : ");
	}
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(IP);
	client_addr.sin_port = htons(19999);

	client_len = sizeof(client_addr);

	if (connect(client_sockfd, (struct sockaddr *)&client_addr, client_len) < 0)
	{
		perror("Connect error: ");
		exit(0);
	}
    printf("server connect\n");


	/* ALSA 장치 설정을 위한 구조체 */
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

	/* TODO : get input from GOOGLE SPEECH here? */
	/* 
	   using SIGUSR1
	   get_speech(); 
	 */
	write(client_sockfd, query, BUFSIZ);

    memset(tmp, 0x00, sizeof(tmp));
    read(client_sockfd, tmp, 1);
    if(strcmp(tmp, "1") == 0){
    }
    else{
        system("mpg123 update.mp3");
    }

	read(client_sockfd, (char *)&wavheader, sizeof(wavheader));
    system("mpg123 play.mp3");

	/* 출력을 위한 ALSA PCM 장치 열기 */
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if(rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		return -1;
	}

	/* 오디오 디바이스에 하드웨어 파라미터 설정을 위한 공간 확보 */
	snd_pcm_hw_params_alloca(&params);

	/* 기본 값으로 초기화 */
	snd_pcm_hw_params_any(handle, params);

	/* 헤더에서 채널에 대한 정보를 가져와서 출력하고 설정하기 */
	channels = wavheader.nChannels;
	printf("Wave Channel Mode : %s\n", (channels)? "Stereo":"Mono");
	snd_pcm_hw_params_set_channels(handle, params, channels);

	/* 오디오 포맷 설정 */
	printf("Wave Bytes : %d\n", wavheader.nblockAlign);
	switch(wavheader.nblockAlign) {
		case 1:                                   /* 모노 8비트 */
			format = SND_PCM_FORMAT_U8; 
			break;
		case 2:                                   /* 모노 16비트 이거나 스테레오 8비트 */
			format = (!channels)?SND_PCM_FORMAT_S16_LE:SND_PCM_FORMAT_U8;
			break;
		case 4:                                    /* 스테레오 파일인 경우 */
			format = SND_PCM_FORMAT_S16_LE;
			break;
		default:
			printf("Unknown byte rate for sound\n");
			break;
	};

	/* 인터리브드 모드로 설정 */
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	/* 오디오의 포맷 설정 */
	snd_pcm_hw_params_set_format(handle, params, format);


	/* Wav 파일의 헤더에서 정보를 가져와 오디오의 샘플링 레이트 설정 */
	printf("Wave Sampling Rate : 0x%d\n", wavheader.sampleRate);
	val = wavheader.sampleRate;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

	/* 출력을 위한 32 개의 프레임 설정 */
	frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	/* ALSA 드라이버에 하드웨어 파리미터 적용 */
	rc = snd_pcm_hw_params(handle, params);
	if(rc < 0) { 
		fprintf(stderr, "Unable to set hw parameters: %s\n", snd_strerror(rc));
		return -1;
	}

	/* 하나의 주기에 필요한 가장 큰 버퍼의 사이즈 얻기 */
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	/* 버퍼의 크기 =  프레임의 수 x 채널 x 바이트/샘플 */
	buf_size = frames * channels * ((format == SND_PCM_FORMAT_S16_LE)?2:1); 
	/* 출력을 위한 버퍼 공간 확보 */
	buffer = (char*)malloc(buf_size);

	/* ALSA의 주기 시간을 가져오기 */
	snd_pcm_hw_params_get_period_time(params, &val, &dir); 

	while(count = read(client_sockfd, buffer, buf_size))
	{
		if(sig_check == 1) {
            while(1){
                if(sig_check == 0)
                    break;
                else if(sig_check == 3 || sig_check == 4)
                    goto end;
            }
		}
		if(sig_check == 2) {
			break;
		}
		if(sig_check == 3) {
            goto end;
		}
		if(sig_check == 4) {
            break;
		}
		if(count <= 0)
			break;
		rc = snd_pcm_writei(handle, buffer, frames);
		if(rc == -EPIPE) {
			/* 언더런(EPIPE)을 경우의 처리 */
			fprintf(stderr, "Underrun occurred\n");
			snd_pcm_prepare(handle);
		} else if(rc < 0) {                                /* 에러가 발생했을 때의 처리 */
			fprintf(stderr, "error from write: %s\n", snd_strerror(rc));
		} else if(rc != (int)frames) {
			fprintf(stderr, "short write, write %d frames\n", rc);
		}
	}

end:

    close(client_sockfd);
	snd_pcm_drain(handle);
	snd_pcm_close(handle);                  /* 사용이 끝난 장치 닫기 */

	free(buffer);

    is_playing = 0;
    printf("play end\n");
}
