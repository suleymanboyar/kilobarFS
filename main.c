#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct inode {
    int id;                     // Unique for whole FS
    char* name;                 // The name of the file/dir, can be infinit long
    char is_directory;          // dir(1) or file(0)
    char is_readonly;           // Readonly(1) or not(0)
    int filesize;               // Filesize in byte (for dirs the size is 0 byte)
    int num_entries;
    uintptr_t* entries;         // Pointer to array of child inodes, where each inode has size of 64 bit
};

struct inode *load_inode_from_table(char **cur_table){
    struct inode *this = (struct inode*) malloc(sizeof(struct inode));
    if (this == NULL){
        perror("Could not allocate memroy for inode");
        exit(1);
    }

    memcpy(&this->id, *cur_table, sizeof(this->id));
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
    this->entries = (uintptr_t*) malloc(this->num_entries * sizeof(*this->entries));
    if (this->entries == NULL){
        perror("Could not allocate memroy for entries");
        exit(1);
    }
    *cur_table += this->num_entries * sizeof(uintptr_t);
    for (int i = 0; i < this->num_entries; i++) {
        if (this->is_directory){
            struct inode *sub = load_inode_from_table(cur_table);
            this->entries[i] = (uintptr_t) sub;
        }
    }

    return this;
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
    struct inode *root = load_inode_from_table(&cur_table);

    free(table);
    // TODO 2023-10-22: correctly free all entries
    free(root->name);
    free(root->entries);
    free(root);
    return 0;
}
