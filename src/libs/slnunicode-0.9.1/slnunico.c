/*
*	Selene Unicode/UTF-8
*	This additions
*	Copyright (c) 2005 Malete Partner, Berlin, partner@malete.org
*	Available under "Lua 5.0 license", see http://www.lua.org/license.html#5
*	$Id: slnunico.c,v 1.6 2005/02/25 11:30:43 krip Exp $
*
*	contains code from 
** lstrlib.c,v 1.109 2004/12/01 15:46:06 roberto Exp
** Standard library for string operations and pattern-matching
** See Copyright Notice in lua.h
*
*	uses the udata table and a couple of expressions from Tcl 8.4.x UTF-8
* which comes with the following license.terms:

This software is copyrighted by the Regents of the University of
California, Sun Microsystems, Inc., Scriptics Corporation, ActiveState
Corporation and other parties.  The following terms apply to all files
associated with the software unless explicitly disclaimed in
individual files.

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors
and need not follow the licensing terms described here, provided that
the new terms are clearly indicated on the first page of each file where
they apply.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
MODIFICATIONS.

GOVERNMENT USE: If you are acquiring this software on behalf of the
U.S. government, the Government shall have only "Restricted Rights"
in the software and related documentation as defined in the Federal 
Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
are acquiring the software on behalf of the Department of Defense, the
software shall be classified as "Commercial Computer Software" and the
Government shall have only "Restricted Rights" as defined in Clause
252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
authors grant the U.S. Government and others acting in its behalf
permission to use and distribute the software in accordance with the
terms specified in this license. 

(end of Tcl license terms)
*/

/*
According to http://ietf.org/rfc/rfc3629.txt we support up to 4-byte
(21 bit) sequences encoding the UTF-16 reachable 0-0x10FFFF.
Any byte not part of a 2-4 byte sequence in that range decodes to itself.
Ill formed (non-shortest) "C0 80" will be decoded as two code points C0 and 80,
not code point 0; see security considerations in the RFC.
However, UTF-16 surrogates (D800-DFFF) are accepted.

See http://www.unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries
for default grapheme clusters.
Lazy westerners we are (and lacking the Hangul_Syllable_Type data),
we care for base char + Grapheme_Extend, but not for Hangul syllable sequences.

For http://unicode.org/Public/UNIDATA/UCD.html#Grapheme_Extend
we use Mn (NON_SPACING_MARK) + Me (ENCLOSING_MARK),
ignoring the 18 mostly south asian Other_Grapheme_Extend (16 Mc, 2 Cf) from
http://www.unicode.org/Public/UNIDATA/PropList.txt
*/

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lstrlib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#ifndef MAX_CAPTURES /* moved to luaconf.h in 5.1; but this is 5.0 */
#	define MAX_CAPTURES	32	/* arbitrary limit */
/* also new in 5.1 */
/*#	define lua_pushinteger(L,l)	 lua_pushnumber(L, (lua_Number)(l))
  #	define luaL_checkinteger(L,n) luaL_checkint(L, n)
  #	define luaL_optinteger(L,n,d) luaL_optint(L, n, d)
  # define lua_tointeger(L,n)		 ((int)lua_tonumber(L, n))*/
/* should use lua_number2int; but alas */
#endif

#include "slnudata.c"
#define charinfo(c) (~0xFFFF&(c) ? 0 : GetUniCharInfo(c)) /* BMP only */
#define charcat(c) (UNICODE_CATEGORY_MASK & charinfo(c))
#define Grapheme_Extend(code) \
	(1 & (((1<<NON_SPACING_MARK)|(1<<ENCLOSING_MARK)) >> charcat(code)))


enum { /* operation modes */
	MODE_ASCII, /* single byte 7bit */
	MODE_LATIN, /* single byte 8859-1 */
	MODE_UTF8,	/* UTF-8 by code points */
	MODE_GRAPH	/* UTF-8 by grapheme clusters */
#define MODE_MBYTE(mode) (~1&(mode))
};


/* macro to `unsign' a character */
#define uchar(c)				((unsigned char)(c))

typedef const unsigned char cuc; /* it's just toooo long :) */


static void utf8_enco (luaL_Buffer *b, unsigned c)
{
	if (0x80 > c) {
		luaL_putchar(b, c);
		return;
	}
	if (0x800 > c)
		luaL_putchar(b, 0xC0|(c>>6));
	else {
		if (0x10000 > c)
			luaL_putchar(b, 0xE0|(c>>12));
		else {
			luaL_putchar(b, 0xF0|(c>>18));
			luaL_putchar(b, 0x80|(0x3F&(c>>12)));
		}
		luaL_putchar(b, 0x80|(0x3F&(c>>6)));
	}
	luaL_putchar(b, 0x80|(0x3F&c));
}	/* utf8_enco */


