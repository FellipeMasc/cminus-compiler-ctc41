/****************************************************/
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the C-MINUS compiler                         */
/* (generates code for the TM machine)              */
/****************************************************/

#include "cgen.h"
#include "code.h"
#include "globals.h"
#include "symtab.h"

/* tmpOffset is the memory offset for temps
   It is decremented each time a temp is
   stored, and incremented when loaded again
*/
static int tmpOffset = 0;

/* frameOffset tracks local variable positions */
static int frameOffset = 0;

/* globalOffset tracks global variable positions */
static int globalOffset = 0;

/* mainEntry stores the location of main function */
static int mainEntry = -1;

static int skipLoc = -1;

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

static char *checkAllPossibleScopes(TreeNode *t, char *scopeName) {
  int count;
  char **scopePrefixes = getScopePrefixes(scopeName, &count);
  char *possibleScope = scopeName;
  for (int i = 0; i < count; i++) {
    if (st_lookup(t->attr.name, scopePrefixes[i]) != -1) {
      possibleScope = scopePrefixes[i];
    }
  }
  // copy possibleScope to a new string
  if (strcmp(possibleScope, scopeName) == 0) {
    return scopeName;
  }
  char *possibleScopeCopy = (char *)malloc(strlen(possibleScope) + 1);
  strcpy(possibleScopeCopy, possibleScope);
  for (int i = 0; i < count; i++) {
    free(scopePrefixes[i]);
  }
  free(scopePrefixes);
  return possibleScopeCopy;
}

/* prototype for internal recursive code generator */
static void cGen(TreeNode *tree, char *scopeName, int depth);

/* Helper function to allocate memory for variables/arrays */
static void allocateMemory(TreeNode *tree, char *scopeName) {
  if (tree == NULL)
    return;

  // while (tree != NULL) {
  if (tree->nodekind == DeclK && tree->kind.decl == VarDeclK) {
    int size = 1;
    if (tree->isArray && tree->child[0] != NULL) {
      size = tree->child[0]->attr.val + 1; // +1 for base address
    }

    // Store memory location in symbol table
    BucketList l = st_lookup_bucket(tree->attr.name, scopeName);
    if (l != NULL) {
      l->size = size - 1; // i just want the size not the base address
      if (strcmp(scopeName, "") == 0) {
        // Global variable
        l->memloc = globalOffset;
        globalOffset -= size;
      } else {
        // Local variable
        l->memloc = frameOffset;
        frameOffset -= size;
      }
    }

    if (tree->isArray) {
      emitComment("-> declare vector");
      emitRM("LDA", ac, l->memloc, gp, "guard addr of vector");
      emitRM("ST", ac, l->memloc, gp, "store addr of vector");
      emitComment("<- declare vector");
    } else {
      emitComment("-> declare var");
      emitComment("<- declare var");
    }
  }

  // Recurse on siblings
  // tree = tree->sibling;
  //}
}

/* Generate code for statement nodes */
static void genStmt(TreeNode *tree, char *scopeName, int depth) {
  TreeNode *p1, *p2, *p3;
  int savedLoc1, savedLoc2, currentLoc;
  int loc;

  switch (tree->kind.stmt) {

  case IfK:
    if (TraceCode)
      emitComment("-> if");
    p1 = tree->child[0]; // condition
    p2 = tree->child[1]; // then part
    p3 = tree->child[2]; // else part

    /* generate code for test expression */
    cGen(p1, scopeName, depth + 1);
    savedLoc1 = emitSkip(1);
    emitComment("if: jump to else belongs here");

    /* recurse on then part */
    cGen(p2, scopeName, depth + 1);
    savedLoc2 = emitSkip(1);
    emitComment("if: jump to end belongs here");
    currentLoc = emitSkip(0);
    emitBackup(savedLoc1);
    emitRM_Abs("JEQ", ac, currentLoc, "if: jmp to else");
    emitRestore();

    /* recurse on else part */
    cGen(p3, scopeName, depth + 1);
    currentLoc = emitSkip(0);
    emitBackup(savedLoc2);
    emitRM_Abs("LDA", PC, currentLoc, "jmp to end");
    emitRestore();

    if (TraceCode)
      emitComment("<- if");
    break;

  case WhileK:
    if (TraceCode)
      emitComment("-> while");
    p1 = tree->child[0]; // condition
    p2 = tree->child[1]; // body

    savedLoc1 = emitSkip(0);
    emitComment("repeat: jump after body comes back here");

    /* generate code for test */
    cGen(p1, scopeName, depth + 1);
    savedLoc2 = emitSkip(1);
    emitComment("while: jump to end belongs here");

    /* generate code for body */
    cGen(p2, scopeName, depth + 1);
    emitRM_Abs("LDA", PC, savedLoc1, "jump to savedLoc1");
    currentLoc = emitSkip(0);
    emitBackup(savedLoc2);
    emitRM_Abs("JEQ", ac, currentLoc, "repeat: jmp to end");
    emitRestore();

    if (TraceCode)
      emitComment("<- repeat");
    break;

  case CompoundK:
    emitComment("-> compound statement");

    cGen(tree->child[0], scopeName, depth + 1);
    cGen(tree->child[1], scopeName, depth + 1);

    emitComment("<- compound statement");
    break;

  case ReturnK:
    if (TraceCode)
      emitComment("-> return");

    /* Generate code for return expression if present */
    if (tree->child[0] != NULL) {
      cGen(tree->child[0], scopeName, depth + 1);
    }

    if (TraceCode)
      emitComment("<- return");
    break;

  default:
    break;
  }
}

