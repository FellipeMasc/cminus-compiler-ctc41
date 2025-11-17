/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "analyze.h"
#include "globals.h"
#include "symtab.h"

#define MAX_SCOPE_STACK 100

static char **getScopePrefixes(const char *scopeName, int *count) {
  // Split input like "A-B-C" into ["", "A", "A-B", "A-B-C"]
  // Always start result with "" (the global scope)
  // *count will be set to the result array length

  if (!scopeName || !count)
    return NULL;

  // Upper bound: at most strlen(scopeName)+2 items ("" plus each '-' piece)
  int len = strlen(scopeName);
  int maxParts = len + 2;
  char **result = (char **)malloc(maxParts * sizeof(char *));
  int n = 0;

  // Always add the global scope
  result[n++] = strdup("");

  if (len == 0) {
    *count = n;
    return result;
  }

  // Work through scopeName and add each prefix
  char *buf = (char *)malloc(len + 1);
  int buflen = 0;
  for (int i = 0; i < len; ++i) {
    buf[buflen++] = scopeName[i];
    buf[buflen] = '\0';
    if (scopeName[i] == '-') {
      // Add up to, but not including this '-'
      if (buflen > 1) {
        char tmp = buf[buflen - 1];
        buf[buflen - 1] = '\0';
        result[n++] = strdup(buf);
        buf[buflen - 1] = tmp;
      }
    }
  }
  // Add the full scopeName last
  result[n++] = strdup(scopeName);

  free(buf);
  *count = n;
  return result;
}

static void freeScopeList(scopeList scope) {
  if (scope == NULL) {
    return;
  }

  // Free the next nodes first (recursive)
  if (scope->next != NULL) {
    freeScopeList(scope->next);
  }

  // Free dynamically allocated strings (only if they were strdup'd)
  if (scope->name != NULL && strcmp(scope->name, "") != 0) {
    free(scope->name);
  }
  if (scope->type != NULL && strcmp(scope->type, "") != 0) {
    free(scope->type);
  }

  // Free the node itself
  free(scope);
}

static scopeList deepCopyScopeList(scopeList source) {
  if (source == NULL) {
    return NULL;
  }

  // Allocate new scope node
  scopeList copy = (scopeList)malloc(sizeof(struct scopeListRec));

  // Deep copy strings (avoid sharing pointers)
  copy->name = (source->name != NULL) ? strdup(source->name) : NULL;
  copy->type = (source->type != NULL) ? strdup(source->type) : NULL;

  // Copy scalar value
  copy->depth = source->depth;

  // Recursively deep copy the linked list
  copy->next = deepCopyScopeList(source->next);

  // Handle 'end' pointer - need to find the equivalent node in the new list
  if (source->end == NULL) {
    copy->end = NULL;
  } else if (source->end == source) {
    // If end points to itself
    copy->end = copy;
  } else {
    // Find which node 'end' points to and set copy->end to the equivalent
    // For simplicity, traverse to find the same depth/name
    scopeList temp = copy;
    scopeList sourceTemp = source;
    while (sourceTemp != NULL && sourceTemp != source->end) {
      sourceTemp = sourceTemp->next;
      if (temp != NULL)
        temp = temp->next;
    }
    copy->end = temp;
  }

  return copy;
}

static scopeList getInitialScopeList() {
  scopeList initialScope = (scopeList)malloc(sizeof(struct scopeListRec));
  initialScope->type = "global";
  initialScope->depth = 0;
  initialScope->name = "";
  initialScope->next = NULL;
  initialScope->end = initialScope;
  return initialScope;
}

static scopeList buildScopeList(char *name, char *type, int depth) {
  scopeList newScope = (scopeList)malloc(sizeof(struct scopeListRec));
  newScope->name = (name != NULL) ? name : "";
  newScope->type = (type != NULL) ? type : "";
  newScope->depth = depth;
  newScope->next = NULL;
  newScope->end = NULL;
  return newScope;
}

static char *constructScopeName(scopeList currentScopeList) {
  char *scopeName = (char *)malloc(256 * sizeof(char));
  scopeName[0] = '\0';
  int currentDepth = 0;
  scopeList temp = currentScopeList;
  while (temp != NULL) {
    if (temp->depth == currentDepth) {
      if (currentDepth >= 2) {
        strcat(scopeName, "-");
      }
      strcat(scopeName, temp->name);
      currentDepth += 1;
    }
    temp = temp->next;
  }
  return scopeName;
}

