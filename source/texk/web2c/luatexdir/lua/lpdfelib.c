/*tex

    This file will host the encapsulated \PDF\ support code used for inclusion
    and access from \LUA.

*/

#include "ptexlib.h"

/*tex
    We need to avoid collision with some defines in |cpascal.h|.
*/

#undef lpdfelib_orig_input
#undef lpdfelib_orig_output

#ifdef input
#define lpdfelib_orig_input input
#undef input
#endif

#ifdef output
#define lpdfelib_orig_output output
#undef output
#endif

#include "luapplib/pplib.h"

#ifdef lpdfelib_orig_input
#define input  lpdfelib_orig_input
#undef lpdfelib_orig_input
#endif

#ifdef lpdfelib_orig_output
#define output  lpdfelib_orig_output
#undef lpdfelib_orig_output
#endif

#include "lua/luatex-api.h"

/* housekeeping */

/* data types */

#define PDFE_METATABLE            "luatex.pdfe"
#define PDFE_METATABLE_DICTIONARY "luatex.pdfe.dictionary"
#define PDFE_METATABLE_ARRAY      "luatex.pdfe.array"
#define PDFE_METATABLE_STREAM     "luatex.pdfe.stream"
#define PDFE_METATABLE_REFERENCE  "luatex.pdfe.reference"

typedef struct {
    ppdoc *document;
    boolean open;
    boolean isfile;
    int pages;
    int index;
} pdfe_document ;

typedef struct {
    ppdict *dictionary;
    ppref *ref;
} pdfe_dictionary;

typedef struct {
    pparray *array;
    ppref *ref;
} pdfe_array;

typedef struct {
    ppstream *stream;
    ppref *ref;
    int decode;
    int open;
} pdfe_stream;

typedef struct {
    ppref *reference;
} pdfe_reference;

/* housekeeping */

static void pdfe_warning(const char * detail)
{
    formatted_warning("pdfe lib","lua <pdfe %s> expected",detail);
}

/* document helpers */

static pdfe_document *check_isdocument(lua_State * L, int n)
{
    pdfe_document *p = lua_touserdata(L, n);
    if (p != NULL && lua_getmetatable(L, n)) {
        lua_get_metatablelua(luatex_pdfe);
        if (!lua_rawequal(L, -1, -2)) {
            p = NULL;
        }
        lua_pop(L, 2);
        if (p != NULL) {
            return p;
        }
    }
    pdfe_warning("document");
    return NULL;
}

/* dictionary helpers */

static pdfe_dictionary *check_isdictionary(lua_State * L, int n)
{
    pdfe_dictionary *p = lua_touserdata(L, n);
    if (p != NULL && lua_getmetatable(L, n)) {
        lua_get_metatablelua(luatex_pdfe_dictionary);
        if (!lua_rawequal(L, -1, -2)) {
            p = NULL;
        }
        lua_pop(L, 2);
        if (p != NULL) {
            return p;
        }
    }
    pdfe_warning("dictionary");
    return NULL;
}

/* array helpers */

static pdfe_array *check_isarray(lua_State * L, int n)
{
    pdfe_array *p = lua_touserdata(L, n);
    if (p != NULL && lua_getmetatable(L, n)) {
        lua_get_metatablelua(luatex_pdfe_array);
        if (!lua_rawequal(L, -1, -2)) {
            p = NULL;
        }
        lua_pop(L, 2);
        if (p != NULL) {
            return p;
        }
    }
    pdfe_warning("array");
    return NULL;
}

/* stream helpers */

static pdfe_stream *check_isstream(lua_State * L, int n)
{
    pdfe_stream *p = lua_touserdata(L, n);
    if (p != NULL && lua_getmetatable(L, n)) {
        lua_get_metatablelua(luatex_pdfe_stream);
        if (!lua_rawequal(L, -1, -2)) {
            p = NULL;
        }
        lua_pop(L, 2);
        if (p != NULL) {
            return p;
        }
    }
    pdfe_warning("stream");
    return NULL;
}

/* reference helpers */

static pdfe_reference *check_isreference(lua_State * L, int n)
{
    pdfe_reference *p = lua_touserdata(L, n);
    if (p != NULL && lua_getmetatable(L, n)) {
        lua_get_metatablelua(luatex_pdfe_reference);
        if (!lua_rawequal(L, -1, -2)) {
            p = NULL;
        }
        lua_pop(L, 2);
        if (p != NULL) {
            return p;
        }
    }
    pdfe_warning("reference");
    return NULL;
}

/* housekeeping */

#define check_type(field,meta,name) do { \
    lua_get_metatablelua(luatex_##meta); \
    if (lua_rawequal(L, -1, -2)) { \
        lua_pushstring(L,name); \
        return 1; \
    } \
    lua_pop(L, 1); \
} while (0)

static int pdfelib_type(lua_State * L)
{
    void *p = lua_touserdata(L, 1);
    if (p != NULL && lua_getmetatable(L, 1)) {
        check_type(document,  pdfe,           "pdfe");
        check_type(dictionary,pdfe_dictionary,"pdfe.dictionary");
        check_type(array,     pdfe_array,     "pdfe.array");
        check_type(reference, pdfe_reference, "pdfe.reference");
        check_type(stream,    pdfe_stream,    "pdfe.stream");
    }
    return 0;
}

