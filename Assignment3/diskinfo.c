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

int main(int argc, char** argv)
{
    FILE *fp;
    Fat12BootSector bootsector;
    Fat12Entry rootentry;
    int i, file_count = 0, space_used = 0;
    char volume_label[11];
    
    if ((fp=fopen(argv[1],"r")))
    {
        // Load Bootsector
        fseek(fp, 0, SEEK_SET);
        fread(&bootsector, sizeof(Fat12BootSector), 1, fp);

        // Copy Volume Label if Present
        mystrcpy(volume_label, bootsector.volume_label);
        
        // Load Root Directory
        fseek(fp, (bootsector.reserved_sectors + (bootsector.number_of_fats * bootsector.sectors_per_fat)) * bootsector.sector_size, SEEK_SET);
    
        // Read Each Root Directory Entry
        for(i=0; i<bootsector.root_dir_entries; i++){
            fread(&rootentry, sizeof(rootentry), 1, fp);
            if (rootentry.attributes  == 0x08) {
                // Update Volume Label if Present
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
                // File, increment file count and update space used.
                file_count++;
                space_used = space_used + rootentry.file_size;
            }
        }

        // Print out results
        printf("OS Name: %s\n", bootsector.osname);
        printf("Label of the disk: %.11s\n", volume_label);
        printf("Total size of the disk: %d\n", bootsector.total_sectors_short * 512); 
        printf("Free size of the disk: %d\n", bootsector.total_sectors_short * 512 - space_used - bootsector.reserved_sectors * 512);
        printf("\n==============\n");
        printf("The number of files in the root directory (not including subdirectories): %d\n", file_count);
        printf("\n==============\n");
        printf("Number of FAT copies: %d\n", bootsector.number_of_fats);
        printf("Sectors per FAT: %d\n", bootsector.sectors_per_fat);
        
    }

    else
        // Error message
        printf("Failed to open the image file.\n");


    fclose(fp);
    return 0;
}
