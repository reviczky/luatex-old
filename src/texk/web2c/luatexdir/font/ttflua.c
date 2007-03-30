/*
Copyright (c) 1996-2006 Taco Hoekwater <taco@luatex.org>

This file is part of LuaTeX.

LuaTeX is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

LuaTeX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LuaTeX; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

$Id$
*/

#include "ptexlib.h"
#include "luatex-api.h"
#include "ttf.h"

static void 
dump_intfield (lua_State *L, char *name, long int field) {
  lua_pushstring(L,name);
  lua_pushnumber(L,field);
  lua_rawset(L,-3);
}

static void 
dump_stringfield (lua_State *L, char *name, char *field) {
  lua_pushstring(L,name);
  lua_pushstring(L,field);
  lua_rawset(L,-3);
}

static void 
dump_char_ref (lua_State *L, struct splinechar *spchar) {
  
  lua_pushstring(L,"char");
  lua_pushstring(L,spchar->name);
  lua_rawset(L,-3);
}


static void 
dump_lstringfield (lua_State *L, char *name, char *field, int len) {
  lua_pushstring(L,name);
  lua_pushlstring(L,field,len);
  lua_rawset(L,-3);
}

static void 
dump_enumfield (lua_State *L, char *name, int fid, char **fields) {
  lua_pushstring(L,name);
  lua_pushstring(L,fields[fid]);
  lua_rawset(L,-3);
}

static void 
dump_floatfield (lua_State *L, char *name, double field) {
  lua_pushstring(L,name);
  lua_pushnumber(L,field);
  lua_rawset(L,-3);
}

static char tag_string [5] = {0};

static char *make_tag_string (unsigned int field) {
  tag_string[0] = (field&0xFF000000) >> 24;
  tag_string[1] = (field&0x00FF0000) >> 16;
  tag_string[2] = (field&0x0000FF00) >> 8;
  tag_string[3] = (field&0x000000FF);
  return (char *)tag_string;
}

static void 
dump_tag (lua_State *L, char *name, unsigned int field) {
  lua_pushstring(L,name);
  lua_pushstring(L,make_tag_string(field));
  lua_rawset(L,-3);
}

#define NESTED_TABLE(a,b,c) {			\
    int k = 1;							\
    lua_pushnumber(L,k); k++;			\
    lua_createtable(L,0,c);				\
    a(L,b);								\
    lua_rawset(L,-3);					\
    next = b->next;						\
    while (next != NULL) {				\
      lua_pushnumber(L,k); k++;			\
      lua_createtable(L,0,c);			\
      a(L, next);						\
      lua_rawset(L,-3);					\
      next = next->next;				\
    } }




void
do_handle_kernpair (lua_State *L, struct kernpair *kp) {

  if (kp->sc != NULL)
    dump_char_ref(L, kp->sc); 

  dump_intfield(L,"off",          kp->off);
  dump_intfield(L,"sli",          kp->sli);
  dump_intfield(L,"flags",        kp->flags);

  /* dump_intfield(L,"kcid",         kp->kcid); */

#ifdef FONTFORGE_CONFIG_DEVICETABLES
  if (kp->adjusts != NULL) {
    lua_newtable(L);
    handle_devicetab(L,kerns->adjusts[k]);
    lua_setfield(L,-2,"adjusts");
  }
#endif

}

void
handle_kernpair (lua_State *L, struct kernpair *kp) {
  struct kernpair *next;
  NESTED_TABLE(do_handle_kernpair,kp,4);
}

void
handle_splinecharlist (lua_State *L, struct splinecharlist *scl) {

  struct splinecharlist *next;
  int k = 1;

  if (scl->sc != NULL) {
    lua_pushnumber(L,k); k++;
    dump_char_ref(L, scl->sc); 
    lua_rawset(L,-3);
  }
  next = scl->next;
  while( next != NULL) {
    if (next->sc != NULL) {
      lua_pushnumber(L,k); k++;
      dump_char_ref(L,next->sc); 
      lua_rawset(L,-3);
    }
    next = next->next;
  }
}

void handle_vr (lua_State *L, struct vr *pos) {

  dump_intfield(L,"xoff",              pos->xoff); 
  dump_intfield(L,"yoff",              pos->yoff); 
  dump_intfield(L,"h_adv_off",         pos->h_adv_off); 
  dump_intfield(L,"v_adv_off",         pos->v_adv_off);

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    /*  ValDevTab *adjust; */ /* TH TODO ? */
#endif

}

char *possub_type_enum[] = { 
  "null", "position", "pair",  "substitution", "alternate",
  "multiple", "ligature", "lcaret",  "kerning", "vkerning", "anchors",
  "contextpos", "contextsub", "chainpos", "chainsub",
  "reversesub", "max", "kernback", "vkernback" };


void
do_handle_generic_pst (lua_State *L, struct generic_pst *pst) {

  dump_enumfield(L,"type",             pst->type, possub_type_enum); 

  /*dump_intfield(L,"macfeature",        pst->macfeature); */
  dump_intfield(L,"flags",             pst->flags); 
  dump_tag(L,"tag",                    pst->tag); 
  dump_intfield(L,"script_lang_index", (pst->script_lang_index+1)); 

  if (pst->type == pst_position) {
	lua_pushstring(L,"pos");
    lua_createtable(L,0,4);
    handle_vr (L, &pst->u.pos);
    lua_rawset(L,-3);

  } else if (pst->type == pst_pair) {

	lua_pushstring(L,"pair");
    lua_createtable(L,0,2);
    dump_stringfield(L,"paired",pst->u.pair.paired);
    if (pst->u.pair.vr != NULL) {
	  lua_pushstring(L,"vr");
      lua_createtable(L,0,4);
      handle_vr (L, pst->u.pair.vr);
      lua_rawset(L,-3);
    }
    lua_rawset(L,-3);

  } else if (pst->type == pst_substitution) {

	lua_pushstring(L,"subs");
    lua_newtable(L);
    dump_stringfield(L,"variant",pst->u.subs.variant);
    lua_rawset(L,-3);

  } else if (pst->type == pst_alternate) {

	lua_pushstring(L,"alt");
    lua_newtable(L);
    dump_stringfield(L,"components",pst->u.mult.components);
    lua_rawset(L,-3);

  } else if (pst->type == pst_multiple) {

	lua_pushstring(L,"mult");
    lua_newtable(L);
    dump_stringfield(L,"components",pst->u.alt.components);
    lua_rawset(L,-3);

  } else if (pst->type == pst_ligature) {
    
    lua_newtable(L);
    dump_stringfield(L,"components",pst->u.lig.components);
    if (pst->u.lig.lig != NULL) {
      dump_char_ref(L,pst->u.lig.lig);
    }
    lua_setfield(L,-2,"lig");

  } else if (pst->type == pst_lcaret) {

    lua_newtable(L);
    for (k=0;k<pst->u.lcaret.cnt;k++) {
      lua_pushnumber(L,(k+1));
      lua_pushnumber(L,pst->u.lcaret.carets[k]);
      lua_rawset(L,-3);
    }
    lua_setfield(L,-2,"lcaret");
  }
}


void
handle_generic_pst (lua_State *L, struct generic_pst *pst) {
  struct generic_pst *next;
  NESTED_TABLE(do_handle_generic_pst,pst,6);
}

void
do_handle_liglist (lua_State *L, struct liglist *ligofme) {
  if(ligofme->lig != NULL) {
    lua_createtable(L,0,6);
    handle_generic_pst (L,ligofme->lig);
    lua_setfield(L,-2,"lig");    
  }
  dump_char_ref(L,ligofme->first);
  if (ligofme->components != NULL) {
    lua_newtable(L);
    handle_splinecharlist (L,ligofme->components);
    lua_setfield(L,-2,"components");    
  }
  dump_intfield(L,"ccnt",ligofme->ccnt); 
}

void
handle_liglist (lua_State *L, struct liglist *ligofme) {
  struct liglist *next;
  NESTED_TABLE(do_handle_liglist,ligofme,3);
}