#define define_to_string(field,what) \
static int pdfelib_tostring_##field(lua_State * L) { \
    pdfe_##field *p = check_is##field(L, 1); \
    if (p != NULL) { \
        lua_pushfstring(L, "<" what " %p>", (ppdoc *) p->field); \
        return 1; \
    } \
    return 0; \
}

define_to_string(document,  "pdfe")
define_to_string(dictionary,"pdfe.dictionary")
define_to_string(array,     "pdfe.array")
define_to_string(reference, "pdfe.reference")
define_to_string(stream,    "pdfe.stream")

/*

    There is no gc needed as the library itself manages the objects. Normally
    objects don't take much space. Streams use buffers so (I assume) that they
    are not persistent.

*/

#define pdfe_push_dictionary do {					\
    pdfe_dictionary *d = lua_newuserdata(L, sizeof(pdfe_dictionary));	\
    luaL_getmetatable(L, PDFE_METATABLE_DICTIONARY);			\
    lua_setmetatable(L, -2);						\
    d->dictionary = dictionary;						\
  } while(0)

static int pushdictionary(lua_State * L, ppdict *dictionary)
{
    if (dictionary != NULL) {
        pdfe_push_dictionary;
        lua_pushinteger(L,dictionary->size);
        return 2;
    }
    return 0;
}

static int pushdictionaryonly(lua_State * L, ppdict *dictionary)
{
    if (dictionary != NULL) {
        pdfe_push_dictionary;
        return 1;
    }
    return 0;
}

#define pdfe_push_array do {					\
    pdfe_array *a = lua_newuserdata(L, sizeof(pdfe_array));	\
    luaL_getmetatable(L, PDFE_METATABLE_ARRAY);			\
    lua_setmetatable(L, -2);					\
    a->array = array;						\
  } while (0)

static int pusharray(lua_State * L, pparray * array)
{
    if (array != NULL) {
        pdfe_push_array;
        lua_pushinteger(L,array->size);
        return 2;
    }
    return 0;
}

static int pusharrayonly(lua_State * L, pparray * array)
{
    if (array != NULL) {
        pdfe_push_array;
        return 1;
    }
    return 0;
}

#define pdfe_push_stream do {					\
    pdfe_stream *s = lua_newuserdata(L, sizeof(pdfe_stream));	\
    luaL_getmetatable(L, PDFE_METATABLE_STREAM);		\
    lua_setmetatable(L, -2);					\
    s->stream = stream;						\
    s->open = 0;						\
    s->decode = 0;						\
  } while(0)

static int pushstream(lua_State * L, ppstream * stream)
{
    if (stream != NULL) {
        pdfe_push_stream;
        if (pushdictionary(L, stream->dict) > 0)
            return 3;
        else
            return 1;
    }
    return 0;
}

static int pushstreamonly(lua_State * L, ppstream * stream)
{
    if (stream != NULL) {
        pdfe_push_stream;
        if (pushdictionaryonly(L, stream->dict) > 0)
            return 2;
        else
            return 1;
    }
    return 0;
}

#define pdfe_push_reference do {					\
    pdfe_reference *r = lua_newuserdata(L, sizeof(pdfe_reference));	\
    luaL_getmetatable(L, PDFE_METATABLE_REFERENCE);			\
    lua_setmetatable(L, -2);						\
    r->reference = reference;						\
  } while (0)

static int pushreference(lua_State * L, ppref * reference)
{
    if (reference != NULL) {
        pdfe_push_reference;
        lua_pushinteger(L,reference->number);
        return 2;
    }
    return 0;
}

/*
    PPNULL      null          nil
    PPBOOL      boolean       boolean
    PPINT       boolean       integer
    PPNUM       number        float
    PPNAME      name          string
    PPSTRING    string        string + type
    PPARRAY     array         arrayobject       size
    PPDICT      dictionary    dictionaryobject  size
    PPSTREAM    stream        streamobject      dictionary   size
    PPREF       reference     integer
*/

static int pushvalue(lua_State * L, ppobj *object)
{
    switch (object->type) {
        case PPNONE:
        case PPNULL:
            lua_pushnil(L);
            return 1;
            break;
        case PPBOOL:
            {
                int b;
                ppobj_get_bool(object,b);
                lua_pushboolean(L,b);
                return 1;
            }
            break;
        case PPINT:
            {
                ppint i;
                ppobj_get_int(object,i);
                lua_pushinteger(L, i);
                return 1;
            }
            break;
        case PPNUM:
            {
                ppnum n;
                ppobj_get_num(object,n);
                lua_pushnumber(L, n);
                return 1;
            }
            break;
        case PPNAME:
            {
                ppname name = ppobj_get_name(object);
                /*
                size_t n = ppname_size(name);
                lua_pushlstring(L, (const char *) name, n);
                */
                lua_pushstring(L, (const char *) ppname_decoded(name));
                /*
                lua_pushboolean(L, ppname_exec(name));
                return 2;
                */
                return 1;
            }
            break;
        case PPSTRING:
            {
                ppstring str = ppobj_get_string(object);
                size_t n = ppstring_size(str);
                lua_pushlstring(L,(const char *) str, n);
                lua_pushinteger(L, ppstring_type(str));
                return 2;
            }
            break;
        case PPARRAY:
            return pusharray(L, ppobj_get_array(object));
            break;
        case PPDICT:
            return pushdictionary(L, ppobj_get_dict(object));
            break;
        case PPSTREAM:
            return pushstream(L, ppobj_rget_stream(object));
            break;
        case PPREF:
            return pushreference(L, ppobj_get_ref(object));
            break;
    }
    return 0;
}

