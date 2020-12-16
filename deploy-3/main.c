#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fs/operations.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <strings.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 100

/* server's number of threads */
int numberThreads = 0;

/* server socket file descriptor */
int server_socket_fd;

/* number of threads executing a client request */
int in_execution = 0;

/* used to signal when a thread is writing contents in a file */
int is_printing = 0;

/* used to lock when checking if client inserted a print request */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_print = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_wait = PTHREAD_COND_INITIALIZER;


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
 * Applies commands from input file
 */
void applyCommands() {

    struct sockaddr_un client_addr;  /* client socket address */
    int c;  /* holds number of bytes read */

    socklen_t addrlen = sizeof(struct sockaddr_un);  /* gets standard size of socket address */

    char command[MAX_INPUT_SIZE];  /* holds command that is going to be executed */

    int output[1];  /* holds command output after execution. uses an array for simplicity */

    /* loop until file has reached it's end */
    while (1) {

        /* receives message and gets number of bytes read */
        c = recvfrom(server_socket_fd, command, sizeof(command) - 1, 0,
                     (struct sockaddr *) &client_addr, &addrlen);

        if (c <= 0) continue;  /* if inputs is invalid, continues */

        command[c] = '\0';  /* prevents client message from not having a '\0' */

        char token;
        char name_2[MAX_INPUT_SIZE];
        char name_1[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %s", &token, name_1, name_2);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        assert__(pthread_mutex_lock(&lock) == 0, "Error: applyCommands failed to lock!\n")

        /* when we detect a 'p' command, this variable is set to 1 and thus, all subsequent threads
         * will wait in here until the command is concluded before proceeding */
        while (is_printing)
            pthread_cond_wait(&cond_print, &lock);

        /* when a 'p' command is detected, we need to inform the other threads that we have this
         * pending command and after that, we wait until all threads finish their execution */
        if (token == 'p') {
            is_printing++;
            while(in_execution > 0)
                pthread_cond_wait(&cond_wait, &lock);
        }

        in_execution++;  /* signals that we have one more thread in execution */

        assert__(pthread_mutex_unlock(&lock) == 0, "Error: applyCommands failed to unlock!\n")

        switch (token) {
            case 'c':

                switch (name_2[0]) {
                    case 'f':
                        printf("Create file: %s\n", name_1);
                        output[0] = create(name_1, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name_1);
                        output[0] = create(name_1, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;

            case 'l':
                output[0] = lookup(name_1);
                if (output[0] >= 0) printf("Search: %s found\n", name_1);
                else printf("Search: %s not found\n", name_1);
                break;

            case 'd':
                printf("Delete: %s\n", name_1);
                output[0] = delete(name_1);
                break;

            case 'm':
                printf("Move: %s\n", name_1);
                output[0] = move(name_1, name_2);
                break;

            case 'p':
                printf("Print: %s\n", name_1);
                output[0] = print_tecnicofs_tree(name_1);

                assert__(pthread_mutex_lock(&lock) == 0, "Error: applyCommands failed to lock!\n")
                /* printing has finished and signals other threads of this event */
                is_printing--; pthread_cond_broadcast(&cond_print);
                assert__(pthread_mutex_unlock(&lock) == 0, "Error: applyCommands failed to unlock!\n")
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }

        }

        /* sends report back to client */
        sendto(server_socket_fd, output, sizeof(output), 0, (struct sockaddr *) &client_addr, addrlen);

        assert__(pthread_mutex_lock(&lock) == 0, "Error: applyCommands failed to lock!\n")
        in_execution--;
        pthread_cond_signal(&cond_wait);
        assert__(pthread_mutex_unlock(&lock) == 0, "Error: applyCommands failed to unlock!\n")

    }
}


/* auxiliary function used to redirect a thread to the applyCommands function */
void *applyCommand_thread(void* ptr) {
    applyCommands();
    return NULL;
}


int main(int argc, char* argv[]) {

    int sock_fd;  /* server socket file descriptor */

    struct sockaddr_un server_addr;  /* server socket address */
    socklen_t addrlen;  /* size of server socket */

    /* checks if the user inserted the correct amount of inputs */
    assert__(argc == 3, "Error: need 3 inputs.\n")

    /* holds info about each thread id */
    numberThreads = atoi(argv[1]);
    assert__(numberThreads > 0, "Error: program needs to have more than zero threads.\n")
    pthread_t thread_ids[numberThreads];

    /* gets server socket name from the command line */
    char* server_socket_name = argv[2];

    /* creates server side socket */
    assert__((sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) != -1, "Error: couldn't create server socket!\n")

    /* removes possible previous links */
    unlink(server_socket_name);

    /* gets socket size and initializes values */
    addrlen = set_socket_address_unix(server_socket_name, &server_addr);

    /* assigns a local socket address to a socket identified by descriptor socket */
    assert__(bind(sock_fd, (struct sockaddr *) &server_addr, addrlen) != -1, "Error: couldn't bind server socket!\n")

    /* saves server socket file descriptor in a global variable so that other functions can access it */
    server_socket_fd = sock_fd;

    /* init filesystem */
    init_fs();

    /* creates all the requested threads. if it fails, reports an error */
    for (int i = 0; i < numberThreads; i++)
        assert__(pthread_create(&thread_ids[i], NULL, applyCommand_thread, NULL) == 0, "Error: couldn't create a thread!\n")

    /* since our threads will never end, using pthread_join here will create an 'infinite loop' thus
     * keeping our server online without consuming much resources compared to using while(1) */
    pthread_join(thread_ids[0], NULL);

    /* since server never ends, this part will never be run. releases allocated memory */
    destroy_fs();

    exit(EXIT_SUCCESS);

}