void 
handle_splinechar (lua_State *L,struct splinechar *glyph) {
  
  dump_stringfield(L,"name",        glyph->name);
  dump_intfield(L,"unicodeenc",     glyph->unicodeenc);

  lua_createtable(L,4,0);
  lua_pushnumber(L,1);  lua_pushnumber(L,glyph->xmin); lua_rawset(L,-3);
  lua_pushnumber(L,2);  lua_pushnumber(L,glyph->ymin); lua_rawset(L,-3);
  lua_pushnumber(L,3);  lua_pushnumber(L,glyph->xmax); lua_rawset(L,-3);
  lua_pushnumber(L,4);  lua_pushnumber(L,glyph->ymax); lua_rawset(L,-3);
  lua_setfield(L,-2,"boundingbox");

  dump_intfield(L,"orig_pos",       glyph->orig_pos);
  dump_intfield(L,"width",          glyph->width);
  dump_intfield(L,"vwidth",         glyph->vwidth);
  dump_intfield(L,"lsidebearing",   glyph->lsidebearing); 

  /* Layer layers[2];	*/	/* TH Not used */
  /*  int layer_cnt;    */	/* TH Not used */
  /*  StemInfo *hstem;  */	/* TH Not used */
  /*  StemInfo *vstem;	*/	/* TH Not used */
  /*  DStemInfo *dstem;	*/	/* TH Not used */
 
  /* MinimumDistance *md; */    /* TH Not used */
  /* struct charview *views; */ /* TH Not used */
  /* struct charinfo *charinfo;  */ /* TH ? (charinfo.c) */

  /*  struct splinefont *parent; */  /* TH Not used */

  dump_intfield(L,"ticked",                   glyph->ticked); 
  dump_intfield(L,"widthset",                 glyph->widthset); 
  dump_intfield(L,"glyph_class",              glyph->glyph_class);  

  /* TH: internal fontforge stuff
     dump_intfield(L,"ttf_glyph",                glyph->ttf_glyph); 
     dump_intfield(L,"changed",                  glyph->changed); 
     dump_intfield(L,"changedsincelasthinted",   glyph->changedsincelasthinted); 
     dump_intfield(L,"manualhints",              glyph->manualhints); 
     dump_intfield(L,"changed_since_autosave",   glyph->changed_since_autosave); 
     dump_intfield(L,"vconflicts",               glyph->vconflicts); 
     dump_intfield(L,"hconflicts",               glyph->hconflicts); 
     dump_intfield(L,"anyflexes",                glyph->anyflexes); 
     dump_intfield(L,"searcherdummy",            glyph->searcherdummy); 
     dump_intfield(L,"changed_since_search",     glyph->changed_since_search); 
     dump_intfield(L,"wasopen",                  glyph->wasopen); 
     dump_intfield(L,"namechanged",              glyph->namechanged); 
     dump_intfield(L,"blended",                  glyph->blended); 
     dump_intfield(L,"unused_so_far",            glyph->unused_so_far); 
     dump_intfield(L,"numberpointsbackards",     glyph->numberpointsbackards);  
     dump_intfield(L,"instructions_out_of_date", glyph->instructions_out_of_date);  
     dump_intfield(L,"complained_about_ptnums",  glyph->complained_about_ptnums);
  */

 if (glyph->kerns != NULL) {
   lua_newtable(L);
   handle_kernpair(L,glyph->kerns);
   lua_setfield(L,-2,"kerns");
 }
 if (glyph->vkerns != NULL) {
   lua_newtable(L);
   handle_kernpair(L,glyph->vkerns);
   lua_setfield(L,-2,"vkerns");
 }

 if (glyph->dependents != NULL) {
   lua_newtable(L);
   handle_splinecharlist(L,glyph->dependents);
   lua_setfield(L,-2,"dependents");
   
 }
 if (glyph->possub != NULL) {
   lua_newtable(L);
   handle_generic_pst(L,glyph->possub);
   lua_setfield(L,-2,"possub");
 }

 if (glyph->ligofme != NULL) {
   lua_newtable(L);
   handle_liglist(L,glyph->ligofme);
   lua_setfield(L,-2,"ligofme");
 }

 if (glyph->comment != NULL)
   dump_stringfield(L,"comment",              glyph->comment);

 if (glyph->color != COLOR_DEFAULT)
   dump_intfield(L,"color",                   glyph->color);  
  
  /* AnchorPoint *anchor; */ /* TH tobedone ? */
  /*
  uint8 *ttf_instrs;
  int16 ttf_instrs_len;
  int16 countermask_cnt;
  HintMask *countermasks;
  */

  if (glyph->tex_height != TEX_UNDEF)
    dump_intfield(L,"tex_height",              glyph->tex_height);  
  if (glyph->tex_depth != TEX_UNDEF)
    dump_intfield(L,"tex_depth",               glyph->tex_depth);  
  if (glyph->tex_sub_pos != TEX_UNDEF)
    dump_intfield(L,"tex_sub_pos",             glyph->tex_sub_pos);  
  if (glyph->tex_super_pos != TEX_UNDEF)
    dump_intfield(L,"tex_super_pos",           glyph->tex_super_pos);  
    
  /*  struct altuni { struct altuni *next; int unienc; } *altuni; */
}

char *panose_values_0[] = { "Any", "No Fit", "Text and Display", "Script", "Decorative", "Pictorial" };

char *panose_values_1[] = { "Any", "No Fit", "Cove", "Obtuse Cove", "Square Cove", "Obtuse Square Cove",
			    "Square", "Thin", "Bone", "Exaggerated", "Triangle", "Normal Sans",
			    "Obtuse Sans", "Perp Sans", "Flared", "Rounded" } ;

char *panose_values_2[] = { "Any", "No Fit", "Very Light", "Light", "Thin", "Book",
			    "Medium", "Demi", "Bold", "Heavy", "Black", "Nord" } ;

char *panose_values_3[] = { "Any", "No Fit", "Old Style", "Modern", "Even Width",
			    "Expanded", "Condensed", "Very Expanded", "Very Condensed", "Monospaced" };

char *panose_values_4[] = { "Any", "No Fit", "None", "Very Low", "Low", "Medium Low",
			    "Medium", "Medium High", "High", "Very High" };

char *panose_values_5[] = { "Any", "No Fit", "Gradual/Diagonal", "Gradual/Transitional","Gradual/Vertical", 
			    "Gradual/Horizontal", "Rapid/Vertical",  "Rapid/Horizontal",  "Instant/Vertical" };

char *panose_values_6[] = {"Any","No Fit","Straight Arms/Horizontal","Straight Arms/Wedge","Straight Arms/Vertical",
			   "Straight Arms/Single Serif","Straight Arms/Double Serif","Non-Straight Arms/Horizontal",
			   "Non-Straight Arms/Wedge","Non-Straight Arms/Vertical","Non-Straight Arms/Single Serif",
			   "Non-Straight Arms/Double Serif" };

char *panose_values_7[] = { "Any", "No Fit","Normal/Contact","Normal/Weighted","Normal/Boxed","Normal/Flattened",
			    "Normal/Rounded","Normal/Off Center","Normal/Square","Oblique/Contact","Oblique/Weighted",
			    "Oblique/Boxed","Oblique/Flattened","Oblique/Rounded","Oblique/Off Center","Oblique/Square" };

char *panose_values_8[] = { "Any","No Fit","Standard/Trimmed","Standard/Pointed","Standard/Serifed","High/Trimmed",
			    "High/Pointed","High/Serifed","Constant/Trimmed","Constant/Pointed","Constant/Serifed",
			    "Low/Trimmed","Low/Pointed","Low/Serifed"};

char *panose_values_9[] = { "Any","No Fit", "Constant/Small",  "Constant/Standard",
			    "Constant/Large", "Ducking/Small", "Ducking/Standard", "Ducking/Large" };


