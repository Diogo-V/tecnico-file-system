#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
int move(char *from, char *to);
int traverse_path(char *name, int *locked_inumbers, int *amount, int is_lookup);
void print_tecnicofs_tree(FILE *fp);
void unlock_inodes(const int locked_inumbers[MAX_PATH_INODE_LENGTH], int amount);

#endif /* FS_H */
