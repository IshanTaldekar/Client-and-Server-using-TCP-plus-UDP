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
#include <netdb.h>
#include <iostream>
using namespace std;

#define MAXDATASIZE 100  // max number of bytes that can be sent and received

/* handles the client-side of the initial tcp-handshake stage */
int tcp_handshake(char* host_name, char* negotiation_port, char* random_port)
{
    int socket_fd;  // stores information about socket
    struct addrinfo hints, *server_info, *p;
    int getaddrinfo_call_status;
    int num_bytes;
    char buffer[MAXDATASIZE];  

    /* loading up address structs with getaddrinfo() */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP sockets

    /* get information about a host name and/or service and load up a struct sockaddr with the result */
    if ((getaddrinfo_call_status = getaddrinfo(host_name, negotiation_port, &hints, &server_info)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_call_status));
        return -1;
    }

    /* loop through all the results and connect to the first we can */
    for(p = server_info; p != NULL; p = p->ai_next) {
        /* make a socket using socket() call, and ensure that it ran error-free */
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("(client) error during socket creation");
            continue;
        }

        /* connect a socket to a server */
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("(client) error when connecting to host");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -1;
    }

    freeaddrinfo(server_info);  // the server_info structure is no longer needed

    /* initiate a handshake with the server */
    if (send(socket_fd, "117", 3, 0) == -1) {
        perror("(client) error when sending request");
    } else {
        
        /* if handshake successful, the server will send back a random port number */
        if ((num_bytes = recv(socket_fd, buffer, MAXDATASIZE-1, 0)) == -1) {
            perror("(server) error when receiving request");
            return -1;
        }

        buffer[num_bytes] = '\0';
        strcpy(random_port, buffer);  // pass the value of the random port (by reference) to the calling function
    }

    close(socket_fd);  // close the socket / socket descriptor once the handshake stage is complete.
    return 0;
}

/* handles the client-side of the UDP transaction */
int udp_transaction(char* host_name, char* random_port, char* file_name) {
    int socket_fd, getaddrinfo_call_status, num_bytes;
    struct addrinfo hints, *server_info, *p;
    char c;
    char payload[4];
    char buffer[MAXDATASIZE];
    int counter = 0;
    int auth_status = 1;
    struct sockaddr_in incoming_connection_addr;
    socklen_t addr_len;
    FILE* source_file;  // file pointer

    /* loading up address structs with getaddrinfo() */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_DGRAM;  // UDP sockets

    /* get information about a host name and/or service and load up a struct sockaddr with the result */
    if ((getaddrinfo_call_status = getaddrinfo(host_name, random_port, &hints, &server_info)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_call_status));
        return -1;
    }

    /* loop through all the results and make a socket */
    for(p = server_info; p != NULL; p = p->ai_next) {

        /* make a socket using socket() call, and ensure that it ran error-free */
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("(client) error during socket creation - UDP");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to create UDP socket\n");
        return -1;
    }

    source_file = fopen(file_name, "r");  // Open the file corresponding to the <filename> argument from terminal. The file must exist!!

    if (source_file == NULL) {  // if file could'nt be successfully opened, fopen returns a null pointer
        perror("(client) error when opening file");
        exit(EXIT_FAILURE);
    }

    counter = 0;
    do {
        c = getc(source_file);  // returns the character currently pointed by the internal file position indicator of the specified stream.

        /* packs payload with four characters, unless there are less than four characters left in the stream */
        if ((counter % 4 == 0 && counter != 0) || c == EOF) {
            
            /*
            if EOF is encountered somewhere in the middle of a packet, pad the rest of the payload
            (which will be sent back as an acknowledgement) with null characters to maintain data
            integrity 
            */
            if (c == EOF) {
                
                payload[counter] = c;
                for (int j = counter+1; j < 4; j++) {
                    payload[j] = '\0';
                }

            }

            /* send data out over a socket (error-checking included) */
            if ((num_bytes = sendto(socket_fd, payload, 4, MSG_CONFIRM, p->ai_addr, p->ai_addrlen)) == -1) {
                perror("(client) error when calling sendto - UDP");
                exit(EXIT_FAILURE);
            }

            /* wait for authentication from the server */
            addr_len = sizeof incoming_connection_addr;
            if ((num_bytes = recvfrom(socket_fd, buffer, MAXDATASIZE-1 , MSG_WAITFORONE,
                    (struct sockaddr *)&incoming_connection_addr, &addr_len)) == -1) {
                perror("(client) error when calling recvfrom - UDP");
                exit(EXIT_FAILURE);
            }

            buffer[num_bytes] = '\0';
            printf("%s\n", buffer);  // Print the current packet to the output stream on a new-line.

            /* 
            Verifies that the correct authentication packet was received, it is really unnecessary.
            Takes each packet received from the server, and verifies that its contents are what's expected
            given the most recent payload sent out to the server. 
            */
            auth_status = 1;
            for (int i = 0; i < 4; i++) {

                if (toupper(payload[i]) != buffer[i]) {
                    auth_status = 0;
                    break;
                }

            }

            if (auth_status == 0) {
                continue;
            }

            payload[0] = c;
            counter = 1;

        } else {

            payload[counter] = c;
            counter = counter + 1;

        }

    }
    while (c != EOF && auth_status != 0);  // loop until end of file is encountered, and the final authentication has been verified.

    fclose(source_file);  // file pointer is disassociated from the file.
    close(socket_fd);  // close the socket descriptor
}

/* main drives both tcp_handshake and udp_transaction functions using arguments from the input stream */
int main(int argc, char *argv[]) {

    char random_port[MAXDATASIZE];  // stores the random port value returned by the server.
    char *host_name, *negotiation_port, *file_name;  // variables to store the respective inputs from the terminal.

    FILE *source_file;  // file descriptor

    /* ensure that entries corresponding to <server_address>, <negotiation_port> amd <file_name> are provided at run-time */
    if (argc != 4) {
        fprintf(stderr, "usage: client <server_address> <n_port> <file_name>\n");
        exit(EXIT_FAILURE);
    }

    host_name = argv[1];
    negotiation_port = argv[2];
    file_name = argv[3];

    /* call to tcp_handshake, returns result loaded in the random_port variable */
    if (tcp_handshake(host_name, negotiation_port, random_port) == -1) {
        exit(EXIT_FAILURE);
    }

    /* call to udp_transaction, sends data to the server using datagram sockets */
    if (udp_transaction(host_name, random_port, file_name) == -1) {
        exit(EXIT_FAILURE);
    }

}