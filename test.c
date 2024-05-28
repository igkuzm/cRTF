/**
 * File              : test.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 18.01.2024
 * Last Modified Date: 28.05.2024
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include <stdio.h>
#include "rtfreadr.h"
#include "mswordtype.h"
#include <string.h>
#include "str.h"

struct str str;

int info_cb(void *d, tINFO t, const char *s)
{
	printf("%d: %s\n", t, s);
	return 0;
}

int date_cb(void *data, tDATE t, DATE *d)
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

int char_cb(void *d, STREAM s, prop_t *p, int ch)
{
	putchar(ch);
	char c = ch;
	str_append(&str, &c, 1);
	return 0;
}

int pict_cb(void *d, prop_t *p, PICT *pict)
{
	printf("TYPE: %d\n", pict->type);
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

	prop_t p;
	rnotify_t n;
	memset(&(n), 0, sizeof(rnotify_t));

	str_init(&str, BUFSIZ);

	n.font_cb = font_cb;
	n.char_cb = char_cb;
	n.style_cb = style_cb;
	n.char_cb = char_cb;
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


