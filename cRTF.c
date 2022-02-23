/**
 * File              : cRTF.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 06.09.2021
 * Last Modified Date: 23.02.2022
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include "cRTF.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>

//convert int to binary char array. return number of chars
int 
c_rft_int_to_binary_char_array(char **binary, int n){
	int i=0;
	int a[30];
	for (i = 0; n > 0; ++i) {
		a[i]=n%2;
		n=n/2;
	}
	char *bin=calloc(30, sizeof(char));
	if (bin == NULL) {
		perror("string bin calloc");
		exit(EXIT_FAILURE);
	}
	int k=0;
	for (i = i-1; i >=0; i--) {
		bin[k]=(a[i]==0) ? '0' : '1';	
		k++;
	}
	bin[k]='\0';
	*binary=bin;

	return k;
}

//convert char to string of bytes (size = 8).
int 
c_rtf_utf8_char_to_byte_array(char c, char *s) {
	for (int i = 7; i >= 0; --i) {
	   s[7-i] = (c & (1 << i)) ? '1' : '0';
	}
	return 7;
}

char*
c_rtf_utf8_string_to_rtf_control_word(const char *string){
	char *control_word=malloc((strlen(string)*6 + 10)*sizeof(char));
	if (control_word == NULL) {
		perror("utfstring malloc");
		exit(EXIT_FAILURE);
	}
	sprintf(control_word, "\\uc0 "); //add \\uc0 to string
	for (int i = 0; string[i]!='\0'; ++i) {
		char byte[33];
		int byte_len=0;
		unsigned char a = string[i];
		if ((a & 0b11110000) == 240) {
			//4-byte - 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			char byte0[8]; c_rtf_utf8_char_to_byte_array(string[i], byte0); i++;
			char byte1[8]; c_rtf_utf8_char_to_byte_array(string[i], byte1); i++;
			char byte2[8]; c_rtf_utf8_char_to_byte_array(string[i], byte2); i++;
			char byte3[8]; c_rtf_utf8_char_to_byte_array(string[i], byte3);						
			sprintf(byte, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 
				byte0[5], byte0[6], byte0[7], 
				byte1[2], byte1[3], byte1[4], byte1[5], byte1[6], byte1[7],		
				byte2[2], byte2[3], byte2[4], byte2[5], byte2[6], byte2[7],		
				byte3[2], byte3[3], byte3[4], byte3[5], byte3[6], byte3[7]
			);		
			byte_len=21;
		}
		else if ((a & 0b11100000) == 224) {
			//3byte - 1110xxxx 10xxxxxx 10xxxxxx
			char byte0[8]; c_rtf_utf8_char_to_byte_array(string[i], byte0); i++;
			char byte1[8]; c_rtf_utf8_char_to_byte_array(string[i], byte1); i++;
			char byte2[8]; c_rtf_utf8_char_to_byte_array(string[i], byte2);			
			sprintf(byte, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 
				byte0[4], byte0[5], byte0[6], byte0[7], 
				byte1[2], byte1[3], byte1[4], byte1[5], byte1[6], byte1[7],		
				byte2[2], byte2[3], byte2[4], byte2[5], byte2[6], byte2[7]
			);		
			byte_len=16;		
		}
		else if ((a & 0b11000000) == 192) {
			//2byte - 110xxxxx 10xxxxxx
			char byte0[8]; c_rtf_utf8_char_to_byte_array(string[i], byte0); i++;
			char byte1[8]; c_rtf_utf8_char_to_byte_array(string[i], byte1);
			sprintf(byte, "%c%c%c%c%c%c%c%c%c%c%c", 
				byte0[3], byte0[4], byte0[5], byte0[6], byte0[7], 
				byte1[2], byte1[3], byte1[4], byte1[5], byte1[6], byte1[7]
			);		
			byte_len=11;	
		}
		else {
			//1byte - 0xxxxxxx
			char byte0[8]; c_rtf_utf8_char_to_byte_array(string[i], byte0);
			sprintf(byte, "%c%c%c%c%c%c%c%c", 
				byte0[0],byte0[1], byte0[2], byte0[3], byte0[4], byte0[5], byte0[6], byte0[7]
			);			
			byte_len=8;
		}
		//generate string
		int k=0;
		for (int i = 0; i < byte_len; ++i) {
			int bit=byte[i]-'0';
			k=k+bit*pow(2,byte_len-1-i);
		};
		sprintf(control_word, "%s\\u%d ", control_word, k);
	}
	return control_word;
}

char*
c_rtf_utf8_char_from_rtf_decimal_code_string(char *decimal_code_string){
	
	//allocate utf8_char
	char* utf8_char = calloc(4, sizeof(char));
	if (utf8_char == NULL) {
		perror("utf8_char alloc");
		exit(EXIT_FAILURE);
	}		

	//get binary code from decimal code
	char *binary_char_array;
	int binary_count = c_rft_int_to_binary_char_array(&binary_char_array, atoi(decimal_code_string));
	char k; //char from binary 
	int l; //counter
	
	if (binary_count>11 && binary_count<=16){ //utf8 char with 3 ansi chars
		k=0;
		//3 byteChar
		int m=0;
		char char2[8], char1[8], char0[8];
		for (l = 0; l < binary_count; ++l) {
			char bit = (binary_char_array[binary_count-l]) ? binary_char_array[binary_count-l] : 0;
			if (l<6){
				char2[m]=bit;	
			}
			if (l>=6 && l<12){
				char1[m]=bit;								
			}							
			if (l>=12){
				char0[m]=bit;								
			}							
			m++;
		}	
		char0[0]=1;
		char0[1]=1;
		char0[2]=1;
		char0[3]=0;

		char1[0]=1;
		char1[1]=0;

		char2[0]=1;
		char2[1]=0;
		
		for (l = 0; l < 8; ++l) {
			int bit=char0[l]-'0';
			k=k+bit*pow(2,7-l);
		};						
		sprintf(utf8_char, "%s%c",utf8_char,k);
		
		for (l = 0; l < 8; ++l) {
			int bit=char1[l]-'0';
			k=k+bit*pow(2,7-l);
		};						
		sprintf(utf8_char, "%s%c",utf8_char,k);

		for (l = 0; l < 8; ++l) {
			int bit=char2[l]-'0';
			k=k+bit*pow(2,7-l);
		};						
		sprintf(utf8_char, "%s%c",utf8_char,k);
		
	}					

	if (binary_count==11){ //utf8 char with 2 ansi chars
		k=0;
		//2 byteChar
		char char1[8], char0[8];
		char0[0]=1;
		char0[1]=1;
		char0[2]=0;
		char0[3]=binary_char_array[0];
		char0[4]=binary_char_array[1];
		char0[5]=binary_char_array[2];
		char0[6]=binary_char_array[3];
		char0[7]=binary_char_array[4];
		
		for (l = 0; l < 8; ++l) {
			int bit=char0[l]-'0';
			k=k+bit*pow(2,7-l);
		};						
		sprintf(utf8_char, "%s%c",utf8_char,k);

		
		k=0;
		char1[0]=1;
		char1[1]=0;
		char1[2]=binary_char_array[5];
		char1[3]=binary_char_array[6];
		char1[4]=binary_char_array[7];
		char1[5]=binary_char_array[8];
		char1[6]=binary_char_array[9];
		char1[7]=binary_char_array[10];

		for (l = 0; l < 8; ++l) {
			int bit=char1[l]-'0';
			k=k+bit*pow(2,7-l);
		};						
		sprintf(utf8_char, "%s%c",utf8_char,k);
	}

	if (binary_count==8){ //utf8 char with ansi char
		k=0;
		//1 byteChar
		char char0[8];
		char0[0]=binary_char_array[0];
		char0[1]=binary_char_array[1];
		char0[2]=binary_char_array[2];
		char0[3]=binary_char_array[3];
		char0[4]=binary_char_array[4];
		char0[5]=binary_char_array[5];
		char0[6]=binary_char_array[6];
		char0[7]=binary_char_array[7];
		
		for (l = 0; l < 8; ++l) {
			int bit=char0[l]-'0';
			k=k+bit*pow(2,7-l);
		};						
		sprintf(utf8_char, "%s%c",utf8_char,k);
	}

	return utf8_char;
}

char*
c_rtf_utf8_char_from_rtf_control_word(char *control_word, int start_of_decimal_code, int control_word_len){
	
	//get decimal code from control_word
	char decimal_code[10];	
	for (int i = start_of_decimal_code; i < control_word_len; ++i) {
		sprintf(decimal_code, "%s%c", decimal_code, control_word[i]);
	}						

	return c_rtf_utf8_char_from_rtf_decimal_code_string(decimal_code);
}

void
c_rtf_parse_rtf_string(char *input, int input_len, char *output){
	int startOfWord;
	if (input[0]=='-'){ //some words starts with '-'
		if (input[1]=='\\'){
			if (input[2]=='u'){
				sprintf(output,"%s%c",output,'-');					
				startOfWord=3;
				char *UTFCHAR = c_rtf_utf8_char_from_rtf_control_word(input, startOfWord, input_len);
				sprintf(output,"%s%s",output,UTFCHAR);					
				free(UTFCHAR);
			}
		}
	}			
	if (input[0]=='('){ //some words starts with '('
		if (input[1]=='\\'){
			if (input[2]=='u'){
				sprintf(output,"%s%c",output,'(');												
				startOfWord=3;
				char *UTFCHAR = c_rtf_utf8_char_from_rtf_control_word(input, startOfWord, input_len);
				sprintf(output,"%s%s",output,UTFCHAR);					
				free(UTFCHAR);
			}
		}
	}			
	if (input[0]=='\\'){
		if (input[1]=='u'){
			if (input[2]=='c'){//\uc0 - start of text block 
				if (input[3] == '0') {
					startOfWord=6;
				}
			}					
			else {
				startOfWord=2;
			}
			char *UTFCHAR = c_rtf_utf8_char_from_rtf_control_word(input, startOfWord, input_len);
			sprintf(output,"%s%s",output,UTFCHAR);					
			free(UTFCHAR);
		}
		if (input[1]=='\''){ //some words starts with \'ab
			if (input[2] == 'a') {
				if (input[3] == 'b'){
					if (input[4] == '\\') {
						sprintf(output,"%s%c",output,'\'');													
						startOfWord=6;					
						char *UTFCHAR = c_rtf_utf8_char_from_rtf_control_word(input, startOfWord, input_len);
						sprintf(output,"%s%s",output,UTFCHAR);					
						free(UTFCHAR);								
					}
				}
			}
		}				
		if (input[1]=='\''){ //find \'bb to close '
			if (input[2] == 'b') {
				if (input[3] == 'b'){
					sprintf(output,"%s%c",output,'\'');										
				}	
			}				
		}				
	}
}

//the array of service RTF words 
int 
c_rtf_service_words_array(char ***array){
	char **serviceWordsArray = malloc(BUFSIZ * sizeof(char)); //allocate array
	if (serviceWordsArray == NULL) { //check if the array was allocated - otherwise exit carefuly
		perror("serviceWordsArray malloc");
		exit(EXIT_FAILURE);
	}
	
	serviceWordsArray[0] = "fonttbl";
	serviceWordsArray[1] = "colortbl";
	serviceWordsArray[2] = "datastore";
	serviceWordsArray[3] = "themedata";
	serviceWordsArray[4] = "stylesheet";
	serviceWordsArray[5] = "info";
	serviceWordsArray[6] = "picw";
	serviceWordsArray[7] = "pich";
	
	if (array == NULL) {
		errno = EPERM; //if there is no array to return - exit carefuly 	
		fprintf(stderr, "Error occured: %d (%s)\n", errno, strerror(errno));
		exit(errno);
	}
	*array = serviceWordsArray;
	return 8;
}

//function to check rtf word for service words
bool c_rtf_rtf_string_is_plain_text(char *word){
	char **array;
	int count = c_rtf_service_words_array(&array); 
	int i;
	for (i = 0; i < count; ++i) {
		if (strcmp(word, array[i])) { //if the word matches service word
			return false;
		}
	}
	return true;
}

void
c_rtf_alloc_text(cRTF *rtf)
{
	rtf->text=malloc(2*sizeof(char));
	if (rtf->text == NULL) { //stop program to do not damage data
		perror("rtf->text malloc");
		exit(EXIT_FAILURE);  
	}	
	rtf->len=2;
	sprintf(rtf->text, "");
}

void
c_rtf_realloc_text(cRTF *rtf, int plus_size)
{
	rtf->len += plus_size;
	rtf->text=realloc(rtf->text, rtf->len * sizeof(char));
	if (rtf->text == NULL) { //stop program to do not damage data
		perror("rtf->text realloc");
		exit(EXIT_FAILURE);  
	}				
}

int
c_rtf_parse_rtf(const char *filename, cRTF *rtf)
{
	FILE *fp;
	
	//safe open filename
	char _filename[BUFSIZ];
	strncpy(_filename, (const char*)filename, sizeof(_filename) - 1); //safe copy of string (no buffer overload)
	_filename[sizeof(_filename) - 1] = '\0';

	//open file for stream
    fp = fopen(_filename, "r");		
    if (fp == NULL) { 
		errno = ENOENT;
        fprintf(stderr, "Can't open file: %s. Error: %d (%s)\n", _filename, errno, strerror(errno)); 
		exit(errno);  
    } 		

	//allocate output text
	c_rtf_alloc_text(rtf);

	char read[BUFSIZ];
	int read_len=0;
    while (1) { 
        char ch = fgetc(fp); //get char from file stream 
        if ((int)ch == EOF) { 
			sprintf(rtf->text,"%s%c",rtf->text,'\n');					
			//check if it is end of file, or error
			if (!feof(fp)) { //if it is error
				if (ferror(fp)){
					fprintf(stderr, "Error while processing file: %s. Error: %d (%s)\n", _filename, errno, strerror(errno)); 
					exit(errno);  					
				}
			}
            break; 
        }
		if (ch==' ' || ch==13 || ch==10 || ch=='\t' || ch=='\n'|| ch=='\r'|| ch=='}' || ch=='{' ) {//find the end of the words
			
			if (read_len==0 || read_len==1){ //word is one char
				if (ch==' '|| ch==13 || ch==10 || ch=='\t' || ch=='\n'|| ch=='\r' || ch==','){ //print the char
					c_rtf_realloc_text(rtf, read_len);
					sprintf(rtf->text,"%s%c",rtf->text,ch);				
				}
			}			
			
			if (read[0] == '\\' || read[0] == '-' || read[0] == '(') {
				c_rtf_realloc_text(rtf, read_len);
				c_rtf_parse_rtf_string(read, read_len, rtf->text);
			}

			else {	//print word, if read[0] != '\\'
				int i;
				int stop = 0;
				for (i = 0; i < read_len; ++i) {
					if (read[i] != '\\' && !stop) { 
						if (c_rtf_rtf_string_is_plain_text(read)) {
							c_rtf_realloc_text(rtf, 1);
							sprintf(rtf->text,"%s%c",rtf->text,read[i]);					
						}
					}
					if (read[i] == '\\') { //find end of line
						if (i == read_len - 1) { //end of line
							sprintf(rtf->text,"%s%c",rtf->text,'\n');					
						}
						else {
							if (read[i+1] == 'u') {
								stop = 1;
								int k, new_read_len = 0;
								char new_read[BUFSIZ];
								for (k = i+2; k < read_len; k++) {
									new_read[k] = read[k+i+2];
									new_read_len++;
								}
								c_rtf_realloc_text(rtf, new_read_len);
								c_rtf_parse_rtf_string(new_read, new_read_len, rtf->text);
							}
						}
					}
				}
			}

			read_len=0;
			read[0]='\0';
		}
		else {
			read[read_len]=ch;
			read_len++;
		}
    } 
    fclose(fp); 	
	return 0;
}

void 
c_rtf_print_rtf(const char *filename)
{
	cRTF rtf;
	c_rtf_parse_rtf(filename, &rtf);
	printf("%s", rtf.text);
	free(rtf.text);
}

cRTFTable *
c_rtf_table_new(int columns) 
{
	cRTFTable* table = malloc(sizeof(cRTFTable));
	if (table == NULL) { //stop program to do not damage data
		perror("cRTFTable malloc");
		exit(EXIT_FAILURE);  
	}	
	table->argc = columns;	
	table->next = NULL;
	table->argv = NULL;
	table->titles = NULL;	
	return table;
}

void
c_rtf_table_free(cRTFTable* table)
{
	cRTFTable *ptr = table;
	while (ptr != NULL) {
		if (ptr->titles != NULL) {
			free(ptr->titles);
		}
		if (ptr->argv != NULL) {
			free(ptr->argv);
		}
		cRTFTable *next = ptr->next;
		free(ptr);
		ptr=NULL;
		ptr=next;
	}
}

void c_rtf_table_set_columns(cRTFTable* table, int columns){
	table->argc = columns;
}

void
c_rtf_table_fill_row(cRTFTable* table, const char* argv[])
{
	for (int i = 0; i < table->argc; ++i) {
		table->argv = malloc(BUFSIZ*table->argc*sizeof(char));
		if (table->argv == NULL) { //stop program to do not damage data
			perror("cRTFTable->argv malloc");
			exit(EXIT_FAILURE);  
		}		
		strncpy(table->argv[i], argv[i], BUFSIZ-1);
		table->argv[i][BUFSIZ - 1] = '\0';
	}
}

void
c_rtf_table_append_row_data(cRTFTable* table, const char* argv[])
{
	if (table->argv == NULL) {
		c_rtf_table_fill_row(table, argv);	
	} else {
		cRTFTable* last = table->next;
		while (last->next != NULL) {
			last = last->next;	
		}	
		cRTFTable* new = c_rtf_table_new(table->argc);
		c_rtf_table_fill_row(new, argv);
		last->next = new;
	}
}

void
c_rtf_table_append_row(cRTFTable* table, ...)
{
	va_list valist;
	va_start(valist, table);
	const char **argv = malloc(table->argc * BUFSIZ * sizeof(char));
	if (argv == NULL) {
		perror("argv malloc");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < table->argc; ++i) {
		char *arg = va_arg(valist, char*);
		argv[i] = arg;
	}
	va_end(valist);
	c_rtf_table_append_row_data(table, argv);
	free(argv);
}

void
c_rtf_table_set_titles_data(cRTFTable* table, const char* titles[])
{
	if (table->titles == NULL) {
		table->titles = malloc(BUFSIZ*table->argc*sizeof(char));
		if (table->titles == NULL) { //stop program to do not damage data
			perror("cRTFTable->column_titles malloc");
			exit(EXIT_FAILURE);  
		}	
		
		for (int i = 0; i < table->argc; ++i) {
			strncpy(table->titles[i], titles[i], BUFSIZ-1);
			table->titles[i][BUFSIZ - 1] = '\0';
		}	
	}
}

void
c_rtf_table_set_titles(cRTFTable* table, ...)
{
	va_list valist;
	va_start(valist, table);
	const char **argv = malloc(table->argc * BUFSIZ * sizeof(char));
	if (argv == NULL) {
		perror("argv malloc");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < table->argc; ++i) {
		char *arg = va_arg(valist, char*);
		argv[i] = arg;
	}
	va_end(valist);
	c_rtf_table_set_titles_data(table, argv);
	free(argv);
}

int
c_rtf_table_size(cRTFTable *table)
{
	int i = 0;
	cRTFTable* ptr = table;
	while (ptr != NULL) {
		ptr = ptr->next;	
		i++;
	}
	return i;
}

void
c_rtf_table_foreach_row(cRTFTable* table, void* user_data, int (*callback)(int argc, char** argv, void* user_data))
{
	cRTFTable* ptr = table;
	while (ptr != NULL) {
		callback(table->argc, ptr->argv, user_data);
		ptr = ptr->next;	
	}	
}

RTFtable *create_RTFtable(char ***rows_val, char **column_names, int column_num, int rows_num){
	RTFtable *table = malloc(sizeof(RTFtable));
	if (table == NULL) {
		errno = ENOMEM;
		fprintf(stderr, "ERROR. Cannot allocate memory: %d (%s)\n", errno, strerror(errno));		
		exit(errno);  
	}	
	
	table->rows_val = rows_val;
	table->column_names = column_names;
	table->column_num = column_num;
	table->rows_num = rows_num;

	return table;
}

char *RTFstringFromRTFtable(RTFtable *table, bool withBorders){
	//allocate output text
	char *text=calloc(BUFSIZ*BUFSIZ, sizeof(char));
	if (text == NULL) { //stop program to do not damage data
		errno = ENOMEM;
		fprintf(stderr, "ERROR. Cannot allocate memory: %d (%s)\n", errno, strerror(errno));		
		exit(errno);  
	}	
	sprintf(text, "{\\rtf1\\ansi\\deff0\n");
	sprintf(text, "%s\\trowd\n", text);
	int i;

	//create cells
	for (i = 0; i < table->column_num; ++i) {
		if (withBorders) {
			sprintf(text, "%s\\clbrdrt\\brdrs\\clbrdrl\\brdrs\\clbrdrb\\brdrs\\clbrdrr\\brdrs\n", text);
		}
		char cell[20];
		sprintf(cell, "\\cellx%d000", i+1);
		sprintf(text, "%s%s\n", text, cell);
	}

	//make titles
	for (i = 0; i < table->column_num; ++i) {
		char cell[BUFSIZ];
		sprintf(cell, "\\intbl \\qc \\b {%s} \\b0 \\cell\n", table->column_names[i]);
		sprintf(text, "%s%s\n", text, cell);		
	}	
	sprintf(text, "%s\\row\n", text);		

	//make text
	int k;
	for (k = 0; k < table->rows_num; ++k) {
		char **row = table->rows_val[k];
		for (i = 0; i < table->column_num; ++i) {
			char cell[BUFSIZ];
			sprintf(cell, "\\intbl \\ql %s\\cell\n", row[i]);
			sprintf(text, "%s%s\n", text, cell);		
		}	
		sprintf(text, "%s\\row\n", text);		
	}
	
	//close table
	sprintf(text, "%s}\n", text);		

	return text;
}
