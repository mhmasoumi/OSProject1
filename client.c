#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>


#include <netinet/in.h>
#include <string.h>
#include <unistd.h> 
#include <stdio.h>


typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int main(int argc, char* argv[])
{
    unsigned short port = 9000;

    int client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_fd < 0)
        perror("CLIENT: socket creation failed.");

    sockaddr_in server_addr_struct;
    server_addr_struct.sin_family = AF_INET;
    server_addr_struct.sin_port = htons(port);
    server_addr_struct.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_socket_fd, (sockaddr*) &server_addr_struct, sizeof(server_addr_struct)) < 0)
        perror("CLIENT: connection failed.");
    
    //everything else
    char *msg = "No was here!";
    int len, bytes_sent;

    bytes_sent = send(client_socket_fd, msg, strlen(msg), 0);

    printf("%d", bytes_sent);

    getchar();


    close(client_socket_fd);


    return 0;
}