static scopeList getCurrentScopeList(scopeList initialScopeList, TreeNode *t) {
  int initialDepth = initialScopeList->end->depth;
  char *initialName = initialScopeList->end->name;
  // char *initialName = constructScopeName(initialScopeList);
  scopeList currentScope = buildScopeList(t->attr.name, t->type, initialDepth);

  scopeList copyOfInitialScopeList = deepCopyScopeList(initialScopeList);
  switch (t->nodekind) {
  case StmtK:
    switch (t->kind.stmt) {
    case IfK:
      copyOfInitialScopeList->end = currentScope;
      scopeList temp = copyOfInitialScopeList;
      while (temp->next != NULL) {
        temp = temp->next;
      }
      temp->next = currentScope;
      currentScope->name = initialName;
      // currentScope->depth = initialDepth + 1;
      return copyOfInitialScopeList;
    case WhileK:
      copyOfInitialScopeList->end = currentScope;
      scopeList tempWhile = copyOfInitialScopeList;
      while (tempWhile->next != NULL) {
        tempWhile = tempWhile->next;
      }
      tempWhile->next = currentScope;
      currentScope->name = initialName;
      // currentScope->depth = initialDepth + 1;
      return copyOfInitialScopeList;
    case CompoundK:
      if (initialScopeList->end->type != NULL &&
          strcmp(initialScopeList->end->type, "block") == 0) {
        scopeList temp = copyOfInitialScopeList;
        while (temp->next != NULL) {
          temp = temp->next;
        }
        temp->next = currentScope;
        copyOfInitialScopeList->end = currentScope;
        copyOfInitialScopeList->end->name = "block";
        currentScope->depth = initialDepth + 1;
        return copyOfInitialScopeList;
      } else {
        copyOfInitialScopeList->end = currentScope;
        scopeList temp = copyOfInitialScopeList;
        while (temp->next != NULL) {
          temp = temp->next;
        }
        temp->next = currentScope;
        currentScope->name = initialName;
        currentScope->depth = initialDepth;
        return copyOfInitialScopeList;
      }
    default:
      break;
    }
  case DeclK:
    switch (t->kind.decl) {
    case FunDeclK:
      copyOfInitialScopeList->end = currentScope;
      scopeList temp = copyOfInitialScopeList;
      while (temp->next != NULL) {
        temp = temp->next;
      }
      temp->next = currentScope;
      currentScope->depth = initialDepth + 1;
      return copyOfInitialScopeList;
    default:
      break;
    }
    break;
  default:
    free(currentScope);
    return NULL;
    break;
  }
  return NULL;
}

/* Procedure traverse is a generic recursive
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc
 * in postorder to tree pointed to by t
 */
static void traverse(TreeNode *t, void (*preProc)(TreeNode *, scopeList),
                     void (*postProc)(TreeNode *, scopeList),
                     scopeList initialScopeList) {
  if (t != NULL) {

    preProc(t, initialScopeList);
    scopeList currentScopeList = getCurrentScopeList(initialScopeList, t);
    if (currentScopeList == NULL) {
      currentScopeList = initialScopeList;
    }
    {
      int i;
      for (i = 0; i < MAXCHILDREN; i++) {
        traverse(t->child[i], preProc, postProc, currentScopeList);
      }
    }
    postProc(t, currentScopeList);
    traverse(t->sibling, preProc, postProc, initialScopeList);
  }
}

