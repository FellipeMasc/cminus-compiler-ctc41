/****************************************************/
/* File: util.c                                     */
/* Utility function implementation                  */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "util.h"
#include "../build/parser.h"
#include "globals.h"

/* Procedure printToken prints a token
 * and its lexeme to the listing file
 */
void printToken(TokenType token, const char *tokenString) {
  switch (token) {
  case IF:
  case THEN:
  case ELSE:
  case END:
  case REPEAT:
  case UNTIL:
  case READ:
  case WRITE:
  case VOID:
  case INT:
  case WHILE:
  case RETURN:
    pc("reserved word: %s\n", tokenString);
    break;
  case ASSIGN:
    pc(":=\n");
    break;
  case LT:
    pc("<\n");
    break;
  case GT:
    pc(">\n");
    break;
  case LTE:
    pc("<=\n");
    break;
  case GTE:
    pc(">=\n");
    break;
  case EQ:
    pc("=\n");
    break;
  case EQQ:
    pc("==\n");
    break;
  case NEQ:
    pc("!=\n");
    break;
  case LPAREN:
    pc("(\n");
    break;
  case RPAREN:
    pc(")\n");
    break;
  case LBRACE:
    pc("{\n");
    break;
  case RBRACE:
    pc("}\n");
    break;
  case LBRACKET:
    pc("[\n");
    break;
  case RBRACKET:
    pc("]\n");
    break;
  case SEMI:
    pc(";\n");
    break;
  case COMMA:
    pc(",\n");
    break;
  case PLUS:
    pc("+\n");
    break;
  case MINUS:
    pc("-\n");
    break;
  case TIMES:
    pc("*\n");
    break;
  case OVER:
    pc("/\n");
    break;
  case 0:
    pc("EOF\n");
    break;
  case NUM:
    pc("NUM, val= %s\n", tokenString);
    break;
  case ID:
    pc("ID, name= %s\n", tokenString);
    break;
  case ERROR:
    pce("ERROR: %s\n", tokenString);
    break;
  default: /* should never happen */
    pce("Unknown token: %d\n", token);
  }
}

/* Function newStmtNode creates a new statement
 * node for syntax tree construction
 */
TreeNode *newStmtNode(StmtKind kind) {
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;
  if (t == NULL)
    pce("Out of memory error at line %d\n", lineno);
  else {
    for (i = 0; i < MAXCHILDREN; i++)
      t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = StmtK;
    t->kind.stmt = kind;
    t->lineno = lineno;
  }
  return t;
}

/* Function newExpNode creates a new expression
 * node for syntax tree construction
 */
TreeNode *newExpNode(ExpKind kind) {
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;
  if (t == NULL)
    pce("Out of memory error at line %d\n", lineno);
  else {
    for (i = 0; i < MAXCHILDREN; i++)
      t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = ExpK;
    t->kind.exp = kind;
    t->lineno = lineno;
    t->type = "void";
  }
  return t;
}

/*Function newDeclNode*/
TreeNode *newDeclNode(DeclKind kind) {
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;
  if (t == NULL)
    pce("Out of memory error at line %d\n", lineno);
  else {
    for (i = 0; i < MAXCHILDREN; i++)
      t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = DeclK;
    t->kind.decl = kind;
    t->lineno = lineno;
  }
  return t;
}

/* Function copyString allocates and makes a new
 * copy of an existing string
 */
char *copyString(char *s) {
  int n;
  char *t;
  if (s == NULL)
    return NULL;
  n = strlen(s) + 1;
  t = malloc(n);
  if (t == NULL)
    pce("Out of memory error at line %d\n", lineno);
  else
    strcpy(t, s);
  return t;
}

/* Variable indentno is used by printTree to
 * store current number of spaces to indent
 */
static int indentno = -4;

/* macros to increase/decrease indentation */
#define INDENT indentno += 4
#define UNINDENT indentno -= 4

/* printSpaces indents by printing spaces */
static void printSpaces(void) {
  int i;
  for (i = 0; i < indentno; i++)
    pc(" ");
}

/* procedure printTree prints a syntax tree to the
 * listing file using indentation to indicate subtrees
 */
void printTree(TreeNode *tree) {
  int i;
  INDENT;
  while (tree != NULL) {
    printSpaces();
    if (tree->nodekind == StmtK) {
      switch (tree->kind.stmt) {
      case IfK:
        pc("Conditional selection\n");
        break;
      case WhileK:
        pc("Iteration (loop)\n");
        break;
      case CompoundK:
        pc("Block\n");
        break;
      case ReturnK:
        pc("Return\n");
        break;
      default:
        pce("Unknown StmtNode kind\n");
        break;
      }
    } else if (tree->nodekind == ExpK) {
      switch (tree->kind.exp) {
      case OpK:
        pc("Op%s: ", strcmp(tree->type, "unary") == 0 ? "(unary)" : "");
        printToken(tree->attr.op, "\0");
        break;
      case ConstK:
        pc("Const: %d\n", tree->attr.val);
        break;
      case IdK:
        pc("Id: %s\n", tree->attr.name);
        break;
      case AssignK:
        pc("Assign to %s: %s\n", tree->isArray ? "array" : "var",
           tree->attr.name);
        break;
      case CallK:
        pc("Function call: %s\n", tree->attr.name);
        break;
      case VarK:
        pc("Id: %s\n", tree->attr.name);
        break;
      case TypeSpecK:
        pc("TypeSpec\n");
        break;
      default:
        pce("Unknown ExpNode kind\n");
        break;
      }
    } else if (tree->nodekind == DeclK) {
      switch (tree->kind.decl) {
      case VarDeclK:
        pc("Declare %s %s: %s\n", tree->type, tree->isArray ? "array" : "var",
           tree->attr.name);
        break;
      case FunDeclK:
        pc("Declare function (return type \"%s\"): %s\n", tree->typeReturn,
           tree->attr.name);
        break;
      case ParamK:
        pc("Function param (%s %s): %s\n", tree->type,
           tree->isArray ? "array" : "var", tree->attr.name);
        break;
      case TypeK:
        pc("TypeSpec: %s\n", tree->attr.name ? tree->attr.name : "");
        break;
      default:
        pce("Unknown DeclNode kind\n");
        break;
      }
    } else
      pce("Unknown node kind\n");
    for (i = 0; i < MAXCHILDREN; i++)
      printTree(tree->child[i]);
    tree = tree->sibling;
  }
  UNINDENT;
}

void printLine(FILE *redundant_source) {
  char line[1024];
  char *ret = fgets(line, 1024, redundant_source);
  if (ret) {
    pc("%d: %-1s", lineno, line);
    if (feof(redundant_source))
      pc("\n");
  }
}