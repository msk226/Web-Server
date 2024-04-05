/* 
   A simple server in the internet domain using TCP
   Usage:./server port (E.g. ./server 10000 )
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define MAX_BUFFER_SIZE 2048

void error(char *msg)
{
    perror(msg);
    exit(1);
}

char * get_file_type(char * url){

    size_t url_len = strlen(url);
    for (int i = url_len - 1; i >= 0; i--) {
        if (url[i] == '.') {
            // '.' 이후의 문자열을 추출하여 파일 확장자로 설정
            const char* extension = &url[i + 1];

            // 추출된 확장자가 이미지 파일 확장자인지 확인
            if (strcasecmp(extension, "jpg") == 0) {
                return "image/jpeg";
            } else if (strcasecmp(extension, "png") == 0) {
                return "image/png";
            } else if (strcasecmp(extension, "gif") == 0) {
                return "image/gif";
            } else if (strcasecmp(extension, "pdf") == 0) {
                return "application/pdf";
            } else if (strcasecmp(extension, "mp3") == 0) {
                return "audio/mpeg";
            } else {
                return NULL; // 지원하지 않는 확장자인 경우
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]){

    int socketfd, new_socketfd; //descriptors return from socket and accept system calls
    int portno; // port number
    int n;
    socklen_t clilen;

    char buffer[MAX_BUFFER_SIZE];
    char *token, *url; 
    
    char file_path[256];
    char response[1024]; 
    
     
    /*sockaddr_in: Structure Containing an Internet Address*/
    struct sockaddr_in serv_addr, cli_addr;

     if (argc < 2) // 만약 포트 번호 입력 하지 않았다면, 에러 발생
     {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
     }

    socketfd = socket(AF_INET, SOCK_STREAM, 0); // 소켓 생성

    if (socketfd < 0) // 만약 소켓이 정상적으로 생성 되지 않았다면, 에러 발생
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr)); // serv_addr의 메모리를 0으로 초기화 한다. 

    portno = atoi(argv[1]); //atoi converts from String to Integer  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) // 소켓 바인딩 -> 소켓에 IP 정보와 포트 번호 부여
        error("ERROR on binding");

    listen(socketfd, 5); // Listen for socket connections. Backlog queue (connections to wait) is 5

    printf("Server: Waiting for client's connection on port %d...\n", portno);

    while(1){

        clilen = sizeof(cli_addr);

        new_socketfd = accept(socketfd, (struct sockaddr *)&cli_addr, &clilen);
        if (new_socketfd < 0)
            error("ERROR on accept");

        memset(buffer, 0, sizeof(buffer));

        n = read(new_socketfd, buffer, 2048);
        if (n < 0) error("ERROR reading from socket");  
        //printf("HTTP Request is %s  \n",buffer);

        // HTTP 요청에서 첫 번째 라인 추출
        token = strtok(buffer, "\n");

        // 첫 번째 라인에서 URL 추출
        if (token != NULL) {
        // 두 번째 토큰이 URL
        token = strtok(token, " ");
        token = strtok(NULL, " ");
        if (token != NULL) {
            url = token;
            }
        }

        memset(file_path, 0, sizeof(file_path));
        memset(response, 0, sizeof(response));

        sprintf(file_path, "./files%s", url);

        //printf("Requested file's Path : %s\n", file_path);
        // 파일 있는지 확인 후 실행. 만약 파일 없으면, 404 던지도록. 
        char *file_name = url;
        if (access(file_name, F_OK) != -1) {

        }
        int fd = open(file_path, O_RDONLY);

        if (fd < 0){ // 여기를 500 에러로 바꾸기
            sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
            printf("Response message : \n%s", response);
            send(new_socketfd, response, strlen(response), 0);
        } 
        else{
            // 헤더 요소들 더 추가하기. 
            sprintf(response,"HTTP/1.1 200 OK\r\nContent-Type:%s\r\n\r\n", get_file_type(file_path));
            printf("Response message : \n%s", response);
            send(new_socketfd, response, strlen(response), 0);
            char file_buffer[MAX_BUFFER_SIZE];
            while(read(fd, file_buffer, sizeof(file_buffer)) != -1){
                write(new_socketfd, file_buffer, sizeof(file_buffer));
            }
            close(fd);
        }
        close(new_socketfd);
    }
    close(socketfd);

}
