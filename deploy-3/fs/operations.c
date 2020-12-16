#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATTENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 * Return:
 *  - number of directories in this path
 */
int split_parent_child_from_path(char * path, char ** parent, char ** child) {

    int n_slashes = 0, last_slash_location = 0;
    int len = (int) strlen(path);

    // deal with trailing slash ( a/x vs a/x/ )
    if (path[len-1] == '/') {
        path[len-1] = '\0';
    }

    for (int i=0; i < len; ++i) {
        if (path[i] == '/' && path[i+1] != '\0') {
            last_slash_location = i;
            n_slashes++;
        }
    }

    if (n_slashes == 0) { // root directory
        *parent = "";
        *child = path;
        return ++n_slashes;
    }

    path[last_slash_location] = '\0';
    *parent = path;
    *child = path + last_slash_location + 1;

    /* returns the number of nodes in this path */
    return ++n_slashes;
}


/*
 * Checks if an inumber is already inside the locked inumbers array.
 *
 * Input:
 *   - inumber: integer to look for
 *   - arr: array of inumbers
 *   - size: integer holding the size of the array
 * Return:
 *   - 0 if not found and 1 if found
 * */
int check_if_node_is_in_array(int inumber, const int *arr, int size) {
    for(int i = 0; i < size; i++) if(arr[i] == inumber) return 1;
    return 0;
}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
    inode_table_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);

    if (root != FS_ROOT) {
        printf("failed to create node for tecnicofs root\n");
        exit(EXIT_FAILURE);
    }
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
    inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
    if (dirEntries == NULL) {
        return FAIL;
    }
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (dirEntries[i].inumber != FREE_INODE) {
            return FAIL;
        }
    }
    return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
    if (entries == NULL) {
        return FAIL;
    }
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
    return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

    int parent_inumber, child_inumber;
    char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
    /* use for copy */
    type pType;
    union Data pdata;

    /* holds all the inode id's locked while doing this operation */
    int locked_inumbers[MAX_PATH_INODE_LENGTH];
    int amount = 0;

    strcpy(name_copy, name);
    split_parent_child_from_path(name_copy, &parent_name, &child_name);

    /* gets parent directory's inode number (locks all the used inodes) */
    parent_inumber = traverse_path(parent_name, locked_inumbers, &amount, 0);

    if (parent_inumber == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to create %s, invalid parent dir %s\n", name, parent_name);
        return FAIL;
    }

    /* gets parent inode info */
    inode_get(parent_inumber, &pType, &pdata);

    if(pType != T_DIRECTORY) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to create %s, parent %s is not a dir\n", name, parent_name);
        return FAIL;
    }

    if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to create %s, already exists in dir %s\n", child_name, parent_name);
        return FAIL;
    }

    /* create node and add entry to folder that contains new node */
    child_inumber = inode_create(nodeType);
    if (child_inumber == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to create %s in  %s, couldn't allocate inode\n", child_name, parent_name);
        return FAIL;
    }

    if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("could not add entry %s in dir %s\n", child_name, parent_name);
        return FAIL;
    }

    unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */

    return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

    int parent_inumber, child_inumber;
    char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
    /* use for copy */
    type pType, cType;
    union Data pdata, cdata;

    /* holds all the inode id's locked while doing this operation */
    int locked_inumbers[MAX_PATH_INODE_LENGTH];
    int amount = 0;

    strcpy(name_copy, name);
    split_parent_child_from_path(name_copy, &parent_name, &child_name);

    /* gets parent directory's inode number (locks all the used inodes) */
    parent_inumber = traverse_path(parent_name, locked_inumbers, &amount, 0);

    if (parent_inumber == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to delete %s, invalid parent dir %s\n", child_name, parent_name);
        return FAIL;
    }

    inode_get(parent_inumber, &pType, &pdata);

    if(pType != T_DIRECTORY) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to delete %s, parent %s is not a dir\n", child_name, parent_name);
        return FAIL;
    }

    child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

    if (child_inumber == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("could not delete %s, does not exist in dir %s\n", name, parent_name);
        return FAIL;
    }

    /* locks (write) the directory where file/directory will be created */
    assert__(lock_write(child_inumber) == SUCCESS, "Error: delete failed to lock an inode!\n")
    locked_inumbers[amount++] = child_inumber;

    inode_get(child_inumber, &cType, &cdata);

    if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("could not delete %s: is a directory and not empty\n", name);
        return FAIL;
    }

    /* remove entry from folder that contained deleted node */
    if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to delete %s from dir %s\n", child_name, parent_name);
        return FAIL;
    }

    if (inode_delete(child_inumber) == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("could not delete inode number %d from dir %s\n", child_inumber, parent_name);
        return FAIL;
    }

    unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */

    return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name) {

    /* holds all the inode id's locked while doing this operation */
    int locked_inumbers[MAX_PATH_INODE_LENGTH];
    int amount = 0;

    /* traverses path and lock all the used inodes */
    int res = traverse_path(name, locked_inumbers, &amount, 1);

    /* unlocks all the used nodes */
    unlock_inodes(locked_inumbers, amount);

    return res;
}


