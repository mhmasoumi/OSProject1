#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h> 
#include <fcntl.h>

#define PENDING_NUM 5
#define MAX_CLIENT_NUM 40
#define MAX_GROUP_NUM 20

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;


typedef struct {
    int socket_fd;
    char username[256];
} Client;

typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int maxi;
    Client clients[FD_SETSIZE];
    int max_port;
} Pool;

typedef struct {
    char name[256];
    int socket;
    sockaddr_in addr_struct;
    Client clients[MAX_CLIENT_NUM];
    int maxi;
    int port;
} Group;


void substr(char* destination, char* source, int start, int end) {
    for (int i = start; source[i] != '\n' && source[i] != '\0' && i != end; i++)
            *(destination + i - start) = source[i];
}

void handle_input(char* buf, int client_fd, Pool* pool, Group* groups[]) {
   
    if (strncmp(buf, "username", 8) == 0) {
        for (int i = 0; i < pool->maxi; i++)
            if (pool->clients[i].socket_fd == client_fd) {
                substr(pool->clients[i].username, buf, 9, 255);
                break;
            }
        printf("Username is assigned to a client.\n");
        write(client_fd, "Username assigned sucessfully.\n", 256);
    }
    else if (strncmp(buf, "pvchat", 6) == 0) {
        
    } 
    else if (strncmp(buf, "listgroups", 10) == 0) {
        char to_client[256];
        for(int i = 0; i < MAX_GROUP_NUM; i++)
            if (groups[i] != NULL) {
                strcat(to_client, groups[i]->name);
                strcat(to_client, "\n");
            }
        write(client_fd, to_client, 256);
    } 
    else if (strncmp(buf, "creategroup", 11) == 0) {
        int all_busy = 1;

        for (int i = 0; i < MAX_GROUP_NUM; i++) {
            if (groups[i] == NULL) {
                Group new_group;
                substr(new_group.name, buf, 12, 255);
                
                new_group.socket = socket(AF_INET, SOCK_STREAM, 0);

                // lose the pesky "address already in use" error message
                int yes = 1;
                setsockopt(new_group.socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                sockaddr_in addr_struct;
                new_group.addr_struct = addr_struct;
                new_group.addr_struct.sin_family = AF_INET;
                pool->max_port++;
                new_group.addr_struct.sin_port = htons(pool->max_port);
                new_group.addr_struct.sin_addr.s_addr = INADDR_ANY;
                new_group.port = pool->max_port;
                new_group.maxi = 0;

                int opts = 1;
                if ((opts = fcntl(new_group.socket, F_GETFL)) < 0) 
                    printf("SERVER: ERORR\n");
                
                opts = (opts | O_NONBLOCK); 
                if (fcntl(new_group.socket, F_SETFL, opts) < 0)
                    printf("SERVER: ERROR\n");

                if (bind(new_group.socket, (sockaddr*) &new_group.addr_struct, sizeof(new_group.addr_struct)) < 0)
                    printf("SERVER: Group binding failed.\n");

                if (listen(new_group.socket, PENDING_NUM) < 0)
                    printf("SERVER: Group listening failed.\n");

                groups[i] = &new_group;
                all_busy = 0;

                FD_SET(groups[i]->socket, &pool->read_set); 
                if (groups[i]->socket > pool->maxfd)
                    pool->maxfd = groups[i]->socket;
                
                printf("New group: %s is created.\n", groups[i]->name);
                write(client_fd, "Group created successfully.", 256);

                break;
            }
        }

        if (all_busy == 1) {
            printf("Group creation failed due to number of current groups.\n");
            write(client_fd, "Server: No room for any new group", 256);
        }
        
    } 
    else if (strncmp(buf, "joingroup", 9) == 0) {
        char group_name[256];
        substr(group_name, buf, 10, 255);
        int group_found = 0;

        for (int i = 0; i < MAX_GROUP_NUM; i++)
            if (groups[i] != NULL)
                if (strcmp(groups[i]->name, group_name) == 0) {
                    char port_str[256];
                    sprintf(port_str, "groupport: %d", groups[i]->port);
                    write(client_fd, port_str, 256);
                    group_found = 1;
                    break;
                }

        if (group_found == 0) {
            printf("User requested joining non-existing group\n");
            write(client_fd, "Group not found.", 256);
        }

    } 
}

void add_client(int new_socket, Pool* pool) {
    if (new_socket > pool->maxfd)
        pool->maxfd = new_socket;
    
    FD_SET(new_socket ,&pool->read_set);

    Client new_client;
    new_client.socket_fd = new_socket;
    pool->clients[pool->maxi] = new_client;
    pool->maxi++;
}


void check_clients(Pool* pool, Group* groups[]) {

    char buf[256];
    for(int i = 0; i < pool->maxi; i++)
        if(FD_ISSET(pool->clients[i].socket_fd, &pool->ready_set))
            if ((read(pool->clients[i].socket_fd, buf, 256)) == 0) {
                FD_CLR(pool->clients[i].socket_fd, &pool->read_set);
                close(pool->clients[i].socket_fd);
            }
            else
                handle_input(buf, pool->clients[i].socket_fd, pool, groups);
        
}

void check_group_clients(Pool* pool, Group* groups[]) {
    for (int i = 0; i < MAX_GROUP_NUM; i++) {
        if (groups[i] != NULL) {
            if (FD_ISSET(groups[i]->socket, &pool->ready_set)) {  
                int clen2 = sizeof(groups[i]->addr_struct);
                int new_group_client_socket = accept(groups[i]->socket, (sockaddr*) &groups[i]->addr_struct, &clen2);
                if (new_group_client_socket > pool->maxfd)
                    pool->maxfd = new_group_client_socket;
                FD_SET(new_group_client_socket ,&pool->read_set);
                Client new_group_clinet;
                new_group_clinet.socket_fd = new_group_client_socket;
                groups[i]->clients[groups[i]->maxi] = new_group_clinet;
                groups[i]->maxi++;
            }

            for (int j = 0; j < groups[i]->maxi; j++)
                if (FD_ISSET(groups[i]->clients[j].socket_fd, &pool->ready_set)) {
                    char buf[256];
                    if ((read(groups[i]->clients[j].socket_fd, buf, 256)) == 0) {
                        FD_CLR(groups[i]->clients[j].socket_fd, &pool->read_set);
                        close(groups[i]->clients[j].socket_fd);
                    }
                    else 
                        for (int l = 0; l < groups[i]->maxi; l++)
                            if (groups[i]->clients[l].socket_fd != groups[i]->clients[j].socket_fd)
                                write(groups[i]->clients[l].socket_fd, buf, strlen(buf));
                }
        }
    }
}

void runserver(unsigned short port) {

    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0)
        printf("Server socket creation failed.");

    // lose the pesky "address already in use" error message
    int yes = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));


    sockaddr_in server_addr_struct;
    server_addr_struct.sin_family = AF_INET;
    server_addr_struct.sin_port = htons(port);
    server_addr_struct.sin_addr.s_addr = INADDR_ANY;

    int opts = 1;
    if ((opts = fcntl(server_socket_fd, F_GETFL)) < 0) 
        printf("SERVER: ERORR\n");
    
    opts = (opts | O_NONBLOCK); 
    if (fcntl(server_socket_fd, F_SETFL, opts) < 0)
        printf("SERVER: ERROR\n");

    if (bind(server_socket_fd, (sockaddr*) &server_addr_struct, sizeof(server_addr_struct)) < 0)
        printf("Server binding failed.\n");

    if (listen(server_socket_fd, PENDING_NUM) < 0)
        printf("Server listening failed.\n");

    printf("Server Started.\n");

    int clen = sizeof(server_addr_struct);
    Pool pool;
    FD_ZERO(&pool.read_set);
    FD_ZERO(&pool.ready_set);
    FD_SET(server_socket_fd, &pool.read_set);
    pool.maxfd = server_socket_fd;
    pool.maxi = 0;
    pool.max_port = port;


    Group* groups[MAX_GROUP_NUM];

    while (1) { 
    
        pool.ready_set = pool.read_set; 
       
        select(pool.maxfd + 1, &pool.ready_set, NULL, NULL,  NULL);

        if (FD_ISSET(server_socket_fd, &pool.ready_set)) { 
            int new_socket = accept(server_socket_fd, (sockaddr*) &server_addr_struct , &clen);

            add_client(new_socket, &pool);

        }
        
        check_clients(&pool, groups);
        //check_group_clients(&pool, groups);

    }

}



int main(int argc, char* argv[]) {

    unsigned short port = (argc > 1) ? atoi(argv[1]) : 9000;

    runserver(port);

    return 0;
}