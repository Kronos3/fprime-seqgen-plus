%include {
    #include <iostream>
    #include <debug/print_debug.h>
    #include "cc.h"
    #include "compilation/context.h"
    #include "grammar/grammar.h"
}

%top {
    #define cc_ctx (static_cast<Context*>(yycontext))

    static
    void lexer_error_cb(void* context_v,
                        const char* input,
                        const ASTPosition* position,
                        const char* state_name)
    {
        Context* context = static_cast<Context*>(context_v);
        context->emit_error(position, "Unmatched token in state" + std::string(state_name));
    }

    static
    void parser_error_cb(void* context_v,
                         const char* const* token_names,
                         const TokenPosition* position,
                         uint32_t last_token,
                         uint32_t current_token,
                         const uint32_t expected_tokens[],
                         uint32_t expected_tokens_n)
    {
        Context* context = static_cast<Context*>(context_v);
        std::stringstream err_os;
        err_os << "expected ";

        for (int i = 0; i < expected_tokens_n; i++)
        {
            if (i > 0)
            {
                err_os << ", ";
                if (i + 1 == expected_tokens_n) err_os << "or ";
            }

            err_os << token_names[expected_tokens[i]];
        }

        err_os << " before " << token_names[current_token]
               << " (and after " << token_names[last_token] << ")";
        context->emit_error((ASTPosition*)position, err_os.str());
    }

    static inline char ll_handle_ascii(
                void* yycontext,
                const ASTPosition* position,
                const char* yytext)
    {
        Context* context = static_cast<Context*>(yycontext);
        if (yytext[1] != '\\')
        {
            return yytext[1];
        }
        else
        {
            // Handle all escape sequences supported in C
            switch(yytext[2])
            {
    #define HANDLE_ESCAPE(n, c) case (n): \
                return (c); \
                break
            HANDLE_ESCAPE('a', '\a');
            HANDLE_ESCAPE('b', '\b');
    //        HANDLE_ESCAPE('e', '\e');
            HANDLE_ESCAPE('f', '\f');
            HANDLE_ESCAPE('n', '\n');
            HANDLE_ESCAPE('r', '\r');
            HANDLE_ESCAPE('t', '\t');
            HANDLE_ESCAPE('v', '\v');
            HANDLE_ESCAPE('\\', '\\');
            HANDLE_ESCAPE('\'', '\'');
            HANDLE_ESCAPE('"', '"');
            HANDLE_ESCAPE('?', '?');
            default:
                context->emit_error(position,
                                    "unhandled escape sequence '\\%c'",
                                    yytext[2]);
                return 0;
    #undef HANDLE_ESCAPE
            }
        }
    }
}

%option parser_type="LALR(1)"
%option prefix="cc"
%option annotate_line="TRUE"
%option track_position_type="ASTPosition"
%option parsing_stack_size="4096"
%option parsing_error_cb="parser_error_cb"
%option lexing_error_cb="lexer_error_cb"

%union {
    char ascii;
    char* identifier;
    int64_t integer;
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
    FieldDecl* fields;
    StructDecl* structure;
}

%destructor <identifier> { free($$); }
%destructor <global>     { delete $$; }
%destructor <f_args>     { delete $$; }
%destructor <function>   { delete $$; }
%destructor <stmt>       { delete $$; }
%destructor <loop>       { delete $$; }
%destructor <if_clause>  { delete $$; }
%destructor <multi_stmt> { delete $$; }
%destructor <args>       { delete $$; }
%destructor <expr>       { delete $$; }
%destructor <v_decl>     { delete $$; }

%token <ascii> ASCII
%token <identifier> IDENTIFIER
%token <identifier> LITERAL
%token <integer> INTEGER
%token <floating> FLOATING

// Keywords
%token IF ELSE FOR WHILE CONTINUE BREAK RETURN

// Operators
%token EQ NE GT GE LT LE L_AND L_OR INC DEC
%token '.' PTR

// ASCII
%token '(' ')' '+' '-' '*' '/' '{' '}' '!' ':' ',' ';' '=' '&' '|' '^' '~'
%token<type> PRIMITIVE
%token SR SL

%left '+' '-'
%right '*' '/' '='
%left ',' '^' '|' '&' L_AND L_OR
%left LE LT GT GE EQ '('
%left IDENTIFIER
%right QUALIFIER
%right INC DEC SR SL

