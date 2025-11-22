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

typedef struct LineListRec {
  int lineno;
  struct LineListRec *next;
} *LineList;

typedef struct BucketListRec {
  char *name;
  char *type;
  char *dataType;
  char *scope;
  LineList lines;
  int depth;
  int memloc; /* memory location for variable */
  int size; /* size of the variable if it is an array */
  int sizeOfVars; /* size of the variables if it is a function */
  struct BucketListRec *next;
} *BucketList;

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( char * name, int lineno, char * type, char * dataType, char * scope , int depth, int memLoc);

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

/* Add this to symtab.h */
BucketList st_lookup_bucket(char * name, char * scope);

void addSizeOfVars(char *name, int sizeOfVars);

int getSizeOfVars(char *name);

char *getIdType(char *name);

char *getIdTypeScope(char *name, char *scope);

#endif
