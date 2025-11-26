/****************************************************/
/* File: cgen.c                                     */
/* Code Generator for C- (Tiny Machine)             */
/* Versão corrigida:                                */
/* - Arrays com base - índice (pilha decrescente)   */
/* - Globais alocadas com offset negativo de gp     */
/* - CallK seguro usando tmpOffset                  */
/* - AssignK protegendo rhs na pilha                */
/****************************************************/

#include "cgen.h"
#include "analyze.h"
#include "code.h"
#include "globals.h"
#include "symtab.h"
#include <stdlib.h>
#include <string.h>

static int mainEntry = -1;

FunctionInfosRec funcHash[MAX_FUNC_HASH];
static int numFunctions = 0;
static int isFirstFunc = 1;
static int saveMainLoc = 0;


static char *resolveScope(TreeNode *t, char *scope) {
  int count;
  char **p = getScopePrefixes(scope, &count);
  char *ans = scope;

  for (int i = 0; i < count; i++) {
    if (st_lookup(t->attr.name, p[i]) != -1)
      ans = p[i];
  }
  char *ansCopy = strdup(ans);
  for (int i = 0; i < count; i++)
    free(p[i]);
  free(p);

  return ansCopy;
}


void genStmt(TreeNode *t, scopeList scope, char *funcName) {
  TreeNode *p1, *p2, *p3;
  int saved1, saved2, cur;
  scopeList  currentScope;
  currentScope = getCurrentScopeList(scope, t);
  switch (t->kind.stmt) {

  case IfK:
    emitComment("-> if");
    p1 = t->child[0];
    p2 = t->child[1];
    p3 = t->child[2];
    cGen(p1, currentScope, funcName);
    saved1 = emitSkip(1);

    cGen(p2, currentScope, funcName);
    saved2 = emitSkip(1);

    cur = emitSkip(0);
    emitBackup(saved1);
    emitRM_Abs("JEQ", ac, cur, "if: jmp else");
    emitRestore();

    cGen(p3, currentScope, funcName);

    cur = emitSkip(0);
    emitBackup(saved2);
    emitRM_Abs("LDA", PC, cur, "jmp end if");
    emitRestore();

    emitComment("<- if");
    break;

  case WhileK:
    emitComment("-> while");
    p1 = t->child[0];
    p2 = t->child[1];

    saved1 = emitSkip(0);

    cGen(p1, currentScope, funcName);
    saved2 = emitSkip(1);

    cGen(p2, currentScope, funcName);

    emitRM_Abs("LDA", PC, saved1, "while: jump begin");

    cur = emitSkip(0);
    emitBackup(saved2);
    emitRM_Abs("JEQ", ac, cur, "while: exit");
    emitRestore();

    emitComment("<- while");
    break;

  case CompoundK:
    emitComment("-> compound");
    cGen(t->child[0], currentScope, funcName);
    cGen(t->child[1], currentScope, funcName);
    emitComment("<- compound");
    break;

  case ReturnK:
    emitComment("-> return");
    if (t->child[0] != NULL) {
      cGen(t->child[0], currentScope, funcName);
      genEpilogue(t, currentScope, funcName);
    }
    break;

  default:
    break;
  }
}

