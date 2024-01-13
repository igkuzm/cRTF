/**
 * File              : cRTF.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 13.01.2024
 * Last Modified Date: 14.01.2024
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

#define CALLBACK(c, r, ...)\
{\
	if (c)\
		if(c(__VA_ARGS__))\
			return r;\
}

/*! \enum STEP
 *
 *  Detailed description
 */
enum step { 
	ASCII,
	CONTROL_WORD,
	GROUP_OPEN,
	GROUP_CLOSE
};

#define STR(...)\
({char s[BUFSIZ]; sprintf(s, __VA_ARGS__); s;})

static int is_style(char *buf)
{
	if (
			buf[0] == '\\' && 
			buf[1] == 's' && 
			is_digit(buf[2]));
		return 1;
	return 0;
}

static int is_utf(char *buf)
{
	if (
			buf[0] == '\\' && 
			buf[1] == 'u'  &&
			is_digit(buf[1])
			)
		return 1;
	return 0;
}

static int is_li(char *buf)
{
	if (
		buf[0] == '\\' &&
		buf[1] == 'l' &&
		buf[2] == 'i' &&
		is_digit(buf[3])
		)
		return 1;
	return 0;
}

static int is_fs(char *buf)
{
	if (
		buf[0] == '\\' &&
		buf[1] == 'f' &&
		buf[2] == 's' &&
		is_digit(buf[3])
		)
		return 1;
	return 0;
}

static int is_list(char *buf)
{
	if (
		buf[0] == '\\' &&
		buf[1] == 'l' &&
		buf[2] == 's' &&
		is_digit(buf[3])
		)
		return 1;
	return 0;
}

static int is_colwidth(char *buf)
{
	if (
		buf[0] == '\\' &&
		buf[1] == 'c' &&
		buf[2] == 'e' &&
		buf[3] == 'l' &&
		buf[4] == 'l' &&
		buf[5] == 'x' &&
		is_digit(buf[6])
		)
		return 1;
	return 0;
}

struct style {
	int n;
	char value[64];
	char name[16];
};

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
	FILE *fp;
	int ch;
	int level;
	struct style *styles;
	int nstyles;
};

void c_rtf_parser_init(struct c_rtf_parser *p)
{
	memset(p, 0, sizeof(struct c_rtf_parser));
}

void parse_char(struct c_rtf_parser *p, char *buf, int len);

