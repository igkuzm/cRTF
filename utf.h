/**
 * File              : utf.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 27.05.2022
 * Last Modified Date: 27.07.2023
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#ifndef k_lib_utf_h__
#define k_lib_utf_h__

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stdint.h>

/* convert utf32 char to utf8 multybite char array and return number of bytes */ 
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

/* convert utf8 multybite null-terminated string to utf32 null-terminated string 
 * and return it's len */ 
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

/* parse utf8 string and callback utf32 chars; 
 * return non zero in callback to stop function */
static void utf8_to_utf32(
		const char *str, 
		void * user_data, 
		int (*callback)(void * user_data, uint32_t utf32_char))
{
	int i;
	char *ptr = (char *)str;
	while (*ptr){

		// get number of bites
		uint8_t c = *ptr;
		int n = 0;
		if      (c >= 252)/* 6-bytes */
			n = 6;
		else if (c >= 248)/* 5-bytes */
			n = 5;
		else if (c >= 240)/* 4-bytes */
			n = 4;
		else if (c >= 224)/* 3-bytes */
			n = 3;
		else if (c >= 192)/* 2-bytes */
			n = 2;
		else              /* 1-byte */
			n = 1;

		// convert utf8 to utf32 and make callback
		char buf[7];
		for (i = 0; i < n; ++i) {
			buf[i] = *ptr++;	
		}
		buf[i] = 0;
		uint32_t s32[1];
		mbtoc32(s32, buf);
		if (callback)
			if (callback(user_data, *s32))
				return;
	}
}

/* parse utf8-encoded text file and callback utf32 chars; 
 * return non zero in callback to stop function */
static void utf8_file_to_utf32(
		FILE * file, 
		void * user_data, 
		int (*callback)(void * user_data, uint32_t utf32_char))
{
	int ch, i;
    while ((ch = fgetc(file) != EOF)) { 
		// get number of bites
		uint8_t c = ch;
		int n = 0;
		if      (c >= 252)/* 6-bytes */
			n = 6;
		else if (c >= 248)/* 5-bytes */
			n = 5;
		else if (c >= 240)/* 4-bytes */
			n = 4;
		else if (c >= 224)/* 3-bytes */
			n = 3;
		else if (c >= 192)/* 2-bytes */
			n = 2;
		else              /* 1-byte */
			n = 1;

		// convert utf8 to utf32 and make callback
		char buf[7];
		buf[0] = ch;
		for (i = 1; i < n; ++i) {
			ch = fgetc(file);	
			if (ch == EOF)
				return;
			buf[i] = ch;
		}
		buf[i] = 0;
		uint32_t s32[1];
		mbtoc32(s32, buf);
		if (callback)
			if (callback(user_data, *s32))
				return;
	}
}

/* convert utf32 char to utf8 multybite array and make callback; 
 * return non zero in callback to stop function */
static void utf32_to_utf8(
		uint32_t utf32_char, 
		void * user_data, 
		int (*callback)(void * user_data, char c))
{
	int i;
	char s[6];
	int count = c32tomb(s, utf32_char); 
	for (i = 0; i < count; ++i) {
		if (callback)
			if (callback(user_data, s[i]))
				return;
	}
}


#ifdef __cplusplus
}
#endif

#endif //k_lib_utf_h__