void genExp(TreeNode *t, scopeList scope, int isAddr) {
  int loc;
  TreeNode *p1, *p2;
  scopeList currentScope;
  currentScope = getCurrentScopeList(scope, t);
  switch (t->kind.exp) {

  case ConstK:
    emitRM("LDC", ac, t->attr.val, 0, "load const");
    break;

  case IdK: {
    char *scopeName = constructScopeName(currentScope);
    char *res = resolveScope(t, scopeName);
    loc = st_lookup(t->attr.name, res);
    int base = (strcmp(res, "") == 0) ? gp : fp;

    if (isAddr)
      emitRM("LDA", ac, loc, base, "addr id");
    else
      emitRM("LD", ac, loc, base, "load id");

    break;
  }

  case VarK: {
    char *scopeName = constructScopeName(currentScope);
    char *res = resolveScope(t, scopeName);
    loc = st_lookup(t->attr.name, res);

    if (t->isArray) {

      if (TraceCode)
        emitComment("-> Array Id");
      cGen(t->child[0], currentScope, NULL);
      // ac has the index
      loc = st_lookup(t->attr.name, res);
      if (!strcmp(res, "")) {
        // global array. we want mem[gp + loc + index]
        emitRO("ADD", ac, ac, gp, "ac = index + gp"); // ac = index + gp
        emitRM("LD", ac, loc, ac, "ac has the value");
      } else {
        char *idType = getIdType(t->attr.name);
        if (strcmp(idType, "param-array") == 0) {
          emitRM("LD", ac1, initFO - loc, fp,
                 "ac1 = mem[reg(fp) + initFO - loc]");
          // now ac1 has the base address of array
          emitRO("ADD", ac, ac1, ac, "ac = (base_addr + index)");
          emitRM("LD", ac, 0, ac, "ac = mem[ac]");
        } else {
          // local array. we want reg(ac) = mem[fp + initFO - loc -
          // index]. index is on ac
          emitRO("SUB", ac, fp, ac, "ac = fp - index"); // ac = fp - index
          emitRM("LD", ac, -loc + initFO, ac,
                 "ac = mem[fp + initFO - loc - index]");
        }
      }
    } else {
      if (TraceCode)
        emitComment("-> Id");
      loc = st_lookup(t->attr.name, res);
      if (!isAddr) {
        if (!strcmp(res, "")) {
          // escopo global, offset de gp
          emitRM("LD", ac, loc, gp, "load id value");
        } else {
          // escopo local, offset de fp
          emitRM("LD", ac, -loc + initFO, fp, "load local id value");
        }
      } else {
        if (!strcmp(res, "")) {
          // i want to return gp + memloc
          emitRM("LDA", ac, loc, gp, "load global id address");
        } else {
          emitRM("LDA", ac, -loc + initFO, fp,
                 "load local id address");
        }
      }

    }
    break;
  }

  case OpK: {
    if (TraceCode)
      emitComment("-> Op");
    p1 =t->child[0];
    p2 = t->child[1];
    /* gen code for ac = left arg */
    cGen(p1, currentScope, NULL);
    /* gen code to push left operand */
    emitRM("LDA", ac1, 0, ac, "Saving temporary value on ac1");
    emitRM("ST", ac1, 0, sp,
           "Temporary store on stack");
    emitRM("LDA", sp, -1, sp, "Decrement sp");
    /* gen code for ac = right operand */
    cGen(p2, currentScope, NULL);
    emitRM("LDA", sp, +1, sp, "Increment sp again");
    emitRM("LD", ac1, 0, sp,
           "Recovering value on ac1");
    /* now load left operand */
    switch (t->attr.op) {
      case PLUS:
        emitRO("ADD", ac, ac, ac1, "op +");
        break;
      case MINUS: {
        if(p2 == NULL) {
          //Load in ac1 zero
          emitRM("LDC", ac1, 0, ac1, "load zero in ac1");
        }
        emitRO("SUB", ac, ac1, ac, "op -");
        break;
      }
      case TIMES:
        emitRO("MUL", ac, ac, ac1, "op *");
        break;
      case OVER:
        emitRO("DIV", ac, ac1, ac, "op /");
        break;
      case LT:
        emitRO("SUB", ac, ac, ac1, "op <");
        emitRM("JGT", ac, 2, PC, "br if true");
        emitRM("LDC", ac, 0, ac1, "false case");
        emitRM("LDA", PC, 1, PC, "unconditional jmp");
        emitRM("LDC", ac, 1, ac1, "true case");
        break;
      case LTE:
        emitRO("SUB", ac, ac, ac1, "op <=");
        emitRM("JGE", ac, 2, PC, "br if true");
        emitRM("LDC", ac, 0, ac1, "false case");
        emitRM("LDA", PC, 1, PC, "unconditional jmp");
        emitRM("LDC", ac, 1, ac1, "true case");
        break;
      case GT:
        emitRO("SUB", ac, ac, ac1, "op >");
        emitRM("JLT", ac, 2, PC, "br if true");
        emitRM("LDC", ac, 0, ac1, "false case");
        emitRM("LDA", PC, 1, PC, "unconditional jmp");
        emitRM("LDC", ac, 1, ac1, "true case");
        break;
      case GTE:
        emitRO("SUB", ac, ac, ac1, "op >=");
        emitRM("JLE", ac, 2, PC, "br if true");
        emitRM("LDC", ac, 0, ac1, "false case");
        emitRM("LDA", PC, 1, PC, "unconditional jmp");
        emitRM("LDC", ac, 1, ac1, "true case");
        break;
      case EQQ:
        emitRO("SUB", ac, ac, ac1, "op ==");
        emitRM("JEQ", ac, 2, PC, "br if true");
        emitRM("LDC", ac, 0, ac1, "false case");
        emitRM("LDA", PC, 1, PC, "unconditional jmp");
        emitRM("LDC", ac, 1, ac1, "true case");
        break;
      case NEQ:
        emitRO("SUB", ac, ac, ac1, "op !=");
        emitRM("JNE", ac, 2, PC, "br if true");
        emitRM("LDC", ac, 0, ac1, "false case");
        emitRM("LDA", PC, 1, PC, "unconditional jmp");
        emitRM("LDC", ac, 1, ac1, "true case");
        break;
      default:
        emitComment("BUG: Unknown operator");
        break;
    } /* case op */
    if (TraceCode)
      emitComment("<- Op");
    break; /* OpK */
  }

  case AssignK: {
    if (TraceCode)
      emitComment("-> assign");
    if (t->child[1] == NULL) { // no array on left side
    
      cGen(t->child[0], currentScope, NULL);
      char *scopeName = constructScopeName(currentScope);
      char *res = resolveScope(t, scopeName);
      loc = st_lookup(t->attr.name, res);
      if (!strcmp(res, "")) {
        emitRM("ST", ac, loc, gp, "assign: store to global variable");
      } else {
        emitRM("ST", ac, -loc + initFO, fp,
               "assign: store to local variable");
      }
    } else { // assign to array
      cGen(t->child[1], currentScope, NULL);
      // store the result from right side temporarily on ac1
      emitRM("LDA", ac1, 0, ac, "Saving temporary value on ac1");
      cGen(t->child[0], currentScope, NULL); // ac now has the index of the left side array

      char *scopeName = constructScopeName(currentScope);
      char *res = resolveScope(t, scopeName);
      loc = st_lookup(t->attr.name, res); // returns beginning of array loc
      if (!strcmp(res, "")) {
        // right now, ac has the index, but we want it to be loc + idx, base gp
        emitRM(
            "LDA", ac, loc, ac,
            "Loading relative global array index address into ac"); // ac = ac +
                                                                    // memloc
        emitRO("ADD", ac, ac, gp, "adding to gp");
        emitRM("ST", ac1, 0, ac,
               "assign: store to global array"); /* RM     mem(d+reg(s)) =
                                                    reg(r) */
      } else {
        char *idType = getIdType(t->attr.name);
        if (strcmp(idType, "param-array") == 0) {
          // ac2 = fp + LOCALS_OFFSET - loc
          emitRM("LDA", ac2, initFO - loc, fp,
                 "loading param address on ac2");
          emitRM("LD", ac2, 0, ac2,
                 "ac2 = mem[ac2]"); // ac2 now has the true array base address
          emitRO(
              "ADD", ac, ac, ac2,
              "ac = ac2 + ac (base_Addr + index)"); 
                                                    
          emitRM("ST", ac1, 0, ac, "Storing result on array correct place");
        } else {
          emitRM("LDC", ac2, -loc, ac2, "loading array memloc on ac2");
          emitRO("SUB", ac, ac2, ac,
                 "loading array index location on ac (relative to "
                 "local_variables)");
          emitRO("ADD", ac, fp, ac,
                 "adding fp to get index location on frame (except for "
                 "FP_LOCALS_OFFSET)");
          emitRM("ST", ac1, initFO, ac,
                 "adding FP_LOCALS_OFFSET to get abslute index location");
        }
      }
    }
    if (TraceCode)
      emitComment("<- assign");
    break; /* assign_k */
  }
  case CallK: {
    emitComment("-> Call");
    char *scopeName = constructScopeName(currentScope);
    char *res = resolveScope(t, scopeName);

    if (strcmp(t->attr.name, "input") == 0) {
      emitRO("IN", ac, 0, 0, "input");
      break;
    }
    if (strcmp(t->attr.name, "output") == 0) {
      cGen(t->child[0], currentScope, NULL);
      emitRO("OUT", ac, 0, 0, "output");
      break;
    }
    genPrologue(t, scope, t->attr.name);

    emitComment("<- Call");

    break;
  }

  default:
    break;
  }
}