/* catalogdictionary = getcatalog(documentobject) */

static int pdfelib_getcatalog(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p == NULL)
        return 0;
    return pushdictionaryonly(L,ppdoc_catalog(p->document));
}

/* trailerdictionary = gettrailer(documentobject) */

static int pdfelib_gettrailer(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p == NULL)
        return 0;
    return pushdictionaryonly(L,ppdoc_trailer(p->document));
}

/* infodictionary = getinfo(documentobject) */

static int pdfelib_getinfo(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p == NULL)
        return 0;
    return pushdictionaryonly(L,ppdoc_info(p->document));
}

/* key, type, value, detail = getfromdictionary(dictionary,index) */

static int pdfelib_getfromdictionary(lua_State * L)
{
    pdfe_dictionary *d = check_isdictionary(L, 1);
    if (d != NULL) {
        int index = luaL_checkint(L, 2) - 1;
        if (index < d->dictionary->size) {
            ppobj *object = ppdict_at(d->dictionary,index);
            ppname key = ppdict_key(d->dictionary,index);
            lua_pushstring(L,(const char *) key);
            lua_pushinteger(L,(int) object->type);
            return 2 + pushvalue(L,object);
        }
    }
    return 0;
}

static int pdfelib_getfromdictionarybyname(lua_State * L)
{
    pdfe_dictionary *d = check_isdictionary(L, 1);
    if (d != NULL) {
        const char *name = luaL_checkstring(L, 2);
        ppobj *object = ppdict_get_obj(d->dictionary,name);
        if (object != NULL) {
            lua_pushinteger(L,(int) object->type);
            return 1 + pushvalue(L,object);
        }
    }
    return 0;
}

/* type, value, detail = getfromarray(arrayobject,index) */

static int pdfelib_getfromarray(lua_State * L)
{
    pdfe_array *a = check_isarray(L, 1);
    if (a != NULL) {
        int index = luaL_checkint(L, 2) - 1;
        if (index < a->array->size) {
            ppobj *object = pparray_at(a->array,index);
            lua_pushinteger(L,(int) object->type);
            return 1 + pushvalue(L,object);
        }
    }
    return 0;
}

/* t = arraytotable(arrayobject) */

static void pdfelib_totable(lua_State * L, ppobj * object, int flat)
{
    int n = pushvalue(L,object);
    if (flat && n < 2) {
        return;
    }
    /* [value] [extra] [more] */
    lua_createtable(L,n+1,0);
    if (n == 1) {
        /* value { nil, nil } */
        lua_insert(L,-2);
        /* { nil, nil } value */
        lua_rawseti(L,-2,2);
        /* { nil , value } */
    } else if (n == 2) {
        /* value extra { nil, nil, nil } */
        lua_insert(L,-3);
        /* { nil, nil, nil } value extra */
        lua_rawseti(L,-3,3);
        /* { nil, nil, extra } value */
        lua_rawseti(L,-2,2);
        /* { nil, value, extra } */
    } else if (n == 3) {
        /* value extra more { nil, nil, nil, nil } */
        lua_insert(L,-4);
        /* { nil, nil, nil, nil, nil } value extra more */
        lua_rawseti(L,-4,4);
        /* { nil, nil, nil, more } value extra */
        lua_rawseti(L,-3,3);
        /* { nil, nil, extra, more } value */
        lua_rawseti(L,-2,2);
        /* { nil, value, extra, more } */
    }
    lua_pushinteger(L,(int) object->type);
    /* { nil, [value], [extra], [more] } type */
    lua_rawseti(L,-2,1);
    /* { type, [value], [extra], [more] } */
    return;
}

static int pdfelib_arraytotable(lua_State * L)
{
    pdfe_array *a = check_isarray(L, 1);
    if (a != NULL) {
        int flat = lua_isboolean(L,2);
        int i = 0;
        lua_createtable(L,i,0);
        /* table */
        for (i=0;i<a->array->size;i++) {
            ppobj *object = pparray_at(a->array,i);
            pdfelib_totable(L,object,flat);
            /* table { type, [value], [extra], [more] } */
            lua_rawseti(L,-2,i+1);
            /* table[i] = { type, [value], [extra], [more] } */
        }
        return 1;
    }
    return 0;
}

/* t = dictionarytotable(dictionary) */

static int pdfelib_dictionarytotable(lua_State * L)
{
    pdfe_dictionary *d = check_isdictionary(L, 1);
    if (d != NULL) {
        int flat = lua_isboolean(L,2);
        int i = 0;
        lua_createtable(L,0,i);
        /* table */
        for (i=0;i<d->dictionary->size;i++) {
            ppobj *object = ppdict_at(d->dictionary,i);
            ppname key = ppdict_key(d->dictionary,i);
            lua_pushstring(L,(const char *) key);
            /* table key */
            pdfelib_totable(L,object,flat);
            /* table key { type, [value], [extra], [more] } */
            lua_rawset(L,-3);
            /* table[key] = { type, [value], [extra] } */
        }
        return 1;
    }
    return 0;
}

