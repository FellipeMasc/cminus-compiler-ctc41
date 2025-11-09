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
#include "util.c"

#define MAX_SCOPE_STACK 100

// static void pushScope(char *scope, char *scopeStack[MAX_SCOPE_STACK]) {
// scopeStack[++scopeStackTop] = scope; }

// static char *popScope(char *scopeStack[MAX_SCOPE_STACK]) { return
// scopeStack[scopeStackTop--]; }

/* counter for variable memory locations */
static int location = 0;

static struct scopeListRec getInitialScopeList() {
  struct scopeListRec initialScopeList;
  initialScopeList.type = "global";
  initialScopeList.depth = 0;
  initialScopeList.name = "";
  initialScopeList.next = NULL;
  return initialScopeList;
}

static void addScopeToChild(TreeNode *parent, TreeNode *child) {
  if ((parent->nodekind == StmtK && parent->kind.stmt != ReturnK) ||
      (parent->nodekind == DeclK && parent->kind.decl == FunDeclK)) {
    scopeList newScope = (scopeList)malloc(sizeof(struct scopeListRec));
    scopeList childScope = (scopeList)malloc(sizeof(struct scopeListRec));
    newScope->next = NULL;
    parent->nodeScopeList->end->next = newScope;
    childScope->end->depth = parent->nodeScopeList->end->depth + 1;
    *childScope = *parent->nodeScopeList;
    childScope->end = newScope;
    child->nodeScopeList = childScope;
    switch (parent->nodekind) {
    case StmtK:
      switch (parent->kind.stmt) {
      case IfK:
        childScope->end->name = "if";
        childScope->end->type = "if";
        break;
      case WhileK:
        childScope->end->name = "while";
        childScope->end->type = "while";
        break;
      case CompoundK:
        if (parent->nodekind == StmtK && parent->kind.stmt == CompoundK) {
          childScope->type = "block";
          childScope->name = "block";
        } else {
          childScope->name = parent->nodeScopeList->name;
          childScope->type = parent->nodeScopeList->type;
          childScope->end->depth = parent->nodeScopeList->end->depth - 1;
        }
        break;
      default:
        break;
      }
      break;
    case DeclK:
      if (parent->kind.decl == FunDeclK) {
      }
      break;
    case ExpK:
      break;
    }
  } else {

    scopeList childScope = (scopeList)malloc(sizeof(struct scopeListRec));

    *childScope = *parent->nodeScopeList;

    childScope->depth += 1;

    child->nodeScopeList = childScope;
  }
}

/* Procedure traverse is a generic recursive
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc
 * in postorder to tree pointed to by t
 */
static void traverse(TreeNode *t, void (*preProc)(TreeNode *),
                     void (*postProc)(TreeNode *),
                     struct scopeListRec currentScopeList) {
  if (t != NULL) {
    preProc(t);
    {
      int i;
      for (i = 0; i < MAXCHILDREN; i++)

        traverse(t->child[i], preProc, postProc);
    }
    postProc(t);
    traverse(t->sibling, preProc, postProc);
  }
}

/* nullProc is a do-nothing procedure to
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode *t) {
  if (t == NULL)
    return;
  else
    return;
}

/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
static void insertNode(TreeNode *t) {
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
    case AssignK:
      break;
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
      if (st_lookup(t->attr.name) == -1)
        st_insert(t->attr.name, t->lineno, location++);
      else
        st_insert(t->attr.name, t->lineno, 0);
      break;
    }
    case FunDeclK: {
      if (st_lookup(t->attr.name) == -1)
        st_insert(t->attr.name, t->lineno, location++);
      else
        st_insert(t->attr.name, t->lineno, 0);
      break;
    }
    case ParamK: {
      if (st_lookup(t->attr.name) == -1)
        st_insert(t->attr.name, t->lineno, location++);
      else
        st_insert(t->attr.name, t->lineno, 0);
      break;
    }
    case TypeK: {
      if (st_lookup(t->attr.name) == -1)
        st_insert(t->attr.name, t->lineno, location++);
      else
        st_insert(t->attr.name, t->lineno, 0);
      break;
    }
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
  struct scopeListRec initialScopeList = getInitialScopeList();
  traverse(syntaxTree, insertNode, nullProc, initialScopeList);
  if (TraceAnalyze) {
    pc("\nSymbol table:\n\n");
    printSymTab();
  }
}

static void typeError(TreeNode *t, char *message) {
  pce("Type error at line %d: %s\n", t->lineno, message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode *t) {
  // switch (t->nodekind) {
  // case ExpK:
  //   switch (t->kind.exp) {
  //   case OpK:
  //     if ((t->child[0]->type != Integer) || (t->child[1]->type != Integer))
  //       typeError(t, "Op applied to non-integer");
  //     if ((t->attr.op == EQ) || (t->attr.op == LT))
  //       t->type = Boolean;
  //     else
  //       t->type = Integer;
  //     break;
  //   case ConstK:
  //   case IdK:
  //     t->type = Integer;
  //     break;
  //   default:
  //     break;
  //   }
  //   break;
  // case StmtK:
  //   switch (t->kind.stmt) {
  //   case IfK:
  //     if (t->child[0]->type == Integer)
  //       typeError(t->child[0], "if test is not Boolean");
  //     break;
  //   case AssignK:
  //     if (t->child[0]->type != Integer)
  //       typeError(t->child[0], "assignment of non-integer value");
  //     break;
  //   case WriteK:
  //     if (t->child[0]->type != Integer)
  //       typeError(t->child[0], "write of non-integer value");
  //     break;
  //   case RepeatK:
  //     if (t->child[1]->type == Integer)
  //       typeError(t->child[1], "repeat test is not Boolean");
  //     break;
  //   default:
  //     break;
  //   }
  //   break;
  // default:
  //   break;
  // }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode *syntaxTree) {
  struct scopeListRec initialScopeList = getInitialScopeList();
  traverse(syntaxTree, nullProc, checkNode, initialScopeList);
}