void genDecl(TreeNode *t,scopeList scope, char *funcName) {
  int savedLoc;
  scopeList currentScope = getCurrentScopeList(scope, t);
  switch (t->kind.decl) {
  case FunDeclK:
    if (TraceCode)
      emitComment("-> FunDeclK");
    numFunctions++;
    funcHash[numFunctions - 1].funcName = t->attr.name;
    if (isFirstFunc)
      funcHash[numFunctions - 1].startAddr = emitSkip(0) + 1;
    else
      funcHash[numFunctions - 1].startAddr = emitSkip(0);
    funcHash[numFunctions - 1].sizeOfVars =
        getSizeOfVars(t->attr.name);
    if (isFirstFunc) {
      if (strcmp(t->attr.name, "main") == 0) {
        emitRM("ST", fp, 0, sp,
               "Prologue: Storing frame pointer on stack pointer");
        emitRM("LDA", fp, 0, sp,
               "Prologue: FP pointing to current frame function");
        emitRM("LDA", sp, -2, sp, "Prologue: Decrementing SP by 2");
        int sizeOfVars = getSizeOfVars("main");
        emitRM("LDA", sp, -sizeOfVars, sp,
               "Prologue: Allocating memory for local variables");
        /*
        -> Ponteiro para chamador <- fp
        -> Endereço de retorno
        -> Argumentos
        -> Local variables
        -> Temp variables <- sp
        */
      } else { 
        saveMainLoc = emitSkip(1);
        isFirstFunc = 0;
      }
    } else { 

      if (strcmp(t->attr.name,
                  "main") == 0) { 
        savedLoc = emitSkip(0);
        emitBackup(saveMainLoc);
        emitRM_Abs("LDA", PC, savedLoc, "Unconditional relative jmp to main");
        emitRestore();
        emitRM("ST", fp, 0, sp,
               "Prologue: Storing frame pointer on stack pointer");
        emitRM("LDA", fp, 0, sp,
               "Prologue: FP pointing to current frame function");
        emitRM("LDA", sp, -2, sp, "Prologue: Decrementing SP by 2");
        int sizeOfVars = getSizeOfVars("main");
        emitRM("LDA", sp, -sizeOfVars, sp,
               "Prologue: Allocating memory for local variables");
        /*
        -> Ponteiro para chamador <- fp
        -> Endereço de retorno
        -> Argumentos
        -> Local variables
        -> Temp variables <- sp
        */

      } else { 
      }
    }
    cGen(t->child[1], currentScope, t->attr.name);
    if (strcmp(t->attr.name, "main") != 0)
      genEpilogue(t, currentScope, t->attr.name);
    if (TraceCode)
      emitComment("<- FunDeclK");
    break;
  default:
    break;
  }
}

