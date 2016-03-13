#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"


#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// Fake readline function
char* readline(char* prompt)
{
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy{strlen(cpy)-1] = '\0';

  return cpy;
}

// Fake add_sitory function
void add_history(char* unused) {}

// Otherwise include editline headers
#else
#include <editline/readline.h>
#include <histedit.h>
#endif


// Enum for lval possible values
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR};

// Enum for possible error types
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};


// Values structure
typedef struct lval
{
  int type;
  long num;
  char* err;
  char* sym;
  // Count and pointer to a lost of "lval"
  int count;
  struct lval** cell;
} lval;


// Contruct a pointer to a new Number lval
lval lval_num(long x)
{
  lval* v =
    
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

// Creates a new error type lval
lval lval_err(int x)
{
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

// Print lval
void lval_print(lval v)
{
  switch (v.type)
  {
    // In the case the type is number
    case LVAL_NUM: printf("%li", v.num); break;

    // In the case the type is error
    case LVAL_ERR:
      // Check the type of error
      if (v.err == LERR_DIV_ZERO)
      {
	printf("Error: Division by zero!");
      }
      else if (v.err == LERR_BAD_OP)
      {
	printf("Error: Invalid operator!");
      }
      else if (v.err == LERR_BAD_NUM)
      {
	printf("Error: Invalid Number!");
      }
      break;
  }
}


void lval_println(lval v)
{
  lval_print(v);
  putchar('\n');
}
	  
      


lval eval_op(lval x, char* op, lval y)
{
  if (x.type == LVAL_ERR) { return x; }
  else if (y.type == LVAL_ERR) { return y; }

  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  else if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  else if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  else  if (strcmp(op, "/") == 0)
  {
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  }
  return lval_err(LERR_BAD_OP);
}


lval eval(mpc_ast_t* t)
{
  // If tagged as number return it directly
  if (strstr(t->tag, "number"))
  {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }
  // The operator is always second child
  char* op = t->children[1]->contents;

  // Third child of node
  lval x = eval(t->children[2]);

  // Iterate the remaining children and combining
  int i = 3;
  while (strstr(t->children[i]->tag, "expr"))
  {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}


int main(int argc, char** argv)
{
  // Parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // Define parser with language
  mpca_lang(MPCA_LANG_DEFAULT,
	    "\
              number : /-?[0-9]+/; \
              symbol : '+' | '-' | '*' | '/' ; \
              expr : <number> |  '(' <operator> <expr>+ ')'; \
              lispy : /^/<operator> <expr>+/$/; \
            ",
	    Number, Symbol, Sexpr, Expr, Lispy);
  /* Print Version and Exit Infromation */
  puts("Lisp Version 0.0.0.0.1");
  puts("Press Ctrl+c to exit\n");

  while (1)
  {
    // Output our prompt
    char * input = readline("lispy> ");
    // Add input to history
    add_history(input);

    // Attempt to prase the user input
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r))
    {
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    }
    else
    {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
	
    // Free input
    free(input);
  }
  mpc_cleanup(5, Number, Operator, Sexpr, Expr, Lispy);
  return 0;
}