/* end must be > *pp */
static unsigned utf8_deco (const char **pp, const char *end)
{
	register cuc *p = (cuc*)*pp, * const e = (cuc*)end;
	unsigned first = *p, code;

	*pp = (const char*)++p; /* eat one */
	/* check ASCII, dangling cont., non-shortest or not continued */
	if (0xC2 > first || e == p || 0x80 != (0xC0&*p)) return first;
	code = 0x3F&*p++; /* save 1st cont. */
	/* check 2 byte (5+6 = 11 bit) sequence up to 0x7FF */
	if (0xE0 > first) { /* any >= C2 is valid */
		code |= (0x1F&first)<<6;
		goto seq;
	}
	if (e != p && 0x80 == (0xC0&*p)) { /* is continued */
		code = code<<6 | (0x3F&*p++); /* save 2nd */
		if (0xF0 > first) { /* 3 byte (4+6+6 = 16 bit) seq -- want 2nd cont. */
			if ( 0xF800&(code |= (0x0F&first)<<12) /* >0x7FF: not non-shortest */
				/* && 0xD800 != (0xD800 & code) -- nah, let surrogates pass */
			)
				goto seq;
		} else if (e != p && 0x80 == (0xC0&*p) /* check 3rd */
			/* catch 0xF4 < first and other out-of-bounds */
			&& 0x110000 > (code = (0x0F&first)<<18 | code<<6 | (0x3F&*p++))
			&& 0xFFFF < code /* not a 16 bitty */
		)
			goto seq;
	}
	return first;
seq:
	*pp = (const char*)p;
	return code;
}	/* utf8_deco */


/* reverse decode before pp > start */
static unsigned utf8_oced (const char **pp, const char *start)
{
	register cuc *p = (cuc*)*pp, * const s = (cuc*)start;
	unsigned last = *--p, code;

	*pp = (const char*)p; /* eat one */
	/* check non-continuer or at the edge */
	if (0x80 != (0xC0&last) || s == p) return last;
	code = 0x3F&last; /* save last cont. */
	if (0xC0 == (0xE0&*--p)) { /* preceeded by 2 byte seq starter */
		if (0xC2 <= *p) { code |= (0x1F&*p)<<6; goto seq; }
	} else if (0x80 == (0xC0&*p) && s<p) {
		code |= (0x3F&*p)<<6;
		if (0xE0 == (0xF0&*--p)) { /* 3 byte starter */
			if (0xF800&(code |= (0x0F&*p)<<12)) goto seq;
		} else if (0x80 == (0xC0&*p) && s<=--p /* valid 4 byte ? */
			&& 0x110000 > (code |= (0x0F&*p)<<18 | (0x3F&p[1])<<12)
			&& 0xFFFF < code
		)
			goto seq;
	}
	return last;
seq:
	*pp = (const char*)p;
	return code;
}	/* utf8_oced */


/* skip over Grapheme_Extend codes */
static void utf8_graphext (const char **pp, const char *end)
{
	const char *p = *pp;
	for (; p < end; *pp=p) {
		unsigned code = utf8_deco(&p, end);
		if (!Grapheme_Extend(code)) break;
	}
}	/* utf8_graphext */


static int utf8_count (const char **pp, int bytes, int graph, int max)
{
	const char *const end = *pp+bytes;
	int count = 0;
	while (*pp < end && count != max) {
		unsigned code = utf8_deco(pp, end);
		count++;
		if (!graph) continue;
		if (Grapheme_Extend(code) && 1<count) count--; /* uncount */
	}
	if (graph && count == max) /* gather more extending */
		utf8_graphext(pp, end);
	return count;
}	/* utf8_count */



static int unic_len (lua_State *L) {
	size_t l;
	const char *s = luaL_checklstring(L, 1, &l);
	int mode = lua_tointeger(L, lua_upvalueindex(1));
	if (MODE_MBYTE(mode)) l = (size_t)utf8_count(&s, l, mode-2, -1);
	lua_pushinteger(L, l);
	return 1;
}


static ptrdiff_t posrelat (ptrdiff_t pos, size_t len) {
	/* relative string position: negative means back from end */
	return (pos>=0) ? pos : (ptrdiff_t)len+pos+1;
}


static int unic_sub (lua_State *L) {
	size_t l;
	const char *s = luaL_checklstring(L, 1, &l), *p, *e=s+l;
	ptrdiff_t start = luaL_checkinteger(L, 2);
	ptrdiff_t end = luaL_optinteger(L, 3, -1);
	int mode = lua_tointeger(L, lua_upvalueindex(1));

	if (MODE_MBYTE(mode)) { p=s; l = (size_t)utf8_count(&p, l, mode-2, -1); }
	start = posrelat(start, l);
	end = posrelat(end, l);
	if (start < 1) start = 1;
	if (end > (ptrdiff_t)l) end = (ptrdiff_t)l;
	if (start > end)
		lua_pushliteral(L, "");
	else {
		l = end - --start; /* #units */
		if (!(MODE_MBYTE(mode))) /* single byte */
			s += start;
		else {
			if (start) utf8_count(&s, e-s, mode-2, start); /* skip */
			p = s;
			utf8_count(&p, e-p, mode-2, l);
			l = p-s;
		}
		lua_pushlstring(L, s, l);
	}
	return 1;
}


