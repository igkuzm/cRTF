/**
 * File              : test.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 18.01.2024
 * Last Modified Date: 20.01.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include <stdio.h>
#include "rtfreadr.h"
#include "rtftype.h"
#include <string.h>

int info_cb(void *d, INFO_T t, const char *s)
{
	printf("%d: %s\n", t, s);
	return 0;
}

int date_cb(void *data, DATE_T t, DATE *d)
{
	printf("%d: %d.%d.%d %d:%d:%d\n", t, d->day, d->month, d->year, d->hour, d->min, d->sec);
	return 0;
}

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

int pict_cb(void *d, PICT *p)
{
	printf("TYPE: %d\n", p->type);
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
	memset(&(n), 0, sizeof(rnotify_t));

	//n.font_cb = font_cb;
	//n.char_cb = char_cb;
	//n.par_cb = par_cb;
	//n.style_cb = style_cb;
	//n.par_cb = par_cb;
	n.pict_cb = pict_cb;
	n.info_cb = info_cb;
	n.date_cb = date_cb;

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

	return 0;
}


