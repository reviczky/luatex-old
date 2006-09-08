/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include <kpathsea/expand.h>
#include <kpathsea/variable.h>

static const int filetypes[] = {
  kpse_gf_format,
  kpse_pk_format,
  kpse_any_glyph_format,
  kpse_tfm_format, 
  kpse_afm_format, 
  kpse_base_format, 
  kpse_bib_format, 
  kpse_bst_format, 
  kpse_cnf_format,
  kpse_db_format,
  kpse_fmt_format,
  kpse_fontmap_format,
  kpse_mem_format,
  kpse_mf_format, 
  kpse_mfpool_format, 
  kpse_mft_format, 
  kpse_mp_format, 
  kpse_mppool_format, 
  kpse_mpsupport_format,
  kpse_ocp_format,
  kpse_ofm_format, 
  kpse_opl_format,
  kpse_otp_format,
  kpse_ovf_format,
  kpse_ovp_format,
  kpse_pict_format,
  kpse_tex_format,
  kpse_texdoc_format,
  kpse_texpool_format,
  kpse_texsource_format,
  kpse_tex_ps_header_format,
  kpse_troff_font_format,
  kpse_type1_format, 
  kpse_vf_format,
  kpse_dvips_config_format,
  kpse_ist_format,
  kpse_truetype_format,
  kpse_type42_format,
  kpse_web2c_format,
  kpse_program_text_format,
  kpse_program_binary_format,
  kpse_miscfonts_format,
  kpse_web_format,
  kpse_cweb_format,
  kpse_enc_format,
  kpse_cmap_format,
  kpse_sfd_format,
  kpse_opentype_format,
  kpse_pdftex_config_format,
  kpse_lig_format,
  kpse_texmfscripts_format };

static const char *const filetypenames[] = {
  "gf",
  "pk",
  "bitmap font",
  "tfm", 
  "afm", 
  "base", 
  "bib", 
  "bst", 
  "cnf",
  "ls-R",
  "fmt",
  "map",
  "mem",
  "mf", 
  "mfpool", 
  "mft", 
  "mp", 
  "mppool", 
  "MetaPost support",
  "ocp",
  "ofm", 
  "opl",
  "otp",
  "ovf",
  "ovp",
  "graphic/figure",
  "tex",
  "TeX system documentation",
  "texpool",
  "TeX system sources",
  "PostScript header",
  "Troff fonts",
  "type1 fonts", 
  "vf",
  "dvips config",
  "ist",
  "truetype fonts",
  "type42 fonts",
  "web2c files",
  "other text files",
  "other binary files",
  "misc fonts",
  "web",
  "cweb",
  "enc files",
  "cmap files",
  "subfont definition files",
  "opentype fonts",
  "pdftex config",
  "lig files",
  "texmfscripts",
  NULL };

static int find_file (lua_State *L) {
  int i;
  char *st;
  char *ret;
  int ftype = kpse_tex_format;
  int mexist = 0;
  if (!lua_isstring(L,1)) {
    lua_pushstring(L, "not a file name");
    lua_error(L);
  }  
  st = (char *)lua_tostring(L,1);
  i = lua_gettop(L);  
  while (i>1) {
	if (lua_isboolean (L,i)) {
	  mexist = lua_toboolean (L,i);
	} else if (lua_isnumber (L,i)) {
	  mexist = lua_tonumber (L,i) ? 1 : 0 ;
	} else if (lua_isstring(L,i)) {
	  int op = luaL_checkoption(L, i, NULL, filetypenames);
	  ftype = filetypes[op];
	}
	i--;
  }
  lua_pushstring(L, kpse_find_file (st,ftype,mexist));
  return 1;
}

static int expand_path (lua_State *L) {
  const char *st = luaL_checkstring(L,1);
  lua_pushstring(L, kpse_path_expand(st));
  return 1;
}

static int expand_braces (lua_State *L) {
  const char *st = luaL_checkstring(L,1);
  lua_pushstring(L, kpse_brace_expand(st));
  return 1;
}

static int expand_var (lua_State *L) {
  const char *st = luaL_checkstring(L,1);
  lua_pushstring(L, kpse_var_expand(st));
  return 1;
}


static const struct luaL_reg kpselib [] = {
  {"find_file", find_file},
  {"expand_path", expand_path},
  {"expand_var", expand_var},
  {"expand_braces",expand_braces},
  {NULL, NULL}  /* sentinel */
};


int
luaopen_kpse (lua_State *L) 
{
  luaL_register(L,"kpse",kpselib);
  return 1;
}