static int str_reverse (lua_State *L) { /* TODO? whatfor? */
	size_t l;
	luaL_Buffer b;
	const char *s = luaL_checklstring(L, 1, &l), *p = s+l, *q;
	int mode = lua_tointeger(L, lua_upvalueindex(1)), mb = MODE_MBYTE(mode);

	luaL_buffinit(L, &b);
	if (!mb)
		while (s < p--) luaL_putchar(&b, *p);
	else {
		unsigned code;
		while (s < p) {
			q = p;
			code = utf8_oced(&p, s);
			if (MODE_GRAPH == mode)
				while (Grapheme_Extend(code) && p>s) code = utf8_oced(&p, s);
			luaL_addlstring(&b, p, q-p);
		}
	}
	luaL_pushresult(&b);
	return 1;
}



static int unic_lower (lua_State *L) {
	size_t l;
	luaL_Buffer b;
	const char *s = luaL_checklstring(L, 1, &l), * const e=s+l;
	int mode = lua_tointeger(L, lua_upvalueindex(1)), mb = MODE_MBYTE(mode);
	luaL_buffinit(L, &b);
	while (s < e) {
		unsigned c = mb ? utf8_deco(&s, e) : uchar(*s++);
		int info = charinfo(c);
		if (GetCaseType(info)&0x02 && (mode || !(0x80&c))) c += GetDelta(info);
		if (mb) utf8_enco(&b, c); else luaL_putchar(&b, c);
	}
	luaL_pushresult(&b);
	return 1;
}


static int unic_upper (lua_State *L) {
	size_t l;
	luaL_Buffer b;
	const char *s = luaL_checklstring(L, 1, &l), * const e=s+l;
	int mode = lua_tointeger(L, lua_upvalueindex(1)), mb = MODE_MBYTE(mode);
	luaL_buffinit(L, &b);
	while (s < e) {
		unsigned c = mb ? utf8_deco(&s, e) : uchar(*s++);
		int info = charinfo(c);
		if (GetCaseType(info)&0x04 && (mode || !(0x80&c))) c -= GetDelta(info);
		if (mb) utf8_enco(&b, c); else luaL_putchar(&b, c);
	}
	luaL_pushresult(&b);
	return 1;
}


static int str_rep (lua_State *L) {
	size_t l;
	luaL_Buffer b;
	const char *s = luaL_checklstring(L, 1, &l);
	int n = luaL_checkint(L, 2);
	luaL_buffinit(L, &b);
	while (n-- > 0)
		luaL_addlstring(&b, s, l);
	luaL_pushresult(&b);
	return 1;
}


static int unic_byte (lua_State *L) {
	size_t l;
	ptrdiff_t posi, pose;
	const char *s = luaL_checklstring(L, 1, &l), *p, *e=s+l;
	int n, mode = lua_tointeger(L, lua_upvalueindex(1)), mb = MODE_MBYTE(mode);

	if (mb) { p=s; l = (size_t)utf8_count(&p, l, mode-2, -1); }
	posi = posrelat(luaL_optinteger(L, 2, 1), l);
	pose = posrelat(luaL_optinteger(L, 3, posi), l);
	if (posi <= 0) posi = 1;
	if ((size_t)pose > l) pose = l;
	if (0 >= (n = pose - --posi)) return 0;	/* empty interval */
	if (!mb)
		e = (s += posi) + n;
	else {
		if (posi) utf8_count(&s, e-s, mode-2, posi); /* skip */
		p=s;
		utf8_count(&p, e-s, mode-2, n);
		e=p;
	}
	/* byte count is upper bound on #elements */
	luaL_checkstack(L, e-s, "string slice too long");
	for (n=0; s<e; n++)
		lua_pushinteger(L, mb ? utf8_deco(&s, e) : uchar(*s++));
	return n;
}


static int unic_char (lua_State *L) {
	int i, n = lua_gettop(L);	/* number of arguments */
	int mode = lua_tointeger(L, lua_upvalueindex(1)), mb = MODE_MBYTE(mode);
	unsigned lim = mb ? 0x110000 : 0x100;
 
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	for (i=1; i<=n; i++) {
		unsigned c = luaL_checkint(L, i);
		luaL_argcheck(L, lim > c, i, "invalid value");
		if (mb) utf8_enco(&b, c); else luaL_putchar(&b, c);
	}
	luaL_pushresult(&b);
	return 1;
}


static int writer (lua_State *L, const void* b, size_t size, void* B) {
	(void)L;
	luaL_addlstring((luaL_Buffer*) B, (const char *)b, size);
	return 0;
}


static int str_dump (lua_State *L) {
	luaL_Buffer b;
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_settop(L, 1);
	luaL_buffinit(L,&b);
	if (lua_dump(L, writer, &b) != 0)
		luaL_error(L, "unable to dump given function");
	luaL_pushresult(&b);
	return 1;
}



/*
** {======================================================
** PATTERN MATCHING
** =======================================================
* find/gfind(_aux) -> match, push_captures
* gsub -> match, add_s (-> push_captures)
* push_captures, add_s -> push_onecapture
* match ->
* 	start/end_capture -> match,
* 	match_capture, matchbalance, classend -> -,
* 	min/max_expand -> match, singlematch
* 	singlematch -> matchbracketclass, match_class,
* 	matchbracketclass -> match_class -> -,
*/


#define CAP_UNFINISHED	(-1)
#define CAP_POSITION	(-2)