void genPrologue(TreeNode *tree, scopeList scope, char *funcName) {
  scopeList currentScope = getCurrentScopeList(scope, tree);
  char *scopeName = constructScopeName(currentScope);
  char *res = resolveScope(tree, scopeName);
  if (TraceCode)
    emitComment("-> Function Prologue");
  emitComment(scopeName);
  
  emitRM("ST", fp, 0, sp, "Prologue: Storing FP on stack");
  /*
  -> Ponteiro para chamador <- fp
  */
  emitRM("LDA", sp, -2, sp, "Prologue: Decrementing SP by 2");
  /*
  -> Ponteiro para chamador
  -> Endereço de retorno
  -> arg1 <- sp
  */
  int sizeOfVars = getSizeOfVars(funcName);
  TreeNode *currentArg = tree->child[0];
  int jumpAddr = 0;
  int argCount = 0;
  int returnAddr = 0;
  while (currentArg != NULL) {
    if (currentArg->kind.exp == VarK) {
      char *mostSpecificScope = returnMostSpecificScopeName(currentScope, currentArg);
      char *idType = getIdTypeScope(currentArg->attr.name, mostSpecificScope);
      if ((strcmp(idType, "param-array") == 0 || strcmp(idType, "array") == 0) && !strcmp(mostSpecificScope, "")) {
        // array passed by reference
        genExp(currentArg, currentScope, 1);
      } else {
        // passed by value
        genExp(currentArg, currentScope, 0);
      }
    } else {
      genExp(currentArg, currentScope, 0);
    }
    emitRM("ST", ac, 0, sp, "Storing current argument on stack");
    emitRM("LDA", sp, -1, sp, "Decrementing sp");
    argCount++;
    /*
  -> Ponteiro para chamador
  -> Endereço de retorno
  -> arg1 
  -> arg2 <- sp
  ...
  */
    currentArg = currentArg->sibling;
  }

  emitRM("LDA", fp, 2 + argCount, sp,
         "Prologue: FP pointing to current frame function");
  returnAddr = emitSkip(0) + 4;
  emitRM("LDC", ac, returnAddr, ac, "Storing return address on ac");
  emitRM("ST", ac, -1, fp,
         "Store return address on stack");
  emitRM("LDA", sp, -sizeOfVars + argCount, sp,
         "Allocating memory for local variables");
  for (int i = 0; i < numFunctions; i++) {
    if (strcmp(funcHash[i].funcName, tree->attr.name) == 0) {
      jumpAddr = funcHash[i].startAddr;
      break;
    }
  }
  emitRM_Abs("LDA", PC, jumpAddr, "jump to function");

  if (TraceCode)
    emitComment("<- Function Prologue");
}

