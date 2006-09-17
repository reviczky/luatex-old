
#define HIGHPART 68
#define MIDPART 128
#define LOWPART 128

#define Mxmalloc_array(a,b)  malloc(b*sizeof(a))
#define Mxrealloc_array(a,b,c)  realloc(a,c*sizeof(b))
#define Mxfree(a) free(a)

typedef unsigned int  sa_tree_item;

typedef struct {
  integer      code;
  sa_tree_item value;
  int          level;
} sa_stack_item;


typedef struct {
  int size;                 /* initial stack size   */
  int step;                 /* increment stack step */
  int ptr;                  /* current stack point  */
  int dflt;                 /* default item value   */  
  sa_tree_item *** tree;    /* item tree head       */
  sa_stack_item  * stack;   /* stack tree head      */
} sa_tree_head;

typedef sa_tree_head *sa_tree;

extern sa_tree_item get_sa_item     (sa_tree head, integer n) ;
extern void         set_sa_item     (sa_tree head, integer n, sa_tree_item v, integer gl) ;

extern sa_tree      new_sa_tree     (integer size, integer dflt);

extern sa_tree      copy_sa_tree     (sa_tree head);

extern void         dump_sa_tree    (sa_tree a);
extern sa_tree      undump_sa_tree  (void) ;

extern void         restore_sa_stack  (sa_tree a, integer gl);