typedef struct MatchState {
	const char *src_init;	/* init of source string */
	const char *src_end;	/* end (`\0') of source string */
	lua_State *L;
	int level;	/* total number of captures (finished or unfinished) */
	int mode;
	int mb;
	struct {
		const char *init;
		ptrdiff_t len;
	} capture[MAX_CAPTURES];
} MatchState;


#define ESC		'%'
#define SPECIALS	"^$*+?.([%-"


static int check_capture (MatchState *ms, int l) {
	l -= '1';
	if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
		return luaL_error(ms->L, "invalid capture index");
	return l;
}


static int capture_to_close (MatchState *ms)
{
	int level = ms->level;
	for (level--; level>=0; level--)
		if (ms->capture[level].len == CAP_UNFINISHED) return level;
	return luaL_error(ms->L, "invalid pattern capture");
}


static const char *classend (MatchState *ms, const char *p)
{
	switch (*p) {
	case ESC:
		if (!*++p) luaL_error(ms->L, "malformed pattern (ends with `%%')");
		break;
	case '[':
		/* if (*p == '^') p++; -- no effect */
		do {	/* look for a `]' */
			if (!*p) luaL_error(ms->L, "malformed pattern (missing `]')");
			if (ESC == *(p++) && *p) p++;	/* skip escapes (e.g. `%]') */
		} while (']' != *p);
		break;
	default:
		if (!ms->mb) break;
		utf8_deco(&p, p+4);
		return p;
	}
	return p+1;
}	/* classend */


/*
 * The following macros are used for fast character category tests.  The
 * x_BITS values are shifted right by the category value to determine whether
 * the given category is included in the set.
 */ 

#define LETTER_BITS ((1 << UPPERCASE_LETTER) | (1 << LOWERCASE_LETTER) \
    | (1 << TITLECASE_LETTER) | (1 << MODIFIER_LETTER) | (1 << OTHER_LETTER))

#define DIGIT_BITS (1 << DECIMAL_DIGIT_NUMBER)

#define NUMBER_BITS (1 << DECIMAL_DIGIT_NUMBER) \
	| (1 << LETTER_NUMBER) | (1 << OTHER_NUMBER)

#define SPACE_BITS ((1 << SPACE_SEPARATOR) | (1 << LINE_SEPARATOR) \
    | (1 << PARAGRAPH_SEPARATOR))

#define CONNECTOR_BITS (1 << CONNECTOR_PUNCTUATION)

#define PUNCT_BITS ((1 << CONNECTOR_PUNCTUATION) | \
	    (1 << DASH_PUNCTUATION) | (1 << OPEN_PUNCTUATION) | \
	    (1 << CLOSE_PUNCTUATION) | (1 << INITIAL_QUOTE_PUNCTUATION) | \
	    (1 << FINAL_QUOTE_PUNCTUATION) | (1 << OTHER_PUNCTUATION))


/* character c matches class cl. undefined for cl not ascii */
static int match_class (int c, int cl, int mode)
{
	int msk, res;

	switch (0x20|cl /*tolower*/) {
	case 'a' : msk = LETTER_BITS; break;
	case 'c' : msk = 1<<CONTROL; break;
	case 'x' : /* hexdigits */
		if (0x40==(~0x3f&c)/*64-127*/ && 1&(0x7e/*a-f*/>>(0x1f&c))) goto matched;
	case 'd' : msk = 1<<DECIMAL_DIGIT_NUMBER; mode=0;/* ASCII only */ break;
	case 'l' : msk = 1<<LOWERCASE_LETTER; break;
	case 'n' : msk = NUMBER_BITS; break; /* new */
	case 'p' : msk = PUNCT_BITS; break;
	case 's' :
#define STDSPACE /* standard "space" controls 9-13 */ \
		(1<<9/*TAB*/|1<<10/*LF*/|1<<11/*VT*/|1<<12/*FF*/|1<<13/*CR*/)
		if (!(~0x1f & c) && 1&(STDSPACE >> c)) goto matched;
		msk = SPACE_BITS;
		break;
	case 'u' : msk = 1<<UPPERCASE_LETTER; break;
	case 'w' : msk = LETTER_BITS|NUMBER_BITS|CONNECTOR_BITS; break;
	case 'z' : if (!c) goto matched; msk = 0; break;
	default: return cl == c;
	}
	res = 1 & (msk >> charcat(c));
	if (!mode && 0x80&c) res = 0;
	if (0) {
matched:
		res = 1;
	}
	return 0x20&cl /*islower*/ ? res : !res;
}	/* match_class */


/* decode single byte or UTF-8 seq; advance *s */
static unsigned deco (const MatchState *ms, const char **s, const char *e)
{
	return ms->mb ? utf8_deco(s, e) : *(unsigned char*)(*s)++;
}

