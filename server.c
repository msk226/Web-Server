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

/* The get_file_type function is a function that extracts the file extension from the URL received as input and returns it.
 * - param: url: Pointer to a string representing the URL of the requested file.
 * - return: Returns a pointer to a string representing the extension of the file. If not supported, NULL is returned.
 *
 * This function currently supports JPEG, PNG, GIF, PDF and MP3 file extensions.
 */

char * get_file_type(char * url){

    size_t url_len = strlen(url); // Save the length of the entered URL. 
    for (int i = url_len - 1; i >= 0; i--) { // Check the URL from behind.
        if (url[i] == '.') {
            //  Extract the string after the character '.' and set it as the file extension
            const char* extension = &url[i + 1];

            // Check what file extension the extracted extension is.
            if (strcasecmp(extension, "jpeg") == 0) {
                return "image/jpeg";
            } else if (strcasecmp(extension, "png") == 0) {
                return "image/png";
            } else if (strcasecmp(extension, "gif") == 0) {
                return "image/gif";
            } else if (strcasecmp(extension, "pdf") == 0) {
                return "application/pdf";
            } else if (strcasecmp(extension, "mp3") == 0) {
                return "audio/mpeg";
            } else if (strcasecmp(extension, "html") == 0){
                return "text/html";
            } 
            else {
                return NULL; // Return NULL if the extension is not supported
            }
        }
    }
    return NULL; // If there is no extension, NULL is returned. 
}

int main(int argc, char *argv[]){

    int socketfd, client_socket; //descriptors return from socket and accept system calls
    int portno; // port number
    int n;
    socklen_t clilen;

    char buffer[MAX_BUFFER_SIZE]; 
    char *token, *url; 
    
    char file_path[256]; // Create a string to store the requested file path
    char response[1024];  // Create a string to store the response message
     
    /*sockaddr_in: Structure Containing an Internet Address*/
    struct sockaddr_in serv_addr, cli_addr;

     if (argc < 2) // If the port number is not entered, an error occurs
     {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
     }

    socketfd = socket(AF_INET, SOCK_STREAM, 0); // create socket

    if (socketfd < 0) // If the socket was not created properly, an error occurs
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr)); // Initialize the memory of serv_addr to 0.

    portno = atoi(argv[1]); //atoi converts from String to Integer  
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) // Socket binding -> Give IP information and port number to the socket
        error("ERROR on binding");

    listen(socketfd, 5); // Listen for socket connections. Backlog queue (connections to wait) is 5

    printf("Server: Waiting for client's connection on port %d...\n", portno);

    while(1){ // Continue receiving requests from clients in an infinite loop

        clilen = sizeof(cli_addr); // Set the client's address length

        client_socket = accept(socketfd, (struct sockaddr *)&cli_addr, &clilen); // Receive the client's request and create a socket
        if (client_socket < 0) // If the socket was not created successfully, an error occurs
            error("ERROR on accept");

        memset(buffer, 0, sizeof(buffer)); // initialize buffer

        n = read(client_socket, buffer, 2048); // Read data from client.
        if (n < 0) error("ERROR reading from socket");  // If data is not entered correctly, an error occurs
        printf("HTTP Request is %s  \n",buffer); // Request message output

        // Extract first line from HTTP request
        token = strtok(buffer, "\n");

        // Extract URL from first line
        if (token != NULL) {
        // second token is URL
        token = strtok(token, " ");
        token = strtok(NULL, " ");
        if (token != NULL) {
            url = token;
            }
        }

        memset(file_path, 0, sizeof(file_path)); // file_path memory area initialization
        memset(response, 0, sizeof(response)); // Initialize response memory area

        sprintf(file_path, "./files%s", url); // Set the file_path corresponding to the requested URL

        char *file_name = url; // set file name

        // If the file corresponding to the input file name does not exist, a 404 error response is generated.
        if (access(file_path, F_OK) == -1) {
            sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
            printf("Response message : \n%s", response);
            send(client_socket, response, strlen(response), 0);
        }
        // If the file exists
        else{
            int fd = open(file_path, O_RDONLY); // Open the file read-only.

        if (fd < 0){ // If the file cannot be opened properly, a 500 error response is generated.
            sprintf(response, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n");
            printf("Response message : \n%s", response);
            send(client_socket, response, strlen(response), 0);
        } 
        else{ // If the file is opened normally
            
            time_t raw_time; // Get current date and time
            struct tm *info;
            time(&raw_time);
            info = localtime(&raw_time);

            // Remove newline characters from the string returned from the asctime() function
            char* date_str = asctime(info);
            date_str[strcspn(date_str, "\n")] = '\0';

            // Create response message
            sprintf(response, "HTTP/1.1 200 OK\r\n"
                      "Content-Type: %s\r\n"
                      "Connection: close\r\n"
                      "Date: %s\r\n\r\n",
            get_file_type(file_path), date_str);
            printf("Response message : \n%s", response);
            send(client_socket, response, strlen(response), 0); 

            // Allocate memory for file_buffer dynamically
            off_t file_size = lseek(fd, 0, SEEK_END);
            lseek(fd, 0, SEEK_SET);
            
            char *file_buffer = (char *)malloc(file_size); 
            ssize_t bytes_read;
            while ((bytes_read = read(fd, file_buffer, file_size)) > 0) {
                write(client_socket, file_buffer, bytes_read);
            }
            // Free dynamically allocated memory
            free(file_buffer);

            close(fd); // close file
        }
            
        } close(client_socket); // Close client socket
        
    }
    close(socketfd); // Close server socket

}