/* nullProc is a do-nothing procedure to
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode *t, scopeList currentScopeList) {
  if (t == NULL)
    return;
  else
    return;
}

static char *checkAllPossibleScopes(TreeNode *t, scopeList currentScopeList) {
  int count;
  char **scopePrefixes =
      getScopePrefixes(constructScopeName(currentScopeList), &count);
  char *possibleScope = NULL;
  for (int i = 0; i < count; i++) {
    if (st_lookup(t->attr.name, scopePrefixes[i]) != -1) {
      possibleScope = scopePrefixes[i];
    }
  }
  // copy possibleScope to a new string
  if (possibleScope == NULL) {
    return NULL;
  }
  char *possibleScopeCopy = (char *)malloc(strlen(possibleScope) + 1);
  strcpy(possibleScopeCopy, possibleScope);
  for (int i = 0; i < count; i++) {
    free(scopePrefixes[i]);
  }
  free(scopePrefixes);
  return possibleScopeCopy;
}

static char *returnMostSpecificScopeName(scopeList currentScopeList,
                                         TreeNode *t) {
  int count;
  char **scopePrefixes =
      getScopePrefixes(constructScopeName(currentScopeList), &count);
  char *mostSpecificScopeName = NULL;
  for (int i = 0; i < count; i++) {
    if (st_lookup(t->attr.name, scopePrefixes[i]) != -1) {
      mostSpecificScopeName = scopePrefixes[i];
    }
  }
  if (mostSpecificScopeName == NULL) {
    return NULL;
  }
  // copy mostSpecificScopeName to a new string
  char *mostSpecificScopeNameCopy =
      (char *)malloc(strlen(mostSpecificScopeName) + 1);
  strcpy(mostSpecificScopeNameCopy, mostSpecificScopeName);

  for (int i = 0; i < count; i++) {
    free(scopePrefixes[i]);
  }
  free(scopePrefixes);
  return mostSpecificScopeNameCopy;
}

/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
static void insertNode(TreeNode *t, scopeList currentScopeList) {
  char *scopeName = constructScopeName(currentScopeList);
  switch (t->nodekind) {
  case StmtK:
    switch (t->kind.stmt) {
    case IfK:
      break;
    case WhileK:
      break;
    case CompoundK:
      break;
    case ReturnK:
      break;
    default:
      break;
    }
    break;
  case ExpK:
    switch (t->kind.exp) {
    case OpK:
      break;
    case ConstK:
      break;
    case IdK:
      break;
    case AssignK: {
      char *possibleScopeName = checkAllPossibleScopes(t, currentScopeList);
      char *mostSpecificScopeName =
          returnMostSpecificScopeName(currentScopeList, t);

      if (possibleScopeName == NULL) {
        char *message = (char *)malloc(256 * sizeof(char));
        sprintf(message, "'%s' was not declared in this scope", t->attr.name);
        semanticError(t, message);
        free(message);
      } else {
        st_insert(t->attr.name, t->lineno, t->isArray ? "array" : "var",
                  t->type, mostSpecificScopeName, currentScopeList->end->depth);
      }
      break;
    }
    case CallK: {
      char *possibleScopeName = checkAllPossibleScopes(t, currentScopeList);
      char *mostSpecificScopeName =
          returnMostSpecificScopeName(currentScopeList, t);

      if (possibleScopeName == NULL) {
        char *message = (char *)malloc(256 * sizeof(char));
        sprintf(message, "'%s' was not declared in this scope", t->attr.name);
        semanticError(t, message);
        free(message);
      } else {
        st_just_add_lines(t->attr.name, t->lineno, mostSpecificScopeName);
        // if (t->child[0] != NULL) {
        //   if (t->child[0]->kind.exp == ConstK) {
        //     st_just_add_lines(t->child[0]->attr.name, t->lineno,
        //                       mostSpecificScopeName);
        //   }
        //   if (t->child[0]->kind.exp == VarK) {
        //     st_just_add_lines(t->child[0]->attr.name, t->lineno,
        //                       mostSpecificScopeName);
        //   }
        // }
      }
      break;
    }
    case VarK: {
      char *mostSpecificScopeName =
          returnMostSpecificScopeName(currentScopeList, t);
      if (isThereVariableAtSameLine(t->attr.name, t->lineno,
                                    mostSpecificScopeName)) {
        // char *message = (char *)malloc(256 * sizeof(char));
        // sprintf(message, "'%s' was already declared at line %d",
        // t->attr.name,
        //         t->lineno);
        // semanticError(t, message);
        // free(message);
      } else {
        st_insert(t->attr.name, t->lineno, t->isArray ? "array" : "var",
                  t->type, mostSpecificScopeName, currentScopeList->end->depth);
      }
      break;
    }
    case TypeSpecK:
      break;
    default:
      break;
    }
    break;
  case DeclK:
    switch (t->kind.decl) {
    case VarDeclK: {
      if (strcmp(t->type, "void") == 0) {
        semanticError(t, "variable declared void");

      } else if (isThereFunction(t->attr.name)) {
        char *message = (char *)malloc(256 * sizeof(char));
        sprintf(message, "'%s' was already declared as a function",
                t->attr.name);
        semanticError(t, message);
        free(message);
      } else {
        char *idType = t->isArray ? "array" : "var";
        if (st_lookup(t->attr.name, scopeName) == -1)
          st_insert(t->attr.name, t->lineno, idType, t->type, scopeName,
                    currentScopeList->end->depth);
        else {
          char *message = (char *)malloc(256 * sizeof(char));
          sprintf(message, "'%s' was already declared as a variable",
                  t->attr.name);
          semanticError(t, message);
          free(message);
        }
      }
      break;
    }
    case FunDeclK: {
      if (st_lookup(t->attr.name, scopeName) == -1)
        st_insert(t->attr.name, t->lineno, "fun", t->typeReturn, scopeName,
                  currentScopeList->end->depth);
      else
        st_insert(t->attr.name, t->lineno, "fun", t->type, scopeName,
                  currentScopeList->end->depth);
      break;
    }
    case ParamK: {
      if (st_lookup(t->attr.name, scopeName) == -1)
        st_insert(t->attr.name, t->lineno,
                  t->isArray ? "param-array" : "param-var", t->type, scopeName,
                  currentScopeList->end->depth);
      else
        st_insert(t->attr.name, t->lineno,
                  t->isArray ? "param-array" : "param-var", t->type, scopeName,
                  currentScopeList->end->depth);
      break;
    }
    case TypeK:
      break;
    }
    break;
  default:
    break;
  }
}

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode *syntaxTree) {
  st_insert("input", 0, "fun", "int", "", 0);
  st_insert("output", 0, "fun", "void", "", 0);
  scopeList initialScopeList = getInitialScopeList();
  traverse(syntaxTree, insertNode, nullProc, initialScopeList);
  if (TraceAnalyze) {
    pc("\nSymbol table:\n\n");
    printSymTab();
  }
  free(initialScopeList);
}

static void typeError(TreeNode *t, char *message) {
  pce("Type error at line %d: %s\n", t->lineno, message);
  Error = TRUE;
}

void semanticError(TreeNode *t, char *message) {
  pce("Semantic error at line %d: %s\n", t->lineno, message);
  Error = TRUE;
}

void mainError() {
  if (st_lookup("main", "") == -1) {
    char *message = (char *)malloc(256 * sizeof(char));
    sprintf(message, "undefined reference to 'main'");
    pce("Semantic error: %s\n", message);
    free(message);
  }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode *t, scopeList currentScopeList) {
  char *scopeName = constructScopeName(currentScopeList);
  switch (t->nodekind) {
  case StmtK:
    switch (t->kind.stmt) {
    case IfK:
      break;
    case WhileK:
      break;
    case CompoundK:
      break;
    case ReturnK:
      if (t->type != NULL) {
        if (strcmp(t->type, "void") == 0) {
          char *dataType = getDataType(currentScopeList->end->name);
          char *returnType = malloc(256 * sizeof(char));
          if (t->child[0] != NULL) {
            if (t->child[0]->kind.exp == ConstK) {
              returnType = "int";
            }
            if (t->child[0]->kind.exp == VarK) {
              returnType = getDataType(t->child[0]->attr.name);
            }
            if (t->child[0]->kind.exp == CallK) {
              returnType = getDataType(t->child[0]->attr.name);
            }
            if (t->child[0]->kind.exp == OpK) {
              returnType = "int";
            }
          } else {
            returnType = "void";
          }
          if (strcmp(dataType, returnType) != 0) {
            semanticError(t, "Must return same type as function declaration");
          }
        }
      }
      break;
    default:
      break;
    }
    break;
  case ExpK:
    switch (t->kind.exp) {
    case OpK:
      break;
    case ConstK:
      break;
    case IdK:
      break;
    case AssignK: {
      char *possibleScopeName = checkAllPossibleScopes(t, currentScopeList);
      if (possibleScopeName == NULL) {
        break;
      } else {
        if (t->isArray) {
          if (t->child[1]->kind.exp != ConstK && t->child[1]->kind.exp != OpK &&
              getDataType(t->child[1]->attr.name) != NULL &&
              strcmp(getDataType(t->child[1]->attr.name), "void") == 0) {
            semanticError(t, "invalid use of void expression");
          }
        } else {

          if (t->child[0]->kind.exp != ConstK && t->child[0]->kind.exp != OpK &&
              getDataType(t->child[0]->attr.name) != NULL &&
              strcmp(getDataType(t->child[0]->attr.name), "void") == 0) {
            semanticError(t, "invalid use of void expression");
          }
        }
      }
      break;
    }
    case CallK:
      break;
    case VarK:
      break;
    case TypeSpecK:
      break;
    default:
      break;
    }
    break;
  case DeclK:
    switch (t->kind.decl) {
    case VarDeclK: {
    }
    case FunDeclK: {
    }
    case ParamK: {

      break;
    }
    case TypeK:
      break;
    }
    break;
  default:
    break;
  }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode *syntaxTree) {
  scopeList initialScopeList = getInitialScopeList();
  traverse(syntaxTree, nullProc, checkNode, initialScopeList);
  free(initialScopeList);
}
