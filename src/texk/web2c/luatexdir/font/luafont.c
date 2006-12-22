
#include "luatex-api.h"
#include <ptexlib.h>

static void
font_char_to_lua (lua_State *L, internalfontnumber f, int k) {
  int i;
  liginfo l;
  lua_newtable(L);
  lua_pushnumber(L,char_width(f,k));
  lua_setfield(L,-2,"width");
  lua_pushnumber(L,char_height(f,k));
  lua_setfield(L,-2,"height");
  lua_pushnumber(L,char_depth(f,k));
  lua_setfield(L,-2,"depth");
  lua_pushnumber(L,char_italic(f,k));
  lua_setfield(L,-2,"italic");

  if (char_tag(f,k) == list_tag) {
    lua_pushnumber(L,char_remainder(f,k));
    lua_setfield(L,-2,"next");
  }
  if (char_tag(f,k) == ext_tag) {
    lua_newtable(L);			  
    lua_pushnumber(L,ext_top(f,k));
    lua_setfield(L,-2,"top");
    lua_pushnumber(L,ext_mid(f,k));
    lua_setfield(L,-2,"mid");
    lua_pushnumber(L,ext_rep(f,k));
    lua_setfield(L,-2,"rep");
    lua_pushnumber(L,ext_bot(f,k));
    lua_setfield(L,-2,"bot");
    lua_setfield(L,-2,"extensible");
  }
  if (has_kern(f,k)) {
    lua_newtable(L);			  
    for (i=kern_index(f,k);!kern_end(f,i);i++) {
      lua_pushnumber(L,kern_char(f,i));
      lua_pushnumber(L,kern_kern(f,i));
      lua_rawset(L,-3);
    }
    lua_setfield(L,-2,"kerns");
  }
  if (has_lig(f,k)) {
    lua_newtable(L);			  
    for (i=lig_index(f,k);!lig_end(font_lig(f,i));i++) {
      l=font_lig(f,i);
      lua_pushnumber(L,lig_char(l));
      
      lua_newtable(L);			  				  
      lua_pushnumber(L,lig_type(l));
      lua_setfield(L,-2,"type");
      lua_pushnumber(L,lig_replacement(l));
      lua_setfield(L,-2,"char");
      
      lua_rawset(L,-3);
    }
    lua_setfield(L,-2,"ligatures");
  }
}

int
font_to_lua (lua_State *L, int f, char *cnom) {
  int k;
  lua_newtable(L);
  lua_pushstring(L,cnom);
  lua_setfield(L,-2,"name");
  lua_pushstring(L,"");
  lua_setfield(L,-2,"area");
  lua_pushnumber(L,font_size(f));
  lua_setfield(L,-2,"size");
  lua_pushnumber(L,font_dsize(f));
  lua_setfield(L,-2,"designsize");
  lua_pushnumber(L,font_checksum(f));
  lua_setfield(L,-2,"checksum");
  lua_pushnumber(L,font_natural_dir(f));
  lua_setfield(L,-2,"direction");
  lua_pushnumber(L,bchar_label(f));
  lua_setfield(L,-2,"boundarychar_label");
  lua_pushnumber(L,font_bchar(f));
  lua_setfield(L,-2,"boundarychar");
  lua_pushnumber(L,font_false_bchar(f));
  lua_setfield(L,-2,"false_boundarychar");

  /* params */
  lua_newtable(L);
  for (k=1;k<=font_params(f);k++) {
    lua_pushnumber(L,font_param(f,k));
    lua_rawseti(L,-2,k);
  }
  lua_setfield(L,-2,"parameters");
  
  /* chars */
  lua_newtable(L); /* all characters */
  for (k=font_bc(f);k<=font_ec(f);k++) {
    if (char_exists(f,k)) {
      lua_pushnumber(L,k);
      font_char_to_lua(L,f,k);
      lua_rawset(L,-3);
    }
  }
  lua_setfield(L,-2,"characters");
  return 1;
}

static int 
count_hash_items (lua_State *L){
  int n = 0;
  if (!lua_isnil(L,-1)) {
	if (lua_istable(L,-1)) {
	  /* now find the number */
	  lua_pushnil(L);  /* first key */
	  while (lua_next(L, -2) != 0) {
		n++;
		lua_pop(L,1);
	  }
	}
  }
  return n;
}

