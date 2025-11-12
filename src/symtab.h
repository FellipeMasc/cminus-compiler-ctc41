/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "../lib/log.h"

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( char * name, int lineno, char * type, char * dataType, char * scope );

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup ( char * name, char * scope );

/* Procedure printSymTab prints a formatted 
 * list of the symbol table contents 
 */
void printSymTab();

/* Procedure st_just_add_lines adds a line number to the symbol table */
void st_just_add_lines(char *name, int lineno, char *scope);

char *getDataType(char *name);

int isThereFunction(char *name);

int isThereVariableAtSameLine(char *name, int lineno, char *scope);
#endif