/* { { dict, size, objnum }, ... } = pagestotable(document) */

static int pdfelib_pagestotable(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p != NULL) {
        ppdoc *d = p->document;
        ppref *r = NULL;
        int i = 0;
        lua_createtable(L,ppdoc_page_count(d),0);
        /* pages[1..n] */
        for (r = ppdoc_first_page(d), i = 1; r != NULL; r = ppdoc_next_page(d), ++i) {
            lua_createtable(L,3,0);
            pushdictionary(L,ppref_obj(r)->dict);
            /* table dictionary n */
            lua_rawseti(L,-3,2);
            /* table dictionary */
            lua_rawseti(L,-2,1);
            /* table */
            lua_pushinteger(L,r->number);
            /* table reference */
            lua_rawseti(L,-2,3);
            /* table */
            lua_rawseti(L,-2,i);
            /* pages[i] = { dictionary, size, objnum } */
        }
        return 1;
    }
    return 0;
}

/* okay = openstream(streamobject,[decode]) */

static int pdfelib_openstream(lua_State * L)
{
    pdfe_stream *s = check_isstream(L, 1);
    if (s != NULL) {
        if (s->open == 0) {
            if (lua_gettop(L) > 1) {
                s->decode = lua_isboolean(L, 2);
            }
            s->open = 1;
        }
        lua_pushboolean(L,1);
        return 1;
    }
    return 0;
}

/* closestream(streamobject) */

static int pdfelib_closestream(lua_State * L)
{
    pdfe_stream *s = check_isstream(L, 1);
    if (s != NULL) {
        if (s->open >0) {
            ppstream_done(s->stream);
            s->open = 0;
            s->decode = 0;
        }
    }
    return 0;
}

/* string, n = readfromstream(streamobject) */

static int pdfelib_readfromstream(lua_State * L)
{
    pdfe_stream *s = check_isstream(L, 1);
    if (s != NULL) {
        size_t n = 0;
        uint8_t *d = NULL;
        if (s->open == 1) {
            d = ppstream_first(s->stream,&n,s->decode);
            s->open = 2;
        } else if (s->open == 2) {
            d = ppstream_next(s->stream,&n);
        } else {
            return 0;
        }
        lua_pushlstring(L, (const char *) d, n);
        lua_pushinteger(L, (int) n);
        return 2;
    }
    return 0;
}

/* string, n = readwhilestream(streamobject) */

static int pdfelib_readwholestream(lua_State * L)
{
    pdfe_stream *s = check_isstream(L, 1);
    if (s != NULL) {
        uint8_t *b = NULL;
        int decode = 0;
        size_t n = 0;
        if (s->open > 0) {
            ppstream_done(s->stream);
            s->open = 0;
            s->decode = 0;
        }
        if (lua_gettop(L) > 1) {
            decode = lua_isboolean(L, 2);
        }
        b = ppstream_all(s->stream,&n,decode);
        lua_pushlstring(L, (const char *) b, n);
        lua_pushinteger(L, (int) n);
        ppstream_done(s->stream);
        return 2;
    }
    return 0;
}

/* documentobject = open(filename) */

static int pdfelib_open(lua_State * L)
{
    const char *filename = luaL_checkstring(L, 1);
    ppdoc *d = ppdoc_load(filename);
    if (d == NULL) {
        formatted_warning("pdfe lib","no valid pdf file '%s'",filename);
    } else {
        pdfe_document *p = (pdfe_document *) lua_newuserdata(L, sizeof(pdfe_document));
        luaL_getmetatable(L, PDFE_METATABLE);
        lua_setmetatable(L, -2);
        p->document = d;
        p->open = true;
        p->isfile = true;
        return 1;
    }
    return 0;
}

/* documentobject = open(string|pointer,length) */

static int pdfelib_new(lua_State * L)
{
    const char *docstream = NULL;
    char *docstream_usr = NULL ;
    unsigned long long stream_size;
    ppdoc *d;
    switch (lua_type(L, 1)) {
        case LUA_TSTRING:
            /* stream as Lua string */
            docstream = luaL_checkstring(L, 1);
            break;
        case LUA_TLIGHTUSERDATA:
            /* stream as sequence of bytes */
            docstream = (const char *) lua_touserdata(L, 1);
            break;
        default:
            luaL_error(L, "bad <pdfe> argument: string or lightuserdata expected");
            break;
    }
    if (docstream == NULL) {
        luaL_error(L, "bad <pdfe> document");
    }
    /* size of the stream */
    stream_size = (unsigned long long) luaL_checkint(L, 2);
    /* do we need to copy? ask */
    docstream_usr = (char *)xmalloc((unsigned) (stream_size + 1));
    if (! docstream_usr) {
        luaL_error(L, "no room for <pdfe>");
    }
    memcpy(docstream_usr, docstream, (stream_size + 1));
    docstream_usr[stream_size]='\0';
    d = ppdoc_mem(docstream_usr, stream_size);
    if (d != NULL) {
        pdfe_document *p = (pdfe_document *) lua_newuserdata(L, sizeof(pdfe_document));
        luaL_getmetatable(L, PDFE_METATABLE);
        lua_setmetatable(L, -2);
        p->document = d;
        p->open = true;
        p->isfile = false;
        return 1;
    }
    return 0;
}

