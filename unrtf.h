/**
 * File              : unrtf.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 12.01.2024
 * Last Modified Date: 14.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */
#ifndef UNRTF_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* parse RTF file and run callbacks */
static int unrtf(
		const char *filename,
		void *userdata,
		int (*paragraph_start)(void *userdata),
		int (*paragraph_end)(void *userdata),
		int (*bold_start)(void *userdata),
		int (*bold_end)(void *userdata),
		int (*italic_start)(void *userdata),
		int (*italic_end)(void *userdata),
		int (*underline_start)(void *userdata),
		int (*underline_end)(void *userdata),
		int (*table_start)(void *userdata),
		int (*table_end)(void *userdata),
		int (*tablerow_width)(void *userdata, int i, int w),
		int (*tablerow_start)(void *userdata, int n),
		int (*tablerow_end)(void *userdata, int n),
		int (*tablecell_start)(void *userdata, int n),
		int (*tablecell_end)(void *userdata, int n),
		int (*style)(void *userdata, const char *style),
		int (*text)(void *userdata, const char *text, int len),
		int (*image)(void *userdata, const unsigned char *data, size_t len)
		);

/*
 *
 * IMPLIMATION
 *
 *
 */

/* dynamic string structure */
struct _unrtf_str {
	char *str;   //null-terminated c string
	int   len;   //length of string (without last null char)
	int   size;  //allocated size
};

static int 
_unrtf_str_init(struct _unrtf_str *s, size_t size)
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

static int _unrtf_str_realloc(
		struct _unrtf_str *s, int new_size)
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

static void _unrtf_str_append(
		struct _unrtf_str *s, const char *str, int len)
{
	if (!str || len < 1)
		return;

	int new_size, i;
	
	new_size = s->len + len + 1;
	// realloc if not enough size
	if (_unrtf_str_realloc(s, new_size))
		return;

	// append string
	for (i = 0; i < len; ++i)
		s->str[s->len++] = str[i];
  
	s->str[s->len] = 0;
}

/* convert utf32 char to utf8 multybite char array and 
 * return number of bytes */ 
