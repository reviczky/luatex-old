
#include "luatex-api.h"
#include <ptexlib.h>

void
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

  if (char_name(f,k)!=NULL) {
	lua_pushstring(L,char_name(f,k));
	lua_setfield(L,-2,"name");
  }

  if (char_tag(f,k) == list_tag) {
    lua_pushnumber(L,char_remainder(f,k));
    lua_setfield(L,-2,"next");
  }

  lua_pushboolean(L,(char_used(f,k) ? true : false));
  lua_setfield(L,-2,"used");

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

static void
write_lua_parameters (lua_State *L, int f) {
  int k;
  lua_newtable(L);
  for (k=1;k<=font_params(f);k++) {
    lua_pushnumber(L,font_param(f,k));
    switch (k) {
    case slant_code:         lua_setfield(L,-2,"slant");         break;
    case space_code:         lua_setfield(L,-2,"space");         break;
    case space_stretch_code: lua_setfield(L,-2,"space_stretch"); break;
    case space_shrink_code:  lua_setfield(L,-2,"space_shrink");  break;
    case x_height_code:      lua_setfield(L,-2,"x_height");      break;
    case quad_code:          lua_setfield(L,-2,"quad");          break;
    case extra_space_code:   lua_setfield(L,-2,"extra_space");   break;
    default:
      lua_rawseti(L,-2,k);
    }
  }
  lua_setfield(L,-2,"parameters");
}


int
font_to_lua (lua_State *L, int f) {
  int k;
  if (font_cache_id(f)) {
    /* fetch the table from the registry if  it was 
       saved there by font_from_lua() */ 
    lua_rawgeti(L,LUA_REGISTRYINDEX,font_cache_id(f));
    /* fontdimens can be changed from tex code */
    write_lua_parameters(L,f);
    return 1;
  }

  lua_newtable(L);
  lua_pushstring(L,font_name(f));
  lua_setfield(L,-2,"name");
  if(font_area(f)!=NULL) {
	lua_pushstring(L,font_area(f));
	lua_setfield(L,-2,"area");
  }
  if(font_fullname(f)!=NULL) {
	lua_pushstring(L,font_fullname(f));
	lua_setfield(L,-2,"fullname");
  }
  if(font_encodingname(f)!=NULL) {
	lua_pushstring(L,font_encodingname(f));
	lua_setfield(L,-2,"encodingname");
  }

  lua_pushboolean(L,(font_used(f) ? true : false));
  lua_setfield(L,-2,"used");

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
  write_lua_parameters(L,f);
  
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
count_hash_items (lua_State *L, char *name){
  int n = -1;
  lua_getfield(L,-1,name);
  if (!lua_isnil(L,-1)) {
    if (lua_istable(L,-1)) {
      n = 0;
      /* now find the number */
      lua_pushnil(L);  /* first key */
      while (lua_next(L, -2) != 0) {
	n++;
	lua_pop(L,1);
      }
    }
  }
  lua_pop(L,1);
  return n;
}

#define streq(a,b) (strcmp(a,b)==0)

#define append_packet(k) { set_font_packet(f,np,k); np++; }

#define do_store_four(l) {							\
    append_packet((l&0xFF000000)>>24);				\
    append_packet((l&0x00FF0000)>>16);				\
    append_packet((l&0x0000FF00)>>8);				\
    append_packet((l&0x000000FF));  } 

/*
*/

