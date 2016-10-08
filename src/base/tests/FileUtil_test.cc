#include "base/FileUtil.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("Usage: %s -[e|f|d] path\n", argv[0]);
		abort();
	}

	do 
	{
		if (strcmp(argv[1], "-e") == 0)
		{
			if (base::FileUtil::fileExists(argv[2]))
			{
				printf("%s exists\n", argv[2]);
			}
			else
			{
				printf("%s non exists\n", argv[2]);
			}
			break;
		}

		if (strcmp(argv[1], "-f") == 0)
		{
			if (base::FileUtil::isFile(argv[2]))
			{
				printf("%s is file\n", argv[2]);
			}
			else
			{
				printf("%s is not file\n", argv[2]);
			}
			break;
		}

		if (strcmp(argv[1], "-d") == 0)
		{
			if (base::FileUtil::isDirectory(argv[2]))
			{
				printf("%s is isDirectory\n", argv[2]);
			}
			else
			{
				printf("%s is not isDirectory\n", argv[2]);
			}
			break;
		}

		if (strcmp(argv[1], "-r") == 0)
		{
			std::string content;
			if (base::FileUtil::getFileContents(argv[2], &content))
			{
				printf("file content:\n%s\n", content.c_str());
			}
			else
			{
				printf("reading content of file failed: %s\n", argv[2]);
			}
			break;
		}
	} while (false);
	
	return 0;
}