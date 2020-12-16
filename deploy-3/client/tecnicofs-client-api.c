#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>


/* holds server socket address */
struct sockaddr_un server_socket;

/* size of server socket */
socklen_t serv_len = sizeof(struct sockaddr_un);

/* holds client socket file descriptor */
int client_fd;

/* holds client socket path */
char client_path[30] = "/tmp/";

/* holds number of bytes sent */
int c;

/* holds server output. uses an array for simplicity */
int output[1];

/* holds line that is going to be sent to the server */
char line[MAX_INPUT_SIZE];


/*
 * Sets socket address and inits everything.
 *
 * Input:
 *   - path: socket path
 *   - addr: socket address
 * Output:
 *   - size of socket
 * */
int set_socket_address_unix(char *path, struct sockaddr_un *addr) {

    if (addr == NULL) return 0;  /* interrupts if address is invalid */

    bzero((char *)addr, sizeof(struct sockaddr_un));  /* clears possible values in addr variable */
    addr->sun_family = AF_UNIX;  /* sets socket domain as unix */
    strcpy(addr->sun_path, path);  /* copies path to the socket */

    return SUN_LEN(addr);
}


/*
 * Sends message to tecnicofs server telling it to create a file/directory.
 *
 * Input:
 *   - filename: file/directory path that is going to be created
 *   - nodeType: f, creates a file and d, creates a directory
 * Output:
 *   - SUCCESS or FAIL
 * */
int tfsCreate(char *filename, char nodeType) {

    /* clears memory and concatenates everything in a command before sending to the server */
    bzero(line, MAX_INPUT_SIZE);
    strcat(line, "c ");
    strcat(line, filename);
    strcat(line, " ");
    strcat(line, &nodeType);
    strcat(line, "\0");

    /* send message to create and gets the number of bytes sent */
    c = sendto(client_fd, line, strlen(line)+1, 0, (struct sockaddr *) &server_socket, serv_len);

    /* checks if an error occurred */
    assert__(c >= 0, "Error: tfsCreate had an error and couldn't send message!\n")

    /* gets message from the server */
    recvfrom(client_fd, output, sizeof(output), 0, (struct sockaddr *) &server_socket, &serv_len);

    return output[0];
}


/*
 * Sends message to tecnicofs server telling it to delete a file/directory.
 *
 * Input:
 *   - path: file path that is going to be deleted
 * Output:
 *   - SUCCESS or FAIL
 * */
int tfsDelete(char *path) {

    /* clears memory and concatenates everything in a command before sending to the server */
    bzero(line, MAX_INPUT_SIZE);
    strcat(line, "d ");
    strcat(line, path);
    strcat(line, "\0");

    /* send message to create and gets the number of bytes sent */
    c = sendto(client_fd, line, strlen(line) + 1, 0, (struct sockaddr *) &server_socket, serv_len);

    /* checks if an error occurred */
    assert__(c >= 0, "Error: tfsDelete had an error and couldn't send message!\n")

    /* gets message from the server */
    recvfrom(client_fd, output, sizeof(output), 0, (struct sockaddr *) &server_socket, &serv_len);

    return output[0];
}


/*
 * Sends message to tecnicofs server telling it to move a file/directory.
 *
 * Input:
 *   - from: file/directory that is going to be moved
 *   - to: new path for the input file/directory
 * Output:
 *   - SUCCESS or FAIL
 * */
int tfsMove(char *from, char *to) {

    /* clears memory and concatenates everything in a command before sending to the server */
    bzero(line, MAX_INPUT_SIZE);
    strcat(line, "m ");
    strcat(line, from);
    strcat(line, " ");
    strcat(line, to);
    strcat(line, "\0");

    /* send message to create and gets the number of bytes sent */
    c = sendto(client_fd, line, strlen(line) + 1, 0, (struct sockaddr *) &server_socket, serv_len);

    /* checks if an error occurred */
    assert__(c >= 0, "Error: tfsMove had an error and couldn't send message!\n")

    /* gets message from the server */
    recvfrom(client_fd, output, sizeof(output), 0, (struct sockaddr *) &server_socket, &serv_len);

    return output[0];
}


