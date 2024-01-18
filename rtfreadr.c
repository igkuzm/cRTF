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
	rdsSkip 
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
	idestSkip 
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

// Parser vars
int cGroup;
bool fSkipDestIfUnk;
bool isUTF;
long cbBin;
long lParam;
RDS rds;
RIS ris;
SAVE *psave;
FILE *fpIn;

rprop_t *prop;
rnotify_t *no;

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
		 actnSpec,   propSep,       0,                                    // ipropTcelld
		 actnSpec,   propSep,       0,                                    // ipropRow
		 actnSpec,   propSep,       0,                                    // ipropCell
};

// Keyword descriptions
SYM rgsymRtf[] = {
//   keyword      dflt       fPassDflt   kwd              idx
		 "b",         1,         fFalse,     kwdProp,         ipropBold,
		 "ul",        1,         fFalse,     kwdProp,         ipropUnderline,
		 "i",         1,         fFalse,     kwdProp,         ipropItalic,
	   "li",        0,         fFalse,     kwdProp,         ipropLeftInd,
	   "ri",        0,         fFalse,     kwdProp,         ipropRightInd,
	   "fi",        0,         fFalse,     kwdProp,         ipropFirstInd,
	   "cols",      1,         fFalse,     kwdProp,         ipropCols,
	   "sbknone",   sbkNon,    fTrue,      kwdProp,         ipropSbk,
	   "sbkcol",    sbkCol,    fTrue,      kwdProp,         ipropSbk,
	   "sbkeven",   sbkEvn,    fTrue,      kwdProp,         ipropSbk,
	   "sbkodd",    sbkOdd,    fTrue,      kwdProp,         ipropSbk,
	   "sbkpage",   sbkPg,     fTrue,      kwdProp,         ipropSbk,
	   "pgnx",      0,         fFalse,     kwdProp,         ipropPgnX,
	   "pgny",      0,         fFalse,     kwdProp,         ipropPgnY,
	   "pgndec",    pgDec,     fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnucrm",   pgURom,    fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnlcrm",   pgLRom,    fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnucltr",  pgULtr,    fTrue,      kwdProp,         ipropPgnFormat,
	   "pgnlcltr",  pgLLtr,    fTrue,      kwdProp,         ipropPgnFormat,
	   "qc",        justC,     fTrue,      kwdProp,         ipropJust,
	   "ql",        justL,     fTrue,      kwdProp,         ipropJust,
	   "qr",        justR,     fTrue,      kwdProp,         ipropJust,
	   "qj",        justF,     fTrue,      kwdProp,         ipropJust,
	   "paperw",    12240,     fFalse,     kwdProp,         ipropXaPage,
	   "paperh",    15480,     fFalse,     kwdProp,         ipropYaPage,
	   "margl",     1800,      fFalse,     kwdProp,         ipropXaLeft,
	   "margr",     1800,      fFalse,     kwdProp,         ipropXaRight,
	   "margt",     1440,      fFalse,     kwdProp,         ipropYaTop,
	   "margb",     1440,      fFalse,     kwdProp,         ipropYaBottom,
	   "pgnstart",  1,         fTrue,      kwdProp,         ipropPgnStart,
	   "facingp",   1,         fTrue,      kwdProp,         ipropFacingp,
	   "landscape", 1,         fTrue,      kwdProp,         ipropLandscape,
	   "par",       0,         fFalse,     kwdProp,         ipropPar,
	   "pard",      0,         fFalse,     kwdProp,         ipropPard,
	   "\0x0a",     0,         fFalse,     kwdChar,         0x0a,
	   "\0x0d",     0,         fFalse,     kwdChar,         0x0a,
	   "tab",       0,         fFalse,     kwdChar,         0x09,
	   "trowd",     0,         fFalse,     kwdProp,         ipropTrowd,
	   "tcelld",    0,         fFalse,     kwdProp,         ipropTcelld,
	   "ldblquote", 0,         fFalse,     kwdChar,         '"',
	   "rdblquote", 0,         fFalse,     kwdChar,         '"',
	   "bin",       0,         fFalse,     kwdSpec,         ipfnBin,
	   "*",         0,         fFalse,     kwdSpec,         ipfnSkipDest,
	   "'",         0,         fFalse,     kwdSpec,         ipfnHex,
	   "author",    0,         fFalse,     kwdDest,         idestSkip,
	   "buptim",    0,         fFalse,     kwdDest,         idestSkip,
	   "colortbl",  0,         fFalse,     kwdDest,         idestSkip,
	   "comment",   0,         fFalse,     kwdDest,         idestSkip,
	   "creatim",   0,         fFalse,     kwdDest,         idestSkip,
	   "doccomm",   0,         fFalse,     kwdDest,         idestSkip,
	   "fonttbl",   0,         fFalse,     kwdDest,         idestSkip,
	   "footer",    0,         fFalse,     kwdDest,         idestSkip,
	   "footerf",   0,         fFalse,     kwdDest,         idestSkip,
	   "footerl",   0,         fFalse,     kwdDest,         idestSkip,
	   "footerr",   0,         fFalse,     kwdDest,         idestSkip,
	   "footnote",  0,         fFalse,     kwdDest,         idestSkip,
	   "ftncn",     0,         fFalse,     kwdDest,         idestSkip,
	   "ftnsep",    0,         fFalse,     kwdDest,         idestSkip,
	   "ftnsepc",   0,         fFalse,     kwdDest,         idestSkip,
	   "header",    0,         fFalse,     kwdDest,         idestSkip,
	   "headerf",   0,         fFalse,     kwdDest,         idestSkip,
	   "headerl",   0,         fFalse,     kwdDest,         idestSkip,
	   "headerr",   0,         fFalse,     kwdDest,         idestSkip,
	   "info",      0,         fFalse,     kwdDest,         idestSkip,
	   "keywords",  0,         fFalse,     kwdDest,         idestSkip,
	   "operator",  0,         fFalse,     kwdDest,         idestSkip,
	   "pict",      0,         fFalse,     kwdDest,         idestSkip,
	   "printim",   0,         fFalse,     kwdDest,         idestSkip,
	   "private1",  0,         fFalse,     kwdDest,         idestSkip,
	   "revtim",    0,         fFalse,     kwdDest,         idestSkip,
	   "rxe",       0,         fFalse,     kwdDest,         idestSkip,
	   "sect",      0,         fFalse,     kwdProp,         ipropSect,
	   "stylesheet",0,         fFalse,     kwdDest,         idestSkip,
	   "subject",   0,         fFalse,     kwdDest,         idestSkip,
	   "tc",        0,         fFalse,     kwdDest,         idestSkip,
	   "title",     0,         fFalse,     kwdDest,         idestSkip,
	   "txe",       0,         fFalse,     kwdDest,         idestSkip,
	   "u",         0,         fFalse,     kwdUTF,          0,
	   "xe",        0,         fFalse,     kwdDest,         idestSkip,
	   "{",         0,         fFalse,     kwdChar,         '{',
	   "}",         0,         fFalse,     kwdChar,         '}',
	   "\\",        0,         fFalse,     kwdChar,         '\\',
	   "row",       0,         fFalse,     kwdProp,         ipropRow,
	   "cell",      0,         fFalse,     kwdProp,         ipropCell,
	 	};

int isymMax = sizeof(rgsymRtf) / sizeof(SYM);

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
		
		case ipropSect:
			if (no->sect_cb)
				no->sect_cb(no->udata);
		
		case ipropRow:
			if (no->row_cb)
				no->row_cb(no->udata);
		
		case ipropCell:
			if (no->cell_cb)
				no->cell_cb(no->udata);

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
		{
			/*if (ris != risUTF)*/
			ris = risHex;
		}
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

//
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
