/**
 * File              : rtfreadr.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 17.01.2024
 * Last Modified Date: 20.01.2024
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
#include "str.h"

typedef enum { 
	rdsNorm, 
	rdsFonttbl, 
	rdsFalt, 
	rdsColor, 
	rdsSkip,
	rdsStyle,
	rdsInfo,
	rdsInfoString,
	rdsInfoDate,
	rdsShppict,
	rdsPict,
	rdsFootnote,
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
	ipropSec,
	ipropPicttype,
	ipropOmf,
	ipropWmf,
	ipropIbitmap,
	ipropDbitmap,
	ipropPicw,
	ipropPich,
	ipropPicwgoal,
	ipropPichgoal,
	ipropPicscalex,
	ipropPicscaley,
	ipropPicscaled,
	ipropUd,
	ipropVersion,
	ipropNofpages,
	ipropNofword,
	ipropNofchars,
	ipropNofcharsws,
	ipropId,
	ipropFsize,
	ipropFfcolor,
	ipropFbcolor,
	ipropRowjust,
	ipropRowwrite,
	ipropRowgaph,
	ipropCellx,
	ipropClmgf,
	ipropClmrg,
	ipropTrleft,
	ipropTrrh,
	ipropTrhdr,
	ipropTrkeep,
	ipropTrbrdrt,
	ipropTrbrdrb,
	ipropTrbrdrl,
	ipropTrbrdrr,
	ipropTrbrdrh,
	ipropTrbrdrv,
	ipropClbrdrt,
	ipropClbrdrb,
	ipropClbrdrl,
	ipropClbrdrr,
	ipropCellaligm,
	ipropCellshade,
	ipropCelllinecol,
	ipropCellbackcol,
	ipropCellpat,

	ipropMax
} IPROP;

typedef enum {
	actnSpec, 
	actnByte, 
	actnWord,
	actnLong,
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
	propDate,
	propPict,
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
	idestSubject,
	idestComment,
	idestKeywords,
	idestManger,
	idestCompany,
	idestOperator,
	idestCategory,
	idestDoccomm,
	idestHlinkbase,
	idestCreatim,
	idestRevtim,
	idestPrintim,
	idestBuptim,
	idestShppict,
	idestFootnote,
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
		 actnByte,   propChp,    offsetof(CHP, fBold),         // ipropBold
		 actnByte,   propChp,    offsetof(CHP, fItalic),       // ipropItalic
		 actnByte,   propChp,    offsetof(CHP, fUnderline),    // ipropUnderline
		 actnWord,   propPap,    offsetof(PAP, xaLeft),        // ipropLeftInd
		 actnWord,   propPap,    offsetof(PAP, xaRight),       // ipropRightInd
		 actnWord,   propPap,    offsetof(PAP, xaFirst),       // ipropFirstInd
		 actnWord,   propSep,    offsetof(SEP, cCols),         // ipropCols
		 actnWord,   propSep,    offsetof(SEP, xaPgn),         // ipropPgnX
		 actnWord,   propSep,    offsetof(SEP, yaPgn),         // ipropPgnY
		 actnWord,   propDop,    offsetof(DOP, xaPage),        // ipropXaPage
		 actnWord,   propDop,    offsetof(DOP, yaPage),        // ipropYaPage
		 actnWord,   propDop,    offsetof(DOP, xaLeft),        // ipropXaLeft
		 actnWord,   propDop,    offsetof(DOP, xaRight),       // ipropXaRight
		 actnWord,   propDop,    offsetof(DOP, yaTop),         // ipropYaTop
		 actnWord,   propDop,    offsetof(DOP, yaBottom),      // ipropYaBottom
		 actnWord,   propDop,    offsetof(DOP, pgnStart),      // ipropPgnStart
		 actnByte,   propSep,    offsetof(SEP, sbk),           // ipropSbk
		 actnByte,   propSep,    offsetof(SEP, pgnFormat),     // ipropPgnFormat
		 actnByte,   propDop,    offsetof(DOP, fFacingp),      // ipropFacingp
		 actnByte,   propDop,    offsetof(DOP, fLandscape),    // ipropLandscape
		 actnWord,   propPap,    offsetof(PAP, just),          // ipropJust
		 actnSpec,   propPap,    0,                            // ipropPard
		 actnSpec,   propChp,    0,                            // ipropPlain
		 actnSpec,   propSep,    0,                            // ipropSectd
		 actnSpec,   propSep,    0,                            // ipropPar
		 actnSpec,   propSep,    0,                            // ipropTrowd
		 actnSpec,   propSep,    0,                            // ipropTcelld
		 actnSpec,   propSep,    0,                            // ipropSect
		 actnSpec,   propSep,    0,                            // ipropRow
		 actnSpec,   propSep,    0,                            // ipropCell
		 actnWord,   propFnt,    offsetof(FONT, charset),      // ipropFcharset
		 actnWord,   propFnt,    offsetof(FONT, fprq),         // ipropFprq
		 actnByte,   propFnt,    offsetof(FONT, ftype),        // ipropFtype
		 actnSpec,   propFnt,    0,                            // ipropFnum
		 actnWord,   propCol,    offsetof(COLOR, red),         // ipropCred
		 actnWord,   propCol,    offsetof(COLOR, green),       // ipropCgreen
		 actnWord,   propCol,    offsetof(COLOR, blue),        // ipropCblue
		 actnByte,   propFnt,    offsetof(FONT,  ffam),        // ipropFfam
		 actnSpec,   propPap,    0,                            // ipropStyle
		 actnSpec,   propSep,    0,                            // ipropDStyle
		 actnWord,   propDate,   offsetof(DATE, year),         // ipropYear
		 actnWord,   propDate,   offsetof(DATE, month),        // ipropMonth
		 actnWord,   propDate,   offsetof(DATE, day),          // ipropDay
		 actnWord,   propDate,   offsetof(DATE, hour),         // ipropHour
		 actnWord,   propDate,   offsetof(DATE, min),          // ipropMin
		 actnWord,   propDate,   offsetof(DATE, sec),          // ipropSec
		 actnWord,   propPict,   offsetof(PICT, type),         // ipropPicttype
		 actnSpec,   propPict,   0,                            // ipropOmf
		 actnSpec,   propPict,   0,                            // ipropWmf
		 actnSpec,   propPict,   0,                            // ipropIbitmap
		 actnSpec,   propPict,   0,                            // ipropDbitmap
		 actnLong,   propPict,   offsetof(PICT, w),            // ipropPictw
		 actnLong,   propPict,   offsetof(PICT, h),            // ipropPicth
		 actnLong,   propPict,   offsetof(PICT, goalw),        // ipropPictwgoal
		 actnLong,   propPict,   offsetof(PICT, goalh),        // ipropPicthgoal
		 actnWord,   propPict,   offsetof(PICT, scalex),       // ipropPictscalex
		 actnWord,   propPict,   offsetof(PICT, scaley),       // ipropPictscaley
		 actnByte,   propPict,   offsetof(PICT, scaled),       // ipropPictscaled
		 actnSpec,   propPap,    0,                            // ipropUd
		 actnWord,   propDop,    offsetof(DOP, version),       // ipropVersion
		 actnWord,   propDop,    offsetof(DOP, npages),        // ipropNofpages
		 actnWord,   propDop,    offsetof(DOP, nwords),        // ipropNofword
		 actnWord,   propDop,    offsetof(DOP, nchars),        // ipropNofchars
		 actnWord,   propDop,    offsetof(DOP, ncharsws),      // ipropNofcharsws
		 actnWord,   propDop,    offsetof(DOP, id),            // ipropId
		 actnWord,   propChp,    offsetof(CHP, size),          // ipropFsize
		 actnWord,   propChp,    offsetof(CHP, fcolor),        // ipropFfcolor
		 actnWord,   propChp,    offsetof(CHP, bcolor),        // ipropFbcolor
		 actnWord,   propTrp,    offsetof(TRP, just),          // ipropRowjust
		 actnByte,   propTrp,    offsetof(TRP, direction),     // ipropRowwrite
		 actnSpec,   propTrp,    offsetof(TRP, trgaph),        // ipropRowgaph
		 actnSpec,   propTrp,    offsetof(TRP, cellx),         // ipropCellx
		 actnByte,   propTcp,    offsetof(TCP, clmgf),         // ipropClmgf
		 actnByte,   propTcp,    offsetof(TCP, clmrg),         // ipropClmrg
		 actnWord,   propTrp,    offsetof(TRP, trleft),        // ipropTrleft
		 actnWord,   propTrp,    offsetof(TRP, trrh),          // ipropTrrh
		 actnByte,   propTrp,    offsetof(TRP, header),        // ipropTrhdr
		 actnByte,   propTrp,    offsetof(TRP, keep),          // ipropTrkeep
		 actnByte,   propTrp,    offsetof(TRP, bordT),         // ipropTrbdrt
		 actnByte,   propTrp,    offsetof(TRP, bordB),         // ipropTrbdrb
		 actnByte,   propTrp,    offsetof(TRP, bordL),         // ipropTrbdrl
		 actnByte,   propTrp,    offsetof(TRP, bordR),         // ipropTrbdrr
		 actnByte,   propTrp,    offsetof(TRP, bordH),         // ipropTrbdrh
		 actnByte,   propTrp,    offsetof(TRP, bordV),         // ipropTrbdrv
		 actnByte,   propTcp,    offsetof(TCP, bordT),         // ipropClbrdrt
		 actnByte,   propTcp,    offsetof(TCP, bordB),         // ipropClbrdrb
		 actnByte,   propTcp,    offsetof(TCP, bordL),         // ipropClbrdrl
		 actnByte,   propTcp,    offsetof(TCP, bordR),         // ipropClbrdrr
		 actnWord,   propTcp,    offsetof(TCP, alignment),     // ipropCellaligm
		 actnWord,   propTcp,    offsetof(TCP, shading),       // ipropCellshade
		 actnWord,   propTcp,    offsetof(TCP, line_color),    // ipropCelllinecol
		 actnWord,   propTcp,    offsetof(TCP, back_color),    // ipropCellbackcol
		 actnWord,   propTcp,    offsetof(TCP, pattern),       // ipropCellpat

};

// Keyword descriptions
SYM rgsymRtf[] = {
//   keyword       dflt       fPassDflt   kwd              idx
		 "b",          1,         fFalse,     kwdProp,         ipropBold,
		 "clbgbdiag",  patBD,     fTrue,      kwdProp,         ipropCellpat,
		 "clbgcross",  patC,      fTrue,      kwdProp,         ipropCellpat,
		 "clbgdcross", patCD,     fTrue,      kwdProp,         ipropCellpat,
		 "clbgdkbdiag",patDBD,    fTrue,      kwdProp,         ipropCellpat,
		 "clbgdkcross",patDC,     fTrue,      kwdProp,         ipropCellpat,
		 "clbgdkdcross",patDCD,   fTrue,      kwdProp,         ipropCellpat,
		 "clbgdkfdiag",patDFD,    fTrue,      kwdProp,         ipropCellpat,
		 "clbgdkhor",  patDH,     fTrue,      kwdProp,         ipropCellpat,
		 "clbgdkvert", patDV,     fTrue,      kwdProp,         ipropCellpat,
		 "clbgfdiag",  patFD,     fTrue,      kwdProp,         ipropCellpat,
		 "clbghoriz",  patH,      fTrue,      kwdProp,         ipropCellpat,
		 "clbgvert",   patV,      fTrue,      kwdProp,         ipropCellpat,
		 "clbrdrb",    1,         fTrue,      kwdProp,         ipropClbrdrb,
		 "clbrdrl",    1,         fTrue,      kwdProp,         ipropClbrdrl,
		 "clbrdrr",    1,         fTrue,      kwdProp,         ipropClbrdrr,
		 "clbrdrt",    1,         fTrue,      kwdProp,         ipropClbrdrt,
		 "cltxlrtb",   aligmVL,   fTrue,      kwdProp,         ipropCellaligm,
		 "cltxtbrl",   aligmVR,   fTrue,      kwdProp,         ipropCellaligm,
		 "clvertalb",  aligmB,    fTrue,      kwdProp,         ipropCellaligm,
		 "clvertalc",  aligmC,    fTrue,      kwdProp,         ipropCellaligm,
		 "clvertalt",  aligmT,    fTrue,      kwdProp,         ipropCellaligm,
		 "ds",         0,         fFalse,     kwdProp,         ipropDStyle,
		 "i",          1,         fFalse,     kwdProp,         ipropItalic,
		 "s",          0,         fFalse,     kwdProp,         ipropStyle,
		 "trgaph",     0,         fFalse,     kwdProp,         ipropRowgaph,
		 "trql",       justL,     fTrue,      kwdProp,         ipropRowjust,
		 "ud",         0,         fFalse,     kwdProp,         ipropUd,
		 "ul",         1,         fFalse,     kwdProp,         ipropUnderline,
		 "upr",        0,         fFalse,     kwdDest,         idestSkip,
	   "'",          0,         fFalse,     kwdSpec,         ipfnHex,
	   "*",          0,         fFalse,     kwdSpec,         ipfnSkipDest,
	   "\0x0a",      0,         fFalse,     kwdChar,         0x0a,
	   "\0x0d",      0,         fFalse,     kwdChar,         0x0a,
	   "\\",         0,         fFalse,     kwdChar,         '\\',
	   "author",     0,         fFalse,     kwdDest,         idestAuthor,
	   "bin",        0,         fFalse,     kwdSpec,         ipfnBin,
	   "blue",       0,         fFalse,     kwdProp,         ipropCblue,
	   "buptim",     0,         fFalse,     kwdDest,         idestBuptim,
	   "category",   0,         fFalse,     kwdDest,         idestCategory,
	   "cb",         0,         fFalse,     kwdProp,         ipropFbcolor,
	   "cell",       0,         fFalse,     kwdProp,         ipropCell,
	   "cellx",      0,         fFalse,     kwdProp,         ipropCellx,
	   "cf",         0,         fFalse,     kwdProp,         ipropFfcolor,
	   "clcbpat",    0,         fFalse,     kwdProp,         ipropCellbackcol,
	   "clcfpat",    0,         fFalse,     kwdProp,         ipropCelllinecol,
	   "clmgf",      1,         fTrue,      kwdProp,         ipropClmgf,
	   "clmrg",      1,         fTrue,      kwdProp,         ipropClmrg,
	   "clshdng",    0,         fFalse,     kwdProp,         ipropCellshade,
	   "colortbl",   0,         fFalse,     kwdDest,         idestCol,
	   "cols",       1,         fFalse,     kwdProp,         ipropCols,
	   "comment",    0,         fFalse,     kwdDest,         idestComment,
	   "company",    0,         fFalse,     kwdDest,         idestCompany,
	   "creatim",    0,         fFalse,     kwdDest,         idestCreatim,
	   "dibitmap",   0,         fFalse,     kwdProp,         ipropIbitmap,
	   "doccomm",    0,         fFalse,     kwdDest,         idestDoccomm,
	   "dy",         0,         fFalse,     kwdProp,         ipropDay,
	   "emfblip",    pict_emf,  fTrue,      kwdProp,         ipropPicttype,
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
	   "footnote",   0,         fFalse,     kwdDest,         idestFootnote,
	   "fprq",       0,         fFalse,     kwdProp,         ipropFprq,
	   "froman",     froman,    fTrue,      kwdProp,         ipropFfam,
	   "fs",         0,         fFalse,     kwdProp,         ipropFsize,
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
	   "hlinkbase",  0,         fFalse,     kwdDest,         idestHlinkbase,
	   "hr",         0,         fFalse,     kwdProp,         ipropHour,
	   "id",         0,         fFalse,     kwdProp,         ipropId,
	   "info",       0,         fFalse,     kwdDest,         idestInfo,
	   "jpegblip",   pict_jpg,  fTrue,      kwdProp,         ipropPicttype,
	   "keywords",   0,         fFalse,     kwdDest,         idestKeywords,
	   "landscape",  1,         fTrue,      kwdProp,         ipropLandscape,
	   "ldblquote",  0,         fFalse,     kwdChar,         '"',
	   "li",         0,         fFalse,     kwdProp,         ipropLeftInd,
	   "ltlrow",     fFalse,    fTrue,      kwdProp,         ipropRowwrite,
	   "macpict",    pict_mac,  fTrue,      kwdProp,         ipropPicttype,
	   "margb",      1440,      fFalse,     kwdProp,         ipropYaBottom,
	   "margl",      1800,      fFalse,     kwdProp,         ipropXaLeft,
	   "margr",      1800,      fFalse,     kwdProp,         ipropXaRight,
	   "margt",      1440,      fFalse,     kwdProp,         ipropYaTop,
	   "min",        0,         fFalse,     kwdProp,         ipropMin,
	   "mo",         0,         fFalse,     kwdProp,         ipropMonth,
	   "nofchars",   0,         fFalse,     kwdProp,         ipropNofchars,
	   "nofcharsws", 0,         fFalse,     kwdProp,         ipropNofcharsws,
	   "nofpages",   0,         fFalse,     kwdProp,         ipropNofpages,
	   "nofwords",   0,         fFalse,     kwdProp,         ipropNofword,
	   "nonshppict", 0,         fFalse,     kwdDest,         idestSkip,
	   "operator",   0,         fFalse,     kwdDest,         idestOperator,
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
	   "pich",       0,         fFalse,     kwdProp,         ipropPich,
	   "pichgoal",   0,         fFalse,     kwdProp,         ipropPichgoal,
	   "picscaled",  0,         fFalse,     kwdProp,         ipropPicscaled,
	   "picscalex",  0,         fFalse,     kwdProp,         ipropPicscalex,
	   "picscaley",  0,         fFalse,     kwdProp,         ipropPicscaley,
	   "pict",       0,         fFalse,     kwdDest,         idestPict,
	   "picw",       0,         fFalse,     kwdProp,         ipropPicw,
	   "picwgoal",   0,         fFalse,     kwdProp,         ipropPicwgoal,
	   "pmmetafile", 0,         fFalse,     kwdProp,         ipropOmf,
	   "pngblip",    pict_png,  fTrue,      kwdProp,         ipropPicttype,
	   "printim",    0,         fFalse,     kwdDest,         idestPrintim,
	   "private1",   0,         fFalse,     kwdDest,         idestSkip,
	   "qc",         justC,     fTrue,      kwdProp,         ipropJust,
	   "qj",         justF,     fTrue,      kwdProp,         ipropJust,
	   "ql",         justL,     fTrue,      kwdProp,         ipropJust,
	   "qr",         justR,     fTrue,      kwdProp,         ipropJust,
	   "rdblquote",  0,         fFalse,     kwdChar,         '"',
	   "red",        0,         fFalse,     kwdProp,         ipropCred,
	   "revtim",     0,         fFalse,     kwdDest,         idestRevtim,
	   "ri",         0,         fFalse,     kwdProp,         ipropRightInd,
	   "row",        0,         fFalse,     kwdProp,         ipropRow,
	   "rtlrow",     fTrue,     fTrue,      kwdProp,         ipropRowwrite,
	   "rxe",        0,         fFalse,     kwdDest,         idestSkip,
	   "sbkcol",     sbkCol,    fTrue,      kwdProp,         ipropSbk,
	   "sbkeven",    sbkEvn,    fTrue,      kwdProp,         ipropSbk,
	   "sbknone",    sbkNon,    fTrue,      kwdProp,         ipropSbk,
	   "sbkodd",     sbkOdd,    fTrue,      kwdProp,         ipropSbk,
	   "sbkpage",    sbkPg,     fTrue,      kwdProp,         ipropSbk,
	   "sec",        0,         fFalse,     kwdProp,         ipropSec,
	   "sect",       0,         fFalse,     kwdProp,         ipropSect,
	   "shpinst",    0,         fFalse,     kwdDest,         idestShppict,
	   "shppict",    0,         fFalse,     kwdDest,         idestShppict,
	   "stylesheet", 0,         fFalse,     kwdDest,         idestStyle,
	   "subject",    0,         fFalse,     kwdDest,         idestSubject,
	   "tab",        0,         fFalse,     kwdChar,         0x09,
	   "tc",         0,         fFalse,     kwdDest,         idestSkip,
	   "tcelld",     0,         fFalse,     kwdProp,         ipropTcelld,
	   "title",      0,         fFalse,     kwdDest,         idestTitle,
	   "trbrdrb",    1,         fTrue,      kwdProp,         ipropTrbrdrb,
	   "trbrdrh",    1,         fTrue,      kwdProp,         ipropTrbrdrh,
	   "trbrdrl",    1,         fTrue,      kwdProp,         ipropTrbrdrl,
	   "trbrdrr",    1,         fTrue,      kwdProp,         ipropTrbrdrr,
	   "trbrdrt",    1,         fTrue,      kwdProp,         ipropTrbrdrt,
	   "trbrdrv",    1,         fTrue,      kwdProp,         ipropTrbrdrv,
	   "trhdr",      1,         fTrue,      kwdProp,         ipropTrhdr,
	   "trkeep",     1,         fTrue,      kwdProp,         ipropTrkeep,
	   "trleft",     0,         fFalse,     kwdProp,         ipropTrleft,
	   "trowd",      0,         fFalse,     kwdProp,         ipropTrowd,
	   "trqc",       justC,     fTrue,      kwdProp,         ipropRowjust,
	   "trqr",       justR,     fTrue,      kwdProp,         ipropRowjust,
	   "trrh",       0,         fFalse,     kwdProp,         ipropTrrh,
	   "txe",        0,         fFalse,     kwdDest,         idestSkip,
	   "u",          0,         fFalse,     kwdUTF,          0,
	   "version",    0,         fFalse,     kwdProp,         ipropVersion,
	   "wbitmap",    0,         fFalse,     kwdProp,         ipropDbitmap,
	   "wmetafile",  0,         fFalse,     kwdProp,         ipropWmf,
	   "xe",         0,         fFalse,     kwdDest,         idestSkip,
	   "yr",         0,         fFalse,     kwdProp,         ipropYear,
	   "{",          0,         fFalse,     kwdChar,         '{',
	   "}",          0,         fFalse,     kwdChar,         '}',
	   "list",       0,         fFalse,     kwdDest,         idestSkip,
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

PICT pict;

rprop_t *prop;
rnotify_t *no;

// STYLESHEET
STYLE stylesheet[256];
int nstyles;

// INFO
char info[BUFSIZ] = {0};
int  linfo = 0;
INFO_T tinfo;

// DATE
DATE date;
DATE_T tdate;

// RTF parser declarations
int ecPushRtfState(void);
int ecPopRtfState(void);
int ecParseRtfKeyword(FILE *fp);
int ecParseChar(int c);
int ecParseUTF(int c);
int ecTranslateKeyword(char *szKeyword, int param, bool fParam);
int ecPrintChar(int ch);
int ecEndGroupAction(RDS rds);
int ecApplyPropChange(IPROP iprop, long val);
int ecChangeDest(IDEST idest);
int ecParseSpecialKeyword(IPFN ipfn);
int ecParseSpecialProperty(IPROP iprop, int val);
int ecParseHexByte(void);


int isymMax = sizeof(rgsymRtf) / sizeof(SYM);

struct str img;

//
// %%Function: ecApplyPropChange
//
// Set the property identified by _iprop_ to the value _val_.
//
//
int
ecApplyPropChange(IPROP iprop, long val)
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
		case propDate:
			pb = (char *)&date;
			break;
		case propPict:
			pb = (char *)&pict;
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
		case actnLong:
			(*(long *) (pb+rgprop[iprop].offset)) = lParam;
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
		
		case ipropOmf:
			pict.type = pict_omf;
			pict.type_n = val;
			return ecOK;
		case ipropWmf:
			pict.type = pict_wmf;
			pict.type_n = val;
			return ecOK;
		case ipropIbitmap:
			pict.type = pict_ibitmap;
			pict.type_n = val;
			return ecOK;
		case ipropDbitmap:
			pict.type = pict_dbitmap;
			pict.type_n = val;
			return ecOK;
		
		case ipropUd:  // we can read utf (use \ud and skip \udr)
			return ecOK;
		
		case ipropFnum:
			if (rds == rdsFonttbl)
				fnt.num = val;
			else
				prop->chp.font = val;
			return ecOK;

		case ipropRowgaph:
			prop->trp.trgaph[prop->trp.ntrgaph++] = val;
		
		case ipropCellx:
			prop->trp.cellx[prop->trp.ncellx++] = val;
		
		case ipropPar:
			if (no->par_cb)
				no->par_cb(no->udata, &prop->pap);
			//ecPrintChar(0x0a); // print new line
			return ecOK;
		
		case ipropSect:
			if (no->sect_cb)
				no->sect_cb(no->udata, &prop->sep);
			return ecOK;
		
		case ipropRow:
			if (no->row_cb)
				no->row_cb(no->udata, &prop->trp);
			return ecOK;
		
		case ipropCell:
			if (no->cell_cb)
				no->cell_cb(no->udata, &prop->trp, &prop->tcp);
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
			rds = rdsInfo;
			break;
		
		case idestFootnote:
			rds = rdsFootnote;
			if (no->foot_cb)
				no->foot_cb(no->udata, 1);
			break;
		
		case idestTitle:
			info[0] = 0;
			linfo = 0;
			tinfo = info_titile;
			rds = rdsInfoString;
			break;
		
		case idestSubject:
			info[0] = 0;
			linfo = 0;
			tinfo = info_subject;
			rds = rdsInfoString;
			break;
		
		case idestAuthor:
			info[0] = 0;
			linfo = 0;
			tinfo = info_author;
			rds = rdsInfoString;
			break;
		
		case idestManger:
			info[0] = 0;
			linfo = 0;
			tinfo = info_manager;
			rds = rdsInfoString;
			break;
		
		case idestCompany:
			info[0] = 0;
			linfo = 0;
			tinfo = info_company;
			rds = rdsInfoString;
			break;
		
		case idestOperator:
			info[0] = 0;
			linfo = 0;
			tinfo = info_operator;
			rds = rdsInfoString;
			break;
		
		case idestCategory:
			info[0] = 0;
			linfo = 0;
			tinfo = info_category;
			rds = rdsInfoString;
			break;
		
		case idestKeywords:
			info[0] = 0;
			linfo = 0;
			tinfo = info_keywords;
			rds = rdsInfoString;
			break;
		
		case idestComment:
			info[0] = 0;
			linfo = 0;
			tinfo = info_comment;
			rds = rdsInfoString;
			break;
		
		case idestDoccomm:
			info[0] = 0;
			linfo = 0;
			tinfo = info_doccomm;
			rds = rdsInfoString;
			break;
		
		case idestHlinkbase:
			info[0] = 0;
			linfo = 0;
			tinfo = info_hlinkbase;
			rds = rdsInfoString;
			break;
		
		case idestCreatim:
			memset(&date, 0, sizeof(DATE));
			tdate = date_create;
			rds = rdsInfoDate;
			break;
		
		case idestRevtim:
			memset(&date, 0, sizeof(DATE));
			tdate = date_revision;
			rds = rdsInfoDate;
			break;
		
		case idestPrintim:
			memset(&date, 0, sizeof(DATE));
			tdate = date_print;
			rds = rdsInfoDate;
			break;
		
		case idestBuptim:
			memset(&date, 0, sizeof(DATE));
			tdate = date_backup;
			rds = rdsInfoDate;
			break;
		
		case idestShppict:
			rds = rdsShppict;
			break;
		
		case idestPict:
			{
				memset(&pict, 0, sizeof(PICT));
				// try to allocate memory
				if (str_init(&img, 4194304))
					rds = rdsSkip;
				else
					rds = rdsPict;
			}
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
		// parse picture
		if (img.str){
			// convert image hex string to binary
			pict.len = img.len/2;
			pict.data = 
				(unsigned char*)malloc(pict.len);
			if (!pict.data) // not enough memory
				return ecStackOverflow;
			char cur[3];
			unsigned int val;
			size_t i, l;
			for (i = 0, l = 0; i < img.len;) {
				cur[0] = img.str[i++];
				cur[1] = img.str[i++];
				cur[2] = 0;
				sscanf(cur, "%x", &val);
				pict.data[l++] = (unsigned char)val;
			}
			// do callback
			if (no->pict_cb)
				no->pict_cb(no->udata, &pict, &prop->pap);

			free(img.str);
			free(pict.data);
		}
		return ecOK;
	}
	if (rds == rdsInfoString){
		info[linfo] = 0;
		if (*info)
			if (no->info_cb)
				no->info_cb(no->udata, tinfo, info);
		return ecOK;
	}
	if (rds == rdsInfoDate){
		if (no->date_cb)
			no->date_cb(no->udata, tdate, &date);
		return ecOK;
	}

	if (rds == rdsFootnote){
		if (no->foot_cb)
			no->foot_cb(no->udata, 0);
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
	long param = 0;
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

	if (no->command_cb)
		no->command_cb(no->udata, szKeyword, param, fParam);
	
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
ecAddInfoString(int ch)
{
	if (linfo < sizeof(info))
		info[linfo++] = ch;
	return ecOK;
}

int
ecAddPicture(int ch)
{
	// add only if hex
	if (ch == 'a' || ch == 'A' ||
			ch == 'b' || ch == 'B' ||
			ch == 'c' || ch == 'C' ||
			ch == 'd' || ch == 'D' ||
			ch == 'e' || ch == 'E' ||
			ch == 'f' || ch == 'F' ||
			isdigit(ch))
	{
		char c = ch;
		str_append(&img, &c, 1);
	}
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
		if (stylesheet[nstyles].lname < sizeof(stylesheet[nstyles].name))
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
		
		case rdsInfoString:
			return ecAddInfoString(ch);
		
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
		no->char_cb(no->udata, ch, &prop->chp);
	return ecOK;
}