%type<function> function
%type<f_args> f_args
%type<type> type
%type<qualifier_prim> qual
%type<fields> fields_decl
%type<structure> struct_decl
%type<args> args
%type<expr> expr expr_ expr_first expr_primary integral_expr
%type<v_decl> v_decl
%type<stmt> error_types open_stmt closed_stmt simple_stmt decl_stmt
%type<stmt> bracket_stmt stmt case_stmt
%type<multi_stmt> multi_stmt
%type<loop> loop_header
%type<if_clause> if_clause
%type<global> global prog
%start<global> prog

+identifier             [A-Za-z_][A-Za-z_0-9]*
+ascii                  '([\x20-\x7E]|\\.)'

==

"[ \n\t\r\\]+"          { /* skip whitespace */ }
"//[^\n]*"              { /* skip line comments */ }
"=="                    { return EQ; }
"!="                    { return NE; }
">="                    { return GE; }
"<="                    { return LE; }
"\&\&"                  { return L_AND; }
"\|\|"                  { return L_OR; }
"<<"                    { return SL; }
">>"                    { return SR; }
">"                     { return GT; }
"<"                     { return LT; }
"\."                    { return '.'; }
"->"                    { return PTR; }
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
"\{"                    { return '{'; }
"\}"                    { return '}'; }
"\"(\\.|[^\"\\])*\""    { yyval->identifier = strndup(yytext + 1, yylen - 2); return LITERAL; }
"[0-9]+\.[0-9]*"        { yyval->floating = strtod(yytext, NULL); return FLOATING; }
"[0-9]+"                { yyval->integer = strtol(yytext, NULL, 0); return INTEGER; }
"{identifier}"          {
                            int keyword_i = handle_keyword(cc_ctx, yytext, yyval);
                            if (keyword_i >= 0) return keyword_i;
                            yyval->identifier = strdup(yytext);
                            return IDENTIFIER;
                        }
"{ascii}"               { yyval->ascii = ll_handle_ascii(yycontext, yyposition, yytext); return ASCII; }

==

%%
prog: global prog      { $$ = $1; $$->next = $2; }
    | global           { $$ = $1; }
    ;

global:
    function                { $$ = $1; }
    | decl_stmt ';'         { $$ = new ASTGlobalVariable(dynamic_cast<Decl*>($1)); }
    ;

v_decl: TYPENAME IDENTIFIER     { $$ = new TypeDecl($p2, $1, $2); }
      ;

if_clause: IF '(' expr ')'      { $$ = new If($3); }
    ;

// We want an abstract loop construct so that we can handle
// continue and break the same
loop_header:
    FOR '(' simple_stmt expr ';' expr ')'       { $$ = new ForLoop($3, $4, $6); }
  | WHILE '(' expr ')'                          { $$ = new WhileLoop($3); }
  ;

multi_stmt:
    stmt multi_stmt     { if ($1) { $$ = new MultiStatement($1); $$->next = $2; } else { $$ = $2; } }
  | stmt                { $$ = new MultiStatement($1); }
  ;

bracket_stmt:
      '{' multi_stmt '}'  { $$ = $2; }
    | '{' '}'             { $$ = nullptr; }
    ;

stmt: open_stmt
    | closed_stmt
    ;

// The open/closed statement shenanigans is
// meant to make the if/else syntax non-ambigious
// See http://www.parsifalsoft.com/ifelse.html
open_stmt:
    if_clause stmt                          { $$ = $1; $1->then_stmt = $2; }
  | if_clause closed_stmt ELSE open_stmt    { $$ = $1; $1->then_stmt = $2; $1->else_stmt = $4; }
  | loop_header open_stmt                   { $$ = $1; $1->body = $2; }
  ;

decl_stmt:
    v_decl '=' expr             { $$ = new DeclInit($1, $3); } // initializer statement
    | v_decl                    { $$ = new Decl($1); } // declaration statement
    ;

simple_stmt:
      decl_stmt ';'             { $$ = $1; }
    | expr      ';'             { $$ = new Eval($1); }
    ;