static enum step
parse_step(
		struct c_rtf_parser *p, 
		char *buf, int *len)
{
	switch (p->ch) {
		case '{':
			p->level++;
			return GROUP_OPEN;
		case '}':
			//close group
			p->level--;
			return GROUP_CLOSE;
		case '\\':
			// try to get control_word
			{
				p->ch = read_control_word(
						p->ch, p->fp, buf, len);
				if (is_control_word(buf)){
					return CONTROL_WORD;
				} else {
					// check braces
					if (p->ch == '{' || p->ch == '}'){
						// handle them as anscii text
						return ASCII;
					} else if(p->ch == '\'') {
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
			return ASCII;
			break;
	}
	p->ch = fgetc(p->fp);
	return ASCII;
}

static int 
parse_styles(
		struct c_rtf_parser *p, char *word)
{
	// table of styles
	if (strcmp(word, "stylesheet") == 0)
	{
		int level = p->level;
		char buf[BUFSIZ];
		int len = 0;

		// get styles
		int style_level = 0;
		while (level >= p->level) {
			enum step step = 
				parse_step(p, buf, &len);
			if (step == GROUP_OPEN){
				// add new style
				style_level++;
			}
			if (step == CONTROL_WORD && style_level == 1){
				if (is_style(buf)){
					//this is style number
					char *s = buf + 1;
					p->styles[p->nstyles].n = atoi(s);
				} 

				else if (strcmp(buf, "\\qc") == 0)
					strcat(
						p->styles[p->nstyles].name, "CENTER");
				
				else if (strcmp(buf, "\\qr") == 0)
					strcat(
						p->styles[p->nstyles].name, "RIGHT");
				
				else if (strcmp(buf, "\\ql") == 0)
					strcat(
						p->styles[p->nstyles].name, "LEFT");
				
				if (is_li(buf))
					strcat(
						p->styles[p->nstyles].name, "Q");
				
				if (is_list(buf))
					strcat(
						p->styles[p->nstyles].name, "LN");
				
				// this is style properties
				strcat(
						p->styles[p->nstyles].value, buf);
			}
			if (step == ASCII && style_level == 1){
				// this is style description
				strcat(
						p->styles[p->nstyles].value, buf);
			}
			if (step == GROUP_OPEN){
				style_level--;
				if (style_level == 0){
					// iterate style
					p->nstyles++;
				}
			}
		}

		return 1;
	}
	return 0;
}

static int 
parse_text(
		struct c_rtf_parser *p, char *buf, enum step step)
{
	if (step == CONTROL_WORD){
		if (is_utf(buf)){
			// hande utf text
			char unicode[5] = 
			{
				buf[2], 
				buf[3], 
				buf[4], 
				buf[5], 
				0
			};

			uint32_t u;
			sscanf(unicode, "%u", &u);			

			char s[5];
			int l = c32tomb(s, u);
			s[l] = 0;
			CALLBACK(p->text, 1, p->userdata, s, l);
			return 1;
		}
		
		if (strcmp(buf, "\\b") == 0){
			CALLBACK((p->bold_start), 1, p->userdata);
			return 1;
		}
		
		if (strcmp(buf, "\\b0") == 0){
			CALLBACK((p->bold_end), 1, p->userdata);
			return 1;
		}
		
		if (strcmp(buf, "\\i") == 0){
			CALLBACK((p->italic_start), 1, p->userdata);
			return 1;
		}
		
		if (strcmp(buf, "\\i0") == 0){
			CALLBACK((p->italic_end), 1, p->userdata);
			return 1;
		}
		
		if (strcmp(buf, "\\ul") == 0){
			CALLBACK((p->underline_start), 1, p->userdata);
			return 1;
		}
		
		if (strcmp(buf, "\\ul0") == 0){
			CALLBACK((p->underline_end), 1, p->userdata);
			return 1;
		}
	}

	if (step == ASCII){
		// print text
		char s[2];
		s[0] = p->ch;
		s[1] = 0;
		CALLBACK(p->text, 1, p->userdata, s, 1);
		return 1;
	}
	
	return 0;
}

static int 
parse_image(
		struct c_rtf_parser *p, char *word)
{
	if (
			strcmp(word, "\\pict") == 0 || 
			strcmp(word, "\\shppict") == 0 
			)
	{
		int inpicture = 0;

		if (strcmp(word, "\\shppict") == 0){
			// find pict
			int level = p->level;
			char buf[BUFSIZ];
			int len = 0;

			while (level >= p->level) {
				enum step step = 
					parse_step(p, buf, &len);
				
				if (step == CONTROL_WORD){
					if (strcmp(buf, "\\pict") == 0){
						inpicture = 1;
						break;
					}
				}
			}
		} 

		if (!inpicture)
			return 1;
		
		// this is picture
		// data will start after space or newline
		int level = p->level;
		char buf[BUFSIZ];
		int len = 0;
		while (level >= p->level) {
			enum step step = 
				parse_step(p, buf, &len);
				
			if (step == ASCII)
				if (level == p->level)
					if (is_hex(p->ch))
						break;
		}
		
		if (!is_hex(p->ch))
			return 1;
				
		// get image data until '}'
		struct str img;
		str_init(&img, BUFSIZ);
		p->ch = fgetc(p->fp);
		while (p->ch != '}' && p->ch != EOF) {
			char c = p->ch;
			str_append(&img, &c, 1);
			p->ch = fgetc(p->fp);
		}
		
		if (p->ch == EOF)
			return 1;

		// convert image hex string to binary
		size_t size = img.len/2;
		unsigned char *data = 
			(unsigned char*)malloc(size);
		if (!data) // not enough memory
			return 1;

		char cur[3];
		unsigned int val;
		size_t i, l;
		for (i = 0, l = 0; i < img.len;) {
			cur[0] = img.str[i++];
			cur[1] = img.str[i++];
			cur[2] = 0;
			sscanf(cur, "%x", &val);
			data[l++] = (unsigned char)val;
		}

		// callback image data
		CALLBACK(p->image, 1, p->userdata, data, size);

		// free image data and string
		free(data);
		free(img.str);

		// parse char in RTF
		parse_char(p, buf, len);
	}

	return 0;
}

static int 
parse_table(
		struct c_rtf_parser *p, char *word)
{
	if (strcmp(word, "\\trowd") == 0)
	{
		CALLBACK((p->table_start), 1, p->userdata);
		
		int level = p->level;
		char buf[BUFSIZ];
		int len = 0;
		
		int istable = 1;
		int row = 0;
		int cell = 0;

		int colwidth[512];
		int ncolwidth = 0;
					
		CALLBACK((p->tablerow_start), 1, p->userdata, row);

		while (level >= p->level) {
			enum step step = 
				parse_step(p, buf, &len);
			if (step == CONTROL_WORD){
				if (strcmp(buf, "\\lastrow") == 0){
					CALLBACK((p->table_end), 1, p->userdata);
					break;
				}
				if (strcmp(buf, "\\row") == 0){
					CALLBACK((p->tablerow_end), 1, p->userdata, row);
					row++;
					cell = 0;
					continue;
				}
				if (strcmp(buf, "\\intbl") == 0){
					CALLBACK((p->tablecell_start), 1, p->userdata, cell);
					continue;
				}
				if (strcmp(buf, "\\cell") == 0){
					CALLBACK((p->tablecell_end), 1, p->userdata, cell);
					cell++;
					continue;
				}
				if (is_colwidth(buf)){
					char *s = buf + 6;
					colwidth[ncolwidth] = atoi(s);
					CALLBACK((p->tablerow_width), 1, 
							p->userdata, ncolwidth, colwidth[ncolwidth]);
					ncolwidth++;
					continue;
				}
			}
			if (parse_text(p, buf, step))
				continue;
		}
	}

	return 0;
}

static int 
parse_paragraph(
		struct c_rtf_parser *p, char *word)
{
	if (strcmp(word, "\\pard") == 0)
	{
		CALLBACK((p->paragraph_start), 1, p->userdata);
		
		int level = p->level;
		char buf[BUFSIZ];
		int len = 0;

		int par_level = 0;
		while (level >= p->level) {
			enum step step = 
				parse_step(p, buf, &len);
			
			if (parse_text(p, buf, step))
				continue;
		
			if (step == CONTROL_WORD){

				if (strcmp(buf, "\\qc") == 0){
					CALLBACK((p->style), 1, p->userdata, "CENTER");
					continue;
				}
				
				if (strcmp(buf, "\\ql") == 0){
					CALLBACK((p->style), 1, p->userdata, "LEFT");
					continue;
				}
				
				if (strcmp(buf, "\\qr") == 0){
					CALLBACK((p->style), 1, p->userdata, "RIGTH");
					continue;
				}
				
				if (is_li(buf)){
					CALLBACK((p->style), 1, p->userdata, "Q");
					continue;
				}
				
				if (is_list(buf)){
					CALLBACK((p->style), 1, p->userdata, "LN");
					continue;
				}

				if (parse_image(p, buf))
					continue;
				
				if (parse_table(p, buf))
					continue;
				
				if (is_style(buf)){
					// handle with slyles
					char *s = buf + 1;
					int n = atoi(s);
					int i;
					for (i = 0; i < p->nstyles; ++i) {
						if (n == p->styles[i].n){
							CALLBACK((p->style), 1, 
									p->userdata, p->styles[i].name);
							break;
						}	
					}
					continue;
				}
				
				if (strcmp(buf, "\\par") == 0){
					// the end of paragraph
					CALLBACK((p->paragraph_end), 1, p->userdata);
					break;
				}
			}
		}

		return 1;
	}
	return 0;
}

static int 
control_word_parser(
		struct c_rtf_parser *p, char *buf, int len)
{
	if (parse_styles(p, buf))
		return 0;
	
	if (parse_paragraph(p, buf))
		return 0;
	
	if (parse_image(p, buf))
		return 0;
	
	if (parse_table(p, buf))
		return 0;
	
	return 0;
}

void parse_char(struct c_rtf_parser *p, char *buf, int len){
		enum step step = 
			parse_step(p, buf, &len);

		if (step == CONTROL_WORD) {
			// handle with control word
			control_word_parser(p, buf, len);
			return;
		}
		
		if (step == ASCII && p->level == 1){
			// print chars
			return;
		}
}

int c_rtf_parse_file(FILE *fp, struct c_rtf_parser *p)
{
	int ch = fgetc(fp);
	char buf[BUFSIZ];
	int len = 0;

	p->fp = fp;

	// read each char in text file
	while (ch != EOF)
	{
		parse_char(p, buf, len);
	}
	return 0;
}

int c_rtf_parse(
		const char *filename, 
		struct c_rtf_parser *parser)
{
	FILE *fp = fopen(filename, "r");
	if (!fp)
		perror(STR("fopen: %s", filename));

	c_rtf_parse_file(fp, parser); 

	return 0;
}