/* status = unencrypt(documentobject,user,owner) */

static int pdfelib_unencrypt(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p != NULL) {
        size_t u = 0;
        size_t o = 0;
        const char* user = NULL;
        const char* owner = NULL;
        int top = lua_gettop(L);
        if (top > 1) {
            if (lua_type(L,2) == LUA_TSTRING) {
                user = lua_tolstring(L, 2, &u);
            } else {
                /* we're not too picky but normally it will be nil or false */
            }
            if (top > 2) {
                if (lua_type(L,3) == LUA_TSTRING) {
                    owner = lua_tolstring(L, 3, &o);
                } else {
                    /* we're not too picky but normally it will be nil or false */
                }
            }
            lua_pushinteger(L, (int) ppdoc_crypt_pass(p->document,user,u,owner,o));
        }
    }
    lua_pushinteger(L, (int) PPCRYPT_FAIL);
    return 0;
}


/* close(documentobject) */

static int pdfelib_free(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p != NULL) {
        if (p->open) {
            ppdoc_free(p->document);
            p->open = false;
            p->document = NULL;
        }
    }
    return 0;
}

static int pdfelib_close(lua_State * L)
{
    return pdfelib_free(L);
}

/* bytes = size(documentobject) */

static int pdfelib_getsize(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p == NULL)
        return 0;
    lua_pushinteger(L,(int) ppdoc_file_size(p->document));
    return 1;
}

/* major, minor = version(documentobject) */

static int pdfelib_getversion(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p == NULL) {
        return 0;
    } else {
        int minor;
        int major = ppdoc_version_number(p->document, &minor);
        lua_pushinteger(L,(int) major);
        lua_pushinteger(L,(int) minor);
        return 2;
    }
}

/* status = status(documentobject) */

static int pdfelib_getstatus(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p == NULL)
        return 0;
    lua_pushinteger(L,(int) ppdoc_crypt_status(p->document));
    return 1;
}

/* n = nofobjects(documentobject) */

static int pdfelib_getnofobjects(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p == NULL)
        return 0;
    lua_pushinteger(L,(int) ppdoc_objects(p->document));
    return 1;
}

/* n = nofpages(documentobject) */

static int pdfelib_getnofpages(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p == NULL)
        return 0;
    lua_pushinteger(L,(int) ppdoc_page_count(p->document));
    return 1;
}

/* bytes, waste = nofpages(documentobject) */

static int pdfelib_getmemoryusage(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p != NULL) {
        size_t w = 0;
        size_t m = ppdoc_memory(p->document,&w);
        lua_pushinteger(L,(int) m);
        lua_pushinteger(L,(int) w);
        return 2;
    }
    return 0;
}

/* dictionaryobject = getpage(documentobject,pagenumber) */

static int pdfelib_getpage(lua_State * L)
{
    pdfe_document *p = check_isdocument(L, 1);
    if (p != NULL) {
        int page = luaL_checkint(L, 2);
        if (page <= ppdoc_page_count(p->document)) {
            ppref *pp = ppdoc_page(p->document,page);
            return pushdictionaryonly(L, ppref_obj(pp)->dict);
        }
    }
    return 0;
}

/* table = getbox(dictionary,name) */

static int pdfelib_getbox(lua_State * L)
{
    if (lua_gettop(L) > 0 && lua_type(L,1) == LUA_TSTRING) {
        pdfe_dictionary *p = check_isdictionary(L, 1);
        if (p != NULL) {
            const char *key = lua_tostring(L,1);
            pprect box ; box.lx = box.rx = box.ly = box.ry = 0;
            pprect *r = ppdict_get_box(p->dictionary,key,&box);
            if (r != NULL) {
                lua_createtable(L,4,0);
                lua_pushnumber(L,r->lx);
                lua_rawseti(L,-2,1);
                lua_pushnumber(L,r->ly);
                lua_rawseti(L,-2,2);
                lua_pushnumber(L,r->rx);
                lua_rawseti(L,-2,3);
                lua_pushnumber(L,r->ry);
                lua_rawseti(L,-2,4);
                return 1;
            }
        }
    }
    return 0;
}

/* [dictionary|array|stream]object = getfromreference(referenceobject) */

static int pdfelib_getfromreference(lua_State * L)
{
    pdfe_reference *r = check_isreference(L, 1);
    if (r != NULL) {
        ppobj *o = ppref_obj(r->reference);
        lua_pushinteger(L,o->type);
        return 1 + pushvalue(L,o);
    }
    return 0;
}

/* <string>         = getstring    (array|dict|ref,index|key) */
/* <integer>        = getinteger   (array|dict|ref,index|key) */
/* <number>         = getnumber    (array|dict|ref,index|key) */
/* <boolan>         = getboolean   (array|dict|ref,index|key) */
/* <string>         = getname      (array|dict|ref,index|key) */
/* <dictionary>     = getdictionary(array|dict|ref,index|key) */
/* <array>          = getarray     (array|dict|ref,index|key) */
/* <stream>, <dict> = getstream    (array|dict|ref,index|key) */

