#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fs/operations.h"
#include <sys/time.h>

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];

int numberCommands = 0;

int insertQueue = 0;

int removeQueue = 0;

/* file paths */
char *input_file_path;
char *output_file_path;

/* used only for loop synchronization */
pthread_mutex_t lock_remove = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_insert = PTHREAD_MUTEX_INITIALIZER;

/* conditional variables */
pthread_cond_t cond_thread, cond_commands;


/*
 * Inserts a command in in inputCommands[]
 * Input:
 *  - data: command to be inserted
 * Returns:
 *  1
 */
int insertCommand(char* data) {
    assert__(pthread_mutex_lock(&lock_insert) == 0, "Error: insertCommand failed to lock!\n")

    /* wait if all commands in inputCommands are being executed */
    while(numberCommands == MAX_COMMANDS-1){
        pthread_cond_wait(&cond_commands, &lock_insert);
    }

    /* inserts a new command in inputCommands */
    strcpy(inputCommands[insertQueue], data);

    insertQueue++;

    /* if insertQueue has reached the end of inputCommands, return to the beginning */
    if(insertQueue == MAX_COMMANDS)
        insertQueue = 0;

    numberCommands++;

    /* signal to removeCommand, so that new command is executed */
    pthread_cond_signal(&cond_thread);
    assert__(pthread_mutex_unlock(&lock_insert) == 0, "Error: insertCommand failed to unlock!\n")

    return 1;
}


/*
 * Removes a command from inputCommands[] for it to be applied
 * Returns:
 *  - command to apply
 */
char* removeCommand() {

    char* currentCommand;

    /* mutex lock (unlock at line 221 and 169) */
    assert__(pthread_mutex_lock(&lock_remove) == 0, "Error: removeCommand failed to lock!\n")

    /* wait while there are no commands to execute */
    while(numberCommands == 0){
        pthread_cond_wait(&cond_thread, &lock_remove);
    }

    currentCommand = inputCommands[removeQueue];

    /* if command is "EOF", inputCommands has reached it's end */
    if(strcmp(currentCommand, "EOF") == 0){
        return currentCommand;
    }

    removeQueue++;

    /* if removeQueue has reached the end of inputCommands, return to the beginning */
    if(removeQueue == MAX_COMMANDS)
        removeQueue = 0;

    numberCommands--;

    /* in case insertCommand is 'waiting', signal to proceed */
    pthread_cond_signal(&cond_commands);

    return currentCommand;  
}


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}


/*
 * Processes input file
 */
void processInput(){
    char line[MAX_INPUT_SIZE];

    /* opens input file */
    FILE *file = fopen(input_file_path, "r");
    assert__(file != NULL, "Error: invalid input file!\n")

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line) / sizeof(char), file)) {
        char token;
        char type[MAX_INPUT_SIZE];
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %s", &token, name, type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;

            case 'm':
                if (numTokens != 3)
                    errorParse();
                if (insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    } 
    /* file has reached it's end, no more commands to execute, put "EOF" to end loops*/
    insertCommand("EOF");

    numberCommands++;   

    /* closes file for good measure */
    fclose(file);          
}


/*
 * Applies commands from input file
 */
void applyCommands(){

    /* loop until file has reached it's end */
    while (1) {

        const char* command = removeCommand();

        /* if command is "EOF" there are no more commands to execute, break loop and all threads must exit */
        if (strcmp(command, "EOF") == 0) {
            numberCommands = 1;
            assert__(pthread_mutex_unlock(&lock_remove) == 0, "Error: removeCommand failed to unlock!\n")
            pthread_cond_signal(&cond_thread);
            pthread_exit(NULL);
        }

        char token;
        char name_2[MAX_INPUT_SIZE];
        char name_1[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %s", &token, name_1, name_2);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':

                switch (name_2[0]) {
                    case 'f':
                        printf("Create file: %s\n", name_1);
                        create(name_1, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name_1);
                        create(name_1, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;

            case 'l':
                searchResult = lookup(name_1);
                if (searchResult >= 0) printf("Search: %s found\n", name_1);
                else printf("Search: %s not found\n", name_1);
                break;

            case 'd':
                printf("Delete: %s\n", name_1);
                delete(name_1);
                break;

            case 'm':
                printf("Move: %s\n", name_1);
                move(name_1, name_2);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }

        }

        assert__(pthread_mutex_unlock(&lock_remove) == 0, "Error: removeCommand failed to unlock!\n")

    }
}


/* auxiliary function used to redirect a thread to the applyCommands function */
void *applyCommand_thread(void* ptr) {
    applyCommands();
    return NULL;
}


int main(int argc, char* argv[]) {
    /* timer starts */
    struct timeval start, end;
    gettimeofday(&start, NULL);

    /* checks if the user inserted the correct amount of inputs */
    assert__(argc == 4, "Error: need 4 inputs.\n")

    /* init condition variables*/
    pthread_cond_init(&cond_thread, NULL);
    pthread_cond_init(&cond_commands, NULL);

    /* holds info about each thread id */
    numberThreads = atoi(argv[3]);
    assert__(numberThreads > 0, "Error: program needs to have more than zero threads.\n")
    pthread_t thread_ids[numberThreads];

    /* gets path to our input file and our output file from the command line */
    input_file_path = argv[1];
    output_file_path = argv[2];

    /* validates if the user inserted 2 valid file paths */
    assert__(input_file_path, "Error: couldn't open input file.\n")
    assert__(output_file_path, "Error: couldn't open output file.\n")

    /* init filesystem */
    init_fs();

    /* creates all the requested threads. if it fails, reports an error */
    for (int i = 0; i < numberThreads; i++)
        assert__(pthread_create(&thread_ids[i], NULL, applyCommand_thread, NULL) == 0, "Error: couldn't create a thread.\n")

    /* process input and print tree */
    processInput();

    /* waits for all the threads to end. if it fails, reports an error */
    for (int i = 0; i < numberThreads; i++)
        assert__(pthread_join(thread_ids[i], NULL) == 0, "Error: couldn't join a thread.\n")

    /* timer ends */
    gettimeofday(&end, NULL);

    /* number of seconds and microseconds of timer */
    double final_time = (double) ((( (double) (end.tv_sec - start.tv_sec) * 1e6) +
                                   end.tv_usec) - (start.tv_usec))/ 1e6;

    /* opens output file for writing, does so and closes it */
    FILE *file = fopen(output_file_path, "w");
    assert__(file != NULL, "Error: couldn't open output file.\n")
    print_tecnicofs_tree(file);
    fclose(file);

    pthread_cond_destroy(&cond_thread);
    pthread_cond_destroy(&cond_commands);

    /* release allocated memory */
    destroy_fs();

    /* time of execution on the requested format */
    printf("TecnicoFS completed in %.4f seconds.\n", final_time);

    exit(EXIT_SUCCESS);

}
