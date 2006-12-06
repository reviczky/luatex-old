/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

typedef struct statistic {
  const char *name;
  char type;
  void *value;
} statistic ;

extern char *ptexbanner;
/* extern string getcurrentfilenamestring; */

typedef char * (*charfunc)(void);
typedef integer (*intfunc)(void);

char *getbanner (void) {
  return ptexbanner;
}

char *getfilename (void) {
  return makecstring(getcurrentname());
}

char *getlasterror (void) {
  return makecstring(lasterror);
}


extern int luabytecode_max;
extern int luabytecode_bytes;
extern int luastate_max;
extern int luastate_bytes;

static struct statistic stats[] = {
  { "pdf_gone",                  'g', &pdfgone         },
  { "pdf_ptr",                   'g', &pdfptr          },
  { "dvi_gone",                  'g', &dvioffset       },
  { "dvi_ptr",                   'g', &dviptr          },
  { "total_pages",               'g', &totalpages      },
  { "output_file_name",          's', &outputfilename  },
  { "log_name",                  's', &texmflogname    }, /* weird */
  { "banner",                    'S', &getbanner       },
  { "pdftex_banner",             's', &pdftexbanner    },
  /*
   * mem stat 
   */
  { "var_used",                  'g', &varused         },
  { "dyn_used",                  'g', &dynused         },
  /* 
   * traditional tex stats 
   */
  { "str_ptr",                   'g', &strptr          },
  { "init_str_ptr",              'g', &initstrptr      },
  { "max_strings",               'g', &maxstrings      },
  { "pool_ptr",                  'g', &poolptr         },
  { "init_pool_ptr",             'g', &initpoolptr     },
  { "pool_size",                 'g', &poolsize        },
  { "lo_mem_max",                'g', &lomemmax        },
  { "mem_min",                   'g', &memmin          },
  { "mem_end",                   'g', &memend          },
  { "hi_mem_min",                'g', &himemmin        },
  { "cs_count",                  'g', &cscount         },
  { "hash_size",                 'G', &gethashsize     },
  { "hash_extra",                'g', &hashextra       },
  { "font_ptr",                  'g', &fontptr         },
  //{ "font_base",                 'g', &fontbase        },
  { "hyph_count",                'g', &hyphcount       },
  { "hyph_size",                 'g', &hyphsize        },
  { "max_in_stack",              'g', &maxinstack      },
  { "max_nest_stack",            'g', &maxneststack    },
  { "max_param_stack",           'g', &maxparamstack   },
  { "max_buf_stack",             'g', &maxbufstack     },
  { "max_save_stack",            'g', &maxsavestack    },
  { "stack_size",                'g', &stacksize       },
  { "nest_size",                 'g', &nestsize        },
  { "param_size",                'g', &paramsize       },
  { "buf_size",                  'g', &bufsize         },
  { "save_size",                 'g', &savesize        },
  /* pdf stats */
  { "obj_ptr",                   'g', &objptr          },
  { "obj_tab_size",              'g', &objtabsize      },
  //  { "sup_obj_tab_size",          'g', &supobjtabsize   },
  { "pdf_os_cntr",               'g', &pdfoscntr       },
  //  { "pdf_os_max_objs",           'g', &pdfosmaxobjs    },
  { "pdf_os_objidx",             'g', &pdfosobjidx     },
  { "pdf_dest_names_ptr",        'g', &pdfdestnamesptr },
  { "dest_names_size",           'g', &destnamessize   },
  //{ "sup_dest_names_size",       'g', &supdestnamessize},
  { "pdf_mem_ptr",               'g', &pdfmemptr       },
  { "pdf_mem_size",              'g', &pdfmemsize      },
  //{ "sup_pdf_mem_size",          'g', &suppdfmemsize   },
  
  { "largest_used_mark",         'g', &biggestusedmark  },
  //

  { "filename",                  'S', &getfilename     },
  { "inputid",                   'G', &getcurrentname  },
  { "linenumber",                'g', &line            },
  { "lasterrorstring",           'S', &getlasterror    },
  
  // 
  { "luabytecodes",              'g', &luabytecode_max   },
  { "luabytecode_bytes",         'g', &luabytecode_bytes },
  { "luastates",                 'g', &luastate_max      },
  { "luastate_bytes",            'g', &luastate_bytes    },
  
  { NULL,                         0 , 0                } };


static int stats_name_to_id (char *name) {
  int i;
  for (i=0; stats[i].name!=NULL; i++) {
    if (strcmp(stats[i].name, name) == 0)
      return i;
  }
  return 0;
}

static int do_getstat (lua_State *L,int i) {
  int t;
  char *st;
  charfunc f;
  intfunc g;
  int str;
  t = stats[i].type;
  switch(t) {
  case 'S':
	f = stats[i].value;
	st = f();
	lua_pushstring(L,st);
	break;
  case 's':
	str = *(integer *)(stats[i].value);
    if (str) {
      lua_pushstring(L,makecstring(str));
    } else {
  	  lua_pushnil(L);
	}
	break;
  case 'G':
	g = stats[i].value;
	lua_pushnumber(L,g());
	break;
  case 'g':
	lua_pushnumber(L,*(integer *)(stats[i].value));
	break;
  default:
	lua_pushnil(L);
  }
  return 1;
}

static int getstats (lua_State *L) {
  char *st;
  int i;
  if (lua_isstring(L,-1)) {
    st = (char *)lua_tostring(L,-1);
	i = stats_name_to_id(st);
	if (i>0) {
	  return do_getstat(L,i);
	}
  }
  return 0;
}

static int setstats (lua_State *L) {
  return 0;
}

static int statslist (lua_State *L) {
  int i;
  luaL_checkstack(L,1,"out of stack space");
  lua_newtable(L);
  for (i=0; stats[i].name!=NULL; i++) {
	luaL_checkstack(L,2,"out of stack space");
	lua_pushstring(L,stats[i].name);
	do_getstat(L,i);
	lua_rawset(L,-3);
  }
  return 1;
}



static const struct luaL_reg statslib [] = {
  {"list",statslist},
  {NULL, NULL}  /* sentinel */
};

int luaopen_stats (lua_State *L) 
{
  luaL_register(L, "statistics", statslib);
  luaL_newmetatable(L,"stats_meta"); 
  lua_pushstring(L, "__index");
  lua_pushcfunction(L, getstats); 
  lua_settable(L, -3);
  lua_pushstring(L, "__newindex");
  lua_pushcfunction(L, setstats); 
  lua_settable(L, -3);
  lua_setmetatable(L,-2); /* meta to itself */
}