#define pdfelib_get_value_check_1 do {					\
    if (p == NULL) {							\
      if (t == LUA_TSTRING) {						\
	normal_warning("pdfe lib","lua <pdfe dictionary> expected");	\
      } else if (t == LUA_TNUMBER) {					\
	normal_warning("pdfe lib","lua <pdfe array> expected");		\
      } else {								\
	normal_warning("pdfe lib","invalid arguments");			\
      }									\
      return 0;								\
    } else if (! lua_getmetatable(L, 1)) {				\
      normal_warning("pdfe lib","first argument should be a <pde array> or <pde dictionary>"); \
    }									\
  } while (0)

#define pdfelib_get_value_check_2 \
    normal_warning("pdfe lib","second argument should be integer or string");

#define pdfelib_get_value_direct(get_d,get_a) do {			\
    int t = lua_type(L,2);						\
    void *p = lua_touserdata(L, 1);					\
    pdfelib_get_value_check_1;						\
    if (t == LUA_TSTRING) {						\
      const char *key = lua_tostring(L,-2);				\
      lua_get_metatablelua(luatex_pdfe_dictionary);			\
      if (lua_rawequal(L, -1, -2)) {					\
	value = get_d(((pdfe_dictionary *) p)->dictionary, key);	\
      } else {								\
	lua_pop(L,1);							\
	lua_get_metatablelua(luatex_pdfe_reference);			\
	if (lua_rawequal(L, -1, -2)) {					\
	  ppobj * o = ppref_obj((ppref *) (((pdfe_reference *) p)->reference)); \
	  if (o != NULL && o->type == PPDICT) {				\
	    value = get_d((ppdict *)o->dict, key);			\
	  }								\
	}								\
      }									\
    } else if (t == LUA_TNUMBER) {					\
      size_t index = lua_tointeger(L,-2);				\
      lua_get_metatablelua(luatex_pdfe_array);				\
      if (lua_rawequal(L, -1, -2)) {					\
	value = get_a(((pdfe_array *) p)->array, index);		\
      } else {								\
	lua_pop(L,1);							\
	lua_get_metatablelua(luatex_pdfe_reference);			\
	if (lua_rawequal(L, -1, -2)) {					\
	  ppobj * o = ppref_obj((ppref *) (((pdfe_reference *) p)->reference)); \
	  if (o != NULL && o->type == PPARRAY) {			\
	    value = get_a((pparray *) o->array, index);			\
	  }								\
	}								\
      }									\
    } else {								\
      pdfelib_get_value_check_2;					\
    }									\
  } while (0)

#define pdfelib_get_value_indirect(get_d,get_a) do {			\
    int t = lua_type(L,2);						\
    void *p = lua_touserdata(L, 1);					\
    pdfelib_get_value_check_1;						\
    if (t == LUA_TSTRING) {						\
      const char *key = lua_tostring(L,-2);				\
      lua_get_metatablelua(luatex_pdfe_dictionary);			\
      if (lua_rawequal(L, -1, -2)) {					\
	okay = get_d(((pdfe_dictionary *) p)->dictionary, key, &value); \
      } else {								\
	lua_pop(L,1);							\
	lua_get_metatablelua(luatex_pdfe_reference);			\
	if (lua_rawequal(L, -1, -2)) {					\
	  ppobj * o = ppref_obj((ppref *) (((pdfe_reference *) p)->reference)); \
	  if (o != NULL && o->type == PPDICT)				\
	    okay = get_d(o->dict, key, &value);				\
	}								\
      }									\
    } else if (t == LUA_TNUMBER) {					\
      size_t index = lua_tointeger(L,-2);				\
      lua_get_metatablelua(luatex_pdfe_array);				\
      if (lua_rawequal(L, -1, -2)) {					\
	okay = get_a(((pdfe_array *) p)->array, index, &value);		\
      } else {								\
	lua_pop(L,1);							\
	lua_get_metatablelua(luatex_pdfe_reference);			\
	if (lua_rawequal(L, -1, -2)) {					\
	  ppobj * o = ppref_obj((ppref *) (((pdfe_reference *) p)->reference)); \
	  if (o != NULL && o->type == PPARRAY)				\
	    okay = get_a(o->array, index, &value);			\
	}								\
      }									\
    } else {								\
      pdfelib_get_value_check_2;					\
    }									\
  } while (0)

static int pdfelib_getstring(lua_State * L)
{
    if (lua_gettop(L) > 1) {
        ppstring value = NULL;
        pdfelib_get_value_direct(ppdict_rget_string,pparray_rget_string);
        if (value != NULL) {
            lua_pushstring(L,(const char *) value);
            return 1;
        }
    }
    return 0;
}

static int pdfelib_getinteger(lua_State * L)
{
    if (lua_gettop(L) > 1) {
        ppint value = 0;
	int okay = 0;
        pdfelib_get_value_indirect(ppdict_rget_int,pparray_rget_int);
        if (okay) {
            lua_pushinteger(L,(int) value);
            return 1;
        }
    }
    return 0;
}

