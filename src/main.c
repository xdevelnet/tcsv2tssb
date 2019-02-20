/*
 * Copyright (c) 2019, Xdevelnet (xdevelnet at xdevelnet dot org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Some portions of this software was based on csvparser library examples
 * which is licensed under MIT license.
 *
 * Also, for my humble opinion, all these new and shiny libraries has lack
 * of quality, feature support, programmer-friendliness, etc. Seems like old
 * and dusty csvparser (that comes from _ancient_ sourceforge) is STILL better.
 * Unfortunately, it contains A LOT of memory leak issues.
 *
 * Someone, help!
 * You may:
 *
 * 1) Propose me other library. Related thread: https://github.com/JamesRamm/csv_parser/issues/2
 * 2) Fix this goddamn library
 */
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <string.h> // strerror()
#include <errno.h>
#include <unistd.h>
#include <iso646.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <csvparser.c> // http://sourceforge.net/projects/cccsvparser/

#define strizeof(a) (sizeof(a)-1)
#define SSB_BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100) // MUHAHAHAH, I REALLY FOUND THIS MAGNIFICENT <3 <3 <3
#define IS_LITTLE_ENDIAN (((union { unsigned x; unsigned char c; }){1}).c) // requires c99

#define POSIX_FAIL -1
#define SHORTEST_POSSIBLE_CSV_FILENAME_LEN 4 // strlen("a.csv")
#define HEADER_DEFINE_PREFIX_LANG "SSBLANG_"
#define HEADER_DEFINE_PREFIX_BASES "SSBBASE_"

const char ssb_header_itself[] = "SSBTRANSLATI0NS_1";
const char csv_ext_string[] = "csv";
const char ext_string[] = "ssb";

typedef struct {
	int fd;
	unsigned char max;
	size_t metalocation;
	size_t allsize;
} ssb_t;

void *ememcpy(void *dest, const void *src, size_t n) {
	// above
	// like memcpy(), but returns addres of byte after copied block. I.e. dest + n
	memcpy(dest, src, n);
	return dest + n;
}

void swapbytes(void *pv, size_t n) {
	// above
	// Swap bytes in custom length block
	assert(n > 0);

	char *p = pv;
	size_t lo, hi;
	for(lo = 0, hi = n - 1; hi > lo; lo++, hi--) {
		char tmp = p[lo];
		p[lo] = p[hi];
		p[hi] = tmp;
	}
}

int check_rfile_prepare_wfilefd(const char *rfilename, const char *wfileextension) {
	// Check csv file for read and prepare writeonly file with same name, but different extension
	size_t rfilename_len = strlen(rfilename);
	size_t wfileextension_len = strlen(wfileextension);

	if (rfilename_len < SHORTEST_POSSIBLE_CSV_FILENAME_LEN or memcmp(csv_ext_string, rfilename + rfilename_len - strizeof(csv_ext_string), strizeof(csv_ext_string)) != 0) {
		fprintf(stderr, "%s is probably not csv file. If it is, it sould have at least \".csv\" at end of filename.\n", rfilename);
		return POSIX_FAIL;
	}

	char wfilename[rfilename_len + wfileextension_len]; // I know it's much more then enough, IDC
	memcpy(ememcpy(wfilename, rfilename, rfilename_len - strizeof(csv_ext_string)), wfileextension, wfileextension_len + 1); // + 1 because I need null terminator

	int fd = open(wfilename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0) return perror(wfilename), errno;
	return fd;
}