static int _unrtf_c32tomb(char s[6], const uint32_t c32){
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

static int _unrtf_isinword(int ch)
{
	if ( 	
		ch == '\n' ||
		ch == '\r' ||
		ch == ' '  ||
		ch == '\t' ||
		ch == '}'  ||
		ch == '{'  ||
		ch == '\\'
		)
		return 0;
	return 1;
}

static int _unrtf_readword(FILE *fp, char *buf)
{
	int ch = 1;
	int blen = 0;
	while (1)
	{
		ch = fgetc(fp);
		if (ch == EOF)
			break;
		if (_unrtf_isinword(ch))
			buf[blen++] = ch;
		else
			break;;
	}
	buf[blen] = 0;
	return ch;
}

static int _unrtf_iscontrol(char *buf)
{
	if (
			(buf[0] >= 'a' && buf[0] <= 'z')
		 )
		return 1;
	return 0;
}

static int _unrtf_isutf(char *buf)
{
	if (buf[0] == 'u')
		if (buf[1] >= '0' && buf[1] <= '9') 
			return 1;
	return 0;
}

static int _unrtf_isstyle(char *buf)
{
	if (buf[0] == 's')
		if (buf[1] >= '0' && buf[1] <= '9') 
			return 1;
	return 0;
}

static int _unrtf_isli(char *buf)
{
	if (
		buf[0] == 'l' &&
		buf[1] == 'i' &&
		(buf[2] >= '0' || buf[2] <= '9')
		)
		return 1;
	return 0;
}

static int _unrtf_isfs(char *buf)
{
	if (
		buf[0] == 'f' &&
		buf[1] == 's' &&
		(buf[2] >= '0' || buf[2] <= '9')
		)
		return 1;
	return 0;
}

static int _unrtf_islist(char *buf)
{
	if (
		buf[0] == 'l' &&
		buf[1] == 's' &&
		(buf[2] >= '0' || buf[2] <= '9')
		)
		return 1;
	return 0;
}

static int _unrtf_ishex(int ch)
{
	if (
		 ch == 'A' || ch == 'a' ||
		 ch == 'B' || ch == 'b' ||
		 ch == 'C' || ch == 'c' ||
		 ch == 'D' || ch == 'd' ||
		 ch == 'E' || ch == 'e' ||
		 ch == 'F' || ch == 'f' ||
		(ch >= '0' && ch <= '9')
		)
		return 1;
	return 0;
}

static int _unrtf_is8bit(char *buf)
{
	if (buf[0] == '\'')
		if (
			(buf[1] >= '0' && buf[1] <= '9') ||
			(buf[1] >= 'A' && buf[1] <= 'Z') ||
			(buf[1] >= 'a' && buf[1] <= 'z')
			)
		return 1;
	return 0;
}

static int _unrtf_iscolwidth(char *buf)
{
	if (
		buf[0] == 'c' &&
		buf[1] == 'e' &&
		buf[2] == 'l' &&
		buf[3] == 'l' &&
		buf[4] == 'x' &&
		(buf[5] >= '0' || buf[2] <= '9')
		)
		return 1;
	return 0;
}

static int _unrtf_isvalid(int ch)
{
	if ( 	
		ch != '{' &&
		ch != '}'
			)
		return 1;

	return 0;
}

static int _unrtf_flushstr(
		struct _unrtf_str *str,
		void *userdata,
		int (*text)(void *userdata, const char *text, int len))
{
	// flush str
	if (text)
		if (text(userdata, str->str, str->len))
			return 0;

	free(str->str);
	_unrtf_str_init(str, BUFSIZ);
	return 0;
}

struct _unrtf_style {
	int n;
	char name[32];
};

int unrtf(
		const char *filename,
		void *userdata,
		int (*paragraph_start)(void *userdata),
		int (*paragraph_end)(void *userdata),
		int (*bold_start)(void *userdata),
		int (*bold_end)(void *userdata),
		int (*italic_start)(void *userdata),
		int (*italic_end)(void *userdata),
		int (*underline_start)(void *userdata),
		int (*underline_end)(void *userdata),
		int (*table_start)(void *userdata),
		int (*table_end)(void *userdata),
		int (*tablerow_width)(void *userdata, int i, int w),
		int (*tablerow_start)(void *userdata, int n),
		int (*tablerow_end)(void *userdata, int n),
		int (*tablecell_start)(void *userdata, int n),
		int (*tablecell_end)(void *userdata, int n),
		int (*style)(void *userdata, const char *style),
		int (*text)(void *userdata, const char *text, int len),
		int (*image)(void *userdata, const unsigned char *data, size_t len)
		)
{
	// open input stream
	FILE *fp = fopen(filename, "r");
	if (!fp)
		return -1;

	struct _unrtf_style styles[100]; // styles array
	int nstyles = 0;                 // array len
	int current_style_number = 0;

	int colwidth[100],               // column width array
			ncolwidth = 0;               // array len
	
	int paragraph       = 0,
			stylesheet      = 0,
			stylesheetlevel = 0,
			level           = 0,         // level of braces
			istable         = 0,
			row             = 0,
			cell            = 0,
			pict            = 0,
			shppict         = 0,
			nonshppict      = 0;

	struct _unrtf_str str;
	_unrtf_str_init(&str, BUFSIZ);

	int ch = 1;
	while(ch != EOF)
	{
		ch = fgetc(fp);

unrtf_parse_start:
			
		if (ch == '{')
			level++;
		if (ch == '}'){
			level--;
			if (stylesheetlevel > level)
				stylesheet = 0;
		}

		if (ch == '\\'){ // starts with dash
			char buf[BUFSIZ];	
			ch = _unrtf_readword(fp, buf);

			// check if not service word
			if (!_unrtf_iscontrol(buf)){
				if (_unrtf_is8bit(buf)){
					// handle with codepages
					/* TODO: CODEPAGES */
				} else if (buf[0] == '*'){
					// \*. This control symbol identifies
					// destinations whose related text should be
					// ignored if the RTF reader does not recognize
					// the destination.
				} else if (
						paragraph  &&
						!pict      &&
						!shppict   &&
						!nonshppict
						)
				{
				// print it if in paragraph
						_unrtf_str_append(&str, buf,
							 	strlen(buf));
				}
				goto unrtf_parse_start;
			}

			// handle with service word
			if (strcmp(buf, "stylesheet") == 0){
				// table of styles
				stylesheet = 1;
				stylesheetlevel = level;
			}

/*******************************************************/
/* STYLES */	
/*******************************************************/
			if (_unrtf_isstyle(buf) && stylesheet){
				// handle with table of slyles
				char *s = buf + 1;
				current_style_number = atoi(s);
			}

			if (strcmp(buf, "qc") == 0 && stylesheet)
			{
				struct _unrtf_style style =
					{current_style_number, "CENTER"};
				styles[nstyles++] = style; 
			}
			
			if (strcmp(buf, "qr") == 0 && stylesheet)
			{
				struct _unrtf_style style =
					{current_style_number, "RIGHT"};
				styles[nstyles++] = style; 
			}
			
			if (strcmp(buf, "ql") == 0 && stylesheet)
			{
				struct _unrtf_style style =
					{current_style_number, "LEFT"};
				styles[nstyles++] = style; 
			}

			if (stylesheet && _unrtf_isli(buf))
			{
				struct _unrtf_style style =
					{current_style_number, "Q"};
				styles[nstyles++] = style; 
			}

			if (strcmp(buf, "qj") == 0 && stylesheet)
			{
				struct _unrtf_style style =
					{current_style_number, "P"};
				styles[nstyles++] = style; 
			}

			if (stylesheet && _unrtf_islist(buf))
			{
				struct _unrtf_style style =
					{current_style_number, "LN"};
				styles[nstyles++] = style; 
			}

			// set style in paragraph
			if (_unrtf_isstyle(buf) && paragraph){
				// handle with slyles
				char *s = buf + 1;
				int n = atoi(s);
				int i;
				for (i = 0; i < nstyles; ++i) {
					if (n == styles[i].n){
						if (style)
							if (style(userdata, styles[i].name))
								return 0;
						break;
					}	
				}
			}

/*******************************************************/
/* PARAGRAPH */	
/*******************************************************/
			if (strcmp(buf, "pard") == 0)
			{
				pict       = 0;
				shppict    = 0;
				nonshppict = 0;

				// new paragraph
				if (paragraph_start)
					if(paragraph_start(userdata))
						return 0;
				paragraph = 1;
				
				if (istable){
					ncolwidth = 0;
					if (table_end)
						if (table_end(userdata))
							return 0;
				}
			}
			if (strcmp(buf, "par") == 0)
			{
				// end paragraph
				if (!pict && !nonshppict && !shppict){
					_unrtf_flushstr(&str, userdata, text);
					if (paragraph_end)
						if(paragraph_end(userdata))
							return 0;
				}
				paragraph = 0;
			}

			// bold
			if (strcmp(buf, "b") == 0 && paragraph){
				_unrtf_flushstr(&str, userdata, text);
				if (bold_start)
					if (bold_start(userdata))
						return 0;
			}
			if (strcmp(buf, "b0") == 0 && paragraph){
				_unrtf_flushstr(&str, userdata, text);
				if (bold_end)
					if (bold_end(userdata))
						return 0;
			}

			// italic
			if (strcmp(buf, "i") == 0 && paragraph){
				_unrtf_flushstr(&str, userdata, text);
				if (italic_start)
					if (italic_start(userdata))
						return 0;
			}
			if (strcmp(buf, "i0") == 0 && paragraph){
				_unrtf_flushstr(&str, userdata, text);
				if (italic_end)
					if (italic_end(userdata))
						return 0;
			}
			
			// underline
			if (strcmp(buf, "ul") == 0 && paragraph){
				_unrtf_flushstr(&str, userdata, text);
				if (underline_start)
					if (underline_start(userdata))
						return 0;
			}
			if (strcmp(buf, "ul0") == 0 && paragraph){
				_unrtf_flushstr(&str, userdata, text);
				if (underline_end)
					if (underline_end(userdata))
						return 0;
			}

			if (strcmp(buf, "qc") == 0)
				if (style && paragraph)
					if (style(userdata, "CENTER"))
						return 0;
			
			if (strcmp(buf, "ql") == 0)
				if (style && paragraph)
					if (style(userdata, "LEFT"))
						return 0;
			
			if (strcmp(buf, "qr") == 0)
				if (style && paragraph)
					if (style(userdata, "RIGHT"))
						return 0;

			if (_unrtf_isli(buf) && paragraph)
				if (style)
					if (style(userdata, "Q"))
						return 0;

			if (_unrtf_islist(buf) && paragraph)
				if (style)
					if (style(userdata, "LN"))
						return 0;

/*******************************************************/
/* UNICODE */	
/*******************************************************/
			if (_unrtf_isutf(buf)){
				// handle utf
				char unicode[5] = 
				{
					buf[1], 
					buf[2], 
					buf[3], 
					buf[4], 
					0
				};
				
				uint32_t u;
				sscanf(unicode, "%u", &u);			
				char s[7];
				int l = _unrtf_c32tomb(s, u);
				s[l] = 0;
				_unrtf_str_append(&str, (char*)s,
					 	strlen((char*)s));
				
				// if next char is space or ? - drop it
				if (ch == ' ' || ch == '?')
					continue;
			}

/*******************************************************/
/* TABLES */	
/*******************************************************/
			if (strcmp(buf, "trowd") == 0){
				paragraph = 1;
				istable = 1;
				row = 0;
				cell = 0;
				ncolwidth = 0;
				if (table_start)
					if(table_start(userdata))
						return 0;
				if (tablerow_start)
					if (tablerow_start(userdata, row))
						return 0;
			}
			
			if (strcmp(buf, "lastrow") == 0){
				istable = 0;
				ncolwidth = 0;
				if (table_end)
					if (table_end(userdata))
						return 0;
				if (row)
					if (tablerow_end)
						if (tablerow_end(userdata, row))
							return 0;
				paragraph = 0;
			}
			
			if (strcmp(buf, "row") == 0){
				row = 0;
				if (tablerow_end)
					if (tablerow_end(userdata, row))
						return 0;
				row++;
				cell = 0;
			}
			
			if (strcmp(buf, "intbl") == 0){
				if (tablecell_start)
					if (tablecell_start(userdata, cell))
						return 0;
			}
			
			if (strcmp(buf, "cell") == 0){
				_unrtf_flushstr(&str, userdata, text);
				if (tablecell_end)
					if (tablecell_end(userdata, cell))
						return 0;
				cell++;
			}

			if (_unrtf_iscolwidth(buf)){
				char *s = buf + 5;
				colwidth[ncolwidth] = atoi(s);
				if (tablerow_width)
					if (tablerow_width(userdata, 
								ncolwidth, colwidth[ncolwidth]))
						return 0;
				
				ncolwidth++;
			}
			
/*******************************************************/
/* PICTURES */	
/*******************************************************/
			if (strcmp(buf, "shppict") == 0){
				// Specifies a Word 97 picture. This is a
				// destination control word.
				shppict = 1;
			}
			
			if (strcmp(buf, "nonshppict") == 0){
				// Specifies that Word 97 has written a 
				// {\pict destination that it will not read on
				// input. This keyword is for compatibility with
				// other readers.
				nonshppict = 1;
			}
			
			if (
					strcmp(buf, "pict") == 0 &&
					!nonshppict
					)
			{
				// this is picture
				pict = 1;
				// data will start after space or newline
				ch = fgetc(fp);
				while (
						ch != '\n' && 
						ch != '\r' && 
						ch != EOF)
				{
					if (ch == ' '){
						// look if next char is hex
						ch = fgetc(fp);
						if (_unrtf_ishex(ch))
							break;
					}

					ch = fgetc(fp);
				}
				if (ch == EOF)
					break;

				// get next char if not hex
				if (!_unrtf_ishex(ch))
					ch = fgetc(fp);

				// get image data until '}'
				struct _unrtf_str img;
				_unrtf_str_init(&img, BUFSIZ);
				while (ch != '}' && ch != EOF) {
					// add ony if hex (drop spaces and newlines)
					if (_unrtf_ishex(ch)){
						char c = (char)ch;
						_unrtf_str_append(&img, &c, 1);
					}
					ch = fgetc(fp);
				}
				if (ch == EOF)
					break;

				// convert image hex string to binary
				size_t len = img.len/2;
				unsigned char *data = 
					(unsigned char*)malloc(len);
				if (!data) // not enough memory
					break;
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
				if (image)
					if(image(userdata, data, len))	
						return 0;

				// free image data and string
				free(data);
				free(img.str);

				// goto next char in RTF
				continue;
			}
			
			// handle with ch again
			goto unrtf_parse_start;

		} else {
/*******************************************************/
/* ASCII */	
/*******************************************************/
			if (
					paragraph && 
					_unrtf_isvalid(ch) && 
					!pict &&
					!shppict &&
					!nonshppict
					)
			{
				// callback plain text
				char s[2] = {(char)ch, 0};
				_unrtf_str_append(&str, (char*)s, 1);
			}
		}
	}

	fclose(fp);
	return 0;
}
#define UNRTF_H_
#endif /* ifndef UNRTF_H_ */
