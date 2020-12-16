#ifndef INODES_H
#define INODES_H

#include <stdio.h>
#include <stdlib.h>
#include "../tecnicofs-api-constants.h"
#include <pthread.h>
#include <errno.h>


/* FS root inode number */
#define FS_ROOT 0

#define FREE_INODE (-1)
#define INODE_TABLE_SIZE 50
#define MAX_DIR_ENTRIES 20

#define SUCCESS 0
#define FAIL (-1)

#define DELAY 5000

#define MAX_PATH_INODE_LENGTH 100

/* if condition is false displays msg and interrupts execution */
#define assert__(cond, msg) if(! (cond)) { fprintf(stderr, msg); exit(EXIT_FAILURE); }


/*
 * Contains the name of the entry and respective i-number
 */
typedef struct dirEntry {
	char name[MAX_FILE_NAME];
	int inumber;
} DirEntry;

/*
 * Data is either text (file) or entries (DirEntry)
 */
union Data {
	char *fileContents; /* for files */
	DirEntry *dirEntries; /* for directories */
};

/*
 * I-node definition
 */
typedef struct inode_t {    
	type nodeType;
	union Data data;
    pthread_rwlock_t lock;
} inode_t;


void insert_delay(int cycles);
void inode_table_init();
void inode_table_destroy();
int inode_create(type nType);
int inode_delete(int inumber);
int inode_get(int inumber, type *nType, union Data *data);
int inode_set_file(int inumber, char *fileContents, int len);
int dir_reset_entry(int inumber, int sub_inumber);
int dir_add_entry(int inumber, int sub_inumber, char *sub_name);
void inode_print_tree(FILE *fp, int inumber, char *name);
int lock_read(int inumber);
int trylock_read(int inumber);
int lock_write(int inumber);
int trylock_write(int inumber);
int unlock(int inumber);


#endif /* INODES_H */