static int
read_char_packets  (lua_State *L, integer *l_fonts, internal_font_number f, integer np) {
  int i, n, m;
  unsigned l;
  int cmd;
  char *s;
  int ff = 0;
  for (i=1;i<=lua_objlen(L,-1);i++) {
	lua_rawgeti(L,-1,i);
	if (lua_istable(L,-1)) {
	  /* fetch the command code */
	  lua_rawgeti(L,-1,1);
	  if (lua_isstring(L,-1)) {
		s = (char *)lua_tostring(L,-1);
		cmd = 0;
		if      (streq(s,"font"))    {  cmd = packet_font_code;     }
		else if (streq(s,"char"))    {  cmd = packet_char_code;     
		  if (ff==0) {
		    append_packet(packet_font_code);
		    ff = l_fonts[1];
		    do_store_four(ff);
		  }
		} 
		else if (streq(s,"push"))    {  cmd = packet_push_code;    } 
		else if (streq(s,"pop"))     {  cmd = packet_pop_code;     } 
		else if (streq(s,"rule"))    {  cmd = packet_rule_code;    }
		else if (streq(s,"right"))   {  cmd = packet_right_code;   }
		else if (streq(s,"down"))    {  cmd = packet_down_code;    }
		else if (streq(s,"special")) {  cmd = packet_special_code; } 
		
		append_packet(cmd);
		switch(cmd) {
		case packet_push_code:
		case packet_pop_code:
		  break;
		case packet_font_code:
		  lua_rawgeti(L,-2,2);
		  n = lua_tointeger(L,-1);
		  ff = l_fonts[n];
		  do_store_four(ff);
		  lua_pop(L,1);
		  break;
		case packet_char_code:
		  lua_rawgeti(L,-2,2);
		  n = lua_tointeger(L,-1);
		  do_store_four(n);
		  lua_pop(L,1);
		  break;
		case packet_right_code:
		case packet_down_code:
		  lua_rawgeti(L,-2,2);
		  n = lua_tointeger(L,-1);
		  /* TODO this multiplier relates to the font size, apparently */
		  do_store_four(((n<<4)/10));
		  lua_pop(L,1);
		  break;
		case packet_rule_code:
		  lua_rawgeti(L,-2,2);
		  n = lua_tointeger(L,-1);
		  /* here too, twice */
		  do_store_four(((n<<4)/10));
		  lua_rawgeti(L,-2,3);
		  n = lua_tointeger(L,-1);
		  do_store_four(((n<<4)/10));
		  lua_pop(L,2);
		case packet_special_code:
		  lua_rawgeti(L,-2,2);
		  s = (char *)lua_tolstring(L,-1,&l);
 		  if (l>0) {
		    do_store_four(l);
		    m = (int)l;
		    while(m>0) {
		      n = *s++;	  m--;
		      append_packet(n);
		    }
		  }
		  lua_pop(L,1);
		  break;
		default:
		  fprintf(stdout,"Unknown char packet code % in font %d\n",cmd,(int)f);
		}
	  }
	  lua_pop(L,1); /* command code */
	}
	lua_pop(L,1); /* command table */
  }
  append_packet(packet_end_code);
  return np;
}

static int
numeric_field (lua_State *L, char *name, int dflt) {
  int i = dflt;
  lua_getfield(L,-1,name);
  if (lua_isnumber(L,-1)) {	
    i = lua_tonumber(L,-1);
  }
  lua_pop(L,1);
  return i;
}

static int
boolean_field (lua_State *L, char *name, int dflt) {
  int i = dflt;
  lua_getfield(L,-1,name);
  if (lua_isboolean(L,-1)) {	
    i = lua_toboolean(L,-1);
  }
  lua_pop(L,1);
  return i;
}

static char *
string_field (lua_State *L, char *name, char *dflt) {
  char *i;
  lua_getfield(L,-1,name);
  if (lua_isstring(L,-1)) {	
    i = xstrdup(lua_tostring(L,-1));
  } else if (dflt==NULL) {
    i = NULL;
  } else {
    i = xstrdup(dflt);
  }
  lua_pop(L,1);
  return i;
}