/*
 * Sends message to tecnicofs server telling it to lookup a file/directory.
 *
 * Input:
 *   - path: file/directory that is going to be searched
 * Output:
 *   - SUCCESS or FAIL
 * */
int tfsLookup(char *path) {

    /* clears memory and concatenates everything in a command before sending to the server */
    bzero(line, MAX_INPUT_SIZE);
    strcat(line, "l ");
    strcat(line, path);
    strcat(line, "\0");

    /* send message to create and gets the number of bytes sent */
    c = sendto(client_fd, line, strlen(line) + 1, 0, (struct sockaddr *) &server_socket, serv_len);

    /* checks if an error occurred */
    assert__(c >= 0, "Error: tfsLookup had an error and couldn't send message!\n")

    /* gets message from the server */
    recvfrom(client_fd, output, sizeof(output), 0, (struct sockaddr *) &server_socket, &serv_len);

    return output[0];
}


/*
 * Sends message to tecnicofs server telling it to print it's tree.
 *
 * Input:
 *   - out_file: file where the contents will be written
 * Output:
 *   - SUCCESS or FAIL
 * */
int tfsPrint(char* out_file) {

    /* clears memory and concatenates everything in a command before sending to the server */
    bzero(line, MAX_INPUT_SIZE);
    strcat(line, "p ");
    strcat(line, out_file);
    strcat(line, "\0");

    /* send message to create and gets the number of bytes sent */
    c = sendto(client_fd, line, strlen(line) + 1, 0, (struct sockaddr *) &server_socket, serv_len);

    /* checks if an error occurred */
    assert__(c >= 0, "Error: tfsPrint had an error and couldn't send message!\n")

    /* gets message from the server */
    recvfrom(client_fd, output, sizeof(output), 0, (struct sockaddr *) &server_socket, &serv_len);

    return output[0];
}


/*
 * Creates and allocates all the resources needed for client socket. Also registers server socket.
 *
 * Input:
 *   - server_socket_path: path to server socket
 * Output:
 *   - EXIT_SUCCESS or error
 * */
int tfsMount(char* server_socket_path) {

    int sock_fd;  /* client socket file descriptor */

    /*Max Number threads is around 2^22*/
    char pid[7];

    /* sets client socket path as: /tmp/<pid> */
    sprintf(pid, "%d", getpid());
    strcat(client_path, pid);

    /* gets server socket so that it can be used by other functions */
    strcpy(server_socket.sun_path, server_socket_path);
    server_socket.sun_family = AF_UNIX;

    struct sockaddr_un client_addr;  /* client socket address */
    socklen_t addrlen;  /* size of client socket */

    /* creates client side socket */
    assert__((sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) != -1, "Error: couldn't create client socket!\n")

    /* removes possible previous links */
    unlink(client_path);

    /* gets socket size and initializes values */
    addrlen = set_socket_address_unix(client_path, &client_addr);

    /* assigns a local socket address to a socket identified by descriptor socket */
    assert__(bind(sock_fd, (struct sockaddr *) &client_addr, addrlen) != -1, "Error: couldn't bind client socket!\n")

    client_fd = sock_fd;  /* saves file descriptor so that all functions can access it */

    return EXIT_SUCCESS;
}


/*
 * Clears previously allocated resources for the client socket
 *
 * Output:
 *   - EXIT_SUCCESS or error
 * */
int tfsUnmount() {

    /* clears previously allocated link */
    unlink(client_path);

    /* shuts down, closes and frees resources associated with the client socket */
    assert__(shutdown(client_fd, SHUT_RDWR) == 0, "Error: tfsMount couldn't shutdown socket!\n")
    assert__(close(client_fd) == 0, "Error: tfsMount couldn't close socket!\n")

    return EXIT_SUCCESS;
}
