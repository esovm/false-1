#include "reasonf.h"
#include "y.tab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>

#include <ctype.h>
#include <unistd.h>

/* Hint the CPU */
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

/* Define the local branches - used in exec2 */
#define setup()  \
  static void *statements[] = {  \
    &&IF_, &&DOT_, &&OPEN_,&&PLUS_, &&MINUS_, &&DIVIDE_, &&MULTIPLY_, &&LT_, &&GT_, &&LTE_, &&GTE_, &&EQUAL_, &&ASSIGN_, &&APPLY_, &&NAME_, &&FLOAT_, &&FETCH_, &&WHILE_, &&INT_, &&DUP_, &&UMINUS_, &&NOT_, &&CAR_, &&CDR_, &&NIL_, &&CONS_, &&DONE_ };

/* Execute the next instruction - 257 is value of first token in lex/yacc */
#define next() do{ it=it->next; goto *statements[it->op-257]; }while(0)

/* Execute against a list [ function call basically] */
#define call(a) do{ t = make_open( it ); \
            t->next = cstack; \
            cstack = t;  \
            it = a->lambda; \
            goto *statements[it->op-257]; \
            } while(0); 

/* Create slab pool of struct node */
SPMALLOC_DECLARE( nodepool, 1024*1024, sizeof(struct node));

/* Friendly rename */
#define un(n)  SPMALLOC_FREE( n, nodepool )

int glob_dump = 0;

const char  g_welcome[] = "false - (C) 2009 Shane Adams";

extern struct node *pc;
extern struct node *prog;
struct trash_t *trash;

// Pretty print to visually verify we are parsing and traversing correctly
void exec( struct node *pc )  {
  struct node *it = pc;

  //LT GT LTE GTE EQUAL

  for( ;; )  {
//  while( it )  {
    if( it->op == OPEN ) {
      exec( it->lambda );
    } else if( it->op == INT )  {
      printf("%d ", it->i );
    } else if( it->op == FLOAT )  {
      printf("%f ", it->f );
    } else if( it->op == PLUS )  {
      printf("+ ");
    } else if( it->op == MINUS )  {
      printf("- ");
    } else if( it->op == MULTIPLY )  {
      printf("* ");
    } else if( it->op == DIVIDE )  {
      printf("/ ");
    } else if( it->op == LT )  {
      printf("< ");
    } else if( it->op == GT )  {
      printf("> ");
    } else if( it->op == EQUAL )  {
      printf("= ");
    } else if( it->op == LTE )  {
      printf("<= ");
    } else if( it->op == GTE )  {
      printf(">= ");
    } else if( it->op == NAME )  {
      printf("%s ", it->name);
    } else if( it->op == ASSIGN )  {
      printf("! ");
    } else if( it->op == APPLY )  {
      printf("@ ");
    } else if( it->op == DOT )  {
      printf(". ");
    } else if( it->op == FETCH )  {
      printf(": ");
    } else if( it->op == WHILE )  {
      printf("# ");
    } else if( it->op == DUP )  {
      printf("$ ");
    } else if( it->op == UMINUS )  {
      printf("_ ");
    } else if( it->op == NOT )  {
      printf("~ ");
    } else if( it->op == CAR )  {
      printf("car ");
    } else if( it->op == CDR )  {
      printf("cdr ");
    } else if( it->op == CONS )  {
      printf("cons ");
    }
    it = it->next;
  }
}