static int pdfelib_getnumber(lua_State * L)
{
    if (lua_gettop(L) > 1) {
        ppnum value = 0;
	int okay = 0;
        pdfelib_get_value_indirect(ppdict_rget_num,pparray_rget_num);
        if (okay) {
            lua_pushnumber(L,value);
            return 1;
        }
    }
    return 0;
}

static int pdfelib_getboolean(lua_State * L)
{
    if (lua_gettop(L) > 1) {
        int value = 0;
	int okay = 0;
        pdfelib_get_value_indirect(ppdict_rget_bool,pparray_rget_bool);
        if (okay) {
            lua_pushboolean(L,value);
            return 1;
        }
    }
    return 0;
}

static int pdfelib_getname(lua_State * L)
{
    if (lua_gettop(L) > 1) {
        ppname value = NULL;
        pdfelib_get_value_direct(ppdict_rget_name,pparray_rget_name);
        if (value != NULL) {
            lua_pushstring(L,(const char *) ppname_decoded(value));
            return 1;
        }
    }
    return 0;
}

static int pdfelib_getdictionary(lua_State * L)
{
    if (lua_gettop(L) > 1) {
        ppdict * value = NULL;
        pdfelib_get_value_direct(ppdict_rget_dict,pparray_rget_dict);
        if (value != NULL) {
            return pushdictionaryonly(L,value);
        }
    }
    return 0;
}

static int pdfelib_getarray(lua_State * L)
{
    if (lua_gettop(L) > 1) {
        pparray * value = NULL;
        pdfelib_get_value_direct(ppdict_rget_array,pparray_rget_array);
        if (value != NULL) {
            return pusharrayonly(L,value);
        }
    }
    return 0;
}

static int pdfelib_getstream(lua_State * L)
{
    if (lua_gettop(L) > 1) {
        ppobj * value = NULL;
        pdfelib_get_value_direct(ppdict_rget_obj,pparray_rget_obj);
        if (value != NULL && value->type == PPSTREAM) {
            return pushstreamonly(L,(ppstream *) value->stream);
        }
    }
    return 0;
}

/* accessors */

static int pdfelib_pushvalue(lua_State * L, ppobj *object)
{
    switch (object->type) {
        case PPNONE:
        case PPNULL:
            lua_pushnil(L);
            break;
        case PPBOOL:
            {
                int b;
                ppobj_get_bool(object,b);
                lua_pushboolean(L,b);
            }
            break;
        case PPINT:
            {
                ppint i;
                ppobj_get_int(object,i);
                lua_pushinteger(L, i);
            }
            break;
        case PPNUM:
            {
                ppnum n;
                ppobj_get_num(object,n);
                lua_pushnumber(L, n);
            }
            break;
        case PPNAME:
            {
                ppname name = ppobj_get_name(object);
                lua_pushstring(L, (const char *) ppname_decoded(name));
            }
            break;
        case PPSTRING:
            {
                ppstring str = ppobj_get_string(object);
                size_t n = ppstring_size(str);
                lua_pushlstring(L,(const char *) str, n);
            }
            break;
        case PPARRAY:
            return pusharrayonly(L, ppobj_get_array(object));
            break;
        case PPDICT:
            return pushdictionary(L, ppobj_get_dict(object));
            break;
        case PPSTREAM:
            return pushstream(L, ppobj_rget_stream(object));
            break;
        case PPREF:
            pushreference(L, ppobj_get_ref(object));
            break;
        default:
            lua_pushnil(L);
            break;
    }
    return 1;
}

static int pdfelib_access(lua_State * L)
{
    if (lua_type(L,2) == LUA_TSTRING) {
        pdfe_document *p = lua_touserdata(L, 1);
        const char *s = lua_tostring(L,2);
        if (lua_key_eq(s,catalog) || lua_key_eq(s,Catalog)) {
            return pushdictionaryonly(L,ppdoc_catalog(p->document));
        } else if (lua_key_eq(s,info) || lua_key_eq(s,Info)) {
            return pushdictionaryonly(L,ppdoc_info(p->document));
        } else if (lua_key_eq(s,trailer) || lua_key_eq(s,Trailer)) {
            return pushdictionaryonly(L,ppdoc_trailer(p->document));
        } else if (lua_key_eq(s,pages) || lua_key_eq(s,Pages)) {
            int i = 0;
            ppref *r;
            ppdoc *d = p->document;
            lua_createtable(L,ppdoc_page_count(d),0);
            /* pages[1..n] */
            for (r = ppdoc_first_page(d), i = 1; r != NULL; r = ppdoc_next_page(d), ++i) {
                pushdictionaryonly(L,ppref_obj(r)->dict);
                lua_rawseti(L,-2,i);
            }
            return 1 ;
        }
    }
    return 0;
}

static int pdfelib_array_access(lua_State * L)
{
    if (lua_type(L,2) == LUA_TNUMBER) {
        pdfe_array *p = lua_touserdata(L, 1);
        ppint index = lua_tointeger(L,2) - 1;
        ppobj *o = pparray_rget_obj(p->array,index);
        if (o != NULL) {
            return pdfelib_pushvalue(L,o);
        }
    }
    return 0;
}