int
font_from_lua (lua_State *L) {
  int f,i,k,n;
  scaled j;
  int bc,ec,nc,nk,nl,ne;
  char *s;
  f = new_font(fontptr+1);
  
  /* at -1, there is a table */
  lua_getfield(L,-1,"area");
  if (lua_isstring(L,-1)) {
	s = xstrdup(lua_tostring(L,-1));
  } else {
	s = xstrdup("");
  }
  set_font_area(f,s);
  lua_pop(L,1);

  lua_getfield(L,-1,"name");
  if (lua_isstring(L,-1)) {
	s = xstrdup(lua_tostring(L,-1));
	set_font_name(f,s);
	lua_pop(L,1);
  } else {
	lua_pop(L,1);
	/* fatal */
	lua_pop(L,1);
	return 0;
  }

  lua_getfield(L,-1,"designsize");
  if (lua_isnumber(L,-1)) {	
	set_font_dsize(f,lua_tonumber(L,-1));
  } else {
	set_font_dsize(f,655360);
  }
  lua_pop(L,1);

  lua_getfield(L,-1,"size");
  if (lua_isnumber(L,-1)) {	
	set_font_size(f,lua_tonumber(L,-1));
	lua_pop(L,1);
  } else {
	set_font_size(f,font_dsize(f));
  }

  lua_getfield(L,-1,"checksum");
  if (lua_isnumber(L,-1)) {	
	set_font_checksum(f,lua_tonumber(L,-1));
  } else {
	set_font_checksum(f,0);
  }
  lua_pop(L,1);

  lua_getfield(L,-1,"direction");
  if (lua_isnumber(L,-1)) {	
	set_font_natural_dir(f,lua_tonumber(L,-1));
  } else {
	set_font_natural_dir(f,0);
  }
  lua_pop(L,1);

  lua_getfield(L,-1,"boundarychar_label");
  if (lua_isnumber(L,-1)) {	
	set_bchar_label(f,lua_tonumber(L,-1));
  } else {
	set_bchar_label(f,0);
  }
  lua_pop(L,1);

  lua_getfield(L,-1,"boundarychar");
  if (lua_isnumber(L,-1)) {	
	set_font_bchar(f,lua_tonumber(L,-1));
  } else {
	set_font_bchar(f,0);
  }
  lua_pop(L,1);

  lua_getfield(L,-1,"false_boundarychar");
  if (lua_isnumber(L,-1)) {	
	set_font_false_bchar(f,lua_tonumber(L,-1));
  } else {
	set_font_false_bchar(f,0);
  }
  lua_pop(L,1);
  
  lua_getfield(L,-1,"hyphenchar");
  if (lua_isnumber(L,-1)) {	
	set_hyphen_char(f,lua_tonumber(L,-1));
  } else {
	set_hyphen_char(f,getdefaulthyphenchar());
  }
  lua_pop(L,1);
  
  lua_getfield(L,-1,"skewchar");
  if (lua_isnumber(L,-1)) {	
	set_skew_char(f,lua_tonumber(L,-1));
  } else {
	set_skew_char(f,getdefaultskewchar());
  }
  lua_pop(L,1);

  /* parameters */

  lua_getfield(L,-1,"parameters");
  if (lua_istable(L,-1)) {	
	n = lua_objlen(L,-1);
	if (n>7) set_font_params(f,n);
	for (i=1;i<=n;i++) {
	  lua_rawgeti(L,-1,i);
	  if (lua_isnumber(L,-1)) {	
		set_font_param(f,i,lua_tonumber(L,-1));
	  } else {
		set_font_param(f,i,0);
	  }
	  lua_pop(L,1);
	}
  } /* else clause handled by new_font() */

  /* characters */

  lua_getfield(L,-1,"characters");
  if (lua_istable(L,-1)) {	
	/* find the bc and ec values */
	ec = 0; bc = -1;
	ne = 0; /* extens */
	nl = 0; /* ligs */
	nk = 0; /* kerns */
	nc = 0; /* characters */
	lua_pushnil(L);  /* first key */
	while (lua_next(L, -2) != 0) {
	  /* value is now -1, new key at -2 */
	  if (lua_isnumber(L,-2)) {
		i = lua_tonumber(L,-2);
		if (i>=0) {
		  if (lua_istable(L,-1)) {
			nc++;
			if (i>ec) ec = i;
			if (bc<0) bc = i;
			if (bc>=0 && i<bc) bc = i;
			lua_getfield(L,-1,"extensible");
			if (!lua_isnil(L,-1)) {
			  ne++;
			}
			lua_pop(L,1);
			
			lua_getfield(L,-1,"kerns");
			k = count_hash_items(L);
			if (k>0) nk += k+1; 
			lua_pop(L,1);

			lua_getfield(L,-1,"ligatures");
			k = count_hash_items(L);
			if (k>0) nl += k+1; 
			lua_pop(L,1);

		  }
		}
	  }
	  lua_pop(L, 1);
	}
	if (bc != -1) {
	  /* todo: check what happens if bc=ec=0 */
	  set_font_bc(f,bc);
	  set_font_ec(f,ec);
	  set_char_infos(f,(ec+1));
	  set_font_widths(f,(nc+1));
	  set_font_heights(f,(nc+1));
	  set_font_depths(f,(nc+1));
	  set_font_italics(f,(nc+1));
	  if (nk>0)	set_font_kerns(f,(nk+1));
	  if (nl>0) set_font_ligs(f,(nl+1));
	  if (ne>0)	set_font_extens(f,ne);
	  /* second loop ... */

	  nc = 0; ne = 0; nk = 1; nl = 1;
	  set_font_width(f,0,0);
	  set_font_height(f,0,0);
	  set_font_depth(f,0,0);
	  set_font_italic(f,0,0);
	  set_font_kern(f,0,0,0);
	  set_font_lig(f,0,0,0,0);
	  lua_pushnil(L);  /* first key */
	  while (lua_next(L, -2) != 0) {
		if (lua_isnumber(L,-2)) {
		  i = lua_tonumber(L,-2);
		  if (i>=0) {
			if (lua_istable(L,-1)) {
			  nc++;
			  /* character table */
			  lua_getfield(L,-1,"width");
			  if (lua_isnumber(L,-1)) j = lua_tonumber(L,-1);  else j = 0;			 
			  set_font_width(f,nc,j);  width_index(f,i) = nc;
			  lua_pop(L,1);

			  lua_getfield(L,-1,"height");
			  if (lua_isnumber(L,-1)) j = lua_tonumber(L,-1);  else j = 0;			 
			  set_font_height(f,nc,j); height_index(f,i) = nc;
			  lua_pop(L,1);

			  lua_getfield(L,-1,"depth");
			  if (lua_isnumber(L,-1)) j = lua_tonumber(L,-1);  else j = 0; 
			  set_font_depth(f,nc,j); depth_index(f,i) = nc;
			  lua_pop(L,1);

			  lua_getfield(L,-1,"italic");
			  if (lua_isnumber(L,-1)) j = lua_tonumber(L,-1);  else j = 0; 
			  set_font_italic(f,nc,j); italic_index(f,i) = nc;
			  lua_pop(L,1);

			  lua_getfield(L,-1,"next");
			  if (lua_isnumber(L,-1)) k = lua_tonumber(L,-1);  else k = 0;
			  char_tag(f,i) = list_tag; char_remainder(f,i) = k;
			  lua_pop(L,1);
			  
			  lua_getfield(L,-1,"extensible");
			  if (lua_istable(L,-1)){ 
				char_tag(f,i) = ext_tag; 
				char_remainder(f,i) = ne++;

				lua_getfield(L,-1,"top");
				if (lua_isnumber(L,-1)) ext_top(f,i) = lua_tonumber(L,-1);
                else ext_top(f,i) = 0;
				lua_pop(L,1);

				lua_getfield(L,-1,"bot");
				if (lua_isnumber(L,-1)) ext_bot(f,i) = lua_tonumber(L,-1);
                else ext_bot(f,i) = 0;
				lua_pop(L,1);

				lua_getfield(L,-1,"mid");
				if (lua_isnumber(L,-1)) ext_mid(f,i) = lua_tonumber(L,-1);
                else ext_mid(f,i) = 0;
				lua_pop(L,1);

				lua_getfield(L,-1,"rep");
				if (lua_isnumber(L,-1)) ext_rep(f,i) = lua_tonumber(L,-1);
                else ext_rep(f,i) = 0;
				lua_pop(L,1);
			  } 
			  lua_pop(L,1);

			  lua_getfield(L,-1,"kerns");
			  if (lua_istable(L,-1)){
				/* */
				kern_index(f,i) = nk;
				lua_pushnil(L);  /* first key */
				while (lua_next(L, -2) != 0) {
				  set_font_kern(f,nk,lua_tonumber(L,-2),lua_tonumber(L,-1));
				  lua_pop(L,1);
				  nk++;
				}
				set_font_kern(f,nk,end_ligkern,0);
				nk++;
			  }
			  lua_pop(L,1);

			  lua_getfield(L,-1,"ligatures");
			  if (lua_istable(L,-1)){
				/* do ligs */
				lua_pushnil(L);  /* first key */
				while (lua_next(L, -2) != 0) {
				  /* */
				  if (lua_istable(L,-1)){
					/* */
				    lig_index(f,i) = nl;
					lua_getfield(L,-1,"type");
					k = lua_tonumber(L,-1);
					lua_pop(L,1);

					lua_getfield(L,-1,"char");
					set_font_lig(f,nl,k,lua_tonumber(L,-2),lua_tonumber(L,-1));
					lua_pop(L,1);
					nl++;
				  }
				  lua_pop(L,1);
				}
				set_font_lig(f,nl,0,end_ligkern,0);
				nl++;
			  }
			  lua_pop(L,1);
			}
		  }
		}
	  }
	  lua_pop(L, 1);
	  
	} else {
	   /* jikes, no characters */
	 }
	 lua_pop(L,1); /* pop the table */
  } else {
	/* jikes, no characters */
  }
	
  return 1;
}