/* s must be < ms->src_end, p < ep */
static const char *singlematch (const MatchState *ms,
	const char *s, const char *p, const char *ep)
{
	int neg = 0;
	unsigned c1, c2;
	unsigned c;
#ifdef OPTIMIZE_SIZE
	c = deco(ms, &s, ms->src_end);
#else
	if (!ms->mb || !(0x80&*s))
		c = *(unsigned char*)s++;
	else
		c = utf8_deco(&s, ms->src_end);
#endif

	switch (*p) {
	case ESC:
		if (match_class(c, uchar(p[1]), ms->mode)) {
	case '.': /* the all class */
#ifndef OPTIMIZE_SIZE
			if (MODE_GRAPH != ms->mode) return s; /* common fast path */
#endif
			goto matched_class;
		}
		s = 0;
		break;
	default:
#ifdef OPTIMIZE_SIZE
		c1 = deco(ms, &p, ep);
#else
		if (!ms->mb || !(0x80&*p))
			c1 = *(unsigned char*)p++;
		else
			c1 = utf8_deco(&p, ep);
#endif
		if (c != c1) s = 0;
		break;
	case '[': /* matchbracketclass */
		ep--; /* now on the ']' */
		if ((neg = '^' == *++p)) p++;	/* skip the `^' */
		while (p < ep) {
			if (*p == ESC) {
				if (match_class(c, uchar(*++p), ms->mode)) goto matched_class_in_brack;
				p++;
				continue;
			}
			c1 = deco(ms, &p, ep);
			if (ep <= p || '-' != *p) { /* not a range */
				const char *op = p, *es;
				if (MODE_GRAPH == ms->mode) utf8_graphext(&p, ep);
				if (c != c1) continue;
				if (MODE_GRAPH != ms->mode) goto matched;
				/* comp grapheme extension */
				es = s;
				utf8_graphext(&es, ms->src_end);
				if (es-s == p-op && (es==s || !memcmp(s, op, es-s))) goto matched;
				continue;

			}
			/* range c1-c2 -- no extend support in range bounds... */
			if (ep == ++p) break; /* bugger - trailing dash */
			c2 = deco(ms, &p, ep);
			if (c2 < c1) { unsigned swap=c1; c1=c2; c2=swap; }
			if (c1 <= c && c <= c2) goto matched_class_in_brack; /* ...but extend match */
		}
		/* not matched */
		neg = !neg;
	matched:
		if (neg) s = 0;
		/* matchbracketclass */
	}
	return s;
matched_class_in_brack: /* matched %something or range in [] */
	if (neg)
		s = 0;
	else {
matched_class: /* matched %something or . */
		if (MODE_GRAPH == ms->mode) utf8_graphext(&s, ms->src_end);
	}
	return s;
}


static const char *match (MatchState *ms, const char *s, const char *p);


static const char *matchbalance (MatchState *ms, const char *s,
																	 const char *p) {
	if (*p == 0 || *(p+1) == 0)
		luaL_error(ms->L, "unbalanced pattern");
	if (*s != *p) return NULL;
	else {
		int b = *p;
		int e = *(p+1);
		int cont = 1;
		while (++s < ms->src_end) {
			if (*s == e) {
				if (--cont == 0) return s+1;
			}
			else if (*s == b) cont++;
		}
	}
	return NULL;	/* string ends out of balance */
}


static const char *max_expand (MatchState *ms,
	const char *s, const char *p, const char *ep)
{
	const char *sp = s, *es;
	while (sp<ms->src_end && (es = singlematch(ms, sp, p, ep)))
		sp = es;
	/* keeps trying to match with the maximum repetitions */
	while (sp>=s) {
		const char *res = match(ms, sp, ep+1);
		if (res || sp==s) return res;
		if (!ms->mb)
			sp--;	/* else didn't match; reduce 1 repetition to try again */
		else {
			unsigned code = utf8_oced(&sp, s);
			if (MODE_GRAPH == ms->mode)
				while (Grapheme_Extend(code) && sp>s) code = utf8_oced(&sp, s);
		}
	}
	return NULL;
}


static const char *min_expand (MatchState *ms,
	const char *s, const char *p, const char *ep)
{
	do {
		const char *res = match(ms, s, ep+1);
		if (res) return res;
		if (s >= ms->src_end) break;
	} while ((s = singlematch(ms, s, p, ep))); /* try with one more repetition */
	return NULL;
}


static const char *start_capture (MatchState *ms, const char *s,
																		const char *p, int what) {
	const char *res;
	int level = ms->level;
	if (level >= MAX_CAPTURES) luaL_error(ms->L, "too many captures");
	ms->capture[level].init = s;
	ms->capture[level].len = what;
	ms->level = level+1;
	if ((res=match(ms, s, p)) == NULL)	/* match failed? */
		ms->level--;	/* undo capture */
	return res;
}


static const char *end_capture (MatchState *ms, const char *s,
																	const char *p) {
	int l = capture_to_close(ms);
	const char *res;
	ms->capture[l].len = s - ms->capture[l].init;	/* close capture */
	if ((res = match(ms, s, p)) == NULL)	/* match failed? */
		ms->capture[l].len = CAP_UNFINISHED;	/* undo capture */
	return res;
}


static const char *match_capture (MatchState *ms, const char *s, int l) {
	size_t len;
	l = check_capture(ms, l);
	len = ms->capture[l].len;
	if ((size_t)(ms->src_end-s) >= len &&
			memcmp(ms->capture[l].init, s, len) == 0)
		return s+len;
	else return NULL;
}


