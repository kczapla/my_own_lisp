#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"


#define LASSERT(args, cond, fmt, ...)                           \
  if (!(cond))                                                  \
    {                                                           \
      lval* err = lval_err(fmt, ##__VA_ARGS__);                 \
      lval_del(args);                                           \
      return err;                                               \
    }

#define LASSERT_NUM(func, args, num)                                    \
  LASSERT(args, args->count == num,                                     \
          "Function '%s' passed too many arguments. "                   \
          "Got %i, Excpected %i.",                                      \
          func, args->count, num);                                      \

#define LASSERT_TYPE(func, args, index, expect)                         \
  LASSERT(args, args->cell[index]->type == expect,                      \
          "Function '%s' passed incorrect type. "                       \
          "Got %s, Exptected %s",                                       \
          func, ltype_name(args->cell[index]->type),                    \
          ltype_name(expect));                                          \

#define LASSERT_NOT_EMPTY(func, args, index)                    \
  LASSERT(args, args->cell[index]->count != 0,                  \
          "Function '%s' passed empty args!",                   \
          func);                                                \


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
  cpy[strlen(cpy)-1] = '\0';

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
enum {LVAL_NUM, LVAL_ERR, LVAL_STRING, LVAL_BOOL, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN };

// Values structure
struct lval
{
  // Basic
  int type;
  long num;
  char* err;
  char* sym;
  char* str;
  

  // Functions
  lbuiltin builtin;
  lenv* env;
  // Symbols of the varibale ex. 'x=...'
  lval* formals;
  // Lines of code that will be executed in order when func is called
  lval* body;
  
  // Expression
  int count;
  lval** cell;
};

struct lenv
{
  lenv* par;
  int count;
  char** syms;
  lval** vals;
};

lval* builtin_add(lenv* e, lval* a);
lval* builtin_cmp(lenv* e, lval* a, char* op);
lval* builtin_cons(lenv* e, lval* a);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_div(lenv* e, lval* a);
lval* builtin_eq(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_ge(lenv* e, lval* a);
lval* builtin_gt(lenv* e, lval* a);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_init(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_lambda(lenv* e, lval* a);
lval* builtin_le(lenv* e, lval* a);
lval* builtin_len(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_lt(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* a);
lval* builtin_ne(lenv* e, lval* a);
lval* builtin_ord(lenv* e, lval* a, char* op);
lval* builtin_put(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_if(lenv* e, lval* a);
lval* builtin_var(lenv* e, lval* a, char* func);
lval* builtin_logic_op(lenv* e, lval* a, char* op);
lval* builtin_or(lenv* e, lval* a);
lval* builtin_and(lenv* e, lval* a);
lval* builtin_not(lenv* e, lval* a);
lval* builtin_or_sym(lenv* e, lval* a);
lval* builtin_and_sym(lenv* e, lval* a);
lval* builtin_not_sym(lenv* e, lval* a);


lval* lval_copy(lval* v);
lval* lval_err(char *fmt, ...);
lval* lval_eval(lenv* e, lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_fun(lbuiltin func);
lval* lval_join(lval* x, lval* y);
lval* lval_read_str(mpc_ast_t* t);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_pop(lval* v, int i);
lval* lval_sym(char* s);
lval* lval_boolean(long x, char* s);
lval* lval_take(lval* v, int i);
int lval_eq(lval* x, lval* y);
void lval_del(lval* v);
void lval_expr_print(lval* v, char open, char close);
void lval_print_str(lval* v);
void lval_print(lval* v);

lenv* lenv_copy(lenv* e);
void lenv_def(lenv* e, lval* k, lval* v);
void lenv_put(lenv* e, lval* k, lval* v);

char* ltype_name(int t);

lenv* lenv_new(void)
{
  lenv* e = malloc(sizeof(lenv));
  e->par = NULL;
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
  // Look for symbol in parent environment
  if (e->par)
    {
      return lenv_get(e->par, k);
    }
  // No symbol found, return error
  return lval_err("Unbound Symbol '%s'!", k->sym);
}

lenv* lenv_copy(lenv* e)
{
  lenv* n = malloc(sizeof(lenv));
  n->par = e->par;
  n->count = e->count;
  n->syms = malloc(sizeof(char*) * n->count);
  n->vals = malloc(sizeof(lval*) * n->count);
  for (int i = 0; i < e->count; i++)
    {
      n->syms[i] = malloc(strlen(e->syms[i])+1);
      strcpy(n->syms[i], e->syms[i]);
      n->vals[i] = lval_copy(e->vals[i]);
    }
  return n;
}

void lenv_def(lenv* e, lval* k, lval* v)
{
  // Iterate till the e has no parent
  while (e->par)
    {
      e = e->par;
    }
  lenv_put(e, k, v);
}

void lenv_put(lenv* e, lval* k, lval* v)
{
  // k - Variable symbol
  // v - what is this variable
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
    lenv_add_builtin(e, "\\", builtin_lambda);
    lenv_add_builtin(e, "=", builtin_put);

    // Comparision functions
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, ">", builtin_gt);
    lenv_add_builtin(e, "<", builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);

    // Logic functions
    lenv_add_builtin(e, "or", builtin_or);
    lenv_add_builtin(e, "||", builtin_or_sym);
    lenv_add_builtin(e, "not", builtin_not_sym);
    lenv_add_builtin(e, "!", builtin_not);
    lenv_add_builtin(e, "and", builtin_and);
    lenv_add_builtin(e, "&&", builtin_and_sym);
    
    // Mathematical funcions
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
}
    

// CONTRUCT a pointer to a new Number lval
lval* lval_num(long x)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

// Function makes default user function
// Formals are arugments of this function and body is body of it
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

lval* lval_boolean(long x, char* s)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_BOOL;
  v->num = x;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
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

lval* lval_str(char* s)
{
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_STRING;
  v->str = malloc(strlen(s) + 1);
  strcpy(v->str, s);
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
    case LVAL_BOOL:
    case LVAL_SYM: free(v->sym); break;
    case LVAL_STRING: free(v->str); break;
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

lval* lval_read_str(mpc_ast_t* t)
{
  // Cut off the final quote character
  t->contents[strlen(t->contents)-1] = '\0';
  // Copy the string missing out the first quote character
  char* unescaped = malloc(strlen(t->contents+1)+1);
  strcpy(unescaped, t->contents+1);
  // Pass thorugh the unescape func
  unescaped = mpcf_unescape(unescaped);
  // Construct a new lval using the string
  lval* str = lval_str(unescaped);
  // Free the string and return
  free(unescaped);
  return str;
}

lval* lval_read_boolean(char* s)
{
  if (strcmp(s, "True") == 0)
    {
      return lval_boolean(1, s);
    }
  else if (strcmp(s, "False") == 0)
    {
      return lval_boolean(0, s);
    }
  else
    {
      return lval_err("invalid booleanean value");
    }
}

int lval_eq(lval* x, lval* y)
{
  // Different types are always unequal
  if (x->type != y->type)
    {
      return 0;
    }

  // Comparision based upon the type
  switch (x->type)
    {
      //compare no value
    case LVAL_BOOL:
    case LVAL_NUM: return (x->num == y->num);
      
    // compare string value
    case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
    case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
    case LVAL_STRING: return (strcmp(x->str, y->str) == 0);

      // compare funcitons
    case LVAL_FUN:
      if (x->builtin || y->builtin)
        {
          return x->builtin == y->builtin;
        }
      else
        {
          return lval_eq(x->formals, y->formals)
            && lval_eq(x->body, y->body);
        }

    case LVAL_QEXPR:
    case LVAL_SEXPR:
      if (x->count != y->count)
        {
          return 0;
        }
      for (int i = 0; i < x->count; i++)
        {
          // if any element not equal then whole list
          if (!lval_eq(x->cell[i], y->cell[i]))
            {
              return 0;
            }
          // Otherwise return 1
        }
      return 1;
    break;
    }
  return 0;
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

lval* lval_call (lenv* e, lval* f, lval* a)
{
  // a is list of passed arguments
  // if builtin the apply this builtin
  if (f->builtin) { return f->builtin(e, a); }

  // check how many arguments passed to the function
  // and how many arguments can be passed
  int given = a->count;
  int total = f->formals->count;

  while (a->count)
    {
      // When count of formals will be zero and loop still active
      // that means that too much args were passed to the func
      if (f->formals->count == 0)
        {
          lval_del(a);
          return lval_err("Function passed to many arguments. "
                          "Got %i, Expected %i.", given, total);
        }
      // Pop func args symbols from formals 
      lval* sym = lval_pop(f->formals, 0);
      
      if (strcmp(sym->sym, "&") == 0)
        {
          // ensure that & is fallowed by another symbol
          // that n args will be bounded too eg. pythons *x
          // that bounds all values into one list represented by x value
          if (f->formals->count != 1)
            {
              lval_del(a);
              return lval_err("Function format invalid. "
                              "Symbol '&' not fallowed by a single symbol.");
            }
          // Next format should be bound to remaining arguments
          lval* nsym = lval_pop(f->formals, 0);
          lenv_put(f->env, nsym, builtin_list(e, a));
          lval_del(sym);
          lval_del(nsym);
          break;
        }
      
      // Pop next arguments from the func
      lval* val = lval_pop(a, 0);
      // bind this values to the function env
      // Old values of sybmols are replaced by new ones (passed as
      // args to func)
      lenv_put(f->env, sym, val);
      // Delete symbol and value
      lval_del(sym);
      lval_del(val);
    }
  // Args are bounded to formals
  lval_del(a);

  if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0)
    {
      if (f->formals->count != 2)
        {
          return lval_err("Function format invalid. "
                          "Symbol '&' not followed by single symbol.");
        }
      // Pop and delete '&' symbol
      lval_del(lval_pop(f->formals, 0));
      // Pop next symbol 'name that represents the list of func args'
      lval* sym = lval_pop(f->formals, 0);
      lval* val = lval_qexpr();
      // Bind to enviroment and delete;
      lenv_put(f->env, sym, val);
      lval_del(sym);
      lval_del(val);
    }
  // if all formals have been bound evaluate them
  if (f->formals->count == 0)
    {
      // Set enviroment parent to evaluation enviroment
      f->env->par = e;
      // Eval and return
      return builtin_eval(f->env,
                          lval_add(lval_sexpr(), lval_copy(f->body)));
    }
  else
    {
      // return partialy evaluated func
      return lval_copy(f);
    }
}

lval* lval_read(mpc_ast_t* t)
{
  // If symbol or number return conversion to that type
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "boolean")) { return lval_read_boolean(t->contents); }
  if (strstr(t->tag, "string")) { return lval_read_str(t); }
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

void lval_print_str(lval* v)
{
  // make a copy of the string
  char* escaped = malloc(strlen(v->str)+1);
  strcpy(escaped, v->str);
  // Pass it thorugh the escape func
  escaped = mpcf_escape(escaped);
  // Print it between " " chars
  printf("\"%s\"", escaped);
  // free the copied string
  free(escaped);
}

// Print lval
void lval_print(lval* v)
{
  switch (v->type)
    {
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_STRING: lval_print_str(v); break;
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
    case LVAL_BOOL:
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
    case LVAL_STRING:
      x->str = malloc(strlen(v->str) + 1);
      strcpy(x->str, v->str); break;
    case LVAL_BOOL:
      x->num = v->num; 
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym);
      break;
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
  lval* body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

lval* builtin_if(lenv* e, lval* a)
{
  LASSERT_NUM("if", a, 3);
  LASSERT_TYPE("if", a, 0, LVAL_NUM);
  LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
  LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

  // Mark both expressions evaluable
  lval *x;
  a->cell[1]->type = LVAL_SEXPR;
  a->cell[2]->type = LVAL_SEXPR;

  if (a->cell[0]->num)
    {
      // if condition is true evaluate first expression
      x = lval_eval(e, lval_pop(a, 1)); 
    }
  else
    {
      x = lval_eval(e, lval_pop(a, 2)); 
    }

  // Delete argument list and return
  lval_del(a);
  return x;
}


lval* builtin_op(lenv* e, lval* a, char* op)
{
  // Ensure all arguments are number
  for (int i = 0; i < a->count; i++)
    {
      if (a->cell[i]->type != LVAL_NUM && a->cell[i]->type != LVAL_BOOL)
        {
          lval_del(a);
          return lval_err("Cannot operate on non-number");
        }
    }

  // Pop the first element
  lval* x = lval_pop(a, 0);
  if (x->type == LVAL_BOOL)
    {
      x->type = LVAL_NUM;
    }

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
  return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a)
{
  return builtin_var(e, a, "=");
}

lval* builtin_var(lenv* e, lval* a, char* func)
{
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
  lval* syms = a->cell[0];
  for (int i = 0; i < syms->count; i++)
    {
      LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
              "Function '%s' cannot define non-symbol. "
              "Got %s, Expected %s.", func, 
              ltype_name(syms->cell[i]->type),
              ltype_name(LVAL_SYM));
    }

  LASSERT(a, (syms->count == a->count-1),
          "Function '%s' passed too many arguments for symbols. "
          "Got %i, Expected %i.", func, syms->count, a->count-1);

  /*   // a->cell[0] => { x y z } */
  /*   // a->cell[>0] => { 1 2 3 4 }, 1, { 1 2}, ... */
  for (int i = 0; i < syms->count; i++)
    {
      // def keyword put variable to the parent env
      if (strcmp(func, "def") == 0 )
        {
          lenv_def(e, syms->cell[i], a->cell[i+1]);
        }
      // '=' symbol will put it locally
      else if (strcpy(func, "="))
        {
          lenv_put(e, syms->cell[i], a->cell[i+1]);
        }
    }
  lval_del(a);
  return lval_sexpr();
}

lval* builtin_gt(lenv* e, lval* a)
{
  return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a)
{
  return builtin_ord(e, a, "<");
}

lval* builtin_ge(lenv* e, lval* a)
{
  return builtin_ord(e, a, ">=");
}

lval* builtin_le(lenv* e, lval* a)
{
  return builtin_ord(e, a, "<=");
}

lval* builtin_cmp(lenv* e, lval* a, char* op)
{
  LASSERT_NUM(op, a, 2);
  int r = -1;
  if (strcmp(op, "==") == 0)
    {
      r = lval_eq(a->cell[0], a->cell[1]);
    }
  else if (strcmp(op, "!=") == 0)
    {
      r = !lval_eq(a->cell[0], a->cell[1]);
    }
  lval_del(a);
  return lval_num(r);
}  

lval* builtin_eq(lenv* e, lval* a)
{
  return builtin_cmp(e, a, "==");
}

lval* builtin_ne(lenv* e, lval* a)
{
  return builtin_cmp(e, a, "!=");
}

lval* builtin_or(lenv* e, lval* a)
{
  return builtin_logic_op(e, a, "or");
}

lval* builtin_or_sym(lenv* e, lval* a)
{
  return builtin_logic_op(e, a, "||");
}

lval* builtin_and(lenv* e, lval* a)
{
  return builtin_logic_op(e, a, "and");
}

lval* builtin_and_sym(lenv* e, lval* a)
{
  return builtin_logic_op(e, a, "&&");
}

lval* builtin_not(lenv* e, lval* a)
{
  return builtin_logic_op(e, a, "not");
}

lval* builtin_not_sym(lenv* e, lval* a)
{
  return builtin_logic_op(e, a, "!");
}

lval* builtin_ord(lenv* e, lval* a, char* op)
{
  LASSERT_NUM(op, a, 2);
  LASSERT_TYPE(op, a, 0, LVAL_NUM);
  LASSERT(a, a->cell[1]->type == LVAL_NUM || a->cell[1]->type == LVAL_BOOL,                     
          "Function '%s' passed incorrect type. "                       
          "Got %s, Exptected %s or %s",
          op, ltype_name(a->cell[1]->type),                    
          ltype_name(LVAL_BOOL), ltype_name(LVAL_NUM));                                          

  int r = 1000000000;
  if (strcmp(op, ">") == 0)
    {
      r = (a->cell[0]->num > a->cell[1]->num);
    }
  else if (strcmp(op, "<") == 0)
    {
      r = (a->cell[0]->num < a->cell[1]->num);
    }
  else if (strcmp(op, ">=") == 0)
    {
      r = (a->cell[0]->num >= a->cell[1]->num);
    }
  else if (strcmp(op, "<=") == 0)
    {
      r = (a->cell[0]->num <= a->cell[1]->num);
    }
  lval_del(a);
  return lval_num(r);
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

lval* builtin_logic_op(lenv* e, lval* a, char* op)
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
  if ((strcmp(op, "!") == 0 || strcmp(op, "not") == 0)  && a->count == 0)
    {
      x->num = !x->num;
    }
  while (a->count > 0)
    {
      lval* y = lval_pop(a, 0);
    
      if ((strcmp(op, "||") == 0) || (strcmp(op, "or") == 0))
        {
          x->num |= y->num;
        }
      else if (strcmp(op, "&&") == 0 || strcmp(op, "and") == 0)
        {
          x->num &= y->num;
        }

      lval_del(y);
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
      lval* err = lval_err("S-expression starts with incorrct type. "
                           "Got %s, Expected %s",
                           ltype_name(f->type), ltype_name(LVAL_FUN));
      lval_del(f);
      lval_del(v);
      return err;
    }

  // Call built-in with operator
  lval* result = lval_call(e, f, v);
  lval_del(f);
  return result;
}

char* ltype_name(int t)
{
  switch(t)
    {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_BOOL: return "Boolean";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    case LVAL_STRING: return "String";
    default: return "Unknown";
    }
}
      

int main(int argc, char** argv)
{
  // Parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Boolean = mpc_new("boolean");
  mpc_parser_t* String = mpc_new("string");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // Define parser with language
  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                   \
              number : /-?[0-9]+/;                              \
              boolean : /True|False/;                           \
              string  : /\"(\\\\.|[^\"])*\"/ ;                  \
              symbol: /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/;         \
              sexpr  : '(' <expr>* ')';                         \
              qexpr  : '{' <expr>* '}';                         \
              expr   : <number>  | <boolean> | <string> | <symbol> | <sexpr> | <qexpr>;\
              lispy  : /^/ <expr>* /$/;                         \
            ",
            Number, Boolean, String, Symbol, Sexpr, Qexpr, Expr, Lispy);
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
  mpc_cleanup(8, Number, Boolean, String, Symbol, Sexpr, Qexpr, Expr, Lispy);
  lenv_del(e);
  return 0;
}


