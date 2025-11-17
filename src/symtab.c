/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "symtab.h"
#include <stdlib.h>
#include <string.h>

/* SIZE is the size of the hash table */
#define SIZE 211

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash function */
static int hash(char *key) {
  int temp = 0;
  int i = 0;
  while (key[i] != '\0') {
    temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

/* the list of line numbers of the source
 * code in which a variable is referenced
 */
// typedef struct LineListRec {
//   int lineno;
//   struct LineListRec *next;
// } *LineList;

/* The record in the bucket lists for
 * each variable, including name,
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
// typedef struct BucketListRec {
//   char *name;
//   char *type;
//   char *dataType;
//   char *scope;
//   LineList lines;
//   int memloc; /* memory location for variable */
//   struct BucketListRec *next;
// } *BucketList;

/* the hash table */
static BucketList hashTable[SIZE];

void st_just_add_lines(char *name, int lineno, char *scope) {
  int h = hash(name);
  BucketList l = hashTable[h];
  while ((l != NULL) &&
         !((strcmp(name, l->name) == 0) && (strcmp(scope, l->scope) == 0)))
    l = l->next;
  LineList t = l->lines;
  while (t->next != NULL)
    t = t->next;
  t->next = (LineList)malloc(sizeof(struct LineListRec));
  t->next->lineno = lineno;
  t->next->next = NULL;
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert(char *name, int lineno, char *type, char *dataType, char *scope,
               int depth) {
  int h = hash(name);
  BucketList l = hashTable[h];
  while ((l != NULL) &&
         !((strcmp(name, l->name) == 0) && (strcmp(scope, l->scope) == 0)))
    l = l->next;
  if (l == NULL) /* variable not yet in table */
  {
    l = (BucketList)malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->lines = (LineList)malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->type = type;
    l->dataType = dataType;
    l->scope = scope;
    l->depth = depth;
    l->lines->next = NULL;
    l->next = hashTable[h];
    hashTable[h] = l;
  } else /* found in table, so just add line number */
  {
    LineList t = l->lines;
    while (t->next != NULL)
      t = t->next;
    t->next = (LineList)malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
} /* st_insert */

/* Function st_lookup returns the memory
 * location of a variable or -1 if not found
 */
int st_lookup(char *name, char *scope) {
  int h = hash(name);
  BucketList l = hashTable[h];
  while ((l != NULL) &&
         !((strcmp(name, l->name) == 0) && (strcmp(scope, l->scope) == 0)))
    l = l->next;
  if (l == NULL)
    return -1;
  else
    return l->memloc;
}

/* Procedure printSymTab prints a formatted
 * list of the symbol table contents
 */
void printSymTab() {
  int i;
  pc("Variable Name  Scope          ID Type  Data Type  Line Numbers Depth\n");
  pc("-------------  --------       -------  ---------  ------------  "
     "---------\n");
  for (i = 0; i < SIZE; ++i) {
    if (hashTable[i] != NULL) {
      BucketList l = hashTable[i];
      while (l != NULL) {
        LineList t = l->lines;
        pc("%-15s", l->name);
        pc("%-15s", l->scope);
        pc("%-12s", l->type);
        pc("%-10s", l->dataType);
        while (t != NULL) {
          if (t->lineno != 0) {
            pc("%3d", t->lineno);
          }
          t = t->next;
        }
        pc(" | %3d", l->depth);
        pc("\n");
        l = l->next;
      }
    }
  }
} /* printSymTab */

char *getDataType(char *name) {
  int h = hash(name);
  BucketList l = hashTable[h];
  while ((l != NULL) && (strcmp(name, l->name) != 0))
    l = l->next;
  if (l == NULL)
    return NULL;
  else
    return l->dataType;
}

int isThereFunction(char *name) {
  int h = hash(name);
  BucketList l = hashTable[h];
  while ((l != NULL) && (strcmp(name, l->name) != 0))
    l = l->next;
  if (l == NULL || strcmp(l->type, "fun") != 0)
    return 0;
  else
    return 1;
}

int isThereVariableAtSameLine(char *name, int lineno, char *scope) {
  int h = hash(name);
  BucketList l = hashTable[h];
  while ((l != NULL) &&
         !((strcmp(name, l->name) == 0) && (strcmp(scope, l->scope) == 0)))
    l = l->next;
  if (l == NULL)
    return 0;
  else {
    LineList t = l->lines;
    while (t != NULL) {
      if (t->lineno == lineno)
        return 1;
      t = t->next;
    }
    return 0;
  }
}

/* Add this to symtab.c */
BucketList st_lookup_bucket(char *name, char *scope) {
  int h = hash(name);
  BucketList l = hashTable[h];
  while ((l != NULL) &&
         !((strcmp(name, l->name) == 0) && (strcmp(scope, l->scope) == 0)))
    l = l->next;
  return l;
}