/* Generate code for expression nodes */
static void genExp(TreeNode *tree, char *scopeName, int depth) {
  int loc;
  TreeNode *p1, *p2;

  switch (tree->kind.exp) {

  case ConstK:
    if (TraceCode)
      emitComment("-> Const");
    emitRM("LDC", ac, tree->attr.val, 0, "load const");
    if (TraceCode)
      emitComment("<- Const");
    break;

  case IdK:
    if (TraceCode)
      emitComment("-> Id");
    char *possibleScopeName = checkAllPossibleScopes(tree, scopeName);
    loc = st_lookup(tree->attr.name, possibleScopeName);
    int memoryPointer = strcmp(possibleScopeName, "") == 0 ? gp : 2;
    // if (loc < 0) {
    //   // Try global scope
    //   loc = st_lookup(tree->attr.name, "");
    // }
    emitRM("LD", ac, loc, memoryPointer, "load id value");
    if (TraceCode)
      emitComment("<- Id");
    break;

  case VarK: {
    emitComment("-> Id");

    char *possibleScopeName = checkAllPossibleScopes(tree, scopeName);
    loc = st_lookup(tree->attr.name, possibleScopeName);
    int memoryPointer = strcmp(possibleScopeName, "") == 0 ? gp : 2;
    /* If array access: var[index] */
    if (tree->child[0] != NULL) {

      emitComment("-> Vector");
      /* Generate code for index expression */
      cGen(tree->child[0], possibleScopeName, depth + 1);

      /* Get base address of array */
      emitRM("LD", ac1, loc, memoryPointer, "get the address of the vector");
      emitRM("LD", 3, 0, ac, "get the value of the index");
      emitRO("SUB", ac, ac1, 3, "get the address");
      emitRM("LD", ac, 0, ac, "get the value of the vector");
      emitComment("<- Vector");
    } else {
      /* Simple variable */
      emitRM("LD", ac, loc, memoryPointer, "load id value");
    }

    emitComment("<- Id");
    break;
  }
  case OpK: {
    if (TraceCode)
      emitComment("-> Op");
    p1 = tree->child[0];
    p2 = tree->child[1];
    int memoryPointer = strcmp(scopeName, "") == 0 ? gp : 2;
    /* gen code for ac = left arg */
    cGen(p1, scopeName, depth + 1);
    /* gen code to push left operand */
    emitRM("ST", ac, frameOffset--, memoryPointer, "op: push left");
    /* gen code for ac = right operand */
    cGen(p2, scopeName, depth + 1);
    /* now load left operand */
    emitRM("LD", ac1, ++frameOffset, memoryPointer, "op: load left");

    switch (tree->attr.op) {
    case PLUS:
      emitRO("ADD", ac, ac1, ac, "op +");
      break;
    case MINUS:
      emitRO("SUB", ac, ac1, ac, "op -");
      break;
    case TIMES:
      emitRO("MUL", ac, ac1, ac, "op *");
      break;
    case OVER:
      emitRO("DIV", ac, ac1, ac, "op /");
      break;
    case LT:
      emitRO("SUB", ac, ac1, ac, "op <");
      emitRM("JLT", ac, 2, PC, "br if true");
      emitRM("LDC", ac, 0, ac, "false case");
      emitRM("LDA", PC, 1, PC, "unconditional jmp");
      emitRM("LDC", ac, 1, ac, "true case");
      break;
    case LTE:
      emitRO("SUB", ac, ac1, ac, "op <=");
      emitRM("JLE", ac, 2, PC, "br if true");
      emitRM("LDC", ac, 0, ac, "false case");
      emitRM("LDA", PC, 1, PC, "unconditional jmp");
      emitRM("LDC", ac, 1, ac, "true case");
      break;
    case GT:
      emitRO("SUB", ac, ac1, ac, "op >");
      emitRM("JGT", ac, 2, PC, "br if true");
      emitRM("LDC", ac, 0, ac, "false case");
      emitRM("LDA", PC, 1, PC, "unconditional jmp");
      emitRM("LDC", ac, 1, ac, "true case");
      break;
    case GTE:
      emitRO("SUB", ac, ac1, ac, "op >=");
      emitRM("JGE", ac, 2, PC, "br if true");
      emitRM("LDC", ac, 0, ac, "false case");
      emitRM("LDA", PC, 1, PC, "unconditional jmp");
      emitRM("LDC", ac, 1, ac, "true case");
      break;
    case EQ:
      emitRO("SUB", ac, ac1, ac, "op ==");
      emitRM("JEQ", ac, 2, PC, "br if true");
      emitRM("LDC", ac, 0, ac, "false case");
      emitRM("LDA", PC, 1, PC, "unconditional jmp");
      emitRM("LDC", ac, 1, ac, "true case");
      break;
    case NEQ:
      emitRO("SUB", ac, ac1, ac, "op !=");
      emitRM("JNE", ac, 2, PC, "br if true");
      emitRM("LDC", ac, 0, ac, "false case");
      emitRM("LDA", PC, 1, PC, "unconditional jmp");
      emitRM("LDC", ac, 1, ac, "true case");
      break;
    default:
      emitComment("BUG: Unknown operator");
      break;
    }

    if (TraceCode)
      emitComment("<- Op");
    break;
  }
  case AssignK: {
    emitComment("-> assign");

    /* Generate code for rhs */
    cGen(tree->child[0], scopeName, depth + 1);

    char *possibleScopeName = checkAllPossibleScopes(tree, scopeName);
    loc = st_lookup(tree->attr.name, possibleScopeName);
    int memoryPointer = strcmp(possibleScopeName, "") == 0 ? gp : 2;
    /* Check if it's an array assignment */
    if (tree->child[1] != NULL) {
      /* Array assignment: var[index] = value */
      if (TraceCode)
        emitComment("-> assign vector");
      if (TraceCode)
        emitComment("-> Vector");

      /* Value is in ac, save it */
      emitRM("ST", ac, frameOffset--, memoryPointer, "save rhs value");

      /* Generate code for index */
      cGen(tree->child[1], scopeName, depth + 1);

      /* Get base address */

      emitRM("LD", ac1, loc, memoryPointer, "get the address of the vector");
      emitRM("LD", 3, loc, memoryPointer, "get the value of the index");
      emitRM("LDC", 4, 1, 0, "load 1");
      emitRO("ADD", 3, 3, 4, "sub 3 by 1");
      emitRO("SUB", ac1, ac1, 3, "get the address");

      /* Restore value and store */
      emitRM("LD", ac, ++frameOffset, memoryPointer, "restore rhs value");
      emitRM("ST", ac, 0, ac1, "get the value of the vector");
    } else {
      /* Simple variable assignment */
      emitRM("ST", ac, loc, memoryPointer, "assign: store value");
    }

    emitComment("<- assign");
    break;
  }
  case CallK: {
    char *possibleScopeName = checkAllPossibleScopes(tree, scopeName);
    int memoryPointer = strcmp(possibleScopeName, "") == 0 ? gp : 2;
    if (TraceCode) {
      char comment[100];
      sprintf(comment, "-> call function %s", tree->attr.name);
      emitComment(comment);
    }

    /* Handle built-in functions */
    if (strcmp(tree->attr.name, "input") == 0) {
      emitRO("IN", ac, 0, 0, "read integer value");
    } else if (strcmp(tree->attr.name, "output") == 0) {
      /* Generate code for argument */
      if (tree->child[0] != NULL) {
        cGen(tree->child[0], scopeName, depth + 1);
      }
      emitRO("OUT", ac, 0, 0, "write ac");
    } else {
      /* User-defined function call */
      TreeNode *arg = tree->child[0];
      int argCount = 0;

      /* Push arguments onto stack */
      while (arg != NULL) {
        cGen(arg, scopeName, depth + 1);
        emitRM("ST", ac, frameOffset--, memoryPointer, "push argument");
        argCount++;
        arg = arg->sibling;
      }

      /* Save return address */
      emitRM("LDA", ac, 1, PC, "save return address");
      emitRM("ST", ac, frameOffset--, memoryPointer, "push return address");

      /* Jump to function */
      int funcLoc = st_lookup(tree->attr.name, possibleScopeName);
      emitRM_Abs("LDA", PC, funcLoc, "jump to function");

      /* Clean up arguments */
      frameOffset += argCount + 1;
    }

    if (TraceCode)
      emitComment("<- call");
    break;
  }
  default:
    break;
  }
}

