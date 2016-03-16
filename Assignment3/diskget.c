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

// Read the file starting at "cluster"
void fat_read_file(FILE * in, FILE * out,
                   unsigned long fat_start, 
                   unsigned long data_start, 
                   unsigned long cluster_size, 
                   unsigned short cluster, 
                   unsigned long file_size) {
    unsigned char buffer[4096];
    size_t bytes_read, bytes_to_read,
           file_left = file_size, cluster_left = cluster_size;

    // Go to first data cluster
    fseek(in, data_start + cluster_size * (cluster-2), SEEK_SET);
    
    // Read until we run out of file or clusters
    while(file_left > 0 && cluster != 0xFFF) {
        bytes_to_read = sizeof(buffer);
        
        // don't read past the file or cluster end
        if(bytes_to_read > file_left)
            bytes_to_read = file_left;
        if(bytes_to_read > cluster_left)
            bytes_to_read = cluster_left;
        
        // read data from cluster, write to file
        bytes_read = fread(buffer, 1, bytes_to_read, in);
        fwrite(buffer, 1, bytes_read, out);
        
        // decrease byte counters for current cluster and whole file
        cluster_left -= bytes_read;
        file_left -= bytes_read;
        
        // if we have read the whole cluster, read next cluster # from FAT
        if(cluster_left == 0) {
            fseek(in, fat_start + (cluster*3)/2, SEEK_SET);

            if (cluster % 2) {
                cluster = read_odd(in);
            }
            else {
                cluster = read_even(in);
            }

            fseek(in, data_start + cluster_size * (cluster-2), SEEK_SET);
            cluster_left = cluster_size; // reset cluster byte counter
        }
    }
}

int main(int argc, char** argv)
{
    FILE *fp, *out;
    Fat12BootSector bootsector;
    Fat12Entry rootentry;
    int i, j;
    char volume_label[11];
    char filename[9] = "        ", file_ext[4] = "   "; // initially pad with spaces
    int data_start_addr, fat_start, root_sector_start;

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
    
        // Find the root entry with the matching filename
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
            }
            else if (rootentry.filename[0] == 0x05) {
                // Free Entry
            }
            else if (rootentry.filename[0] == 0x00) {
                // End of directory, Break for loop
                break;
            }
            else if (rootentry.filename[0] == 0x2E) {
                // Directory
            }
            else {
  
                if(memcmp(rootentry.filename, filename, 8) == 0 && 
                    memcmp(rootentry.ext, file_ext, 3) == 0) {
                    break;
                }
            }
        }

        if (i == bootsector.root_dir_entries) {
            printf("File not found.");
            return -1;
        }

        // Create new file to write to
        out = fopen(argv[2], "wb");
        
        // Read the Fat file to the newly created output file
        fat_read_file(fp, out,
           fat_start, 
           data_start_addr, 
           bootsector.sectors_per_cluster * bootsector.sector_size, 
           rootentry.first_cluster, 
           rootentry.file_size);
        fclose(out);

    }

    else
        // Error message
        printf("Fail to open the image file.\n");


    fclose(fp);
    return 0;
}
