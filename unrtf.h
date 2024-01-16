/**
 * File              : unrtf.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 16.01.2024
 * Last Modified Date: 16.01.2024
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
	int   np;     // numeric parameter
	int	  hp;			// 1 if has numeric parameter 
	char  cw[32]; // control word (null-terminated string)
} cw_t;

/* read control word from current position of stream and 
 * change current character. Return 1 and set pointer to 
 * cw if found control word */
static int _unrtf_read_cw(
		FILE *fp, // file pointer 
		int  *ch, // pointer to current character
		cw_t *cw) // pointer to control word or NULL
{
	// backslash begins each control word
	if (*ch != '\\')
		return 0;

	cw_t _cw = {0,0,0};

	int i, l=0;
	// iterate chars
	for (i = 0; i < 32; ++i) {
		*ch = fgetc(fp);	
	}

	return 1;
}


#endif /* ifndef UNRTF */
