
#include "luatex-api.h"
#include <ptexlib.h>
#include <sys/stat.h>

#include <luatexdir/luatexextra.h>

const_string LUATEX_IHELP[] = {
    "Usage: luatex --lua=FILE [OPTION]... [TEXNAME[.tex]] [COMMANDS]",
    "   or: luatex --lua=FILE [OPTION]... \\FIRST-LINE",
    "   or: luatex --lua=FILE [OPTION]... &FMT ARGS",
    "  Run luaTeX on TEXNAME, usually creating TEXNAME.pdf.",
    "  Any remaining COMMANDS are processed as luaTeX input, after TEXNAME is read.",
    "",
    "  Alternatively, if the first non-option argument begins with a backslash,",
    "  interpret all non-option arguments as a line of luaTeX input.",
    "",
    "  Alternatively, if the first non-option argument begins with a &, the",
    "  next word is taken as the FMT to read, overriding all else.  Any",
    "  remaining arguments are processed as above.",
    "",
    "  If no arguments or options are specified, prompt for input.",
    "",
    "-lua=FILE               the lua initialization file",
    "-fmt=FORMAT             load the format file FORMAT",
    "-ini                    be initex, for dumping formats",
    "-help                   display this help and exit",
    "-version                output version information and exit",
    NULL
};


static void prepare_cmdline (lua_State *L, char **argv, int argc) {
  int i;
  luaL_checkstack(L, argc+3, "too many arguments to script");
  lua_createtable(L, 0, 0);
  for (i=0; i < argc; i++) {
	lua_pushstring(L, argv[i]);
	lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "arg");
  return;
}

/* The C version of what might wind up in DUMP_VAR.  */
extern string dump_name;

string input_name = NULL;

static string user_progname = NULL;

extern char *ptexbanner;

/* for topenin() */
extern char **argv;
extern int argc;

char *startup_filename = NULL;

/* Reading the options.  */

/* Test whether getopt found an option ``A''.
   Assumes the option index is in the variable `option_index', and the
   option table in a variable `long_options'.  */
#define ARGUMENT_IS(a) STREQ (long_options[option_index].name, a)

/* SunOS cc can't initialize automatic structs, so make this static.  */
static struct option long_options[]
  = { { "fmt",                       1, 0, 0 },
      { "lua",                       1, 0, 0 },
      { "progname",                  1, 0, 0 },
      { "help",                      0, 0, 0 },
      { "ini",                       0, &iniversion, 1 },
      { "version",                   0, 0, 0 },
      { 0, 0, 0, 0 } };

static void parse_options (int argc, char **argv) {
  int g;   /* `getopt' return code.  */
  int option_index;
  opterr = 0; /* dont whine */
  for (;;) {
    g = getopt_long_only (argc, argv, "+", long_options, &option_index);

    if (g == -1) /* End of arguments, exit the loop.  */
      break;
    if (g == '?') /* Unknown option.  */
      continue;

    assert (g == 0); /* We have no short option names.  */

    if (ARGUMENT_IS ("lua")) {
      startup_filename = optarg;
      
    } else if (ARGUMENT_IS ("fmt")) {
      dump_name = optarg;

    } else if (ARGUMENT_IS ("progname")) {
      user_progname = optarg;

    } else if (ARGUMENT_IS ("help")) {
      usagehelp (LUATEX_IHELP, BUG_ADDRESS);

    } else if (ARGUMENT_IS ("version")) {
      printversionandexit (BANNER, COPYRIGHT_HOLDER, AUTHOR);

    }
  }
  /* attempt to find dump_name */
  if (argv[optind] && argv[optind][0] == '&') {
    dump_name = strdup(argv[optind]+1);
  } else if (argv[optind] && argv[optind][0] != '\\') {
    if (argv[optind][0] == '*')
      input_name = strdup(argv[optind]+1);
    else
      input_name = strdup(argv[optind]);
  }
}

/* test for readability */
#define is_readable(a) (stat(a,&finfo)==0) && S_ISREG(finfo.st_mode) &&  \
  (f=fopen(a,"r")) != NULL && !fclose(f)

char *find_filename (char *name, char *envkey) {
  struct stat finfo;
  char *dirname=NULL;
  char *filename=NULL;
  FILE *f;
  if (is_readable(name)) {
    return name;
  } else {
    dirname = getenv(envkey);
    if ((dirname != NULL) && strlen(dirname)) {
      dirname = strdup(getenv(envkey));
      if (*(dirname+strlen(dirname)-1) == '/') {
	*(dirname+strlen(dirname)-1) = 0;
      }
      filename = xmalloc(strlen(dirname)+strlen(name)+2) ;
      filename= concat3(dirname,"/",name);
      if (is_readable(filename)) {
	xfree (dirname);
	return filename;
      }
      xfree(filename);
    }
  }
  return NULL;
}


