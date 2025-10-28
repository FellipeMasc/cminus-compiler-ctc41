%{
#define YYPARSER
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static int savedMultOperator;  
static int savedSumOperator;
static int savedRelOperator;
static int saveNumber;
static char * savedFunctionName;
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void);
int yyerror(char *);

#define MAX_NAME_STACK 100
static char *nameStack[MAX_NAME_STACK];
static int nameStackTop = -1;

static void pushName(char *name) {
  nameStack[++nameStackTop] = name;
}

static char *popName() {
  return nameStack[nameStackTop--];
}

%}



%token IF THEN ELSE END REPEAT UNTIL READ WRITE VOID INT %token WHILE RETURN ASSIGN EQ EQQ NEQ LT GT LTE GTE PLUS %token MINUS TIMES OVER LPAREN RPAREN SEMI COMMA NUM ID %token ENDFILE
%token LBRACE RBRACE LBRACKET RBRACKET ERROR


%% /* Grammar for C- */
program : list_decl {savedTree = $1; };
list_decl : list_decl decl 
                {
                  YYSTYPE t = $1;
                  if (t != NULL) {
                    while (t->sibling != NULL) {
                      t = t->sibling;
                    }
                    t->sibling = $2;
                    $$ = $1;
                  } else {
                    $$ = $2;
                  }
                }
          | decl
                {
                  $$ = $1;
                }
        ;
decl : var_decl 
                {
                  $$ = $1;
                }
          | fun_decl
                {
                  $$ = $1;
                }
                ;
var_decl : type_spec ID { savedName = copyString(prevTokenString); } SEMI
      {
        $$ = newDeclNode(VarDeclK);
        $$->type = $1->type;
        $$->attr.name = savedName;
        $$->lineno = lineno;
        $$->isArray = 0;
      }
            | type_spec ID 
            { savedName = copyString(prevTokenString); }
            LBRACKET NUM 
            { saveNumber = atoi(tokenString); }  
            RBRACKET SEMI
                          {
                            $$ = newDeclNode(VarDeclK);
                            $$->attr.name = savedName;
                            $$->isArray = 1;
                            $$->type = $1->type;
                            $$->child[0] = newExpNode(ConstK);
                            $$->child[0]->attr.val = saveNumber;  
                            $$->lineno = lineno;
                          }
        ;
type_spec : INT 
                {
                  $$ = newExpNode(TypeSpecK);
                  $$->type = copyString(tokenString);
                }
          | VOID
                {
                  $$ = newExpNode(TypeSpecK);
                  $$->type = copyString(tokenString);
                }
        ;
fun_decl : type_spec ID 
           { savedFunctionName = copyString(prevTokenString); 
             savedLineNo = lineno; }
           LPAREN params RPAREN decl_compo
                {
                  $$ = newDeclNode(FunDeclK);
                  $$->attr.name = savedFunctionName;
                  $$->lineno = savedLineNo ;
                  $$->typeReturn = $1->type;
                  $$->child[0] = $5; 
                  $$->child[1] = $7;
                }
        ;
decl_compo : LBRACE local_decl list_stmt RBRACE
                {
                  //YYSTYPE t = $2;
                  //if (t != NULL) {
                   // while (t->sibling != NULL)
                    //  t = t->sibling;
                    //t->sibling = $3;
                   // $$ = $2;
                  //} else {
                    //$$ = $3;
                  //}
                  $$ = newStmtNode(CompoundK);
                  $$->child[0] = $2;
                  $$->child[1] = $3;
                }
        ;
params
  : list_params { $$ = $1; }
  | VOID        { $$ = NULL; }
  ;
list_params : list_params COMMA param 
                {
                  YYSTYPE t = $1;
                  if (t != NULL) {
                    while (t->sibling != NULL) {
                      t = t->sibling;
                    }
                    t->sibling = $3;
                    $$ = $1;
                  } else {
                    $$ = $3;
                  }
                }
          | param
                {
                  $$ = $1;
                }
        ;
param : type_spec ID 
                {
                  $$ = newDeclNode(ParamK);
                  $$->type = $1->type;
                  $$->attr.name = copyString(prevTokenString);
                  $$->isArray = 0;
                }
          | type_spec ID { savedName = copyString(prevTokenString); } LBRACKET RBRACKET
                {
                  $$ = newDeclNode(ParamK);
                  $$->type = $1->type;
                  $$->attr.name = savedName;
                  $$->isArray = 1;
                }
        ;
decl_block : LBRACE local_decl list_stmt RBRACE
                {
                  $$ = newStmtNode(CompoundK);
                  $$->child[0] = $2;
                  $$->child[1] = $3;
                }
        ;
