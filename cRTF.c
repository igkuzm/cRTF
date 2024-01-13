/**
 * File              : cRTF.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 13.01.2024
 * Last Modified Date: 13.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */
/**
 * cRTF.c
 * Copyright (c) 2024 Igor V. Sementsov <ig.kuzm@gmail.com>
 *
 * This program is free software: you can redistribute it 
 * and/or modify it under the terms of the GNU Affero 
 * General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE.  See the GNU Affero General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Affero General
 * Public License along with this program.  If not, see 
 * <https://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* convert utf32 char to utf8 multybite char array and
 * return number of bytes */ 
static int c32tomb(char s[6], const uint32_t c32){
	int i = 0;
	if (c32 <= 0x7F) {
		// Plain single-byte ASCII.
		s[i++] = (char) c32;
	}
	else if (c32 <= 0x7FF) {
		// Two bytes.
		s[i++] = 0xC0 |  (c32 >> 6);
		s[i++] = 0x80 | ((c32 >> 0) & 0x3F);
	}
	else if (c32 <= 0xFFFF) {
		// Three bytes.
		s[i++] = 0xE0 |  (c32 >> 12);
		s[i++] = 0x80 | ((c32 >> 6) & 0x3F);
		s[i++] = 0x80 | ((c32 >> 0) & 0x3F);
	}
	else if (c32 <= 0x1FFFFF) {
		// Four bytes.
		s[i++] = 0xF0 |  (c32 >> 18);
		s[i++] = 0x80 | ((c32 >> 12) & 0x3F);
		s[i++] = 0x80 | ((c32 >> 6)  & 0x3F);
		s[i++] = 0x80 | ((c32 >> 0)  & 0x3F);
	}
	else if (c32 <= 0x3FFFFFF) {
		// Five bytes.
		s[i++] = 0xF8 |  (c32 >> 24);
		s[i++] = 0x80 | ((c32 >> 18) & 0x3F);
		s[i++] = 0x80 | ((c32 >> 12) & 0x3F);
		s[i++] = 0x80 | ((c32 >> 6)  & 0x3F);
		s[i++] = 0x80 | ((c32 >> 0)  & 0x3F);
	}
	else if (c32 <= 0x7FFFFFFF) {
		// Six bytes.
		s[i++] = 0xFC |  (c32 >> 30);
		s[i++] = 0x80 | ((c32 >> 24) & 0x3F);
		s[i++] = 0x80 | ((c32 >> 18) & 0x3F);
		s[i++] = 0x80 | ((c32 >> 12) & 0x3F);
		s[i++] = 0x80 | ((c32 >> 6)  & 0x3F);
		s[i++] = 0x80 | ((c32 >> 0)  & 0x3F);
	}
	else{
		// Invalid char; don't encode anything.
	}	

	return i;
}

/* convert utf8 multybite null-terminated string to utf32 
 * null-terminated string and return it's len */ 
static size_t mbtoc32(uint32_t *s32, const char *s){
	char *ptr = (char *)s;
	size_t i = 0;
	while (*ptr){
		uint8_t c = *ptr;
		if (c >= 252){/* 6-bytes */
			s32[i]  = (*ptr++ & 0x1)  << 30;  // 0b00000001
			s32[i] |= (*ptr++ & 0x3F) << 24;  // 0b00111111	
			s32[i] |= (*ptr++ & 0x3F) << 18;  // 0b00111111
			s32[i] |= (*ptr++ & 0x3F) << 12;  // 0b00111111
			s32[i] |= (*ptr++ & 0x3F) << 6;   // 0b00111111
			s32[i] |=  *ptr++ & 0x3F;         // 0b00111111
			i++;
		} 
		else if (c >= 248){/* 5-bytes */
			s32[i]  = (*ptr++ & 0x3)  << 24;  // 0b00000011
			s32[i] |= (*ptr++ & 0x3F) << 18;  // 0b00111111
			s32[i] |= (*ptr++ & 0x3F) << 12;  // 0b00111111
			s32[i] |= (*ptr++ & 0x3F) << 6;   // 0b00111111
			s32[i] |=  *ptr++ & 0x3F;         // 0b00111111
			i++;
		}
		else if (c >= 240){/* 4-bytes */
			s32[i]  = (*ptr++ & 0x7)  << 18;  // 0b00000111
			s32[i] |= (*ptr++ & 0x3F) << 12;  // 0b00111111
			s32[i] |= (*ptr++ & 0x3F) << 6;   // 0b00111111
			s32[i] |=  *ptr++ & 0x3F;         // 0b00111111
			i++;
		} 
		else if (c >= 224){/* 3-bytes */
			s32[i]  = (*ptr++ & 0xF)  << 12;  // 0b00001111
			s32[i] |= (*ptr++ & 0x3F) << 6;   // 0b00111111
			s32[i] |=  *ptr++ & 0x3F;         // 0b00111111
			i++;                
		}
		else if (c >= 192){/* 2-bytes */
			s32[i]  = (*ptr++ & 0x1F) << 6;   // 0b00011111
			s32[i] |=  *ptr++ & 0x3F;         // 0b00111111 
			i++; 
		} 
		else{/* 1-byte */
			s32[i++] = *ptr++;
		} 
	}

	// null-terminate string
	s32[i] = 0;
	return i;
}	

/* dynamic string structure */
struct str {
	char *str;   //null-terminated c string
	int   len;   //length of string (without last null char)
	int   size;  //allocated size
};

int str_init(struct str *s, size_t size)
{
	// allocate data
	s->str = (char*)malloc(size);
	if (!s->str)
		return -1;

	// set dafaults
	s->str[0]  = 0;
	s->len     = 0;
	s->size    = size;

	return 0;
}