static void
read_lua_parameters (lua_State *L, int f) {
  int i, n;
  char *s;
  lua_getfield(L,-1,"parameters");
  if (lua_istable(L,-1)) {	
    /* the number of parameters is the max(IntegerKeys(L)),7) */
    n = 7;
    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) {
      if (lua_isnumber(L,-2)) {
	i = lua_tonumber(L,-2);
	if (i > n)  n = i;
      }
      lua_pop(L,1); /* pop value */
    }

    if (n>7) set_font_params(f,n);

    /* sometimes it is handy to have all integer keys */
    for (i=1;i<=7;i++) {
      lua_rawgeti(L,-1,i);
      if (lua_isnumber(L,-1)) {
	n = lua_tointeger(L,-1);
	set_font_param(f,i, n);
      }
      lua_pop(L,1); 
    }

    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) {
      if (lua_isnumber(L,-2)) {
	i = lua_tointeger(L,-2);
	if (i>=8) {
	  n = (lua_isnumber(L,-1) ? lua_tointeger(L,-1) : 0);
	  set_font_param(f,i, n);
	}
      } else if (lua_isstring(L,-2)) {
	s = (char *)lua_tostring(L,-2);
	n = (lua_isnumber(L,-1) ? lua_tointeger(L,-1) : 0);

	if       (strcmp("slant",s)== 0)         {  set_font_param(f,slant_code,n); }
	else if  (strcmp("space",s)== 0)         {  set_font_param(f,space_code,n); }
	else if  (strcmp("space_stretch",s)== 0) {  set_font_param(f,space_stretch_code,n); }
	else if  (strcmp("space_shrink",s)== 0)  {  set_font_param(f,space_shrink_code,n); }
	else if  (strcmp("x_height",s)== 0)      {  set_font_param(f,x_height_code,n); }
	else if  (strcmp("quad",s)== 0)          {  set_font_param(f,quad_code,n); }
	else if  (strcmp("extra_space",s)== 0)   {  set_font_param(f,extra_space_code,n); }

      }
      lua_pop(L,1); 
    }
  }
  lua_pop(L,1);

}

/* The caller has fix the state of the lua stack when there is an error! */