static const char *match (MatchState *ms, const char *s, const char *p) {
	init: /* using goto's to optimize tail recursion */
	switch (*p) {
		case '(': {	/* start capture */
			if (*(p+1) == ')')	/* position capture? */
				return start_capture(ms, s, p+2, CAP_POSITION);
			else
				return start_capture(ms, s, p+1, CAP_UNFINISHED);
		}
		case ')': {	/* end capture */
			return end_capture(ms, s, p+1);
		}
		case ESC: {
			switch (*(p+1)) {
				case 'b': {	/* balanced string? */
					s = matchbalance(ms, s, p+2);
					if (s == NULL) return NULL;
					p+=4; goto init;	/* else return match(ms, s, p+4); */
				}
#if 0 /* TODO */
				case 'f': {	/* frontier? */
					const char *ep; char previous;
					p += 2;
					if (*p != '[')
						luaL_error(ms->L, "missing `[' after `%%f' in pattern");
					ep = classend(ms, p);	/* points to what is next */
					/* with UTF-8, getting the previous is more complicated */
					previous = (s == ms->src_init) ? '\0' : *(s-1);
					/* use singlematch to apply all necessary magic */
					if (singlematch(uchar(previous), p, ep-1) ||
						 !singlematch(uchar(*s), p, ep-1)) return NULL;
					p=ep; goto init;	/* else return match(ms, s, ep); */
				}
#endif
				default: {
					if (isdigit(uchar(*(p+1)))) {	/* capture results (%0-%9)? */
						s = match_capture(ms, s, uchar(*(p+1)));
						if (s == NULL) return NULL;
						p+=2; goto init;	/* else return match(ms, s, p+2) */
					}
					goto dflt;	/* case default */
				}
			}
		}
		case '\0': {	/* end of pattern */
			return s;	/* match succeeded */
		}
		case '$': {
			if (*(p+1) == '\0')	/* is the `$' the last char in pattern? */
				return (s == ms->src_end) ? s : NULL;	/* check end of string */
			else goto dflt; /* ??? */
		}
		default: dflt: {	/* it is a pattern item */
			const char *ep = classend(ms, p);	/* points to what is next */
			const char *es = 0;
			if (s < ms->src_end) es = singlematch(ms, s, p, ep);
			switch (*ep) {
				case '?': {	/* optional */
					const char *res;
					if (es && (res=match(ms, es, ep+1))) return res;
					p=ep+1; goto init;	/* else return match(ms, s, ep+1); */
				}
				case '*': {	/* 0 or more repetitions */
					return max_expand(ms, s, p, ep);
				}
				case '+': {	/* 1 or more repetitions */
					return (es ? max_expand(ms, es, p, ep) : NULL);
				}
				case '-': {	/* 0 or more repetitions (minimum) */
					return min_expand(ms, s, p, ep);
				}
				default: {
					if (!es) return NULL;
					s=es; p=ep; goto init;	/* else return match(ms, s+1, ep); */
				}
			}
		}
	}
}



static const char *lmemfind (const char *s1, size_t l1,
															 const char *s2, size_t l2) {
	if (l2 == 0) return s1;	/* empty strings are everywhere */
	else if (l2 > l1) return NULL;	/* avoids a negative `l1' */
	else {
		const char *init;	/* to search for a `*s2' inside `s1' */
		l2--;	/* 1st char will be checked by `memchr' */
		l1 = l1-l2;	/* `s2' cannot be found after that */
		while (l1 > 0 && (init = (const char *)memchr(s1, *s2, l1)) != NULL) {
			init++;	 /* 1st char is already checked */
			if (memcmp(init, s2+1, l2) == 0)
				return init-1;
			else {	/* correct `l1' and `s1' to try again */
				l1 -= init-s1;
				s1 = init;
			}
		}
		return NULL;	/* not found */
	}
}


static void push_onecapture (MatchState *ms, int i) {
	int l = ms->capture[i].len;
	if (l == CAP_UNFINISHED) luaL_error(ms->L, "unfinished capture");
	if (l == CAP_POSITION)
		lua_pushinteger(ms->L, ms->capture[i].init - ms->src_init + 1);
	else
		lua_pushlstring(ms->L, ms->capture[i].init, l);
}


static int push_captures (MatchState *ms, const char *s, const char *e) {
	int i;
	luaL_checkstack(ms->L, ms->level, "too many captures");
	if (ms->level == 0 && s) {	/* no explicit captures? */
		lua_pushlstring(ms->L, s, e-s);	/* return whole match */
		return 1;
	}
	else {	/* return all captures */
		for (i=0; i<ms->level; i++)
			push_onecapture(ms, i);
		return ms->level;	/* number of strings pushed */
	}
}