void 
handle_pfminfo (lua_State *L, struct pfminfo pfm) {

  dump_intfield (L, "pfmset",            pfm.pfmset);
  dump_intfield (L, "winascent_add",     pfm.winascent_add);
  dump_intfield (L, "windescent_add",    pfm.windescent_add);
  dump_intfield (L, "hheadascent_add",   pfm.hheadascent_add);
  dump_intfield (L, "hheaddescent_add",  pfm.hheaddescent_add);
  dump_intfield (L, "typoascent_add",    pfm.typoascent_add);
  dump_intfield (L, "typodescent_add",   pfm.typodescent_add);
  dump_intfield (L, "subsuper_set",      pfm.subsuper_set);
  dump_intfield (L, "panose_set",        pfm.panose_set);
  dump_intfield (L, "hheadset",          pfm.hheadset);
  dump_intfield (L, "vheadset",          pfm.vheadset);
  dump_intfield (L, "pfmfamily",         pfm.pfmfamily);
  dump_intfield (L, "weight",            pfm.weight);
  dump_intfield (L, "width",             pfm.width);
  dump_intfield (L, "avgwidth",          pfm.avgwidth);
  dump_intfield (L, "firstchar",         pfm.firstchar);
  dump_intfield (L, "lastchar",          pfm.lastchar);

  lua_createtable(L,0,10);
  dump_enumfield(L,"familytype",      pfm.panose[0], panose_values_0);
  dump_enumfield(L,"serifstyle",      pfm.panose[1], panose_values_1);
  dump_enumfield(L,"weight",          pfm.panose[2], panose_values_2);
  dump_enumfield(L,"proportion",      pfm.panose[3], panose_values_3);
  dump_enumfield(L,"contrast",        pfm.panose[4], panose_values_4);
  dump_enumfield(L,"strokevariation", pfm.panose[5], panose_values_5);
  dump_enumfield(L,"armstyle",        pfm.panose[6], panose_values_6);
  dump_enumfield(L,"letterform",      pfm.panose[7], panose_values_7);
  dump_enumfield(L,"midline",         pfm.panose[8], panose_values_8);
  dump_enumfield(L,"xheight",         pfm.panose[9], panose_values_9);
  lua_setfield  (L,-2,"panose");

  dump_intfield (L, "fstype",            pfm.fstype);
  dump_intfield (L, "linegap",           pfm.linegap);
  dump_intfield (L, "vlinegap",          pfm.vlinegap);
  dump_intfield (L, "hhead_ascent",      pfm.hhead_ascent);
  dump_intfield (L, "hhead_descent",     pfm.hhead_descent);
  dump_intfield (L, "hhead_descent",     pfm.hhead_descent);
  dump_intfield (L, "os2_typoascent",     pfm.os2_typoascent  );
  dump_intfield (L, "os2_typodescent",    pfm.os2_typodescent );
  dump_intfield (L, "os2_typolinegap",    pfm.os2_typolinegap );
  dump_intfield (L, "os2_winascent",      pfm.os2_winascent	  );
  dump_intfield (L, "os2_windescent",     pfm.os2_windescent  );
  dump_intfield (L, "os2_subxsize",       pfm.os2_subxsize	  );
  dump_intfield (L, "os2_subysize",       pfm.os2_subysize	  );
  dump_intfield (L, "os2_subxoff",        pfm.os2_subxoff	  );
  dump_intfield (L, "os2_subyoff",        pfm.os2_subyoff	  );
  dump_intfield (L, "os2_supxsize",       pfm.os2_supxsize	  );
  dump_intfield (L, "os2_supysize",       pfm.os2_supysize	  );
  dump_intfield (L, "os2_supxoff",        pfm.os2_supxoff	  );
  dump_intfield (L, "os2_supyoff",        pfm.os2_supyoff	  );
  dump_intfield (L, "os2_strikeysize",    pfm.os2_strikeysize );
  dump_intfield (L, "os2_strikeypos",     pfm.os2_strikeypos  );
  dump_stringfield (L, "os2_vendor",      pfm.os2_vendor);
  dump_intfield (L, "os2_family_class",   pfm.os2_family_class);
  dump_intfield (L, "os2_xheight",        pfm.os2_xheight);
  dump_intfield (L, "os2_capheight",      pfm.os2_capheight);
  dump_intfield (L, "os2_defaultchar",    pfm.os2_defaultchar);
  dump_intfield (L, "os2_breakchar",      pfm.os2_breakchar);
  
}


void 
do_handle_enc (lua_State *L, struct enc *enc) { 
  int i;
  
  dump_stringfield(L,"enc_name", enc->enc_name);
  dump_intfield  (L,"char_cnt", enc->char_cnt);

  if (enc->char_cnt && enc->unicode != NULL) {
	lua_createtable(L,enc->char_cnt,1);
	for (i=0;i<enc->char_cnt;i++) {
	  lua_pushnumber(L,i);
	  lua_pushnumber(L,enc->unicode[i]);
	  lua_rawset(L,-3);
	}
	lua_setfield(L,-2,"unicode");
  }

  if (enc->char_cnt && enc->psnames != NULL) {
	lua_createtable(L,enc->char_cnt,1);
	for (i=0;i<enc->char_cnt;i++) {
	  lua_pushnumber(L,i);
	  lua_pushstring(L,enc->psnames[i]);
	  lua_rawset(L,-3);
	}
	lua_setfield(L,-2,"psnames");
  }
  dump_intfield  (L,"builtin",            enc->builtin         );
  dump_intfield  (L,"hidden",             enc->hidden         );
  dump_intfield  (L,"only_1byte",         enc->only_1byte     );
  dump_intfield  (L,"has_1byte",          enc->has_1byte      );
  dump_intfield  (L,"has_2byte",          enc->has_2byte      );
  dump_intfield  (L,"is_unicodebmp",      enc->is_unicodebmp  );
  dump_intfield  (L,"is_unicodefull",     enc->is_unicodefull );
  dump_intfield  (L,"is_custom",          enc->is_custom      );
  dump_intfield  (L,"is_original",        enc->is_original      );  
  dump_intfield  (L,"is_compact",         enc->is_compact     );
  dump_intfield  (L,"is_japanese",        enc->is_japanese      );  
  dump_intfield  (L,"is_korean",          enc->is_korean      );
  dump_intfield  (L,"is_tradchinese",     enc->is_tradchinese );
  dump_intfield  (L,"is_simplechinese",   enc->is_simplechinese);

  if (enc->iso_2022_escape_len > 0) {
	dump_lstringfield (L,"iso_2022_escape", enc->iso_2022_escape, enc->iso_2022_escape_len);
  }
  dump_intfield (L,"low_page", enc->low_page);
  dump_intfield(L,"high_page", enc->high_page);

  dump_stringfield(L,"iconv_name", enc->iconv_name);

  /* TH: iconv internal information, ignorable */
  /* iconv_t *tounicode; */
  /* iconv_t *fromunicode; */
  /* int (*tounicode_func)(int); */
  /* int (*fromunicode_func)(int); */

  dump_intfield  (L,"is_temporary", enc->is_temporary);
  dump_intfield  (L,"char_max", enc->char_max);

}
      
void 
handle_enc (lua_State *L, struct enc *enc) {
  struct enc *next;
  NESTED_TABLE(do_handle_enc,enc,24);
}

void 
handle_encmap (lua_State *L, struct encmap *map) {
  int i;
  dump_intfield(L,"enccount", map->enccount) ;
  dump_intfield(L,"encmax",   map->encmax) ;
  dump_intfield(L,"backmax",  map->backmax) ;
  dump_intfield(L,"ticked",   map->ticked) ;
  if (map->remap != NULL) {
    lua_newtable(L);
    dump_intfield(L,"firstenc", map->remap->firstenc) ;
    dump_intfield(L,"lastenc",  map->remap->lastenc) ;
    dump_intfield(L,"infont",   map->remap->infont) ;
    lua_setfield(L,-2,"remap");
  }
  if (map->encmax > 0 && map->map != NULL) {
    lua_createtable(L,map->encmax,1);
    for (i=0;i<map->encmax;i++) {
      if (map->map[i]!=-1) {
		lua_pushnumber(L,i);
		lua_pushnumber(L,map->map[i]);
		lua_rawset(L,-3);
      }
    }
    lua_setfield(L,-2,"map");
  }

  if (map->backmax > 0 && map->backmap != NULL) {
    lua_newtable(L);
    for (i=0;i<map->backmax;i++) {
      if (map->backmap[i]!=-1) {
		lua_pushnumber(L,i);
		lua_pushnumber(L,map->backmap[i]);
		lua_rawset(L,-3);
      }
    }
    lua_setfield(L,-2,"backmap");
  }

  if (map->enc != NULL) {
    lua_newtable(L);
    handle_enc(L,map->enc);
    lua_setfield(L,-2,"enc");
  }
}

