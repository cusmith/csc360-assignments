#include<stdio.h>
#include<stdlib.h>

void getOSName(FILE *fp, char *osname)
{
	fseek(fp,3L,SEEK_SET);
	fread(osname,1,8,fp);

}

int getSize(FILE *fp)
{
	int *tmp1 = malloc(sizeof(int));
	int *tmp2 = malloc(sizeof(int));
	int retVal;
	fseek(fp,19L,SEEK_SET);
	fread(tmp1,1,1,fp);
	fread(tmp2,1,1,fp);
	retVal = *tmp1+((*tmp2)<<8);
	free(tmp1);
	free(tmp2);
	return retVal;
};


int main(int argc, char** argv)
{
	FILE *fp;
	char *osname = malloc(sizeof(char)*8);
	int size;
	
	if ((fp=fopen(argv[1],"r")))
	{
		printf("Successfully open the image file.\n");
		
		getOSName(fp,osname);
		printf("OS Name: %s\n", osname);
		
		size = getSize(fp);
		printf("Total Sectors: %d\n", size);		

	}

	else
		printf("Fail to open the image file.\n");

	free(osname);
	fclose(fp);
	return 0;
}
