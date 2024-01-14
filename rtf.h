/**
 * File              : rtf.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 04.12.2023
 * Last Modified Date: 14.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

/* functions to handle RTF files */

#ifndef RTF_H_
#define RTF_H_
#include <stdio.h>

/* rtf_from_utf8
 * return string with rtf code from utf8 multibite 
 * sting or NULL on error
 */
static char *
rtf_from_utf8(const char *s);

/* rtf_table_header
 * return string with rtf code of table header
 * or NULL on error
 * %coln   - number of columns
 * %titles - array of columns titles
 * %width  - array of columns width
 */
static char *
rtf_table_header(int coln, const char *titles[], int *width);

/* rtf_table_row
 * return string with rtf code of table row
 * or NULL on error
 * %coln  - number of columns
 * %colv  - columns values
 */
static char *
rtf_table_row(int coln, char *colv[]);

/* rtf_table_row_from_string
 * return string with rtf code of table row
 * or NULL on error
 * %colv  - string with columns values separeted by delim
 * %delim - string with delim chars
 */
static char *
rtf_table_row_from_string(
		const char *colv, const char *delim);


/* rtf_image_to_rtf
 * return size of allocated string with rtf code with image
 * or 0 on error
 * %jpeg_data  - image data in JPEG
 * %size       - size of image data
 * %rtf        - pointer to string with rtf code
 */
static size_t rtf_image_to_rtf(
		void *jpeg_data, size_t size, char **rtf);

	
/* IMPLIMATION */
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

char *
rtf_from_utf8(const char *s)
{
	if (!s)
		return NULL;
	
	size_t len = strlen(s);
	char *out = (char *)malloc(len * 6 + 1);
	if (!out)
		return NULL;
	strcpy(out, "");

	char *ptr = (char *)s;
	uint32_t c32;
	while(*ptr){
		// get char
		uint8_t c = *ptr;
		if (c >= 252){/* 6-bytes */
			c32  = (*ptr++ & 0x1)  << 30;  // 0b00000001
			c32 |= (*ptr++ & 0x3F) << 24;  // 0b00111111
			c32 |= (*ptr++ & 0x3F) << 18;  // 0b00111111
			c32 |= (*ptr++ & 0x3F) << 12;  // 0b00111111
			c32 |= (*ptr++ & 0x3F) << 6;   // 0b00111111
			c32 |=  *ptr++ & 0x3F;         // 0b00111111
			sprintf(out, "%s\\u%d ", out, c32);
		}	
		else if (c >= 248){/* 5-bytes */
			c32  = (*ptr++ & 0x3)  << 24;  // 0b00000011
			c32 |= (*ptr++ & 0x3F) << 18;  // 0b00111111
			c32 |= (*ptr++ & 0x3F) << 12;  // 0b00111111
			c32 |= (*ptr++ & 0x3F) << 6;   // 0b00111111
			c32 |=  *ptr++ & 0x3F;         // 0b00111111
			sprintf(out, "%s\\u%d ", out, c32);
		}
		else if (c >= 240){/* 4-bytes */
			c32  = (*ptr++ & 0x7)  << 18;  // 0b00000111
			c32 |= (*ptr++ & 0x3F) << 12;  // 0b00111111
			c32 |= (*ptr++ & 0x3F) << 6;   // 0b00111111
			c32 |=  *ptr++ & 0x3F ;        // 0b00111111
			sprintf(out, "%s\\u%d ", out, c32);
		}
		else if (c >= 224){/* 3-bytes */
			c32  = (*ptr++ & 0xF)  << 12;  // 0b00001111
			c32 |= (*ptr++ & 0x3F) << 6;   // 0b00111111
			c32 |=  *ptr++ & 0x3F;         // 0b00111111
			sprintf(out, "%s\\u%d ", out, c32);
		}
		else if (c >= 192){/* 2-bytes */
			c32  = (*ptr++ & 0x1F) << 6;   // 0b00011111
			c32 |=  *ptr++ & 0x3F;         // 0b00111111
			sprintf(out, "%s\\u%d ", out, c32);
		}
		else{/* 1-byte */
			sprintf(out, "%s%c", out, *ptr++);
		}
	}
	strcat(out, " ");
	return out;
}

struct _rtf_str{
	char *str;   //null-terminated c string
	int   len;   //length of string (without last null char)
	int   size;  //allocated size
};

static int 
_rtf_str_init(struct _rtf_str *s, size_t size)
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

