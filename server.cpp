/*
 CSE-4153 - DATA COMMUNICATION NETWORKS: Programming Assignment 1

 * Author:
   Name: Ishan Taldekar
   Net-id: iut1
   Student-id: 903212069

 * Description:
   A basic client server program where TCP is used to perform a handshake and UDP is used to transfer data.

 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sstream>
#include <string>
using namespace std;

#define BACKLOG 10  // the number of pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once

int main(int argc, char const *argv[]) {

    int socket_fd, new_connection_fd = 0;  // information on sockets (listening on socket_fd, new connection on new_connection_fd)
    int socket_bind_status, num_bytes, r_port_value;
    struct addrinfo hints, *server_info, *p;
    int getaddrinfo_call_status;
    socklen_t sin_size, addr_len;
    int yes = 1;
    string r_port;
    struct sockaddr_in client_addr;  // connector's address information
    char buffer[MAXDATASIZE];  // character buffer for incoming data
    char payload[4];  // character array for outgoing data
    bool end_of_file_flag = false;
    char* n_port;  // negotiation port
    FILE* destination_file;  // file pointer

    /* ensure that the <negotiation_port> is provided and available at run-time */
    if (argc != 2) {
        fprintf(stderr, "usage: server <n_port> \n");
        exit(EXIT_FAILURE);
    }

    n_port = (char *)argv[1];  // the negotiation port should be the second given argument (index: 1)

    /* loading up address structs with getaddrinfo() */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP sockets
    hints.ai_flags = AI_PASSIVE;  // fill in my IP for me

    /* get information about a host name and/or service and load up a struct sockaddr with the result */
    if ((getaddrinfo_call_status = getaddrinfo(NULL, n_port, &hints, &server_info)) != 0) {  // checking for errors
        fprintf(stderr, "(server) error when calling getaddrinfo: %s\n", gai_strerror(getaddrinfo_call_status));
        exit(EXIT_FAILURE);
    }

    /* loop through all the results and bind to the first port we can */
    for (p = server_info; p != NULL; p = p->ai_next) {

        /* make a socket using socket() call, and ensure that it ran error-free */
        if ((socket_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol)) == -1) {
            perror("(server) error during socket creation");
            continue;
        }  // alternative/old way: socket_fd = socket(PF_INET, SOCK_STREAM, 0);

        /* helps avoid the "Address already in use" error message */
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("(server) error when calling setsockopt");
            exit(EXIT_FAILURE);
        }

        /* associate a socket with an IP address and port number using the bind() call, and make sure it ran error-free */
        if ((socket_bind_status = bind(socket_fd, server_info->ai_addr, server_info->ai_addrlen)) == -1) {
            close(socket_fd);
            perror("(server) error during socket binding");
            continue;
        }

        break;

    }

    freeaddrinfo(server_info);  // the server_info structure is no longer needed

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(EXIT_FAILURE);
    }

    /* tell the socket to 'listen' for incoming connections */
    if (listen(socket_fd, BACKLOG) == -1) {
        perror("(server) error when calling listen");
        exit(EXIT_FAILURE);
    }

    while(true) {
    
        sin_size = sizeof client_addr;  // holds the size of the client address

        /* accept an incoming connection on a listening socket (error-checking included) */
        if ((new_connection_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &sin_size)) == -1) {
            perror("(server) error when accepting incoming connection");
            continue;
        }

        /* receive data on a socket */
        if ((num_bytes = recv(new_connection_fd, buffer, MAXDATASIZE-1, 0)) == -1) {
            perror("(server) error when receiving request");
            exit(EXIT_FAILURE);
        }

        buffer[num_bytes] = '\0';

        close(socket_fd);  // close the socket descriptor, no longer needed.

        /* 
        verify that the client is initiating a handshake, and then return a random, valid port number
        for further communication if handshake initiated.
        */
        if (strcmp(buffer, "117") == 0) {

            srand(time(NULL));
            r_port_value = rand() % 64512 + 1024;
            stringstream ss;
            ss << r_port_value;
            r_port = ss.str();

            if (send(new_connection_fd, r_port.c_str(), r_port.length(), 0) == -1) {
                perror("(client) error when sending request");
            }

        }

        printf("Handshake detected. Selected a random port %d\n", r_port_value);

        close(new_connection_fd);  // server closes socket / socket descriptor once handshake stage is complete.
        break;

    }

    /* server next prepares for a transaction using UDP sockets */

    /* loading up address structs with getaddrinfo() */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP sockets
    hints.ai_flags = AI_PASSIVE;  // fill in my IP for me

    /* get information about a host name and/or service and load up a struct sockaddr with the result */
    if ((getaddrinfo_call_status = getaddrinfo(NULL, r_port.c_str(), &hints, &server_info)) != 0) {
        fprintf(stderr, "(server) error when calling getaddrinfo - UDP: %s\n", gai_strerror(getaddrinfo_call_status));
        exit(EXIT_FAILURE);
    }

    /* loop through all the results and bind to the first we can */
    for(p = server_info; p != NULL; p = p->ai_next) {

        /* make a socket using socket() call, and ensure that it ran error-free */
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("(server) error during socket creation - UDP");
            continue;
        }

        /* helps avoid the "Address already in use" error message */
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("(server) error when calling setsockopt");
            exit(EXIT_FAILURE);
        }

        /* associate a socket with an IP address and port number using the bind() call, and make sure it ran error-free */
        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("(server) error when binding - UDP");
            continue;
        }

        break;

    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket (UDP)\n");
        return 2;
    }

    freeaddrinfo(server_info);  // the server_info structure is no longer needed

    /*
    open a file named 'dataReceived.txt' in 'w+' mode
    w+' mode stands for write/update: Create an empty file and open it for update
    (both for input and output). If a file with the same name already exists its contents
    are discarded and the file is treated as a new empty file.
    */
    destination_file = fopen("dataReceived.txt", "w+");

    /* loop until EOF encountered at the end of the file */
    while(!end_of_file_flag) {

        /* receive a packet on the given socket */
        addr_len = sizeof client_addr;
        if ((num_bytes = recvfrom(socket_fd, buffer, MAXDATASIZE - 1, MSG_WAITFORONE, (struct sockaddr *) &client_addr,
                                  &addr_len)) == -1) {
            perror("(server) error when calling recvfrom - UDP");
            exit(EXIT_FAILURE);
        }

        buffer[num_bytes] = '\0';

        /* write each incoming packet to the file 'dataReceived.txt' */
        for (int i = 0; i < 4; i++) {

            /*
            if EOF is encountered somewhere in the middle of a packet, pad the rest of the payload
            (which will be sent back as an acknowledgement) with null characters to maintain data
            integrity.
            */
            if (buffer[i] == EOF) {

                for (int j = i; j < 4; j++) {
                    payload[j] = '\0';
                }

                end_of_file_flag = true;
                break;

            }

            fputc(buffer[i], destination_file);  // write each character in packet to the file
            payload[i] = toupper(buffer[i]);  // convert each character to upper-case to send back as acknowledgement
        }

        /* send data out over a socket (error-checking included) */
        if ((num_bytes = sendto(socket_fd, payload, 4, MSG_CONFIRM, (sockaddr *) &client_addr, sizeof(client_addr))) ==
            -1) {
            perror("(client) error when calling sendto - UDP");
            exit(EXIT_FAILURE);
        }
    }

    fclose(destination_file);  // file pointer is disassociated from the file
    close(socket_fd);  // close socket descriptor

    return 0;
}
