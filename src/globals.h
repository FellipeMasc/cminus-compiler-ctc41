/****************************************************/
/* File: globals.h                                  */
/* Global types and vars for TINY compiler          */
/* must come before other include files             */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "../lib/log.h"
#include "symtab.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* MAXRESERVED = the number of reserved words */
#define MAXRESERVED 8
typedef int TokenType;


extern FILE *source;           /* source code text file */
extern FILE *redundant_source; /* source code text file */
extern FILE *listing;          /* listing output text file */
extern FILE *code;             /* code text file for TM simulator */

extern int lineno; /* source line number for listing */

/**************************************************/
/***********   Syntax tree for parsing ************/
/**************************************************/


// /* ExpType is used for type checking */
typedef enum { Void, Integer, Boolean } ExpType;

typedef enum { StmtK, ExpK, DeclK } NodeKind;

typedef enum {
  VarDeclK, // var_decl
  FunDeclK, // fun_decl
  ParamK,   // param
  TypeK     // type_spec (if you want to represent it)
} DeclKind;

typedef enum {
  IfK,       // decl_sel (if statement)
  WhileK,    // decl_ite (while statement)
  CompoundK, // decl_compo (compound statement)
  ReturnK,   // decl_return
} StmtKind;

typedef enum {
  OpK,      // operators in expressions
  ConstK,   // NUM
  IdK,      // ID (simple variable)
  AssignK,  // var = exp
  CallK,    // ativ (function call)
  VarK,     // var (variable with possible array access)
  TypeSpecK // For storing INT or VOID
} ExpKind;

typedef struct scopeListRec {
  char *name;
  char *type;
  int depth;
  struct scopeListRec *end;
  struct scopeListRec *next;
} *scopeList;

#define MAXCHILDREN 3

typedef struct treeNode {
  struct treeNode *child[MAXCHILDREN];
  struct treeNode *sibling;
  int lineno;
  NodeKind nodekind;
  union {
    StmtKind stmt;
    ExpKind exp;
    DeclKind decl;
  } kind;
  union {
    TokenType op;
    int val;
    char *name;
  } attr;
  char *type; /* for type checking of exps */
  char *typeReturn;
  int isArray;
  scopeList nodeScopeList;
} TreeNode;

/**************************************************/
/***********   Flags for tracing       ************/
/**************************************************/

/* EchoSource = TRUE causes the source program to
 * be echoed to the listing file with line numbers
 * during parsing
 */
extern int EchoSource;

/* TraceScan = TRUE causes token information to be
 * printed to the listing file as each token is
 * recognized by the scanner
 */
extern int TraceScan;

/* TraceParse = TRUE causes the syntax tree to be
 * printed to the listing file in linearized form
 * (using indents for children)
 */
extern int TraceParse;

/* TraceAnalyze = TRUE causes symbol table inserts
 * and lookups to be reported to the listing file
 */
extern int TraceAnalyze;

/* TraceCode = TRUE causes comments to be written
 * to the TM code file as code is generated
 */
extern int TraceCode;

/* Error = TRUE prevents further passes if an error occurs */
extern int Error;

extern int FirstLine;
#endif
