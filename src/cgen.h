/****************************************************/
/* File: cgen.h                                     */
/* The code generator interface to the TINY compiler*/
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _CGEN_H_
#define _CGEN_H_

#include "../build/parser.h"
#include "globals.h"

#define ofpFO 0
#define retFO -1
#define initFO -2
#define MAX_FUNC_HASH 20

typedef struct FunctionInfosRec {
  char *funcName;
  int startAddr;
  int sizeOfVars;
} FunctionInfosRec;


/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. 
 */
void codeGen(TreeNode * syntaxTree, FILE * code);

void cGen(TreeNode *t, scopeList scope, char *funcName);

void genPrologue(TreeNode *t, scopeList scope, char *funcName);
void genEpilogue(TreeNode *t, scopeList scope, char *funcName);
void genStmt(TreeNode *t, scopeList scope, char *funcName);
void genExp(TreeNode *t, scopeList scope, int isAddr);
void genDecl(TreeNode *t, scopeList scope, char *funcName);

#endif
