/* web2c -- convert the pseudo-Pascal generated by Tangle to C.
   The output depends on many C macros and some postprocessing by other
   programs.

   Arguments:
   -f:  force strict interpretation of semantics of for stmt
   (never used with TeX and friends)
   -t:  special optimizations for tex.p->tex*.c
   -m:  special optimizations for mf.p->mf*.c
   -c:  supply the base part of the name of the coerce.h file
   -h:  supply the name of the standard header file
   -d:  generate some additional debugging output

   The majority of this program (which includes ptoc.yacc and ptoc.lex)
   was written by Tomas Rokicki, with modifications by Tim Morgan, et al. */

#include "web2c.h"
#include "web2c-parser.h"


/* Changing this value will probably stimulate bugs in some
   preprocessors -- those which want to put the expansion of a macro
   entirely on one line.  */
#define max_line_length 78

#define max_strings 50000
#define hash_prime 7883
#define sym_table_size 50000
#define unused 271828

/* Says whether to give voluminous progress reports.  */
boolean debug = false;
int indent = 0;
int line_pos = 0;
int last_brace = 0;
int block_level = 0;
int last_tok;
int tex = 0, strict_for = 0, mf = 0;

char safe_string[80];
char var_list[200];
char field_list[200];
char last_id[80];
char z_id[80];
char next_temp[] = "zzzaa";
char coerce_name[100] = "coerce.h";
string program_name;

long last_i_num;
int ii, l_s;
long lower_bound, upper_bound;
FILE *out;
FILE *coerce;
int pf_count = 1;

char *std_header = "null.h";	/* Default include filename */

char strings[max_strings];
int hash_list[hash_prime];
short global = 1;
struct sym_entry sym_table[sym_table_size];
int next_sym_free = -1, next_string_free = 0;
int mark_sym_free, mark_string_free;

int argc;
char **gargv;

extern int yyleng;

void
find_next_temp (void)
{
  next_temp[4]++;
  if (next_temp[4] > 'z')
    {
      next_temp[4] = 'a';
      next_temp[3]++;
    }
}

void
normal (void)
{
  out = stdout;
}

void
new_line (void)
{
  if (!out)
    return;
  if (line_pos > 0)
    {
      putc ('\n', out);
      line_pos = 0;
    }
}


/* Output the string S to the file `out'.  */

void
my_output (string s)
{
  int len = strlen (s);
  int less_indent = 0;

  if (!out)
    return;

  if (line_pos + len > max_line_length)
    new_line ();

  if (indent > 1 && (strcmp (s, "case") == 0 || strcmp (s, "default") == 0))
    less_indent = 2;

  while (line_pos < indent * 2 - less_indent) {
    fputs ("  ", out);
    line_pos += 2;
  }

  /* Output the token.  */
  fputs (s, out);

  /* Omitting the space for parentheses makes fixwrites lose.  Sigh.
     What a kludge.  */
  if (!(len == 1 && (*s == ';' || *s == '[' || *s == ']')))
    putc (' ', out);
  line_pos += len + 1;

  last_brace = (s[0] == '}');
}

void
semicolon (void)
{
  if (!last_brace) {
    my_output (";");
    new_line ();
    last_brace = 1;
  }
}

static int
hash (const_string id)
{
  register int i = 0, j;
  for (j = 0; id[j] != 0; j++)
    i = (i + i + id[j]) % hash_prime;
  return i;
}

int
search_table (const_string id)
{
  int ptr;
  ptr = hash_list[hash (id)];
  while (ptr != -1)
    {
      if (strcmp (id, sym_table[ptr].id) == 0)
	return (ptr);
      else
	ptr = sym_table[ptr].next;
    }
  return -1;
}


/* Add ID to the symbol table.  Leave it up to the caller to assign to
   the `typ' field.  Return the index into the `sym_table' array.  */
int
add_to_table (string id)
{
  int h, ptr;
  h = hash (id);
  ptr = hash_list[h];
  hash_list[h] = ++next_sym_free;
  sym_table[next_sym_free].next = ptr;
  sym_table[next_sym_free].val = unused;
  sym_table[next_sym_free].id = strings + next_string_free;
  sym_table[next_sym_free].var_formal = false;
  sym_table[next_sym_free].var_not_needed = false;
  strcpy (strings + next_string_free, id);
  next_string_free += strlen (id) + 1;
  return next_sym_free;
}

void
remove_locals (void)
{
  int h, ptr;
  for (h = 0; h < hash_prime; h++)
    {
      next_sym_free = mark_sym_free;
      next_string_free = mark_string_free;
      ptr = hash_list[h];
      while (ptr > next_sym_free)
	ptr = sym_table[ptr].next;
      hash_list[h] = ptr;
    }
  global = 1;
}

void
mark (void)
{
  mark_sym_free = next_sym_free;
  mark_string_free = next_string_free;
  global = 0;
}


void
initialize (void)
{
  register int i;

  for (i = 0; i < hash_prime; hash_list[i++] = -1)
    ;

  normal ();

  coerce = xfopen (coerce_name, FOPEN_W_MODE);
}

int
main (int argc, string *argv)
{
  int error, i;

  for (i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch (argv[i][1])
	{
	case 't':
	  tex = true;
	  break;
	case 'm':
	  mf = true;
	  break;
	case 'f':
	  strict_for = true;
	  break;
	case 'h':
	  std_header = &argv[i][2];
	  break;
	case 'd':
	  debug = true;
	  break;
	case 'c':
          program_name = &argv[i][2];
	  sprintf (coerce_name, "%s.h", program_name);
	  break;
	default:
	  fprintf (stderr, "web2c: Unknown option %s, ignored\n", argv[i]);
	  break;
	}
    else
      {
	fprintf (stderr, "web2c: Unknown argument %s, ignored\n", argv[i]);
      }

  initialize ();
  error = yyparse ();
  new_line ();

  xfclose (coerce, coerce_name);

  if (debug)
    {
      fprintf (stderr, "%d symbols.\n", next_sym_free);
      fprintf (stderr, "%d strings.\n", next_string_free);
    }

  return EXIT_SUCCESS;
}
