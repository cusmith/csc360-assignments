#include "disk.h"

// Helper function for copying strings
void mystrcpy(char dest[], char src[])
{
int i = 0;

while(src[i] != '\0')
    {
    dest[i] = src[i];
    i++;
    }

dest[i] = '\0';
}

// Read the even FAT entry at the current file pointer Y Z _ X _ _ (XYZ)
short read_even(FILE * fp) {
    char *tmp1 = malloc(sizeof(char));
    char *tmp2 = malloc(sizeof(char));
    short retVal, tmp;

    fread(tmp1,1,1,fp);
    fread(tmp2,1,1,fp);
    retVal = *tmp1;
    retVal = retVal & 0x00FF;
    tmp = *tmp2 & 0x0F;
    tmp <<= 8;
    retVal = retVal + tmp;
    free(tmp1);
    free(tmp2);
    return retVal;
}

// Read the odd FAT entry at the current file pointer _ _ Z _ X Y (XYZ)
short read_odd(FILE * fp) {
    char *tmp1 = malloc(sizeof(char));
    char *tmp2 = malloc(sizeof(char));
    short retVal, tmp;

    fread(tmp1,1,1,fp);
    fread(tmp2,1,1,fp);
    retVal = *tmp1;
    retVal >>= 4;
    retVal = retVal & 0x000F;
    tmp = *tmp2;
    tmp <<=4;
    tmp = tmp & 0x0FF0;
    retVal = retVal + tmp;
    free(tmp1);
    free(tmp2);
    return retVal;
}

// Find the first free cluster to write the input file to
short find_free_cluster(FILE * fp, 
        unsigned long fat_start, 
        unsigned long data_start, 
        unsigned long cluster_size,
        unsigned long fat_size) {
    int i;

    for (i = 2; i < fat_size; i = i + 3) {
        fseek(fp, fat_start + i, SEEK_SET);
        if (read_even(fp) == 0x000) {
            fseek(fp, fat_start + i, SEEK_SET);

            return fat_start + i;
        }
        fseek(fp, fat_start + i, SEEK_SET);
        if (read_odd(fp) == 0x000) {
            return fat_start + i + 1;
        }
    }
    return 0;
}

// Write to the specified cluster from the input file
int fat_write_cluster(FILE * fp, FILE * input_file,
                   unsigned long data_start, 
                   unsigned long cluster_size, 
                   unsigned short cluster, 
                   unsigned long file_left) {
    unsigned char buffer[4096];
    size_t bytes_read, bytes_to_read;

    // Go to first data cluster
    fseek(fp, data_start + cluster_size * (cluster-2), SEEK_SET);
    
    // Read until we run out of file or clusters
    bytes_to_read = sizeof(buffer);
    
    // don't read past the file or cluster end
    if(bytes_to_read > file_left)
        bytes_to_read = file_left;
    
    // read data from cluster, write to file
    bytes_read = fread(buffer, 1, bytes_to_read, input_file);
    fwrite(buffer, 1, bytes_read, fp);
    
    // decrease byte counters for current cluster and whole file
    file_left -= bytes_read;
    
    return 0;
}

int main(int argc, char** argv)
{
    FILE *fp, *input_file;
    Fat12BootSector bootsector;
    Fat12Entry rootentry;
    int i, j, file_remaining;
    char volume_label[11];
    char filename[9] = "        ", file_ext[4] = "   "; // initially pad with spaces
    int data_start_addr, fat_start, root_sector_start;
    short free_cluster;

    if ((fp=fopen(argv[1],"r")))
    {

        // Format Filename and Extension
        for(i=0; i<8 && argv[2][i] != '.' && argv[2][i] != 0; i++)
            filename[i] = argv[2][i];
        for(j=1; j<=3 && argv[2][i+j] != 0; j++)
            file_ext[j-1] = argv[2][i+j];

        // Load Bootsector
        fseek(fp, 0, SEEK_SET);
        fread(&bootsector, sizeof(Fat12BootSector), 1, fp);
        mystrcpy(volume_label, bootsector.volume_label);
        
        // Record fat start
        fat_start = bootsector.reserved_sectors * bootsector.sector_size;
        root_sector_start = fat_start + ((bootsector.number_of_fats * bootsector.sectors_per_fat)) * bootsector.sector_size;
        data_start_addr = root_sector_start + bootsector.root_dir_entries * sizeof(rootentry);

        fseek(fp, root_sector_start, SEEK_SET);
    
        // Find the first free root entry
        for(i=0; i<bootsector.root_dir_entries; i++){
            fread(&rootentry, sizeof(rootentry), 1, fp);
            if (rootentry.attributes  == 0x08) {
                mystrcpy(volume_label, rootentry.filename);
            }
            else if (rootentry.attributes == 0x0F) {
                // Fake Entry
            }
            else if (rootentry.filename[0] == 0xE5) {
                // Free Entry
                break;
            }
            else if (rootentry.filename[0] == 0x05) {
                // Free Entry
                break;
            }
            else if (rootentry.filename[0] == 0x00) {
                // End of directory, Break for loop
                break;
            }
            else if (rootentry.filename[0] == 0x2E) {
                // Directory
            }
            else {
                // File
            }
        }

        if (i == bootsector.root_dir_entries) {
            printf("File not found.");
            return -1;
        }

        
        // Find a free cluster
        free_cluster = find_free_cluster(fp, 
            fat_start, 
            data_start_addr, 
            bootsector.sectors_per_cluster * bootsector.sector_size,
            bootsector.sectors_per_fat * bootsector.sector_size);

        // Open the input file
        input_file = fopen(argv[2], "r");
        fseek(input_file, 0L, SEEK_END);

        // Determine the input file size
        file_remaining = ftell(input_file);
        fseek(input_file, 0L, SEEK_SET);

        // Write the filename and extension to the 
        fseek(fp, root_sector_start + i * sizeof(rootentry), SEEK_SET);
        fwrite(filename, 1, 8, fp);
        fwrite(file_ext, 1, 3, fp);

        // Write to the first free cluster
        fat_write_cluster(fp, input_file,
                   data_start_addr, 
                   bootsector.sectors_per_cluster * bootsector.sector_size, 
                   free_cluster, 
                   file_remaining);

    }

    else
        // Error message
        printf("Fail to open the image file.\n");


    fclose(fp);
    return 0;
}