static int unic_find (lua_State *L) {
	size_t l1, l2;
	const char *s = luaL_checklstring(L, 1, &l1);
	const char *p = luaL_checklstring(L, 2, &l2);
	ptrdiff_t init = posrelat(luaL_optinteger(L, 3, 1), l1) - 1;
	if (init < 0) init = 0;
	else if ((size_t)(init) > l1) init = (ptrdiff_t)l1;
	if (lua_toboolean(L, 4) ||	/* explicit request? */
			strpbrk(p, SPECIALS) == NULL) {	/* or no special characters? */
		/* do a plain search */
		const char *s2 = lmemfind(s+init, l1-init, p, l2);
		if (s2) {
			lua_pushinteger(L, s2-s+1);
			lua_pushinteger(L, s2-s+l2);
			return 2;
		}
	}
	else {
		MatchState ms;
		int anchor = (*p == '^') ? (p++, 1) : 0;
		const char *s1=s+init;
		ms.L = L;
		ms.src_init = s;
		ms.src_end = s+l1;
		ms.mode = lua_tointeger(L, lua_upvalueindex(1));
		ms.mb = MODE_MBYTE(ms.mode);
		do {
			const char *res;
			ms.level = 0;
			if ((res=match(&ms, s1, p)) != NULL) {
				lua_pushinteger(L, s1-s+1);	/* start */
				lua_pushinteger(L, res-s);	 /* end */
				return push_captures(&ms, NULL, 0) + 2;
			}
		} while (s1++<ms.src_end && !anchor);
	}
	lua_pushnil(L);	/* not found */
	return 1;
}


static int gfind_aux (lua_State *L) {
	MatchState ms;
	const char *s = lua_tostring(L, lua_upvalueindex(1));
	size_t ls = lua_strlen(L, lua_upvalueindex(1));
	const char *p = lua_tostring(L, lua_upvalueindex(2));
	const char *src;
	ms.L = L;
	ms.src_init = s;
	ms.src_end = s+ls;
	ms.mode = lua_tointeger(L, lua_upvalueindex(4));
	ms.mb = MODE_MBYTE(ms.mode);
	for (src = s + (size_t)lua_tointeger(L, lua_upvalueindex(3));
			 src <= ms.src_end;
			 src++) {
		const char *e;
		ms.level = 0;
		if ((e = match(&ms, src, p)) != NULL) {
			int newstart = e-s;
			if (e == src) newstart++;	/* empty match? go at least one position */
			lua_pushinteger(L, newstart);
			lua_replace(L, lua_upvalueindex(3));
			return push_captures(&ms, src, e);
		}
	}
	return 0;	/* not found */
}


static int gfind (lua_State *L) {
	luaL_checkstring(L, 1);
	luaL_checkstring(L, 2);
	lua_settop(L, 2);
	lua_pushinteger(L, 0);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_pushcclosure(L, gfind_aux, 4);
	return 1;
}


static void add_s (MatchState *ms, luaL_Buffer *b,
									 const char *s, const char *e) {
	lua_State *L = ms->L;
	if (lua_isstring(L, 3)) {
		const char *news = lua_tostring(L, 3);
		size_t l = lua_strlen(L, 3);
		size_t i;
		for (i=0; i<l; i++) {
			if (news[i] != ESC)
				luaL_putchar(b, news[i]);
			else {
				i++;	/* skip ESC */
				if (!isdigit(uchar(news[i])))
					luaL_putchar(b, news[i]);
				else {
					int level = check_capture(ms, news[i]);
					push_onecapture(ms, level);
					luaL_addvalue(b);	/* add capture to accumulated result */
				}
			}
		}
	}
	else {	/* is a function */
		int n;
		lua_pushvalue(L, 3);
		n = push_captures(ms, s, e);
		lua_call(L, n, 1);
		if (lua_isstring(L, -1))
			luaL_addvalue(b);	/* add return to accumulated result */
		else
			lua_pop(L, 1);	/* function result is not a string: pop it */
	}
}


static int unic_gsub (lua_State *L) {
	size_t srcl;
	const char *src = luaL_checklstring(L, 1, &srcl);
	const char *p = luaL_checkstring(L, 2);
	int max_s = luaL_optint(L, 4, srcl+1);
	int anchor = (*p == '^') ? (p++, 1) : 0;
	int n = 0;
	MatchState ms;
	luaL_Buffer b;
	luaL_argcheck(L,
		lua_gettop(L) >= 3 && (lua_isstring(L, 3) || lua_isfunction(L, 3)),
		3, "string or function expected");
	luaL_buffinit(L, &b);
	ms.L = L;
	ms.src_init = src;
	ms.src_end = src+srcl;
	ms.mode = lua_tointeger(L, lua_upvalueindex(1));
	ms.mb = MODE_MBYTE(ms.mode);
	while (n < max_s) {
		const char *e;
		ms.level = 0;
		e = match(&ms, src, p);
		if (e) {
			n++;
			add_s(&ms, &b, src, e);
		}
		if (e && e>src) /* non empty match? */
			src = e;	/* skip it */
		else if (src < ms.src_end)
			luaL_putchar(&b, *src++);
		else break;
		if (anchor) break;
	}
	luaL_addlstring(&b, src, ms.src_end-src);
	luaL_pushresult(&b);
	lua_pushinteger(L, n);	/* number of substitutions */
	return 2;
}

/* }====================================================== */


/* maximum size of each formatted item (> len(format('%99.99f', -1e308))) */
#define MAX_ITEM	512
/* maximum size of each format specification (such as '%-099.99d') */
#define MAX_FORMAT	20