boolean
font_from_lua (lua_State *L, int f) {
  int i,k,n,r,t;
  int s_top; /* lua stack top */
  scaled j;
  int bc; /* first char index */
  int ec; /* last char index */
  int nc; /* number of character info items */
  int ne; /* number of extensible table items */
  int nl; /* number of ligature table items */
  int nk; /* number of kern table items */
  int np; /* number of virtual packet bytes */
  int ctr;
  char *s;
  integer *l_fonts = NULL;
  /* the table is at stack index -1 */

  s = string_field(L,"area",NULL);               set_font_area(f,s);
  s = string_field(L,"fullname",NULL);           set_font_fullname(f,s);
  s = string_field(L,"encodingname",NULL);       set_font_encodingname(f,s);

  s = string_field(L,"name",NULL);               set_font_name(f,s);

  if (s==NULL) {
    pdftex_fail("lua-loaded font [%d] has no name!",f);
    return false;
  }

  i = numeric_field(L,"designsize",655360);      set_font_dsize(f,i);
  i = numeric_field(L,"size",font_dsize(f));     set_font_size(f,i);
  i = numeric_field(L,"checksum",0);             set_font_checksum(f,i);
  i = numeric_field(L,"direction",0);            set_font_natural_dir(f,i);
  i = numeric_field(L,"boundarychar_label",0);   set_bchar_label(f,i);
  i = numeric_field(L,"boundarychar",0);         set_font_bchar(f,i);
  i = numeric_field(L,"false_boundarychar",0);   set_font_false_bchar(f,i);
  i = numeric_field(L,"hyphenchar",get_default_hyphen_char()); set_hyphen_char(f,i);
  i = numeric_field(L,"skewchar",get_default_skew_char());     set_skew_char(f,i);
  i = boolean_field(L,"used",0);                 set_font_used(f,i);

  s = string_field(L,"type",NULL);
  if (s != NULL){
    if (strcmp(s,"virtual") == 0) {
      set_font_type(f,virtual_font_type);
    } else {
      set_font_type(f,real_font_type);
    }
  }
  
  /* now fetch the base fonts, if needed */
  if(font_type(f) == virtual_font_type) {
    n = count_hash_items(L,"fonts");
    if (n>0) {
      l_fonts = xmalloc((n+1)*sizeof(integer));
      memset (l_fonts,0,(n+1)*sizeof(integer));
      lua_getfield(L,-1,"fonts");
      for (i=1;i<=n;i++) {
	lua_rawgeti(L,-1,i);
	if (lua_istable(L,-1)) {
	  lua_rawgeti(L,-1, 1);
	  if (lua_isstring(L,-1)) {
	    s = (char *)lua_tostring(L,-1);
	    lua_rawgeti(L,-2, 2);
	    t = (lua_isnumber(L,-1) ? lua_tonumber(L,-1) : -1000);
	    lua_pop(L,1);

	    /* TODO: the stack is messed up, otherwise this 
	     * explicit resizing would not be needed 
	     */
	    s_top = lua_gettop(L);
	    l_fonts[i] = find_font_id(s,"",t);
	    lua_settop(L,s_top);
	  }
	  lua_pop(L,1); /* pop name */
	}
	lua_pop(L,1); /* pop list entry */
      }
      lua_pop(L,1); /* pop list entry */
    } else {
      set_font_type(f,new_font_type);
      fprintf(stderr, "No local fonts");
    }
  }

  /* parameters */
  read_lua_parameters(L,f);

  /* characters */
  lua_getfield(L,-1,"characters");
  if (lua_istable(L,-1)) {	
    /* find the array size values */
    ec = 0; bc = -1; nc = 0;	
    ne = 0; nl = 0; nk = 0; np = 0;
    lua_pushnil(L);  /* first key */
    while (lua_next(L, -2) != 0) {
      if (lua_isnumber(L,-2)) {
	i = lua_tointeger(L,-2);
	if (i>=0) {
	  if (lua_istable(L,-1)) {
	    nc++;
	    if (i>ec) ec = i;
	    if (bc<0) bc = i;
	    if (bc>=0 && i<bc) bc = i;
	    lua_getfield(L,-1,"extensible");
	    if (!lua_isnil(L,-1)) { ne++; }
	    lua_pop(L,1);
	    
	    k = count_hash_items(L,"kerns");
	    if (k>0) nk += k+1; 
	    k = count_hash_items(L,"ligatures");
	    if (k>0) nl += k+1; 

	    if (font_type(f)==virtual_font_type) {
	      lua_getfield(L,-1,"commands");
	      k = lua_objlen(L,-1);
	      /* this is an approximation only, but it will sort itself out
	       * anyway, so there is little point in being too precise.
	       * this pre-alloc fits perfectly for a single 'char' command, when
	       * the default font command will be prepended by the code below.
	       */
	      if (k>0) np += 5+(5*k)+1; 
	      lua_pop(L,1);
	    }
	  }
	}
      }
      lua_pop(L, 1);
    }

    if (bc != -1) {
      /* todo: check what happens if bc=ec=0 */
      set_font_bc(f,bc);
      set_font_ec(f,ec);
      set_char_infos(f,(ec+1)); /* have to use |ec| here instead of |nc| because |bc| can be nonzero */
      set_font_widths(f,(nc+1));
      set_font_heights(f,(nc+1));
      set_font_depths(f,(nc+1));
      set_font_italics(f,(nc+1));
      if (nk>0) set_font_kerns(f,(nk+1));
      if (nl>0) set_font_ligs(f,(nl+1));
      if (ne>0) set_font_extens(f,(ne+1));
      if (np>0) set_font_packets(f,(np+1));
      set_font_width(f,0,0);
      set_font_height(f,0,0);
      set_font_depth(f,0,0);
      set_font_italic(f,0,0);
      if (nk>0) set_font_kern(f,0,0,0);
      if (nl>0) set_font_lig(f,0,0,0,0);
      if (np>0) set_font_packet(f,0,0);

      /* second loop ... */
      nc = 0; ne = 1; nk = 1; nl = 1; np = 1;
      lua_pushnil(L);  /* first key */
      while (lua_next(L, -2) != 0) {
	if (lua_isnumber(L,-2)) {
	  i = lua_tonumber(L,-2);
	  if (i>=0) {
	    if (lua_istable(L,-1)) {
	      nc++;
	      /* character table */
	      j = numeric_field(L,"width",0);  set_font_width(f,nc,j);  width_index(f,i) = nc;
	      j = numeric_field(L,"height",0); set_font_height(f,nc,j); height_index(f,i) = nc;
	      j = numeric_field(L,"depth",0);  set_font_depth(f,nc,j);  depth_index(f,i) = nc;
	      j = numeric_field(L,"italic",0); set_font_italic(f,nc,j); italic_index(f,i) = nc;
	      
	      k = boolean_field(L,"used",0);   set_char_used(f,i,k);
	      s = string_field(L,"name",NULL); set_char_name(f,i,s);
	      
	      char_tag(f,i) = 0; 
	     
	      k = numeric_field(L,"next",-1); 
	      if (j>=0) {
		char_tag(f,i) = list_tag; 
		char_remainder(f,i) = k;
	      }
			  
	      lua_getfield(L,-1,"extensible");
	      if (lua_istable(L,-1)){ 
		char_tag(f,i) = ext_tag; 
		char_remainder(f,i) = ne++;
		k = numeric_field(L,"top",0);  ext_top(f,i) = k;
		k = numeric_field(L,"bot",0);  ext_bot(f,i) = k;
		k = numeric_field(L,"mid",0);  ext_mid(f,i) = k;
		k = numeric_field(L,"rep",0);  ext_rep(f,i) = k;
	      }
	      lua_pop(L,1);
		  
	      kern_index(f,i) = 0;
	      lua_getfield(L,-1,"kerns");
	      if (lua_istable(L,-1)) {  /* there are kerns */
		ctr = 0;
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
		  k = lua_tonumber(L,-2); /* adjacent char */
		  j = lua_tonumber(L,-1); /* movement */
		  if (!has_kern(f,i)) 
		    kern_index(f,i) = nk;
		  set_font_kern(f,nk,k,j);
		  ctr++;
		  nk++;
		  lua_pop(L,1);
		}
		if (ctr>0) {
		  set_font_kern(f,nk,end_ligkern,0);
		  nk++;
		}
	      }
	      lua_pop(L,1);


	      /* packet commands */
	      set_char_packet(f,i,0);
	      if(font_type(f)==virtual_font_type) {
		lua_getfield(L,-1,"commands");
		if (lua_istable(L,-1)){ 
		  set_char_packet(f,i,np);
		  np = read_char_packets(L,(integer *)l_fonts,f,np);
		}
		lua_pop(L,1);
	      }

	      /* ligatures */
	      lig_index(f,i) = 0;
	      lua_getfield(L,-1,"ligatures");
	      if (lua_istable(L,-1)){/* do ligs */
		ctr = 0;
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
		  t = lua_tonumber(L,-2); /* adjacent char */
		  if (lua_istable(L,-1)){ 
		    r = numeric_field(L,"char",-1); /* ligature */
		    if (r != -1) {		
		      k = numeric_field(L,"type",0); /* ligtype */
		      if (!has_lig(f,i)) 
			set_char_lig(f,i,nl);
		      set_font_lig(f,nl,k,t,r);
		      ctr++;
		      nl++;
		    }
		  }
		  lua_pop(L,1); /* iterator value */
		}
		/* guard against empty tables */
		if (ctr>0) {
		  set_font_lig(f,nl,0,end_ligkern,0);
		  nl++;
		}
	      }
	      lua_pop(L,1); /* ligatures table */
	    }
	  }
	}
	lua_pop(L, 1);
      }
      lua_pop(L, 1);
      
    } else { /* jikes, no characters */
      pdftex_warn("lua-loaded font [%d] has no characters!",f);
    }

    r = luaL_ref(Luas[0],LUA_REGISTRYINDEX); /* pops the table */
    set_font_cache_id(f,r);

  } else { /* jikes, no characters */
    pdftex_warn("lua-loaded font [%d] has no character table!",f);
  }
  if (l_fonts!=NULL) 
    free(l_fonts);
  return true;
}