static int _str_realloc(
		struct str *s, int new_size)
{
	while (s->size < new_size){
		// do realloc
		void *p = realloc(s->str, s->size + BUFSIZ);
		if (!p)
			return -1;
		s->str = (char*)p;
		s->size += BUFSIZ;
	}
	return 0;
}

void str_append(
		struct str *s, const char *str, int len)
{
	if (!str)
		return;

	int new_size, i;
	
	new_size = s->len + len + 1;
	// realloc if not enough size
	if (_str_realloc(s, new_size))
		return;

	// append string
	for (i = 0; i < len; ++i)
		s->str[s->len++] = str[i];
  
	s->str[s->len] = 0;
}

#define str_appendf(s, ...)\
	({\
	 char str[BUFSIZ];\
	 snprintf(str, BUFSIZ-1, __VA_ARGS__);\
	 str[BUFSIZ-1] = 0;\
	 str_append(s, str, strlen(str));\
	 })

static int is_digit(int ch)
{
	if (ch >= '0' && ch <= '9')
		return 1;
	return 0;
}

static int is_low_letter(int ch)
{
	if (ch >= 'a' && ch <= 'z')
		return 1;
	return 0;
}

static int is_up_letter(int ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return 1;
	return 0;
}

static int is_letter(int ch)
{
	if (is_low_letter(ch) || is_up_letter(ch))
		return 1;
	return 0;
}

static int is_hex(int ch)
{
	if (
			ch == 'A' || ch == 'a' ||
			ch == 'B' || ch == 'b' ||
			ch == 'C' || ch == 'c' ||
			ch == 'D' || ch == 'd' ||
			ch == 'F' || ch == 'f' ||
			is_digit(ch)
			)
		return 1;
	return 0;
}

static int
is_control_word_terminated(int ch)
{
	if (
			is_low_letter(ch) ||
			is_digit(ch)      || 
			ch == ' '         || // space as  part of word  
			ch == '-'         
			)
		return 0;
	return 1;
}


static int is_control_word(char *buf)
{
	if (buf[0] == '\\')
		if(
				is_letter(buf[1]) || 
				buf[1] == '*'
			)
			return 1;
	return 0;
}

static int
read_control_word(int ch, FILE *fp, char *buf, int *len)
{
	if (ch != '\\')
		return ch;
	
	int i = 0;
	buf[i++] = ch;
	while (1)
	{
		ch = fgetc(fp);
		if (ch == EOF)
			break;
		if (!is_control_word_terminated(ch))
			buf[i++] = ch;
		else if (ch == ' '){ // space is part of control word
												 // read next char to return it
			ch = fgetc(fp);
			break;
		}
		else
			break;;
	}
	buf[i] = 0;
	if (len)
		*len = i;

	return ch;
}

#define CALLBACK(c, ...)\
{\
	if (c)\
		if(c(__VA_ARGS__))\
			return 0;\
}

static int 
c_rtf_parse_file(
		FILE *fp, void *userdata, 
		int(*open_group)(void *userdata, int *ch),
		int(*close_group)(void *userdata, int *ch),
		int(*control_word)(
			void *userdata, 
			int *ch, 
			char *control_word, 
			int len),
		int(*anscii)(void *userdata, int ch))
{
	int ch = fgetc(fp);
	char buf[BUFSIZ];

	// read each char in text file
	while (ch != EOF)
	{
		switch (ch) {
			case '{':
				//open group
				CALLBACK(open_group, userdata, &ch)
				break;
			case '}':
				//close group
				CALLBACK(close_group, userdata, &ch)
				break;
			case '\\':
				// try to get control_word
				{
					int l;
					ch = read_control_word(ch, fp, buf, &l);
					if (is_control_word(buf)){
						CALLBACK(control_word, userdata, &ch, buf, l)
					} else {
						// check braces
						if (ch == '{' || ch == '}'){
							// handle them as anscii text
							CALLBACK(anscii, userdata, ch)
						} else if(ch == '\'') {
							// this is 8-bit coded char
							/* TODO: handle with 8-bit codepages */
						} else {
							/* TODO: handle other variants */
						}
					}
				}
				break;

			default:
				// handle as anscii
				CALLBACK(anscii, userdata, ch)
				break;
		}
	}

	ch = fgetc(fp);
	return 0;
}

#define STR(...)

struct c_rtf_parser {
	void *userdata;
	int (*paragraph_start)(void *userdata);
	int (*paragraph_end)(void *userdata);
	int (*bold_start)(void *userdata);
	int (*bold_end)(void *userdata);
	int (*italic_start)(void *userdata);
	int (*italic_end)(void *userdata);
	int (*underline_start)(void *userdata);
	int (*underline_end)(void *userdata);
	int (*table_start)(void *userdata);
	int (*table_end)(void *userdata);
	int (*tablerow_width)(void *userdata, int i, int w);
	int (*tablerow_start)(void *userdata, int n);
	int (*tablerow_end)(void *userdata, int n);
	int (*tablecell_start)(void *userdata, int n);
	int (*tablecell_end)(void *userdata, int n);
	int (*style)(void *userdata, const char *style);
	int (*text)(void *userdata, const char *text, int len);
	int (*image)(void *userdata, const unsigned char *data, size_t len);
};

int c_rtf_parse(
		const char *filename, 
		struct c_rtf_parser *parser)
{
	FILE *fp = fopen(filename, "r");
	if (!fp)
		perror(STR("fopen: %s", filename));

	return 0;
}
