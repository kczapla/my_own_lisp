#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"


#define LASSERT(args, cond, fmt, ...)				\
  if (!(cond))							\
    {								\
      lval* err = lval_err(fmt, ##__VA_ARGS__);			\
      lval_del(args);						\
      return err;						\
    }

#define LASSERT_NUM(func, args, num)					\
  LASSERT(args, args->count == num,					\
	  "Function '%s' passed too many arguments. \ "			\
	  "Got %i, Excpected %i.",					\
	  func, args->count, num);					\

#define LASSERT_TYPE(func, args, index, expect)				\
  LASSERT(args, args->cell[index]->type == expect,			\
	  "Function '%s' passed incorrect type.\ "			\
	  "Got %s, Exptected %s",					\
	  func, ltype_name(args->cell[index]->type),			\
	  ltype_name(expect));						\

#define LASSERT_NOT_EMPTY(func, args, index)			\
  LASSERT(args, args->cell[index]->count != 0,			\
	  "Function '%s' passed empty args!",			\
	  func);						\


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

// Forward declarations
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
// Lbuiltin is pointer to the function wich args are pointers to lenv and lval
// and returns pointer to lval
typedef lval*(*lbuiltin)(lenv*, lval*);

// Enum for lval possible values
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN };

// Values structure
struct lval
{
  // Basic
  int type;
  long num;
  char* err;
  char* sym;

  // Functions
  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;
  
  // Expression
  int count;
  lval** cell;
};

struct lenv
{
  int count;
  char** syms;
  lval** vals;
};

lval* builtin_add(lenv* e, lval* a);
lval* builtin_cons(lenv* e, lval* a);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_div(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_init(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_len(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);

lval* lval_copy(lval* v);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_err(char *fmt, ...);
lval* lval_eval(lenv* e, lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_fun(lbuiltin func);
lval* lval_join(lval* x, lval* y);
lval* lval_pop(lval* v, int i);
lval* lval_sym(char* s);
lval* lval_take(lval* v, int i);
void lval_del(lval* v);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);

lenv* lenv_copy(lenv* e);

char* ltype_name(int t);

lenv* lenv_new(void)
{
  lenv* e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del(lenv* e)
{
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

lval* lenv_get(lenv* e, lval* k)
{
  // Iterate all over the items in enviroment
  for (int i = 0; i < e->count; i++)
    {
    if (strcmp(e->syms[i], k->sym) == 0)
      {
	return lval_copy(e->vals[i]);
      }
    }
  // No symbol found, return error
  return lval_err("Unbound Symbol '%s'!", k->sym);
}

void lenv_put(lenv* e, lval* k, lval* v)
{
  // iterate over all items in the enviroment
  // to see whereas variable already exists
  for (int i = 0; i < e->count; i++)
    {
    // if found delete it and replace with new
    if (strcmp(e->syms[i], k->sym) == 0)
      {
	lval_del(e->vals[i]);
	e->vals[i] = lval_copy(v);
	return;
      }
    }

  // If no entry, allocate mem for a new one
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  // Now copy val and its new symbol to allocated space
  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func)
{
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv* e)
{
    // List of functions
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);

    // Add variables
    lenv_add_builtin(e, "def", builtin_def);
			
    // Mathematical funcions
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
}
    

// Contruct a pointer to a new Number lval
lval* lval_num(long x)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_lambda(lval* formals, lval* body)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;

  // Set builtin to null
  v->builtin = NULL;
  // Build new enviroment
  v->env = lenv_new();
  // Set formals and body
  v->formals = formals;
  v->body = body;
  return v;
}
 
// make a new func
lval* lval_fun(lbuiltin func)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = func;
  return v;
}

// Construct a pointer to a new error type lval
lval* lval_err(char* fmt, ...)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  // Create a va list and init it
  va_list va;
  va_start(va, fmt);
  // Alloc buffer
  v->err = malloc(512);
  // Print error string with 511 chars only
  vsnprintf(v->err, 511, fmt, va);
  // Realocate the size of the error msg
  v->err = realloc(v->err, strlen(v->err) + 1);
  // Clean up the va list
  va_end(va);

  return v;
}

// Construct a pointer to a new symbol type lval
lval* lval_sym(char* s)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_sexpr(void)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

// Delete the structure
void lval_del(lval* v)
{
  switch (v->type)
    {
    case LVAL_NUM: break;
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    case LVAL_FUN:
      if (!v->builtin)
	{
	  lenv_del(v->env);
	  lval_del(v->formals);
	  lval_del(v->body);
	}
      break;
    // If Qexpr or Sexpr then delete all elements inside
    case LVAL_QEXPR:  
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++)
	{
	  lval_del(v->cell[i]);
	}
      free(v->cell);
      break;
    }
  free(v);
}


