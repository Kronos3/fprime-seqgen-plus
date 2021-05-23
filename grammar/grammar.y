%top {
    #include "cc.h"
    #include "compilation/context.h"
    #include "grammar/grammar.h"

    namespace cc { extern Context* cc_ctx; }
}

%option parser_type="LALR(1)"
%option prefix="cc"
%option track_position="TRUE"
%option track_position_type="ASTValue::TokenPosition"
%option parsing_stack_size="4096"

%union {
    char* identifier;
    int integer;
    double floating;
    ASTGlobal* global;
    ASTFunction* function;
    Expression* expr;
    Arguments* f_args;
    CallArguments* args;
    TypeDecl* v_decl;
    const Type* type;
    Statement* stmt;
    MultiStatement* multi_stmt;
    If* if_clause;
    Loop* loop;
}

%destructor <identifier> { free($$); }
%destructor <global> { delete $$; }
%destructor <f_args> { delete $$; }
%destructor <function> { delete $$; }
%destructor <stmt> { delete $$; }
%destructor <loop> { delete $$; }
%destructor <if_clause> { delete $$; }
%destructor <multi_stmt> { delete $$; }
%destructor <args> { delete $$; }
%destructor <expr> { delete $$; }
%destructor <v_decl> { delete $$; }

%token <identifier> VARIABLE
%token <identifier> LITERAL
%token <integer> INTEGER
%token <floating> FLOATING
%token IF
%token ELSE
%token FOR
%token WHILE
%token CONTINUE
%token BREAK
%token RETURN
%token EQ
%token NE
%token GT
%token GE
%token LT
%token LE
%token L_AND
%token L_OR
%token INC
%token DEC
%token<type> TYPE
%token '('
%token ')'
%token '+'
%token '-'
%token '*'
%token '/'
%token '{'
%token '}'
%token '!'
%token ','
%token ';'
%token '='
%token '&'
%token '|'
%token '^'
%token '~'

%left '+'
%left '-'
%right '*'
%right '/'
%left ','
%left '^'
%left '|'
%left '&'
%left L_AND
%left L_OR
%right '='
%left LE
%left LT
%left GT
%left GE
%left EQ
%left '('
%right INC
%right DEC

%type<function> function
%type<f_args> f_args
%type<args> args
%type<expr> expr
%type<expr> expr_
%type<expr> expr_first
%type<v_decl> v_decl
%type<stmt> open_stmt
%type<stmt> closed_stmt
%type<stmt> simple_stmt
%type<multi_stmt> multi_stmt
%type<stmt> bracket_stmt
%type<stmt> stmt
%type<loop> loop_header
%type<if_clause> if_clause
%type<global> global
%type<global> prog
%start<global> prog

==

"[\n]"                  { position->line++; }
"[ \t\r\\]+"            { /* skip */ }
"=="                    { return EQ; }
"!="                    { return NE; }
">="                    { return GE; }
"<="                    { return LE; }
"\&\&"                  { return L_AND; }
"\|\|"                  { return L_OR; }
">"                     { return GT; }
"<"                     { return LT; }
"!"                     { return '!'; }
";"                     { return ';'; }
","                     { return ','; }
"\+\+"                  { return INC; }
"--"                    { return DEC; }
"="                     { return '='; }
"\|"                    { return '|'; }
"\&"                    { return '&'; }
"\^"                    { return '^'; }
"\("                    { return '('; }
"\)"                    { return ')'; }
"\+"                    { return '+'; }
"-"                     { return '-'; }
"\*"                    { return '*'; }
"/"                     { return '/'; }
"{"                     { return '{'; }
"}"                     { return '}'; }
"\"(\\.|[^\"\\])*\""    { yyval->identifier = strndup(yytext + 1, len - 2); return LITERAL; }
"[0-9]+\.[0-9]*"        { yyval->floating = strtod(yytext, NULL); return FLOATING; }
"[0-9]"                 { yyval->integer = strtol(yytext, NULL, 0); return INTEGER; }
"[A-Za-z_][A-Za-z_0-9]*" {
                            int keyword_i = handle_keyword(cc_ctx, yytext, yyval);
                            if (keyword_i >= 0) return keyword_i;
                            yyval->identifier = strdup(yytext);
                            return VARIABLE;
                        }

==

%%
prog: global prog      { $$ = $1; $$->next = $2; }
    | global           { $$ = $1; }
    ;

global:
    function                { $$ = $1; }
    | v_decl                { $$ = new ASTGlobalVariable($1); }
    | v_decl '=' expr_ ';'  { $$ = new ASTGlobalVariable($1, $3); }
    ;

v_decl: TYPE VARIABLE           { $$ = new TypeDecl($p2, $1, $2); }
        ;

if_clause: IF '(' expr ')'      { $$ = new If($3); }
    ;

// We want an abstract loop construct so that we can handle
// continue and break the same
loop_header:
    FOR '(' simple_stmt ';' expr ';' expr ')'  { $$ = new ForLoop($3, $5, $7); }
  | WHILE '(' expr ')'                         { $$ = new WhileLoop($3); }
  ;

multi_stmt:
    stmt multi_stmt { if ($1) { $$ = new MultiStatement($1); $$->next = $2; } else { $$ = $2; } }
  | stmt            { $$ = new MultiStatement($1); }
  ;

bracket_stmt:
    '{' multi_stmt '}'  { $$ = $2; }
  | '{' '}'             { $$ = nullptr; }
    ;

stmt:
    open_stmt       { $$ = $1; }
  | closed_stmt     { $$ = $1; }
  ;