struct node *stack[100];
int exec2( struct node *pc, int idx )  {
  struct node *it = pc;
  struct node *a, *b, *c;
  struct symbol *s;
  struct node *cstack=0;
  struct node *t=0;
  // Setup branch definitions
  setup();

  // Jump to first instruction
  goto *statements[it->op-257];

CONS_:
        if( idx < 1 ) errx(0, "dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        stack[++idx] = make_cons( a,b );
        un(a); un(b); 
        next();
CDR_:
        if( idx < 0 ) errx(0, "dammit");
        a = stack[idx--]; 
        if( a->op == NAME )  {
          b = sym_get( a->name );
          if( b && b->lambda )  {
            stack[++idx] = make_dupe(b->lambda->next); 
          } else {
            stack[++idx] = make_op(NIL); 
          }
          b = sym_get( a->name )->lambda;
          stack[++idx] = make_dupe(b->next); 
        } else if( a->op == OPEN )  {
          //stack[++idx] = make_dupe(a->lambda->next); 
          stack[++idx] = make_open(a->lambda->next); 
        } else  {
          stack[++idx] = make_dupe(a->next); 
        }
        un(a);
        next();
CAR_:
        if( idx < 0 ) errx(0, "dammit");
        a = stack[idx--]; 
        if( a->op == NAME )  {
          b = sym_get( a->name );
          if( b && b->lambda )  {
            stack[++idx] = make_dupe(b); 
            stack[idx]->next = 0;  // isolate car
          } else {
            stack[++idx] = make_op(NIL); 
          }
        } else if( a->op == OPEN ) {
          stack[++idx] = make_dupe(a->lambda); 
          stack[idx]->next = 0; // isolate car
        } else if( a->next != NULL ) {
          stack[++idx] = make_dupe(a);
          stack[idx]->next = 0;  // isolate car
        } else {
          stack[++idx] = make_op(NIL); 
        }
        un(a);
        next();
DUP_:
        if( unlikely(idx < 0) ) errx(0,"dammit");
        stack[++idx] = make_dupe(stack[idx-1]);
        next();
DOT_:
        if( unlikely(idx < 0) ) errx(0,"dammit");
        switch( stack[idx]->op )  {
          case FLOAT:
            printf("%f", stack[idx]->f );
            break;
          case INT:
            printf("%i", stack[idx]->i );
            break;
          case NIL:
            printf("nil ");
            break;
        }
        idx--;
        next();
IF_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--];
        b = stack[idx--];
        if( b->op != INT && b->op != FLOAT )  {
          errx(0, "Applying if to non numeric.");
        } else {
          if( b->op == INT ? b->i : b->f )  {
            // true execute list
            call(a);
          }
        }
        next();
WHILE_:
        if( unlikely(idx < 2) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
again:
        idx = exec2( b->lambda, idx );
        if( unlikely(idx < 0) ) errx(0,"dammit");
        c = stack[idx--];
        if( c->op != INT && c->op != FLOAT ) { 
          errx(0, "Applying boolf to non numeric.");
        } else {
          if( likely(c->op == INT ? c->i  : c->f) )  {
            un(c);
            idx = exec2( a->lambda, idx );
            goto again;
          } else {
            un(a); un(b); un(c);
          }
        }
        next();
FETCH_:
        if( unlikely(idx < 0) ) errx(0,"dammit");
        a = stack[idx--]; 
        if( a->op != NAME )  {
          errx(0, "Applying : to a non symbol."); 
        } else {
          stack[++idx] = sym_get( a->name ); 
        }
        un(a);
        next();
APPLY_:
        if( unlikely(idx < 0) ) errx(0,"dammit");
        a = stack[idx--]; 
        if( a->op != OPEN  && a->op != NAME )  {
          errx(0, "Applying @ to a non lambda."); 
        } else if( a->op == NAME )  {
          call( sym_get( a->name )->lambda );
          //idx = exec2( sym_get( a->name )->lambda, idx );
        } else  {
          call( a );
          //idx = exec2( a->lambda, idx );
        }
        un(a);
        next();
ASSIGN_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( a->op != NAME ) {
					errx(0,"Applying ! to a non symbol."); 
        } else if( b->op == INT || b->op == FLOAT )  {
          sym_replace( a->name, b->op == INT ? 
						make_int( b->i ) : make_float( b->f ) );    
        } else if( b->op == OPEN )  {
          sym_replace( a->name, make_open( b->lambda ) );
        }
        un(a);
        un(b);
        next();
NIL_:
        stack[++idx] = make_op( NIL );
        next();
NAME_:
        stack[++idx] = make_name( it->name );
        next();
OPEN_:
        stack[++idx] = make_open( it->lambda );
        next();
INT_:
        stack[++idx] = make_int( it->i );
        next();
FLOAT_:
        stack[++idx] = make_float( it->i );
        next();
NOT_:
        if( unlikely(idx < 0) ) errx(0,"dammit");
        a = stack[idx]; 
        if( a->op != INT && a->op != FLOAT )  {
					errx(0,"Applying _ to a non numeric."); 
        } else {
          a->op == INT ? (a->i = !a->i) : (a->f = !a->f);
        }
        next();
UMINUS_:
        if( unlikely(idx < 0) ) errx(0,"dammit");
        a = stack[idx]; 
        if( a->op != INT && a->op != FLOAT )  {
					errx(0,"Applying _ to a non numeric."); 
        } else {
          a->op == INT ? (a->i *= -1) : (a->f *= -1);
        }
        next();
PLUS_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( unlikely(a->op != INT) && unlikely(a->op != FLOAT) )  {
          errx(0,"Applying + to non numeric data.");
        } else if( likely(a->op == FLOAT || b->op == FLOAT)) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f + ( a->op == FLOAT ? a->f : a->i );
          un(a); un(b);
          stack[++idx] = c;
        } else {
          // Both are ints so reuse a to store total and place back on stack
          a->i += b->i;
          stack[++idx] = a;
          un(b);
        }
        next();
MINUS_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( ! a->op == INT || ! a->op == FLOAT )  {
          errx(0,"Applying - to non numeric data.");
        } else if( a->op == FLOAT || b->op == FLOAT) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f - ( a->op == FLOAT ? a->f : a->i );
        } else {
          c = make_int( b->i );
          c->i = c->i - a->i;
        }
        un(a); un(b);
        stack[++idx] = c;
        next();