static void
handle_psdict (lua_State *L, struct psdict *private) {
  int k;
  if (private->keys != NULL && private->values != NULL) {
	for (k=0;k<private->next;k++) {
	  lua_pushstring(L,private->keys[k]);
	  lua_pushstring(L,private->values[k]);
	  lua_rawset(L,-3);
	}
  }
}

char *ttfnames_enum[ttf_namemax] = { "copyright", "family", "subfamily", "uniqueid",
    "fullname", "version", "postscriptname", "trademark",
    "manufacturer", "designer", "descriptor", "venderurl",
    "designerurl", "license", "licenseurl", "idontknow",
    "preffamilyname", "prefmodifiers", "compatfull", "sampletext",
    "cidfindfontname"};


void 
do_handle_ttflangname (lua_State *L, struct ttflangname *names) {
  int k;
  dump_intfield(L,"lang", names->lang) ;
  lua_createtable(L,0,ttf_namemax);
  for (k=0;k<ttf_namemax;k++) {
	lua_pushstring(L,ttfnames_enum[k]);
	lua_pushstring(L,names->names[k]);
	lua_rawset(L,-3);
  }
  lua_setfield(L, -2 , "names");
}


void 
handle_ttflangname (lua_State *L, struct ttflangname *names) {
  struct ttflangname *next;
  NESTED_TABLE(do_handle_ttflangname,names,2);
}


void
do_handle_anchorclass (lua_State *L, struct anchorclass *anchor) {

  dump_stringfield(L,"name",           anchor->name);
  dump_tag(L,"feature_tag",            anchor->feature_tag);
  dump_intfield(L,"script_lang_index", (anchor->script_lang_index+1));
  dump_intfield(L,"flags",             anchor->flags);
  dump_intfield(L,"merge_with",        anchor->merge_with);
  dump_intfield(L,"type",              anchor->type);
  dump_intfield(L,"processed",         anchor->processed);
  dump_intfield(L,"has_mark",          anchor->has_mark);
  dump_intfield(L,"matches",           anchor->matches);
  dump_intfield(L,"ac_num",            anchor->ac_num);

}

void
handle_anchorclass (lua_State *L, struct anchorclass *anchor) {
  struct anchorclass *next;
  NESTED_TABLE(do_handle_anchorclass,anchor,10);
}

void
do_handle_table_ordering (lua_State *L, struct table_ordering *orders) {
  int i;

  dump_tag(L,"table_tag",          orders->table_tag);
  lua_newtable(L);
  for ( i=0; orders->ordered_features[i]!=0; ++i ) {
    lua_pushnumber(L,(i+1));
    lua_pushstring(L,make_tag_string(orders->ordered_features[i]));
    lua_rawset(L,-3);
  }
  lua_setfield(L,-2,"ordered_features");
}

void
handle_table_ordering (lua_State *L, struct table_ordering *orders) {
  struct table_ordering *next;
  NESTED_TABLE(do_handle_table_ordering,orders,2);
}

void
do_handle_ttf_table  (lua_State *L, struct ttf_table *ttf_tab) {

  dump_tag(L,"tag",               ttf_tab->tag);
  dump_intfield(L,"len",          ttf_tab->len);
  dump_intfield(L,"maxlen",       ttf_tab->maxlen);
  dump_lstringfield(L,"data", (char *)ttf_tab->data, ttf_tab->len);

}

void
handle_ttf_table  (lua_State *L, struct ttf_table *ttf_tab) {
  struct ttf_table *next;
  NESTED_TABLE(do_handle_ttf_table,ttf_tab,4);
}

void
handle_script_record (lua_State *L, struct script_record *sl) {
  int k;
  if (sl->script != 0) {
	dump_tag(L,"script",          sl->script);
	if (sl->langs != NULL) {
	  k = 0;
	  lua_newtable(L);
	  while (sl->langs[k] != 0) {
		lua_pushnumber(L,(k+1));
		lua_pushstring(L,make_tag_string(sl->langs[k]));
		lua_rawset(L,-3);
		k++;
	  }
	  lua_setfield(L,-2,"langs");
	}
  }
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
void
handle_devicetab (lua_State *L, struct devicetable *adjusts) {
  int i,k;
  k = adjusts->last_pixel_size -  adjusts->first_pixel_size;  
  dump_intfield(L,"first_pixel_size",  adjusts->first_pixel_size);
  dump_intfield(L,"last_pixel_size",   adjusts->last_pixel_size);
  lua_newtable(L);
  for (i= 0; i<k; i++) {
    lua_pushnumber(L,(i+1));
    lua_pushnumber(L,adjusts->corrections[i]);
    lua_rawset(L,-3)
  }
  lua_setfield(L,-2,"corrections");
}
#endif

void
do_handle_kernclass (lua_State *L, struct kernclass *kerns) {
  int k;
  
  dump_intfield(L,"first_cnt",       kerns->first_cnt);
  dump_intfield(L,"second_cnt",      kerns->second_cnt);

  lua_createtable(L,kerns->first_cnt,1);
  for (k=0;k<kerns->first_cnt;k++) {
    lua_pushnumber(L,(k+1));
	lua_pushstring(L,kerns->firsts[k]);
	lua_rawset(L,-3);
  }
  lua_setfield(L,-2,"firsts");

  lua_createtable(L,kerns->second_cnt,1);
  for (k=0;k<kerns->second_cnt;k++) {
    lua_pushnumber(L,(k+1));
	lua_pushstring(L,kerns->seconds[k]);
	lua_rawset(L,-3);
  }
  lua_setfield(L,-2,"seconds");

  dump_intfield(L,"sli",             kerns->sli);
  dump_intfield(L,"flags",           kerns->flags);
  dump_intfield(L,"kcid",            kerns->kcid); /* probably not needed */

  lua_createtable(L,kerns->second_cnt*kerns->first_cnt,1);
  for (k=0;k<(kerns->second_cnt*kerns->first_cnt);k++) {
    lua_pushnumber(L,(k+1));
    lua_pushnumber(L,kerns->offsets[k]);
    lua_rawset(L,-3);
  }
  lua_setfield(L,-2,"offsets");

#ifdef FONTFORGE_CONFIG_DEVICETABLES
  lua_newtable(L);
  for (k=0;k<(kerns->second_cnt*kerns->first_cnt);k++) {
    lua_pushnumber(L,(k+1));
    lua_newtable(L);
    handle_devicetab(L,kerns->adjusts[k]);
    lua_rawset(L,-3);
  }
  lua_setfield(L,-2,"adjusts");
#endif
    
}

void
handle_kernclass (lua_State *L, struct kernclass *kerns) {
  struct kernclass *next;
  NESTED_TABLE(do_handle_kernclass,kerns,8);
}


#define DUMP_NUMBER_ARRAY(s,cnt,item) {				\
    if (cnt>0) {						\
      int kk;							\
      lua_newtable(L);						\
      for (kk=0;kk<cnt;kk++) {					\
	lua_pushnumber(L,(kk+1));				\
	lua_pushnumber(L,item[kk]);				\
	lua_rawset(L,-3); }					\
      lua_setfield(L,-2,s); } }


#define DUMP_STRING_ARRAY(s,cnt,item) {				\
    if (cnt>0) {						\
      int kk;							\
      lua_newtable(L);						\
      for (kk=0;kk<cnt;kk++) {					\
	lua_pushnumber(L,(kk+1));				\
	lua_pushstring(L,item[kk]);				\
	lua_rawset(L,-3); }					\
      lua_setfield(L,-2,s); } }