static int _rtf_str_realloc(
		struct _rtf_str *s, int new_size)
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

static void _rtf_str_append(
		struct _rtf_str *s, const char *str, int len)
{
	if (!str || len < 1)
		return;

	int new_size, i;
	
	new_size = s->len + len + 1;
	// realloc if not enough size
	if (_rtf_str_realloc(s, new_size))
		return;

	// append string
	for (i = 0; i < len; ++i)
		s->str[s->len++] = str[i];
  
	s->str[s->len] = 0;
}

#define _rtf_str_appendf(s, ...)\
	({char str[BUFSIZ];sprintf(str, __VA_ARGS__);\
			_rtf_str_append(s, str, strlen(str));});

char *
rtf_table_header(
		int coln, const char *titles[], int *width)
{
	int i, w=0;
	struct _rtf_str s;
	_rtf_str_init(&s, BUFSIZ);
	
	_rtf_str_appendf(&s, 
			  "\\pard\\par\\trowd\n");
	for (i = 0; i < coln; ++i) {
		w += width[i];
		_rtf_str_appendf(&s, 
				"\\clbrdrt\\brdrs"
				"\\clbrdrl\\brdrs"
				"\\clbrdrb\\brdrs"
				"\\clbrdrr\\brdrs\n"
				"\\cellx%d\n", 
				w);
	}
	for (i = 0; i < coln; ++i) {
		char *title = rtf_from_utf8(titles[i]);
		_rtf_str_appendf(&s, 
				"\\intbl %s \\cell\n", 
				title);
		free(title);
	}

	return s.str;
}

char *
rtf_table_row(
		int coln, char *colv[])
{
	int i;
	struct _rtf_str s;
	if (_rtf_str_init(&s, BUFSIZ))
		return NULL;

	_rtf_str_appendf(&s, 
				"\\trowd\n");
	
	for (i = 0; i < coln; ++i)
		_rtf_str_appendf(&s, 
				"\\intbl %s \\cell\n",
				rtf_from_utf8(colv[i]));
	
	_rtf_str_appendf(&s, 
				"\\row\n");
	
	return s.str;
}

char *
rtf_table_row_from_string(
		const char *colv, const char *delim)
{
	int i;
	struct _rtf_str s;
	if (_rtf_str_init(&s, BUFSIZ))
		return NULL;

	_rtf_str_appendf(&s, 
				"\\trowd\n");
	
	// do safe strtok
	char *str = strdup(colv);
	if (str == NULL)
		return NULL;

	// loop through the string to extract 
	// tokens
	char *t;
	for(t=strtok(str, delim), i=0; 
			t; 
			t=strtok(NULL, delim), ++i) 
		_rtf_str_appendf(&s, 
				"\\intbl %s \\cell\n",
				rtf_from_utf8(t));
	
	_rtf_str_appendf(&s, 
				"\\row\n");
	
	free(str);
	return s.str;
}

static unsigned char * _rtf_bin_to_strhex(
		const unsigned char *bin,
		unsigned int binsz,
		unsigned char **result)
{
	unsigned char hex_str[] = "0123456789abcdef";
	unsigned int  i;

	if (!binsz)
		return NULL;
	
	if (!(*result = 
				(unsigned char *)malloc(binsz * 2 + 1)))
		return NULL;

	(*result)[binsz * 2] = 0;

	for (i = 0; i < binsz; ++i)
	{
		(*result)[i * 2 + 0] = hex_str[(bin[i] >> 4) & 0x0F];
		(*result)[i * 2 + 1] = hex_str[(bin[i]     ) & 0x0F];
	}

	return (*result);
}

/* convert image to RTF string */
size_t rtf_image_to_rtf(
		void *jpeg_data, size_t size, char **rtf)
{
	struct _rtf_str s;
	if (_rtf_str_init(&s, size * 2 + BUFSIZ))
		return 0;

	// append image header to rtf
	_rtf_str_appendf(&s, 
			"{\\pict\\picw0\\pich0\\picwgoal10254"
			"\\pichgoal6000\\jpegblip\n");
	
	// append image data to rtf
	unsigned char *str;
	_rtf_bin_to_strhex(
		(unsigned char*)(jpeg_data),
		size, &str);
	_rtf_str_append(
			&s, (char*)str, size*2);
	free(str);
	
	// append image close to rtf
	_rtf_str_appendf(&s, "}\n");

	if (rtf)
		*rtf = s.str;
	else
		free(s.str);
	
	return s.len;
}

#endif /* ifndef RTF_H_ */