local_decl : local_decl var_decl 
                {
                  YYSTYPE t = $1;
                  if (t != NULL) {
                    while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $2;
                    $$ = $1;
                  } else {
                    $$ = $2;
                  }
                }
          | 
                {
                  $$ = NULL;
                }
        ;
list_stmt : list_stmt stmt 
                {
                  YYSTYPE t = $1;
                  if (t != NULL) {
                    while (t->sibling != NULL) {
                      t = t->sibling;
                    }
                    t->sibling = $2;
                    $$ = $1;
                  } else {
                    $$ = $2;
                  }
                }
          | 
                {
                  $$ = NULL;
                }
              ;
stmt : exp_decl {$$ = $1;}
| decl_compo {$$ = $1;}
| decl_sel {$$ = $1;}
| decl_ite {$$ = $1;}
| decl_return {$$ = $1;}
| decl {$$ = $1;}
        ;
exp_decl : exp SEMI 
                {
                  $$ = $1;
                }
          | SEMI
                {
                  $$ = NULL;
                }
decl_sel : IF LPAREN exp RPAREN stmt 
                {
                  $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                }
          | IF LPAREN exp RPAREN stmt ELSE stmt
                {
                  $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                  $$->child[2] = $7;
                }
        ;
decl_ite : WHILE LPAREN exp RPAREN stmt
                {
                  $$ = newStmtNode(WhileK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                }
        ;
decl_return : RETURN SEMI 
                {
                  $$ = newStmtNode(ReturnK);
                }
          | RETURN exp SEMI
                {
                  $$ = newStmtNode(ReturnK);
                  $$->child[0] = $2;
                }
        ;
exp : var EQ exp 
                {
                  $$ = newExpNode(AssignK);
                  if($1->isArray) {
                    $$->child[0] = $1->child[0];
                    $$->child[1] = $3;
                  } else {
                    $$->child[0] = $3;
                  }
                  $$->isArray = $1->isArray;
                  $$->attr.name = $1->attr.name;
                }
          | simple_exp
                {
                  $$ = $1;
                }
        ;
var : ID 
                {
                  $$ = newExpNode(VarK);
                  $$->isArray = 0;
                  $$->attr.name = copyString(prevTokenString);
                }
      | ID { savedName = copyString(prevTokenString); } LBRACKET exp RBRACKET
      {
        $$ = newExpNode(VarK);
        $$->attr.name = savedName;
        $$->isArray = 1;
        $$->child[0] = $4;
      }
      ;
simple_exp : sum_exp rel sum_exp 
                {
                  $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = savedRelOperator;
                }
          | sum_exp
                {
                  $$ = $1;
                }
        ;
rel : LTE {savedRelOperator = LTE;}
| LT {savedRelOperator = LT;}
| GT {savedRelOperator = GT;}
| GTE {savedRelOperator = GTE;}
| EQQ {savedRelOperator = EQQ;}
| NEQ {savedRelOperator = NEQ;}
        ;
sum_exp : sum_exp PLUS term 
                {
                  $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = PLUS;
                }
          |
          sum_exp MINUS term 
                {
                  $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = MINUS;
                }
          |
          MINUS term
                {
                  $$ = newExpNode(OpK);
                  $$->child[0] = $2;
                  $$->attr.op = MINUS;
                  $$->type = "unary";
                }
          | term
                {
                  $$ = $1;
                }
        ;

term : term TIMES factor 
                {
                  $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = TIMES;
                }
                  |
term OVER factor 
                {
                  $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = OVER;
                }
          | factor
                {
                  $$ = $1;
                }
        ;
factor  : LPAREN exp RPAREN 
                {
                  $$ = $2;
                }
          | var 
                {
                  $$ = $1;
                }
          | ativ 
                {
                  $$ = $1;
                }
          | NUM 
                {
                  $$ = newExpNode(ConstK);
                  $$->attr.val = atoi(tokenString);
                }
        ;
ativ : ID 
       { pushName(copyString(prevTokenString)); }
       LPAREN args RPAREN
                {
                  $$ = newExpNode(CallK);
                  $$->attr.name = popName();
                  $$->child[0] = $4; 
                }
        ;

args : list_args 
                {
                  $$ = $1;
                }
          | 
                {
                  $$ = NULL;
                }
        ;

list_args : list_args COMMA exp 
                {
                  YYSTYPE t = $1;
                  if (t != NULL) {
                    while (t->sibling != NULL) {
                      t = t->sibling;
                    }
                    t->sibling = $3;
                    $$ = $1;
                  } else {
                    $$ = $3;
                  }
                }
          | exp
                {
                  $$ = $1;
                }
        ;

%%
int yyerror(char * message)
{ pce("Syntax error at line %d: %s\n",lineno,message);
  pce("Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ 
	yyparse();
  return savedTree;
}