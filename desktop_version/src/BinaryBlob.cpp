#include "BinaryBlob.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: Abstract to FileSystemUtils! */
#include <physfs.h>

binaryBlob::binaryBlob()
{
	numberofHeaders = 0;
	for (int i = 0; i < 128; i += 1)
	{
		m_memblocks[i] = NULL;
		m_headers[i].valid = false;

		for (int j = 0; j < 48; j += 1)
		{
			m_headers[i].name[j] = '\0';
		}
	}
}

binaryBlob::~binaryBlob()
{
	for (int i = 0; i < 128; i += 1)
		free(m_memblocks[i]);
}

#ifdef VVV_COMPILEMUSIC
void binaryBlob::AddFileToBinaryBlob(const char* _path)
{
	long size;
	char * memblock;

	FILE *file = fopen(_path, "rb");
	if (file != NULL)
	{
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);

		memblock = (char*) malloc(size);
		fread(memblock, 1, size, file);

		fclose(file);

		printf("The complete file size: %li\n", size);

		m_memblocks[numberofHeaders] = memblock;
		for (int i = 0; _path[i]; i += 1)
		{
			m_headers[numberofHeaders].name[i] = _path[i];
		}

		m_headers[numberofHeaders].valid = true;
		m_headers[numberofHeaders].size = size;
		numberofHeaders += 1;
	}
	else
	{
		printf("Unable to open file\n");
	}
}

void binaryBlob::writeBinaryBlob(const char* _name)
{
	FILE *file = fopen(_name, "wb");
	if (file != NULL)
	{
		fwrite((char*) &m_headers, 1, sizeof(resourceheader) * 128, file);

		for (int i = 0; i < numberofHeaders; i += 1)
		{
			fwrite(m_memblocks[i], 1, m_headers[i].size, file);
		}

		fclose(file);
	}
	else
	{
		printf("Unable to open new file for writing. Feels bad.\n");
	}
}
#endif

bool binaryBlob::unPackBinary(const char* name)
{
	PHYSFS_sint64 size;
	for (int i = 0; i < 128; ++i) { free(m_memblocks[i]); m_memblocks[i] = NULL; }

	PHYSFS_File *handle = PHYSFS_openRead(name);
	if (handle == NULL)
	{
		printf("Unable to open file %s\n", name);
		return false;
	}

	size = PHYSFS_fileLength(handle);

	if (size < (PHYSFS_sint64)sizeof(m_headers) || size > 64 * 1024 * 1024 ||
		PHYSFS_read(handle, &m_headers, 1, sizeof(m_headers)) != sizeof(m_headers))
	{
		PHYSFS_close(handle);
		return false;
	}

	int offset = 0 + (sizeof(resourceheader) * 128);

	for (int i = 0; i < 128; i += 1)
	{
		if (m_headers[i].valid)
		{
			if (memchr(m_headers[i].name, 0, sizeof(m_headers[i].name)) == NULL ||
				m_headers[i].size < 0 || m_headers[i].size > size - offset)
			{
				PHYSFS_close(handle);
				return false;
			}
			PHYSFS_seek(handle, offset);
			m_memblocks[i] = (char*) malloc(m_headers[i].size);
			if (m_memblocks[i] == NULL || PHYSFS_read(handle, m_memblocks[i], 1, m_headers[i].size) != m_headers[i].size)
			{
				PHYSFS_close(handle);
				return false;
			}
			offset += m_headers[i].size;
		}
	}
	PHYSFS_close(handle);

	printf("The complete reloaded file size: %li\n", size);

	for (int i = 0; i < 128; i += 1)
	{
		if (m_headers[i].valid == false)
		{
			break;
		}

		printf("%s unpacked\n", m_headers[i].name);
	}

	return true;
}

int binaryBlob::getIndex(const char* _name)
{
	for (int i = 0; i < 128; i += 1)
	{
		if (m_headers[i].valid && strcmp(_name, m_headers[i].name) == 0)
		{
			return i;
		}
	}
	return -1;
}

int binaryBlob::getSize(int _index)
{
	return _index >= 0 && _index < 128 ? m_headers[_index].size : 0;
}

char* binaryBlob::getAddress(int _index)
{
	return _index >= 0 && _index < 128 ? m_memblocks[_index] : NULL;
}