static int pdfelib_array_size(lua_State * L)
{
    pdfe_array *p = lua_touserdata(L, 1);
    lua_pushinteger(L,p->array->size);
    return 1;
}

static int pdfelib_dictionary_access(lua_State * L)
{
    pdfe_dictionary *p = lua_touserdata(L, 1);
    if (lua_type(L,2) == LUA_TSTRING) {
        ppstring key = lua_tostring(L,2);
        ppobj *o = ppdict_rget_obj(p->dictionary,key);
        if (o != NULL) {
            return pdfelib_pushvalue(L,o);
        }
    } else if (lua_type(L,2) == LUA_TNUMBER) {
        ppint index = lua_tointeger(L,2) - 1;
        ppobj *o = ppdict_at(p->dictionary,index);
        if (o != NULL) {
            return pdfelib_pushvalue(L,o);
        }
    }
    return 0;
}

static int pdfelib_dictionary_size(lua_State * L)
{
    pdfe_dictionary *p = lua_touserdata(L, 1);
    lua_pushinteger(L,p->dictionary->size);
    return 1;
}

/* initialization */

static const struct luaL_Reg pdfelib[] = {
    /* management */
    { "type",                    pdfelib_type },
    { "open",                    pdfelib_open },
    { "new",                     pdfelib_new },
    { "close",                   pdfelib_close },
    { "unencrypt",               pdfelib_unencrypt },
    /* statistics */
    { "getversion",              pdfelib_getversion },
    { "getstatus",               pdfelib_getstatus },
    { "getsize",                 pdfelib_getsize },
    { "getnofobjects",           pdfelib_getnofobjects },
    { "getnofpages",             pdfelib_getnofpages },
    { "getmemoryusage",          pdfelib_getmemoryusage },
    /* getters */
    { "getcatalog",              pdfelib_getcatalog },
    { "gettrailer",              pdfelib_gettrailer },
    { "getinfo",                 pdfelib_getinfo },
    { "getpage",                 pdfelib_getpage },
    { "getbox",                  pdfelib_getbox },
    { "getfromreference",        pdfelib_getfromreference },
    { "getfromdictionary",       pdfelib_getfromdictionary },
    { "getfromdictionarybyname", pdfelib_getfromdictionarybyname },
    { "getfromarray",            pdfelib_getfromarray },
    { "dictionarytotable",       pdfelib_dictionarytotable },
    { "arraytotable",            pdfelib_arraytotable },
    { "pagestotable",            pdfelib_pagestotable },
    /* more getters */
    { "getstring",               pdfelib_getstring },
    { "getinteger",              pdfelib_getinteger },
    { "getnumber",               pdfelib_getnumber },
    { "getboolean",              pdfelib_getboolean },
    { "getname",                 pdfelib_getname },
    { "getdictionary",           pdfelib_getdictionary },
    { "getarray",                pdfelib_getarray },
    { "getstream",               pdfelib_getstream },
    /* streams */
    { "readwholestream",         pdfelib_readwholestream },
    /* not really needed */
    { "openstream",              pdfelib_openstream },
    { "readfromstream",          pdfelib_readfromstream },
    { "closestream",             pdfelib_closestream },
    /* done */
    { NULL,                      NULL}
};

static const struct luaL_Reg pdfelib_m[] = {
    { "__tostring", pdfelib_tostring_document },
    { "__gc",       pdfelib_free },
    { "__index",    pdfelib_access },
    { NULL,         NULL}
};

static const struct luaL_Reg pdfelib_m_dictionary[] = {
    { "__tostring", pdfelib_tostring_dictionary },
    { "__index",    pdfelib_dictionary_access },
    { "__len",      pdfelib_dictionary_size },
    { NULL,         NULL}
};

static const struct luaL_Reg pdfelib_m_array[] = {
    { "__tostring", pdfelib_tostring_array },
    { "__index",    pdfelib_array_access },
    { "__len",      pdfelib_array_size },
    { NULL,         NULL}
};

static const struct luaL_Reg pdfelib_m_stream[] = {
    { "__tostring", pdfelib_tostring_stream },
    { NULL,         NULL}
};

static const struct luaL_Reg pdfelib_m_reference[] = {
    { "__tostring", pdfelib_tostring_reference },
    { NULL,         NULL}
};

int luaopen_pdfe(lua_State * L)
{
    luaL_newmetatable(L, PDFE_METATABLE_DICTIONARY);
    luaL_openlib(L, NULL, pdfelib_m_dictionary, 0);

    luaL_newmetatable(L, PDFE_METATABLE_ARRAY);
    luaL_openlib(L, NULL, pdfelib_m_array, 0);

    luaL_newmetatable(L, PDFE_METATABLE_STREAM);
    luaL_openlib(L, NULL, pdfelib_m_stream, 0);

    luaL_newmetatable(L, PDFE_METATABLE_REFERENCE);
    luaL_openlib(L, NULL, pdfelib_m_reference, 0);

    /* the main metatable of pdfe userdata */

    luaL_newmetatable(L, PDFE_METATABLE);
    luaL_openlib(L, NULL, pdfelib_m, 0);
    luaL_openlib(L, "pdfe", pdfelib, 0);

    return 1;
}
