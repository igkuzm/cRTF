/**
 * File              : rtfreadr.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 17.01.2024
 * Last Modified Date: 19.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "rtftype.h"
#include "rtfreadr.h"
#include "utf.h"

typedef enum { 
	rdsNorm, 
	rdsFonttbl, 
	rdsFalt, 
	rdsColor, 
	rdsSkip,
	rdsStyle,
	rdsInfo,
	rdsTitle,
	rdsAuthor,
	rdsShppict,
	rdsPict,
} RDS;                    // Rtf Destination State

typedef enum { 
	risNorm, 
	risBin, 
	risHex
} RIS;                    // Rtf Internal State

typedef struct save       // property save structure
{
	struct save *pNext;     // next save
	CHP chp;
	PAP pap;
	SEP sep;
	DOP dop;
	RDS rds;
	RIS ris;
	TRP trp;
	TCP tcp;
} SAVE;
// What types of properties are there?
typedef enum {
	ipropBold, 
	ipropItalic, 
	ipropUnderline, 
	ipropLeftInd,
	ipropRightInd, 
	ipropFirstInd, 
	ipropCols, 
	ipropPgnX,
	ipropPgnY, 
	ipropXaPage, 
	ipropYaPage, 
	ipropXaLeft,
	ipropXaRight, 
	ipropYaTop, 
	ipropYaBottom, 
	ipropPgnStart,
	ipropSbk, 
	ipropPgnFormat, 
	ipropFacingp, 
	ipropLandscape,
	ipropJust, 
	ipropPard, 
	ipropPlain, 
	ipropSectd,
	ipropPar, 
	ipropTrowd, 
	ipropTcelld, 
	ipropSect, 
	ipropRow, 
	ipropCell, 
	ipropFcharset,
	ipropFprq,
	ipropFtype,
	ipropFnum,
	ipropCred,
	ipropCgreen,
	ipropCblue,
	ipropFfam,
	ipropStyle,
	ipropDStyle,
	ipropYear,
	ipropMonth,
	ipropDay,
	ipropHour,
	ipropMin,
	
	ipropMax
} IPROP;

typedef enum {
	actnSpec, 
	actnByte, 
	actnWord
} ACTN;

typedef enum {
	propChp, 
	propPap, 
	propSep, 
	propDop,
	propTrp,
	propTcp,
	propFnt,
	propCol,
	propInfo,
} PROPTYPE;

typedef struct propmod
{
	ACTN actn;                 // size of value
	PROPTYPE prop;             // structure containing value
	int   offset;              // offset of value from base of structure
} PROP;

typedef enum {
	ipfnBin, 
	ipfnHex, 
	ipfnSkipDest
} IPFN;

typedef enum {
	idestPict, 
	idestSkip,
	idestFnt,
	idestCol,
	idestFalt,
	idestStyle,
	idestInfo,
	idestTitle,
	idestAuthor,
	idestCreatim,
	idestShppict,
} IDEST;

typedef enum {
	kwdChar, 
	kwdDest, 
	kwdProp, 
	kwdSpec, 
	kwdUTF
} KWD;

typedef struct symbol
{
		char *szKeyword;       // RTF keyword
		int   dflt;            // default value to use
		bool fPassDflt;        // true to use default value from this table
		KWD   kwd;             // base action to take
		int   idx;             // index into property table if kwd == kwdProp
													 // index into destination table if kwd == kwdDest
													 // character to print if kwd == kwdChar
} SYM;

// RTF parser tables
// Property descriptions
PROP rgprop [ipropMax] = {
		 actnByte,   propChp,       offsetof(CHP, fBold),                 // ipropBold
		 actnByte,   propChp,       offsetof(CHP, fItalic),               // ipropItalic
		 actnByte,   propChp,       offsetof(CHP, fUnderline),            // ipropUnderline
		 actnWord,   propPap,       offsetof(PAP, xaLeft),                // ipropLeftInd
		 actnWord,   propPap,       offsetof(PAP, xaRight),               // ipropRightInd
		 actnWord,   propPap,       offsetof(PAP, xaFirst),               // ipropFirstInd
		 actnWord,   propSep,       offsetof(SEP, cCols),                 // ipropCols
		 actnWord,   propSep,       offsetof(SEP, xaPgn),                 // ipropPgnX
		 actnWord,   propSep,       offsetof(SEP, yaPgn),                 // ipropPgnY
		 actnWord,   propDop,       offsetof(DOP, xaPage),                // ipropXaPage
		 actnWord,   propDop,       offsetof(DOP, yaPage),                // ipropYaPage
		 actnWord,   propDop,       offsetof(DOP, xaLeft),                // ipropXaLeft
		 actnWord,   propDop,       offsetof(DOP, xaRight),               // ipropXaRight
		 actnWord,   propDop,       offsetof(DOP, yaTop),                 // ipropYaTop
		 actnWord,   propDop,       offsetof(DOP, yaBottom),              // ipropYaBottom
		 actnWord,   propDop,       offsetof(DOP, pgnStart),              // ipropPgnStart
		 actnByte,   propSep,       offsetof(SEP, sbk),                   // ipropSbk
		 actnByte,   propSep,       offsetof(SEP, pgnFormat),             // ipropPgnFormat
		 actnByte,   propDop,       offsetof(DOP, fFacingp),              // ipropFacingp
		 actnByte,   propDop,       offsetof(DOP, fLandscape),            // ipropLandscape
		 actnByte,   propPap,       offsetof(PAP, just),                  // ipropJust
		 actnSpec,   propPap,       0,                                    // ipropPard
		 actnSpec,   propChp,       0,                                    // ipropPlain
		 actnSpec,   propSep,       0,                                    // ipropSectd
		 actnSpec,   propSep,       0,                                    // ipropPar
		 actnSpec,   propSep,       0,                                    // ipropTrowd
		 actnSpec,   propSep,       0,                                    // ipropTcelld
		 actnSpec,   propSep,       0,                                    // ipropSect
		 actnSpec,   propSep,       0,                                    // ipropRow
		 actnSpec,   propSep,       0,                                    // ipropCell
		 actnWord,   propFnt,       offsetof(FONT, charset),              // ipropFcharset
		 actnWord,   propFnt,       offsetof(FONT, fprq),                 // ipropFprq
		 actnByte,   propFnt,       offsetof(FONT, ftype),                // ipropFtype
		 actnWord,   propFnt,       offsetof(FONT, num),                  // ipropFnum
		 actnWord,   propCol,       offsetof(COLOR, red),                 // ipropCred
		 actnWord,   propCol,       offsetof(COLOR, green),               // ipropCgreen
		 actnWord,   propCol,       offsetof(COLOR, blue),                // ipropCblue
		 actnByte,   propFnt,       offsetof(FONT,  ffam),                // ipropFfam
		 actnSpec,   propPap,       0,                                    // ipropStyle
		 actnSpec,   propSep,       0,                                    // ipropDStyle
		 actnWord,   propInfo,      offsetof(INFO, year),                 // ipropYear
		 actnWord,   propInfo,      offsetof(INFO, month),                // ipropMonth
		 actnWord,   propInfo,      offsetof(INFO, day),                  // ipropDay
		 actnWord,   propInfo,      offsetof(INFO, hour),                 // ipropHour
		 actnWord,   propInfo,      offsetof(INFO, min),                  // ipropMin
};

// Keyword descriptions
SYM rgsymRtf[] = {
//   keyword       dflt       fPassDflt   kwd              idx
		 "b",          1,         fFalse,     kwdProp,         ipropBold,
		 "ds",         0,         fFalse,     kwdProp,         ipropDStyle,
		 "i",          1,         fFalse,     kwdProp,         ipropItalic,
		 "s",          0,         fFalse,     kwdProp,         ipropStyle,
		 "ul",         1,         fFalse,     kwdProp,         ipropUnderline,
	   "'",          0,         fFalse,     kwdSpec,         ipfnHex,
	   "*",          0,         fFalse,     kwdSpec,         ipfnSkipDest,
	   "\0x0a",      0,         fFalse,     kwdChar,         0x0a,
	   "\0x0d",      0,         fFalse,     kwdChar,         0x0a,
	   "\\",         0,         fFalse,     kwdChar,         '\\',
	   "author",     0,         fFalse,     kwdDest,         idestAuthor,
	   "bin",        0,         fFalse,     kwdSpec,         ipfnBin,
	   "blue",       0,         fFalse,     kwdProp,         ipropCblue,
	   "buptim",     0,         fFalse,     kwdDest,         idestSkip,
	   "cell",       0,         fFalse,     kwdProp,         ipropCell,
	   "colortbl",   0,         fFalse,     kwdDest,         idestCol,
	   "cols",       1,         fFalse,     kwdProp,         ipropCols,
	   "comment",    0,         fFalse,     kwdDest,         idestSkip,
	   "creatim",    0,         fFalse,     kwdDest,         idestCreatim,
	   "doccomm",    0,         fFalse,     kwdDest,         idestSkip,
	   "dy",         0,         fFalse,     kwdProp,         ipropDay,
	   "f",          0,         fFalse,     kwdProp,         ipropFnum,
	   "facingp",    1,         fTrue,      kwdProp,         ipropFacingp,
	   "falt",       0,         fFalse,     kwdDest,         idestFalt,
	   "fbidi",      fbidi,     fTrue,      kwdProp,         ipropFfam,
	   "fcharset",   0,         fFalse,     kwdProp,         ipropFcharset,
	   "fdecor",     fdecor,    fTrue,      kwdProp,         ipropFfam,
	   "fi",         0,         fFalse,     kwdProp,         ipropFirstInd,
	   "fmodern",    fmodern,   fTrue,      kwdProp,         ipropFfam,
	   "fnil",       fnil,      fTrue,      kwdProp,         ipropFfam,
	   "fonttbl",    0,         fFalse,     kwdDest,         idestFnt,
	   "footer",     0,         fFalse,     kwdDest,         idestSkip,
	   "footerf",    0,         fFalse,     kwdDest,         idestSkip,
	   "footerl",    0,         fFalse,     kwdDest,         idestSkip,
	   "footerr",    0,         fFalse,     kwdDest,         idestSkip,
	   "footnote",   0,         fFalse,     kwdDest,         idestSkip,
	   "fprq",       0,         fFalse,     kwdProp,         ipropFprq,
	   "froman",     froman,    fTrue,      kwdProp,         ipropFfam,
	   "fscript",    fscript,   fTrue,      kwdProp,         ipropFfam,
	   "fswiss",     fswiss,    fTrue,      kwdProp,         ipropFfam,
	   "ftech",      ftech,     fTrue,      kwdProp,         ipropFfam,
	   "ftncn",      0,         fFalse,     kwdDest,         idestSkip,
	   "ftnsep",     0,         fFalse,     kwdDest,         idestSkip,
	   "ftnsepc",    0,         fFalse,     kwdDest,         idestSkip,
	   "fttruetype", 1,         fFalse,     kwdProp,         ipropFtype,
	   "green",      0,         fFalse,     kwdProp,         ipropCgreen,
	   "header",     0,         fFalse,     kwdDest,         idestSkip,
	   "headerf",    0,         fFalse,     kwdDest,         idestSkip,
	   "headerl",    0,         fFalse,     kwdDest,         idestSkip,
	   "headerr",    0,         fFalse,     kwdDest,         idestSkip,
	   "hr",         0,         fFalse,     kwdProp,         ipropHour,
	   "info",       0,         fFalse,     kwdDest,         idestInfo,
	   "keywords",   0,         fFalse,     kwdDest,         idestSkip,
	   "landscape",  1,         fTrue,      kwdProp,         ipropLandscape,
	   "ldblquote",  0,         fFalse,     kwdChar,         '"',
	   "li",         0,         fFalse,     kwdProp,         ipropLeftInd,
	   "margb",      1440,      fFalse,     kwdProp,         ipropYaBottom,
	   "margl",      1800,      fFalse,     kwdProp,         ipropXaLeft,
	   "margr",      1800,      fFalse,     kwdProp,         ipropXaRight,
	   "margt",      1440,      fFalse,     kwdProp,         ipropYaTop,
	   "min",        0,         fFalse,     kwdProp,         ipropMin,
	   "mo",         0,         fFalse,     kwdProp,         ipropMonth,
	   "noshppict",  0,         fFalse,     kwdDest,         idestSkip,
	   "operator",   0,         fFalse,     kwdDest,         idestSkip,
	   "paperh",     15480,     fFalse,     kwdProp,         ipropYaPage,
	   "paperw",     12240,     fFalse,     kwdProp,         ipropXaPage,
	   "par",        0,         fFalse,     kwdProp,         ipropPar,
	   "pard",       0,         fFalse,     kwdProp,         ipropPard,
	   "pgndec",     pgDec,     fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnlcltr",   pgLLtr,    fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnlcrm",    pgLRom,    fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnstart",   1,         fTrue,      kwdProp,         ipropPgnStart,
	   "pgnucltr",   pgULtr,    fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnucrm",    pgURom,    fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnx",       0,         fFalse,     kwdProp,         ipropPgnX,
	   "pgny",       0,         fFalse,     kwdProp,         ipropPgnY,
	   "pict",       0,         fFalse,     kwdDest,         idestPict,
	   "printim",    0,         fFalse,     kwdDest,         idestSkip,
	   "private1",   0,         fFalse,     kwdDest,         idestSkip,
	   "qc",         justC,     fTrue,      kwdProp,         ipropJust,
	   "qj",         justF,     fTrue,      kwdProp,         ipropJust,
	   "ql",         justL,     fTrue,      kwdProp,         ipropJust,
	   "qr",         justR,     fTrue,      kwdProp,         ipropJust,
	   "rdblquote",  0,         fFalse,     kwdChar,         '"',
	   "red",        0,         fFalse,     kwdProp,         ipropCred,
	   "revtim",     0,         fFalse,     kwdDest,         idestSkip,
	   "ri",         0,         fFalse,     kwdProp,         ipropRightInd,
	   "row",        0,         fFalse,     kwdProp,         ipropRow,
	   "rxe",        0,         fFalse,     kwdDest,         idestSkip,
	   "sbkcol",     sbkCol,    fTrue,      kwdProp,         ipropSbk,
	   "sbkeven",    sbkEvn,    fTrue,      kwdProp,         ipropSbk,
	   "sbknone",    sbkNon,    fTrue,      kwdProp,         ipropSbk,
	   "sbkodd",     sbkOdd,    fTrue,      kwdProp,         ipropSbk,
	   "sbkpage",    sbkPg,     fTrue,      kwdProp,         ipropSbk,
	   "sect",       0,         fFalse,     kwdProp,         ipropSect,
	   "shppict",    0,         fFalse,     kwdDest,         idestShppict,
	   "shpinst",    0,         fFalse,     kwdDest,         idestShppict,
	   "stylesheet", 0,         fFalse,     kwdDest,         idestStyle,
	   "subject",    0,         fFalse,     kwdDest,         idestSkip,
	   "tab",        0,         fFalse,     kwdChar,         0x09,
	   "tc",         0,         fFalse,     kwdDest,         idestSkip,
	   "tcelld",     0,         fFalse,     kwdProp,         ipropTcelld,
	   "title",      0,         fFalse,     kwdDest,         idestTitle,
	   "trowd",      0,         fFalse,     kwdProp,         ipropTrowd,
	   "txe",        0,         fFalse,     kwdDest,         idestSkip,
	   "u",          0,         fFalse,     kwdUTF,          0,
	   "xe",         0,         fFalse,     kwdDest,         idestSkip,
	   "yr",         0,         fFalse,     kwdProp,         ipropYear,
	   "{",          0,         fFalse,     kwdChar,         '{',
	   "}",          0,         fFalse,     kwdChar,         '}',
	 	};

// Parser vars
int cGroup;
bool fSkipDestIfUnk;
bool isUTF;
long cbBin;
long lParam;
RDS rds;
RIS ris;
FONT fnt;
COLOR col;
SAVE *psave;
FILE *fpIn;

INFO info;

rprop_t *prop;
rnotify_t *no;

// STYLESHEET
STYLE stylesheet[256];
int nstyles;

// RTF parser declarations
int ecPushRtfState(void);
int ecPopRtfState(void);
int ecParseRtfKeyword(FILE *fp);
int ecParseChar(int c);
int ecParseUTF(int c);
int ecTranslateKeyword(char *szKeyword, int param, bool fParam);
int ecPrintChar(int ch);
int ecEndGroupAction(RDS rds);
int ecApplyPropChange(IPROP iprop, int val);
int ecChangeDest(IDEST idest);
int ecParseSpecialKeyword(IPFN ipfn);
int ecParseSpecialProperty(IPROP iprop, int val);
int ecParseHexByte(void);


int isymMax = sizeof(rgsymRtf) / sizeof(SYM);

struct data_t {
	unsigned char *data;
	int len;
	int size;
};

int data_init(struct data_t *d)
{
	d->size = BUFSIZ;
	d->data = malloc(d->size);
	if (!d->data)
		return -1;
	d->len = 0;
	return 0;
}

void data_append(struct data_t *d, unsigned char c)
{
	int new_len = d->len + 1;
	if (new_len > d->size){
		void *ptr = realloc(d->data, d->size + BUFSIZ);
		if (!ptr)
			return;
		d->size += BUFSIZ;
	}

	d->data[d->len++] = c;
}

struct data_t picture;

//
// %%Function: ecApplyPropChange
//
// Set the property identified by _iprop_ to the value _val_.
//
//
int
ecApplyPropChange(IPROP iprop, int val)
{
	char *pb;
	if (rds == rdsSkip)             // If we're skipping text,
		return ecOK;                  // don't do anything.
		 
	switch (rgprop[iprop].prop)
	{
		case propDop:
			pb = (char *)&(prop->dop);
			break;
		case propSep:
			pb = (char *)&(prop->sep);
			break;
		case propPap:
			pb = (char *)&(prop->pap);
			break;
		case propChp:
			pb = (char *)&(prop->chp);
			break;
		case propTrp:
			pb = (char *)&(prop->trp);
			break;
		case propTcp:
			pb = (char *)&(prop->tcp);
			break;
		case propFnt:
			pb = (char *)&fnt;
			break;
		case propCol:
			pb = (char *)&col;
			break;
		case propInfo:
			pb = (char *)&info;
			break;
		 
		default:
			if (rgprop[iprop].actn != actnSpec)
				return ecBadTable;
			break;
	}

	switch (rgprop[iprop].actn)
	{
		case actnByte:
			pb[rgprop[iprop].offset] = (unsigned char) val;
			break;
		case actnWord:
			(*(int *) (pb+rgprop[iprop].offset)) = val;
			break;
		case actnSpec:
			return ecParseSpecialProperty(iprop, val);
			break;
		 
		default:
			return ecBadTable;
	}
	return ecOK;
}

//
// %%Function: ecParseSpecialProperty
//
// Set a property that requires code to evaluate.
//
int
ecParseSpecialProperty(IPROP iprop, int val)
{
	switch (iprop)
	{
		case ipropPard:
			memset(&(prop->pap), 0, sizeof(PAP));
			return ecOK;
		case ipropPlain:
			memset(&(prop->chp), 0, sizeof(CHP));
			return ecOK;
		case ipropSectd:
			memset(&(prop->sep), 0, sizeof(SEP));
			return ecOK;
		case ipropTrowd:
			memset(&(prop->trp), 0, sizeof(TRP));
			return ecOK;
		case ipropTcelld:
			memset(&(prop->tcp), 0, sizeof(TCP));
			return ecOK;
		
		case ipropPar:
			if (no->par_cb)
				no->par_cb(no->udata);
			//ecPrintChar(0x0a); // print new line
			return ecOK;
		
		case ipropSect:
			if (no->sect_cb)
				no->sect_cb(no->udata);
			return ecOK;
		
		case ipropRow:
			if (no->row_cb)
				no->row_cb(no->udata);
			return ecOK;
		
		case ipropCell:
			if (no->cell_cb)
				no->cell_cb(no->udata);
			return ecOK;

		case ipropStyle:
			if (rds == rdsStyle) // add to stylesheet
				stylesheet[nstyles].s = val;
			else{
				// apply styles to paragraph prop
				int i;
				for (i = 0; i < nstyles; ++i){
					if (stylesheet[i].s == val){
						prop->chp = stylesheet[i].chp;
						prop->pap = stylesheet[i].pap;
					}
				}
				prop->pap.s = val;
			}
			return ecOK;
		
		case ipropDStyle:
			if (rds == rdsStyle) // add to stylesheet
				stylesheet[nstyles].ds = val;
			else {
				// apply styles to section prop
				int i;
				for (i = 0; i < nstyles; ++i){
					if (stylesheet[i].s == val){
						prop->chp = stylesheet[i].chp;
						prop->pap = stylesheet[i].pap;
						prop->sep = stylesheet[i].sep;
					}
				}
				prop->sep.ds = val;
			}
			return ecOK;

		default:
			return ecBadTable;
	}
	return ecBadTable;
}

//
// %%Function: ecTranslateKeyword.
//
// Step 3.
// Search rgsymRtf for szKeyword and evaluate it appropriately.
//
// Inputs:
// szKeyword:      The RTF control to evaluate.
// param:          The parameter of the RTF control.
// fParam:         fTrue if the control had a parameter; (that is, if param is valid)
//                 fFalse if it did not.

int
ecTranslateKeyword(char *szKeyword, int param, bool fParam)
{
	int isym;
	
	// search for szKeyword in rgsymRtf
	for (isym = 0; isym < isymMax; isym++)
		if (strcmp(szKeyword, rgsymRtf[isym].szKeyword) == 0)
			break;
			
	if (isym == isymMax)        // control word not found
	{
		if (fSkipDestIfUnk)       // if this is a new destination
			rds = rdsSkip;          // skip the destination
															// else just discard it
		fSkipDestIfUnk = fFalse;
		return ecOK;
	}

	// found it!        
	// use kwd and idx to determine what to do with it.
	fSkipDestIfUnk = fFalse;
	
	switch (rgsymRtf[isym].kwd)
	{
		case kwdProp:
			if (rgsymRtf[isym].fPassDflt || !fParam)
				param = rgsymRtf[isym].dflt;
			return ecApplyPropChange(rgsymRtf[isym].idx, param);
		case kwdChar:
			return ecParseChar(rgsymRtf[isym].idx);
		case kwdDest:
			return ecChangeDest(rgsymRtf[isym].idx);
		case kwdSpec:
			return ecParseSpecialKeyword(rgsymRtf[isym].idx);
		case kwdUTF:
			return ecParseUTF(param);
			
		default:
			return ecBadTable;
	}
	return ecBadTable;
}

//
// %%Function: ecChangeDest
//
// Change to the destination specified by idest.
// There's usually more to do here than this...
//
int
ecChangeDest(IDEST idest)
{
	if (rds == rdsSkip)    // if we're skipping text,
		return ecOK;         // don't do anything
	
	switch (idest)
	{
		case idestFnt:
			memset(&fnt, 0, sizeof(FONT));
			rds = rdsFonttbl;
			break;

		case idestCol:
			memset(&col, 0, sizeof(COLOR));
			rds = rdsColor;
			break;
		
		case idestFalt:
			rds = rdsFalt;
			break;
		
		case idestStyle:
			rds = rdsStyle;
			break;
		
		case idestInfo:
			memset(&info, 0, sizeof(INFO));
			rds = rdsInfo;
			break;
		
		case idestAuthor:
			rds = rdsAuthor;
			break;
		
		case idestTitle:
			rds = rdsTitle;
			break;
		
		case idestShppict:
			rds = rdsShppict;
			break;
		
		case idestPict:
			{
				// try to allocate memory
				if (data_init(&picture))
					rds = rdsSkip;
				else
					rds = rdsPict;
			}
			break;
		
		case idestCreatim:
			break;

		default:
			rds = rdsSkip;     // when in doubt, skip it...
			break;
	}
	return ecOK;
}

//
// %%Function: ecEndGroupAction
//
// The destination specified by rds is coming to a close.
// If there's any cleanup that needs to be done, do it now.
//
int
ecEndGroupAction(RDS rds)
{
	if (rds == rdsPict){
		printf("PICTURE\n");
		// parse picture
		if (picture.data){
			int i; 
			for (i = 0; i < picture.len; ++i) {
				printf("%c", picture.data[i]);
			}
			printf("\n");
			free(picture.data);
		}
	}
	return ecOK;
}

//
// %%Function: ecParseSpecialKeyword
//
// Evaluate an RTF control that needs special processing.
//
int
ecParseSpecialKeyword(IPFN ipfn)
{
	if (rds == rdsSkip && ipfn != ipfnBin) // if we're skipping, and it's not
		return ecOK;                         // the \bin keyword, ignore it.
			 
	switch (ipfn)
	{
		case ipfnBin:
			ris = risBin;
			cbBin = lParam;
			break;
		case ipfnSkipDest:
			fSkipDestIfUnk = fTrue;
			break;
		case ipfnHex:
			ris = risHex;
		break;
			 
		default:
			return ecBadTable;
	}
	return ecOK;
}

//
// %%Function: ecRtfParse
//
// Step 1:
// Isolate RTF keywords and send them to ecParseRtfKeyword;
// Push and pop state at the start and end of RTF groups;
// Send text to ecParseChar for further processing.

int ecRtfParse(
		FILE *fp,
		rprop_t *_prop,
		rnotify_t *_no
		)
{
	fpIn = fp;
	prop = _prop;
	no = _no;
			
	memset(&info, 0, sizeof(INFO));
	prop->dop.info = &info;

	int ch;
	int ec;
	int cNibble = 2;
	int b = 0;
	while ((ch = getc(fp)) != EOF)
	{
		if (cGroup < 0)
			return ecStackUnderflow;
		if (ris == risBin) // if we're parsing binary data, 
											 // handle it directly
		{
			if ((ec = ecParseChar(ch)) != ecOK)
				return ec;
		}
		else
		{
			switch (ch)
			{
				case '{':
					if ((ec = ecPushRtfState()) != ecOK)
						return ec;
						break;
				case '}':
					if ((ec = ecPopRtfState()) != ecOK)
						return ec;
						break;
				case '\\':
					if ((ec = ecParseRtfKeyword(fp)) != ecOK)
						return ec;
						break;
				case 0x0d:
				case 0x0a:  // cr and lf are noise characters...
						break;
				default:
					if (ris == risNorm)
					{
						if ((ec = ecParseChar(ch)) != ecOK)
							return ec;
					}
					else {
						if (ris != risHex)
							return ecAssertion;
						
						if (isUTF){ // skip HEX if after UTF code
							cNibble--;
							if (!cNibble)
							{
								cNibble = 2;
								b = 0;
								ris = risNorm;
							}
							break;
						}

						b = b << 4;
						if (isdigit(ch))
							b += (char) ch - '0';
						else
						{
							if (islower(ch))
							{
								if (ch < 'a' || ch > 'f')
									return ecInvalidHex;
								b += (char) ch - 'a';
							}
							else
							{
								if (ch < 'A' || ch > 'F')
									return ecInvalidHex;
								b += (char) ch - 'A';
							}
						}
						cNibble--;
						if (!cNibble)
						{
							if ((ec = ecParseChar(b)) != ecOK)
								return ec;
							cNibble = 2;
							b = 0;
							ris = risNorm;
						}
					}         // end else (ris != risNorm)
					break;
				}           // switch
			}         // else (ris != risBin)
		}							// while
		if (cGroup < 0)
			return ecStackUnderflow;
		if (cGroup > 0)
			return ecUnmatchedBrace;
	return ecOK;
}

//
// %%Function: ecPushRtfState
//
// Save relevant info on a linked list of SAVE structures.
//
int
ecPushRtfState(void)
{
	SAVE *psaveNew = malloc(sizeof(SAVE));
	if (!psaveNew)
		return ecStackOverflow;
	psaveNew -> pNext = psave;
	psaveNew -> chp = prop->chp;
	psaveNew -> pap = prop->pap;
	psaveNew -> sep = prop->sep;
	psaveNew -> dop = prop->dop;
	psaveNew -> trp = prop->trp;
	psaveNew -> tcp = prop->tcp;
	psaveNew -> rds = rds;
	psaveNew -> ris = ris;
	ris = risNorm;
	psave = psaveNew;
	cGroup++;
	return ecOK;
}

//
// %%Function: ecPopRtfState
//
// If we're ending a destination (that is, the destination is changing),
// call ecEndGroupAction.
// Always restore relevant info from the top of the SAVE list.
//
int
ecPopRtfState(void)
{
	SAVE *psaveOld;
	int ec;
	if (!psave)
		return ecStackUnderflow;
	if (rds != psave->rds)
	{
		if ((ec = ecEndGroupAction(rds)) != ecOK)
			return ec;
	}
	prop->chp = psave->chp;
	prop->pap = psave->pap;
	prop->sep = psave->sep;
	prop->dop = psave->dop;
	prop->trp = psave->trp;
	prop->tcp = psave->tcp;
	rds = psave->rds;
	ris = psave->ris;
	psaveOld = psave;
	psave = psave->pNext;
	cGroup--;
	free(psaveOld);

	return ecOK;
}

//
// %%Function: ecParseRtfKeyword
//
// Step 2:
// get a control word (and its associated value) and
// call ecTranslateKeyword to dispatch the control.
//
int
ecParseRtfKeyword(FILE *fp)
{
	int ch;
	char fParam = fFalse;
	char fNeg = fFalse;
	int param = 0;
	char *pch;
	char szKeyword[30];
	char szParameter[20];
	szKeyword[0] = '\0';
	szParameter[0] = '\0';
	
	if ((ch = getc(fp)) == EOF)
		return ecEndOfFile;
		 
	// a control symbol; no delimiter.
	if (!isalpha(ch)) 
	{
		szKeyword[0] = (char) ch;
		szKeyword[1] = '\0';
		return ecTranslateKeyword(szKeyword, 0, fParam);
	}
		 
	for (pch = szKeyword; isalpha(ch); ch = getc(fp))
		*pch++ = (char) ch;
		 
	*pch = '\0';
	if (ch == '-')
	{
		fNeg    = fTrue;
		if ((ch = getc(fp)) == EOF)
			return ecEndOfFile;
	}

	if (isdigit(ch))
	{
		// a digit after the control means we have a parameter
		fParam = fTrue;
		
		for (pch = szParameter; isdigit(ch); ch = getc(fp))
			*pch++ = (char) ch;
				 
		*pch = '\0';
		param = atoi(szParameter);
		
		if (fNeg)
			param = -param;
				 
		lParam = atol(szParameter);
		
		if (fNeg)
			param = -param;
	}
	
	if (ch != ' ')
		ungetc(ch, fp);
		 
	return ecTranslateKeyword(szKeyword, param, fParam);
}

int
ecAddFont(int ch, char alt)
{
	if (ch == ';'){
		fnt.name[fnt.lname] = 0;
		fnt.falt[fnt.lfalt] = 0;

		if (no->font_cb)
			no->font_cb(no->udata, &fnt);
		memset(&fnt, 0, sizeof(FONT));
		return ecOK;
	}

	if (alt)
		fnt.falt[fnt.lfalt++] = ch;
	else 
		fnt.name[fnt.lname++] = ch;
	
	return ecOK;
}

int
ecAddColor(int ch)
{
	if (ch == ';'){
		if (no->color_cb)
			no->color_cb(no->udata, &col);
		memset(&col, 0, sizeof(COLOR));
	}
	return ecOK;
}

int
ecAddTitle(int ch)
{
	if (info.ltitle < sizeof(info.title))
		info.title[info.ltitle++] = ch;
	info.title[info.ltitle] = 0;
	return ecOK;
}

int
ecAddAuthor(int ch)
{
	if (info.lauthor < sizeof(info.author))
		info.author[info.lauthor++] = ch;
	info.author[info.lauthor] = 0;
	return ecOK;
}

int
ecAddPicture(int ch)
{
	data_append(&picture, ch);
	return ecOK;
}

int
ecAddStyle(int ch)
{
	if (ch == ';'){
		stylesheet[nstyles].chp = prop->chp;
		stylesheet[nstyles].pap = prop->pap;
		stylesheet[nstyles].sep = prop->sep;
		if (no->style_cb)
			no->style_cb(no->udata, &(stylesheet[nstyles]));
		nstyles++;
	} else 
		stylesheet[nstyles].name[stylesheet[nstyles].lname++] = ch;
	return ecOK;
}

// %%Function: ecParseChar
//
// Route the character to the appropriate destination stream.
//
int
ecParseChar(int ch)
{
	if (ris == risBin && --cbBin <= 0)
		ris = risNorm;
	switch (rds)
	{
		case rdsSkip:
			// Toss this character.
			return ecOK;
		
		case rdsFonttbl:
			return ecAddFont(ch, 0);
		
		case rdsFalt:
			return ecAddFont(ch, 1);
		
		case rdsColor:
			return ecAddColor(ch);

		case rdsStyle:
			return ecAddStyle(ch);
		
		case rdsAuthor:
			return ecAddAuthor(ch);
		
		case rdsTitle:
			return ecAddTitle(ch);
		
		case rdsPict:
			return ecAddPicture(ch);
		
		case rdsNorm:
			// Output a character. Properties are valid at this point.
			return ecPrintChar(ch);
			
		default:
			// handle other destinations....
			return ecOK;
	}
}

//
// %%Function: ecParseUTF
//
// Route the unicode character to the appropriate destination stream.
//
int
ecParseUTF(int ch)
{
	isUTF = fTrue; 
	// Output a character. Properties are valid at this point.
	int i;
	char s[6];
	int len = c32tomb(s, ch);
	for (i = 0; i < len; ++i) {
		ecParseChar(s[i]);
	}	
	return ecOK;
}

//
// %%Function: ecPrintChar
//
// Send a character to the output file.
//
int
ecPrintChar(int ch)
{
	if (no->char_cb)
		no->char_cb(no->udata, ch);
	return ecOK;
}
