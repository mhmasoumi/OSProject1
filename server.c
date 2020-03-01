#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>

#include <string.h>
#include <unistd.h> 
#include <stdio.h>

#define PENDING_NUM 5


typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int main(int argc, char* argv[]) {
    
    unsigned short port = 9000;

    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0)
        perror("SERVER: socket creation failed.");

    // lose the pesky "address already in use" error message
    int yes = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));


    sockaddr_in server_addr_struct;
    memset(&server_addr_struct, 0, sizeof(server_addr_struct));
    server_addr_struct.sin_family = AF_INET;
    server_addr_struct.sin_port = htons(port);
    server_addr_struct.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket_fd, (sockaddr*) &server_addr_struct, sizeof(server_addr_struct)) < 0)
        perror("SERVER: binding failed.");

    if (listen(server_socket_fd, PENDING_NUM) < 0)
        perror("SERVER: listening failed.");
    
    int clilen = sizeof(server_addr_struct);
    int newsockfd = accept(server_socket_fd, (sockaddr*)&server_addr_struct, &clilen);

    getchar();

    char buf[100];

    int bytes_recieved = recv(newsockfd, buf, sizeof(buf), 0);

    printf("%s", buf);

    close(server_socket_fd);

    return 0;
}