static void addquoted (lua_State *L, luaL_Buffer *b, int arg) {
	size_t l;
	const char *s = luaL_checklstring(L, arg, &l);
	luaL_putchar(b, '"');
	while (l--) {
		switch (*s) {
			case '"': case '\\': case '\n': {
				luaL_putchar(b, '\\');
				luaL_putchar(b, *s);
				break;
			}
			case '\0': {
				luaL_addlstring(b, "\\000", 4);
				break;
			}
			default: {
				luaL_putchar(b, *s);
				break;
			}
		}
		s++;
	}
	luaL_putchar(b, '"');
}


static const char *scanformat (lua_State *L, const char *strfrmt,
																 char *form, int *hasprecision) {
	const char *p = strfrmt;
	while (strchr("-+ #0", *p)) p++;	/* skip flags */
	if (isdigit(uchar(*p))) p++;	/* skip width */
	if (isdigit(uchar(*p))) p++;	/* (2 digits at most) */
	if (*p == '.') {
		p++;
		*hasprecision = 1;
		if (isdigit(uchar(*p))) p++;	/* skip precision */
		if (isdigit(uchar(*p))) p++;	/* (2 digits at most) */
	}
	if (isdigit(uchar(*p)))
		luaL_error(L, "invalid format (width or precision too long)");
	if (p-strfrmt+2 > MAX_FORMAT)	/* +2 to include `%' and the specifier */
		luaL_error(L, "invalid format (too long)");
	form[0] = ESC;
	strncpy(form+1, strfrmt, p-strfrmt+1);
	form[p-strfrmt+2] = 0;
	return p;
}


static int str_format (lua_State *L) {
	int arg = 1;
	size_t sfl;
	const char *strfrmt = luaL_checklstring(L, arg, &sfl);
	const char *strfrmt_end = strfrmt+sfl;
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	while (strfrmt < strfrmt_end) {
		if (*strfrmt != ESC)
			luaL_putchar(&b, *strfrmt++);
		else if (*++strfrmt == ESC)
			luaL_putchar(&b, *strfrmt++);	/* %% */
		else { /* format item */
			char form[MAX_FORMAT];	/* to store the format (`%...') */
			char buff[MAX_ITEM];	/* to store the formatted item */
			int hasprecision = 0;
			if (isdigit(uchar(*strfrmt)) && *(strfrmt+1) == '$')
				return luaL_error(L, "obsolete option (d$) to `format'");
			arg++;
			strfrmt = scanformat(L, strfrmt, form, &hasprecision);
			switch (*strfrmt++) {
				case 'c':	case 'd':	case 'i': {
					sprintf(buff, form, luaL_checkint(L, arg));
					break;
				}
				case 'o':	case 'u':	case 'x':	case 'X': {
					sprintf(buff, form, (unsigned int)(luaL_checknumber(L, arg)));
					break;
				}
				case 'e':	case 'E': case 'f':
				case 'g': case 'G': {
					sprintf(buff, form, luaL_checknumber(L, arg));
					break;
				}
				case 'q': {
					addquoted(L, &b, arg);
					continue;	/* skip the `addsize' at the end */
				}
				case 's': {
					size_t l;
					const char *s = luaL_checklstring(L, arg, &l);
					if (!hasprecision && l >= 100) {
						/* no precision and string is too long to be formatted;
							 keep original string */
						lua_pushvalue(L, arg);
						luaL_addvalue(&b);
						continue;	/* skip the `addsize' at the end */
					}
					else {
						sprintf(buff, form, s);
						break;
					}
				}
				default: {	/* also treat cases `pnLlh' */
					return luaL_error(L, "invalid option to `format'");
				}
			}
			luaL_addlstring(&b, buff, strlen(buff));
		}
	}
	luaL_pushresult(&b);
	return 1;
}


static const luaL_reg uniclib[] = {
	{"len", unic_len}, /* cluster/byte opt. */
	{"sub", unic_sub}, /* cluster/byte opt. */
	{"reverse", str_reverse},
	{"lower", unic_lower},
	{"upper", unic_upper},
	{"char", unic_char},
	{"rep", str_rep},
	{"byte", unic_byte}, /* no cluster ! */
	{"format", str_format},
	{"dump", str_dump},
	{"find", unic_find}, /* cluster */
	{"gfind", gfind}, /* cluster */
	{"gsub", unic_gsub}, /* cluster */
	{NULL, NULL}
};


/*
** Open string library
*/
LUALIB_API int luaopen_unicode (lua_State *L) {
	lua_pushinteger(L, MODE_ASCII);
	luaL_openlib(L, "unicode.ascii", uniclib, 1);
#ifdef SLNUNICODE_AS_STRING
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_pushstring(L, "string");
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
#endif
	lua_pushinteger(L, MODE_LATIN);
	luaL_openlib(L, "unicode.latin1", uniclib, 1);
	lua_pushinteger(L, MODE_GRAPH);
	luaL_openlib(L, "unicode.grapheme", uniclib, 1);
	lua_pushinteger(L, MODE_UTF8);
	luaL_openlib(L, "unicode.utf8", uniclib, 1);
	return 1;
}