void handle_fpst_rule (lua_State *L, struct fpst_rule *rule, int format) {
  int k;


  if (format == pst_glyphs) {

    lua_newtable(L);
    dump_stringfield(L,"names",rule->u.glyph.names);
    dump_stringfield(L,"back",rule->u.glyph.back);
    dump_stringfield(L,"fore",rule->u.glyph.fore);
    lua_setfield(L,-2,"glyph");

  } else if (format == pst_class) {
  
    lua_newtable(L);
    DUMP_NUMBER_ARRAY("nclasses", rule->u.class.ncnt,rule->u.class.nclasses);
    DUMP_NUMBER_ARRAY("bclasses", rule->u.class.bcnt,rule->u.class.bclasses);
    DUMP_NUMBER_ARRAY("fclasses", rule->u.class.fcnt,rule->u.class.fclasses);
#if 0
    DUMP_NUMBER_ARRAY("allclasses", 0,rule->u.class.allclasses);
#endif
    lua_setfield(L,-2,"class");

  } else if (format == pst_coverage) {

    lua_newtable(L);
    DUMP_STRING_ARRAY("ncovers", rule->u.coverage.ncnt,rule->u.coverage.ncovers);
    DUMP_STRING_ARRAY("bcovers", rule->u.coverage.bcnt,rule->u.coverage.bcovers);
    DUMP_STRING_ARRAY("fcovers", rule->u.coverage.fcnt,rule->u.coverage.fcovers);
    lua_setfield(L,-2,"coverage");

  } else if (format == pst_reversecoverage) {

    lua_newtable(L);
    DUMP_STRING_ARRAY("ncovers", rule->u.rcoverage.always1,rule->u.rcoverage.ncovers);
    DUMP_STRING_ARRAY("bcovers", rule->u.rcoverage.bcnt,rule->u.rcoverage.bcovers);
    DUMP_STRING_ARRAY("fcovers", rule->u.rcoverage.fcnt,rule->u.rcoverage.fcovers);
    dump_stringfield(L,"replacements", rule->u.rcoverage.replacements);
    lua_setfield(L,-2,"rcoverage");
  }
  
  if (rule->lookup_cnt>0) {
    lua_newtable(L);
    for (k=0;k<rule->lookup_cnt;k++) {
      lua_pushnumber(L,(k+1));
      lua_newtable(L);
      dump_intfield(L,"seq",rule->lookups[k].seq);
      dump_tag(L,"lookup_tag",rule->lookups[k].lookup_tag);
      lua_rawset(L,-3);
    }
    lua_setfield(L,-2,"lookups");
  }
}

static char *fpossub_format_enum [] = { "glyphs", "class","coverage","reversecoverage"};


void 
do_handle_generic_fpst(lua_State *L, struct generic_fpst *fpst) {

  dump_intfield (L,"type", fpst->type);
  dump_enumfield(L,"format", fpst->format, fpossub_format_enum);
  dump_intfield (L,"script_lang_index", (fpst->script_lang_index+1));
  dump_intfield (L,"flags", fpst->flags);
  dump_tag(L,"tag",fpst->tag);

  dump_intfield (L,"nccnt", fpst->nccnt);
  dump_intfield (L,"bccnt", fpst->bccnt);
  dump_intfield (L,"fccnt", fpst->fccnt);
  dump_intfield (L,"rule_cnt", fpst->rule_cnt);

  DUMP_STRING_ARRAY("nclass",fpst->nccnt,fpst->nclass);
  DUMP_STRING_ARRAY("bclass",fpst->nccnt,fpst->bclass);
  DUMP_STRING_ARRAY("fclass",fpst->nccnt,fpst->fclass);

  if (fpst->rule_cnt>0) {
    lua_createtable(L,fpst->rule_cnt,1);
    for (k=0;k<fpst->rule_cnt;k++) {
      lua_pushnumber(L,(k+1));
      lua_newtable(L);
      handle_fpst_rule(L,&(fpst->rules[k]),fpst->format);
      lua_rawset(L,-3);
    }
    lua_setfield(L,-2,"rules");
  }
  dump_intfield (L,"ticked", fpst->ticked);
}

void 
handle_generic_fpst(lua_State *L, struct generic_fpst *fpst) {
  struct generic_fpst *next;
  NESTED_TABLE(do_handle_generic_fpst,fpst,14);
}


void
do_handle_otfname (lua_State *L, struct otfname *oname) {
  dump_intfield(L,"lang",        oname->lang);
  dump_stringfield(L,"name",     oname->name);
}

void
handle_otfname (lua_State *L, struct otfname *oname) {
  struct otfname *next;
  NESTED_TABLE(do_handle_otfname,oname,2);
}

void 
do_handle_macname (lua_State *L, struct macname *featname) {
  dump_intfield(L,"enc",         featname->enc);
  dump_intfield(L,"lang",        featname->lang);
  dump_stringfield(L,"name",     featname->name);
}

void 
handle_macname (lua_State *L, struct macname *featname) {
  struct macname *next;
  NESTED_TABLE(do_handle_macname,featname,3);
}

void 
do_handle_macsetting (lua_State *L, struct macsetting *settings) {
  dump_intfield(L,"setting",            settings->setting);
  dump_intfield(L,"strid",              settings->strid);
  dump_intfield(L,"initially_enabled",  settings->initially_enabled);
  if (settings->setname != NULL) {
    lua_newtable(L);
    handle_macname(L,settings->setname);
    lua_setfield(L,-2,"setname");
  }
}

void 
handle_macsetting (lua_State *L, struct macsetting *settings) {
  struct macsetting *next;
  NESTED_TABLE(do_handle_macsetting,settings,4);
}


void 
do_handle_macfeat (lua_State *L, struct macfeat *features) {

  dump_intfield(L,"feature",         features->feature);
  dump_intfield(L,"ismutex",         features->ismutex);
  dump_intfield(L,"default_setting", features->default_setting);
  dump_intfield(L,"strid",           features->strid);

  if (features->featname != NULL) {
    lua_newtable(L);
    handle_macname(L,features->featname);
    lua_setfield(L,-2,"featname");
  }

  if (features->settings != NULL) {
    lua_newtable(L);
    handle_macsetting(L,features->settings);
    lua_setfield(L,-2,"settings");
  }
}

void 
handle_macfeat (lua_State *L, struct macfeat *features) {
  struct macfeat *next;
  NESTED_TABLE(do_handle_macfeat,features,6);
}

void free_splinefont(struct splinefont *sf) ; /* forward */

char *tex_type_enum[4] = { "unset", "text", "math", "mathext"};

/* has an offset of 1, ui_none = 0. */
char *uni_interp_enum[9] = {
  "unset", "none", "adobe", "greek", "japanese",
  "trad_chinese", "simp_chinese", "korean", "ams" };
	
void
handle_splinefont_info(lua_State *L, struct splinefont *sf) {
  dump_stringfield(L,"fontname",        sf->fontname);
  dump_stringfield(L,"fullname",        sf->fullname);
  dump_stringfield(L,"familyname",      sf->familyname);
  dump_stringfield(L,"weight",          sf->weight);
}