/*
* Moves a file/directory from a path to another one.
* Input:
*   - from: current path of the file/directory to move
*   - to: new path of this file/directory
*/
int move(char* from, char* to) {

    /* used to hold name while splitting string */
    char name_copy_1[MAX_FILE_NAME], name_copy_2[MAX_FILE_NAME];

    /* from variables */
    int parent_from_inumber, child_from_inumber;
    char *parent_from, *child_from;

    /* to variables */
    int parent_to_inumber, child_to_inumber;
    char *parent_to, *child_to;

    /* used for copy */
    type pType_from, cType_from;
    union Data pdata_from, cdata_from;
    type pType_to;
    union Data pdata_to;

    /* holds result of trying to find a substring in a string */
    char* sub_str;

    /* holds all the inode id's locked while doing this operation */
    int locked_inumbers[MAX_PATH_INODE_LENGTH];
    int amount = 0;

    /* separates all the traversed inodes */
    strcpy(name_copy_1, from);
    split_parent_child_from_path(name_copy_1, &parent_from, &child_from);
    strcpy(name_copy_2, to);
    split_parent_child_from_path(name_copy_2, &parent_to, &child_to);

    /* checks if we are trying to put a a directory/file inside itself. if so, interrupts */
    if ((sub_str = strstr(name_copy_2, name_copy_1)) != NULL && strcmp(sub_str, name_copy_2) == 0) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to move %s, can't move a dir inside itself\n", from);
        return FAIL;
    }

    /* checks which path is shorter and traverses that one first because, otherwise, it would
     * create a deadlock since we would try to lock for writing when there was already a
     * read lock in place. if they have the same number of nodes, the order won't matter because
     * only two cases can happen: either both parent dirs are the same (we use a trylock to fix
     * this) or they are different and if so, it won't interfere with the process */
    if (strcmp(parent_from, parent_to) > 0) {
        parent_to_inumber = traverse_path(parent_to, locked_inumbers, &amount, 0);
        parent_from_inumber = traverse_path(parent_from, locked_inumbers, &amount, 0);
    } else {
        parent_from_inumber = traverse_path(parent_from, locked_inumbers, &amount, 0);
        parent_to_inumber = traverse_path(parent_to, locked_inumbers, &amount, 0);
    }

    /* if we couldn't find it, returns an error */
    if (parent_from_inumber == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to move %s, invalid parent_from dir %s\n", from, parent_from);
        return FAIL;
    } else if (parent_to_inumber == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to move %s, invalid parent_to dir %s\n", from, parent_to);
        return FAIL;
    }

    /* get both parents inodes information */
    inode_get(parent_from_inumber, &pType_from, &pdata_from);
    inode_get(parent_to_inumber, &pType_to, &pdata_to);

    /* if it wasn't a directory, child will automatically not exist so, we throw an error */
    if (pType_from != T_DIRECTORY) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to move %s, parent_from %s is not a dir\n", from, parent_from);
        return FAIL;

        /* if it wasn't a directory, we can't move anything to there, so we throw an error */
    } else if (pType_to != T_DIRECTORY) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to move %s, parent_to %s is not a dir\n", to, parent_to);
        return FAIL;
    }

    /* tries to get child inumber. it can be 'FAIL' if not found */
    child_from_inumber = lookup_sub_node(child_from, pdata_from.dirEntries);

    /* locks (write) the directory/file that will be moved */
    assert__(lock_write(child_from_inumber) == SUCCESS, "Error: move failed to lock an inode!\n")
    locked_inumbers[amount++] = child_from_inumber;

    /* if we couldn't find the node that is going to be moves, we show an error */
    if (child_from_inumber == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to move %s, child_from does not exist in dir %s\n", child_from, parent_from);
        return FAIL;
    }

    /* since we already have the child's inumber, we can get it's information */
    inode_get(child_from_inumber, &cType_from, &cdata_from);

    /* checks if there is already a node with this child name in this directory */
    child_to_inumber = lookup_sub_node(child_to, pdata_to.dirEntries);

    /* if we found a node with this name, we throw an error */
    if (child_to_inumber != FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to move %s, child_to already exists in dir %s\n", child_to, parent_to);
        return FAIL;
    }

    /* remove entry from folder that contained moved node */
    if (dir_reset_entry(parent_from_inumber, child_from_inumber) == FAIL) {
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("failed to move %s from dir %s\n", child_from, parent_from);
        return FAIL;
    }

    /* adds removed node to the destiny directory */
    if (dir_add_entry(parent_to_inumber, child_from_inumber, child_to) == FAIL) {
        dir_add_entry(parent_from_inumber, child_from_inumber, child_from);  /* if an error occurred, we have to add back the removed directory */
        unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */
        printf("could not move entry %s in dir %s\n", child_from, parent_to);
        return FAIL;
    }

    unlock_inodes(locked_inumbers, amount);  /* unlocks all the used inodes */

    return SUCCESS;
}