void
lua_initialize (int ac, char **av) {
  FILE *test;
  int kpse_init;
  int dist;
  int tex_table_id;
  int pdf_table_id;
  /* Save to pass along to topenin.  */
  argc = ac;
  argv = av;

  ptexbanner = BANNER;

  /* Must be initialized before options are parsed.  */
  interactionoption = 4;
  dump_name = NULL;
  /* parse commandline */
  parse_options(ac,av);

  luainterpreter (0);
  /* hide the 'tex' and 'pdf' table */
  tex_table_id = hide_lua_table(Luas[0],"tex");
  pdf_table_id = hide_lua_table(Luas[0],"pdf");

  prepare_cmdline(Luas[0], argv, argc);  /* collect arguments */
  /* */
  
  if (startup_filename!=NULL)
    startup_filename= find_filename(startup_filename,"LUATEXDIR");
    
  /* now run the file */
  if (startup_filename!=NULL) {
    if(luaL_loadfile(Luas[0],startup_filename)|| lua_pcall(Luas[0],0,0,0)) {
      fprintf(stdout,"Error in config file loading: %s\n", lua_tostring(Luas[0],-1));
      exit(1);
    }
    /* unhide the 'tex' and 'pdf' table */
    unhide_lua_table(Luas[0],"tex",tex_table_id);
    unhide_lua_table(Luas[0],"pdf",pdf_table_id);

    /* kpse_init */    
    kpse_init = -1;
    getluaboolean("texconfig","kpse_init",&kpse_init);

    if (kpse_init!=0) {
      if (!user_progname)
	user_progname = (string)dump_name;
      if (!user_progname) {
	if (iniversion)
	  user_progname=input_name;
      }
      if (!user_progname) {
	fprintf(stdout,"kpathsea mode needs a --progname or --fmt switch\n");
	exit(1);
      }
      kpse_set_program_name (argv[0], user_progname);
    }
    /* prohibit_file_trace (boolean) */
    tracefilenames = 1;
    getluaboolean("texconfig","trace_file_names",&tracefilenames);
    
    /* src_special_xx */
    insertsrcspecialauto = insertsrcspecialeverypar = 
      insertsrcspecialeveryparend = insertsrcspecialeverycr = 
      insertsrcspecialeverymath =  insertsrcspecialeveryhbox =
      insertsrcspecialeveryvbox = insertsrcspecialeverydisplay = false;
    getluaboolean("texconfig","src_special_auto", &insertsrcspecialauto);
    getluaboolean("texconfig","src_special_everypar", &insertsrcspecialeverypar );
    getluaboolean("texconfig","src_special_everyparend",&insertsrcspecialeveryparend);
    getluaboolean("texconfig","src_special_everycr",&insertsrcspecialeverycr );
    getluaboolean("texconfig","src_special_everymath",&insertsrcspecialeverymath);
    getluaboolean("texconfig","src_special_everyhbox",&insertsrcspecialeveryhbox);
    getluaboolean("texconfig","src_special_everyvbox",&insertsrcspecialeveryvbox );
    getluaboolean("texconfig","src_special_everydisplay", &insertsrcspecialeverydisplay);

    srcspecialsp=insertsrcspecialauto | insertsrcspecialeverypar |
      insertsrcspecialeveryparend | insertsrcspecialeverycr |
      insertsrcspecialeverymath |  insertsrcspecialeveryhbox |
      insertsrcspecialeveryvbox | insertsrcspecialeverydisplay;

    /* file_line_error */
    filelineerrorstylep = false;
    getluaboolean("texconfig","file_line_error", &filelineerrorstylep);

    /* halt_on_error */
    haltonerrorp = false;
    getluaboolean("texconfig","halt_on_error",  &haltonerrorp);

    if (dump_name) {
      /* adjust array for Pascal and provide extension, if needed */
      dist = strlen(dump_name)-strlen(DUMP_EXT);
      if (strstr(dump_name,DUMP_EXT) == dump_name+dist)
	DUMP_VAR = concat (" ", dump_name);
      else
	DUMP_VAR = concat3 (" ", dump_name, DUMP_EXT);
      DUMP_LENGTH_VAR = strlen (DUMP_VAR + 1);
    } else {
      /* For dump_name to be NULL is a bug.  */
      if (!iniversion)
	abort();
    }
  } else {
    fprintf(stdout,"Missing configuration file\n");
	exit(1);
  }
}