/* Generate code for declarations */
static void genDecl(TreeNode *tree, char *scopeName, int depth) {
  if (tree == NULL)
    return;

  switch (tree->kind.decl) {

  case FunDeclK: {
    char comment[100];
    sprintf(comment, "-> Init Function (%s)", tree->attr.name);
    emitComment(comment);
    int isMain = strcmp(tree->attr.name, "main") == 0;

    if (!isMain && skipLoc == -1) {
      skipLoc = emitSkip(1);
    }
    /* Store function entry point */
    int funcEntry = emitSkip(0);
    BucketList l = st_lookup_bucket(tree->attr.name, "");
    if (l != NULL) {
      l->memloc = funcEntry;
    }

    if (isMain) {
      mainEntry = funcEntry;

      if (skipLoc >= 0) {
        emitBackup(skipLoc);
        emitRM_Abs("LDA", PC, mainEntry, "jump to main");
        emitRestore();
      }
      /* For main, emit jump to main BEFORE the function body */
      /* This will be backpatched later in codeGen */
    } else {
      /* Only non-main functions need return address storage */
      emitRM("ST", ac, -1, 2, "store return address from ac");
    }

    /* Save frame offset */
    int savedFrameOffset = frameOffset;
    frameOffset = -2; // Reserve space for return address and old frame pointer

    /* Process parameters */
    TreeNode *param = tree->child[0];
    while (param != NULL) {
      if (param->nodekind == DeclK && param->kind.decl == ParamK) {
        char comment[100];
        sprintf(comment, "-> Param %s", param->isArray ? "vet" : "");
        emitComment(comment);
        BucketList pl = st_lookup_bucket(param->attr.name, tree->attr.name);
        if (pl != NULL) {
          pl->memloc = frameOffset--;
        }
        sprintf(comment, "<- Param %s", param->isArray ? "vet" : "");
        emitComment(comment);
      }
      param = param->sibling;
    }

    /* Generate code for function body */
    cGen(tree->child[1], tree->attr.name, depth + 1);

    /* Restore frame offset */
    frameOffset = savedFrameOffset;
    if (!isMain) {
      /* Only non-main functions need return epilogue */
      emitRM("LDA", ac1, 0, 2, "save current fp into ac1");
      emitRM("LD", 2, 0, 2, "make fp = ofp");
      emitRM("LD", PC, -1, ac1, "return to caller");
    };
    emitComment("<- End Function");
    break;
  }
  case VarDeclK:
    allocateMemory(tree, scopeName);
    break;
  case ParamK:
    /* Already handled in allocateMemory */
    break;

  default:
    break;
  }
}

/* Recursive code generation */
static void cGen(TreeNode *tree, char *scopeName, int depth) {
  if (tree != NULL) {
    switch (tree->nodekind) {
    case StmtK:
      genStmt(tree, scopeName, depth);
      break;
    case ExpK:
      genExp(tree, scopeName, depth);
      break;
    case DeclK:
      genDecl(tree, scopeName, depth);
      break;
    default:
      break;
    }
    cGen(tree->sibling, scopeName, depth);
  }
}

/************************************************/
/* Main code generation procedure              */
/************************************************/
void codeGen(TreeNode *syntaxTree, FILE *codeFile) {
  emitComment("TINY Compilation to TM Code");
  emitComment("Standard prelude:");
  emitRM("LD", mp, 0, ac, "load maxaddress from location 0");
  emitRM("LD", 2, 0, ac, "load maxaddress from location 0");
  emitRM("ST", ac, 0, ac, "clear location 0");
  emitComment("End of standard prelude.");

  // emitComment("-> Init Function (main)");

  /* Generate code for all declarations */
  cGen(syntaxTree, "", 0);

  /* Backpatch jump to main */

  emitComment("End of execution.");
  emitRO("HALT", 0, 0, 0, "");
}