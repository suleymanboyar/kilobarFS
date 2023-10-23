#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct inode {
    int id;                     // Unique for whole FS
    char* name;                 // The name of the file/dir, can be infinit long
    char is_directory;          // dir(1) or file(0)
    char is_readonly;           // Readonly(1) or not(0)
    int filesize;               // Filesize in byte (for dirs the size is 0 byte)
    int num_entries;            // Keeps track of the number of entries
    int entries_size;           // Keeps track of the memory size allocated to entires
    uintptr_t* entries;         // Pointer to array of child inodes, where each inode has size of 64 bit
} inode;

static size_t ID_COUNTER = 0;

size_t next_pow2(int x) {
    x--;
    x |= x>>1;
    x |= x>>2;
    x |= x>>4;
    x |= x>>8;
    x |= x>>16;
    x++;
    return x;
}

inode* init_inode() {
    inode *node = malloc(sizeof(inode));
    if (node == NULL){
        perror("Could not allocate memroy for inode\n");
        exit(EXIT_FAILURE);
    }
    node->id = -1;
    node->name = NULL;
    node->is_directory = 0;
    node->is_readonly = 0;
    node->filesize = 0;
    node->num_entries = 0;
    node->entries_size = 0;
    node->entries = NULL;
    return node;
}

inode *load_inode_from_table(char **cur_table){
    inode *this = init_inode();

    memcpy(&this->id, *cur_table, sizeof(this->id));
    ID_COUNTER = this->id + 1;
    *cur_table += sizeof(this->id);

    int name_length;    // Includes null byte
    memcpy(&name_length, *cur_table, sizeof(name_length));
    *cur_table += sizeof(name_length);
    this->name = (char*) malloc(name_length);
    strncpy(this->name, *cur_table, name_length);
    *cur_table += name_length;

    memcpy(&this->is_directory, *cur_table, sizeof(this->is_directory));
    *cur_table += sizeof(this->is_directory);
    memcpy(&this->is_readonly, *cur_table, sizeof(this->is_readonly));
    *cur_table += sizeof(this->is_readonly);
    memcpy(&this->filesize, *cur_table, sizeof(this->filesize));
    *cur_table += sizeof(this->filesize);
    memcpy(&this->num_entries, *cur_table, sizeof(this->num_entries));
    *cur_table += sizeof(this->num_entries);
    this->entries_size = this->num_entries;

    if (this->num_entries > 0){
        this->entries = malloc(this->num_entries * sizeof(*this->entries));
        if (this->entries == NULL){
            perror("Could not allocate memroy for entries\n");
            exit(EXIT_FAILURE);
        }

        *cur_table += this->num_entries * sizeof(uintptr_t);
        for (int i = 0; i < this->num_entries; i++) {
            if (this->is_directory){
                inode *sub = load_inode_from_table(cur_table);
                this->entries[i] = (uintptr_t) sub;
            }
        }
    } else {
        this->entries = NULL;
    }

    return this;
}

void create_dir(inode *dir, const char* name, bool readonly) {
    // don't allow creating directories in direcotries which is readonly
    if (dir->is_readonly)
        return;

    if (dir->num_entries == dir->entries_size) {
        dir->entries_size = next_pow2(dir->entries_size+1);
        dir->entries = realloc(dir->entries, dir->entries_size * sizeof(*dir->entries));
        if (dir->entries == NULL) {
            perror("Failed to resize entries\n");
            exit(EXIT_FAILURE);
        }
    }

    inode *sub_dir = init_inode();
    sub_dir->id = ID_COUNTER++;
    sub_dir->name = strdup(name);
    sub_dir->is_directory = 1;
    sub_dir->is_readonly = readonly;
    sub_dir->filesize = 0;
    int ind = dir->num_entries < 0 ? 0 : dir->num_entries++;
    dir->entries[ind] = (uintptr_t) sub_dir;
}

void create_file(inode *dir, const char* name, bool readonly) {
    // don't allow creating files in direcotries which is readonly
    if (dir->is_readonly)
        return;

    if (dir->num_entries == dir->entries_size) {
        dir->entries_size = next_pow2(dir->entries_size+1);
        dir->entries = realloc(dir->entries, dir->entries_size * sizeof(*dir->entries));
        if (dir->entries == NULL) {
            perror("Failed to resize entries\n");
            exit(EXIT_FAILURE);
        }
    }

    inode *file = init_inode();
    dir->id = ID_COUNTER++;
    dir->name = strdup(name);
    dir->is_directory = 0;
    dir->is_readonly = readonly;
    dir->filesize = 0;
    int ind = dir->num_entries < 0 ? 0 : dir->num_entries++;
    dir->entries[ind] = (uintptr_t) file;
}

inode *find_dir(inode *dir, const char* name) {
    if (!dir->is_directory){
        perror("Expected a directory as root for seaching for a sub-directory\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < dir->num_entries; i++) {
        inode *node = (inode*) dir->entries[i];
        if (node->is_directory && strcmp(node->name, name) == 0)
            return node;
    }
    return NULL;
}

inode *find_file(inode *dir, const char* name) {
    if (!dir->is_directory){
        perror("Expected a directory as root for seaching for a file\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < dir->num_entries; i++) {
        inode *node = (inode*) dir->entries[i];
        if (!node->is_directory && strcmp(node->name, name) == 0)
            return node;
    }
    return NULL;
}

void free_fs(inode *this) {
    for (int i = 0; i < this->num_entries && this->is_directory; ++i) {
        free_fs((inode*)(this->entries[i]));
    }
    free(this->name);
    free(this->entries);
    free(this);
}

static void _print_fs(inode *node, int indent){
    // search through all of the direcotries first
    for (int i = 0; i < node->num_entries; ++i) {
        inode *sub_node = (inode*)(node->entries[i]);
        if (sub_node->is_directory) {
            printf("%*s- %s/\n", indent, "", sub_node->name);
            _print_fs(sub_node, indent+1);
        }
    }

    // print each file at the end of the directories
    for (int i = 0; i < node->num_entries; ++i) {
        inode *sub_node = (inode*)(node->entries[i]);
        if (!sub_node->is_directory) {
            printf("%*s- %s\n", indent, "", sub_node->name);
        }
    }
}

void print_fs(inode *node){
    if (!node->is_directory || strcmp(node->name, "/") == 0)
        printf("- %s\n", node->name);
    else
        printf("- %s/\n", node->name);

    if (node->is_directory)
        _print_fs(node, 1);
}

int main(void) {
    FILE *f = fopen("master_file_table", "rb");
    fseek(f, 0, SEEK_END);
    long table_size = ftell(f);
    rewind(f);
    char *table = malloc(table_size);
    fread(table, table_size, 1, f);
    fclose(f);
    char *cur_table = table;    // keep track of cursor

    inode *root = load_inode_from_table(&cur_table);
    print_fs(root);

    free(table);
    free_fs(root);
    return 0;
}
