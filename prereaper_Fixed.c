#define _CRT_SECURE_NO_WARNINGS
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SZ_BUFF 1024
#define SZ_WINDOW 4096

typedef struct _PRE
{
	long szFile;
	long szCFile;
	long offset;
	unsigned long checksum;
	char * name;
} PRE;

void mkdirs(char * filepath)
{
	char * fpcp = filepath;
	char * fetch = fpcp;
	char * cwd = _getcwd(0, 0);

	char * temp = 0;
	long tlen = 0;

	while (*fpcp && *fetch)
	{
		while (*fetch && *fetch != '\\')
		{
			fetch++;
			if (*fetch == '/') *fetch = '\\';
		}

		if (!*fetch) break;

		tlen = (fetch - fpcp) + 2;
		temp = (char *)malloc(tlen);
		memset(temp, 0, tlen--);
		strncpy(temp, (const char *)fpcp, tlen);

		if (_chdir(temp))
		{
			_mkdir(temp);
			_chdir(temp);
		}

		free(temp);

		if (*(fetch++))
		{
			fpcp = fetch;
		}
		else break;
	}

	_chdir(cwd);
	free(cwd);
}

int decompress_out(long * data, FILE * newfile, PRE * header)
{
	void * ucbuff;
	unsigned char * dptr;
	unsigned char * ucptr;
	unsigned char bm[8];
	int i, k;
	unsigned long offset;
	unsigned char size;
	unsigned char flag;
	long location;
	int ret;
	long window;

	if (!(ucbuff = malloc(header->szFile)))
	{
		printf("Failed to malloc %d bytes for decompression buffer\n", header->szFile);
		return -1;
	}

	dptr = (unsigned char *)data;
	ucptr = (unsigned char*)ucbuff;
	memset(bm, 0x00, 8);
	memset(ucbuff, 0x00, header->szFile);

	location = 0;

	while (location < header->szCFile)
	{
		bm[0] = dptr[0] << 7;
		bm[0] = bm[0] >> 7;
		bm[1] = dptr[0] << 6;
		bm[1] = bm[1] >> 7;
		bm[2] = dptr[0] << 5;
		bm[2] = bm[2] >> 7;
		bm[3] = dptr[0] << 4;
		bm[3] = bm[3] >> 7;
		bm[4] = dptr[0] << 3;
		bm[4] = bm[4] >> 7;
		bm[5] = dptr[0] << 2;
		bm[5] = bm[5] >> 7;
		bm[6] = dptr[0] << 1;
		bm[6] = bm[6] >> 7;
		bm[7] = dptr[0] >> 7;

		dptr++;
		location++;

		for (i = 0; i < 8; i++)
		{
			if (location >= header->szCFile)
				break;

			if (bm[i])
			{
				memmove(ucptr++, dptr++, 1);
				location++;
			}
			else
			{
				flag = dptr[1] >> 4;
				window = (long)((ucptr - (unsigned char *)ucbuff) / SZ_WINDOW) * SZ_WINDOW;

				if ((ucptr - (unsigned char *)ucbuff) < (window + (flag * 256) + dptr[0] + 18))
					window = window - SZ_WINDOW;

				offset = dptr[0] + 18 + (flag * 256) + window;
				size = dptr[1] << 4;
				size = size >> 4;
				size = size + 3;

				for (k = 0; k < size; k++)
				{
					memmove(ucptr, (unsigned char *)ucbuff + offset, 1);
					ucptr++;
					offset++;
				}

				dptr = dptr + 2;
				location = location + 2;
			}
		}
	}

	ret = (int)fwrite(ucbuff, 1, header->szFile, newfile);
	free(ucbuff);
	return ret;
}

int main(int nArgs, char * pszArgs[])
{
	int i;
	FILE * pFile;
	FILE * outFile;
	char * buffer;
	char * bufpnt;
	unsigned long fLen;

	_chdir("C:\\");
	char * cwd = _getcwd(0, 0);

	unsigned long flen;
	unsigned long numf;

	PRE * head;

	printf("PRXReaper for THUG, v0.1 by reaper (feat. Sporadic)\n");
	printf("Warning: Keep out of reach of children!\n\n");

	if (nArgs > 1)
	{
		pFile = fopen(pszArgs[1], "rb");
		fseek(pFile, 0, SEEK_END);
		fLen = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		buffer = (char *)malloc(fLen);
		bufpnt = buffer;
		fread(buffer, 1, fLen, pFile);
		fclose(pFile);

		memcpy(&flen, bufpnt, 4);
		bufpnt += 8;
		memcpy(&numf, bufpnt, 4);
		bufpnt += 4;

		head = (PRE *)malloc(sizeof(PRE));

		printf("Please wait...\n");

		for (i = 0; i < numf; i++)
		{
			while ((bufpnt - buffer) < flen)
			{
				memset(head, 0, sizeof(PRE));
				memcpy(head, bufpnt, 16);
				bufpnt += 16;

				head->name = bufpnt;
				bufpnt += head->offset;

				mkdirs(head->name);
				outFile = fopen(head->name, "wb");

				if (head->szCFile)
				{
					decompress_out((long *)bufpnt, outFile, head);
					bufpnt += head->szCFile;

					while (((bufpnt - buffer) % 4) != 0) bufpnt++;   /* Align pointer to 32bits */
				}
				else
				{
					fwrite(bufpnt, 1, head->szFile, outFile);
					bufpnt += head->szFile;

					while (((bufpnt - buffer) % 4) != 0) bufpnt++;
				}

				fclose(outFile);
			}
		}

		free(head);
		free(buffer);

		printf("Files extracted to: %s\n", cwd);
	}

	system("PAUSE");
	return 0;
}