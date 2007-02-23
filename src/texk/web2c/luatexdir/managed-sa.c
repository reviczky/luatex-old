
#include "luatex-api.h"
#include <ptexlib.h>
#include "managed-sa.h"


static void
store_sa_stack (sa_tree a, integer n, integer v, integer gl) {
  sa_stack_item st;
  st.code  = n;
  st.value = v;
  st.level = gl;
  if (a->stack == NULL) {
    a->stack = Mxmalloc_array(sa_stack_item,a->size);
  } else if (((a->ptr)+1)>=a->size) {
    a->size += a->step;
    a->stack = Mxrealloc_array(a->stack,sa_stack_item,a->size);
  }
  (a->ptr)++;
  a->stack[a->ptr] = st;
}

static void
skip_in_stack (sa_tree a, integer n) {
  int p = a->ptr;
  if (a->stack == NULL)
	return;
  while (p>0) {
    if (a->stack[p].code == n && a->stack[p].level > 0) {
      a->stack[p].level  = -(a->stack[p].level);
    }
    p--;
  }
}

sa_tree_item
get_sa_item (sa_tree head, integer n) {
  unsigned char h,m,l;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  if ((head->tree == NULL) ||
      (head->tree[h] == NULL) ||
      (head->tree[h][m] == NULL)) {
    return head->dflt;
  }
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  return head->tree[h][m][l];
}

void
set_sa_item (sa_tree head, integer n, sa_tree_item v, integer gl) {
  unsigned char h,m,l;
  int i;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  if (head->tree == NULL) {
    head->tree = (sa_tree_item ***) Mxmalloc_array(sa_tree_item **,HIGHPART);
    for  (i=0; i<HIGHPART; i++) { head->tree[i] = NULL; }  
  }
  if (head->tree[h] == NULL) {
    head->tree[h] = (sa_tree_item **) Mxmalloc_array(sa_tree_item *,MIDPART); 
    for  (i=0; i<MIDPART; i++) { head->tree[h][i] = NULL; }  
  }
  if (head->tree[h][m] == NULL) {
    head->tree[h][m] = (sa_tree_item *) Mxmalloc_array(sa_tree_item,LOWPART);
    for  (i=0; i<LOWPART; i++) { head->tree[h][m][i] = head->dflt; }  
  }
  if (gl<=1) {
    skip_in_stack(head,n);
  } else {
    store_sa_stack(head,n,head->tree[h][m][l],gl);
  }
  head->tree[h][m][l] = v;
}

static void
rawset_sa_item (sa_tree head, integer n, integer v) {
  unsigned char h,m,l;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  head->tree[h][m][l] = v;
}

void
clear_sa_stack (sa_tree a) {
  if (a->stack != NULL) {
	Mxfree(a->stack);
  }
  a->stack = NULL;
  a->ptr   = 0;
  a->size  = a->step;
}

void
destroy_sa_tree (sa_tree a) {
  unsigned char h,m,l;
  if (a == NULL)
    return;
  if (a->tree != NULL) {
    for (h=0; h<HIGHPART;h++ ) {
      if (a->tree[h] != NULL) {
		for (m=0; m<MIDPART; m++ ) {
		  if (a->tree[h][m] != NULL) {
			Mxfree(a->tree[h][m]);
		  }
		}
		Mxfree(a->tree[h]);
	  }
	}
	Mxfree(a->tree);
  }
  if (a->stack != NULL) {
	Mxfree(a->stack);
  }
  Mxfree(a);
}


sa_tree
copy_sa_tree(sa_tree b) {
  unsigned char h,m,l;
  sa_tree a = (sa_tree)Mxmalloc_array(sa_tree_head,1);
  a->step  = b->step;
  a->size  = b->size;
  a->dflt  = b->dflt;
  a->stack = NULL;
  a->ptr   = 0;
  a->tree = NULL;
  if (b->tree !=NULL) {
	a->tree = (sa_tree_item ***)Mxmalloc_array(void *,HIGHPART);
	for (h=0; h<HIGHPART;h++ ) {  
	  if (b->tree[h] != NULL) {
		a->tree[h]=(sa_tree_item **)Mxmalloc_array(void *,MIDPART);
		for (m=0; m<MIDPART; m++ )  { 
		  if (b->tree[h][m]!=NULL) { 
			a->tree[h][m]=Mxmalloc_array(sa_tree_item,LOWPART);
			for (l=0; l<LOWPART; l++)  { 
			  a->tree[h][m][l] =  b->tree[h][m][l] ;
			} 
		  } else {
			a->tree[h][m] = NULL; 
		  }   
		}  
	  } else { 
		a->tree[h]= NULL; 
	  } 
	}
  }
  return a;
}


sa_tree
new_sa_tree (integer size, sa_tree_item dflt) {
  sa_tree_head *a;
  a = (sa_tree_head *)xmalloc(sizeof(sa_tree_head));
  a->dflt    = dflt;
  a->stack   = NULL;
  a->tree    = NULL;
  a->size    = size;
  a->step    = size;
  a->ptr     = 0;
  return (sa_tree)a;
}

void
restore_sa_stack (sa_tree head, integer gl) {
  sa_stack_item st;
  if (head->stack == NULL)
	return;
  while (head->ptr>0 && abs(head->stack[head->ptr].level)>=gl) {
	st = head->stack[head->ptr];
	if (st.level>0) {
	  rawset_sa_item (head, st.code, st.value);
	}
	(head->ptr)--;
  }
}


void
dump_sa_tree (sa_tree a) {
  boolean f;
  unsigned int x;
  unsigned char h,m,l;
  if (a == NULL)
    return;
  dump_int(a->step);
  dump_int(a->dflt);
  if (a->tree != NULL) {
    for (h=0; h<HIGHPART;h++ ) {
      if (a->tree[h] != NULL) {
	f = 1;  dump_qqqq(f);
	for (m=0; m<MIDPART; m++ ) {
	  if (a->tree[h][m] != NULL) {
	    f = 1;  dump_qqqq(f);
	    for (l=0;l<LOWPART;l++) { 
	      x = a->tree[h][m][l];  dump_int(x);  
	    }
	  } else { 
	    f = 0;  dump_qqqq(f);
	  } 
	}
      } else { 
	f = 0;  dump_qqqq(f);  
      } 
    }
  }
}


sa_tree
undump_sa_tree(void) {
  unsigned int x;
  unsigned char h,m,l;
  boolean f;
  sa_tree a  = (sa_tree)Mxmalloc_array(sa_tree_head,1);
  undump_int(x) ; a->step  = x; a->size  = x;
  undump_int(x) ; a->dflt  = x;
  a->stack = Mxmalloc_array(sa_stack_item,a->step);
  a->ptr   = 0;
  a->tree = (sa_tree_item ***)Mxmalloc_array(void *,HIGHPART);
  for (h=0; h<HIGHPART;h++ ) {  
    undump_qqqq(f);  
    if (f>0)  { 
      a->tree[h]=(sa_tree_item **)Mxmalloc_array(void *,MIDPART);
      for (m=0; m<MIDPART; m++ )  { 
	undump_qqqq(f);
        if (f>0) { 
	  a->tree[h][m]=Mxmalloc_array(sa_tree_item,LOWPART);
	  for (l=0; l<LOWPART; l++)  { 
	    undump_int(x);  a->tree[h][m][l] = x; 
	  } 
	} else {
	  a->tree[h][m] = NULL; 
	}   
      }  
    } else { 
      a->tree[h]= NULL; 
    } 
  }
  return a;
}