int isalpha_real(char c) {
	// above
	// Like isalpha(), but really checks for ASCII latin character, so function argument have char type instead of int
	return ((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') ? 1 : 0);
}

static inline char force_uppecase(char c) {
	// above
	// makes character in uppercase, but expects only latin ASCII character, e.g. a-z A-Z
	if (c > 96) return c - ('a' - 'A');
	return c;
}

size_t latincpy_and_make_them_uppercase(void *dest, const void *src, size_t length) {
	// above
	// same as latincpy(), but also makes copied characters uppercase
	register char *d = dest;
	register const char *s = src;
	while (s < (const char *) src + length) {
		if (isalpha_real(*s)) *d++ = force_uppecase(*s);
		s++;
	}
	return d - (const char *) dest;
}

void header_add_lang(const char *str, size_t row, int hfd) {
	// above
	// Add new language to header file
	size_t length = strlen(str);
	char buffer[length];
	buffer[latincpy_and_make_them_uppercase(buffer, str, length)] = '\0';
	dprintf(hfd, "#define "HEADER_DEFINE_PREFIX_LANG"%s %zu\n", buffer, row);
}

void header_add_bases(const char *str, size_t col, int hfd) {
	// above
	// Add new translation identity to header file
	size_t length = strlen(str);
	char buffer[length];
	buffer[latincpy_and_make_them_uppercase(buffer, str, length)] = '\0';
	dprintf(hfd, "#define "HEADER_DEFINE_PREFIX_BASES"%s %zu\n", buffer, col);
}

// below
// data structure which is just 8 bytes long
struct eightbytes {
	char zb[8];
};

void ssb_add_header(ssb_t *ssbo) {
	// above
	// Add header which is idetifier for particular SSB file type. In this case it's SSB with translations.
	// Contains format signature + 8 bytes, which is made for storing additional metadata
	// Current version (SSBTRANSLATIONS_1) contains two unsigned 32bit integers, which is made for storing
	// amount of rows and cols.
	SSB_BUILD_BUG_ON(sizeof(struct eightbytes) != 8);
	char header_buffer[strizeof(ssb_header_itself) + sizeof(struct eightbytes)];
	memcpy(header_buffer, ssb_header_itself, strizeof(ssb_header_itself));
	memset(header_buffer + strizeof(ssb_header_itself), '\0', sizeof(struct eightbytes));
	write(ssbo->fd, header_buffer, sizeof(header_buffer));
	ssbo->metalocation = strizeof(ssb_header_itself);
}

void ssb_add(const char *string, ssb_t *ssbo, bool is_newline) {
	// above
	// Add new record for SSB
	if (is_newline) {
		uint8_t newstring_bytes[ssbo->max];
		memset(newstring_bytes, UCHAR_MAX, ssbo->max);
		write(ssbo->fd, newstring_bytes, ssbo->max);
	}
	size_t len = strlen(string);
	char lenbytes[ssbo->max];
	size_t cpyshift = 0;
	if (IS_BIG_ENDIAN) cpyshift = sizeof(size_t) - ssbo->max; // test it some1 please
	memcpy(lenbytes, (char *) &len + cpyshift, ssbo->max);
	if (IS_BIG_ENDIAN) swapbytes(lenbytes, ssbo->max); // be to le
	write(ssbo->fd, lenbytes, ssbo->max);
	write(ssbo->fd, string, len);
}

void ssb_and_h_add(const char *string, size_t row, size_t col, ssb_t *ssbo, int hfd) {
	if (col == 0) {
		header_add_lang(string, row, hfd);
		ssb_add(string, ssbo, true);
		return;
	}
	if (row == 0) {
		header_add_bases(string, col, hfd);
		ssb_add(string, ssbo, false);
		return;
	}
	ssb_add(string, ssbo, false);
}

void add_meta(int fd, size_t location, uint32_t row, uint32_t col) {
	if (IS_BIG_ENDIAN) swapbytes(&row, sizeof(col)), swapbytes(&col, sizeof(col)); // be to le
	pwrite(fd, &row, sizeof(row), location);
	pwrite(fd, &col, sizeof(col), location + sizeof(row));
}

int main(int argc, char **argv) {
	if (argc < 2) return fprintf(stderr, "Pass csv file as an command-line argument.\n"), EXIT_FAILURE;
	if (access(argv[1], R_OK) == POSIX_FAIL) return fprintf(stderr, "%s is not available: %s\n", argv[1], strerror(errno)), EXIT_FAILURE;

	int ssbfd, headerfd;
	if ((ssbfd = check_rfile_prepare_wfilefd(argv[1], ext_string)) == POSIX_FAIL) return EXIT_FAILURE;
	if ((headerfd = check_rfile_prepare_wfilefd(argv[1], "h")) == POSIX_FAIL) return close(ssbfd), EXIT_FAILURE;
	CsvParser *csvparser = CsvParser_new(argv[1], ",", 0);
	CsvRow *row;
	ssb_t ssbo = {.fd = ssbfd, .max = sizeof(uint16_t)};
	ssb_add_header(&ssbo);
	uint32_t colcount = 0;
	uint32_t rowcount = 0;
	while ((row = CsvParser_getRow(csvparser)) ) {
		// newline
		char **rowFields = CsvParser_getFields(row);
		for (colcount = 0 ; colcount < CsvParser_getNumFields(row) ; colcount++) {
			ssb_and_h_add(rowFields[colcount], rowcount, colcount, &ssbo, headerfd);
			//printf("FIELD: %s ", rowFields[colcount]);
		}
		rowcount++;
		CsvParser_destroy_row(row);
	}
	CsvParser_destroy(csvparser);
	add_meta(ssbfd, ssbo.metalocation, rowcount, colcount);
	close(ssbfd);
	close(headerfd);
	return EXIT_SUCCESS;
}