void
handle_splinefont(lua_State *L, struct splinefont *sf) {
  int k;

  handle_splinefont_info (L,sf);

  dump_stringfield(L,"copyright",       sf->copyright);
  dump_stringfield(L,"filename",        sf->filename);
  dump_stringfield(L,"defbasefilename", sf->defbasefilename);
  dump_stringfield(L,"version",         sf->version);
	  
  dump_floatfield(L,"italicangle",      sf->italicangle);
  dump_floatfield(L,"upos",             sf->upos);
  dump_floatfield(L,"uwidth",           sf->uwidth);
  
  dump_intfield(L,"ascent",             sf->ascent);
  dump_intfield(L,"descent",            sf->descent);
  dump_intfield(L,"vertical_origin",    sf->vertical_origin);
  dump_intfield(L,"uniqueid",           sf->uniqueid);
  dump_intfield(L,"glyphcnt",           sf->glyphcnt);
  dump_intfield(L,"glyphmax",           sf->glyphmax);

  lua_createtable(L,sf->glyphcnt,0);
  for (k=0;k<sf->glyphcnt;k++) {
    lua_pushnumber(L,k);
    lua_createtable(L,0,12);
    handle_splinechar(L,sf->glyphs[k]);
    lua_rawset(L,-3);
  }
  lua_setfield(L,-2,"glyphs");

  dump_intfield(L,"changed",                   sf->changed);
  dump_intfield(L,"hasvmetrics",               sf->hasvmetrics);
  dump_intfield(L,"order2",                    sf->order2);
  dump_intfield(L,"strokedfont",               sf->strokedfont);
  dump_intfield(L,"weight_width_slope_only",   sf->weight_width_slope_only);
  dump_intfield(L,"head_optimized_for_cleartype",sf->head_optimized_for_cleartype);

  /* struct fontview *fv; */ /* TH: looks like this is unused */
  
  dump_enumfield(L,"uni_interp",  (sf->uni_interp+1), uni_interp_enum);
  
  /* NameList *for_new_glyphs; */ /* TH: looks like this is unused */

  if (sf->map != NULL ) {
    lua_newtable(L);
    handle_encmap(L,sf->map);
    lua_setfield(L,-2,"map");
  }
  
  /* Layer grid; */ /* TH: unused */

  if (sf->private != NULL) {
    lua_newtable(L);
    handle_psdict(L, sf->private);
    lua_setfield(L,-2,"private");
  }
  
  dump_stringfield(L,"xuid",    sf->xuid);
  
  lua_createtable(L,0,40);
  handle_pfminfo(L,sf->pfminfo);
  lua_setfield(L,-2,"pfminfo");
  
  if (sf->names != NULL) {
    lua_newtable(L);
    handle_ttflangname(L,sf->names);
    lua_setfield(L,-2,"names");
  }
  
  lua_createtable(L,0,4);
  dump_stringfield(L,"registry",    sf->cidregistry);
  dump_stringfield(L,"ordering",    sf->ordering);
  dump_intfield   (L,"version",     sf->cidversion);
  dump_intfield   (L,"supplement",  sf->supplement);
  lua_setfield(L,-2,"cidinfo");
  
  if (sf->subfontcnt>0) {
    lua_createtable(L,sf->subfontcnt,0);
    for (k=0;k<sf->subfontcnt;k++) {
      lua_pushnumber(L,(k+1));
      handle_splinefont(L,sf->subfonts[k]);
      lua_rawset(L,-3);
    }
    lua_setfield(L,-2,"subfonts");
  }
  
  if (sf->cidmaster != NULL) {
    lua_newtable(L);
    handle_splinefont(L, sf->cidmaster);
    lua_setfield(L,-2,"cidmaster");
  }
  
  dump_stringfield(L,"comments",    sf->comments);
  
  if (sf->anchor != NULL) {
    lua_newtable(L);
    handle_anchorclass(L,sf->anchor);
    lua_setfield(L,-2,"anchor");
  }

  /* struct glyphnamehash *glyphnames; */ /* TH unused */

  if (sf->orders != NULL) {
    lua_newtable(L);
    handle_table_ordering(L,sf->orders);
    lua_setfield(L,-2,"orders");
  }

  if (sf->ttf_tables != NULL) {
    lua_newtable(L);
    handle_ttf_table(L,sf->ttf_tables);
    lua_setfield(L,-2,"ttf_tables");
  }

  /* TH this is most likely empty, appears to be filled by the tottf writer */
  if (sf->ttf_tab_saved != NULL) {
    lua_newtable(L);
    handle_ttf_table(L,sf->ttf_tab_saved);
    lua_setfield(L,-2,"ttf_tab_saved");
  }
	
  /* struct instrdata *instr_dlgs;  */ /* TH unused. */
  /* struct shortview *cvt_dlg; */ /* TH unused, this is a window object */

  /* TH the object pointed to is a array of script_records, with last one NULL */
  if (sf->script_lang != NULL) {
    k = 0;
    lua_newtable(L);
    while (sf->script_lang[k] != NULL) {
      lua_pushnumber(L,(k+1));
      lua_newtable(L);
      handle_script_record(L,sf->script_lang[k]);
      lua_rawset(L,-3);
      k++;
    }
    lua_setfield(L,-2,"script_lang");
  }

  if (sf->kerns != NULL) { /* TH: strange, empty? */
    lua_newtable(L);
    handle_kernclass(L,sf->kerns);
    lua_setfield(L,-2,"kerns");
  }
  if (sf->vkerns != NULL) {
    lua_newtable(L);
    handle_kernclass(L,sf->vkerns);
    lua_setfield(L,-2,"vkerns");
  }
  
  /* struct kernclasslistdlg *kcld; */ /* TH unused, this is a window object */
  /* struct kernclasslistdlg *vkcld; */ /* TH unused, this is a window object */

  if (sf->texdata.type != tex_unset) {
    lua_newtable(L);
    dump_enumfield(L,"type",  sf->texdata.type, tex_type_enum);
    lua_newtable(L);
    for (k=0;k<22;k++) {
      lua_pushnumber(L,k);
      lua_pushnumber(L,sf->texdata.params[k]);
      lua_rawset(L,-3);
    }
    lua_setfield(L,-2,"params");
    lua_setfield(L,-2,"texdata");
  }

  lua_newtable(L);
  dump_intfield(L,"tt_cur", sf->gentags.tt_cur);
  dump_intfield(L,"tt_max", sf->gentags.tt_max);
  if (sf->gentags.tagtype != NULL) {
    lua_createtable(L,sf->gentags.tt_max,0);
    for (k=0;k<sf->gentags.tt_max;k++) {
      lua_pushnumber(L,(k+1));
      lua_newtable(L);
      dump_enumfield(L,"type", sf->gentags.tagtype[k].type, possub_type_enum);
      dump_tag(L,"tag", sf->gentags.tagtype[k].tag);
      lua_rawset(L,-3);
    }
    lua_setfield(L,-2,"tagtype");
  }
  lua_setfield(L,-2,"gentags");

  if (sf->possub != NULL) {
    lua_newtable(L);
    handle_generic_fpst(L,sf->possub);
    lua_setfield(L,-2,"possub");
  }

  /* ASM *sm;	*/ /* TH: TODO (AAT-related)*/
 
  /*
  if (sf->features != NULL) {
    lua_newtable(L);
    handle_macfeat(L,sf->features);
    lua_setfield(L,-2,"features");
  }
  */
  dump_stringfield(L,"chosenname",    sf->chosenname);
  
  /* struct mmset *mm;*/ /* TH: TODO (Adobe MM) */
  
  dump_intfield(L,"macstyle",    sf->macstyle);
  dump_intfield(L,"sli_cnt",     sf->sli_cnt);
  dump_stringfield(L,"fondname",    sf->fondname);
  
  dump_intfield(L,"design_size",     sf->design_size);
  dump_intfield(L,"fontstyle_id",     sf->fontstyle_id);
  
  if (sf->fontstyle_name != NULL) {
    lua_newtable(L);
    handle_otfname(L,sf->fontstyle_name);
    lua_setfield(L,-2,"fontstyle_name");
  }
   
  dump_intfield(L,"design_range_bottom",sf->design_range_bottom);
  dump_intfield(L,"design_range_top",   sf->design_range_top);
  dump_floatfield(L,"strokewidth",      sf->strokewidth);
  dump_intfield(L,"mark_class_cnt",     sf->mark_class_cnt);
  
  if (sf->mark_class_cnt>0) {
    lua_newtable(L);
    for ( k=0; k<sf->mark_class_cnt; ++k ) {
      lua_pushnumber(L,(k+1));
      lua_pushstring(L,sf->mark_classes[k]);
      lua_rawset(L,-3);
    }
    lua_setfield(L,-1,"mark_classes");

    lua_newtable(L);
    for ( k=0; k<sf->mark_class_cnt; ++k ) {
      lua_pushnumber(L,(k+1));
      lua_pushstring(L,sf->mark_class_names[k]);
      lua_rawset(L,-3);
    }
    lua_setfield(L,-1,"mark_class_names");
  }
  
  dump_intfield(L,"creationtime",     sf->creationtime);
  dump_intfield(L,"modificationtime", sf->modificationtime);

  dump_intfield(L,"os2_version",      sf->os2_version);
  dump_intfield(L,"gasp_version",     sf->gasp_version);
  dump_intfield(L,"gasp_cnt",         sf->gasp_cnt);
  
  if (sf->gasp!= NULL) {
    lua_newtable(L);
    dump_intfield(L,"ppem",       sf->gasp->ppem);
    dump_intfield(L,"flags",      sf->gasp->flags);
    lua_setfield(L,-1,"gasp");
  }

}

