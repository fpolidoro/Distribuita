#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char* argv[])
{
	struct stat file1, file2;
	
	if (argc != 3)
	{
		printf("Usage: %s <file1> <file2>\n", argv[0]);
		return -1;
	}
	
	int result = stat(argv[1], &file1);
	if(result != 0)
	{
		printf("Error opening file %s\n", argv[1]);
		return -1;
	}
		
	result = stat(argv[2], &file2);
	if(result != 0)
	{
		printf("Error opening file %s\n", argv[2]);
		return -1;
	}
		
	if(file1.st_mtime != file2.st_mtime)
	{
		printf("Timestamps are not equal\n");
		return 1;
	}
	
	printf("Timestamps are EQUAL\n");
		
	return 0;
}
