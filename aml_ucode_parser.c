/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright 2019 BayLibre SAS */
/* Copyright 2015 Amlogic */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define PACK ('P' << 24 | 'A' << 16 | 'C' << 8 | 'K')
#define CODE ('C' << 24 | 'O' << 16 | 'D' << 8 | 'E')

#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)              __ALIGN_MASK(x,(typeof(x))(a)-1)

const char zeros[0x8000] = { 0 };

struct firmware_header_s {
	int magic;
	int checksum;
	char name[32];
	char cpu[16];
	char format[32];
	char version[32];
	char author[32];
	char date[32];
	char commit[16];
	int data_size;
	unsigned int time;
	char reserved[128];
};

struct package_header_s {
	int magic;
	int size;
	int checksum;
	char reserved[128];
};

struct package_s {
	union {
		struct package_header_s header;
		char buf[256];
	};
	char data[0];
};

struct info_header_s {
	char name[32];
	char format[32];
	char cpu[32];
	int length;
};

struct package_info_s {
	union {
		struct info_header_s header;
		char buf[256];
	};
	char data[0];
};

int parse_package(void *buf)
{
	struct package_info_s *pack_info;
	char *pack_data;

	pack_data = ((struct package_s *)buf)->data;
	pack_info = (struct package_info_s *)pack_data;

	printf("%s ; %s ; %s ; %d\n", pack_info->header.name, pack_info->header.format, pack_info->header.cpu, pack_info->header.length);

	return 512;
}

void dump_to_file(const char *name, void *buf, int size)
{
	int fd = open(name, O_CREAT | O_WRONLY, 0644);

	if (size <= 0x4000) {
		write(fd, buf, size);
		write(fd, zeros, 0x4000-size);
	} else { /* H264 firmwares with extendedd data */
		write(fd, buf, 0x4000);
		write(fd, buf + 0x4000, 0x1000);
		write(fd, buf + 0x2000, 0x1000);
		write(fd, buf + 0x6000, 0x1000);
		write(fd, buf + 0x3000, 0x1000);
		write(fd, buf + 0x5000, 0x1000);
	}

	close(fd);
}

int code_real_size(void *buf, int orig_size)
{
	const char *c = buf;
	int i;

	for (i = orig_size - 1; !c[i]; --i);
	return i + 1;
}

int parse_code(void *buf)
{
	struct firmware_header_s *fh = buf;
	int real_size;

	printf("Name: %s\n", fh->name);
	printf("CPU: %s\n", fh->cpu);
	printf("Format: %s\n", fh->format);
	printf("Version: %s\n", fh->version);
	printf("Author: %s\n", fh->author);
	printf("Date: %s\n", fh->date);
	printf("Size: %d\n", fh->data_size);

	buf += 512;
	real_size = code_real_size(buf, fh->data_size);
	printf("RealSize: %d (%08X)\n", real_size, real_size);
	dump_to_file(fh->name, buf, real_size);
	printf("\n");
	return 512+fh->data_size;
}

int dump_file(const char* path, void **buf, int *size)
{
	FILE *f = fopen(path, "rb");

	if (!f) {
		printf("Couldn't open %s\n", path);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	*buf = malloc(*size + 1);
	fread(*buf, *size, 1, f);
	fclose(f);

	printf("Loaded %s: %d bytes\n", path, *size);
	return 0;
}

int main(int argc, char *argv[])
{
	void *buf;
	int size;
	int cur = 256; // skip first 256 bytes (signature)

	if (dump_file("video_ucode.bin", &buf, &size))
		return EXIT_FAILURE;

	while (cur < size) {
		struct package_header_s* ph = buf + cur;

		switch (ph->magic)
		{
		case PACK:
			cur += parse_package(buf + cur);
			break;
		case CODE:
			cur += parse_code(buf + cur);
			break;
		default:
			cur = ALIGN(cur + 1, 16);
			break;
		}
	}

	return 0;
}