// The open/closed statement shenanigans is
// meant to make the if/else syntax non-ambigious
// See http://www.parsifalsoft.com/ifelse.html
open_stmt:
    if_clause stmt                          { $$ = $1; $1->then_stmt = $2; }
  | if_clause closed_stmt ELSE open_stmt    { $$ = $1; $1->then_stmt = $2; $1->else_stmt = $4; }
  | loop_header open_stmt                   { $$ = $1; $1->body = $2; }
  ;

simple_stmt:
    v_decl '=' expr             { $$ = new DeclInit($1, $3); } // initializer statement
    | v_decl                    { $$ = new Decl($1); } // declaration statement
    | expr                      { $$ = new Eval($1); }
    ;

closed_stmt:
      simple_stmt ';'           { $$ = $1; }
    | CONTINUE ';'              { $$ = new Continue($p1); }
    | BREAK    ';'              { $$ = new Break($p1); }
    | RETURN   ';'              { $$ = new Return($p1); }
    | RETURN  expr  ';'         { $$ = new Return($p1, $2); }
    | ';'                       { cc_ctx->emit_warning($p1, "Empty statement"); $$ = nullptr; }
    | bracket_stmt              { $$ = $1; }
    | loop_header closed_stmt   { $$ = $1; $1->body = $2; }
    | if_clause closed_stmt ELSE closed_stmt { $$ = $1; $1->then_stmt = $2; $1->else_stmt = $4; }
    ;


// Function arguments have the type in their
// declaration
f_args:
      v_decl ',' f_args         { $$ = new Arguments($1); $$->next = $3; }
    | v_decl                    { $$ = new Arguments($1); }
    ;

// Function declarations
function:
      TYPE VARIABLE '(' f_args ')' '{'
            multi_stmt
        '}'                                 { $$ = new ASTFunctionDefine($p1, $1, $2, $4, $7); }
    | TYPE VARIABLE '(' ')' '{'
            multi_stmt
        '}'                                 { $$ = new ASTFunctionDefine($p1, $1, $2, nullptr, $6); }
    | TYPE VARIABLE '(' f_args ')' ';'      { $$ = new ASTFunction($p1, $1, $2, $4); }
    | TYPE VARIABLE '(' ')' ';'             { $$ = new ASTFunction($p1, $1, $2, nullptr); }
    ;

// Argument list passed to a function/command
args:
      expr ',' args             { $$ = new CallArguments($1); $$->next = $3; }
    | expr                      { $$ = new CallArguments($1); }
    ;


// These expressions allow us to create an order of
// operations as needed. These are reduced before the
// main expressions are reduced
expr_first:
        INTEGER                 { $$ = new ImmIntExpr($p1, $1); }
      | FLOATING                { $$ = new ImmFloatExpr($p1, $1); }
      | LITERAL                 { $$ = new LiteralExpr($p1, $1); }
      | VARIABLE                { $$ = new VariableExpr($p1, $1); }
      | expr_ '/' expr_         { $$ = new BinaryExpr($1, $3, BinaryExpr::A_DIV); }
      | expr_ '*' expr_         { $$ = new BinaryExpr($1, $3, BinaryExpr::A_MUL); }
      | '(' expr ')'            { $$ = $2; }
      | VARIABLE '(' args ')'   { $$ = new CallExpr($p1, $1, $3); }   // Function call (no pointer/indirect calls)
      | VARIABLE '(' ')'        { $$ = new CallExpr($p1, $1, nullptr); }   // Function call (no pointer/indirect calls)
      ;

// These secondary expressions take lower precedence than the
// expr_first expressions
expr_: expr_first            { $$ = $1; }
    | expr_ LT expr_         { $$ = new BinaryExpr($1, $3, BinaryExpr::L_LT); }
    | expr_ GT expr_         { $$ = new BinaryExpr($1, $3, BinaryExpr::L_GT); }
    | expr_ LE expr_         { $$ = new BinaryExpr($1, $3, BinaryExpr::L_LE); }
    | expr_ GE expr_         { $$ = new BinaryExpr($1, $3, BinaryExpr::L_GE); }
    | expr_ EQ expr_         { $$ = new BinaryExpr($1, $3, BinaryExpr::L_EQ); }
    | expr_ '+' expr_        { $$ = new BinaryExpr($1, $3, BinaryExpr::A_ADD); }
    | expr_ '-' expr_        { $$ = new BinaryExpr($1, $3, BinaryExpr::A_SUB); }
    | expr_ '&' expr_        { $$ = new BinaryExpr($1, $3, BinaryExpr::B_AND); }
    | expr_ '|' expr_        { $$ = new BinaryExpr($1, $3, BinaryExpr::B_OR); }
    | expr_ '^' expr_        { $$ = new BinaryExpr($1, $3, BinaryExpr::B_XOR); }
    | '~' expr_              { $$ = new UnaryExpr($2, UnaryExpr::B_NOT); }
    | '!' expr_              { $$ = new UnaryExpr($2, UnaryExpr::L_NOT); }
    | INC expr_              { $$ = new UnaryExpr($2, UnaryExpr::INC_PRE); }
    | DEC expr_              { $$ = new UnaryExpr($2, UnaryExpr::DEC_PRE); }
    | expr_ INC              { $$ = new UnaryExpr($1, UnaryExpr::INC_POST); }
    | expr_ DEC              { $$ = new UnaryExpr($1, UnaryExpr::DEC_POST); }
    | expr_ L_AND expr_      { $$ = new BinaryExpr($1, $3, BinaryExpr::L_AND); }
    | expr_ L_OR expr_       { $$ = new BinaryExpr($1, $3, BinaryExpr::B_OR); }
    ;

// These tertiary expressions will evaluate/reduce last
// The '=' is meant to take lowest precedence so that it
// ends up at the highest point in the AST
expr: expr_                  { $$ = $1; }
    | expr '=' expr_         { $$ = new AssignExpr($1, $3); }
    ;

%%
