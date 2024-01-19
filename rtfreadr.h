/**
 * File              : rtfreadr.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 18.01.2024
 * Last Modified Date: 19.01.2024
 Title Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include <stdio.h>
#include "rtftype.h"

typedef	struct rtfprop {
	/* data */
	CHP chp;
	PAP pap;
	SEP sep;
	DOP dop;
	TRP trp;
	TCP tcp;
} rprop_t;

typedef struct rtfnotify {
	void *udata;
	int (*font_cb)(void *udata, FONT *p);
	int (*style_cb)(void *udata, STYLE *s);
	int (*color_cb)(void *udata, COLOR *c);
	int (*sect_cb)(void *udata);
	int (*par_cb)(void *udata);
	int (*row_cb)(void *udata);
	int (*cell_cb)(void *udata);
	int (*char_cb)(void *udata, int ch);
	int (*picture_cb) (void *udata, void *data, int len);
} rnotify_t;

/* parse RTF file and run callbacks */
int ecRtfParse(FILE *fp, rprop_t *prop, rnotify_t *no);

// RTF parser error codes
#define ecOK									0     // Everything's fine!
#define ecStackUnderflow      1     // Unmatched '}'
#define ecStackOverflow       2     // Too many '{' -- memory exhausted
#define ecUnmatchedBrace      3     // RTF ended during an open group.
#define ecInvalidHex          4     // invalid hex character found in data
#define ecBadTable            5     // RTF table (sym or prop) invalid
#define ecAssertion           6     // Assertion failure
#define ecEndOfFile           7     // End of file reached while reading RTF