/*
 * Lookup for a given path. Does not unlock traveled inodes.
 * Input:
 *  - name: path of node
 *  - locked_inumbers: array that will hold all the inumbers of the traveled by inodes
 *  - amount: number of used locks
 *  - is_lookup: 1 if operation is lookup() and 0 if it is not
 * Returns:
 *  - inumber: identifier of the i-node, if found
 *  - FAIL: otherwise
 */
int traverse_path(char *name, int *locked_inumbers, int *amount, int is_lookup) {

    char full_path[MAX_FILE_NAME];
    char delim[] = "/";

    strcpy(full_path, name);

    /* start at root node */
    int current_inumber = FS_ROOT;

    /* use for copy and to store data */
    type nType;
    union Data data;

    /* used to make strtok_r thread safe */
    char *save_ptr;

    char *path = strtok_r(full_path, delim, &save_ptr);

    /* puts root in locking queue if it isn't there already */
    if ( ! check_if_node_is_in_array(current_inumber, locked_inumbers, *amount)) {
        if (path == NULL && ! is_lookup) lock_write(current_inumber);
        else lock_read(current_inumber);
        locked_inumbers[*amount] = current_inumber;
        *amount += 1;
    }

    /* get root inode data */
    inode_get(current_inumber, &nType, &data);

    /* search for all sub nodes */
    while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
        path = strtok_r(NULL, delim, &save_ptr);
        if ( ! check_if_node_is_in_array(current_inumber, locked_inumbers, *amount)) {
            if (path == NULL && ! is_lookup) lock_write(current_inumber);
            else lock_read(current_inumber);
            locked_inumbers[*amount] = current_inumber;
            *amount += 1;
        }
        inode_get(current_inumber, &nType, &data);
    }
    return current_inumber;
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - output_file_path: output file path
 * Output:
 *  - SUCCESS or error
 */
int print_tecnicofs_tree(char* output_file_path) {
    FILE *out = fopen(output_file_path, "w");
    assert__(out != NULL, "Error: print_tecnico_tree couldn't open output file!\n")
    inode_print_tree(out, FS_ROOT, "");
    fclose(out);
    return SUCCESS;
}


/*
 * Unlocks all the locked inodes inside the array.
 * Input:
 *   - locked_inumbers: array which holds all the inumbers of the locked nodes
 *   - amount: number of locks used
 * */
void unlock_inodes(const int locked_inumbers[MAX_PATH_INODE_LENGTH], int amount) {
    for(int i = 0; i < amount; i++) {
        assert__(unlock(locked_inumbers[i]) == SUCCESS, "Error: unlock_inodes failed to unlock a node!\n")
    }
}
