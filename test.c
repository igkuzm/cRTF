/**
 * File              : test.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 18.01.2024
 * Last Modified Date: 19.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include <stdio.h>
#include "rtfreadr.h"

int style_cb(void *d, STYLE *f)
{
	printf("%d. %s\n", f->s, f->name);
	return 0;
}

int font_cb(void *d, FONT *f)
{
	printf("%d. %s\n", f->num, f->name);
	return 0;
}

int char_cb(void *d, int ch)
{
	putchar(ch);
	return 0;
}

int par_cb(void *d)
{
	printf("par\n");
	return 0;
}

//
// %%Function: main
//
// Main loop. Initialize and parse RTF.
//
int main(int argc, char *argv[])
{
	FILE *fp;
	int ec;

	rprop_t p;
	rnotify_t n;
	//n.font_cb = font_cb;
	//n.char_cb = char_cb;
	//n.par_cb = par_cb;
	//n.style_cb = style_cb;
	n.par_cb = par_cb;

	if (argc < 2)
		printf ("Usage: %s filename\n", argv[0]);

	fp = fopen(argv[1], "r");
	if (!fp)
	{
		printf ("Can't open test file!\n");
		return 1;
	}
	
	if ((ec = ecRtfParse(fp, &p, &n)) != ecOK)
		printf("error %d parsing rtf\n", ec);
	else
		printf("Parsed RTF file OK\n");
	fclose(fp);

	printf("Author: %s\n", p.dop.info->author);
	printf("Title: %s\n", p.dop.info->title);
	
	return 0;
}