MULTIPLY_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( ! a->op == INT || ! a->op == FLOAT )  {
          errx(0,"Applying + to non numeric data.");
        } else if( a->op == FLOAT || b->op == FLOAT) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f * ( a->op == FLOAT ? a->f : a->i );
        } else {
          c = make_int( b->i );
          c->i = c->i * a->i;
        }
        un(a); un(b);
        stack[++idx] = c;
        next();
DIVIDE_:
        if( idx < 1 ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( ! a->op == INT || ! a->op == FLOAT )  {
          errx(0,"Applying + to non numeric data.");
        } else if( a->op == FLOAT || b->op == FLOAT) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f / ( a->op == FLOAT ? a->f : a->i );
        } else {
          c = make_int( b->i );
          c->i = c->i / a->i;
        }
        un(a); un(b);
        stack[++idx] = c;
        next();
LT_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( ! a->op == INT || ! a->op == FLOAT )  {
          errx(0,"Applying + to non numeric data.");
        } else if( a->op == FLOAT || b->op == FLOAT) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f < ( a->op == FLOAT ? a->f : a->i );
          un(a); un(b);
          stack[++idx] = c;
        } else {
          // Both are ints so reuse a to store total and place back on stack
          a->i = b->i < a->i;
          //c = make_int( b->i );
          //c->i = c->i < a->i;
          un(b);
          stack[++idx] = a;
        }
        next();
GT_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( ! a->op == INT || ! a->op == FLOAT )  {
          errx(0,"Applying + to non numeric data.");
        } else if( a->op == FLOAT || b->op == FLOAT) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f > ( a->op == FLOAT ? a->f : a->i );
        } else {
          c = make_int( b->i );
          c->i = c->i > a->i;
        }
        un(a); un(b);
        stack[++idx] = c;
        next();
EQUAL_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( ! a->op == INT || ! a->op == FLOAT )  {
          errx(0,"Applying + to non numeric data.");
        } else if( a->op == FLOAT || b->op == FLOAT) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f == ( a->op == FLOAT ? a->f : a->i );
        	un(a); un(b);
        	stack[++idx] = c;
        } else {
          // Both are ints so reuse a to store total and place back on stack
          a->i = b->i == a->i;
          //c = make_int( b->i );
          //c->i = c->i == a->i;
          un(b);
          stack[++idx] = a;
        }
        next();
LTE_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( ! a->op == INT || ! a->op == FLOAT )  {
          errx(0,"Applying + to non numeric data.");
        } else if( a->op == FLOAT || b->op == FLOAT) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f <= ( a->op == FLOAT ? a->f : a->i );
        } else {
          c = make_int( b->i );
          c->i = c->i <= a->i;
        }
        un(a); un(b);
        stack[++idx] = c;
        next();
GTE_:
        if( unlikely(idx < 1) ) errx(0,"dammit");
        a = stack[idx--]; 
        b = stack[idx--]; 
        if( ! a->op == INT || ! a->op == FLOAT )  {
          errx(0,"Applying + to non numeric data.");
        } else if( a->op == FLOAT || b->op == FLOAT) {
          c = make_float( b->op == FLOAT ? b->f : b->i );
          c->f = c->f >= ( a->op == FLOAT ? a->f : a->i );
        } else {
          c = make_int( b->i );
          c->i = c->i >= a->i;
        }
        un(a); un(b);
        stack[++idx] = c;
        next();
DONE_:
        if( cstack ) {
          it=cstack->lambda;
          a = cstack;
          cstack = cstack->next;
          un(a);
				  next();
        } else {
          goto done;
        }
    
done:
  return idx;
}

void uggopt( int argc, char **argv )  {
  int c;
  while( (c=getopt(argc, argv, "d")) != -1 )  {
    switch(c)  {
      case 'd':
        glob_dump = 1;
      break;
    }
  }
}

int reasonf_main( int argc, char **argv) {

  uggopt(argc, argv );

  // Welcome
  printf("> %s \n", g_welcome );

  // Parse the program
  yyparse();

  cap(make_op(DONE));
  exec2( prog, -1 );

  return 0;
}


