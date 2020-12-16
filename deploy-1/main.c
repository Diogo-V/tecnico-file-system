#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fs/operations.h"
#include <sys/time.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

/* if condition is false displays msg and interrupts execution */
#define assert__(cond, msg) if(! (cond)) { fprintf(stderr, msg); exit(EXIT_FAILURE); }

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];

int numberCommands = 0;

int headQueue = 0;

/* file paths */
char *input_file_path;
char *output_file_path;

/* desired sync type (input from user) */
syncType sync;

/* thread sync locks */
pthread_rwlock_t rw_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t mtx_lock = PTHREAD_MUTEX_INITIALIZER;


int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}


char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}


void processInput(){
    char line[MAX_INPUT_SIZE];

    /* opens input file and checks if it is valid */
    FILE *file = fopen(input_file_path, "r");
    assert__(file != NULL, "Error: couldn't open input file.\n")

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), file)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

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
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }

    /* closes file for good measure */
    fclose(file);

}


/* Unlocks previously locked section */
void unlock() {
    switch (sync) {
        case MUTEX:
            assert__(pthread_mutex_unlock(&mtx_lock) == 0, "Error: thread failed to unlock using mutex!\n")
            break;

        case RWLOCK:
            assert__(pthread_rwlock_unlock(&rw_lock) == 0, "Error: thread failed to unlock using rw_lock!\n")
            break;

        case NOSYNC:
            break;

        default:
            fprintf(stderr, "Error: invalid sync type\n");
            exit(EXIT_FAILURE);
    }

}


/*
 * Locks critical section using the requested sync format.
 * Inputs:
 *   - rw_type: should be WRITING if used in a creating/deleting section and should be READING if
 *              used in a reading section.
*/
void lock(rwlock_type rw_type) {
    switch (sync) {
        case MUTEX:
            assert__(pthread_mutex_lock(&mtx_lock) == 0, "Error: thread failed to lock using mutex!\n")
            break;

        case RWLOCK:
            switch (rw_type) {
                case WRITING:
                    assert__(pthread_rwlock_wrlock(&rw_lock) == 0, "Error: thread failed to lock using rwlock!\n")
                    break;
                case READING:
                    assert__(pthread_rwlock_rdlock(&rw_lock) == 0, "Error: thread failed to lock using rdlock!\n")
                    break;
                default:
                    fprintf(stderr, "Error: invalid rwlock_type in lock\n");
                    exit(EXIT_FAILURE);
            }
            break;

        case NOSYNC:
            break;

        default:
            fprintf(stderr, "Error: invalid sync type\n");
            exit(EXIT_FAILURE);
    }
}


void applyCommands(){

    /* Uses mutex, regardless of requested sync type because we need to change numberCommands and
     * get our command to process */
    assert__(pthread_mutex_lock(&mtx_lock) == 0, "Error: thread failed to lock using mutex!\n")
    while (numberCommands > 0){

        const char* command = removeCommand();
        assert__(pthread_mutex_unlock(&mtx_lock) == 0, "Error: thread failed to unlock using mutex!\n")

        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':

                switch (type) {
                    case 'f':
                        lock(WRITING);
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        unlock();
                        break;
                    case 'd':
                        lock(WRITING);
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        unlock();
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;

            case 'l':
                lock(READING);
                searchResult = lookup(name);
                unlock();
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;

            case 'd':
                lock(WRITING);
                printf("Delete: %s\n", name);
                delete(name);
                unlock();
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }

        }
    }
}


/* auxiliary function used to redirect a thread to the applyCommands function */
void *applyCommand_tread(void* ptr) {
    applyCommands();
    return NULL;
}


syncType string_to_syncType(char* string) {
    static char const * syncType_string[] = {"mutex", "rwlock", "nosync" };
    for (int i = 0; i < END; ++i)
        if (!strcmp(syncType_string[i], string))
            return i;
    return END;
}


int main(int argc, char* argv[]) {

    /* timer starts */
    struct timeval start, end;
    gettimeofday(&start, NULL);

    /* checks if the user inserted the correct amount of inputs */
    assert__(argc == 5, "Error: need 5 inputs.\n")

    /* holds info about each thread id */
    numberThreads = atoi(argv[3]);
    assert__(numberThreads > 0, "Error: program needs to have more than zero threads.\n")
    pthread_t thread_ids[numberThreads];

    /* gets path to our input file and our output file from the command line */
    input_file_path = argv[1];
    output_file_path = argv[2];

    /* validates if the user inserted 2 valid file paths */
    assert__(input_file_path, "Error: no input file inserted.\n")
    assert__(output_file_path, "Error: no output file inserted.\n")

    /* gets requested sync type from the command line. verifies if everything is correct */
    sync = string_to_syncType(argv[4]);
    assert__(sync == MUTEX || sync == RWLOCK || sync == NOSYNC, "Error: invalid sync value.\n")

    /* blocks the user from using a nosync strategy if he input more than one thread of execution */
    assert__(! (sync == NOSYNC && numberThreads != 1), "Error: only 1 thread is allowed when using nosync.\n")

    /* init filesystem */
    init_fs();
    /* process input and print tree */
    processInput();

    /* creates all the requested threads. if it fails, reports an error */
    for (int i = 0; i < numberThreads; i++)
        assert__(pthread_create(&thread_ids[i], NULL, applyCommand_tread, NULL) == 0, "Error: couldn't create a thread.\n")

    /* waits for all the threads to end. if it fails, reports an error */
    for (int i = 0; i < numberThreads; i++)
        assert__(pthread_join(thread_ids[i], NULL) == 0, "Error: couldn't join a thread.\n")

    /* opens output file for writing, does so and closes it */
    FILE *file = fopen(output_file_path, "w");
    assert__(file != NULL, "Error: couldn't open output file.\n")
    print_tecnicofs_tree(file);
    fclose(file);

    /* release allocated memory */
    destroy_fs();

    /* Destroys locks */
    pthread_mutex_destroy(&mtx_lock);
    pthread_rwlock_destroy(&rw_lock);

    /* timer ends */
    gettimeofday(&end, NULL);

    /* number of seconds and microseconds of timer */
    double final_time = (double) ((( (double) (end.tv_sec - start.tv_sec) * 1e6) +
                                    end.tv_usec) - (start.tv_usec))/ 1e6;

    /* time of execution on the requested format */
    printf("TecnicoFS completed in %.4f seconds.\n", final_time);

    exit(EXIT_SUCCESS);
}