void genEpilogue(TreeNode *tree, scopeList scope, char *funcName) {
  if (TraceCode)
    emitComment("-> Function Epilogue");
  int sizeOfVars = getSizeOfVars(funcName);
  /*
  -> Ponteiro para chamador 
  -> Endereço de retorno
  -> Argumentos
  -> Local variables
  -> Temp variables <- sp
  */
  emitRM("LDA", sp, sizeOfVars, sp, "Removing local variables from stack"); // reg(sp) = reg(sp) + sizeOfVars
  /*
  -> Ponteiro para chamador
  -> Endereço de retorno 
  -> Argumentos <- sp
  -> Local variables
  -> Temp variables
  */
  emitRM("LD", fp, 2, sp, "Getting previous FP from stack"); // reg(fp) = mem[reg(sp)+2]
  /*
  -> Ponteiro para chamador <- fp
  -> Endereço de retorno
  -> Argumentos <- sp
  -> Local variables
  -> Temp variables
  */
  emitRM("LDA", sp, 2, sp, "Removing ofpFO and retFO from stack");
  /*
  reg(sp) = reg(sp) + 2
    -> Ponteiro para chamador <- fp <- sp
    -> Endereço de retorno
    -> Argumentos
    -> Local variables
    -> Temp variables
    */
  emitRM("LD", ac1, -1, sp, "Loading return address in ac1"); 
  /*
  reg(ac1) = mem[reg(sp)-1]
    -> Ponteiro para chamador <- fp
    -> Endereço de retorno <- ac1
    -> Argumentos
    -> Local variables
    -> Temp variables
    */
  emitRM("LDA", PC, 0, ac1, "returning"); // reg(PC) = reg(ac1)

  if (TraceCode)
    emitComment("<- Function Epilogue");
}

void cGen(TreeNode *t, scopeList scope, char *funcName) {
  if (t) {

    switch (t->nodekind) {
    case DeclK:
      genDecl(t, scope, funcName);
      break;
    case StmtK:
      genStmt(t, scope, funcName);
      break;
    case ExpK:
      genExp(t, scope,0);
      break;
    }

    cGen(t->sibling, scope, funcName);
  }
}

void codeGen(TreeNode *syntaxTree, FILE *codeFile) {
  emitComment("TINY Compilation to TM Code");
  emitComment("Standard prelude:");
  emitRM("LD", mp, 0, ac, "load maxaddress from location 0");
  emitRM("ST", ac, 0, ac, "clear location 0");
  emitRM("LDA", sp, 0, mp, "pointing sp to the top of the memory");

  emitRM("LDC", gp, 0, 0, "init GP to 0");
  emitComment("End of standard prelude.");

  scopeList initialScope = getInitialScopeList();
  cGen(syntaxTree, initialScope, NULL);

  emitComment("End of execution.");
  emitRO("HALT", 0, 0, 0, "");
}