lval* lval_read_num(mpc_ast_t* t)
{
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}


lval* lval_add(lval* v, lval* x)
{
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_add_front(lval* v, lval* x)
{
  // Number of elements before realloc
  int cells_no = v->count;
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  memmove(&v->cell[1], &v->cell[0], sizeof(lval*) * cells_no);
  v->cell[0] = x;
  return v;
}

lval* lval_read(mpc_ast_t* t)
{
  // If symbol or number return conversion to that type
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  // If root (>) or sexpr then create empty list
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

  for (int i = 0; i < t->children_num; i++)
    {
      if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
      if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
      if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
      if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
      if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
      x = lval_add(x, lval_read(t->children[i]));
    }
  return x;
}

// Print lval
void lval_print(lval* v)
{
  switch (v->type)
    {
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_FUN:
      if (v->builtin) { printf("<builtin>"); break; }
      else
	{
	  printf("(\\ ");
	  lval_print(v->formals);
	  putchar(' ');
	  lval_print(v->body);
	  putchar(')');
	}
      break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    }
}

lval* lval_copy(lval* v)
{
  lval* x = malloc(sizeof(lval));
  x->type = v->type;
  switch (v->type)
    {
    case LVAL_NUM: x->num = v->num; break;
    case LVAL_FUN:
      if (v->builtin) { x->builtin = v->builtin; break; }
      else
	{
	  x->builtin = NULL;
	  x->env = lenv_copy(v->env);
	  x->formals = lval_copy(v->formals);
	  x->body = lval_copy(v->body);
	}
      break;
    case LVAL_ERR:
      x->err = malloc(strlen(v->err) + 1);
      strcpy(x->err, v->err); break;
    case LVAL_SYM:
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym); break;
    case LVAL_SEXPR: 
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);
      for (int i = 0; i < x->count; i++)
	{
	  x->cell[i] = lval_copy(v->cell[i]);
	}
      break;
    }
  return x;
}

void lval_println(lval* v)
{
  lval_print(v);
  putchar('\n');
}

void lval_expr_print(lval* v, char open, char close)
{
  putchar(open);
  for (int i = 0; i < v->count; i++)
    {
      // Print value contained within
      lval_print(v->cell[i]);
      if (i != (v->count-1))
	{
	  putchar(' ');
	}
    }
  putchar(close);
}

lval* builtin_lambda(lenv* e, lval* a)
{
  LASSERT_NUM("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  /* Check first Q-Expression contains only Symbols */
  for (int i = 0; i < a->cell[0]->count; i++)
    {
    LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
      "Cannot define non-symbol. Got %s, Expected %s.",
      ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }

  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 1);
  lval_del(a);

  return lval_lambda(formals, body);
}

lval* builtin_op(lenv* e, lval* a, char* op)
{
  // Ensure all arguments are number
  for (int i = 0; i < a->count; i++)
    {
      if (a->cell[i]->type != LVAL_NUM)
	{
	  lval_del(a);
	  return lval_err("Cannot operate on non-number");
	}
    }

  // Pop the first element
  lval* x = lval_pop(a, 0);

  // If no arguments and sub then perform unary negation
  if ((strcmp(op, "-") == 0) && a->count == 0)
    {
      x->num = -x->num;
    }

  while (a->count > 0)
    {
      lval* y = lval_pop(a, 0);
    
      if (strcmp(op, "+") == 0) { x->num += y->num; }
      else if (strcmp(op, "-") == 0) { x->num -= y->num; }
      else if (strcmp(op, "*") == 0) { x->num *= y->num; }
      else  if (strcmp(op, "/") == 0)
	{
	  if (y->num == 0)
	    {
	      lval_del(x); lval_del(y);
	      x = lval_err("Division by zero!");
	      break;
	    }
	  x->num /= y->num;
	}
      lval_del(y);
    }
  lval_del(a);
  return x;
}

lval* builtin_add(lenv* e, lval* a)
{
    return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a)
{
    return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a)
{
    return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a)
{
    return builtin_op(e, a, "/");
}