int 
make_ttf_table (lua_State *L, char *filename, scaled atsize, char *tt_type, int info_only) {
  FILE *ttf;
  SplineFont *sf;

  ttf = fopen(filename,"rb");
  if ( ttf == NULL ) {
	lua_pushboolean(L,0);
  }  else {
    sf = _SFReadTTF(ttf,0,0,filename,0,tt_type);
    fclose(ttf);
    if (sf!=NULL) {
      if(info_only) {
		lua_createtable(L,0,4);
		handle_splinefont_info(L,sf);
		free_splinefont(sf);
      } else {
		lua_createtable(L,0,60);
		handle_splinefont(L,sf);
		free_splinefont(sf);
      }

    } else {
      lua_pushboolean(L,0);
    }
  }
  return 1;
}


#define save_free(what) { if (what != NULL) {  free (what);  what = NULL;  } }

#define free_pfminfo(a) save_free(a)
#define free_otfname OtfNameListFree
#define free_ttflangname TTFLangNamesFree
#define free_ttf_table TtfTablesFree
#define free_anchorclass AnchorClassesFree
#define free_table_ordering TableOrdersFree
#define free_asm ASMFree
#define free_macfeat MacFeatListFree
#define free_encmap EncMapFree
#define free_psdict PSDictFree
#define free_generic_fpst FPSTFree
#define free_kernclass KernClassListFree
#define free_scriptrecordlist ScriptRecordListFree
#define free_splinechar  SplineCharFree


void SplineCharFreeContents(SplineChar *sc) {
    int i;
    if ( sc==NULL )
      return;
    free(sc->name);
    free(sc->comment);
    for ( i=0; i<sc->layer_cnt; ++i ) {
	SplinePointListsFree(sc->layers[i].splines);
	RefCharsFree(sc->layers[i].refs);
	/* ImageListsFree(sc->layers[i].images); */
	/* image garbage collection????!!!! */
	/* UndoesFree(sc->layers[i].undoes); */
	/* UndoesFree(sc->layers[i].redoes); */
    }
    /* StemInfosFree(sc->hstem); */
    /* StemInfosFree(sc->vstem); */
    DStemInfosFree(sc->dstem);
    MinimumDistancesFree(sc->md);
    KernPairsFree(sc->kerns);
    KernPairsFree(sc->vkerns);
    AnchorPointsFree(sc->anchor);
    SplineCharListsFree(sc->dependents);
    PSTFree(sc->possub);
    free(sc->ttf_instrs);
    free(sc->countermasks);
#ifdef FONTFORGE_CONFIG_TYPE3
    free(sc->layers);
#endif
    AltUniFree(sc->altuni);
}


void SplineCharFree(SplineChar *sc) {
    if ( sc==NULL )
      return;
    SplineCharFreeContents(sc);
    chunkfree(sc,sizeof(SplineChar));
}


void KernClassListFree(KernClass *kc) {
    int i;
    KernClass *n;

    while ( kc ) {
	for ( i=1; i<kc->first_cnt; ++i )
	    free(kc->firsts[i]);
	for ( i=1; i<kc->second_cnt; ++i )
	    free(kc->seconds[i]);
	free(kc->firsts);
	free(kc->seconds);
	free(kc->offsets);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	for ( i=kc->first_cnt*kc->second_cnt-1; i>=0 ; --i )
	    free(kc->adjusts[i].corrections);
	free(kc->adjusts);
#endif
	n = kc->next;
	chunkfree(kc,sizeof(KernClass));
	kc = n;
    }
}


void PSDictFree(struct psdict *dict) {
    int i;

    if ( dict==NULL )
      return;
    for ( i=0; i<dict->next; ++i ) {
        if ( dict->keys!=NULL ) free(dict->keys[i]);
        free(dict->values[i]);
    }
    free(dict->keys);
    free(dict->values);
    free(dict);
}

static void EncodingFree(Encoding *enc) {
    int i;

    if ( enc==NULL )
      return;
    free(enc->enc_name);
    free(enc->unicode);
    if ( enc->psnames!=NULL ) {
	for ( i=0; i<enc->char_cnt; ++i )
	    free(enc->psnames[i]);
	free(enc->psnames);
    }
    free(enc);
}

void EncMapFree(EncMap *map) {
    if ( map==NULL )
      return;

    if ( map->enc->is_temporary )
	EncodingFree(map->enc);
    free(map->map);
    free(map->backmap);
    free(map->remap);
    chunkfree(map,sizeof(EncMap));
}

void AnchorClassesFree(AnchorClass *an) {
    AnchorClass *anext;
    for ( ; an!=NULL; an = anext ) {
	anext = an->next;
	free(an->name);
	chunkfree(an,sizeof(AnchorClass));
    }
}

void TableOrdersFree(struct table_ordering *ord) {
    struct table_ordering *onext;
    for ( ; ord!=NULL; ord = onext ) {
	onext = ord->next;
	free(ord->ordered_features);
	chunkfree(ord,sizeof(struct table_ordering));
    }
}

void TtfTablesFree(struct ttf_table *tab) {
    struct ttf_table *next;

    for ( ; tab!=NULL; tab = next ) {
	next = tab->next;
	free(tab->data);
	chunkfree(tab,sizeof(struct ttf_table));
    }
}


void TTFLangNamesFree(struct ttflangname *l) {
    struct ttflangname *next;
    int i;

    while ( l!=NULL ) {
	next = l->next;
	for ( i=0; i<ttf_namemax; ++i )
	    free(l->names[i]);
	chunkfree(l,sizeof(*l));
	l = next;
    }
}

void OtfNameListFree(struct otfname *on) {
    struct otfname *on_next;

    for ( ; on!=NULL; on = on_next ) {
	on_next = on->next;
	free(on->name);
	chunkfree(on,sizeof(*on));
    }
}


void MacNameListFree(struct macname *mn) {
    struct macname *next;
    while ( mn!=NULL ) {
	next = mn->next;
	free(mn->name);
	chunkfree(mn,sizeof(struct macname));
	mn = next;
    }
}

void MacSettingListFree(struct macsetting *ms) {
    struct macsetting *next;
    while ( ms!=NULL ) {
	next = ms->next;
	MacNameListFree(ms->setname);
	chunkfree(ms,sizeof(struct macsetting));
	ms = next;
    }
}

void MacFeatListFree(MacFeat *mf) {
    MacFeat *next;
    while ( mf!=NULL ) {
	next = mf->next;
	MacNameListFree(mf->featname);
	MacSettingListFree(mf->settings);
	chunkfree(mf,sizeof(MacFeat));
	mf = next;
    }
}

void ASMFree(ASM *sm) {
    ASM *next;
    int i;

    while ( sm!=NULL ) {
	next = sm->next;
	if ( sm->type==asm_insert ) {
	    for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
		free( sm->state[i].u.insert.mark_ins );
		free( sm->state[i].u.insert.cur_ins );
	    }
	} else if ( sm->type==asm_kern ) {
	    for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
		free( sm->state[i].u.kern.kerns );
	    }
	}
	for ( i=4; i<sm->class_cnt; ++i )
	    free(sm->classes[i]);
	free(sm->state);
	free(sm->classes);
	chunkfree(sm,sizeof(ASM));
	sm = next;
    }
}


