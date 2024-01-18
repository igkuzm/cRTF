/**
 * File              : unrtf.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 16.01.2024
 * Last Modified Date: 17.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */
/**
 * unrtf.h
 * Copyright (c) 2024 Igor V. Sementsov <ig.kuzm@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/* parse RTF file */

#ifndef UNRTF
#define UNRTF 

#include <stdio.h>
#include <stdlib.h>

typedef struct unrtf_control_word {
	/* data */
	char  *np;    // numeric parameter or NULL
	char  cw[64]; // control word (null-terminated string)
} cw_t;

/*! \enum UNRTF_CH
 *
 *  character from rtf parse
 */
enum UNRTF_CH { 
	UNRTF_OB,     // open braces '{'
	UNRTF_CB,     // close braces '}'
	UNRTF_ASCII,  // ANSI char
	UNRTF_CW,     // Control Word
	UNRTF_CS,     // Control Symbol
	UNRTF_UTF8,   // utf8 coded multybite char
	UNRTF_CP,     // codepage coded 8-bit char
	UNRTF_BIN,    // binary data
};

/* return 1 if lowercase alphabetic characters between ‘a’
 * and ‘z’ inclusive */
static int _islch(int ch){
 if (ch >= 'a' && ch <= 'z')
	 return 1;
 return 0;
} 

/* return 1 if digital characters between ‘0’ and ‘9’ 
 * inclusive */
static int _isdigit(int ch){
 if (ch >= '0' && ch <= '9')
	 return 1;
 return 0;
} 


/* read control word from current position of stream and 
 * change current character. Return 1 and set pointer to 
 * cw if found control word */
static int _unrtf_read_cw(
		FILE *fp, // file pointer 
		int  *ch, // pointer to current character
		cw_t *cw) // pointer to control word
{
	// backslash begins each control word
	if (*ch != '\\')
		return 0;

	*cw = {NULL, 0};

	int i, l=0, hp = 0;
	// iterate chars
	for (i = 0; i < 64; ++i) {
		*ch = fgetc(fp);	
		if (_islch(*ch)){
			// append char to cw
			cw->cw[l++] = *ch;
			continue;
		}

		if (_isdigit(*ch) || *ch == '-'){
			// null-terminate cw
			cw->cw[l++] = 0;

			// add pointer of numeric parameter to buffer
			// if not set
			if (hp)
				cw->np = &(cw->cw[l]);
			hp = 1;
			
			// add numeric parameter to buffer
			cw->cw[l++] = *ch;
			continue;
		}

		// A space delimiter. In this case, the space is part 
		// of the control word
		if (*ch == ' '){
			// iterate ch and stop the loop
			*ch = fgetc(fp);	
		}

		/* stop the loop */
		// null-terminate string
		cw->cw[l] = 0;

		// check if we have control word
		if (cw->cw[0] != 0)
			return 1;

		return 0;
	}

	// buffer overload
	*cw = {NULL,0,};
	return 0;
}

/* parse RTF file */
static UNRTF_CH 
unrtf_parse(
		FILE *fp, int *ch, unsigned char value[64])
{
	int l = 0;
	value[0] = 0;
	*ch = fgetc(fp);

	if (*ch == '{')
		return UNRTF_CB;
	
	if (*ch == '}')
		return UNRTF_CB;
	

	if (*ch == '\\'){
		if (_unrtf_read_cw(fp, ch, (cw_t*)value)){
			return UNRTF_CW;	
		} 
		else {
			// if it is '\\{' or '\\}' simbol - use them as text
			if (*ch == '{' || *ch == '}'){
				value[l++] = *ch;
				*ch = fgetc(fp);
				return UNRTF_ASCII;
			}
		}
	} else {
		buf[l++] = ch;
		buf[l] = 0;
		if (blen)
			*blen = l;
		ch = fgetc(fp);
		return ch;
	}

	return ch;
}


#endif /* ifndef UNRTF */
