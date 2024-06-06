/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
size_t string_buf_ptr = 0;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */
bool has_null=false;
bool is_max_len(){
  return string_buf_ptr>=MAX_STR_CONST;
}
void add2string_buf(char c){
  string_buf[string_buf_ptr++] = c;
}

#define set_err_msg(str)  yylval.error_msg = str
%}
/* add this option so that won't report error of undefined reference to yylex */
%option noyywrap 
%option stack
%option yylineno

%x COMMENT_STATE_LINE
%x COMMENT_STATE_LINES
%x STRING_STATE
/*
 * Define names for regular expressions here.
 */

DARROW          =>
DIGIT           [0-9]
INTEGER         [0-9]+                      
LETTER      		[A-Za-z]
UPPER_LETTER    [A-Z]
LOWER_LETTER    [a-z]
TYPE_IDENTIFIER ({UPPER_LETTER}(_|{LETTER}|{DIGIT})*)
OBJ_IDENTIFIER  ({LOWER_LETTER}(_|{LETTER}|{DIGIT})*)
IDENTIFIER      ((_|{LETTER})(_|{LETTER}|{DIGIT})*)
WHITESPACE      [ \f\r\t\v]*
COMMENT_ONELINE    "--"
COMMENT_LINES_BEG  "(*"
COMMENT_LINES_END  "*)"
STRING_BEG         \"
%%

 /*
  *  Nested comments
  */
{COMMENT_ONELINE}         { yy_push_state(COMMENT_STATE_LINE); }
{COMMENT_LINES_BEG}       { yy_push_state(COMMENT_STATE_LINES); }
{COMMENT_LINES_END}       { yylval.error_msg = "Unmatched *)"; return ERROR; }
{STRING_BEG}              { string_buf_ptr = 0; yy_push_state(STRING_STATE); }

<COMMENT_STATE_LINE>{
\n        { curr_lineno++; yy_pop_state();}
.             
}

<COMMENT_STATE_LINES>{
{COMMENT_LINES_BEG}						{yy_push_state(COMMENT_STATE_LINES);}
{COMMENT_LINES_END}						{yy_pop_state();}
<<EOF>>                       {yylval.error_msg = "EOF in comment"; yy_pop_state(); return ERROR;}
\n                            {curr_lineno++;}
.
}

<STRING_STATE>{
\"        { 
            if(is_max_len()) {
              set_err_msg("String constant too long"); 
              yy_pop_state(); 
              return ERROR; 
            } 
            else if(has_null){
              set_err_msg("String contains null character");
              yy_pop_state();
              return ERROR;
            }
            else{ 
              add2string_buf('\0'); 
              yylval.symbol = stringtable.add_string(string_buf); 
              yy_pop_state(); 
              return STR_CONST;
            } 
          }
\n        { 
            curr_lineno++;
            if(is_max_len()) { 
              set_err_msg("String constant too long"); 
            } 
            else if(has_null){
              set_err_msg("String contains null character");
            }
            else{ 
              set_err_msg("Unterminated string constant"); 
            } 
            yy_pop_state(); 
            return ERROR;
          }
\0        { has_null = true; }
<<EOF>>   {yylval.error_msg = "EOF in string constant"; yy_pop_state(); return ERROR;}
\\b       { if(is_max_len()) { /* do nothing here! */ } else{ add2string_buf('\b'); } }
\\t	      { if(is_max_len()) { } else{ add2string_buf('\t'); } }
\\n       { if(is_max_len()) { } else{ add2string_buf('\n'); } }
\\f	      { if(is_max_len()) { } else{ add2string_buf('\f'); } }
\\\0      { has_null = true; }
\\\n      { curr_lineno++; if(is_max_len()) { } else{ add2string_buf('\n'); } }            /* multiple line string,end with '\' */
\\.       { if(is_max_len()) { } else{ add2string_buf(yytext[1]); } }      /* '\' and other characters：\1,just Ignore \ */ 
[^\\\n\"]+    {
                std::string input(yytext, yyleng);
                if (input.find_first_of('\0') != std::string::npos) {
                  has_null=true;
                }else{
                  for(char c:input){
                    if(is_max_len()) { break; } 
                    else{
                      add2string_buf(c);
                    }
                  }
                }
              }
}
 /*
  *  The multiple-character operators.
  */
{DARROW}		return DARROW;
"<-"        return ASSIGN;
"<="        return LE;
"+"         return '+';
"/"         return '/';
"-"         return '-';
"*"         return '*';
"="         return '=';
"<"         return '<';
"."         return '.';
"~"         return '~';
","         return ',';
";"         return ';';
":"         return ':';
"("         return '(';
")"         return ')';
"@"         return '@';
"{"         return '{';
"}"         return '}';

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
(?i:class)  return CLASS;
(?i:else)   return ELSE;
t(?i:rue)   { yylval.boolean = 1; return BOOL_CONST; }
f(?i:alse)  { yylval.boolean = 0; return BOOL_CONST; }
(?i:if)     return IF;
(?i:fi)     return FI;
(?i:in)     return IN;
(?i:inherits)     return INHERITS;
(?i:isvoid)     return ISVOID;
(?i:let)     return LET;
(?i:loop)     return LOOP;
(?i:pool)     return POOL;
(?i:then)     return THEN;
(?i:while)     return WHILE;
(?i:case)     return CASE;
(?i:esac)     return ESAC;
(?i:new)     return NEW;
(?i:of)     return OF;
(?i:not)     return NOT;

{INTEGER}    { yylval.symbol = inttable.add_string(yytext); return INT_CONST;}
 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */
{TYPE_IDENTIFIER} { yylval.symbol = idtable.add_string(yytext); return TYPEID; }
{OBJ_IDENTIFIER}  { yylval.symbol = idtable.add_string(yytext); return OBJECTID; }
\n                {curr_lineno++;}
{WHITESPACE}+     /* Ignore WHITESPACE */
.                 {yylval.error_msg = yytext; return ERROR;}
%%
