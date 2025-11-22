/****************************************************/
/* File: analyze.h                                  */
/* Semantic analyzer interface for TINY compiler    */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _ANALYZE_H_
#define _ANALYZE_H_

#include "globals.h"


/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode *);

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode *);

/* Function semanticError reports semantic errors */
void semanticError(TreeNode *, char *);

void mainError();

char **getScopePrefixes(const char *scopeName, int *count);

void freeScopeList(scopeList scope);

scopeList deepCopyScopeList(scopeList source);

scopeList getInitialScopeList();

scopeList buildScopeList(char *name, char *type, int depth);

char *constructScopeName(scopeList currentScopeList);

char *returnMostSpecificScopeName(scopeList currentScopeList, TreeNode *t);

scopeList getCurrentScopeList(scopeList initialScopeList, TreeNode *t);



#endif