static void FPSTRuleContentsFree(struct fpst_rule *r, enum fpossub_format format) {
    int j;
	
    switch ( format ) {
	case pst_glyphs:
	  free(r->u.glyph.names);
	  free(r->u.glyph.back);
	  free(r->u.glyph.fore);
	  break;
	case pst_class:
	  free(r->u.class.nclasses);
	  free(r->u.class.bclasses);
	  free(r->u.class.fclasses);
      break;
	case pst_reversecoverage:
	  free(r->u.rcoverage.replacements);
	case pst_coverage:
	  for ( j=0 ; j<r->u.coverage.ncnt ; ++j )
		free(r->u.coverage.ncovers[j]);
	  free(r->u.coverage.ncovers);
	  for ( j=0 ; j<r->u.coverage.bcnt ; ++j )
		free(r->u.coverage.bcovers[j]);
	  free(r->u.coverage.bcovers);
	  for ( j=0 ; j<r->u.coverage.fcnt ; ++j )
		free(r->u.coverage.fcovers[j]);
	  free(r->u.coverage.fcovers);
	  break;
	case pst_formatmax:
	  break;
    }
    free(r->lookups);
}


void FPSTFree(FPST *fpst) {
    FPST *next;
    int i;

    while ( fpst!=NULL ) {
	next = fpst->next;
	for ( i=0; i<fpst->nccnt; ++i )
	    free(fpst->nclass[i]);
	for ( i=0; i<fpst->bccnt; ++i )
	    free(fpst->bclass[i]);
	for ( i=0; i<fpst->fccnt; ++i )
	    free(fpst->fclass[i]);
	free(fpst->nclass); free(fpst->bclass); free(fpst->fclass);
	for ( i=0; i<fpst->rule_cnt; ++i ) {
	    FPSTRuleContentsFree( &fpst->rules[i],fpst->format );
	}
	free(fpst->rules);
	chunkfree(fpst,sizeof(FPST));
	fpst = next;
    }
}

void DStemInfosFree(DStemInfo *h) {
    DStemInfo *hnext;

    for ( ; h!=NULL; h = hnext ) {
	hnext = h->next;
	chunkfree(h,sizeof(DStemInfo));
    }
}

void KernPairsFree(KernPair *kp) {
    KernPair *knext;
    for ( ; kp!=NULL; kp = knext ) {
	knext = kp->next;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( kp->adjust!=NULL ) {
	    free(kp->adjust->corrections);
	    chunkfree(kp->adjust,sizeof(DeviceTable));
	}
#endif
	chunkfree(kp,sizeof(KernPair));
    }
}

void AnchorPointsFree(AnchorPoint *ap) {
    AnchorPoint *anext;
    for ( ; ap!=NULL; ap = anext ) {
	anext = ap->next;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	free(ap->xadjust.corrections);
	free(ap->yadjust.corrections);
#endif
	chunkfree(ap,sizeof(AnchorPoint));
    }
}

void PSTFree(PST *pst) {
    PST *pnext;
    for ( ; pst!=NULL; pst = pnext ) {
	pnext = pst->next;
	if ( pst->type==pst_lcaret )
	    free(pst->u.lcaret.carets);
	else if ( pst->type==pst_pair ) {
	    free(pst->u.pair.paired);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    ValDevFree(pst->u.pair.vr[0].adjust);
	    ValDevFree(pst->u.pair.vr[1].adjust);
#endif
	    chunkfree(pst->u.pair.vr,sizeof(struct vr [2]));
	} else if ( pst->type!=pst_position ) {
	    free(pst->u.subs.variant);
	} else if ( pst->type==pst_position ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    ValDevFree(pst->u.pos.adjust);
#endif
	}
	chunkfree(pst,sizeof(PST));
    }
}

void MinimumDistancesFree(MinimumDistance *md) {
    MinimumDistance *next;

    while ( md!=NULL ) {
	next = md->next;
	chunkfree(md,sizeof(MinimumDistance));
	md = next;
    }
}

void AltUniFree(struct altuni *altuni) {
    struct altuni *next;

    while ( altuni ) {
	next = altuni->next;
	chunkfree(altuni,sizeof(struct altuni));
	altuni = next;
    }
}

void SplineCharListsFree(struct splinecharlist *dlist) {
    struct splinecharlist *dnext;
    for ( ; dlist!=NULL; dlist = dnext ) {
	dnext = dlist->next;
	chunkfree(dlist,sizeof(struct splinecharlist));
    }
}


void ScriptRecordFree(struct script_record *sr) {
    int i;

    for ( i=0; sr[i].script!=0; ++i )
	free( sr[i].langs );
    free( sr );
}

void ScriptRecordListFree(struct script_record **script_lang) {
    int i;

    if ( script_lang==NULL )
return;
    for ( i=0; script_lang[i]!=NULL; ++i )
	ScriptRecordFree(script_lang[i]);
    free( script_lang );
}



void
free_splinefont(struct splinefont *sf) {
  int k;

  /* struct mmset *mm;*/ /* TH: TODO (Adobe MM) */
  
  /* struct fontview *fv; */ /* TH: looks like this is unused */
  /* NameList *for_new_glyphs; */ /* TH: looks like this is unused */
  /* Layer grid; */ /* TH: unused */
  /* struct glyphnamehash *glyphnames; */ /* TH unused */
  /* struct instrdata *instr_dlgs;  */ /* TH unused. */
  /* struct shortview *cvt_dlg; */ /* TH unused, this is a window object */
  /* struct kernclasslistdlg *kcld; */ /* TH unused, this is a window object */
  /* struct kernclasslistdlg *vkcld; */ /* TH unused, this is a window object */

  for (k=0;k<sf->glyphcnt;k++) {    free_splinechar(sf->glyphs[k]);  }

  if (sf->sm != NULL)             {  free_asm(sf->sm); }
  if (sf->map != NULL )           {  free_encmap(sf->map);  }
  if (sf->private != NULL)        {  free_psdict( sf->private);  }
  if (sf->names != NULL)          {  free_ttflangname(sf->names);  }
  if (sf->anchor != NULL)         {  free_anchorclass(sf->anchor);  }
  if (sf->orders != NULL)         {  free_table_ordering(sf->orders);  }
  if (sf->ttf_tables != NULL)     {  free_ttf_table(sf->ttf_tables);  }
  if (sf->ttf_tab_saved != NULL)  {  free_ttf_table(sf->ttf_tab_saved);  }
  if (sf->kerns != NULL)          {  free_kernclass(sf->kerns);  }
  if (sf->vkerns != NULL)         {  free_kernclass(sf->vkerns);  }
  if (sf->possub != NULL)         {  free_generic_fpst(sf->possub);  }
  if (sf->features != NULL)       {  free_macfeat(sf->features);  }
  if (sf->fontstyle_name != NULL) {  free_otfname(sf->fontstyle_name);  }
     
  /* free(sf->pfminfo); */ /* not needed */
  
  if (sf->subfontcnt>0) {
    for (k=0;k<sf->subfontcnt;k++) {
      free_splinefont(sf->subfonts[k]);
    }
    save_free(sf->subfonts);
  }
    
  if (sf->script_lang != NULL) {
    free_scriptrecordlist(sf->script_lang);
  }


  if (sf->mark_class_cnt>0) {
    for ( k=0; k<sf->mark_class_cnt; ++k ) {
      save_free(sf->mark_classes[k]);
      save_free(sf->mark_class_names[k]);
    }
    save_free(sf->mark_classes );
    save_free(sf->mark_class_names );
  }
    
  /* simple structures */
  save_free(sf->gasp);
  save_free(sf->gentags.tagtype);
  save_free(sf->glyphs);

  /* string fields */
  save_free(sf->fontname);
  save_free(sf->fullname);
  save_free(sf->familyname);
  save_free(sf->weight);
  save_free(sf->copyright);
  save_free(sf->filename);
  save_free(sf->defbasefilename);
  save_free(sf->version);
  save_free(sf->origname);
  save_free(sf->autosavename);
  save_free(sf->xuid);
  save_free(sf->cidregistry);
  save_free(sf->ordering);
  save_free(sf->comments);
  save_free(sf->chosenname);
  save_free(sf->fondname);

  /* done */
  free(sf);
}
