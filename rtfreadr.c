/**
 * File              : rtfreadr.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 17.01.2024
 * Last Modified Date: 18.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "rtftype.h"
#include "rtfdecl.h"
#include "utf.h"
int cGroup;
bool fSkipDestIfUnk;
bool isUTF;
long cbBin;
long lParam;
RDS rds;
RIS ris;
CHP chp;
PAP pap;
SEP sep;
DOP dop;
SAVE *psave;
FILE *fpIn;
//
// %%Function: main
//
// Main loop. Initialize and parse RTF.
//
int main(int argc, char *argv[])
{
		 FILE *fp;
		 int ec;
		 if (argc < 2)
						printf ("Usage: %s filename\n", argv[0]);

		 fp = fpIn = fopen(argv[1], "r");
		 if (!fp)
		 {
						printf ("Can't open test file!\n");
						return 1;
		 }
		 if ((ec = ecRtfParse(fp)) != ecOK)
						printf("error %d parsing rtf\n", ec);
		 else
						printf("Parsed RTF file OK\n");
			fclose(fp);
			return 0;
}
//
// %%Function: ecRtfParse
//
// Step 1:
// Isolate RTF keywords and send them to ecParseRtfKeyword;
// Push and pop state at the start and end of RTF groups;
// Send text to ecParseChar for further processing.
//
int
ecRtfParse(FILE *fp)
{
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
							/* TODO: CODEPAGE */
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
	psaveNew -> chp = chp;
	psaveNew -> pap = pap;
	psaveNew -> sep = sep;
	psaveNew -> dop = dop;
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
	chp = psave->chp;
	pap = psave->pap;
	sep = psave->sep;
	dop = psave->dop;
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
	// unfortunately, we don't do a whole lot here as far as layout goes...
	putchar(ch);
	return ecOK;
}

