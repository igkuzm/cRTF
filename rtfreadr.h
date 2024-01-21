/**
 * File              : rtfreadr.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 18.01.2024
 * Last Modified Date: 19.01.2024
 Title Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include <stdio.h>
#include "rtftype.h"

typedef enum {
	sMain,
	sFootnotes,
} STREAM;

// command chars
enum {
	PAR = 257,  // end of parafraph
	SECT,				// end of section
	ROW,        // end of row
	CELL,       // end of cell
	FTNSEP,     // separates footnotes from the document
	FTNSEPC,    // separates continued footnotes from the document
	FTNCN,      // a notice for continued footnotes
	AFTNSEP,    // separates endnotes from the document
	AFTNSEPC,   // separates continued endnotes from the document
	AFTNCN,     // a notice for continued endnotes
	LIST,			  // list
};

typedef	struct rtfprop {
	/* data */
	CHP chp;
	PAP pap;
	SEP sep;
	DOP dop;
	TRP trp;
	TCP tcp;
} prop_t;

typedef enum {
	info_author,
	info_titile,
	info_subject,
	info_comment,
	info_keywords,
	info_manager,
	info_company,
	info_operator,
	info_category,
	info_doccomm,
	info_hlinkbase,
} tINFO;

typedef enum {
	date_create,
	date_revision,
	date_print,
	date_backup,
} tDATE;

typedef struct rtfnotify {
	void *udata;
	int (*command_cb)(void *udata, const char *s, int param, char fParam);
	int (*font_cb)(void *udata, FONT *p);
	int (*info_cb)(void *udata, tINFO t, const char *s);
	int (*date_cb)(void *udata, tDATE t, DATE *d);
	int (*style_cb)(void *udata, STYLE *s);
	int (*color_cb)(void *udata, COLOR *c);
	int (*char_cb)(void *udata, STREAM s, prop_t *p, int ch);
	int (*pict_cb)(void *udata, prop_t *p, PICT *pict);
} rnotify_t;

/* parse RTF file and run callbacks */
int ecRtfParse(FILE *fp, prop_t *prop, rnotify_t *no);

// RTF parser error codes
#define ecOK									0     // Everything's fine!
#define ecStackUnderflow      1     // Unmatched '}'
#define ecStackOverflow       2     // Too many '{' -- memory exhausted
#define ecUnmatchedBrace      3     // RTF ended during an open group.
#define ecInvalidHex          4     // invalid hex character found in data
#define ecBadTable            5     // RTF table (sym or prop) invalid
#define ecAssertion           6     // Assertion failure
#define ecEndOfFile           7     // End of file reached while reading RTF
