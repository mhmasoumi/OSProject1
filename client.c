#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <string.h>
#include <unistd.h> 

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;


void substr(char* destination, char* source, int start, int end) {
    for (int i = start; i != end && source[i] != '\n' && source[i] != '\0'; i++)
            *(destination + i - start) = source[i];
}


void runclient(unsigned short port) {

    int client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_fd < 0)
        printf("socket creation failed.\n");

    sockaddr_in server_addr_struct;
    server_addr_struct.sin_family = AF_INET;
    server_addr_struct.sin_port = htons(port);
    server_addr_struct.sin_addr.s_addr = INADDR_ANY;


    if (connect(client_socket_fd, (sockaddr*) &server_addr_struct, sizeof(server_addr_struct)) < 0)
        printf("connection to server failed.\n");
    else
        printf("Connected.\n");

    fd_set read_set, ready_set;
    FD_ZERO(&read_set);
    FD_ZERO(&ready_set);

    FD_SET(0, &read_set);
    FD_SET(client_socket_fd, &read_set);
    int maxfd = client_socket_fd;
    
    char to_server[256] = "username ", from_server[256], username[256];
    
    printf("To quit type <quit>. To help type <help>. Choose your username: ");
    fgets(username, 4098, stdin);
    strcat(to_server, username);
    write(client_socket_fd, to_server, sizeof(to_server));

    while(1) {
        ready_set = read_set;

        bzero(to_server, 256);
        bzero(from_server, 256);


        select(maxfd + 1, &ready_set, NULL, NULL,  NULL);

        if (FD_ISSET(0, &ready_set)) {
            fgets(to_server, 256, stdin);
            write(client_socket_fd, to_server, sizeof(to_server));
        }

        if (FD_ISSET(client_socket_fd, &ready_set)) {
            
            if (read(client_socket_fd, from_server, sizeof(from_server)) < 0)
                printf("error at receving data from serer.");
            
            if (strncmp(from_server, "groupport:", 10) == 0) {
                char group_port_str[256];
                substr(group_port_str, from_server, 11, 255);
                int group_port = atoi(group_port_str);
                sockaddr_in group_addr_struct;
                group_addr_struct.sin_family = AF_INET;
                group_addr_struct.sin_port = htons(group_port);
                group_addr_struct.sin_addr.s_addr = INADDR_ANY;

                printf("%d\n", group_port);

                close(client_socket_fd);
                client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                if (connect(client_socket_fd, (sockaddr*) &group_addr_struct, sizeof(group_addr_struct)) < 0)
                    printf("connection to group failed.\n");
                else
                    printf("Joined group.\n");
                
                

            }  
            else
                printf("%s\n", from_server);

        }
        



    }


    close(client_socket_fd);

}

int main(int argc, char* argv[])
{
    unsigned short port = argc > 1 ? atoi(argv[1]) : 9000;

    runclient(port);

    return 0;
}