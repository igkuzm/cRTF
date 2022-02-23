/**
 * File              : cRTF.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 06.09.2021
 * Last Modified Date: 23.02.2022
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#ifndef cRTF_h__
#define cRTF_h__
#endif

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stdbool.h>

typedef struct c_rtf_table_t {
	char** titles;
	char** argv;
	int argc;
	struct c_rtf_table_t* next;
} cRTFTable;

typedef struct c_rft_rtf_t {
	char *text;
	int len;
} cRTF;

//convers C String (UFT8 codepage) to RTF unicode string (\u0000)
char *c_rtf_utf8_string_to_rtf_control_word(const char *string);

//parse RTF file to cRTF
int c_rtf_parse_rtf(const char *filename, cRTF *rtf);

//print RTF file to stdout
void c_rtf_print_rtf(const char *filename);

/* TABLES */
//new table
cRTFTable * c_rtf_table_new(int columns);

//free table
void c_rtf_table_free(cRTFTable* table);

void c_rtf_table_set_columns(cRTFTable* table, int columns);

//append table with strings. c_rtf_table_append_row(table, str0, str1, str2) 
void c_rtf_table_append_row(cRTFTable* table, ...);

//set titles for table. c_rtf_table_set_titles(table, title0, title1, title2)
void c_rtf_table_set_titles(cRTFTable* table, ...);

//get number of rows
int c_rtf_table_size(cRTFTable *table);

//run callback foreach row
int c_rtf_table_foreach_row(cRTFTable* table, void* user_data, int (*callback)(int argc, char** argv, void* user_data));

//struct to present table with rows, columns
typedef struct{
	char ***rows_val;
	char **column_names;
	int column_num;
	int rows_num;
} RTFtable;

//create table
RTFtable *create_RTFtable(char ***rows_val, char **column_names, int column_num, int rows_num);

//make RTF-style string from table
char *RTFstringFromRTFtable(RTFtable *table, bool withBorders);

#ifdef __cplusplus
}
#endif


