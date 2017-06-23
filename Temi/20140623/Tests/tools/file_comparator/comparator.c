#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#define BUF_SIZE	512

int
main(int argc, char* argv[])
{
	if(argc != 4)
	{
		printf("Usage: %s <file1> <file2> <error>\n", argv[0]);
		return -1;
	}
	char	buffer[BUF_SIZE];
	char	buffer2[BUF_SIZE];
	float	data1, data2, temp;
	float	err = strtof(argv[3], NULL);
	int 	error = 0;		// Returns 0 on success
	int 	res;
	FILE* f1 = fopen (argv[1], "rt");
	FILE* f2 = fopen (argv[2], "rt");
	if(f1 == NULL || f2 == NULL)
	{
		printf("Error opening file\nExiting.");
		return -2;
	}
	while(fgets(buffer, BUF_SIZE, f1) != NULL)
	{
		if(fgets(buffer2, BUF_SIZE, f2) == NULL)
		{
			error = 1;
			break;
		}
		if((res = sscanf (buffer, "%f\n", &data1)) != 1)
		{
			error = 1;
			break;
		}
		if((res = sscanf (buffer2, "%f\n", &data2)) != 1)
		{
			error = 1;
			break;
		}
		temp = fabs(data1-data2);
		if(temp > err)
		{
			error = 1;
			break;
		}
	}
	if(fgets(buffer2, BUF_SIZE, f2) != NULL)
		error = 1;		// <file2> is longer!!
	fclose(f1);
	fclose(f2);
	return error;
}
