/**
 * File              : rtftype.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 17.01.2024
 * Last Modified Date: 19.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#ifndef RTFTYPE_H
#define RTFTYPE_H
#include <stdio.h>
#define fTrue 1
#define fFalse 0

/* fonts */
typedef enum {
	fnil,              // Unknown or default fonts (the default) 
	froman,            // Roman, proportionally spaced serif fonts
	fswiss,
	fmodern,
	fscript,
	fdecor,
	ftech,
	fbidi,             // Arabic, Hebrew, or other bidirectional font

} FFAM;              // font family
										 
// Specifies the pitch of a font in the font table. 
typedef enum {
	fDpich,    // Default pitch
	fFpich,    // Fixed pitch
	fVpich     // Variable pitch 
} FPCH;

typedef struct font {
	int  num;          // font number
	char name[64];     // font name
	int  lname;        // len of name
	FFAM ffam;         // font family
	int  charset;      //
	int	 falt[64];     // alternative font 
	int  lfalt;        // len of falt
	FPCH fprq;         // Specifies the pitch of a font in the font table. 
	char ftype;        // font type (nil/truetype)
	int  cpg;          // codepage
} FONT;

/* colors */
typedef struct color {
	char red;          // 0-255
	char green;        // 0-255
	char blue;         // 0-255
} COLOR;

/* Character Properties */
typedef struct char_prop
{
	char fBold;
	char fUnderline;
	char fItalic;
	int  font;
	int  size;
	int  fcolor;
	int  bcolor;
} CHP;               // CHaracter Properties

/* Paragraph justification */
typedef enum {
	justL,             // left
	justR,             // rigth
	justC,             // center
	justF              // full
} JUST;

/* Paragraph properties */
typedef struct para_prop
{
  int xaLeft;        // left indent in twips
  int xaRight;       // right indent in twips
	int xaFirst;       // first line indent in twips
	JUST just;         // justification
	int font;          // font 
	int s;             // paragraph style 
} PAP;               // PAragraph Properties

/* Section break type */
typedef enum {
	sbkNon,            // No section break
	sbkCol,            // Section starts a new column  
	sbkEvn,            // Section starts at an even page  
	sbkOdd,            // Section starts at an odd page 
	sbkPg              // Section starts a new page (by Default)
} SBK;

/* Page number formating */
typedef enum {
	pgDec,             // Page-number format is decimal
	pgURom,            // Page-number format is uppercase roman numeral 
	pgLRom,            // Page-number format is lowercase roman numeral 
	pgULtr,            // Page-number format is uppercase letter 
	pgLLtr             // Page-number format is lowerrcase letter 
} PGN;

/* Section properties */
typedef struct sect_prop
{
	int cCols;        // number of columns
	SBK sbk;          // section break type
	int xaPgn;        // x position of page number in twips
	int yaPgn;        // y position of page number in twips
	PGN pgnFormat;    // how the page number is formatted
	int ds;           // section style
} SEP;              // SEction Properties


/* table row borders */
typedef	struct tbr_borders {
	char top;
	char left;
	char bottom;
	char right;
	char horizontal;
	char vertical;
} TRB;

/* table cell borders */
typedef	struct tbc_borders {
	char top;
	char left;
	char bottom;
	char right;
} TCB;

/* table cell alignment */
typedef	struct tbc_alignment {
	char top;
	char centred;
	char bottom;
	char lvertical;
	char rvertical;
} TCA;

/* table row properties */
typedef struct tbr_prop {
	JUST just;
	int  trgaph[32];  // space bitwin cells
	int  cellx[32];   // size of cell (right boundary)
	int  trrh;        // table row height
	TRB  borders;
	char header;      // this row is header
	char keep;        // keep this row from pagebreak
} TRP;

/* table cell properties */
typedef struct tbc_prop {
	TCA  alignment;
	TCB  borders;
	char direction;   // 0 - left-to right, 1 - right to left
} TCP;

/* charset */
typedef enum {
	charset_ansi,
	charset_mac,
	charset_pc,
	charset_pca
} CHSET;

typedef struct rtf_info {
	/* data */
	char  title[32];  // document title
	int   ltitle;     // length of title
	char  author[32]; // document author
	int   lauthor;    // length of author string
	int   year;
	int   month;
	int   day;
	int   hour;
	int   min;
} INFO;

/* Document properties */
typedef struct doc_prop
{
	int   xaPage;     // page width in twips
	int   yaPage;     // page height in twips
	int   xaLeft;     // left margin in twips
	int   yaTop;      // top margin in twips
	int   xaRight;    // right margin in twips
	int   yaBottom;   // bottom margin in twips
	int   pgnStart;   // starting page number in twips
	char  fFacingp;   // facing pages enabled?
	char  fLandscape; // landscape or portrait??
	int   deflang;    // Default language
	int   deff;       // Default font
	int   defftab;    // Default tab width
	int   cpg;        // codepage
	CHSET chset;      // charset
	INFO  *info;      // Document info
} DOP;							// DOcument Properties

typedef	struct style {
	int s;           // paragraph style
	int ds;          // section style
	int sbedeon;     // based on style
	int next;        // next style
	char hidden;
	char name[64];     // style name
	int  lname;        // len of name
	CHP chp;
	PAP pap;
	SEP sep;
} STYLE;

// picture type
typedef	enum {
	pict_emf,     //
	pict_png,     //
	pict_jpg,     //
	pict_mac,     // Source of the picture is QuickDraw
	pict_wmf,     // 
	pict_omf,     // Source of the picture is an OS/2 metafile
	pict_ibitmap, // Source of the picture is a Windows device-independent bitmap 
	pict_dbitmap, // Source of the picture is a Windows device-dependent bitmap 
} PICT_T;

typedef struct picture {
						
} PICT;

#endif