lval* builtin_def(lenv* e, lval* a)
{
  LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

  // First argument is symbol list
  lval* syms = a->cell[0];

  // Ensure all elements of first list are symbols
  for (int i = 0; i < syms->count; i++)
    {
      LASSERT_TYPE("def", syms, i, LVAL_SYM);
    }

  LASSERT(a, syms->count == a->count-1, 
	    "Function def cannot define incorrect "
	    "number of values symbols!");

    // Assign copies of values to symbols
    for (int i = 0; i < syms->count; i++)
    {
	lenv_put(e, syms->cell[i], a->cell[i+1]);
    }

    lval_del(a);
    return lval_sexpr();
}
  
lval* builtin_head(lenv* e, lval* a)
{
  LASSERT_NUM("head", a, 1);
  LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("head", a, 0);

  lval* v = lval_take(a, 0);
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

lval* builtin_tail(lenv* e, lval* a)
//  Function returns qexpr without first element
{
  LASSERT_NUM("tail", a, 1);
  LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("tail", a, 0);

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_init(lenv* e, lval* a)
//  Function returns qexpr without first element
{
  LASSERT_NUM("init", a, 1);
  LASSERT_TYPE("init", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("init", a, 0);

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, v->count - 1));
  return v;
}

lval* builtin_len(lenv* e, lval* a)
//  Function returns qexpr without first element
{
  LASSERT_NUM("len", a, 1);
  LASSERT_TYPE("len", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("len", a, 0);

  lval* v = lval_take(a, 0);
  lval* x = lval_num(v->count);
  return x;
}

lval* builtin_cons(lenv* e, lval* a)
//  Function returns qexpr without first element
{
  LASSERT_NUM("cons", a, 2);
  LASSERT_TYPE("cons", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("cons", a, 1, LVAL_NUM);
  LASSERT_NOT_EMPTY("cons", a, 0);

  lval* v = lval_pop(a, 0);
  lval* x = lval_pop(a, 0);
  lval_add_front(v, x);
  return v;
}

lval* builtin_list(lenv* e, lval* a)
{
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a)
{
  LASSERT_NUM("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a)
{
  for (int i = 0; i < a->count; i++)
    {
      LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

  lval* x = lval_pop(a, 0);
  while (a->count)
    {
      x = lval_join(x, lval_pop(a, 0));
    }

  lval_del(a);
  return x;
}

lval* lval_join(lval* x, lval* y)
{
  while (y->count)
    {
      x = lval_add(x, lval_pop(y, 0));
    }
  // Delete the empty 'y' and return 'x'
  lval_del(y);
  return x;
}

lval* lval_pop(lval* v, int i)
{
  // Find the item at "i"
  lval* x = v->cell[i];

  // Shift memory after the item at "i" over the top
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
  // Decrease the count of items in the list
  v->count--;

  //Reallocate the memory used
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}


lval* lval_take(lval* v, int i)
{
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}


lval* lval_eval(lenv* e, lval* v)
{
  if (v->type == LVAL_SYM){
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  
  if (v->type == LVAL_SEXPR)
    {
      return lval_eval_sexpr(e, v);
    }
  return v;
}


lval* lval_eval_sexpr(lenv* e, lval* v)
{
  // Evaluate children
  for (int i = 0; i < v->count; i++)
    {
      v->cell[i] = lval_eval(e, v->cell[i]);
    }

  // Error Checking
  for (int i = 0; i < v->count; i++)
    {
      if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

  // Empty expression
  if (v->count == 0) { return v; }

  // Single Expression
  if (v->count == 1) { return lval_take(v, 0); }

  // Ensure first element is a symbol
  lval * f = lval_pop(v, 0);
  if (f->type != LVAL_FUN)
    {
      lval_del(f);
      lval_del(v);
      return lval_err("first element is not a function!");
    }

  // Call built-in with operator
  lval* result = f->builtin(e, v);
  lval_del(f);
  return result;
}

char* ltype_name(int t)
{
  switch(t)
    {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
    }
}
      


int main(int argc, char** argv)
{
  // Parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // Define parser with language
  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                   \
              number : /-?[0-9]+/;	                        \
              symbol: /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/;         \
              sexpr  : '(' <expr>* ')';                         \
              qexpr  : '{' <expr>* '}';                         \
              expr   : <number> | <symbol> | <sexpr> | <qexpr> ;\
              lispy  : /^/ <expr>* /$/;				\
            ",
	    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  /* Print Version and Exit Infromation */
  puts("Lisp Version 0.0.0.0.1");
  puts("Press Ctrl+c to exit\n");

  lenv* e = lenv_new();
  lenv_add_builtins(e);
  
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
	  mpc_ast_print(r.output);
	  lval* result = lval_eval(e, lval_read(r.output));
	  lval_println(result);
	  lval_del(result);
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
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  lenv_del(e);
  return 0;
}