closed_stmt:
      simple_stmt               { $$ = $1; }
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
      type IDENTIFIER '(' f_args ')' '{' multi_stmt '}' { $$ = new ASTFunctionDefine($p1, *$p8, $1, $2, $4, $7); }
    | type IDENTIFIER '(' ')' '{' multi_stmt '}'        { $$ = new ASTFunctionDefine($p1, *$p7, $1, $2, nullptr, $6); }
    | type IDENTIFIER '(' f_args ')' ';'                { $$ = new ASTFunction($p1, $1, $2, $4); }
    | type IDENTIFIER '(' ')' ';'                       { $$ = new ASTFunction($p1, $1, $2, nullptr); }
    ;

// Argument list passed to a function/command
args:
      expr ',' args             { $$ = new CallArguments($1); $$->next = $3; }
    | expr                      { $$ = new CallArguments($1); }
    ;


// These expressions allow us to create an order of
// operations as needed. These are reduced before the
// main expressions are reduced

expr_primary:
      '(' expr ')'                  { $$ = $2; }
    | IDENTIFIER '(' args ')'       { $$ = new CallExpr($p1, $1, $3); }   // Function call (no pointer/indirect calls)
    | IDENTIFIER '(' ')'            { $$ = new CallExpr($p1, $1, nullptr); }   // Function call (no pointer/indirect calls)
    ;

integral_expr: INTEGER      { $$ = new NumericExpr($p1, NumericExpr::INTEGER, $1); }
             | ASCII        { $$ = new NumericExpr($p1, NumericExpr::ASCII, $1); }
             ;

expr_first:
      IDENTIFIER            { $$ = new VariableExpr($p1, $1); }
    | FLOATING              { $$ = new NumericExpr($p1, NumericExpr::FLOATING, $1); }
    | integral_expr
    | LITERAL               { $$ = new LiteralExpr($p1, $1); }
    | expr_ '/' expr_       { $$ = new BinaryExpr($1, $3, BinaryExpr::A_DIV); }
    | expr_ '*' expr_       { $$ = new BinaryExpr($1, $3, BinaryExpr::A_MUL); }
    ;

// These secondary expressions take lower precedence than the
// expr_first expressions
expr_:
      expr_primary           { $$ = $1; }
    | expr_first             { $$ = $1; }
    | expr_ LT expr_         { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::L_LT); }
    | expr_ GT expr_         { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::L_GT); }
    | expr_ LE expr_         { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::L_LE); }
    | expr_ GE expr_         { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::L_GE); }
    | expr_ EQ expr_         { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::L_EQ); }
    | expr_ '+' expr_        { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::A_ADD); }
    | expr_ '-' expr_        { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::A_SUB); }
    | expr_ '&' expr_        { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::B_AND); }
    | expr_ '|' expr_        { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::B_OR); }
    | expr_ '^' expr_        { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::B_XOR); }
    | '~' expr_              { $$ = UnaryExpr::reduce(cc_ctx, $2, UnaryExpr::B_NOT); }
    | '!' expr_              { $$ = UnaryExpr::reduce(cc_ctx, $2, UnaryExpr::L_NOT); }
    | INC expr_              { $$ = UnaryExpr::reduce(cc_ctx, $2, UnaryExpr::INC_PRE); }
    | DEC expr_              { $$ = UnaryExpr::reduce(cc_ctx, $2, UnaryExpr::DEC_PRE); }
    | expr_ INC              { $$ = UnaryExpr::reduce(cc_ctx, $1, UnaryExpr::INC_POST); }
    | expr_ DEC              { $$ = UnaryExpr::reduce(cc_ctx, $1, UnaryExpr::DEC_POST); }
    | expr_ L_AND expr_      { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::L_AND); }
    | expr_ L_OR expr_       { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::B_OR); }
    | expr_ SR expr_         { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::S_RIGHT); }
    | expr_ SL expr_         { $$ = BinaryExpr::reduce(cc_ctx, $1, $3, BinaryExpr::S_LEFT); }
    ;

// These tertiary expressions will evaluate/reduce last
// The '=' is meant to take lowest precedence so that it
// ends up at the highest point in the AST
expr: expr_                  {
                                 try
                                 {
                                     $$ = new ConstantExpr($p1, $1->get_constant(cc_ctx));
                                     delete $1;
                                 }
                                 catch (ASTException& exp) { $$ = $1; }
                             }
    | expr '=' expr_         { $$ = new AssignExpr($1, $3); }
    ;

%%
