#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    unsigned char jump[3]; //00
    char osname[8]; //03
    unsigned short sector_size; //0b
    unsigned char sectors_per_cluster; //0d
    unsigned short reserved_sectors; //0e
    unsigned char number_of_fats; //10
    unsigned short root_dir_entries; //11
    unsigned short total_sectors_short; //13
    unsigned char media_descriptor; //15
    unsigned short sectors_per_fat; //16
    unsigned short sectors_per_track; //18
    unsigned short number_of_heads; //1a
    unsigned long hidden_sectors; //1c
    unsigned long total_sectors_long;  //20
    unsigned short ignore; //24
    unsigned char boot_signature; //26
    unsigned long volume_id; //27
    char volume_label[11]; //2b
    char fs_type[8]; //36
} __attribute((packed)) Fat12BootSector;

typedef struct {
    unsigned char filename[8]; //00
    unsigned char ext[3]; //08
    unsigned char attributes; //11
    unsigned char reserved; //12
    unsigned char create_time_fine; //13
    unsigned short create_time; //14
    unsigned short create_date; //16
    unsigned short last_access_date; //18
    unsigned short ea_index; //20
    unsigned short last_modified_time; //22
    unsigned short last_modified_date; //24
    unsigned short first_cluster; //26
    unsigned int file_size;
} __attribute((packed)) Fat12Entry;
