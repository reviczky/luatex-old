/* Copyright (C) 2007 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*			   Python Interface to FontForge		      */


#ifndef _NO_PYTHON
#include "Python.h"
#include "structmember.h"

#if !defined( Py_RETURN_NONE )
/* Not defined before 2.4 */
# define Py_RETURN_NONE		return( Py_INCREF(Py_None), Py_None )
#endif
#define Py_RETURN(self)		return( Py_INCREF((PyObject *) (self)), (PyObject *) (self) )

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

#include "pfaeditui.h"
#include "ttf.h"
#include "plugins.h"
#include "ustring.h"
#include "scripting.h"
#include "scriptfuncs.h"
#include <math.h>
#include <unistd.h>

static FontView *fv_active_in_ui = NULL;

/* A contour is a list of points, some on curve, some off. */
/* A closed contour is a circularly linked list */
/* A contour may be either quadratic or cubic */
/* cubic contours have two off-curve points between on-curve points -- or none for lines */
/* quadratic contours have one off-curve points between on-curve -- or none for lines */
/*  quadratic contours may also have two adjacent off-curve points, in which case an on-curve point is interpolated between (as in truetype) */
/* A layer is a set of contours all to be drawn together */

typedef struct ff_point {
    PyObject_HEAD
    /* Type-specific fields go here. */
    float x,y;
    int on_curve;
} PyFF_Point;
static PyTypeObject PyFF_PointType;

typedef struct ff_contour {
    PyObject_HEAD
    /* Type-specific fields go here. */
    int pt_cnt, pt_max;
    struct ff_point **points;
    short is_quadratic, closed;		/* bit flags, but access to short is faster */
} PyFF_Contour;
static PyTypeObject PyFF_ContourType;

typedef struct ff_layer {
    PyObject_HEAD
    /* Type-specific fields go here. */
    short cntr_cnt, cntr_max;
    struct ff_contour **contours;
    int is_quadratic;		/* bit flags, but access to int is faster */
} PyFF_Layer;
static PyTypeObject PyFF_LayerType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineChar *sc;
    uint8 replace;
    uint8 ended;
    uint8 changed;
} PyFF_GlyphPen;
static PyTypeObject PyFF_GlyphPenType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineChar *sc;
} PyFF_Glyph;
static PyTypeObject PyFF_GlyphType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineFont *sf;
} PyFF_Private;
static PyTypeObject PyFF_PrivateType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    FontView *fv;
    int by_glyphs;
} PyFF_Selection;
static PyTypeObject PyFF_SelectionType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    SplineFont *sf;
    struct ttf_table *cvt;
} PyFF_Cvt;
static PyTypeObject PyFF_CvtType;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    FontView *fv;
    PyFF_Private *private;
    PyFF_Cvt *cvt;
    PyFF_Selection *selection;
} PyFF_Font;
static PyTypeObject PyFF_FontType;

static int SSSelectOnCurve(SplineSet *ss,int pos);
static SplineSet *SSFromContour(PyFF_Contour *, int *start);
static PyFF_Contour *ContourFromSS(SplineSet *,PyFF_Contour *);
static SplineSet *SSFromLayer(PyFF_Layer *);
static PyFF_Layer *LayerFromSS(SplineSet *,PyFF_Layer *);
static PyFF_Layer *LayerFromLayer(Layer *,PyFF_Layer *);

/* ************************************************************************** */
/* Utilities */
/* ************************************************************************** */

struct flaglist { char *name; int flag; };
static int FlagsFromString(char *str,struct flaglist *flags) {
    int i;
    for ( i=0; flags[i].name!=NULL; ++i )
	if ( strcmp(str,flags[i].name)==0 )
return( flags[i].flag );

    PyErr_Format( PyExc_TypeError, "Unknown flag %s", str );
return( 0x80000000 );
}

static int FlagsFromTuple(PyObject *tuple,struct flaglist *flags) {
    int ret = 0,temp;
    int i;
    char *str = NULL;
    PyObject *obj;

    /* Might be omitted */
    if ( tuple == NULL )
return( 0 );
    /* Might just be one string, might be a tuple of strings */
    if ( PyString_Check(tuple)) {
	str = PyString_AsString(tuple);
return( FlagsFromString(str,flags));
    } else if ( PySequence_Check(tuple)) {
	ret = 0;
	for ( i=0; i<PySequence_Size(tuple); ++i ) {
	    obj = PySequence_GetItem(tuple,i);
	    if ( obj==Py_None )
	continue;
	    if ( !PyString_Check(obj)) {
		PyErr_Format(PyExc_TypeError, "Bad flag tuple, must be strings");
return( 0x80000000 );
	    }
	    str = PyString_AsString(obj);
	    temp = FlagsFromString(str,flags);
	    if ( temp==0x80000000 )
return( 0x80000000 );
	    ret |= temp;
	}
return( ret );
    } else {
	PyErr_Format(PyExc_TypeError, "Bad flag tuple, must be a tuple of strings (or a string)");
return( 0x80000000 );
    }
}

static PyObject *PyFF_ValToObject(Val *val) {
    if ( val->type==v_int || val->type==v_unicode )
return( Py_BuildValue("i", val->u.ival ));
    else if ( val->type==v_str )
return( Py_BuildValue("s", val->u.sval ));
    else if ( val->type==v_real )
return( Py_BuildValue("d", val->u.fval ));
    else if ( val->type==v_arr || val->type==v_arrfree )
	PyErr_SetString(PyExc_NotImplementedError, "Array -> tuple conversion not yet implemented. I didn't think I needed to.");
return( NULL );
}

static PyObject *PyFV_From_FV(FontView *fv) {
    if ( fv->python_fv_object==NULL ) {
	fv->python_fv_object = PyFF_FontType.tp_alloc(&PyFF_FontType,0);
	((PyFF_Font *) (fv->python_fv_object))->fv = fv;
	Py_INCREF( (PyObject *) (fv->python_fv_object) );	/* for the pointer in my fv */
    }
return( fv->python_fv_object );
}

static PyObject *PyFV_From_FV_I(FontView *fv) {
    PyObject *f = PyFV_From_FV(fv);
    Py_INCREF(f);
return( f );
}

static PyObject *PySC_From_SC(SplineChar *sc) {
    if ( sc->python_sc_object==NULL ) {
	sc->python_sc_object = PyFF_GlyphType.tp_alloc(&PyFF_GlyphType,0);
	((PyFF_Glyph *) (sc->python_sc_object))->sc = sc;
	Py_INCREF( (PyObject *) (sc->python_sc_object) );	/* for the pointer in my fv */
    }
return( sc->python_sc_object );
}

static PyObject *PySC_From_SC_I(SplineChar *sc) {
    PyObject *s = PySC_From_SC(sc);
    Py_INCREF(s);
return( s );
}

static int PyFF_cant_set(PyFF_Font *self,PyObject *value, void *closure) {
    PyErr_Format(PyExc_TypeError, "Cannot set this member");
return( 0 );
}

/* ************************************************************************** */
/* FontForge methods */
/* ************************************************************************** */
static PyObject *PyFF_GetPrefs(PyObject *self, PyObject *args) {
    const char *prefname;
    Val val;

    /* Pref names are ascii so no need to worry about encoding */
    if ( !PyArg_ParseTuple(args,"s",&prefname) )
return( NULL );
    memset(&val,0,sizeof(val));

    if ( !GetPrefs((char *) prefname,&val)) {
	PyErr_Format(PyExc_NameError, "Unknown preference item in GetPrefs: %s", prefname );
return( NULL );
    }
return( PyFF_ValToObject(&val));
}

static PyObject *PyFF_SetPrefs(PyObject *self, PyObject *args) {
    const char *prefname;
    Val val;
    double d;

    memset(&val,0,sizeof(val));
    /* Pref names are ascii so no need to worry about encoding */
    if ( PyArg_ParseTuple(args,"si",&prefname,&val.u.ival) )
	val.type = v_int;
    else {
	PyErr_Clear();
	if ( PyArg_ParseTuple(args,"ses",&prefname,"UTF-8", &val.u.sval) )
	    val.type = v_str;
	else {
	    PyErr_Clear();
	    if ( PyArg_ParseTuple(args,"sd",&d) ) {
		val.u.fval = d;
		val.type = v_real;
	    } else
return( NULL );
	}
    }

    if ( !SetPrefs((char *) prefname,&val,NULL)) {
	PyErr_Format(PyExc_NameError, "Unknown preference item in SetPrefs: %s", prefname );
return( NULL );
    }
Py_RETURN_NONE;
}

static PyObject *PyFF_SavePrefs(PyObject *self, PyObject *args) {

    SavePrefs();
Py_RETURN_NONE;
}

static PyObject *PyFF_LoadPrefs(PyObject *self, PyObject *args) {

    LoadPrefs();
Py_RETURN_NONE;
}

static PyObject *PyFF_DefaultOtherSubrs(PyObject *self, PyObject *args) {

    DefaultOtherSubrs();
Py_RETURN_NONE;
}

static PyObject *PyFF_ReadOtherSubrsFile(PyObject *self, PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

    if ( ReadOtherSubrsFile((char *) filename)<=0 ) {
	PyErr_Format(PyExc_ImportError, "Could not find OtherSubrs file %s",  filename);
return( NULL );
    }

Py_RETURN_NONE;
}

static PyObject *PyFF_LoadEncodingFile(PyObject *self, PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

    ParseEncodingFile((char *) filename);

Py_RETURN_NONE;
}

static PyObject *PyFF_LoadNamelist(PyObject *self, PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

    LoadNamelist((char *) filename);

Py_RETURN_NONE;
}

static PyObject *PyFF_LoadNamelistDir(PyObject *self, PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

    LoadNamelistDir((char *) filename);

Py_RETURN_NONE;
}


static PyObject *PyFF_LoadPlugin(PyObject *self, PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

#if !defined(NOPLUGIN)
    LoadPlugin((char *) filename);
#endif

Py_RETURN_NONE;
}

static PyObject *PyFF_LoadPluginDir(PyObject *self, PyObject *args) {
    const char *filename;

    /* here we do want the default encoding */
    if ( !PyArg_ParseTuple(args,"s", &filename) )
return( NULL );

#if !defined(NOPLUGIN)
    LoadPluginDir((char *) filename);
#endif

Py_RETURN_NONE;
}

static PyObject *PyFF_PreloadCidmap(PyObject *self, PyObject *args) {
    const char *filename, *reg, *order;
    int supplement;

    /* here we do want the default encoding for the filename, the others should be ascii */
    if ( !PyArg_ParseTuple(args,"sssi", &filename, &reg, &order, &supplement) )
return( NULL );

    LoadMapFromFile((char *) filename, (char *) reg, (char *) order, supplement);

Py_RETURN_NONE;
}

static PyObject *PyFF_UnicodeFromName(PyObject *self, PyObject *args) {
    char *name;
    PyObject *ret;

    if ( !PyArg_ParseTuple(args,"es", "UTF-8", &name ))
return( NULL );

    ret = Py_BuildValue("i", UniFromName((char *) name, ui_none,&custom));
    free(name);
return( ret );
}

static PyObject *PyFF_Version(PyObject *self, PyObject *args) {
    extern const char *source_version_str;

return( Py_BuildValue("s", source_version_str ));
}

static PyObject *PyFF_FontTuple(PyObject *self, PyObject *args) {
    extern FontView *fv_list;
    FontView *fv;
    int cnt;
    PyObject *tuple;

    for ( fv=fv_list, cnt=0; fv!=NULL; fv=fv->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( fv=fv_list, cnt=0; fv!=NULL; fv=fv->next, ++cnt )
	PyTuple_SET_ITEM(tuple,cnt,PyFV_From_FV_I(fv));

return( tuple );
}

static PyObject *PyFF_ActiveFont(PyObject *self, PyObject *args) {

    if ( fv_active_in_ui==NULL )
Py_RETURN_NONE;

return( PyFV_From_FV_I( fv_active_in_ui ));
}

static FontView *FVAppend(FontView *fv) {
    /* Normally fontviews get added to the fv list when their windows are */
    /*  created. but we don't create any windows here, so... */
    FontView *test;

    if ( fv_list==NULL ) fv_list = fv;
    else {
	for ( test = fv_list; test->next!=NULL; test=test->next );
	test->next = fv;
    }
return( fv );
}

static FontView *SFAdd(SplineFont *sf) {
    if ( sf->fv!=NULL )
	/* All done */;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    else if ( !no_windowing_ui )
	FontViewCreate(sf);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    else
	FVAppend(_FontViewCreate(sf));
return( sf->fv );
}

static PyObject *PyFF_OpenFont(PyObject *self, PyObject *args) {
    char *filename, *locfilename;
    int openflags = 0;
    SplineFont *sf;

    if ( !PyArg_ParseTuple(args,"es|i", "UTF-8", &filename, &openflags ))
return( NULL );
    locfilename = utf82def_copy(filename);
    sf = LoadSplineFont(locfilename,openflags);
    free(filename); free(locfilename);
    if ( sf==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "Open failed");
return( NULL );
    }
return( PyFV_From_FV_I( SFAdd( sf )));
}

static PyObject *PyFF_FontsInFile(PyObject *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    PyObject *tuple;
    char **ret;
    int cnt, i;

    if ( !PyArg_ParseTuple(args,"es","UTF-8",&filename) )
return( NULL );
    locfilename = utf82def_copy(filename);
    free(filename);
    ret = GetFontNames(locfilename);
    free(locfilename);
    cnt = 0;
    if ( ret!=NULL ) for ( cnt=0; ret[cnt]!=NULL; ++cnt );

    tuple = PyTuple_New(cnt);
    for ( i=0; i<cnt; ++i )
	PyTuple_SetItem(tuple,i,Py_BuildValue("s",ret[i]));
return( tuple );
}

static PyObject *PyFF_ParseTTFInstrs(PyObject *self, PyObject *args) {
    PyObject *binstr;
    char *instr_str;
    int icnt;
    uint8 *instrs;

    if ( !PyArg_ParseTuple(args,"s",&instr_str) )
return( NULL );
    instrs = _IVParse(NULL,instr_str,&icnt);
    if ( instrs==NULL ) {
	PyErr_Format(PyExc_TypeError, "Failed to parse instructions" );
return( NULL );
    }
    binstr = PyString_FromStringAndSize((char *) instrs,icnt);
    free(instrs);

return( binstr );
}

static PyObject *PyFF_UnParseTTFInstrs(PyObject *self, PyObject *args) {
    PyObject *tuple, *ret;
    int icnt, i;
    uint8 *instrs;
    char *as_str;

    if ( !PyArg_ParseTuple(args,"O",&tuple) )
return( NULL );
    if ( !PySequence_Check(tuple)) {
	PyErr_Format(PyExc_TypeError, "Argument must be a sequence" );
return( NULL );
    }
    if ( PyString_Check(tuple)) {
	char *space; int len;
	PyString_AsStringAndSize(tuple,&space,&len);
	instrs = gcalloc(len,sizeof(uint8));
	icnt = len;
	memcpy(instrs,space,len);
    } else {
	icnt = PySequence_Size(tuple);
	instrs = galloc(icnt);
	for ( i=0; i<icnt; ++i ) {
	    instrs[i] = PyInt_AsLong(PySequence_GetItem(tuple,i));
	    if ( PyErr_Occurred())
return( NULL );
	}
    }
    as_str = _IVUnParseInstrs(instrs,icnt);
    ret = PyString_FromString(as_str);
    free(as_str); free(instrs);
return( ret );
}

static struct flaglist printmethod[] = {
    { "lp", 0 },
    { "lpr", 1 },
    { "ghostview", 2 },
    { "ps-file", 3 },
    { "command", 4 },
    { "pdf-file", 5 },
    NULL };

static PyObject *PyFF_printSetup(PyObject *self, PyObject *args) {
    char *ptype, *pcmd = NULL;
    int iptype;

    if ( !PyArg_ParseTuple(args,"s|sii", &ptype, &pcmd, &pagewidth, &pageheight ) )
return( NULL );
    iptype = FlagsFromString(ptype,printmethod);
    if ( iptype==0x80000000 ) {
	PyErr_Format(PyExc_TypeError, "Unknown printing method" );
return( NULL );
    }

    printtype = iptype;
    if ( pcmd!=NULL && iptype==4 )
	printcommand = copy(pcmd);
    else if ( pcmd!=NULL && (iptype==0 || iptype==1) )
	printlazyprinter = copy(pcmd);
Py_RETURN(self);
}

/* ************************************************************************** */
/* Points */
/* ************************************************************************** */
static void PyFF_TransformPoint(PyFF_Point *self, double transform[6]) {
    double x,y;

    x = transform[0]*self->x + transform[2]*self->y + transform[4];
    y = transform[1]*self->x + transform[3]*self->y + transform[5];
    self->x = rint(1024*x)/1024;
    self->y = rint(1024*y)/1024;
}

static PyObject *PyFFPoint_Transform(PyFF_Point *self, PyObject *args) {
    double m[6];

    if ( !PyArg_ParseTuple(args,"(dddddd)",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( NULL );
    PyFF_TransformPoint(self,m);
    Py_INCREF((PyObject *) self);
return( (PyObject *) self );
}

static int PyFFPoint_compare(PyFF_Point *self,PyObject *other) {
    double x, y;

    /* I'd like to accept general sequences but there is no PyArg_ParseSequence*/
    if ( PyTuple_Check(other) && PyTuple_Size(other)==2 ) {
	if ( !PyArg_ParseTuple(other,"dd", &x, &y ))
return( -1 );
    } else if ( PyType_IsSubtype(&PyFF_PointType,other->ob_type) ) {
	x = ((PyFF_Point *) other)->x;
	y = ((PyFF_Point *) other)->y;
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( -1 );
    }
    if ( RealNear(self->x,x) ) {
	if ( RealNear(self->y,y))
return( 0 );
	else if ( self->y>y )
return( 1 );
    } else if ( self->x>x )
return( 1 );

return( -1 );
}

static PyObject *PyFFPoint_Repr(PyFF_Point *self) {
    char buffer[200];

    sprintf(buffer,"fontforge.point(%g,%g,%s)", self->x, self->y, self->on_curve?"True":"False" );
return( PyString_FromString(buffer));
}

static PyObject *PyFFPoint_Str(PyFF_Point *self) {
    char buffer[200];

    sprintf(buffer,"<FFPoint (%g,%g) %s>", self->x, self->y, self->on_curve?"on":"off" );
return( PyString_FromString(buffer));
}

static PyMemberDef FFPoint_members[] = {
    {"x", T_FLOAT, offsetof(PyFF_Point, x), 0,
     "x coordinate"},
    {"y", T_FLOAT, offsetof(PyFF_Point, y), 0,
     "y coordinate"},
    {"on_curve", T_INT, offsetof(PyFF_Point, on_curve), 0,
     "whether this point lies on the curve or is a control point"},
    {NULL}  /* Sentinel */
};

static PyMethodDef FFPoint_methods[] = {
    {"transform", (PyCFunction)PyFFPoint_Transform, METH_VARARGS,
	     "Transforms the point by the transformation matrix (a 6 element tuple of reals)" },
    NULL
};

static PyObject *PyFFPoint_New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyFF_Point *self;

    self = (PyFF_Point *)type->tp_alloc(type, 0);
    if ( self!=NULL ) {
	self->x = self->y = 0;
	self->on_curve = 1;
	if ( args!=NULL && !PyArg_ParseTuple(args, "|ffi",
		    &self->x, &self->y, &self->on_curve))
return( NULL );
    }

    return (PyObject *)self;
}

static PyTypeObject PyFF_PointType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.point",         /*tp_name*/
    sizeof(PyFF_Point),		/*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    (cmpfunc) PyFFPoint_compare, /*tp_compare*/
    (reprfunc) PyFFPoint_Repr, /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFPoint_Str,  /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "fontforge Point objects", /* tp_doc */
    0                       ,  /* tp_traverse */
    0,			       /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FFPoint_methods,           /* tp_methods */
    FFPoint_members,           /* tp_members */
    0,		               /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,			       /* tp_init */
    0,                         /* tp_alloc */
    PyFFPoint_New	       /* tp_new */
};

static PyFF_Point *PyFFPoint_CNew(double x, double y, int on_curve) {
    /* Convenience routine for creating a new point from C */
    PyFF_Point *self = (PyFF_Point *) PyFFPoint_New(&PyFF_PointType,NULL,NULL);
    self->x = x;
    self->y = y;
    self->on_curve = on_curve;
return( self );
}

/* ************************************************************************** */
/* Contour iterator type */
/* ************************************************************************** */

typedef struct {
    PyObject_HEAD
    int pos;
    PyFF_Contour *contour;
} contouriterobject;
static PyTypeObject PyFF_ContourIterType;

static PyObject *contouriter_new(PyObject *contour) {
    contouriterobject *ci;
    ci = PyObject_New(contouriterobject, &PyFF_ContourIterType);
    if (ci == NULL)
return NULL;
    ci->contour = ((PyFF_Contour *) contour);
    Py_INCREF(contour);
    ci->pos = 0;
return (PyObject *)ci;
}

static void contouriter_dealloc(contouriterobject *ci) {
    Py_XDECREF(ci->contour);
    PyObject_Del(ci);
}

static PyObject *contouriter_iternext(contouriterobject *ci) {
    PyFF_Contour *contour = ci->contour;
    PyObject *pt;

    if ( contour == NULL)
return NULL;

    if ( ci->pos<contour->pt_cnt ) {
	pt = (PyObject *) contour->points[ci->pos++];
	Py_INCREF(pt);
return( pt );
    }

return NULL;
}

static PyTypeObject PyFF_ContourIterType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"contouriter",				/* tp_name */
	sizeof(contouriterobject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)contouriter_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)contouriter_iternext,	/* tp_iternext */
	0,					/* tp_methods */
	0,
};

/* ************************************************************************** */
/* Contours */
/* ************************************************************************** */
static int PyFFContour_clear(PyFF_Contour *self) {
    int i;

    for ( i=0; i<self->pt_cnt; ++i )
	Py_DECREF(self->points[i]);
    self->pt_cnt = 0;

return 0;
}

static void PyFFContour_dealloc(PyFF_Contour *self) {
    PyFFContour_clear(self);
    PyMem_Del(self->points);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *PyFFContour_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyFF_Contour *self;

    self = (PyFF_Contour *)type->tp_alloc(type, 0);
    if ( self!=NULL ) {
	self->points = NULL;
	self->pt_cnt = self->pt_max = 0;
	self->is_quadratic = self->closed = 0;
    }

return (PyObject *)self;
}

static int PyFFContour_init(PyFF_Contour *self, PyObject *args, PyObject *kwds) {
    int quad=0;

    if ( args!=NULL && !PyArg_ParseTuple(args, "|i", &quad))
return -1;

    self->is_quadratic = (quad!=0);
return 0;
}

static PyObject *PyFFContour_Str(PyFF_Contour *self) {
    char *buffer, *pt;
    int i;
    PyObject *ret;

    buffer=pt=galloc(self->pt_cnt*30+30);
    strcpy(buffer, self->is_quadratic? "<Contour(quadratic)\n":"<Contour(cubic)\n");
    pt = buffer+strlen(buffer);
    for ( i=0; i<self->pt_cnt; ++i ) {
	sprintf( pt, "  (%g,%g) %s\n", self->points[i]->x, self->points[i]->y,
		self->points[i]->on_curve ? "on" : "off" );
	pt += strlen( pt );
    }
    strcpy(pt,">");
    ret = PyString_FromString( buffer );
    free( buffer );
return( ret );
}

static int PyFFContour_docompare(PyFF_Contour *self,PyObject *other,
	double pt_err, double spline_err) {
    SplineSet *ss, *ss2;
    int ret;

    if ( !PyType_IsSubtype(&PyFF_ContourType,other->ob_type) ) {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( -1 );
    }
    ss = SSFromContour(self,NULL);
    ss2 = SSFromContour((PyFF_Contour *) other,NULL);
    ret = SSsCompare(ss,ss2,pt_err,spline_err,NULL);
    SplinePointListFree(ss);
    SplinePointListFree(ss2);
return(ret);
}

static int PyFFContour_compare(PyFF_Contour *self,PyObject *other) {
    const double pt_err = .5, spline_err = 1;
    int i,ret;

    ret = PyFFContour_docompare(self,other,pt_err,spline_err);
    if ( !(ret&SS_NoMatch) )
return( 0 );
    /* There's no real ordering on this guys. Make up something that is */
    /*  at least consistent */
    if ( self->pt_cnt < ((PyFF_Contour *) other)->pt_cnt )
return( -1 );
    else if ( self->pt_cnt > ((PyFF_Contour *) other)->pt_cnt )
return( 1 );
    /* If there's a difference, then there must be at least one point to be */
    /*  different. And we only get here if there were a difference */
    for ( i=0; i<self->pt_cnt; ++i ) {
	ret = PyFFPoint_compare(self->points[i],(PyObject *) (((PyFF_Contour *) other)->points[i]) );
	if ( ret!=0 )
return( ret );
    }
return( -1 );		/* Arbetrary... but we can't get here=>all points same */
}

/* ************************************************************************** */
/* Contour getters/setters */
/* ************************************************************************** */
static PyObject *PyFF_Contour_get_is_quadratic(PyFF_Contour *self,void *closure) {
return( Py_BuildValue("i", self->is_quadratic ));
}

static int PyFF_Contour_set_is_quadratic(PyFF_Contour *self,PyObject *value,void *closure) {
    int val;
    SplineSet *ss, *ss2;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );

    val = (val!=0);
    if ( val == self->is_quadratic )
return( 0 );
    if ( self->pt_cnt!=0 ) {
	ss = SSFromContour(self,NULL);
	PyFFContour_clear(self);
	if ( val )
	    ss2 = SplineSetsTTFApprox(ss);
	else
	    ss2 = SplineSetsPSApprox(ss);
	SplinePointListFree(ss);
	ContourFromSS(ss2,self);
	SplinePointListFree(ss2);
    }
    self->is_quadratic = (val!=0);
return( 0 );
}

static PyObject *PyFF_Contour_get_closed(PyFF_Contour *self,void *closure) {
return( Py_BuildValue("i", self->closed ));
}

static int PyFF_Contour_set_closed(PyFF_Contour *self,PyObject *value,void *closure) {
    int val;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );

    val = (val!=0);
    if ( val == self->closed )
return( 0 );
    if ( !val ) {
	self->closed = false;
	if ( self->pt_cnt>1 && self->points[0]->on_curve )
	    self->points[self->pt_cnt++] = PyFFPoint_CNew(self->points[0]->x,self->points[0]->y,true);
    } else {
	self->closed = true;
	if ( self->pt_cnt>1 && self->points[0]->on_curve &&
		self->points[self->pt_cnt-1]->on_curve &&
		self->points[0]->x == self->points[self->pt_cnt-1]->x &&
		self->points[0]->y == self->points[self->pt_cnt-1]->y ) {
	    --self->pt_cnt;
	    Py_DECREF(self->points[self->pt_cnt]);
	}
    }
return( 0 );
}

static PyGetSetDef PyFFContour_getset[] = {
    {"is_quadratic",
	 (getter)PyFF_Contour_get_is_quadratic, (setter)PyFF_Contour_set_is_quadratic,
	 "Whether this is an quadratic or cubic contour", NULL},
    {"closed",
	 (getter)PyFF_Contour_get_closed, (setter)PyFF_Contour_set_closed,
	 "Whether this is a closed contour", NULL},
    { NULL }
};

/* ************************************************************************** */
/* Contour sequence */
/* ************************************************************************** */
static int PyFFContour_Length( PyObject *self ) {
return( ((PyFF_Contour *) self)->pt_cnt );
}

static PyObject *PyFFContour_Concat( PyObject *_c1, PyObject *_c2 ) {
    PyFF_Contour *c1 = (PyFF_Contour *) _c1, *c2 = (PyFF_Contour *) _c2;
    PyFF_Contour *self;
    int i;
    PyFF_Contour dummy;
    PyFF_Point *dummies[1];
    double x,y;

    if ( PyType_IsSubtype(&PyFF_PointType,c2->ob_type) ) {
	memset(&dummy,0,sizeof(dummy));
	dummy.pt_cnt = 1;
	dummy.points = dummies; dummies[0] = (PyFF_Point *) _c2;
	c2 = &dummy;
    } else if ( !PyType_IsSubtype(&PyFF_ContourType,c1->ob_type) ||
	    !PyType_IsSubtype(&PyFF_ContourType,c2->ob_type) ||
	    c1->is_quadratic != c2->is_quadratic ) {
	if ( PyTuple_Check(_c2) && PyArg_ParseTuple(_c2,"dd",&x,&y)) {
	    PyFF_Point *pt = PyFFPoint_CNew(x,y,true);
	    memset(&dummy,0,sizeof(dummy));
	    dummy.pt_cnt = 1;
	    dummy.points = dummies; dummies[0] = pt;
	    c2 = &dummy;
	} else {
	    PyErr_Format(PyExc_TypeError, "Both arguments must be Contours of the same order");
return( NULL );
	}
    }
    self = (PyFF_Contour *)PyFF_ContourType.tp_alloc(&PyFF_ContourType, 0);
    self->is_quadratic = c1->is_quadratic;
    self->closed = c1->closed;
    self->pt_max = self->pt_cnt = c1->pt_cnt + c2->pt_cnt;
    self->points = PyMem_New(PyFF_Point *,self->pt_max);
    for ( i=0; i<c1->pt_cnt; ++i ) {
	Py_INCREF(c1->points[i]);
	self->points[i] = c1->points[i];
    }
    for ( i=0; i<c2->pt_cnt; ++i ) {
	Py_INCREF(c2->points[i]);
	self->points[c1->pt_cnt+i] = c2->points[i];
    }
Py_RETURN( (PyObject *) self );
}

static PyObject *PyFFContour_InPlaceConcat( PyObject *_self, PyObject *_c2 ) {
    PyFF_Contour *self = (PyFF_Contour *) _self, *c2 = (PyFF_Contour *) _c2;
    int i, old_cnt;
    PyFF_Contour dummy;
    PyFF_Point *dummies[1];
    double x,y;

    if ( PyType_IsSubtype(&PyFF_PointType,c2->ob_type) ) {
	memset(&dummy,0,sizeof(dummy));
	dummy.pt_cnt = 1;
	dummy.points = dummies; dummies[0] = (PyFF_Point *) _c2;
	c2 = &dummy;
    } else if ( !PyType_IsSubtype(&PyFF_ContourType,self->ob_type) ||
	    !PyType_IsSubtype(&PyFF_ContourType,c2->ob_type) ||
	    self->is_quadratic != c2->is_quadratic ) {
	if ( PyTuple_Check(_c2) && PyArg_ParseTuple(_c2,"dd",&x,&y)) {
	    PyFF_Point *pt = PyFFPoint_CNew(x,y,true);
	    memset(&dummy,0,sizeof(dummy));
	    dummy.pt_cnt = 1;
	    dummy.points = dummies; dummies[0] = pt;
	    c2 = &dummy;
	} else {
	    PyErr_Format(PyExc_TypeError, "Both arguments must be Contours of the same order");
return( NULL );
	}
    }
    old_cnt = self->pt_cnt;
    self->pt_max = self->pt_cnt = self->pt_cnt + c2->pt_cnt;
    self->points = PyMem_Resize(self->points,PyFF_Point *,self->pt_max);
    for ( i=0; i<c2->pt_cnt; ++i ) {
	Py_INCREF(c2->points[i]);
	self->points[old_cnt+i] = c2->points[i];
    }
Py_RETURN( self );
}

static PyObject *PyFFContour_Index( PyObject *self, int pos ) {
    PyFF_Contour *cont = (PyFF_Contour *) self;
    PyObject *ret;

    if ( pos<0 || pos>=cont->pt_cnt ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( NULL );
    }
    ret = (PyObject *) cont->points[pos];
    Py_INCREF(ret);
return( ret );
}

static int PyFFContour_IndexAssign( PyObject *self, int pos, PyObject *val ) {
    PyFF_Contour *cont = (PyFF_Contour *) self;
    PyObject *old;

    if ( !PyType_IsSubtype(&PyFF_PointType,val->ob_type) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Point");
return( -1 );
    }
    if ( pos<0 || pos>=cont->pt_cnt ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( -1 );
    }
    if ( cont->points[pos]->on_curve != ((PyFF_Point *) val)->on_curve &&
	    !cont->is_quadratic ) {
	PyErr_Format(PyExc_TypeError, "Replacement point must have the same on_curve setting as original in a cubic contour");
return( -1 );
    }

    old = (PyObject *) cont->points[pos];
    cont->points[pos] = (PyFF_Point *) val;
    Py_DECREF( old );
return( 0 );
}

static PyObject *PyFFContour_Slice( PyObject *self, int start, int end ) {
    PyFF_Contour *cont = (PyFF_Contour *) self;
    PyFF_Contour *ret;
    int len, i;

    if ( start<0 || end <0 || end>=cont->pt_cnt || start>=cont->pt_cnt ) {
	PyErr_Format(PyExc_ValueError, "Slice specification out of range" );
return( NULL );
    }

    if ( end<start )
	len = end + (cont->pt_cnt-1 - start) + 1;
    else
	len = end-start + 1;

    ret = (PyFF_Contour *)PyFF_ContourType.tp_alloc(&PyFF_ContourType, 0);
    ret->is_quadratic = cont->is_quadratic;
    ret->closed = false;
    ret->pt_max = ret->pt_cnt = len;
    ret->points = PyMem_New(PyFF_Point *,ret->pt_max);

    if ( end<start ) {
	for ( i=start; i<cont->pt_cnt; ++i )
	    ret->points[i-start] = cont->points[i];
	for ( i=0; i<=end; ++i )
	    ret->points[(cont->pt_cnt-start)+i] = cont->points[i];
    } else {
	for ( i=start; i<=end; ++i )
	    ret->points[i-start] = cont->points[i];
    }
    for ( i=0; i<ret->pt_cnt; ++i )
	Py_INCREF(ret->points[i]);
return( (PyObject *) ret );
}

static int PyFFContour_SliceAssign( PyObject *_self, int start, int end, PyObject *_rpl ) {
    PyFF_Contour *self = (PyFF_Contour *) _self, *rpl = (PyFF_Contour *) _rpl;
    int i, diff;

    if ( !PyType_IsSubtype(&PyFF_ContourType,rpl->ob_type) ) {
	PyErr_Format(PyExc_TypeError, "Replacement must be a (FontForge) Contour");
return( -1 );
    }
    if ( end<start ) {
	PyErr_Format(PyExc_ValueError, "Slice specification out of order" );
return( -1 );
    } else if ( start<0 || end>=self->pt_cnt ) {
	PyErr_Format(PyExc_ValueError, "Slice specification out of range" );
return( -1 );
    }

    diff = rpl->pt_cnt - ( end-start+1 );
    for ( i=start; i<=end; ++i )
	Py_DECREF(self->points[i]);
    if ( diff>0 ) {
	if ( self->pt_cnt+diff >= self->pt_max ) {
	    self->pt_max = self->pt_cnt + diff;
	    self->points = PyMem_Resize(self->points,PyFF_Point *,self->pt_max);
	}
	for ( i=self->pt_cnt-1; i>end; --i )
	    self->points[i+diff] = self->points[i];
    } else if ( diff<0 ) {
	for ( i=end+1; i<self->pt_cnt; ++i )
	    self->points[i+diff] = self->points[i];
    }
    self->pt_cnt += diff;
    for ( i=0; i<=rpl->pt_cnt; ++i ) {
	self->points[i+start] = rpl->points[i];
	Py_INCREF(rpl->points[i]);
    }
return( 0 );
}

static int PyFFContour_Contains(PyObject *_self, PyObject *_pt) {
    PyFF_Contour *self = (PyFF_Contour *) _self;
    float x,y;
    int i;

    if ( PySequence_Check(_pt)) {
	if ( !PyArg_ParseTuple(_pt,"ff", &x, &y ));
return( -1 );
    } else if ( !PyType_IsSubtype(&PyFF_PointType,_pt->ob_type) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Point");
return( -1 );
    } else {
	x = ((PyFF_Point *) _pt)->x;
	y = ((PyFF_Point *) _pt)->y;
    }

    for ( i=0; i<self->pt_cnt; ++i )
	if ( self->points[i]->x == x && self->points[i]->y == y )
return( 1 );

return( 0 );
}

static PySequenceMethods PyFFContour_Sequence = {
    PyFFContour_Length,		/* length */
    PyFFContour_Concat,		/* concat */
    NULL,			/* repeat */
    PyFFContour_Index,		/* subscript */
    PyFFContour_Slice,		/* slice */
    PyFFContour_IndexAssign,	/* subscript assign */
    PyFFContour_SliceAssign,	/* slice assign */
    PyFFContour_Contains,	/* contains */
    PyFFContour_InPlaceConcat	/* inplace_concat */
};

/* ************************************************************************** */
/* Contour methods */
/* ************************************************************************** */
static PyObject *PyFFContour_IsEmpty(PyFF_Contour *self) {
    /* Arg checking done elsewhere */
return( Py_BuildValue("i",self->pt_cnt==0 ) );
}

static PyObject *PyFFContour_Start(PyFF_Contour *self, PyObject *args) {
    double x,y;

    if ( self->pt_cnt!=0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour not empty");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "dd", &x, &y ))
return( NULL );
    if ( 1>self->pt_max )
	self->points = PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    self->points[0] = PyFFPoint_CNew(x,y,true);
    self->pt_cnt = 1;

Py_RETURN( self );
}

static PyObject *PyFFContour_LineTo(PyFF_Contour *self, PyObject *args) {
    double x,y;
    int pos = -1, i;

    if ( self->pt_cnt==0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour empty");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "dd|i", &x, &y, &pos )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "(dd)|i", &x, &y, &pos ))
return( NULL );
    }
    if ( pos<0 || pos>=self->pt_cnt-1 )
	pos = self->pt_cnt-1;
    while ( pos>=0 && !self->points[pos]->on_curve )
	--pos;
    if ( pos<0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour contains no on-curve points");
return( NULL );
    }
    if ( self->pt_cnt >= self->pt_max )
	self->points = PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    for ( i=self->pt_cnt-1; i>pos; --i )
	self->points[i+1] = self->points[i];
    self->points[pos+1] = PyFFPoint_CNew(x,y,true);

Py_RETURN( self );
}

static PyObject *PyFFContour_CubicTo(PyFF_Contour *self, PyObject *args) {
    double x[3],y[3];
    PyFF_Point *np, *pp, *p;
    int pos=-1, i;

    if ( self->is_quadratic || self->pt_cnt==0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour quadratic, or empty");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "(dd)(dd)(dd)|i", &x[0], &y[0], &x[1], &y[1], &x[2], &y[2], &pos )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "dddddd|i", &x[0], &y[0], &x[1], &y[1], &x[2], &y[2], &pos ))
return( NULL );
    }
    np = PyFFPoint_CNew(x[0],y[0],false);
    pp = PyFFPoint_CNew(x[1],y[1],false);
    p = PyFFPoint_CNew(x[2],y[2],true);
    if ( p==NULL ) {
	Py_XDECREF(pp);
	Py_XDECREF(np);
return( NULL );
    }

    if ( pos<0 || pos>=self->pt_cnt-1 )
	pos = self->pt_cnt-1;
    while ( pos>=0 && !self->points[pos]->on_curve )
	--pos;
    if ( pos<0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour contains no on-curve points");
return( NULL );
    }
    if ( self->pt_cnt+3 >= self->pt_max )
	self->points = PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    for ( i=self->pt_cnt-1; i>pos; --i )
	self->points[i+3] = self->points[i];
    self->points[pos+1] = np;
    self->points[pos+2] = pp;
    self->points[pos+3] = p;
Py_RETURN( self );
}


static PyObject *PyFFContour_QuadraticTo(PyFF_Contour *self, PyObject *args) {
    double x[2],y[2];
    PyFF_Point *cp, *p;
    int pos=-1, i;

    if ( !self->is_quadratic || self->pt_cnt==0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour cubic, or empty");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "(dd)(dd)|i", &x[0], &y[0], &x[1], &y[1], &pos )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "dddd|i", &x[0], &y[0], &x[1], &y[1], &pos ))
return( NULL );
    }
    cp = PyFFPoint_CNew(x[0],y[0],false);
    p = PyFFPoint_CNew(x[1],y[1],true);
    if ( p==NULL ) {
	Py_XDECREF(cp);
return( NULL );
    }

    if ( pos<0 || pos>=self->pt_cnt-1 )
	pos = self->pt_cnt-1;
    while ( pos>=0 && !self->points[pos]->on_curve )
	--pos;
    if ( pos<0 ) {
	PyErr_SetString(PyExc_AttributeError, "Contour contains no on-curve points");
return( NULL );
    }
    if ( self->pt_cnt+2 >= self->pt_max )
	self->points = PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    for ( i=self->pt_cnt-1; i>pos; --i )
	self->points[i+2] = self->points[i];
    self->points[pos+1] = cp;
    self->points[pos+2] = p;
Py_RETURN( self );
}

static PyObject *PyFFContour_InsertPoint(PyFF_Contour *self, PyObject *args) {
    double x,y;
    PyFF_Point *p=NULL;
    int pos = -1, i;
    int on = true;

    if ( !PyArg_ParseTuple( args, "(ddi)|i", &x, &y, &on, &pos )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "(dd)|ii", &x, &y, &on, &pos )) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTuple( args, "dd|ii", &x, &y, &on, &pos )) {
		PyErr_Clear();
		if ( !PyArg_ParseTuple( args, "O|i", &p, &pos ) ||
			!PyType_IsSubtype(&PyFF_PointType,p->ob_type))
return( NULL );
	    }
	}
    }

    if ( pos<0 || pos>=self->pt_cnt-1 )
	pos = self->pt_cnt-1;
    if ( self->pt_cnt >= self->pt_max )
	self->points = PyMem_Resize(self->points,PyFF_Point *,self->pt_max += 10);
    for ( i=self->pt_cnt-1; i>pos; --i )
	self->points[i+1] = self->points[i];
    if ( p==NULL )
	self->points[pos+1] = PyFFPoint_CNew(x,y,on);
    else {
	self->points[pos+1] = p;
	Py_INCREF( (PyObject *) p);
    }

Py_RETURN( self );
}

static PyObject *PyFFContour_MakeFirst(PyFF_Contour *self, PyObject *args) {
    int pos = -1, off;
    int i;
    PyFF_Point **temp, **old;

    if ( !PyArg_ParseTuple( args, "i", &pos ))
return( NULL );

    temp = PyMem_New(PyFF_Point *,self->pt_max);
    old = self->points;
    for ( i=pos; i<self->pt_cnt; ++i )
	temp[i-pos] = old[i];
    off = i;
    for ( i=0; i<pos; ++i )
	temp[i+off] = old[i];
    self->points = temp;
    PyMem_Del(old);

Py_RETURN( self );
}

static PyObject *PyFFContour_ReverseDirection(PyFF_Contour *self, PyObject *args) {
    int i, j;
    PyFF_Point **temp, **old;

    /* Arg checking done elsewhere */

    temp = PyMem_New(PyFF_Point *,self->pt_max);
    old = self->points;
    temp[0] = old[0];
    for ( i=self->pt_cnt-1, j=1; i>0; --i, ++j )
	temp[j] = old[i];
    self->points = temp;
    PyMem_Del(old);

Py_RETURN( self );
}

static PyObject *PyFFContour_IsClockwise(PyFF_Contour *self, PyObject *args) {
    SplineSet *ss;
    int ret;

    /* Arg checking done elsewhere */

    ss = SSFromContour(self,NULL);
    if ( ss==NULL ) {
	PyErr_SetString(PyExc_AttributeError, "Empty Contour");
return( NULL );
    }
    ret = SplinePointListIsClockwise(ss);
    SplinePointListFree(ss);
return( Py_BuildValue("i",ret ) );
}

static PyObject *PyFFContour_Merge(PyFF_Contour *self, PyObject *args) {
    SplineSet *ss;
    int i, pos;

    ss = SSFromContour(self,NULL);
    if ( ss==NULL ) {
	PyErr_SetString(PyExc_AttributeError, "Empty Contour");
return( NULL );
    }
    for ( i=0; i<PySequence_Size(args); ++i ) {
	pos = PyInt_AsLong(PySequence_GetItem(args,i));
	if ( PyErr_Occurred())
return( NULL );
	SSSelectOnCurve(ss,pos);
    }
    SplineCharMerge(NULL,&ss,1);
    if ( ss==NULL ) {
	for ( i=0; i<self->pt_cnt; ++i )
	    Py_DECREF(self->points[i]);
	self->pt_cnt = 0;
    } else {
	ContourFromSS(ss,self);
	SplinePointListFree(ss);
    }
Py_RETURN( self );
}

struct flaglist simplifyflags[] = {
    { "cleanup", -1 },
    { "ignoreslopes", 1 },
    { "ignoreextrema", 2 },
    { "smoothcurves", 4 },
    { "choosehv", 8 },
    { "forcelines", 16 },
    { "nearlyhvlines", 32 },
    { "mergelines", 64 },
    { "setstarttoextremum", 128 },
    { "removesingletonpoints", 256 },
    { NULL }
};

static PyObject *PyFFContour_selfIntersects(PyFF_Contour *self, PyObject *args) {
    SplineSet *ss;
    Spline *s, *s2;
    PyObject *ret;

    ss = SSFromContour(self,NULL);
    ret = SplineSetIntersect(ss,&s,&s2) ? Py_True : Py_False;
    SplinePointListFree(ss);
    Py_INCREF( ret );
return( ret );
}

static PyObject *PyFFContour_Simplify(PyFF_Contour *self, PyObject *args) {
    SplineSet *ss;
    static struct simplifyinfo smpl = { sf_normal,.75,.2,10 };
    int i;

    smpl.err = 1;
    smpl.linefixup = 2;
    smpl.linelenmax = 10;

    ss = SSFromContour(self,NULL);
    if ( ss==NULL )
Py_RETURN( self );		/* As simple as it can be */

    if ( PySequence_Size(args)>=1 )
	smpl.err = PyFloat_AsDouble(PySequence_GetItem(args,0));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=2 )
	smpl.flags = FlagsFromTuple( PySequence_GetItem(args,1),simplifyflags);
    if ( !PyErr_Occurred() && PySequence_Size(args)>=3 )
	smpl.tan_bounds = PyFloat_AsDouble( PySequence_GetItem(args,2));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=4 )
	smpl.linefixup = PyFloat_AsDouble( PySequence_GetItem(args,3));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=5 )
	smpl.linelenmax = PyFloat_AsDouble( PySequence_GetItem(args,4));
    if ( PyErr_Occurred() )
return( NULL );
    SplinePointListSimplify(NULL,ss,&smpl);
    if ( ss==NULL ) {
	for ( i=0; i<self->pt_cnt; ++i )
	    Py_DECREF(self->points[i]);
	self->pt_cnt = 0;
    } else {
	ContourFromSS(ss,self);
	SplinePointListFree(ss);
    }
Py_RETURN( self );
}

static PyObject *PyFFContour_Transform(PyFF_Contour *self, PyObject *args) {
    int i;
    double m[6];

    if ( !PyArg_ParseTuple(args,"(dddddd)",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( NULL );
    for ( i=0; i<self->pt_cnt; ++i )
	PyFF_TransformPoint(self->points[i],m);
Py_RETURN( self );
}

static PyObject *PyFFContour_Round(PyFF_Contour *self, PyObject *args) {
    double factor=1;
    int i;

    if ( !PyArg_ParseTuple(args,"d",&factor ) )
return( NULL );
    for ( i=0; i<self->pt_cnt; ++i ) {
	self->points[i]->x = rint( factor*self->points[i]->x )/factor;
	self->points[i]->y = rint( factor*self->points[i]->y )/factor;
    }
Py_RETURN( self );
}

static PyObject *PyFFContour_Cluster(PyFF_Contour *self, PyObject *args) {
    double within = .1, max = .5;
    SplineChar sc;
    SplineSet *ss;
#ifdef FONTFORGE_CONFIG_TYPE3
    Layer layers[2];
#endif

    if ( !PyArg_ParseTuple(args,"|dd", &within, &max ) )
return( NULL );

    ss = SSFromContour(self,NULL);
    if ( ss==NULL )
Py_RETURN( self );		/* no points=> no clusters */
    memset(&sc,0,sizeof(sc));
#ifdef FONTFORGE_CONFIG_TYPE3
    memset(layers,0,sizeof(layers));
    sc.layers = layers;
#endif
    sc.layers[ly_fore].splines = ss;
    sc.name = "nameless";
    SCRoundToCluster( &sc,ly_fore,false,within,max);
    ContourFromSS(sc.layers[ly_fore].splines,self);
    SplinePointListFree(sc.layers[ly_fore].splines);
Py_RETURN( self );
}

struct flaglist addextremaflags[] = {
    { "all", 0 },
    { "only_good", 2 },
    { "only_good_rm", 3 },
    { NULL }
};

static PyObject *PyFFContour_AddExtrema(PyFF_Contour *self, PyObject *args) {
    int emsize = 1000;
    char *flag = NULL;
    int ae = ae_only_good;
    SplineSet *ss;

    if ( !PyArg_ParseTuple(args,"|si", &flag, &emsize ) )
return( NULL );
    if ( flag!=NULL )
	ae = FlagsFromString(flag,addextremaflags);
    if ( ae==0x80000000 )
return( NULL );

    ss = SSFromContour(self,NULL);
    if ( ss==NULL )
Py_RETURN( self );		/* no points=> nothing to do */
    SplineSetAddExtrema(NULL,ss,ae,emsize);
    ContourFromSS(ss,self);
    SplinePointListFree(ss);
Py_RETURN( self );
}

static PyObject *PyFFContour_BoundingBox(PyFF_Contour *self, PyObject *args) {
    double xmin, xmax, ymin, ymax;
    int i;

    if ( self->pt_cnt==0 )
return( Py_BuildValue("(dddd)", 0,0,0,0 ));

    xmin = xmax = self->points[0]->x;
    ymin = ymax = self->points[0]->y;
    for ( i=1; i<self->pt_cnt; ++i ) {
	if ( self->points[i]->x < xmin ) xmin = self->points[i]->x;
	if ( self->points[i]->x > xmax ) xmax = self->points[i]->x;
	if ( self->points[i]->y < ymin ) ymin = self->points[i]->y;
	if ( self->points[i]->y > ymax ) ymax = self->points[i]->y;
    }
return( Py_BuildValue("(dddd)", xmin,ymin, xmax,ymax ));
}

static PyObject *PyFFContour_GetSplineAfterPoint(PyFF_Contour *self, PyObject *args) {
    int pnum,prev;
    BasePoint start, ncp, pcp, end;
    double cx,cy, bx,by;

    if ( !PyArg_ParseTuple(args,"i", &pnum ) )
return( NULL );
    if ( pnum>=self->pt_cnt ) {
	PyErr_Format(PyExc_ValueError, "Point index out of range" );
return( NULL );
    }
    if ( self->is_quadratic ) {
	if ( self->points[pnum]->on_curve ) {
	    start.x = self->points[pnum]->x; start.y = self->points[pnum]->y;
	    if ( ++pnum>=self->pt_cnt )
		pnum = 0;
	    if ( self->points[pnum]->on_curve ) {
		end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
return( Py_BuildValue("((dddd)(dddd))", 0,0,end.x-start.x,start.x, 0,0,end.y-start.y,start.y ));
	    } else {
		ncp.x = self->points[pnum]->x; ncp.y = self->points[pnum]->y;
		if ( ++pnum>=self->pt_cnt )
		    pnum = 0;
		if ( self->points[pnum]->on_curve ) {
		    end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
		} else {
		    end.x = (self->points[pnum]->x+ncp.x)/2; end.y = (self->points[pnum]->y+ncp.y)/2;
		}
	    }
	} else {
	    ncp.x = self->points[pnum]->x; ncp.y = self->points[pnum]->y;
	    if ( ( prev = pnum-1 )<0 ) prev = self->pt_cnt-1;
	    if ( self->points[prev]->on_curve ) {
		start.x = self->points[prev]->x; start.y = self->points[prev]->y;
	    } else {
		start.x = (self->points[prev]->x+ncp.x)/2; start.y = (self->points[prev]->y+ncp.y)/2;
	    }
	    if ( ++pnum>=self->pt_cnt )
		pnum = 0;
	    if ( self->points[pnum]->on_curve ) {
		end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
	    } else {
		end.x = (self->points[pnum]->x+ncp.x)/2; end.y = (self->points[pnum]->y+ncp.y)/2;
	    }
	}
	cx = 2*(ncp.x-start.x); cy = 2*(ncp.y-start.y);
return( Py_BuildValue("((dddd)(dddd))", 0,end.x-start.x-cx,cx,start.x,
					0,end.y-start.y-cy,cy,start.y ));
    } else {
	if ( self->points[pnum]->on_curve ) {
	    if ( ( --pnum )<0 ) prev = self->pt_cnt-1;
	    if ( self->points[pnum]->on_curve ) {
		if ( ( --pnum )<0 ) prev = self->pt_cnt-1;
	    }
	}
	start.x = self->points[pnum]->x; start.y = self->points[pnum]->y;
	if ( ++pnum>=self->pt_cnt ) pnum = 0;
	if ( self->points[pnum]->on_curve ) {
	    end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
return( Py_BuildValue("((dddd)(dddd))", 0,0,end.x-start.x,start.x, 0,0,end.y-start.y,start.y ));
	}
	ncp.x = self->points[pnum]->x; ncp.y = self->points[pnum]->y;
	if ( ++pnum>=self->pt_cnt ) pnum = 0;
	pcp.x = self->points[pnum]->x; pcp.y = self->points[pnum]->y;
	if ( ++pnum>=self->pt_cnt ) pnum = 0;
	end.x = self->points[pnum]->x; end.y = self->points[pnum]->y;
	cx = 3*(ncp.x-start.x); cy = 3*(ncp.y-start.y);
	bx = 3*(pcp.x-ncp.x)-cx; by = 3*(pcp.y-ncp.y)-cy;
return( Py_BuildValue("((dddd)(dddd))", end.x-start.x-cx-bx,bx,cx,start.x,
					end.y-start.y-cy-by,by,cy,start.y ));
    }
}

static void do_pycall(PyObject *obj,char *method,PyObject *args_tuple) {
    PyObject *func, *result;

    func = PyObject_GetAttrString(obj,method);	/* I hope this is right */
    if ( func==NULL ) {
fprintf( stderr, "Failed to find %s in %s\n", method, obj->ob_type->tp_name );
	Py_DECREF(args_tuple);
return;
    }
    if (!PyCallable_Check(func)) {
	PyErr_Format(PyExc_TypeError, "Method, %s, is not callable", method );
	Py_DECREF(args_tuple);
	Py_DECREF(func);
return;
    }
    result = PyEval_CallObject(func, args_tuple);
    Py_DECREF(args_tuple);
    Py_XDECREF(result);
    Py_DECREF(func);
}

static PyObject *PointTuple(PyFF_Point *pt) {
    PyObject *pt_tuple = PyTuple_New(2);

    PyTuple_SetItem(pt_tuple,0,Py_BuildValue("d",pt->x));
    PyTuple_SetItem(pt_tuple,1,Py_BuildValue("d",pt->y));
return( pt_tuple );
}

static PyObject *PyFFContour_draw(PyFF_Contour *self, PyObject *args) {
    PyObject *pen, *tuple;
    PyFF_Point **points, **freeme=NULL;
    int i, start, off, last, j;

    if ( !PyArg_ParseTuple(args,"O", &pen ) )
return( NULL );
    if ( self->pt_cnt<2 )
Py_RETURN( self );

    points = self->points;
    /* The pen protocol demands that we start with an on-curve point */
    /* this means we may need to rotate our point list. */
    /* The pen protocol allows a contour of entirely off curve quadratic points */
    /*  so make a special check for that (can't occur in cubics) */
    for ( start=0; start<self->pt_cnt; ++start )
	if ( points[start]->on_curve )
    break;
    if ( start==self->pt_cnt ) {
	if ( self->is_quadratic ) {
	    tuple = PyTuple_New(self->pt_cnt+1);
	    for ( i=0; i<self->pt_cnt; ++i ) {
		PyTuple_SetItem(tuple,i,PointTuple(points[i]));
	    }
	    PyTuple_SetItem(tuple,i,Py_None); Py_INCREF(Py_None);
	    do_pycall(pen,"qCurveTo",tuple);
	} else {
	    PyErr_Format(PyExc_TypeError, "A cubic contour must have at least one oncurve point to be drawn" );
return( NULL );
	}
    } else {
	if ( start!=0 ) {
	    points = freeme = galloc(self->pt_cnt*sizeof(struct PyFF_Point *));
	    for ( i=start; i<self->pt_cnt; ++i )
		points[i-start] = self->points[i];
	    off = i;
	    for ( i=0; i<start; ++i )
		points[i+off] = self->points[i];
	}

	tuple = PyTuple_New(1);
	PyTuple_SetItem(tuple,0,PointTuple(points[0]));
	do_pycall(pen,"moveTo",tuple);
	if ( PyErr_Occurred())
return( NULL );
	last = 0;
	for ( i=1; i<self->pt_cnt; ++i ) {
	    if ( !points[i]->on_curve )
	continue;
	    if ( i-last==1 ) {
		tuple = PyTuple_New(1);
		PyTuple_SetItem(tuple,0,PointTuple(points[i]));
		do_pycall(pen,"lineTo",tuple);
	    } else if ( i-last==3 && !self->is_quadratic ) {
		tuple = PyTuple_New(3);
		PyTuple_SetItem(tuple,0,PointTuple(points[i-2]));
		PyTuple_SetItem(tuple,1,PointTuple(points[i-1]));
		PyTuple_SetItem(tuple,2,PointTuple(points[i]));
		do_pycall(pen,"curveTo",tuple);
	    } else if ( self->is_quadratic ) {
		tuple = PyTuple_New(i-last);
		for ( j=last+1; j<=i; ++j )
		    PyTuple_SetItem(tuple,j-(last+1),PointTuple(points[j]));
		do_pycall(pen,"qCurveTo",tuple);
	    } else {
		PyErr_Format(PyExc_TypeError, "Wrong number of off-curve points on a cubic contour");
return( NULL );
	    }
	    if ( PyErr_Occurred())
return( NULL );
	    last = i;
	}
	if ( i-last==3 && !self->is_quadratic && self->closed ) {
	    tuple = PyTuple_New(3);
	    PyTuple_SetItem(tuple,0,PointTuple(points[i-2]));
	    PyTuple_SetItem(tuple,1,PointTuple(points[i-1]));
	    PyTuple_SetItem(tuple,2,PointTuple(points[0]));
	    do_pycall(pen,"curveTo",tuple);
	} else if ( i-last!=1 && self->is_quadratic && self->closed ) {
	    tuple = PyTuple_New(i-last);
	    for ( j=last+1; j<i; ++j )
		PyTuple_SetItem(tuple,j-(last+1),PointTuple(points[j]));
	    PyTuple_SetItem(tuple,j-(last+1),PointTuple(points[0]));
	    do_pycall(pen,"qCurveTo",tuple);
	}
    }
    free(freeme);

    if ( PyErr_Occurred())
return( NULL );

    tuple = PyTuple_New(0);
    if ( self->closed )
	do_pycall(pen,"closePath",tuple);
    else
	do_pycall(pen,"endPath",tuple);
    if ( PyErr_Occurred())
return( NULL );
	    
Py_RETURN( self );
}

static PyMethodDef PyFFContour_methods[] = {
    {"isEmpty", (PyCFunction)PyFFContour_IsEmpty, METH_NOARGS,
	     "Returns whether a contour contains no points" },
    {"moveTo", (PyCFunction)PyFFContour_Start, METH_VARARGS,
	     "Place an initial point on an empty contour" },
    {"lineTo", (PyCFunction)PyFFContour_LineTo, METH_VARARGS,
	     "Append a line to a contour (optionally specifying position)" },
    {"cubicTo", (PyCFunction)PyFFContour_CubicTo, METH_VARARGS,
	     "Append a cubic curve to a (cubic) contour (optionally specifying position)" },
    {"quadraticTo", (PyCFunction)PyFFContour_QuadraticTo, METH_VARARGS,
	     "Append a quadratic curve to a (quadratic) contour (optionally specifying position)" },
    {"insertPoint", (PyCFunction)PyFFContour_InsertPoint, METH_VARARGS,
	     "Add a curve point to a contour (optionally specifying position)" },
    {"makeFirst", (PyCFunction)PyFFContour_MakeFirst, METH_VARARGS,
	     "Rotate a contour so that the specified point is first" },
    {"reverseDirection", (PyCFunction)PyFFContour_ReverseDirection, METH_NOARGS,
	     "Reverse a closed contour so that the second point on it is the penultimate and vice versa." },
    {"isClockwise", (PyCFunction)PyFFContour_IsClockwise, METH_NOARGS,
	     "Determine if a contour is oriented in a clockwise direction. If the contour intersects itself the results are indeterminate." },
    {"merge", (PyCFunction)PyFFContour_Merge, METH_VARARGS,
	     "Removes the specified on-curve point leaving the contour otherwise intact" },
    {"selfIntersects", (PyCFunction)PyFFContour_selfIntersects, METH_NOARGS,
	     "Returns whether this contour intersects itself" },
    {"simplify", (PyCFunction)PyFFContour_Simplify, METH_VARARGS,
	     "Smooths a contour" },
    {"transform", (PyCFunction)PyFFContour_Transform, METH_VARARGS,
	     "Transform a contour by a 6 element matrix." },
    {"addExtrema", (PyCFunction)PyFFContour_AddExtrema, METH_VARARGS,
	     "Add Extrema to a contour" },
    {"round", (PyCFunction)PyFFContour_Round, METH_VARARGS,
	     "Round points on a contour" },
    {"cluster", (PyCFunction)PyFFContour_Cluster, METH_VARARGS,
	     "Round points on a contour" },
    {"boundingBox", (PyCFunction)PyFFContour_BoundingBox, METH_NOARGS,
	     "Finds a bounding box for the countour (xmin,ymin,xmax,ymax)" },
    {"getSplineAfterPoint", (PyCFunction)PyFFContour_GetSplineAfterPoint, METH_VARARGS,
	     "Returns the coordinates of two cubic splines, one for x movement, one for y.\n"
	     "(Quadratic curves will have 0s for the first coordinates).\n"
	     "The spline will be either the on after the specified point for on-curve points.\n"
	     "or the one through the specified point for control points." },
    {"draw", (PyCFunction)PyFFContour_draw, METH_VARARGS,
	     "Support for the \"pen\" protocol (I hope)\nhttp://just.letterror.com/ltrwiki/PenProtocol" },
    {NULL}  /* Sentinel */
};

static PyTypeObject PyFF_ContourType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.contour",       /*tp_name*/
    sizeof(PyFF_Contour),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyFFContour_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    (cmpfunc)PyFFContour_compare, /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    &PyFFContour_Sequence,     /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFContour_Str,/*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_CHECKTYPES,
			       /*tp_flags*/
    "fontforge Contour objects", /* tp_doc */
    0/*(traverseproc)FFContour_traverse*/,  /* tp_traverse */
    (inquiry)PyFFContour_clear,  /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    contouriter_new,	       /* tp_iter */
    0,		               /* tp_iternext */
    PyFFContour_methods,       /* tp_methods */
    0,			       /* tp_members */
    PyFFContour_getset,        /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyFFContour_init,/* tp_init */
    0,                         /* tp_alloc */
    PyFFContour_new,           /* tp_new */
};

/* ************************************************************************** */
/* Layer iterator type */
/* ************************************************************************** */

typedef struct {
    PyObject_HEAD
    int pos;
    PyFF_Layer *layer;
} layeriterobject;
static PyTypeObject PyFF_LayerIterType;

static PyObject *layeriter_new(PyObject *layer) {
    layeriterobject *li;
    li = PyObject_New(layeriterobject, &PyFF_LayerIterType);
    if (li == NULL)
return NULL;
    li->layer = ((PyFF_Layer *) layer);
    Py_INCREF(layer);
    li->pos = 0;
return (PyObject *)li;
}

static void layeriter_dealloc(layeriterobject *li) {
    Py_XDECREF(li->layer);
    PyObject_Del(li);
}

static PyObject *layeriter_iternext(layeriterobject *li) {
    PyFF_Layer *layer = li->layer;
    PyObject *c;

    if ( layer == NULL)
return NULL;

    if ( li->pos<layer->cntr_cnt ) {
	c = (PyObject *) layer->contours[li->pos++];
	Py_INCREF(c);
return( c );
    }

return NULL;
}

static PyTypeObject PyFF_LayerIterType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"layeriter",				/* tp_name */
	sizeof(layeriterobject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)layeriter_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)layeriter_iternext,	/* tp_iternext */
	0,					/* tp_methods */
	0,
};

/* ************************************************************************** */
/* Layers */
/* ************************************************************************** */

static int PyFFLayer_clear(PyFF_Layer *self) {
    int i;

    for ( i=0; i<self->cntr_cnt; ++i )
	Py_DECREF(self->contours[i]);
    self->cntr_cnt = 0;

return 0;
}

static void PyFFLayer_dealloc(PyFF_Layer *self) {
    PyFFLayer_clear(self);
    PyMem_Del(self->contours);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *PyFFLayer_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyFF_Layer *self;

    self = (PyFF_Layer *)type->tp_alloc(type, 0);
    if ( self!=NULL ) {
	self->contours = NULL;
	self->cntr_cnt = self->cntr_max = 0;
	self->is_quadratic = 0;
    }

return (PyObject *)self;
}

static int PyFFLayer_init(PyFF_Layer *self, PyObject *args, PyObject *kwds) {
    int quad=0;

    if ( args!=NULL && !PyArg_ParseTuple(args, "|i", &quad))
return -1;

    self->is_quadratic = (quad!=0);
return 0;
}

static PyObject *PyFFLayer_Str(PyFF_Layer *self) {
    char *buffer, *pt;
    int cnt, i,j;
    PyFF_Contour *contour;
    PyObject *ret;

    cnt = 0;
    for ( i=0; i<self->cntr_cnt; ++i )
	cnt += self->contours[i]->pt_cnt;
    buffer=pt=galloc(cnt*30+self->cntr_cnt*30+30);
    strcpy(buffer, self->is_quadratic? "<Layer(quadratic)\n":"<Layer(cubic)\n");
    pt = buffer+strlen(buffer);
    for ( i=0; i<self->cntr_cnt; ++i ) {
	contour = self->contours[i];
	strcpy(pt, " <Contour\n" );
	pt += strlen(pt);
	for ( j=0; j<contour->pt_cnt; ++j ) {
	    sprintf( pt, "  (%g,%g) %s\n", contour->points[j]->x, contour->points[j]->y,
		    contour->points[j]->on_curve ? "on" : "off" );
	    pt += strlen( pt );
	}
	strcpy(pt," >\n");
	pt += strlen(pt);
    }
    strcpy(pt,">");
    ret = PyString_FromString( buffer );
    free( buffer );
return( ret );
}

static int PyFFLayer_docompare(PyFF_Layer *self,PyObject *other,
	double pt_err, double spline_err) {
    SplineSet *ss, *ss2;
    int ret;

    ss = SSFromLayer(self);
    if ( PyType_IsSubtype(&PyFF_ContourType,other->ob_type) ) {
	ss2 = SSFromContour((PyFF_Contour *) other,NULL);
    } else if ( PyType_IsSubtype(&PyFF_LayerType,other->ob_type) ) {
	ss2 = SSFromLayer((PyFF_Layer *) other);
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( -1 );
    }
    ret = SSsCompare(ss,ss2,pt_err,spline_err,NULL);
    SplinePointListsFree(ss);
    SplinePointListsFree(ss2);
return(ret);
}

static int PyFFLayer_compare(PyFF_Layer *self,PyObject *other) {
    const double pt_err = .5, spline_err = 1;
    int i,j,ret;

    ret = PyFFLayer_docompare(self,other,pt_err,spline_err);
    if ( !(ret&SS_NoMatch) )
return( 0 );

    /* There's no real ordering on these guys. Make up something that is */
    /*  at least consistent */
    if ( PyType_IsSubtype(&PyFF_ContourType,other->ob_type) )
return( -1 );

    /* Ok, both are layers */
    if ( self->cntr_cnt < ((PyFF_Layer *) other)->cntr_cnt )
return( -1 );
    else if ( self->cntr_cnt > ((PyFF_Layer *) other)->cntr_cnt )
return( 1 );
    /* If there's a difference, then there must be at least one point to be */
    /*  different. And we only get here if there were a difference */
    for ( j=0; j<self->cntr_cnt; ++j ) {
	PyFF_Contour *scon = self->contours[j], *ocon = ((PyFF_Layer *) other)->contours[j];
	if ( scon->pt_cnt<ocon->pt_cnt )
return( -1 );
	else if ( scon->pt_cnt>ocon->pt_cnt )
return( 1 );
	for ( i=0; i<scon->pt_cnt; ++i ) {
	    ret = PyFFPoint_compare(scon->points[i],(PyObject *) ocon->points[i]);
	    if ( ret!=0 )
return( ret );
	}
    }

return( -1 );		/* Arbetrary... but we can't get here=>all points same */
}

/* ************************************************************************** */
/* Layer getters/setters */
/* ************************************************************************** */
static PyObject *PyFF_Layer_get_is_quadratic(PyFF_Layer *self,void *closure) {
return( Py_BuildValue("i", self->is_quadratic ));
}

static int PyFF_Layer_set_is_quadratic(PyFF_Layer *self,PyObject *value,void *closure) {
    int val;
    SplineSet *ss, *ss2;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );

    val = (val!=0);
    if ( val == self->is_quadratic )
return( 0 );
    ss = SSFromLayer(self);
    PyFFLayer_clear(self);
    if ( val )
	ss2 = SplineSetsTTFApprox(ss);
    else
	ss2 = SplineSetsPSApprox(ss);
    SplinePointListFree(ss);
    self->is_quadratic = (val!=0);
    LayerFromSS(ss2,self);
    SplinePointListFree(ss2);
return( 0 );
}

static PyGetSetDef PyFFLayer_getset[] = {
    {"is_quadratic",
	 (getter)PyFF_Layer_get_is_quadratic, (setter)PyFF_Layer_set_is_quadratic,
	 "Whether this is an quadratic or cubic layer", NULL},
    { NULL }
};

/* ************************************************************************** */
/* Layer sequence */
/* ************************************************************************** */
static int PyFFLayer_Length( PyObject *self ) {
return( ((PyFF_Layer *) self)->cntr_cnt );
}

static PyObject *PyFFLayer_Concat( PyObject *_c1, PyObject *_c2 ) {
    PyFF_Layer *c1 = (PyFF_Layer *) _c1, *c2 = (PyFF_Layer *) _c2;
    PyFF_Layer *self;
    int i;
    PyFF_Layer dummy;
    PyFF_Contour *dummies[1];

    if ( PyType_IsSubtype(&PyFF_ContourType,c2->ob_type) &&
	    c1->is_quadratic == c2->is_quadratic ) {
	memset(&dummy,0,sizeof(dummy));
	dummy.cntr_cnt = 1;
	dummy.contours = dummies; dummies[0] = (PyFF_Contour *) _c2;
	c2 = &dummy;
    } else if ( !PyType_IsSubtype(&PyFF_LayerType,c1->ob_type) ||
	    !PyType_IsSubtype(&PyFF_LayerType,c2->ob_type) ||
	    c1->is_quadratic != c2->is_quadratic ) {
	PyErr_Format(PyExc_TypeError, "Both arguments must be Layers of the same order");
return( NULL );
    }
    self = (PyFF_Layer *)PyFF_LayerType.tp_alloc(&PyFF_LayerType, 0);
    self->is_quadratic = c1->is_quadratic;
    self->cntr_max = self->cntr_cnt = c1->cntr_cnt + c2->cntr_cnt;
    self->contours = PyMem_New(PyFF_Contour *,self->cntr_max);
    for ( i=0; i<c1->cntr_cnt; ++i ) {
	Py_INCREF(c1->contours[i]);
	self->contours[i] = c1->contours[i];
    }
    for ( i=0; i<c2->cntr_cnt; ++i ) {
	Py_INCREF(c2->contours[i]);
	self->contours[c1->cntr_cnt+i] = c2->contours[i];
    }
Py_RETURN( self );
}

static PyObject *PyFFLayer_InPlaceConcat( PyObject *_self, PyObject *_c2 ) {
    PyFF_Layer *self = (PyFF_Layer *) _self, *c2 = (PyFF_Layer *) _c2;
    int i, old_cnt;
    PyFF_Layer dummy;
    PyFF_Contour *dummies[1];

    if ( PyType_IsSubtype(&PyFF_ContourType,c2->ob_type) &&
	    self->is_quadratic == ((PyFF_Contour *) c2)->is_quadratic ) {
	memset(&dummy,0,sizeof(dummy));
	dummy.cntr_cnt = 1;
	dummy.contours = dummies; dummies[0] = (PyFF_Contour *) _c2;
	c2 = &dummy;
    } else if ( !PyType_IsSubtype(&PyFF_LayerType,self->ob_type) ||
	    !PyType_IsSubtype(&PyFF_LayerType,c2->ob_type) ||
	    self->is_quadratic != c2->is_quadratic ) {
	PyErr_Format(PyExc_TypeError, "Both arguments must be Layers of the same order");
return( NULL );
    }
    old_cnt = self->cntr_cnt;
    self->cntr_cnt += c2->cntr_cnt;
    if ( self->cntr_cnt >= self->cntr_max ) {
	self->cntr_max = self->cntr_cnt;
	self->contours = PyMem_Resize(self->contours,PyFF_Contour *,self->cntr_max);
    }
    for ( i=0; i<c2->cntr_cnt; ++i ) {
	Py_INCREF(c2->contours[i]);
	self->contours[old_cnt+i] = c2->contours[i];
    }
Py_RETURN( self );
}

static PyObject *PyFFLayer_Index( PyObject *self, int pos ) {
    PyFF_Layer *layer = (PyFF_Layer *) self;
    PyObject *ret;

    if ( pos<0 || pos>=layer->cntr_cnt ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( NULL );
    }
    ret = (PyObject *) layer->contours[pos];
    Py_INCREF(ret);
return( ret );
}

static int PyFFLayer_IndexAssign( PyObject *self, int pos, PyObject *val ) {
    PyFF_Layer *layer = (PyFF_Layer *) self;
    PyFF_Contour *contour;
    PyFF_Contour *old;

    if ( !PyType_IsSubtype(&PyFF_ContourType,val->ob_type) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Contour");
return( -1 );
    }
    contour = (PyFF_Contour *) val;
    if ( pos<0 || pos>=layer->cntr_cnt ) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( -1 );
    }
    if ( contour->is_quadratic!=layer->is_quadratic ) {
	PyErr_Format(PyExc_TypeError, "Replacement contour must have the same order as the layer");
return( -1 );
    }

    old = layer->contours[pos];
    layer->contours[pos] = contour;
    Py_DECREF( old );
return( 0 );
}

static PySequenceMethods PyFFLayer_Sequence = {
    PyFFLayer_Length,		/* length */
    PyFFLayer_Concat,		/* concat */
    NULL,			/* repeat */
    PyFFLayer_Index,		/* subscript */
    NULL,			/* slice */
    PyFFLayer_IndexAssign,	/* subscript assign */
    NULL,			/* slice assign */
    NULL,			/* contains */
    PyFFLayer_InPlaceConcat	/* inplace_concat */
};

/* ************************************************************************** */
/* Layer methods */
/* ************************************************************************** */
static PyObject *PyFFLayer_IsEmpty(PyFF_Layer *self) {
    /* Arg checking done elsewhere */
return( Py_BuildValue("i",self->cntr_cnt==0 ) );
}

static PyObject *PyFFLayer_selfIntersects(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss;
    Spline *s, *s2;
    PyObject *ret;

    ss = SSFromLayer(self);
    ret = SplineSetIntersect(ss,&s,&s2) ? Py_True : Py_False;
    SplinePointListFree(ss);
    Py_INCREF( ret );
return( ret );
}

static PyObject *PyFFLayer_Simplify(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss;
    static struct simplifyinfo smpl = { sf_normal,.75,.2,10 };

    smpl.err = 1;
    smpl.linefixup = 2;
    smpl.linelenmax = 10;

    ss = SSFromLayer(self);
    if ( ss==NULL )
Py_RETURN( self );		/* As simple as it can be */

    if ( PySequence_Size(args)>=1 )
	smpl.err = PyFloat_AsDouble(PySequence_GetItem(args,0));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=2 )
	smpl.flags = FlagsFromTuple( PySequence_GetItem(args,1),simplifyflags);
    if ( !PyErr_Occurred() && PySequence_Size(args)>=3 )
	smpl.tan_bounds = PyFloat_AsDouble( PySequence_GetItem(args,2));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=4 )
	smpl.linefixup = PyFloat_AsDouble( PySequence_GetItem(args,3));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=5 )
	smpl.linelenmax = PyFloat_AsDouble( PySequence_GetItem(args,4));
    if ( PyErr_Occurred() )
return( NULL );
    ss = SplineCharSimplify(NULL,ss,&smpl);
    LayerFromSS(ss,self);
    SplinePointListsFree(ss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_Transform(PyFF_Layer *self, PyObject *args) {
    int i, j;
    double m[6];
    PyFF_Contour *cntr;

    if ( !PyArg_ParseTuple(args,"(dddddd)",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( NULL );
    for ( i=0; i<self->cntr_cnt; ++i ) {
	cntr = self->contours[i];
	for ( j=0; j<cntr->pt_cnt; ++j )
	    PyFF_TransformPoint(cntr->points[j],m);
    }
Py_RETURN( self );
}

static PyObject *PyFFLayer_Round(PyFF_Layer *self, PyObject *args) {
    double factor=1;
    int i,j;
    PyFF_Contour *cntr;

    if ( !PyArg_ParseTuple(args,"d",&factor ) )
return( NULL );
    for ( i=0; i<self->cntr_cnt; ++i ) {
	cntr = self->contours[i];
	for ( j=0; j<cntr->pt_cnt; ++j ) {
	    cntr->points[j]->x = rint( factor*cntr->points[j]->x )/factor;
	    cntr->points[j]->y = rint( factor*cntr->points[j]->y )/factor;
	}
    }
Py_RETURN( self );
}

static PyObject *PyFFLayer_Cluster(PyFF_Layer *self, PyObject *args) {
    double within = .1, max = .5;
    SplineChar sc;
#ifdef FONTFORGE_CONFIG_TYPE3
    Layer layers[2];
#endif
    SplineSet *ss;

    if ( !PyArg_ParseTuple(args,"|dd", &within, &max ) )
return( NULL );

    ss = SSFromLayer(self);
    if ( ss==NULL )
Py_RETURN( self );		/* no contours=> no clusters */
    memset(&sc,0,sizeof(sc));
#ifdef FONTFORGE_CONFIG_TYPE3
    memset(layers,0,sizeof(layers));
    sc.layers = layers;
#endif
    sc.layers[ly_fore].splines = ss;
    sc.name = "nameless";
    SCRoundToCluster( &sc,ly_fore,false,within,max);
    LayerFromSS(sc.layers[ly_fore].splines,self);
    SplinePointListsFree(sc.layers[ly_fore].splines);
Py_RETURN( self );
}

static PyObject *PyFFLayer_AddExtrema(PyFF_Layer *self, PyObject *args) {
    int emsize = 1000;
    char *flag = NULL;
    int ae = ae_only_good;
    SplineSet *ss;

    if ( !PyArg_ParseTuple(args,"|si", &flag, &emsize ) )
return( NULL );
    if ( flag!=NULL )
	ae = FlagsFromString(flag,addextremaflags);

    ss = SSFromLayer(self);
    if ( ss==NULL )
Py_RETURN( self );		/* no contours=> nothing to do */
    SplineCharAddExtrema(NULL,ss,ae,emsize);
    LayerFromSS(ss,self);
    SplinePointListsFree(ss);
Py_RETURN( self );
}

struct flaglist linecap[] = {
    { "butt", lc_butt },
    { "round", lc_round },
    { "square", lc_square },
    { NULL }
};

struct flaglist linejoin[] = {
    { "miter", lj_miter },
    { "round", lj_round },
    { "bevel", lj_bevel },
    { NULL }
};

struct flaglist strokeflags[] = {
    { "removeinternal", 1 },
    { "removeexternal", 2 },
    { "cleanup", 4 },
    { NULL }
};

static int Stroke_Parse(StrokeInfo *si, PyObject *args) {
    char *str, *cap="butt", *join="round";
    double width=0, minor=1, angle=0;
    int c, j, f;
    PyObject *flagtuple=NULL;

    str =PyString_AsString(PySequence_GetItem(args,0));
    memset(si,0,sizeof(*si));
    if ( str==NULL )
return( -1 );
    if ( strcmp(str,"circular")==0 ) {
	if ( !PyArg_ParseTuple(args,"sd|ssO", &str, &width, &cap, &join, &flagtuple ) )
return( -1 );
	si->stroke_type = si_std;
    } else if ( strcmp(str,"eliptical")==0 ) {
	if ( !PyArg_ParseTuple(args,"sddd|ssO", &str, &width, &minor, &angle, &cap, &join, &flagtuple ) )
return( -1 );
	si->stroke_type = si_elipse;
    } else if ( strcmp(str,"caligraphic")==0 ) {
	if ( !PyArg_ParseTuple(args,"sddd|O", &str, &width, &minor, &angle, &flagtuple ) )
return( -1 );
	si->stroke_type = si_caligraphic;
    } else {
	PyErr_Format(PyExc_ValueError, "Unknown stroke type %s", str );
return( -1 );
    }
    if ( width<=0 || minor<=0 ) {
	PyErr_Format(PyExc_ValueError, "Stroke width must be positive" );
return( -1 );
    }
    c = FlagsFromString(cap,linecap);
    j = FlagsFromString(join,linejoin);
    f = FlagsFromTuple(flagtuple,strokeflags);
    if ( c==0x80000000 || j==0x80000000 || f==0x80000000 ) {
	PyErr_Format(PyExc_ValueError, "Bad value for line cap, join or flags" );
return( -1 );
    }
    si->radius = width/2;
    si->join = j;
    si->cap = c;
    if ( f&1 )
	si->removeinternal = true;
    if ( f&2 )
	si->removeexternal = true;
    if ( f&4 )
	si->removeoverlapifneeded = true;
    si->penangle = angle;

    if ( si->stroke_type == si_caligraphic )
	si->ratio = minor/width;
    else if ( si->stroke_type == si_elipse )
	si->minorradius = minor/2;
return( 0 );
}

static PyObject *PyFFLayer_Stroke(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss, *newss;
    StrokeInfo si;

    if ( Stroke_Parse(&si,args)==-1 )
return( NULL );

    ss = SSFromLayer(self);
    if ( ss==NULL )
Py_RETURN( self );		/* no contours=> nothing to do */
    newss = SSStroke(ss,&si,NULL);
    SplinePointListFree(ss);
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_Correct(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss, *newss;
    int changed = false;

    ss = SSFromLayer(self);
    if ( ss==NULL )
Py_RETURN( self );		/* no contours=> nothing to do */
    newss = SplineSetsCorrect(ss,&changed);
    /* same old splinesets */
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_RemoveOverlap(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss, *newss;

    ss = SSFromLayer(self);
    if ( ss==NULL )
Py_RETURN( self );		/* no contours=> nothing to do */
    newss = SplineSetRemoveOverlap(NULL,ss,over_remove);
    /* Frees the old splinesets */
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_Intersect(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss, *newss;

    ss = SSFromLayer(self);
    if ( ss==NULL )
Py_RETURN( self );		/* no contours=> nothing to do */
    newss = SplineSetRemoveOverlap(NULL,ss,over_intersect);
    /* Frees the old splinesets */
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_Exclude(PyFF_Layer *self, PyObject *args) {
    SplineSet *ss, *newss, *excludes, *tail;
    PyObject *obj;

    if ( !PyArg_ParseTuple(args,"O", &obj ) )
return( NULL );
    if ( !PyType_IsSubtype(&PyFF_LayerType,obj->ob_type) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Layer");
return( NULL );
    }

    ss = SSFromLayer(self);
    if ( ss==NULL )
Py_RETURN( self );		/* no contours=> nothing to do */
    excludes = SSFromLayer((PyFF_Layer *) obj);
    for ( tail=ss; tail->next!=NULL; tail=tail->next );
    tail->next = excludes;
    while ( excludes!=NULL ) {
	excludes->first->selected = true;
	excludes = excludes->next;
    }
    newss = SplineSetRemoveOverlap(NULL,ss,over_exclude);
    /* Frees the old splinesets */
    LayerFromSS(newss,self);
    SplinePointListsFree(newss);
Py_RETURN( self );
}

static PyObject *PyFFLayer_BoundingBox(PyFF_Layer *self, PyObject *args) {
    double xmin, xmax, ymin, ymax;
    int i,j,none;
    PyFF_Contour *cntr;

    none = true;
    for ( j=0; j<self->cntr_cnt; ++j ) {
	cntr = self->contours[j];
	for ( i=1; i<cntr->pt_cnt; ++i ) {
	    if ( none ) {
		xmin = xmax = cntr->points[0]->x;
		ymin = ymax = cntr->points[0]->y;
	    } else {
		if ( cntr->points[i]->x < xmin ) xmin = cntr->points[i]->x;
		if ( cntr->points[i]->x > xmax ) xmax = cntr->points[i]->x;
		if ( cntr->points[i]->y < ymin ) ymin = cntr->points[i]->y;
		if ( cntr->points[i]->y > ymax ) ymax = cntr->points[i]->y;
	    }
	}
    }
    if ( none )
return( Py_BuildValue("(dddd)", 0,0,0,0 ));

return( Py_BuildValue("(dddd)", xmin,ymin, xmax,ymax ));
}

static PyObject *PyFFLayer_draw(PyFF_Layer *self, PyObject *args) {
    int i;

    for ( i=0; i<self->cntr_cnt; ++i )
	PyFFContour_draw(self->contours[i],args);
Py_RETURN( self );
}

static PyMethodDef PyFFLayer_methods[] = {
    {"isEmpty", (PyCFunction)PyFFLayer_IsEmpty, METH_NOARGS,
	     "Returns whether a layer contains no contours" },
    {"simplify", (PyCFunction)PyFFLayer_Simplify, METH_VARARGS,
	     "Smooths a layer" },
    {"selfIntersects", (PyCFunction)PyFFLayer_selfIntersects, METH_NOARGS,
	     "Returns whether this layer intersects itself" },
    {"transform", (PyCFunction)PyFFLayer_Transform, METH_VARARGS,
	     "Transform a layer by a 6 element matrix." },
    {"addExtrema", (PyCFunction)PyFFLayer_AddExtrema, METH_VARARGS,
	     "Add Extrema to a layer" },
    {"round", (PyCFunction)PyFFLayer_Round, METH_VARARGS,
	     "Round contours on a layer" },
    {"cluster", (PyCFunction)PyFFLayer_Cluster, METH_VARARGS,
	     "Round contours on a layer" },
    {"boundingBox", (PyCFunction)PyFFLayer_BoundingBox, METH_NOARGS,
	     "Finds a bounding box for the layer (xmin,ymin,xmax,ymax)" },
    {"correctDirection", (PyCFunction)PyFFLayer_Correct, METH_NOARGS,
	     "Orient a layer so that external contours are clockwise and internal counter clockwise." },
    {"stroke", (PyCFunction)PyFFLayer_Stroke, METH_VARARGS,
	     "Strokes the countours in a layer" },
    {"removeOverlap", (PyCFunction)PyFFLayer_RemoveOverlap, METH_NOARGS,
	     "Remove overlapping areas from a layer." },
    {"intersect", (PyCFunction)PyFFLayer_Intersect, METH_NOARGS,
	     "Leaves the areas where the contours of a layer overlap." },
    {"exclude", (PyCFunction)PyFFLayer_Exclude, METH_VARARGS,
	     "Exclude the area of the argument (also a layer) from the current layer" },
    {"draw", (PyCFunction)PyFFLayer_draw, METH_VARARGS,
	     "Support for the \"pen\" protocol (I hope)\nhttp://just.letterror.com/ltrwiki/PenProtocol" },
    {NULL}  /* Sentinel */
};

static PyTypeObject PyFF_LayerType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.layer",         /*tp_name*/
    sizeof(PyFF_Layer),        /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyFFLayer_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    (cmpfunc)PyFFLayer_compare,/*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    &PyFFLayer_Sequence,       /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFLayer_Str,  /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_CHECKTYPES,
			       /*tp_flags*/
    "fontforge Layer objects", /* tp_doc */
    0/*(traverseproc)FFLayer_traverse*/,  /* tp_traverse */
    (inquiry)PyFFLayer_clear,  /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    layeriter_new,	       /* tp_iter */
    0,		               /* tp_iternext */
    PyFFLayer_methods,         /* tp_methods */
    0,			       /* tp_members */
    PyFFLayer_getset,          /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyFFLayer_init,  /* tp_init */
    0,                         /* tp_alloc */
    PyFFLayer_new,             /* tp_new */
};

/* ************************************************************************** */
/* Conversion routines between SplineSets and Contours & Layers */
/* ************************************************************************** */
static int SSSelectOnCurve(SplineSet *ss,int pos) {
    SplinePoint *sp;

    while ( ss!=NULL ) {
	for ( sp=ss->first ; ; ) {
	    if ( sp->ttfindex == pos ) {
		sp->selected = true;
return( true );
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp == ss->first )
	break;
	}
	ss = ss->next;
    }
return( 0 );
}

static SplineSet *SSFromContour(PyFF_Contour *c,int *tt_start) {
    int start = 0, next;
    int i, skipped=0, index;
    int nexti, previ;
    SplineSet *ss;
    SplinePoint *sp;

    if ( c->pt_cnt==0 ) {
	/*PyErr_Format(PyExc_TypeError, "Empty contour");*/
return( NULL );
    }

    if ( tt_start!=NULL )
	start = *tt_start;
    i = 0;
    next = start;

    ss = chunkalloc(sizeof(SplineSet));

    if ( c->is_quadratic ) {
	if ( !c->points[0]->on_curve ) {
	    if ( c->pt_cnt==1 ) {
		ss->first = ss->last = SplinePointCreate(c->points[0]->x,c->points[0]->y);
return( ss );
	    }
	    ++i;
	    ++next;
	    skipped = true;
	}
	while ( i<c->pt_cnt ) {
	    if ( c->points[i]->on_curve ) {
		sp = SplinePointCreate(c->points[i]->x,c->points[i]->y);
		sp->ttfindex = next++;
		index = -1;
		if ( i>0 && !c->points[i-1]->on_curve )
		    index = i-1;
		else if ( i==0 && !c->points[c->pt_cnt-1]->on_curve )
		    index = c->pt_cnt-1;
		if ( index!=-1 ) {
		    sp->prevcp.x = c->points[index]->x;
		    sp->prevcp.y = c->points[index]->y;
		    sp->noprevcp = false;
		}
		if ( ss->last==NULL )
		    ss->first = sp;
		else
		    SplineMake2(ss->last,sp);
		ss->last = sp;
	    } else {
		if ( !c->points[i-1]->on_curve ) {
		    sp = SplinePointCreate((c->points[i]->x+c->points[i-1]->x)/2,(c->points[i]->y+c->points[i-1]->y)/2);
		    sp->ttfindex = -1;
		    sp->prevcp.x = c->points[i-1]->x;
		    sp->prevcp.y = c->points[i-1]->y;
		    sp->noprevcp = false;
		    if ( ss->last==NULL )
			ss->first = sp;
		    else
			SplineMake2(ss->last,sp);
		    ss->last = sp;
		}
		ss->last->nextcp.x = c->points[i]->x;
		ss->last->nextcp.y = c->points[i]->y;
		ss->last->nonextcp = false;
		ss->last->nextcpindex = next++;
	    }
	    ++i;
	}
	if ( skipped ) {
	    i = c->pt_cnt;
	    if ( !c->points[i-1]->on_curve ) {
		sp = SplinePointCreate((c->points[0]->x+c->points[i-1]->x)/2,(c->points[0]->y+c->points[i-1]->y)/2);
		sp->ttfindex = -1;
		sp->prevcp.x = c->points[i-1]->x;
		sp->prevcp.y = c->points[i-1]->y;
		sp->noprevcp = false;
		if ( ss->last==NULL )
		    ss->first = sp;
		else
		    SplineMake2(ss->last,sp);
		ss->last = sp;
	    }
	    ss->last->nextcp.x = c->points[0]->x;
	    ss->last->nextcp.y = c->points[0]->y;
	    ss->last->nonextcp = false;
	    ss->last->nextcpindex = start;
	}
    } else {
	for ( i=0; i<c->pt_cnt; ++i ) {
	    if ( c->points[i]->on_curve )
	break;
	    ++next;
	}
	for ( i=0; i<c->pt_cnt; ++i ) {
	    if ( !c->points[i]->on_curve )
	continue;
	    sp = SplinePointCreate(c->points[i]->x,c->points[i]->y);
	    sp->ttfindex = next++;
	    nexti = previ = -1;
	    if ( i==0 )
		previ = c->pt_cnt-1;
	    else
		previ = i-1;
	    if ( !c->points[previ]->on_curve ) {
		sp->prevcp.x = c->points[previ]->x;
		sp->prevcp.y = c->points[previ]->y;
		if ( sp->prevcp.x!=sp->me.x || sp->prevcp.y!=sp->me.y )
		    sp->noprevcp = false;
	    }
	    if ( i==c->pt_cnt-1 )
		nexti = 0;
	    else
		nexti = i+1;
	    if ( !c->points[nexti]->on_curve ) {
		sp->nextcp.x = c->points[nexti]->x;
		sp->nextcp.y = c->points[nexti]->y;
		next += 2;
		if ( sp->nextcp.x!=sp->me.x || sp->nextcp.y!=sp->me.y )
		    sp->nonextcp = false;
		if ( nexti==c->pt_cnt-1 )
		    nexti = 0;
		else
		    ++nexti;
		if ( c->points[nexti]->on_curve ) {
		    PyErr_Format(PyExc_TypeError, "In cubic splines there must be exactly 2 control points between on curve points");
return( NULL );
		}
		if ( nexti==c->pt_cnt-1 )
		    nexti = 0;
		else
		    ++nexti;
		if ( !c->points[nexti]->on_curve ) {
		    PyErr_Format(PyExc_TypeError, "In cubic splines there must be exactly 2 control points between on curve points");
return( NULL );
		}
	    }
	    if ( ss->last==NULL )
		ss->first = sp;
	    else
		SplineMake3(ss->last,sp);
	    ss->last = sp;
	}
	if ( ss->last==NULL ) {
	    PyErr_Format(PyExc_TypeError, "Empty contour");
return( NULL );
	}
    }
    if ( c->closed ) {
	SplineMake2(ss->last,ss->first);
	ss->last = ss->first;
    }
    if ( tt_start!=NULL )
	*tt_start = next;
return( ss );
}

static PyFF_Contour *ContourFromSS(SplineSet *ss,PyFF_Contour *ret) {
    int k, cnt;
    SplinePoint *sp, *skip;

    if ( ret==NULL )
	ret = (PyFF_Contour *) PyFFContour_new(&PyFF_ContourType,NULL,NULL);
    else
	PyFFContour_clear(ret);
    ret->closed = ss->first->prev!=NULL;
    for ( k=0; k<2; ++k ) {
	if ( ss->first->next == NULL ) {
	    if ( k )
		ret->points[0] = PyFFPoint_CNew(ss->first->me.x,ss->first->me.y,true);
	    cnt = 1;
	} else if ( ss->first->next->order2 ) {
	    ret->is_quadratic = true;
	    cnt = 0;
	    skip = NULL;
	    if ( ss->first->ttfindex==0xffff ) {
		skip = ss->first->prev->from;
		if ( k )
		    ret->points[cnt] = PyFFPoint_CNew(skip->nextcp.x,skip->nextcp.y,false);
		++cnt;
	    }
	    for ( sp=ss->first; ; ) {
		if ( sp->ttfindex!=0xffff ) {
		    if ( k )
			ret->points[cnt] = PyFFPoint_CNew(sp->me.x,sp->me.y,true);
		    ++cnt;
		}
		if ( sp->nextcpindex!=0xffff && sp!=skip ) {
		    if ( k )
			ret->points[cnt] = PyFFPoint_CNew(sp->nextcp.x,sp->nextcp.y,false);
		    ++cnt;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	} else {
	    ret->is_quadratic = false;
	    for ( sp=ss->first, cnt=0; ; ) {
		if ( k )
		    ret->points[cnt] = PyFFPoint_CNew(sp->me.x,sp->me.y,true);
		++cnt;			/* Sp itself */
		if ( sp->next==NULL )
	    break;
		if ( !sp->nonextcp || !sp->next->to->noprevcp ) {
		    if ( k ) {
			ret->points[cnt  ] = PyFFPoint_CNew(sp->nextcp.x,sp->nextcp.y,false);
			ret->points[cnt+1] = PyFFPoint_CNew(sp->next->to->prevcp.x,sp->next->to->prevcp.y,false);
		    }
		    cnt += 2;		/* not a line => 2 control points */
		}
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
	if ( !k ) {
	    if ( cnt>=ret->pt_max ) {
		ret->pt_max = cnt;
		ret->points = PyMem_Resize(ret->points,PyFF_Point *,cnt);
	    }
	    ret->pt_cnt = cnt;
	}
    }
return( ret );
}

static SplineSet *SSFromLayer(PyFF_Layer *layer) {
    int start = 0;
    SplineSet *head=NULL, *tail, *cur;
    int i;

    for ( i=0; i<layer->cntr_cnt; ++i ) {
	cur = SSFromContour( layer->contours[i],&start );
	if ( cur!=NULL ) {
	    if ( head==NULL )
		head = cur;
	    else
		tail->next = cur;
	    tail = cur;
	}
    }
return( head );
}

static PyFF_Layer *LayerFromSS(SplineSet *ss,PyFF_Layer *layer) {
    SplineSet *cur;
    int cnt, i;

    if ( layer==NULL )
	layer = (PyFF_Layer *) PyFFLayer_new(&PyFF_LayerType,NULL,NULL);

    for ( cnt=0, cur=ss; cur!=NULL; cur=cur->next, ++cnt );
    if ( cnt>layer->cntr_max )
	layer->contours = PyMem_Resize(layer->contours,PyFF_Contour *,layer->cntr_max = cnt );
    if ( cnt>layer->cntr_cnt ) {
	for ( i=layer->cntr_cnt; i<cnt; ++i )
	    layer->contours[i] = NULL;
    } else if ( cnt<layer->cntr_cnt ) {
	for ( i = cnt; i<layer->cntr_cnt; ++i )
	    Py_DECREF( (PyObject *) layer->contours[i] );
    }
    layer->cntr_cnt = cnt;
    for ( cnt=0, cur=ss; cur!=NULL; cur=cur->next, ++cnt ) {
	layer->contours[cnt] = ContourFromSS(cur,layer->contours[cnt]);
	layer->is_quadratic = layer->contours[cnt]->is_quadratic;
    }
return( layer );
}

static PyFF_Layer *LayerFromLayer(Layer *inlayer,PyFF_Layer *ret) {
    /* May want to copy fills and pens someday!! */
return( LayerFromSS(inlayer->splines,ret));
}

/* ************************************************************************** */
/* GlyphPen Standard Methods */
/* ************************************************************************** */

static void PyFF_GlyphPen_dealloc(PyFF_GlyphPen *self) {
    if ( self->sc!=NULL ) {
	if ( self->changed )
	    SCCharChangedUpdate(self->sc);
	self->sc = NULL;
    }
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *PyFFGlyphPen_Str(PyFF_GlyphPen *self) {
return( PyString_FromFormat( "<GlyphPen for %s>", self->sc->name ));
}

/* ************************************************************************** */
/*  GlyphPen Methods  */
/* ************************************************************************** */
static void GlyphClear(PyObject *self) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    SCClearContents(sc);
    ((PyFF_GlyphPen *) self)->replace = false;
}

static PyObject *PyFFGlyphPen_moveTo(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    SplineSet *ss;
    double x,y;

    if ( !((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The moveTo operator may not be called while drawing a contour");
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "(dd)", &x, &y )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "dd", &x, &y ))
return( NULL );
    }
    if ( ((PyFF_GlyphPen *) self)->replace )
	GlyphClear(self);
    ss = chunkalloc(sizeof(SplineSet));
    ss->next = sc->layers[ly_fore].splines;
    sc->layers[ly_fore].splines = ss;
    ss->first = ss->last = SplinePointCreate(x,y);

    ((PyFF_GlyphPen *) self)->ended = false;
    ((PyFF_GlyphPen *) self)->changed = true;
Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_lineTo(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    SplinePoint *sp;
    SplineSet *ss;
    double x,y;

    if ( ((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The lineTo operator must be preceded by a moveTo operator" );
return( NULL );
    }
    if ( !PyArg_ParseTuple( args, "(dd)", &x, &y )) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple( args, "dd", &x, &y ))
return( NULL );
    }
    ss = sc->layers[ly_fore].splines;
    sp = SplinePointCreate(x,y);
    SplineMake(ss->last,sp,sc->parent->order2);
    ss->last = sp;

Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_curveTo(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    SplinePoint *sp;
    SplineSet *ss;
    double x[3],y[3];

    if ( ((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The curveTo operator must be preceded by a moveTo operator" );
return( NULL );
    }
    if ( sc->parent->order2 ) {
	if ( !PyArg_ParseTuple( args, "(dd)(dd)", &x[1], &y[1], &x[2], &y[2] )) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTuple( args, "dddd", &x[1], &y[1], &x[2], &y[2] ))
return( NULL );
	}
	x[0] = x[1]; y[0] = y[1];
    } else {
	if ( !PyArg_ParseTuple( args, "(dd)(dd)(dd)", &x[0], &y[0], &x[1], &y[1], &x[2], &y[2] )) {
	    PyErr_Clear();
	    if ( !PyArg_ParseTuple( args, "dddddd", &x[0], &y[0], &x[1], &y[1], &x[2], &y[2] ))
return( NULL );
	}
    }
    ss = sc->layers[ly_fore].splines;
    sp = SplinePointCreate(x[2],y[2]);
    sp->prevcp.x = x[1]; sp->prevcp.y = y[1];
    sp->noprevcp = false;
    ss->last->nextcp.x = x[0], ss->last->nextcp.y = y[0];
    ss->last->nonextcp = false;
    SplineMake(ss->last,sp,sc->parent->order2);
    ss->last = sp;

Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_qCurveTo(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    SplinePoint *sp;
    SplineSet *ss;
    double x0,y0, x1,y1, x2,y2;
    int len, i;
    PyObject *pt_tuple;

    if ( !sc->parent->order2 ) {
	PyErr_Format(PyExc_EnvironmentError, "qCurveTo only applies to quadratic fonts" );
return( NULL );
    }

    len = PySequence_Size(args);
    if ( PySequence_GetItem(args,len-1)== Py_None ) {
	--len;
	if ( !((PyFF_GlyphPen *) self)->ended ) {
	    PyErr_Format(PyExc_EnvironmentError, "qCurveTo must describe an entire contour if its last argument is None");
return( NULL );
	} else if ( len<2 ) {
	    PyErr_Format(PyExc_EnvironmentError, "qCurveTo must have at least two tuples");
return( NULL );
	}

	pt_tuple = PySequence_GetItem(args,0);
	if ( !PyArg_ParseTuple(pt_tuple,"dd", &x0, &y0 ))
return( NULL );

	ss = chunkalloc(sizeof(SplineSet));
	ss->next = sc->layers[ly_fore].splines;
	sc->layers[ly_fore].splines = ss;

	x1 = x0; y1 = y0;
	for ( i=1; i<len; ++i ) {
	    pt_tuple = PySequence_GetItem(args,i);
	    if ( !PyArg_ParseTuple(pt_tuple,"dd", &x2, &y2 ))
return( NULL );
	    sp = SplinePointCreate((x1+x2)/2,(y1+y2)/2);
	    sp->noprevcp = false;
	    sp->prevcp.x = x1; sp->prevcp.y = y1;
	    sp->nonextcp = false;
	    sp->nextcp.x = x2; sp->nextcp.y = y2;
	    if ( ss->first==NULL )
		ss->first = ss->last = sp;
	    else {
		SplineMake2(ss->last,sp);
		ss->last = sp;
	    }
	    x1=x2; y1=y2;
	}
	sp = SplinePointCreate((x0+x2)/2,(y0+y2)/2);
	sp->noprevcp = false;
	sp->prevcp.x = x2; sp->prevcp.y = y2;
	sp->nonextcp = false;
	sp->nextcp.x = x0; sp->nextcp.y = y0;
	SplineMake2(ss->last,sp);
	SplineMake2(sp,ss->first);
	ss->last = ss->first;

	/*((PyFF_GlyphPen *) self)->ended = true;*/
	((PyFF_GlyphPen *) self)->changed = true;
    } else {
	if ( ((PyFF_GlyphPen *) self)->ended ) {
	    PyErr_Format(PyExc_EnvironmentError, "The curveTo operator must be preceded by a moveTo operator" );
return( NULL );
	} else if ( len<2 ) {
	    PyErr_Format(PyExc_EnvironmentError, "qCurveTo must have at least two tuples");
return( NULL );
	}
	ss = sc->layers[ly_fore].splines;
	pt_tuple = PySequence_GetItem(args,0);
	if ( !PyArg_ParseTuple(pt_tuple,"dd", &x1, &y1 ))
return( NULL );
	ss->last->nextcp.x = x1; ss->last->nextcp.y = y1;
	ss->last->nonextcp = false;
	for ( i=1; i<len-1; ++i ) {
	    pt_tuple = PySequence_GetItem(args,i);
	    if ( !PyArg_ParseTuple(pt_tuple,"dd", &x2, &y2 ))
return( NULL );
	    sp = SplinePointCreate((x1+x2)/2,(y1+y2)/2);
	    sp->noprevcp = false;
	    sp->prevcp.x = x1; sp->prevcp.y = y1;
	    sp->nonextcp = false;
	    sp->nextcp.x = x2; sp->nextcp.y = y2;
	    SplineMake2(ss->last,sp);
	    ss->last = sp;
	    x1=x2; y1=y2;
	}
	pt_tuple = PySequence_GetItem(args,i);
	if ( !PyArg_ParseTuple(pt_tuple,"dd", &x2, &y2 ))
return( NULL );
	sp = SplinePointCreate(x2,y2);
	sp->noprevcp = false;
	sp->prevcp.x = x1; sp->prevcp.y = y1;
	SplineMake2(ss->last,sp);
	ss->last = sp;
    }

Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_closePath(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    SplineSet *ss;

    if ( ((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The curveTo operator must be preceded by a moveTo operator" );
return( NULL );
    }

    ss = sc->layers[ly_fore].splines;
    if ( ss->first!=ss->last && RealNear(ss->first->me.x,ss->last->me.x) &&
	    RealNear(ss->first->me.y,ss->last->me.y)) {
	ss->first->prevcp = ss->last->prevcp;
	ss->first->noprevcp = ss->last->noprevcp;
	ss->last->prev->to = ss->first;
	ss->first->prev = ss->last->prev;
	SplinePointFree(ss->last);
    } else {
	SplineMake(ss->last,ss->first,sc->parent->order2);
    }
    ss->last = ss->first;
	
    ((PyFF_GlyphPen *) self)->ended = true;
Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_endPath(PyObject *self, PyObject *args) {

    if ( ((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The curveTo operator must be preceded by a moveTo operator" );
return( NULL );
    }

    ((PyFF_GlyphPen *) self)->ended = true;
Py_RETURN( self );
}

static PyObject *PyFFGlyphPen_addComponent(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_GlyphPen *) self)->sc;
    real transform[6];
    SplineChar *rsc;
    double m[6];
    char *str;
    int j;

    if ( !((PyFF_GlyphPen *) self)->ended ) {
	PyErr_Format(PyExc_EnvironmentError, "The addComponent operator may not be called while drawing a contour");
return( NULL );
    }
    if ( ((PyFF_GlyphPen *) self)->replace )
	GlyphClear(self);

    memset(m,0,sizeof(m));
    m[0] = m[3] = 1; 
    if ( !PyArg_ParseTuple(args,"s|(dddddd)",&str,
	    &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( NULL );
    rsc = SFGetChar(sc->parent,-1,str);
    if ( rsc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No glyph named %s", str);
return( NULL );
    }
    for ( j=0; j<6; ++j )
	transform[j] = m[j];
    _SCAddRef(sc,rsc,transform);
    
Py_RETURN( self );
}

static PyMethodDef PyFF_GlyphPen_methods[] = {
    { "moveTo", PyFFGlyphPen_moveTo, METH_VARARGS, "Start a new contour at a point (a two element tuple)" },
    { "lineTo", PyFFGlyphPen_lineTo, METH_VARARGS, "Draws a line from the current point to the argument (a two element tuple)" },
    { "curveTo", PyFFGlyphPen_curveTo, METH_VARARGS, "Draws a cubic or quadratic bezier curve from the current point to the last arg" },
    { "qCurveTo", PyFFGlyphPen_qCurveTo, METH_VARARGS, "Draws a series of quadratic bezier curves" },
    { "closePath", PyFFGlyphPen_closePath, METH_VARARGS, "Closes the current contour (and ends it)" },
    { "endPath", PyFFGlyphPen_endPath, METH_VARARGS, "Ends the current contour (without closing it)" },
    { "addComponent", PyFFGlyphPen_addComponent, METH_VARARGS, "Adds a reference into the glyph" },
    NULL
};
/* ************************************************************************** */
/*  GlyphPen Type  */
/* ************************************************************************** */

static PyTypeObject PyFF_GlyphPenType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.glyphPen",      /*tp_name*/
    sizeof(PyFF_GlyphPen),     /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) PyFF_GlyphPen_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,		               /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFGlyphPen_Str,/*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "FontForge GlyphPen object",/* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    PyFF_GlyphPen_methods,     /* tp_methods */
    0,			       /* tp_members */
    0,			       /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    /*(initproc)PyFF_GlyphPen_init*/0,  /* tp_init */
    0,                         /* tp_alloc */
    /*PyFF_GlyphPen_new*/0        /* tp_new */
};

/* ************************************************************************** */
/* Glyph Standard Methods */
/* ************************************************************************** */

static void PyFF_Glyph_dealloc(PyFF_Glyph *self) {
    if ( self->sc!=NULL )
	self->sc = NULL;
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *PyFFGlyph_Str(PyFF_Glyph *self) {
return( PyString_FromFormat( "<Glyph %s in font %s>", self->sc->name, self->sc->parent->fontname ));
}

static int PyFFGlyph_docompare(PyFF_Glyph *self,PyObject *other,
	double pt_err, double spline_err) {
    SplineSet *ss2;
    int ret;

    if ( PyType_IsSubtype(&PyFF_GlyphType,other->ob_type) ) {
	SplineChar *sc = self->sc;
	SplineChar *sc2 = ((PyFF_Glyph *) other)->sc;
	int ret;
	SplinePoint *dummy;

	ret = CompareLayer(NULL,
		sc->layers[ly_fore].splines,sc2->layers[ly_fore].splines,
		sc->layers[ly_fore].refs,sc2->layers[ly_fore].refs,
		pt_err,spline_err,sc->name,false,&dummy);
return( ret );
    } else if ( PyType_IsSubtype(&PyFF_ContourType,other->ob_type) ) {
	ss2 = SSFromContour((PyFF_Contour *) other,NULL);
    } else if ( PyType_IsSubtype(&PyFF_LayerType,other->ob_type) ) {
	ss2 = SSFromLayer((PyFF_Layer *) other);
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected type");
return( -1 );
    }
    if ( self->sc->layers[ly_fore].refs!=NULL )
return( SS_NoMatch | SS_RefMismatch );
    ret = SSsCompare(self->sc->layers[ly_fore].splines,ss2,pt_err,spline_err,NULL);
    SplinePointListsFree(ss2);
return(ret);
}

static int PyFFGlyph_compare(PyFF_Glyph *self,PyObject *other) {
    const double pt_err = .5, spline_err = 1;
    int ret;
    SplineChar *sc1, *sc2;

    ret = PyFFGlyph_docompare(self,other,pt_err,spline_err);
    if ( !(ret&SS_NoMatch) )
return( 0 );

    /* There's no real ordering on these guys. Make up something that is */
    /*  at least consistent */
    if ( !PyType_IsSubtype(&PyFF_GlyphType,other->ob_type) )
return( -1 );
    /* Ok, both are glyphs */
    sc1 = self->sc; sc2 = ((PyFF_Glyph *) other)->sc;
return( sc1<sc2 ? -1 : 1 );
}

/* ************************************************************************** */
/* Glyph getters/setters */
/* ************************************************************************** */

static PyObject *PyFF_Glyph_get_userdata(PyFF_Glyph *self,void *closure) {
    if ( self->sc->python_data==NULL ) {
	self->sc->python_data = Py_None;
	Py_INCREF(Py_None);
    }
    Py_XINCREF( (PyObject *) (self->sc->python_data) );
return( self->sc->python_data );
}

static int PyFF_Glyph_set_userdata(PyFF_Glyph *self,PyObject *value,void *closure) {
    PyObject *old = self->sc->python_data;

    Py_INCREF(value);
    self->sc->python_data = value;
    Py_XDECREF(old);
return( 0 );
}

static PyObject *PyFF_Glyph_get_glyphname(PyFF_Glyph *self,void *closure) {

return( Py_BuildValue("s", self->sc->name ));
}

static int PyFF_Glyph_set_glyphname(PyFF_Glyph *self,PyObject *value,void *closure) {
    char *str = PyString_AsString(value);

    if ( str==NULL )
return( -1 );

    free( self->sc->name );
    self->sc->name = copy(str);
return( 0 );
}

static PyObject *PyFF_Glyph_get_unicode(PyFF_Glyph *self,void *closure) {

return( Py_BuildValue("i", self->sc->unicodeenc ));
}

static int PyFF_Glyph_set_unicode(PyFF_Glyph *self,PyObject *value,void *closure) {
    int uenc;

    uenc = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->unicodeenc = uenc;
return( 0 );
}

static PyObject *PyFF_Glyph_get_changed(PyFF_Glyph *self,void *closure) {

return( Py_BuildValue("i", self->sc->changed ));
}

static int PyFF_Glyph_set_changed(PyFF_Glyph *self,PyObject *value,void *closure) {
    int uenc;

    uenc = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->changed = uenc;
return( 0 );
}

static PyObject *PyFF_Glyph_get_texheight(PyFF_Glyph *self,void *closure) {

return( Py_BuildValue("i", self->sc->tex_height ));
}

static int PyFF_Glyph_set_texheight(PyFF_Glyph *self,PyObject *value,void *closure) {
    int val;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->tex_height = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_texdepth(PyFF_Glyph *self,void *closure) {

return( Py_BuildValue("i", self->sc->tex_depth ));
}

static int PyFF_Glyph_set_texdepth(PyFF_Glyph *self,PyObject *value,void *closure) {
    int val;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->tex_depth = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_originalgid(PyFF_Glyph *self,void *closure) {

return( Py_BuildValue("i", self->sc->orig_pos ));
}

static PyObject *PyFF_Glyph_get_width(PyFF_Glyph *self,void *closure) {

return( Py_BuildValue("i", self->sc->width ));
}

static int PyFF_Glyph_set_width(PyFF_Glyph *self,PyObject *value,void *closure) {
    int val;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    SCSynchronizeWidth(self->sc,val,self->sc->width,self->sc->parent->fv);
return( 0 );
}

static PyObject *PyFF_Glyph_get_lsb(PyFF_Glyph *self,void *closure) {
    DBounds b;

    SplineCharFindBounds(self->sc,&b);

return( Py_BuildValue("d", b.minx ));
}

static int PyFF_Glyph_set_lsb(PyFF_Glyph *self,PyObject *value,void *closure) {
    int val;
    real trans[6];
    DBounds b;
    SplineChar *sc = self->sc;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    SplineCharFindBounds(sc,&b);

    memset(trans,0,sizeof(trans));
    trans[0] = trans[3] = 1.0;
    trans[4] = val - b.minx;
    if ( trans[4]!=0 )
	FVTrans(sc->parent->fv,sc,trans,NULL,fvt_dobackground);
return( 0 );
}

static PyObject *PyFF_Glyph_get_rsb(PyFF_Glyph *self,void *closure) {
    DBounds b;

    SplineCharFindBounds(self->sc,&b);

return( Py_BuildValue("d", self->sc->width - b.maxx ));
}

static int PyFF_Glyph_set_rsb(PyFF_Glyph *self,PyObject *value,void *closure) {
    int val;
    DBounds b;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );

    SplineCharFindBounds(self->sc,&b);
    SCSynchronizeWidth(self->sc,rint( val+b.maxx ),self->sc->width,self->sc->parent->fv);
return( 0 );
}

static PyObject *PyFF_Glyph_get_vwidth(PyFF_Glyph *self,void *closure) {

return( Py_BuildValue("i", self->sc->vwidth ));
}

static int PyFF_Glyph_set_vwidth(PyFF_Glyph *self,PyObject *value,void *closure) {
    int val;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->vwidth = val;
return( 0 );
}

static PyObject *PyFF_Glyph_get_font(PyFF_Glyph *self,void *closure) {

return( PyFV_From_FV_I(self->sc->parent->fv));
}

static PyObject *PyFF_Glyph_get_references(PyFF_Glyph *self,void *closure) {
    RefChar *ref;
    int cnt;
    SplineChar *sc = self->sc;
    PyObject *tuple;

    for ( ref=sc->layers[ly_fore].refs, cnt=0; ref!=NULL; ++cnt, ref=ref->next );
    tuple = PyTuple_New(cnt);
    for ( ref=sc->layers[ly_fore].refs, cnt=0; ref!=NULL; ++cnt, ref=ref->next )
	PyTuple_SET_ITEM(tuple,cnt,Py_BuildValue("(s(dddddd))", ref->sc->name,
		ref->transform[0], ref->transform[1], ref->transform[2],
		ref->transform[3], ref->transform[4], ref->transform[5]));
return( tuple );
}

static int PyFF_Glyph_set_references(PyFF_Glyph *self,PyObject *value,void *closure) {
    int i, j, cnt;
    double m[6];
    real transform[6];
    char *str;
    SplineChar *sc = self->sc, *rsc;
    SplineFont *sf = sc->parent;
    RefChar *refs, *next;

    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a tuple of references");
return( -1 );
    }
    cnt = PySequence_Size(value);
    for ( refs=sc->layers[ly_fore].refs; refs!=NULL; refs = next ) {
	next = refs->next;
	SCRemoveDependent(sc,refs);
    }
    sc->layers[ly_fore].refs = NULL;
    for ( i=0; i<cnt; ++i ) {
	if ( !PyArg_ParseTuple(PySequence_GetItem(value,i),"s(dddddd)",&str,
		&m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( -1 );
	rsc = SFGetChar(sf,-1,str);
	if ( rsc==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No glyph named %s", str);
return( -1 );
	}
	for ( j=0; j<6; ++j )
	    transform[j] = m[j];
	_SCAddRef(sc,rsc,transform);
    }
return( 0 );
}

static PyObject *PyFF_Glyph_get_ttfinstrs(PyFF_Glyph *self,void *closure) {
    SplineChar *sc = self->sc;
    PyObject *binstr;

    binstr = PyString_FromStringAndSize((char *) sc->ttf_instrs,sc->ttf_instrs_len);
return( binstr );
}

static int PyFF_Glyph_set_ttfinstrs(PyFF_Glyph *self,PyObject *value,void *closure) {
    int i, cnt;
    SplineChar *sc = self->sc;

    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a sequence of integers");
return( -1 );
    }
    cnt = PySequence_Size(value);
    free(sc->ttf_instrs); sc->ttf_instrs = NULL; sc->ttf_instrs_len = cnt;
    if ( cnt==0 )
return( 0 );
    if ( PyString_Check(value)) {
	char *space; int len;
	PyString_AsStringAndSize(value,&space,&len);
	sc->ttf_instrs = gcalloc(len,sizeof(uint8));
	sc->ttf_instrs_len = len;
	memcpy(sc->ttf_instrs,space,len);
    } else {
	sc->ttf_instrs = gcalloc(cnt,sizeof(uint8));
	for ( i=0; i<cnt; ++i ) {
	    int val = PyInt_AsLong(PySequence_GetItem(value,i));
	    if ( PyErr_Occurred()!=NULL )
return( -1 );
	    sc->ttf_instrs[i] = val;
	}
    }
return( 0 );
}

struct flaglist glyphclasses[] = {
    { "automatic", 0 },
    { "noclass", 1 },
    { "baseglyph", 2 },
    { "baseligature", 3 },
    { "mark", 4 },
    { "component", 5 },
    NULL
};

static PyObject *PyFF_Glyph_get_glyphclass(PyFF_Glyph *self,void *closure) {
return( Py_BuildValue("s", glyphclasses[self->sc->glyph_class].name ));
}

static int PyFF_Glyph_set_glyphclass(PyFF_Glyph *self,PyObject *value,void *closure) {
    int gc;
    char *str = PyString_AsString(value);

    if ( str==NULL )
return( -1 );
    gc = FlagsFromString(str,glyphclasses);
    if ( gc==0x80000000 )
return( -1 );
    self->sc->glyph_class = gc;
return( 0 );
}

static PyObject *PyFF_Glyph_get_a_layer(PyFF_Glyph *self,int layeri) {
    SplineChar *sc = self->sc;
    Layer *layer;
    PyFF_Layer *ly;

    if ( layeri<-1 || layeri>=sc->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Bad layer" );
return( NULL );
    } else if ( layeri==-1 )
	layer = &sc->parent->grid;
    else
	layer = &sc->layers[layeri];
    ly = LayerFromLayer(layer,NULL);
    ly->is_quadratic = sc->parent->order2;	/* If the layer is empty we won't know */

return( (PyObject * ) ly );
}

static int PyFF_Glyph_set_a_layer(PyFF_Glyph *self,PyObject *value,void *closure, int layeri) {
    SplineChar *sc = self->sc;
    Layer *layer;
    SplineSet *ss, *newss;
    int isquad;

    if ( layeri<-1 || layeri>=sc->layer_cnt ) {
	PyErr_Format(PyExc_ValueError, "Bad layer" );
return( -1 );
    } else if ( layeri==-1 )
	layer = &sc->parent->grid;
    else
	layer = &sc->layers[layeri];
    if ( PyType_IsSubtype(&PyFF_LayerType,value->ob_type) ) {
	isquad = ((PyFF_Layer *) value)->is_quadratic;
	ss = SSFromLayer( (PyFF_Layer *) value);
    } else if ( PyType_IsSubtype(&PyFF_ContourType,value->ob_type) ) {
	isquad = ((PyFF_Contour *) value)->is_quadratic;
	ss = SSFromContour( (PyFF_Contour *) value,NULL);
    } else {
	PyErr_Format(PyExc_TypeError, "Argument must be a layer or a contour" );
return( -1 );
    }
    if ( sc->parent->order2!=isquad ) {
	if ( sc->parent->order2 )
	    newss = SplineSetsTTFApprox(ss);
	else
	    newss = SplineSetsPSApprox(ss);
	SplinePointListsFree(ss);
	ss = newss;
    }
    SplinePointListsFree(layer->splines);
    layer->splines = ss;

    SCCharChangedUpdate(sc);
return( 0 );
}

static PyObject *PyFF_Glyph_get_foreground(PyFF_Glyph *self,void *closure) {
return( PyFF_Glyph_get_a_layer(self,ly_fore));
}

static int PyFF_Glyph_set_foreground(PyFF_Glyph *self,PyObject *value,void *closure) {
return( PyFF_Glyph_set_a_layer(self,value,closure,ly_fore));
}

static PyObject *PyFF_Glyph_get_background(PyFF_Glyph *self,void *closure) {
return( PyFF_Glyph_get_a_layer(self,ly_back));
}

static int PyFF_Glyph_set_background(PyFF_Glyph *self,PyObject *value,void *closure) {
return( PyFF_Glyph_set_a_layer(self,value,closure,ly_back));
}

static PyObject *PyFF_Glyph_get_hints(StemInfo *head) {
    StemInfo *h;
    int cnt;
    PyObject *tuple;

    for ( h=head, cnt=0; h!=NULL; h=h->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( h=head, cnt=0; h!=NULL; h=h->next, ++cnt ) {
	double start, width;
	start = h->start; width = h->width;
	if ( h->ghost && width>0 ) {
	    start += width;
	    width = -width;
	}
	PyTuple_SetItem(tuple,cnt,Py_BuildValue("(dd)", start, width ));
    }

return( tuple );
}

static int PyFF_Glyph_set_hints(PyFF_Glyph *self,int is_v,PyObject *value) {
    SplineChar *sc = self->sc;
    StemInfo *head=NULL, *tail=NULL, *cur;
    int i, cnt;
    double start, width;
    StemInfo **_head = is_v ? &sc->vstem : &sc->hstem;

    cnt = PySequence_Size(value);
    if ( cnt==-1 )
return( -1 );
    for ( i=0; i<cnt; ++i ) {
	if ( !PyArg_ParseTuple(PySequence_GetItem(value,i),"(dd)", &start, &width ))
return( -1 );
	cur = chunkalloc(sizeof(StemInfo));
	if ( width==-20 || width==-21 )
	    cur->ghost = true;
	if ( width<0 ) {
	    start += width;
	    width = -width;
	}
	cur->start = start;
	cur->width = width;
	if ( tail==NULL )
	    head = cur;
	else
	    tail->next = cur;
	tail = cur;
    }

    StemInfosFree(*_head);
    SCClearHintMasks(sc,true);
    *_head = HintCleanup(head,true,1);
    if ( is_v ) {
	SCGuessVHintInstancesList(sc);
	sc->vconflicts = StemListAnyConflicts(sc->vstem);
    } else {
	SCGuessHHintInstancesList(sc);
	sc->hconflicts = StemListAnyConflicts(sc->hstem);
    }

    SCCharChangedUpdate(sc);
return( 0 );
}

static PyObject *PyFF_Glyph_get_hhints(PyFF_Glyph *self,void *closure) {
return( PyFF_Glyph_get_hints(self->sc->hstem));
}

static int PyFF_Glyph_set_hhints(PyFF_Glyph *self,PyObject *value,void *closure) {
return( PyFF_Glyph_set_hints(self,false,value));
}

static PyObject *PyFF_Glyph_get_vhints(PyFF_Glyph *self,void *closure) {
return( PyFF_Glyph_get_hints(self->sc->vstem));
}

static int PyFF_Glyph_set_vhints(PyFF_Glyph *self,PyObject *value,void *closure) {
return( PyFF_Glyph_set_hints(self,true,value));
}

static PyObject *PyFF_Glyph_get_comment(PyFF_Glyph *self,void *closure) {
    if ( self->sc->comment==NULL )
return( Py_BuildValue("s", "" ));
    else
return( PyUnicode_DecodeUTF8(self->sc->comment,strlen(self->sc->comment),NULL));
}

static int PyFF_Glyph_set_comment(PyFF_Glyph *self,PyObject *value,void *closure) {
    char *newv;
    PyObject *temp;

    if ( PyUnicode_Check(value)) {
	/* Need to force utf8 encoding rather than accepting the "default" */
	/*  which would happen if we treated unicode as a string */
	temp = PyUnicode_AsUTF8String(value);
	newv = copy( PyString_AsString(temp));
	Py_DECREF(temp);
    } else
	newv = copy( PyString_AsString(value));
    if ( newv==NULL )
return( -1 );
    free(self->sc->comment);
    self->sc->comment = newv;
return( 0 );
}

static struct flaglist ap_types[] = {
    { "mark", at_mark },
    { "base", at_basechar },
    { "ligature", at_baselig },
    { "basemark", at_basemark },
    { "entry", at_centry },
    { "exit", at_cexit },
    NULL };

static PyObject *PyFF_Glyph_get_anchorPoints(PyFF_Glyph *self,void *closure) {
    SplineChar *sc = self->sc;
    AnchorPoint *ap;
    int cnt;
    PyObject *tuple;

    for ( ap=sc->anchor, cnt=0; ap!=NULL; ap=ap->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( ap=sc->anchor, cnt=0; ap!=NULL; ap=ap->next, ++cnt ) {
	if ( ap->type == at_baselig )
	    PyTuple_SetItem(tuple,cnt,Py_BuildValue("(ssddi)", ap->anchor->name,
		    ap_types[ap->type].name, ap->me.x, ap->me.y, ap->lig_index ));
	else
	    PyTuple_SetItem(tuple,cnt,Py_BuildValue("(ssdd)", ap->anchor->name,
		    ap_types[ap->type].name, ap->me.x, ap->me.y ));
    }

return( tuple );
}

static AnchorPoint *APFromTuple(SplineChar *sc,PyObject *tuple) {
    char *ac_name, *type;
    double x, y;
    int lig_index=-1;
    AnchorPoint *ap;
    AnchorClass *ac;
    SplineFont *sf = sc->parent;
    int aptype;

    if ( !PyArg_ParseTuple(tuple, "ssdd|i", &ac_name, &type, &x, &y, &lig_index ))
return( NULL );
    aptype = FlagsFromString(type,ap_types);
    if ( aptype==0x80000000 )
return( NULL );
    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( strcmp(ac->name,ac_name)==0 )
    break;
    }
    if ( ac==NULL ) {
	PyErr_Format(PyExc_TypeError, "No anchor class named %s", ac_name);
return( NULL );
    }
    switch ( ac->type ) {
      case act_mark:
	if ( aptype!=at_mark && aptype!=at_basechar ) {
	    PyErr_Format(PyExc_TypeError, "You must specify either a mark or a base anchor type for this anchor class, %s.", ac_name );
return( NULL );
	}
      break;
      case act_mkmk:
	if ( aptype!=at_mark && aptype!=at_basemark ) {
	    PyErr_Format(PyExc_TypeError, "You must specify either a mark or a base mark anchor type for this anchor class, %s.", ac_name );
return( NULL );
	}
      break;
      case act_mklg:
	if ( aptype!=at_mark && aptype!=at_baselig ) {
	    PyErr_Format(PyExc_TypeError, "You must specify either a mark or a ligature anchor type for this anchor class, %s.", ac_name );
return( NULL );
	}
      break;
      case act_curs:
	if ( aptype!=at_centry && aptype!=at_cexit ) {
	    PyErr_Format(PyExc_TypeError, "You must specify either an entry or an exit anchor type for this anchor class, %s.", ac_name );
return( NULL );
	}
      break;
    }
    if ( lig_index==-1 && aptype==at_baselig ) {
	PyErr_Format(PyExc_TypeError, "You must specify a ligature index for a ligature anchor point" );
return( NULL );
    } else if ( lig_index!=-1 && aptype!=at_baselig ) {
	PyErr_Format(PyExc_TypeError, "You may not specify a ligature index for a non-ligature anchor point" );
return( NULL );
    }

    ap = chunkalloc(sizeof(AnchorPoint));
    ap->anchor = ac;
    ap->type = aptype;
    ap->me.x = x;
    ap->me.y = y;
    if ( aptype==at_baselig )
	ap->lig_index = lig_index;
return( ap );
}

static int PyFF_Glyph_set_anchorPoints(PyFF_Glyph *self,PyObject *value,void *closure) {
    AnchorPoint *aphead=NULL, *aplast = NULL, *ap;
    int i;
    SplineChar *sc = self->sc;

    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Expected a tuple of anchor points" );
return( -1 );
    }
    
    for ( i=0; i<PySequence_Size(value); ++i ) {
	ap = APFromTuple(sc,PySequence_GetItem(value,i));
	if ( ap==NULL )
return( -1 );
	if ( aphead==NULL )
	    aphead = ap;
	else
	    aplast->next = ap;
	aplast = ap;
    }
    AnchorPointsFree(sc->anchor);
    sc->anchor = aphead;
return( 0 );
}

static PyObject *PyFF_Glyph_get_color(PyFF_Glyph *self,void *closure) {
return( Py_BuildValue("i", self->sc->color ));
}

static int PyFF_Glyph_set_color(PyFF_Glyph *self,PyObject *value,void *closure) {
    int val;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    self->sc->color = val;
return( 0 );
}

static PyGetSetDef PyFF_Glyph_getset[] = {
    {"userdata",
	 (getter)PyFF_Glyph_get_userdata, (setter)PyFF_Glyph_set_userdata,
	 "arbetrary user data", NULL},
    {"anchorPoints",
	 (getter)PyFF_Glyph_get_anchorPoints, (setter)PyFF_Glyph_set_anchorPoints,
	 "glyph name", NULL},
    {"glyphname",
	 (getter)PyFF_Glyph_get_glyphname, (setter)PyFF_Glyph_set_glyphname,
	 "glyph name", NULL},
    {"unicode",
	 (getter)PyFF_Glyph_get_unicode, (setter)PyFF_Glyph_set_unicode,
	 "Unicode code point for this glyph, or -1", NULL},
    {"foreground",
	 (getter)PyFF_Glyph_get_foreground, (setter)PyFF_Glyph_set_foreground,
	 "Returns the foreground layer of the glyph", NULL},
    {"background",
	 (getter)PyFF_Glyph_get_background, (setter)PyFF_Glyph_set_background,
	 "Returns the foreground layer of the glyph", NULL},
    {"references",
	 (getter)PyFF_Glyph_get_references, (setter)PyFF_Glyph_set_references,
	 "A tuple of all references in the glyph", NULL},
    {"color",
	 (getter)PyFF_Glyph_get_color, (setter)PyFF_Glyph_set_color,
	 "Glyph color", NULL},
    {"comment",
	 (getter)PyFF_Glyph_get_comment, (setter)PyFF_Glyph_set_comment,
	 "Glyph comment", NULL},
    {"glyphclass",
	 (getter)PyFF_Glyph_get_glyphclass, (setter)PyFF_Glyph_set_glyphclass,
	 "glyph class", NULL},
    {"texheight",
	 (getter)PyFF_Glyph_get_texheight, (setter)PyFF_Glyph_set_texheight,
	 "TeX glyph height", NULL},
    {"texdepth",
	 (getter)PyFF_Glyph_get_texdepth, (setter)PyFF_Glyph_set_texdepth,
	 "TeX glyph depth", NULL},
    {"ttinstrs",
	 (getter)PyFF_Glyph_get_ttfinstrs, (setter)PyFF_Glyph_set_ttfinstrs,
	 "TrueType Instructions for this glyph", NULL},
    {"changed",
	 (getter)PyFF_Glyph_get_changed, (setter)PyFF_Glyph_set_changed,
	 "Flag indicating whether this glyph has changed", NULL},
    {"originalgid",
	 (getter)PyFF_Glyph_get_originalgid, (setter)PyFF_cant_set,
	 "Original GID", NULL},
    {"width",
	 (getter)PyFF_Glyph_get_width, (setter)PyFF_Glyph_set_width,
	 "Glyph's advance width", NULL},
    {"left_side_bearing",
	 (getter)PyFF_Glyph_get_lsb, (setter)PyFF_Glyph_set_lsb,
	 "Glyph's left side bearing", NULL},
    {"right_side_bearing",
	 (getter)PyFF_Glyph_get_rsb, (setter)PyFF_Glyph_set_rsb,
	 "Glyph's right side bearing", NULL},
    {"vwidth",
	 (getter)PyFF_Glyph_get_vwidth, (setter)PyFF_Glyph_set_vwidth,
	 "Glyph's vertical advance width", NULL},
    {"font",
	 (getter)PyFF_Glyph_get_font, (setter)PyFF_cant_set,
	 "Font containing the glyph", NULL},
    {"hhints",
	 (getter)PyFF_Glyph_get_hhints, (setter)PyFF_Glyph_set_hhints,
	 "The horizontal hints of the glyph as a tuple, one entry per hint. Each hint is itself a tuple containing the start location and width of the hint", NULL},
    {"vhints",
	 (getter)PyFF_Glyph_get_vhints, (setter)PyFF_Glyph_set_vhints,
	 "The vertical hints of the glyph as a tuple, one entry per hint. Each hint is itself a tuple containing the start location and width of the hint", NULL},
    {NULL}  /* Sentinel */
};

/* ************************************************************************** */
/*  Glyph Methods  */
/* ************************************************************************** */

static PyObject *PyFFGlyph_Build(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    if ( SFIsSomethingBuildable(sc->parent,sc,false) )
	SCBuildComposit(sc->parent,sc,true,sc->parent->fv);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_canonicalContours(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    CanonicalContours(sc);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_canonicalStart(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    SPLsStartToLeftmost(sc);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_AddReference(PyObject *self, PyObject *args) {
    double m[6];
    real transform[6];
    char *str;
    SplineChar *sc = ((PyFF_Glyph *) self)->sc, *rsc;
    SplineFont *sf = sc->parent;
    int j;

    memset(m,0,sizeof(m));
    m[0] = m[3] = 1; 
    if ( !PyArg_ParseTuple(args,"s|(dddddd)",&str,
	    &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) )
return( NULL );
    rsc = SFGetChar(sf,-1,str);
    if ( rsc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No glyph named %s", str);
return( NULL );
    }
    for ( j=0; j<6; ++j )
	transform[j] = m[j];
    _SCAddRef(sc,rsc,transform);

Py_RETURN( self );
}

static PyObject *PyFFGlyph_addAnchorPoint(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    AnchorPoint *ap = APFromTuple(sc,args);

    if ( ap==NULL )
return( NULL );
    ap->next = sc->anchor;
    sc->anchor = ap;

Py_RETURN( self );
}

static char *glyphpen_keywords[] = { "replace", NULL };
static PyObject *PyFFGlyph_GlyphPen(PyObject *self, PyObject *args, PyObject *keywds) {
    int replace = true;
    PyObject *gp;

    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "|i", glyphpen_keywords,
	    &replace ))
return( NULL );
    gp = PyFF_GlyphPenType.tp_alloc(&PyFF_GlyphPenType,0);
    ((PyFF_GlyphPen *) gp)->sc = ((PyFF_Glyph *) self)->sc;
    ((PyFF_GlyphPen *) gp)->replace = replace;
    ((PyFF_GlyphPen *) gp)->ended = true;
    ((PyFF_GlyphPen *) gp)->changed = false;
    /* tp_alloc increments the reference count for us */
return( gp );
}

static PyObject *PyFFGlyph_draw(PyObject *self, PyObject *args) {
    PyObject *layer, *result, *pen;
    RefChar *ref;
    PyObject *tuple;

    if ( !PyArg_ParseTuple(args,"O", &pen ) )
return( NULL );

    layer = PyFF_Glyph_get_a_layer((PyFF_Glyph *) self,ly_fore);
    result = PyFFLayer_draw( (PyFF_Layer *) layer,args);
    Py_XDECREF(layer);

    for ( ref = ((PyFF_Glyph *) self)->sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	tuple = Py_BuildValue("s(dddddd)", ref->sc->name,
		ref->transform[0], ref->transform[1], ref->transform[2],
		ref->transform[3], ref->transform[4], ref->transform[5]);
	do_pycall(pen,"addComponent",tuple);
    }
return( result );
}

static PyObject *PyFFGlyph_addHint(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int is_v;
    double start, width;
    StemInfo *h;

    if ( !PyArg_ParseTuple(args,"idd", &is_v, &start, &width ) )
return( NULL );

    h = chunkalloc(sizeof(StemInfo));
    if ( width==-20 || width==-21 )
	h->ghost = true;
    if ( width<0 ) {
	start += width;
	width = -width;
    }
    h->start = start;
    h->width = width;
    if ( is_v ) {
	SCGuessVHintInstancesAndAdd(sc,h,0x80000000,0x80000000);
	h->next = sc->vstem;
	sc->vstem = HintCleanup(h,true,1);
	sc->vconflicts = StemListAnyConflicts(sc->vstem);
    } else {
	SCGuessHHintInstancesAndAdd(sc,h,0x80000000,0x80000000);
	h->next = sc->hstem;
	sc->hstem = HintCleanup(h,true,1);
	sc->hconflicts = StemListAnyConflicts(sc->hstem);
    }
Py_RETURN( self );
}

static PyObject *PyFFGlyph_autoHint(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    SplineCharAutoHint(sc,NULL);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( !no_windowing_ui )
	SCUpdateAll(sc);
#endif
Py_RETURN( self );
}

static PyObject *PyFFGlyph_autoInstr(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    SCAutoInstr(sc,NULL);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_autoTrace(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    char **at_args;

    at_args = AutoTraceArgs(false);
    if ( at_args==(char **) -1 ) {
	PyErr_Format(PyExc_EnvironmentError, "Bad autotrace args" );
return(NULL);
    }
    _SCAutoTrace(sc, at_args);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_import(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    char *filename;
    char *locfilename = NULL, *pt;

    if ( !PyArg_ParseTuple(args,"es","UTF-8",&filename) )
return( NULL );
    locfilename = utf82def_copy(filename);
    free(filename);

    pt = strrchr(locfilename,'.');
    if ( pt==NULL ) pt=locfilename;

    if ( strcasecmp(pt,".eps")==0 || strcasecmp(pt,".ps")==0 || strcasecmp(pt,".art")==0 )
	SCImportPS(sc,ly_fore,locfilename,false,0);
#ifndef _NO_LIBXML
    else if ( strcasecmp(pt,".svg")==0 )
	SCImportSVG(sc,ly_fore,locfilename,NULL,0,false);
    else if ( strcasecmp(pt,".glif")==0 )
	SCImportGlif(sc,ly_fore,locfilename,NULL,0,false);
#endif
    /* else if ( strcasecmp(pt,".fig")==0 )*/
    else {
	GImage *image = GImageRead(locfilename);
	if ( image==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "Could not load image file %s", locfilename );
return(NULL);
	}
	SCAddScaleImage(sc,image,false,ly_back);
    }
    free( locfilename );
Py_RETURN( self );
}

static PyObject *PyFFGlyph_export(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    char *filename;
    char *locfilename = NULL;
    char *pt;
    int pixels=100, bits=8;
    int format= -1;
    FILE *file;

    if ( !PyArg_ParseTuple(args,"es|ii","UTF-8",&filename,&pixels,&bits) )
return( NULL );
    locfilename = utf82def_copy(filename);
    free(filename);

    pt = strrchr(locfilename,'.');
    if ( pt==NULL ) pt=locfilename;
    if ( strcasecmp(pt,".xbm")==0 ) {
	format=0; bits=1;
    } else if ( strcasecmp(pt,".bmp")==0 )
	format=1;
    else if ( strcasecmp(pt,".png")==0 )
	format=2;

    if ( format!=-1 ) {
	if ( !ExportImage(locfilename,sc,format,pixels,bits)) {
	    PyErr_Format(PyExc_EnvironmentError, "Could not create image file %s", locfilename );
return( NULL );
	}
    } else {
	file = fopen( locfilename,"w");
	if ( file==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "Could not create file %s", locfilename );
return( NULL );
	}

	if ( strcasecmp(pt,".eps")==0 || strcasecmp(pt,".ps")==0 || strcasecmp(pt,".art")==0 )
	    _ExportEPS(file,sc,true);
	else if ( strcasecmp(pt,".pdf")==0 )
	    _ExportPDF(file,sc);
	else if ( strcasecmp(pt,".svg")==0 )
	    _ExportSVG(file,sc);
	else if ( strcasecmp(pt,".glif")==0 )
	    _ExportGlif(file,sc);
	/* else if ( strcasecmp(pt,".fig")==0 )*/
	else {
	    PyErr_Format(PyExc_TypeError, "Unknown extension to export: %s", pt );
	    free( locfilename );
return( NULL );
	}
	fclose(file);
    }
    free( locfilename );
Py_RETURN( self );
}

static PyObject *PyFFGlyph_unlinkRef(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    char *refname=NULL;
    RefChar *ref;
    int any = false;

    if ( !PyArg_ParseTuple(args,"|s",&refname) )
return( NULL );
    for ( ref = sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	if ( refname==NULL || strcmp(ref->sc->name,refname)==0 ) {
	    SCRefToSplines(sc,ref);
	    any = true;
	}
    }

    if ( !any && refname!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No reference named %s found in glyph %s", refname, sc->name );
return( NULL );
    }
    if ( any )
	SCCharChangedUpdate(sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_unlinkThisGlyph(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;

    UnlinkThisReference(NULL,sc);
Py_RETURN( self );
}

static PyObject *TupleOfGlyphNames(char *str,int extras) {
    int cnt;
    char *pt, *start;
    PyObject *tuple;
    int ch;

    for ( pt=str; *pt==' '; ++pt );
    if ( *pt=='\0' )
return( PyTuple_New(extras));

    for ( cnt=1; *pt; ++pt ) {
	if ( *pt==' ' ) {
	    ++cnt;
	    while ( pt[1]==' ' ) ++pt;
	}
    }
    tuple = PyTuple_New(extras+cnt);
    for ( pt=str, cnt=0; *pt; ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt=='\0' )
    break;
	start = pt;
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	ch = *pt; *pt = '\0';
	PyTuple_SetItem(tuple,extras+cnt,PyString_FromString(start));
	*pt = ch;
	++cnt;
    }
return( tuple );
}

static char *GlyphNamesFromTuple(PyObject *glyphs) {
    int cnt, len;
    char *str, *ret, *pt;
    int i;

    if ( PyString_Check(glyphs))
return( copy( PyString_AsString(glyphs)) );

    if ( !PySequence_Check(glyphs) ) {
	PyErr_Format(PyExc_TypeError,"Expected tuple of glyph names");
return(NULL );
    }
    cnt = PySequence_Size(glyphs);
    len = 0;
    for ( i=0; i<cnt; ++i ) {
	str = PyString_AsString(PySequence_GetItem(glyphs,i));
	if ( str==NULL ) {
	    PyErr_Format(PyExc_TypeError,"Expected tuple of glyph names");
return( NULL );
	}
	len += strlen(str)+1;
    }

    ret = pt = galloc(len+1);
    for ( i=0; i<cnt; ++i ) {
	str = PyString_AsString(PySequence_GetItem(glyphs,i));
	strcpy(pt,str);
	pt += strlen(pt);
	*pt++ = ' ';
    }
    if ( pt!=ret )
	--pt;
    *pt = '\0';
return( ret );
}

static PyObject *PyFFGlyph_getPosSub(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    SplineFont *sf = sc->parent, *sf_sl = sf;
    int i, j, cnt;
    PyObject *ret, *temp;
    PST *pst;
    KernPair *kp;
    struct lookup_subtable *sub;
    char *subname;

    if ( sf_sl->cidmaster!=NULL ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    subname = PyString_AsString(PySequence_GetItem(args,0));
    if ( subname==NULL )
return( NULL );
    if ( *subname=='*' )
	sub = NULL;
    else {
	sub = SFFindLookupSubtable(sf,subname);
	if ( sub==NULL ) {
	    PyErr_Format(PyExc_KeyError, "Unknown lookup subtable: %s",subname);
return( NULL );
	}
    }

    for ( i=0; i<2; ++i ) {
	cnt = 0;
	for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type==pst_lcaret )
	continue;
	    if ( pst->subtable == sub || sub==NULL ) {
		if ( i ) {
		    switch ( pst->type ) {
		      default:
			Py_INCREF(Py_None);
			PyTuple_SetItem(ret,i,Py_None);
/* The important things here should not be translated. We hope the user will */
/*  never see this. Let's not translate it at all */
			LogError( "Unexpected PST type in GetPosSub (%d).\n", pst->type );
		      break;
		      case pst_position:
			PyTuple_SetItem(ret,cnt,Py_BuildValue("(ssiiii)",
				pst->subtable->subtable_name,"Position",
			        pst->u.pos.xoff,pst->u.pos.yoff,
			        pst->u.pos.h_adv_off, pst->u.pos.v_adv_off ));
		      break;
		      case pst_pair:
			PyTuple_SetItem(ret,cnt,Py_BuildValue("(sssiiiiiiii)",
				pst->subtable->subtable_name,"Pair",pst->u.pair.paired,
			        pst->u.pair.vr[0].xoff,pst->u.pair.vr[0].yoff,
			        pst->u.pair.vr[0].h_adv_off, pst->u.pair.vr[0].v_adv_off,
			        pst->u.pair.vr[1].xoff,pst->u.pair.vr[1].yoff,
			        pst->u.pair.vr[1].h_adv_off, pst->u.pair.vr[1].v_adv_off ));
		      break;
		      case pst_substitution:
			PyTuple_SetItem(ret,cnt,Py_BuildValue("(sss)",
				pst->subtable->subtable_name,"Substitution",pst->u.subs.variant));
		      break;
		      case pst_alternate:
		      case pst_multiple:
		      case pst_ligature:
			temp = TupleOfGlyphNames(pst->u.mult.components,2);
			PyTuple_SetItem(temp,0,PyString_FromString(pst->subtable->subtable_name));
			PyTuple_SetItem(temp,1,PyString_FromString(
				pst->type==pst_alternate?"AltSubs":
				pst->type==pst_multiple?"MultSubs":
			                    "Ligature"));
			PyTuple_SetItem(ret,cnt,temp);
		      break;
		    }
		}
		++cnt;
	    }
	}
	for ( j=0; j<2; ++j ) {
	    if ( sub==NULL || sub->lookup->lookup_type==gpos_pair ) {
		for ( kp= (j==0 ? sc->kerns : sc->vkerns); kp!=NULL; kp=kp->next ) {
		    if ( sub==NULL || sub==kp->subtable ) {
			if ( i ) {
			    int xadv1, yadv1, xadv2;
			    xadv1 = yadv1 = xadv2 = 0;
			    if ( j )
				yadv1 = kp->off;
			    else if ( SCRightToLeft(sc))
				xadv2 = kp->off;
			    else
				xadv1 = kp->off;
			    PyTuple_SetItem(ret,cnt,Py_BuildValue("(sssiiiiiiii)",
				    kp->subtable->subtable_name,"Pair",kp->sc->name,
			            0,0,xadv1,yadv1,
			            0,0,xadv2,0));
			}
			++cnt;
		    }
		}
	    }
	}
	if ( i==0 )
	    ret = PyTuple_New(cnt);
    }
return( ret );
}

static PyObject *PyFFGlyph_removePosSub(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    SplineFont *sf = sc->parent, *sf_sl = sf;
    int is_v;
    PST *pst, *next, *prev;
    KernPair *kp, *kpnext, *kpprev;
    struct lookup_subtable *sub;
    char *subname;

    if ( sf_sl->cidmaster!=NULL ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    subname = PyString_AsString(PySequence_GetItem(args,0));
    if ( subname==NULL )
return( NULL );
    if ( *subname=='*' )
	sub = NULL;
    else {
	sub = SFFindLookupSubtable(sf,subname);
	if ( sub==NULL ) {
	    PyErr_Format(PyExc_KeyError, "Unknown lookup subtable: %s",subname);
return( NULL );
	}
    }

    for ( prev=NULL, pst = sc->possub; pst!=NULL; pst=next ) {
	next = pst->next;
	if ( pst->type==pst_lcaret ) {
	    prev = pst;
    continue;
	}
	if ( pst->subtable == sub || sub==NULL ) {
	    if ( prev==NULL )
		sc->possub = next;
	    else
		prev->next = next;
	    pst->next = NULL;
	    PSTFree(pst);
	} else
	    prev = pst;
    }
    for ( is_v=0; is_v<2; ++is_v ) {
	for ( kpprev=NULL, kp = is_v ? sc->vkerns: sc->kerns; kp!=NULL; kp=kpnext ) {
	    kpnext = kp->next;
	    if ( kp->subtable == sub || sub==NULL ) {
		if ( kpprev!=NULL )
		    kpprev->next = kpnext;
		else if ( is_v )
		    sc->vkerns = kpnext;
		else
		    sc->kerns = kpnext;
		kp->next = NULL;
		KernPairsFree(kp);
	    } else
		kpprev = kp;
	}
    }
Py_RETURN( self );
}

static PyObject *PyFFGlyph_addPosSub(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc, *osc;
    SplineFont *sf = sc->parent, *sf_sl = sf;
    PST temp, *pst;
    struct lookup_subtable *sub;
    char *subname, *other;
    KernPair *kp;
    PyObject *others;

    memset(&temp,0,sizeof(temp));

    if ( sf_sl->cidmaster!=NULL ) sf_sl = sf_sl->cidmaster;
    else if ( sf_sl->mm!=NULL ) sf_sl = sf_sl->mm->normal;

    subname = PyString_AsString(PySequence_GetItem(args,0));
    if ( subname==NULL )
return( NULL );
    sub = SFFindLookupSubtable(sf,subname);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_KeyError, "Unknown lookup subtable: %s",subname);
return( NULL );
    }

    temp.subtable = sub;

    if ( sub->lookup->lookup_type==gpos_single ) {
	if ( !PyArg_ParseTuple(args,"(shhhh)", &subname,
		&temp.u.pos.xoff, &temp.u.pos.yoff,
		&temp.u.pos.h_adv_off, &temp.u.pos.v_adv_off))
return( NULL );
	temp.type = pst_position;
    } else if ( sub->lookup->lookup_type==gpos_pair ) {
	temp.type = pst_pair;
	temp.u.pair.vr = chunkalloc(sizeof(struct vr [2]));
	if ( !PyArg_ParseTuple(args,"(sshhhhhhhh)", &subname, &other,
		&temp.u.pair.vr[0].xoff, &temp.u.pair.vr[0].yoff,
		&temp.u.pair.vr[0].h_adv_off, &temp.u.pair.vr[0].v_adv_off,
		&temp.u.pair.vr[1].xoff, &temp.u.pair.vr[1].yoff,
		&temp.u.pair.vr[1].h_adv_off, &temp.u.pair.vr[1].v_adv_off))
return( NULL );
	if ( temp.u.pair.vr[0].xoff==0 && temp.u.pair.vr[0].yoff==0 &&
		temp.u.pair.vr[1].xoff==0 && temp.u.pair.vr[1].yoff==0 &&
		temp.u.pair.vr[1].v_adv_off==0 ) {
	    int off =0x7fffffff;
	    if ( temp.u.pair.vr[0].h_adv_off==0 && temp.u.pair.vr[1].h_adv_off==0 &&
		    sub->vertical_kerning )
		off = temp.u.pair.vr[0].v_adv_off;
	    else if ( temp.u.pair.vr[0].h_adv_off==0 && temp.u.pair.vr[0].v_adv_off==0 &&
		    SCRightToLeft(sc))
		off = temp.u.pair.vr[1].h_adv_off;
	    else if ( temp.u.pair.vr[0].v_adv_off==0 && temp.u.pair.vr[1].h_adv_off==0 )
		off = temp.u.pair.vr[0].h_adv_off;
	    if ( off!=0x7fffffff && (osc=SFGetChar(sf,-1,other))!=NULL ) {
		chunkfree(temp.u.pair.vr,sizeof(struct vr [2]));
		kp = chunkalloc(sizeof(KernPair));
		kp->sc = osc;
		kp->off = off;
		kp->subtable = sub;
		if ( sub->vertical_kerning ) {
		    kp->next = sc->vkerns;
		    sc->vkerns = kp;
		} else {
		    kp->next = sc->kerns;
		    sc->kerns = kp;
		}
Py_RETURN( self );
	    }
	}
	temp.u.pair.paired = copy(other);
    } else if ( sub->lookup->lookup_type==gsub_single ) {
	if ( !PyArg_ParseTuple(args,"(ss)", &subname, &other))
return( NULL );
	temp.type = pst_substitution;
	temp.u.subs.variant = copy(other);
    } else {
	if ( !PyArg_ParseTuple(args,"(sO)", &subname, &others))
return( NULL );
	other = GlyphNamesFromTuple(others);
	if ( other==NULL )
return( NULL );
	if ( sub->lookup->lookup_type>=gsub_alternate )
	    temp.type = pst_alternate;
	else if ( sub->lookup->lookup_type>=gsub_multiple )
	    temp.type = pst_multiple;
	else if ( sub->lookup->lookup_type>=gsub_ligature )
	    temp.type = pst_ligature;
	else {
	    PyErr_Format(PyExc_KeyError, "Unexpected lookup type: %s",sub->lookup->lookup_name);
return( NULL );
	}
	temp.u.subs.variant = other;
    }
    pst = chunkalloc(sizeof(PST));
    *pst = temp;
    pst->next = sc->possub;
    sc->possub = pst;
Py_RETURN( self );
}

static PyObject *PyFFGlyph_selfIntersects(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    Spline *s, *s2;
    PyObject *ret;
    SplineSet *ss;

    ss = LayerAllSplines(&sc->layers[ly_fore]);
    ret = SplineSetIntersect(ss,&s,&s2) ? Py_True : Py_False;
    LayerUnAllSplines(&sc->layers[ly_fore]);
    Py_INCREF( ret );
return( ret );
}

static struct flaglist trans_flags[] = {
    { "partialRefs", fvt_partialreftrans },
    { "round", fvt_round_to_int },
    NULL };

static PyObject *PyFFGlyph_Transform(PyObject *self, PyObject *args) {
    SplineChar *sc = ((PyFF_Glyph *) self)->sc;
    int i;
    double m[6];
    real t[6];
    int flags;
    PyObject *flagO=NULL;

    if ( !PyArg_ParseTuple(args,"(dddddd)|O",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5],
	    &flagO) )
return( NULL );
    flags = FlagsFromTuple(flagO,trans_flags);
    if ( flags==0x80000000 )
return( NULL );
    flags |= fvt_dobackground;
    for ( i=0; i<6; ++i )
	t[i] = m[i];
    FVTrans(sc->parent->fv,sc,t,NULL,flags);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Simplify(PyFF_Glyph *self, PyObject *args) {
    static struct simplifyinfo smpl = { sf_normal,.75,.2,10 };
    SplineChar *sc = self->sc;
    SplineFont *sf = sc->parent;
    int em = sf->ascent+sf->descent;

    smpl.err = em/1000.;
    smpl.linefixup = em/500.;
    smpl.linelenmax = em/100.;

    if ( PySequence_Size(args)>=1 )
	smpl.err = PyFloat_AsDouble(PySequence_GetItem(args,0));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=2 )
	smpl.flags = FlagsFromTuple( PySequence_GetItem(args,1),simplifyflags);
    if ( !PyErr_Occurred() && PySequence_Size(args)>=3 )
	smpl.tan_bounds = PyFloat_AsDouble( PySequence_GetItem(args,2));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=4 )
	smpl.linefixup = PyFloat_AsDouble( PySequence_GetItem(args,3));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=5 )
	smpl.linelenmax = PyFloat_AsDouble( PySequence_GetItem(args,4));
    if ( PyErr_Occurred() )
return( NULL );
    sc->layers[ly_fore].splines = SplineCharSimplify(sc,sc->layers[ly_fore].splines,&smpl);
    SCCharChangedUpdate(self->sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Round(PyFF_Glyph *self, PyObject *args) {
    double factor=1;

    if ( !PyArg_ParseTuple(args,"|d",&factor ) )
return( NULL );
    SCRound2Int( self->sc,factor);
    SCCharChangedUpdate(self->sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Cluster(PyFF_Glyph *self, PyObject *args) {
    double within = .1, max = .5;

    if ( !PyArg_ParseTuple(args,"|dd", &within, &max ) )
return( NULL );

    SCRoundToCluster( self->sc,ly_fore,false,within,max);
    SCCharChangedUpdate(self->sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_AddExtrema(PyFF_Glyph *self, PyObject *args) {
    int emsize = 1000;
    char *flag = NULL;
    int ae = ae_only_good;
    SplineChar *sc = self->sc;
    SplineFont *sf = sc->parent;

    if ( !PyArg_ParseTuple(args,"|si", &flag, &emsize ) )
return( NULL );
    if ( flag!=NULL )
	ae = FlagsFromString(flag,addextremaflags);

    SplineCharAddExtrema(sc,sc->layers[ly_fore].splines,ae,sf->ascent+sf->descent);
    SCCharChangedUpdate(sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Stroke(PyFF_Glyph *self, PyObject *args) {
    StrokeInfo si;
    SplineSet *newss;

    if ( Stroke_Parse(&si,args)==-1 )
return( NULL );

    newss = SSStroke(self->sc->layers[ly_fore].splines,&si,self->sc);
    SplinePointListFree(self->sc->layers[ly_fore].splines);
    self->sc->layers[ly_fore].splines = newss;
    SCCharChangedUpdate(self->sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Correct(PyFF_Glyph *self, PyObject *args) {
    int changed = false;

    self->sc->layers[ly_fore].splines = SplineSetsCorrect(self->sc->layers[ly_fore].splines,&changed);
    if ( changed )
	SCCharChangedUpdate(self->sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_RemoveOverlap(PyFF_Glyph *self, PyObject *args) {

    self->sc->layers[ly_fore].splines = SplineSetRemoveOverlap(self->sc,self->sc->layers[ly_fore].splines,over_remove);
    SCCharChangedUpdate(self->sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Intersect(PyFF_Glyph *self, PyObject *args) {

    self->sc->layers[ly_fore].splines = SplineSetRemoveOverlap(self->sc,self->sc->layers[ly_fore].splines,over_intersect);
    SCCharChangedUpdate(self->sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_Exclude(PyFF_Glyph *self, PyObject *args) {
    SplineSet *ss, *excludes, *tail;
    PyObject *obj;

    if ( !PyArg_ParseTuple(args,"O", &obj ) )
return( NULL );
    if ( !PyType_IsSubtype(&PyFF_LayerType,obj->ob_type) ) {
	PyErr_Format(PyExc_TypeError, "Value must be a (FontForge) Layer");
return( NULL );
    }

    excludes = SSFromLayer((PyFF_Layer *) obj);
    ss = self->sc->layers[ly_fore].splines;
    for ( tail=ss; tail->next!=NULL; tail=tail->next );
    tail->next = excludes;
    while ( excludes!=NULL ) {
	excludes->first->selected = true;
	excludes = excludes->next;
    }
    self->sc->layers[ly_fore].splines = SplineSetRemoveOverlap(NULL,ss,over_exclude);
    /* Frees the old splinesets */
    SCCharChangedUpdate(self->sc);
Py_RETURN( self );
}

static PyObject *PyFFGlyph_BoundingBox(PyFF_Glyph *self, PyObject *args) {
    DBounds bb;

    SplineCharFindBounds(self->sc,&bb);

return( Py_BuildValue("(dddd)", bb.minx,bb.miny, bb.maxx,bb.maxy ));
}

static PyObject *PyFFGlyph_clear(PyFF_Glyph *self, PyObject *args) {

    SCClearContents(self->sc);
    SCCharChangedUpdate(self->sc);

Py_RETURN(self);
}

static PyObject *PyFFGlyph_isWorthOutputting(PyFF_Glyph *self, PyObject *args) {
    PyObject *ret;

    ret = SCWorthOutputting(self->sc) ? Py_True : Py_False;
    Py_INCREF( ret );
return( ret );
}

static PyMethodDef PyFF_Glyph_methods[] = {
    { "glyphPen", (PyCFunction) PyFFGlyph_GlyphPen, METH_VARARGS | METH_KEYWORDS, "Create a pen object which can draw into this glyph"},
    { "draw", (PyCFunction) PyFFGlyph_draw, METH_VARARGS , "Draw the glyph's outline to the pen argument"},
    { "addExtrema", (PyCFunction) PyFFGlyph_AddExtrema, METH_VARARGS, "Add extrema to the contours of the glyph"},
    { "addReference", PyFFGlyph_AddReference, METH_VARARGS, "Add a reference"},
    { "addAnchorPoint", PyFFGlyph_addAnchorPoint, METH_VARARGS, "Adds an anchor point"},
    { "addHint", PyFFGlyph_addHint, METH_VARARGS, "Add a postscript hint (is_vertical_hint,start_pos,width)"},
    { "addPosSub", PyFFGlyph_addPosSub, METH_VARARGS, "Adds position/substitution data to the glyph"},
    { "autoHint", PyFFGlyph_autoHint, METH_NOARGS, "Guess at postscript hints"},
    { "autoInstr", PyFFGlyph_autoInstr, METH_NOARGS, "Guess (badly) at truetype instructions"},
    { "autoTrace", PyFFGlyph_autoTrace, METH_NOARGS, "Autotrace any background images"},
    { "boundingBox", (PyCFunction) PyFFGlyph_BoundingBox, METH_NOARGS, "Finds the minimum bounding box for the glyph (xmin,ymin,xmax,ymax)" },
    { "build", PyFFGlyph_Build, METH_NOARGS, "If the current glyph is an accented character\nand all components are in the font\nthen build it out of references" },
    { "canonicalContours", (PyCFunction) PyFFGlyph_canonicalContours, METH_NOARGS, "Orders the contours in the current glyph by the x coordinate of their leftmost point. (This can reduce the size of the postscript charstring needed to describe the glyph(s)."},
    { "canonicalStart", (PyCFunction) PyFFGlyph_canonicalStart, METH_NOARGS, "Sets the start point of all the contours of the current glyph to be the leftmost point on the contour."},
    { "clear", (PyCFunction) PyFFGlyph_clear, METH_NOARGS, "Clears the contents of a glyph and makes it not worth outputting" },
    { "cluster", (PyCFunction) PyFFGlyph_Cluster, METH_VARARGS, "Cluster the points of a glyph towards common values" },
    { "correctDirection", (PyCFunction) PyFFGlyph_Correct, METH_NOARGS, "Orient a layer so that external contours are clockwise and internal counter clockwise." },
    { "exclude", (PyCFunction) PyFFGlyph_Exclude, METH_VARARGS, "Exclude the area of the argument (a layer) from the current glyph"},
    { "export", PyFFGlyph_export, METH_VARARGS, "Export the glyph, the format is determined by the extension. (provide the filename of the image file)" },
    { "getPosSub", PyFFGlyph_getPosSub, METH_VARARGS, "Gets position/substitution data from the glyph"},
    { "import", PyFFGlyph_import, METH_VARARGS, "Import a background image or a foreground eps/svg/etc. (provide the filename of the image file)" },
    { "intersect", (PyCFunction) PyFFGlyph_Intersect, METH_NOARGS, "Leaves the areas where the contours of a glyph overlap."},
    { "isWorthOutputting", (PyCFunction) PyFFGlyph_isWorthOutputting, METH_NOARGS, "Returns whether the glyph is worth outputting" },
    { "removeOverlap", (PyCFunction) PyFFGlyph_RemoveOverlap, METH_NOARGS, "Remove overlapping areas from a glyph"},
    { "removePosSub", PyFFGlyph_removePosSub, METH_VARARGS, "Removes position/substitution data from the glyph"},
    { "round", (PyCFunction)PyFFGlyph_Round, METH_VARARGS, "Rounds point coordinates (and reference translations) to integers"},
    { "selfIntersects", (PyCFunction)PyFFGlyph_selfIntersects, METH_NOARGS, "Returns whether this glyph intersects itself" },
    { "simplify", (PyCFunction)PyFFGlyph_Simplify, METH_VARARGS, "Simplifies a glyph" },
    { "stroke", (PyCFunction)PyFFGlyph_Stroke, METH_VARARGS, "Strokes the countours in a glyph"},
    { "transform", (PyCFunction)PyFFGlyph_Transform, METH_VARARGS, "Transform a glyph by a 6 element matrix." },
    { "unlinkRef", PyFFGlyph_unlinkRef, METH_VARARGS, "Unlink a reference and turn it into outlines"},
    { "unlinkThisGlyph", PyFFGlyph_unlinkThisGlyph, METH_NOARGS, "Unlink all references to the current glyph in any other glyph in the font."},
    NULL
};
/* ************************************************************************** */
/*  Glyph Type  */
/* ************************************************************************** */

static PyTypeObject PyFF_GlyphType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.glyph",         /*tp_name*/
    sizeof(PyFF_Glyph),	       /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) PyFF_Glyph_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    (cmpfunc)PyFFGlyph_compare,/*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,		               /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFGlyph_Str,  /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "FontForge Glyph object",  /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    PyFF_Glyph_methods,        /* tp_methods */
    0,			       /* tp_members */
    PyFF_Glyph_getset,         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    /*(initproc)PyFF_Glyph_init*/0,  /* tp_init */
    0,                         /* tp_alloc */
    /*PyFF_Glyph_new*/0        /* tp_new */
};

/* ************************************************************************** */
/* Cvt sequence object */
/* ************************************************************************** */

static void PyFFCvt_dealloc(PyFF_Cvt *self) {
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *PyFFCvt_new(PyFF_Font *owner) {
    PyFF_Cvt *self;

    if ( owner->cvt!=NULL )
Py_RETURN( owner->cvt );
    self = PyObject_New(PyFF_Cvt, &PyFF_CvtType);
    ((PyFF_Cvt *) self)->sf = owner->fv->sf;
    ((PyFF_Cvt *) self)->cvt = SFFindTable(self->sf,CHR('c','v','t',' '));
    owner->cvt = self;
Py_RETURN( self );
}

static PyObject *PyFFCvt_Str(PyFF_Cvt *self) {
return( PyString_FromFormat( "<cvt table for font %s>", self->sf->fontname ));
}

/* ************************************************************************** */
/* Cvt sequence */
/* ************************************************************************** */

static int PyFFCvt_Length( PyObject *self ) {
    if ( ((PyFF_Cvt *) self)->cvt==NULL )
return( 0 );
    else
return( ((PyFF_Cvt *) self)->cvt->len/2 );
}

static struct ttf_table *BuildCvt(SplineFont *sf,int initial_size) {
    struct ttf_table *cvt;

    cvt = chunkalloc(sizeof(struct ttf_table));
    cvt->next = sf->ttf_tables;
    sf->ttf_tables = cvt;
    cvt->tag = CHR('c','v','t',' ');
    cvt->data = galloc(initial_size);
    cvt->len = 0;
    cvt->maxlen = initial_size;
return( cvt );
}

static PyObject *PyFFCvt_Concat( PyObject *_c1, PyObject *_c2 ) {
    PyFF_Cvt *c1 = (PyFF_Cvt *) _c1, *c2 = (PyFF_Cvt *) _c2;
    PyObject *ret;
    int i, len2, len1;
    int is_cvt2;

    len1 = PyFFCvt_Length(_c1);
    if ( PyType_IsSubtype(&PyFF_CvtType,c2->ob_type) ) {
	len2 = PyFFCvt_Length(_c2);
	is_cvt2 = true;
    } else if ( PySequence_Check(_c2)) {
	is_cvt2 = false;
	len2 = PySequence_Size(_c2);
    } else {
	PyErr_Format(PyExc_TypeError, "The second argument must be either another cvt or a tuple of integers");
return( NULL );
    }
    ret = PyTuple_New(len1+len2);
    for ( i=0; i<len1; ++i )
	PyTuple_SetItem(ret,i,Py_BuildValue("i",memushort(c1->cvt->data,c1->cvt->len,i*sizeof(uint16))) );
    if ( is_cvt2 ) {
	for ( i=0; i<len2; ++i )
	    PyTuple_SetItem(ret,len1+i,Py_BuildValue("i",memushort(c2->cvt->data,c2->cvt->len,i*sizeof(uint16))) );
    } else {
	for ( i=0; i<len2; ++i )
	    PyTuple_SetItem(ret,len1+i,Py_BuildValue("i",PySequence_GetItem(_c2,i)));
    }
Py_RETURN( (PyObject *) ret );
}

static PyObject *PyFFCvt_InPlaceConcat( PyObject *_self, PyObject *_c2 ) {
    PyFF_Cvt *self = (PyFF_Cvt *) _self, *c2 = (PyFF_Cvt *) _c2;
    int i;
    int len1, len2, is_cvt2;
    struct ttf_table *cvt;

    len1 = PyFFCvt_Length(_self);
    if ( PyType_IsSubtype(&PyFF_CvtType,c2->ob_type) ) {
	len2 = PyFFCvt_Length(_c2);
	is_cvt2 = true;
    } else if ( PySequence_Check(_c2)) {
	is_cvt2 = false;
	len2 = PySequence_Size(_c2);
    } else {
	PyErr_Format(PyExc_TypeError, "The second argument must be either another cvt or a tuple of integers");
return( NULL );
    }

    if ( self->cvt==NULL )
	self->cvt = BuildCvt(self->sf,(len1+len2)*2);
    cvt = self->cvt;
    if ( (len1+len2)*2 >= cvt->maxlen )
	cvt->data = grealloc(cvt->data,cvt->maxlen = 2*(len1+len2)+10 );
    if ( is_cvt2 ) {
	if ( len2!=0 )
	    memcpy(cvt->data+len1*sizeof(uint16),c2->cvt->data, 2*len2);
    } else {
	for ( i=0; i<len2; ++i ) {
	    int val = PyInt_AsLong(PySequence_GetItem(_c2,i));
	    if ( PyErr_Occurred())
return( NULL );
	    memputshort(cvt->data,sizeof(uint16)*(len1+i),val);
	}
    }
    cvt->len += 2*len2;
Py_RETURN( self );
}

static PyObject *PyFFCvt_Index( PyObject *self, int pos ) {
    PyFF_Cvt *c = (PyFF_Cvt *) self;

    if ( c->cvt==NULL || pos<0 || pos>=c->cvt->len/2) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( NULL );
    }
return( Py_BuildValue("i",memushort(c->cvt->data,c->cvt->len,pos*sizeof(uint16))) );
}

static int PyFFCvt_IndexAssign( PyObject *self, int pos, PyObject *value ) {
    PyFF_Cvt *c = (PyFF_Cvt *) self;
    struct ttf_table *cvt;
    int val;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred())
return( -1 );
    if ( c->cvt==NULL )
	c->cvt = BuildCvt(c->sf,2);
    cvt = c->cvt;
    if ( cvt==NULL || pos<0 || pos>cvt->len/2) {
	PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( -1 );
    }
    if ( 2*pos>=cvt->maxlen )
	cvt->data = grealloc(cvt->data,cvt->maxlen = sizeof(uint16)*pos+10 );
    if ( 2*pos>=cvt->len )
	cvt->len = sizeof(uint16)*pos;
    memputshort(cvt->data,sizeof(uint16)*pos,val);
return( 0 );
}

static PyObject *PyFFCvt_Slice( PyObject *self, int start, int end ) {
    PyFF_Cvt *c = (PyFF_Cvt *) self;
    struct ttf_table *cvt;
    int len, i;
    PyObject *ret;

    cvt = c->cvt;
    if ( cvt==NULL || end<start || end <0 || 2*start>=cvt->len ) {
	PyErr_Format(PyExc_ValueError, "Slice specification out of range" );
return( NULL );
    }

    len = end-start + 1;

    ret = PyTuple_New(len);
    for ( i=start; i<=end; ++i )
	PyTuple_SetItem(ret,i-start,Py_BuildValue("i",memushort(cvt->data,cvt->len,2*i)));

return( (PyObject *) ret );
}

static int PyFFCvt_SliceAssign( PyObject *_self, int start, int end, PyObject *rpl ) {
    PyFF_Cvt *c = (PyFF_Cvt *) _self;
    struct ttf_table *cvt;
    int len, i;

    cvt = c->cvt;
    if ( cvt==NULL || end<start || end <0 || 2*start>=cvt->len ) {
	PyErr_Format(PyExc_ValueError, "Slice specification out of range" );
return( -1 );
    }

    len = end-start + 1;

    if ( len!=PySequence_Size(rpl) ) {
	if ( !PyErr_Occurred())
	    PyErr_Format(PyExc_ValueError, "Replacement is different size than slice" );
return( -1 );
    }
    for ( i=start; i<=end; ++i ) {
	memputshort(cvt->data,sizeof(uint16)*i,
		PyInt_AsLong(PySequence_GetItem(rpl,i-start)));
	if ( PyErr_Occurred())
return( -1 );
    }
return( 0 );
}

static int PyFFCvt_Contains(PyObject *_self, PyObject *_val) {
    PyFF_Cvt *c = (PyFF_Cvt *) _self;
    struct ttf_table *cvt;
    int i;
    int val;

    val = PyInt_AsLong(_val);
    if ( PyErr_Occurred())
return( -1 );

    cvt = c->cvt;
    if ( cvt==NULL )
return( 0 );

    for ( i=0; i<cvt->len/2; ++i )
	if ( memushort(cvt->data,cvt->len,2*i)==val )
return( 1 );

return( 0 );
}

static PySequenceMethods PyFFCvt_Sequence = {
    PyFFCvt_Length,		/* length */
    PyFFCvt_Concat,		/* concat */
    NULL,			/* repeat */
    PyFFCvt_Index,		/* subscript */
    PyFFCvt_Slice,		/* slice */
    PyFFCvt_IndexAssign,	/* subscript assign */
    PyFFCvt_SliceAssign,	/* slice assign */
    PyFFCvt_Contains,	/* contains */
    PyFFCvt_InPlaceConcat	/* inplace_concat */
};

static PyTypeObject PyFF_CvtType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.cvt",	       /*tp_name*/
    sizeof(PyFF_Cvt),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyFFCvt_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    &PyFFCvt_Sequence,         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFCvt_Str,    /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES,
			       /*tp_flags*/
    "fontforge cvt objects",   /* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,			       /* tp_methods */
    0,			       /* tp_members */
    0,			       /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,				/* tp_init */
    0,                         /* tp_alloc */
    0,			       /* tp_new */
};
/* ************************************************************************** */
/* Selection Standard Methods */
/* ************************************************************************** */

static void PyFFSelection_dealloc(PyFF_Selection *self) {
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *PyFFSelection_new(PyFF_Font *owner) {
    PyFF_Selection *self;

    if ( owner->selection != NULL )
Py_RETURN( owner->selection );
    self = PyObject_New(PyFF_Selection, &PyFF_SelectionType);
    self->fv = owner->fv;
    owner->selection = self;
Py_RETURN( self );
}

static PyObject *PyFFSelection_Str(PyFF_Selection *self) {
return( PyString_FromFormat( "<Selection for %s>", self->fv->sf->fontname ));
}

static PyObject *PyFFSelection_ByGlyphs(PyFF_Selection *real_selection,void *closure) {
    PyFF_Selection *self;

    self = PyObject_New(PyFF_Selection, &PyFF_SelectionType);
    self->fv = real_selection->fv;
    self->by_glyphs=1;
Py_RETURN( self );
}

/* ************************************************************************** */
/* Font Selection based methods */
/* ************************************************************************** */

enum { sel_more=1, sel_less=2,
	sel_unicode=4, sel_encoding=8,
	sel_singletons=16, sel_ranges=32 };
struct flaglist select_flags[] = {
    { "more", sel_more },
    { "less", sel_less },
    { "unicode", sel_unicode },
    { "encoding", sel_encoding },
    { "singletons", sel_singletons },
    { "ranges", sel_ranges },
    NULL };

static PyObject *PyFFSelection_All(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Selection *) self)->fv;
    int i;

    for ( i=0; i<fv->map->enccount; ++i )
	fv->selected[i] = true;
Py_RETURN(self);
}

static PyObject *PyFFSelection_None(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Selection *) self)->fv;
    int i;

    for ( i=0; i<fv->map->enccount; ++i )
	fv->selected[i] = false;
Py_RETURN(self);
}

static PyObject *PyFFSelection_Changed(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Selection *) self)->fv;
    int i, gid;

    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( (gid=fv->map->map[i])!=-1 && fv->sf->glyphs[gid]!=NULL )
	    fv->selected[i] = fv->sf->glyphs[gid]->changed;
	else
	    fv->selected[i] = false;
    }
Py_RETURN(self);
}

static PyObject *PyFFSelection_Invert(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Selection *) self)->fv;
    int i;

    for ( i=0; i<fv->map->enccount; ++i )
	fv->selected[i] = !fv->selected[i];
Py_RETURN(self);
}

static int SelIndex(PyObject *arg, FontView *fv, int ints_as_unicode) {
    int enc;

    if ( PyString_Check(arg)) {
	char *name = PyString_AsString(arg);
	enc = SFFindSlot(fv->sf, fv->map, -1, name );
    } else if ( PyInt_Check(arg)) {
	enc = PyInt_AsLong(arg);
	if ( ints_as_unicode )
	    enc = SFFindSlot(fv->sf, fv->map, enc, NULL );
    } else if ( PyType_IsSubtype(&PyFF_GlyphType,arg->ob_type) ) {
	SplineChar *sc = ((PyFF_Glyph *) arg)->sc;
	if ( sc->parent == fv->sf )
	    enc = fv->map->backmap[sc->orig_pos];
	else
	    enc = SFFindSlot(fv->sf, fv->map, sc->unicodeenc, sc->name );
    } else {
	PyErr_Format(PyExc_TypeError, "Unexpected argument type");
return( -1 );
    }
    if ( enc<0 || enc>=fv->map->enccount ) {
	PyErr_Format(PyExc_ValueError, "Encoding is out of range" );
return( -1 );
    }
return( enc );
}

static PyObject *PyFFSelection_select(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Selection *) self)->fv;
    int flags = sel_encoding|sel_singletons;
    int i, j, cnt = PyTuple_Size(args);
    int range_started = false, range_first = -1;
    int enc;

    for ( i=0; i<cnt; ++i ) {
	PyObject *arg = PyTuple_GetItem(args,i);
	if ( !PyString_Check(arg) && PySequence_Check(arg)) {
	    int newflags = FlagsFromTuple(arg,select_flags);
	    if ( newflags==0x80000000 )
return( NULL );
	    if ( (newflags&(sel_more|sel_less)) == 0 )
		newflags |= (flags&(sel_more|sel_less));
	    if ( (newflags&(sel_unicode|sel_encoding)) == 0 )
		newflags |= (flags&(sel_unicode|sel_encoding));
	    if ( (newflags&(sel_singletons|sel_ranges)) == 0 )
		newflags |= (flags&(sel_singletons|sel_ranges));
	    flags = newflags;
	    if ( i==0 && (flags&(sel_more|sel_less)) == 0 )
		FVDeselectAll(fv);
	    range_started = false;
	} else {
	    if ( i==0 )
		FVDeselectAll(fv);
	    enc = SelIndex(arg,fv,flags&sel_unicode);
	    if ( enc==-1 )
return( NULL );
	    if ( flags&sel_less )
		fv->selected[enc] = 0;
	    else
		fv->selected[enc] = 1;
	    if ( flags&sel_ranges ) {
		if ( !range_started ) {
		    range_started = true;
		    range_first = enc;
		} else {
		    if ( range_first>enc ) {
			for ( j=enc; j<=range_first; ++j )
			    fv->selected[j] = (flags&sel_less)?0:1;
		    } else {
			for ( j=range_first; j<=enc; ++j )
			    fv->selected[j] = (flags&sel_less)?0:1;
		    }
		}
	    }
	}
    }

Py_RETURN(self);
}

static PyObject *fontiter_New(PyObject *font, int bysel);

static PyObject *PySelection_iter(PyObject *self) {
return( fontiter_New(self, 1+((PyFF_Selection *) self)->by_glyphs ));
}

static PyMethodDef PyFFSelection_methods[] = {
    { "select", PyFFSelection_select, METH_VARARGS, "Selects glyphs in the font" },
    { "all", PyFFSelection_All, METH_NOARGS, "Selects all glyphs in the font" },
    { "none", PyFFSelection_None, METH_NOARGS, "Deselects everything" },
    { "changed", PyFFSelection_Changed, METH_NOARGS, "Selects those glyphs which have changed" },
    { "invert", PyFFSelection_Invert, METH_NOARGS, "Inverts the selection" },
    NULL
};

static PyGetSetDef PyFFSelection_getset[] = {
    {"byGlyphs",
	 (getter)PyFFSelection_ByGlyphs, (setter)PyFF_cant_set,
	 "returns a selection object whose iterator will return glyph objects (rather than encoding indeces)", NULL},
    {NULL}  /* Sentinel */
};

/* ************************************************************************** */
/* Selection mapping */
/* ************************************************************************** */

static int PyFFSelection_Length( PyObject *self ) {
return( ((PyFF_Selection *) self)->fv->map->enccount );
}

static PyObject *PyFFSelection_Index( PyObject *self, PyObject *index ) {
    PyFF_Selection *c = (PyFF_Selection *) self;
    PyObject *ret;
    int pos;

    pos = SelIndex(index,c->fv,false);
    if ( pos==-1 )
return( NULL );

    ret = c->fv->selected[pos] ? Py_True : Py_False;
    Py_INCREF( ret );
return( ret );
}

static int PyFFSelection_IndexAssign( PyObject *self, PyObject *index, PyObject *value ) {
    PyFF_Selection *c = (PyFF_Selection *) self;
    int val;
    int pos, cnt;

    if ( PySequence_Check(index)) {
	cnt = PySequence_Size(index);
	for ( pos=0; pos<cnt; ++pos )
	    if ( PyFFSelection_IndexAssign(self,PySequence_GetItem(index,pos),value)==-1 )
return( -1 );

return( 0 );
    }

    pos = SelIndex(index,c->fv,false);
    if ( pos==-1 )
return( -1 );

    if ( value==Py_True )
	val = 1;
    else if ( value==Py_False )
	val = 0;
    else {
	val = PyInt_AsLong(value);
	if ( PyErr_Occurred())
return( -1 );
    }
    c->fv->selected[pos] = val;
return( 0 );
}

static PyMappingMethods PyFFSelection_Mapping = {
    PyFFSelection_Length,		/* length */
    PyFFSelection_Index,		/* subscript */
    PyFFSelection_IndexAssign		/* subscript assign */
};

static PyTypeObject PyFF_SelectionType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.selection",     /*tp_name*/
    sizeof(PyFF_Selection),    /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyFFSelection_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,			       /*tp_as_sequence*/
    &PyFFSelection_Mapping,    /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFSelection_Str,    /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES,
			       /*tp_flags*/
    "fontforge selection objects",   /* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    PySelection_iter,          /* tp_iter */
    0,		               /* tp_iternext */
    PyFFSelection_methods,     /* tp_methods */
    0,			       /* tp_members */
    PyFFSelection_getset,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,				/* tp_init */
    0,                         /* tp_alloc */
    0,			       /* tp_new */
};

/* ************************************************************************** */
/* Private dictionary iterator type */
/* ************************************************************************** */

typedef struct {
	PyObject_HEAD
	PyFF_Private *private;
	int pos;
} privateiterobject;
static PyTypeObject PyFF_PrivateIterType;

static PyObject *privateiter_new(PyObject *private) {
    privateiterobject *di;
    di = PyObject_New(privateiterobject, &PyFF_PrivateIterType);
    if (di == NULL)
return NULL;
    Py_INCREF(private);
    di->private = (PyFF_Private *) private;
    di->pos = 0;
return (PyObject *)di;
}

static void privateiter_dealloc(privateiterobject *di) {
    Py_XDECREF(di->private);
    PyObject_Del(di);
}

static PyObject *privateiter_iternextkey(privateiterobject *di) {
    PyFF_Private *d = di->private;

    if (d == NULL || d->sf->private==NULL )
return NULL;

    if ( di->pos<d->sf->private->next )
return( Py_BuildValue("s",d->sf->private->keys[di->pos++]) );

return NULL;
}

static PyTypeObject PyFF_PrivateIterType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"dictionary-keyiterator",		/* tp_name */
	sizeof(privateiterobject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)privateiter_dealloc, 	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)privateiter_iternextkey,	/* tp_iternext */
	0,					/* tp_methods */
	0
};

/* ************************************************************************** */
/* Private Dict Standard Methods */
/* ************************************************************************** */

static void PyFF_Private_dealloc(PyFF_Private *self) {
    if ( self->sf!=NULL )
	self->sf = NULL;
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *PyFFPrivate_Str(PyFF_Private *self) {
return( PyString_FromFormat( "<Private Dictionary for %s>", self->sf->fontname ));
}

/* ************************************************************************** */
/* ************************** Private Dictionary **************************** */
/* ************************************************************************** */

static int PyFF_PrivateLength( PyObject *self ) {
    struct psdict *private = ((PyFF_Private *) self)->sf->private;
    if ( private==NULL )
return( 0 );
    else
return( private->next );
}

static PyObject *PyFF_PrivateIndex( PyObject *self, PyObject *index ) {
    SplineFont *sf = ((PyFF_Private *) self)->sf;
    struct psdict *private = sf->private;
    char *value=NULL;
    char *pt, *end;
    double temp;
    PyObject *tuple;

    if ( PyString_Check(index)) {
	char *name = PyString_AsString(index);
	if ( private!=NULL )
	    value = PSDictHasEntry(private,name);
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be a string" );
return( NULL );
    }
    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "No such entry" );
return( NULL );
    }
    strtod(value,&end); while ( *end==' ' ) ++end;
    if ( *end=='\0' )
return( Py_BuildValue("d",strtod(value,NULL)) );

    if ( *value=='[' ) {
	int cnt = 0;
	pt = value+1;
	forever {
	    strtod(pt,&end);
	    if ( pt==end )
	break;
	    ++cnt;
	    pt = end;
	}
	while ( *pt==' ' ) ++pt;
	if ( *pt==']' ) {
	    tuple = PyTuple_New(cnt);
	    cnt = 0;
	    pt = value+1;
	    forever {
		temp = strtod(pt,&end);
		if ( pt==end )
	    break;
		PyTuple_SetItem(tuple,cnt++,Py_BuildValue("d",temp));
		pt = end;
	    }
return( tuple );
	}
    }
return( Py_BuildValue("s",value));
}

static int PyFF_PrivateIndexAssign( PyObject *self, PyObject *index, PyObject *value ) {
    SplineFont *sf = ((PyFF_Private *) self)->sf;
    struct psdict *private = sf->private;
    char *string, *freeme=NULL;
    char buffer[40];

    if ( PyString_Check(value))
	string = PyString_AsString(index);
    else if ( PyFloat_Check(value)) {
	double temp = PyFloat_AsDouble(value);
	sprintf(buffer,"%g",temp);
	string = buffer;
    } else if ( PyInt_Check(value)) {
	int temp = PyInt_AsLong(value);
	sprintf(buffer,"%d",temp);
	string = buffer;
    } else if ( PySequence_Check(value)) {
	int i; char *pt;
	pt = string = freeme = galloc(PySequence_Size(value)*21+4);
	*pt++ = '[';
	for ( i=0; i<PySequence_Size(value); ++i ) {
	    sprintf( pt, "%g", PyFloat_AsDouble(PySequence_GetItem(value,i)));
	    pt += strlen(pt);
	    *pt++ = ' ';
	}
	if ( pt[-1]==' ' ) --pt;
	*pt++ = ']'; *pt = '\0';
    } else {
	PyErr_Format(PyExc_TypeError, "Tuple expected for argument" );
return( -1 );
    }

    if ( PyString_Check(index)) {
	char *name = PyString_AsString(index);
	if ( private==NULL )
	    sf->private = private = galloc(sizeof(struct psdict));
	PSDictChangeEntry(private,name,string);
    } else {
	free(freeme);
	PyErr_Format(PyExc_TypeError, "Index must be a string" );
return( -1 );
    }
return( 0 );
}

static PyMappingMethods PyFF_PrivateMapping = {
    PyFF_PrivateLength,		/* length */
    PyFF_PrivateIndex,		/* subscript */
    PyFF_PrivateIndexAssign	/* subscript assign */
};

/* ************************************************************************** */
/* ************************* initializer routines *************************** */
/* ************************************************************************** */

static PyTypeObject PyFF_PrivateType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.private",       /*tp_name*/
    sizeof(PyFF_Private), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) PyFF_Private_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    &PyFF_PrivateMapping,      /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFPrivate_Str,/*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "FontForge private dictionary", /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    privateiter_new,		/* tp_iter */
    0,		               /* tp_iternext */
    0,			       /* tp_methods */
    0,			       /* tp_members */
    0,		               /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,			       /* tp_init */
    0,                         /* tp_alloc */
    0			       /* tp_new */
};

/* ************************************************************************** */
/* Font iterator type */
/* ************************************************************************** */

typedef struct {
    PyObject_HEAD
    SplineFont *sf;
    int pos;
    int byselection;
    FontView *fv;
} fontiterobject;
static PyTypeObject PyFF_FontIterType;

static PyObject *fontiter_New(PyObject *font, int bysel) {
    fontiterobject *di;
    di = PyObject_New(fontiterobject, &PyFF_FontIterType);
    if (di == NULL)
return NULL;
    di->sf = ((PyFF_Font *) font)->fv->sf;
    di->fv = ((PyFF_Font *) font)->fv;
    di->pos = 0;
    di->byselection = bysel;
return (PyObject *)di;
}

static PyObject *fontiter_new(PyObject *font) {
return( fontiter_New(font,false) );
}

static void fontiter_dealloc(fontiterobject *di) {
    PyObject_Del(di);
}

static PyObject *fontiter_iternextkey(fontiterobject *di) {
    if ( !di->byselection ) {
	SplineFont *sf = di->sf;

	if (sf == NULL)
return NULL;

	while ( di->pos<sf->glyphcnt ) {
	    if ( sf->glyphs[di->pos]!=NULL )
return( Py_BuildValue("s",sf->glyphs[di->pos++]->name) );
	    ++di->pos;
	}
    } else if ( di->byselection==2 ) {
	int gid;
	FontView *fv = di->fv;
	int enccount = fv->map->enccount;
	while ( di->pos < enccount ) {
	    if ( fv->selected[di->pos] && (gid=fv->map->map[di->pos])!=-1 &&
		    SCWorthOutputting(fv->sf->glyphs[gid]) ) {
		++di->pos;
return( PySC_From_SC_I( fv->sf->glyphs[gid] ) );
	    }
	    ++di->pos;
	}
    } else {
	FontView *fv = di->fv;
	int enccount = fv->map->enccount;
	while ( di->pos < enccount ) {
	    if ( fv->selected[di->pos] )
return( Py_BuildValue("i",di->pos++ ) );
	    ++di->pos;
	}
    }

return NULL;
}

static PyTypeObject PyFF_FontIterType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"fontiter",				/* tp_name */
	sizeof(fontiterobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)fontiter_dealloc,	 	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)fontiter_iternextkey,	/* tp_iternext */
	0,					/* tp_methods */
	0,
};

/* ************************************************************************** */
/* Font Standard Methods */
/* ************************************************************************** */

static void PyFF_Font_dealloc(PyFF_Font *self) {
    if ( self->fv!=NULL )
	self->fv = NULL;
    Py_XDECREF(self->selection);
    Py_XDECREF(self->cvt);
    Py_XDECREF(self->private);
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *PyFF_Font_new(PyTypeObject *type,PyObject *args,PyObject *kwds) {
    PyFF_Font *self;

    self = (PyFF_Font *) (type->tp_alloc)(type,0);
    if ( self!=NULL ) {
	self->fv = SFAdd(SplineFontNew());
    }
return( (PyObject *) self );
}

static PyObject *PyFFFont_close(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;

    if ( fv->gw!=NULL )
	GDrawDestroyWindow(fv->gw);
    else {
	if ( fv_list==fv )
	    fv_list = fv->next;
	else {
	    FontView *n;
	    for ( n=fv_list; n->next!=fv; n=n->next );
	    n->next = fv->next;
	}
	FontViewFree(fv);
    }
    ((PyFF_Font *) self)->fv = NULL;
Py_RETURN_NONE;
}

static PyObject *PyFFFont_Str(PyFF_Font *self) {
return( PyString_FromFormat( "<Font: %s>", self->fv->sf->fontname ));
}

/* ************************************************************************** */
/* sfnt 'name' table stuff */
/* ************************************************************************** */

static struct flaglist *sfnt_name_str_ids, *sfnt_name_mslangs;

/* I can't just use the names in the fontinfo file because they get translated*/
/*  so I must save the untranslated names before that happens */
void scriptingSaveEnglishNames(GTextInfo *ids,GTextInfo *langs) {
    int lcnt,cnt,k;
    char *pt, *ept;

    for ( cnt=0; ids[cnt].text!=NULL; ++cnt );
    sfnt_name_str_ids = gcalloc(cnt+4,sizeof(struct flaglist));
    sfnt_name_str_ids[0].name = "Subfamily";
    sfnt_name_str_ids[0].flag = 2;
    for ( cnt=0; ids[cnt].text!=NULL; ++cnt ) {
	sfnt_name_str_ids[cnt+1].name = (char *) ids[cnt].text;
	sfnt_name_str_ids[cnt+1].flag = (intpt) ids[cnt].userdata;
    }
    ++cnt;
    sfnt_name_str_ids[cnt].name = "Styles";
    sfnt_name_str_ids[cnt++].flag = 2;
    sfnt_name_str_ids[cnt].name = "PostScript";
    sfnt_name_str_ids[cnt].flag = 2;

    for ( cnt=lcnt=0; langs[cnt].text!=NULL; ++cnt )
	if ( (((intpt) langs[cnt].userdata)&0xff00) == 0x400 )
	    ++lcnt;
    sfnt_name_mslangs = gcalloc(cnt+lcnt+4,sizeof(struct flaglist));
    for ( cnt=k=0; langs[cnt].text!=NULL; ++cnt ) {
	pt = strchr((char *) langs[cnt].text,'|');
	pt = pt==NULL ? (char *) langs[cnt].text : pt+1;
	sfnt_name_mslangs[k].name = pt;
	sfnt_name_mslangs[k++].flag = (intpt) langs[cnt].userdata;
	if ( (((intpt) langs[cnt].userdata)&0xff00) == 0x400 &&
		strchr(pt,' ')!=NULL ) {
	    ept = strrchr(pt,' ');
	    sfnt_name_mslangs[k].name = copyn(pt,ept-pt);
	    sfnt_name_mslangs[k++].flag = (intpt) langs[cnt].userdata;
	}
    }
}

static PyObject *sfntnametuple(int lang,int strid,char *name) {
    PyObject *tuple;
    int i;

    tuple = PyTuple_New(3);

    PyTuple_SetItem(tuple,2,Py_BuildValue("s", name));

    FontInfoInit();
    for ( i=0; sfnt_name_mslangs[i].name!=NULL ; ++i )
	if ( sfnt_name_mslangs[i].flag == lang )
    break;
    if ( sfnt_name_mslangs[i].flag == lang )
	PyTuple_SetItem(tuple,0,Py_BuildValue("s", sfnt_name_mslangs[i].name));
    else
	PyTuple_SetItem(tuple,0,Py_BuildValue("i", lang));

    for ( i=0; sfnt_name_str_ids[i].name!=NULL ; ++i )
	if ( sfnt_name_str_ids[i].flag == strid )
    break;
    if ( sfnt_name_str_ids[i].flag == strid )
	PyTuple_SetItem(tuple,1,Py_BuildValue("s", sfnt_name_str_ids[i].name));
    else
	PyTuple_SetItem(tuple,1,Py_BuildValue("i", strid));
return( tuple );
}

static int SetSFNTName(SplineFont *sf,PyObject *tuple,struct ttflangname *english) {
    char *lang_str, *strid_str, *string;
    int lang, strid;
    PyObject *val;
    struct ttflangname *names;

    if ( !PySequence_Check(tuple)) {
	PyErr_Format(PyExc_TypeError, "Value must be a tuple" );
return(0);
    }

    val = PySequence_GetItem(tuple,0);
    if ( PyString_Check(val) ) {
	lang_str = PyString_AsString(val);
	lang = FlagsFromString(lang_str,sfnt_name_mslangs);
	if ( lang==0x80000000 ) {
	    PyErr_Format(PyExc_TypeError, "Unknown language" );
return( 0 );
	}
    } else if ( PyInt_Check(val))
	lang = PyInt_AsLong(val);
    else {
	PyErr_Format(PyExc_TypeError, "Unknown language" );
return( 0 );
    }

    val = PySequence_GetItem(tuple,1);
    if ( PyString_Check(val) ) {
	strid_str = PyString_AsString(val);
	strid = FlagsFromString(strid_str,sfnt_name_str_ids);
	if ( strid==0x80000000 ) {
	    PyErr_Format(PyExc_TypeError, "Unknown string id" );
return( 0 );
	}
    } else if ( PyInt_Check(val))
	strid = PyInt_AsLong(val);
    else {
	PyErr_Format(PyExc_TypeError, "Unknown string id" );
return( 0 );
    }

    for ( names=sf->names; names!=NULL; names=names->next )
	if ( names->lang==lang )
    break;

    if ( PySequence_GetItem(tuple,2)==Py_None ) {
	if ( names!=NULL ) {
	    free( names->names[strid] );
	    names->names[strid] = NULL;
	}
return( 1 );
    }

    string = PyString_AsString(PySequence_GetItem(tuple,2));
    if ( string==NULL )
return( 0 );
    if ( lang==0x409 && english!=NULL && english->names[strid]!=NULL &&
	    strcmp(string,english->names[strid])==0 )
return( 1 );	/* If they set it to the default, there's nothing to do */

    if ( names==NULL ) {
	names = chunkalloc(sizeof( struct ttflangname ));
	names->lang = lang;
	names->next = sf->names;
	sf->names = names;
    }
    free(names->names[strid]);
    names->names[strid] = copy( string );
return( 1 );
}

/* ************************************************************************** */
/* Font getters/setters */
/* ************************************************************************** */

static PyObject *PyFF_Font_get_sfntnames(PyFF_Font *self,void *closure) {
    struct ttflangname *names, *english;
    int cnt, i;
    PyObject *tuple;
    struct ttflangname dummy;
    SplineFont *sf = self->fv->sf;

    memset(&dummy,0,sizeof(dummy));
    DefaultTTFEnglishNames(&dummy, sf);

    cnt = 0;
    for ( english = sf->names; english!=NULL; ++english )
	if ( english->lang==0x409 )
    break;
    for ( i=0; i<ttf_namemax; ++i ) {
	if ( (english!=NULL && english->names[i]!=NULL ) || dummy.names[i]!=NULL )
	    ++cnt;
    }
    for ( names = sf->names; names!=NULL; ++names ) if ( names!=english ) {
	for ( i=0; i<ttf_namemax; ++i ) {
	    if ( names->names[i]!=NULL )
		++cnt;
	}
    }
    tuple = PyTuple_New(cnt);
    cnt = 0;
    for ( i=0; i<ttf_namemax; ++i ) {
	char *nm = (english!=NULL && english->names[i]!=NULL ) ? english->names[i] : dummy.names[i];
	if ( nm!=NULL )
	    PyTuple_SetItem(tuple,cnt++,sfntnametuple(0x409,i,nm));
    }
    for ( names = sf->names; names!=NULL; ++names ) if ( names!=english ) {
	if ( names->names[i]!=NULL )
	    PyTuple_SetItem(tuple,cnt++,sfntnametuple(names->lang,i,names->names[i]));
    }

    for ( i=0; i<ttf_namemax; ++i )
	free( dummy.names[i]);

return( tuple );
}

static int PyFF_Font_set_sfntnames(PyFF_Font *self,PyObject *value,void *closure) {
    SplineFont *sf = self->fv->sf;
    struct ttflangname *names;
    struct ttflangname dummy;
    int i;

    if ( !PySequence_Check(value)) {
	PyErr_Format(PyExc_TypeError, "Value must be a tuple" );
return(-1);
    }

    memset(&dummy,0,sizeof(dummy));
    DefaultTTFEnglishNames(&dummy, sf);

    for ( names = sf->names; names!=NULL; names=names->next ) {
	for ( i=0; i<ttf_namemax; ++i ) {
	    free(names->names[i]);
	    names->names[i] = NULL;
	}
    }
    for ( i=PySequence_Size(value)-1; i>=0; --i )
	if ( !SetSFNTName(sf,PySequence_GetItem(value,i),&dummy) )
return( -1 );

    for ( i=0; i<ttf_namemax; ++i )
	free( dummy.names[i]);

return( 0 );
}

static PyObject *PyFF_Font_get_bitmapSizes(PyFF_Font *self,void *closure) {
    PyObject *tuple;
    int cnt;
    SplineFont *sf = self->fv->sf;
    BDFFont *bdf;

    for ( cnt=0, bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++cnt );

    tuple = PyTuple_New(cnt);
    for ( cnt=0, bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next, ++cnt )
	PyTuple_SetItem(tuple,cnt,Py_BuildValue("i",
		bdf->clut==NULL ? bdf->pixelsize :
				(bdf->pixelsize | (BDFDepth(bdf)<<16)) ));

return( tuple );
}

static int bitmapper(PyFF_Font *self,PyObject *value,int isavail) {
    int cnt, i;
    int *sizes;

    cnt = PyTuple_Size(value);
    if ( PyErr_Occurred())
return( -1 );
    sizes = galloc((cnt+1)*sizeof(int));
    for ( i=0; i<cnt; ++i ) {
	if ( !PyArg_ParseTuple(PyTuple_GetItem(value,i),"i", &sizes[i]))
return( -1 );
	if ( (sizes[i]>>16)==0 )
	    sizes[i] |= 0x10000;
    }
    sizes[i] = 0;

    if ( !BitmapControl(self->fv,sizes,isavail,false) ) {
	free(sizes);
	PyErr_Format(PyExc_EnvironmentError, "Bitmap operation failed");
return( -1 );
    }
    free(sizes);
return( 0 );
}

static int PyFF_Font_set_bitmapSizes(PyFF_Font *self,PyObject *value,void *closure) {
return( bitmapper(self,value,true));
}

static struct flaglist gaspflags[] = {
    { "gridfit",		1 },
    { "antialias",		2 },
    { "symmetric-smoothing",	4 },
    { "gridfit+smoothing",	8 },
    NULL };

static PyObject *PyFF_Font_get_gasp(PyFF_Font *self,void *closure) {
    PyObject *tuple, *flagstuple;
    int i, j, cnt;
    SplineFont *sf = self->fv->sf;

    tuple = PyTuple_New(sf->gasp_cnt);

    for ( i=0; i<sf->gasp_cnt; ++i ) {
	for ( j=cnt=0; gaspflags[j].name!=NULL; ++j )
	    if ( sf->gasp[i].flags & gaspflags[j].flag )
		++cnt;
	flagstuple = PyTuple_New(cnt);
	for ( j=cnt=0; gaspflags[j].name!=NULL; ++j )
	    if ( sf->gasp[i].flags & gaspflags[j].flag )
		PyTuple_SetItem(flagstuple,cnt++,Py_BuildValue("s", gaspflags[j].name));
	PyTuple_SetItem(tuple,i,Py_BuildValue("iO",sf->gasp[i].ppem,flagstuple));
    }

return( tuple );
}

static int PyFF_Font_set_gasp(PyFF_Font *self,PyObject *value,void *closure) {
    SplineFont *sf = self->fv->sf;
    int cnt, i, flag;
    struct gasp *gasp;
    PyObject *flags;

    cnt = PyTuple_Size(value);
    if ( PyErr_Occurred())
return( -1 );
    if ( cnt==0 )
	gasp = NULL;
    else {
	gasp = galloc(cnt*sizeof(struct gasp));
	for ( i=0; i<cnt; ++i ) {
	    if ( !PyArg_ParseTuple(PyTuple_GetItem(value,i),"hO",
		    &gasp[i].ppem, &flags ))
return( -1 );
	    flag = FlagsFromTuple(flags,gaspflags);
	    if ( flag==0x80000000 )
return( -1 );
	    gasp[i].flags = flag;
	}
    }
    free(sf->gasp);
    sf->gasp = gasp;
    sf->gasp_cnt = cnt;
return( 0 );
}

static PyObject *PyFF_Font_get_lookups(PyFF_Font *self,void *closure, int isgpos) {
    PyObject *tuple;
    OTLookup *otl;
    int cnt;
    SplineFont *sf = self->fv->sf;

    cnt = 0;
    for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
	++cnt;

    tuple = PyTuple_New(cnt);

    cnt = 0;
    for ( otl=isgpos ? sf->gpos_lookups : sf->gsub_lookups; otl!=NULL; otl=otl->next )
	PyTuple_SetItem(tuple,cnt++,Py_BuildValue("s",otl->lookup_name));

return( tuple );
}

static PyObject *PyFF_Font_get_gpos_lookups(PyFF_Font *self,void *closure) {
return( PyFF_Font_get_lookups(self,closure,true));
}

static PyObject *PyFF_Font_get_gsub_lookups(PyFF_Font *self,void *closure) {
return( PyFF_Font_get_lookups(self,closure,false));
}

static PyObject *PyFF_Font_get_private(PyFF_Font *self,void *closure) {
    PyFF_Private *private;

    if ( self->private!=NULL )
Py_RETURN( self->private );
    private = (PyFF_Private *) PyObject_New(PyFF_Private, &PyFF_PrivateType);
    if (private == NULL)
return NULL;
    private->sf = self->fv->sf;
    self->private = private;
return (PyObject *)private;
}

static PyObject *PyFF_Font_get_selection(PyFF_Font *self,void *closure) {
return( PyFFSelection_new(self));
}

static int PyFF_Font_set_selection(PyFF_Font *self,PyObject *value,void *closure) {
    PyFF_Selection *sel = (PyFF_Selection *) value;
    int i, len2;
    int is_sel;
    FontView *fv = self->fv;

    if ( PyType_IsSubtype(&PyFF_SelectionType,value->ob_type) ) {
	len2 = PyFFSelection_Length(value);
	is_sel = true;
    } else if ( PySequence_Check(value)) {
	is_sel = false;
	len2 = PySequence_Size(value);
    } else {
	PyErr_Format(PyExc_TypeError, "The value must be either another selection or a tuple of integers");
return( -1 );
    }

    if ( len2>=fv->map->enccount ) {
	PyErr_Format(PyExc_TypeError, "Too much data");
return( -1 );
    }
    if ( is_sel ) {
	if ( len2!=0 )
	    memcpy(fv->selected,sel->fv->selected,len2 );
    } else {
	for( i=0; i<len2; ++i ) {
	    int val;
	    PyObject *obj = PySequence_GetItem(value,i);
	    if ( obj==Py_True )
		val = 1;
	    else if ( obj==Py_False )
		val = 0;
	    else {
		val = PyInt_AsLong(obj);
		if ( PyErr_Occurred())
return( -1 );
	    }
	    fv->selected[i] = val;
	}
    }
return( 0 );
}

static PyObject *PyFF_Font_get_cvt(PyFF_Font *self,void *closure) {
return( PyFFCvt_new(self));
}

static int PyFF_Font_set_cvt(PyFF_Font *self,PyObject *value,void *closure) {
    PyFF_Cvt *c2 = (PyFF_Cvt *) value;
    int i, len2;
    int is_cvt2;
    SplineFont *sf = self->fv->sf;
    struct ttf_table *cvt;

    if ( PyType_IsSubtype(&PyFF_CvtType,value->ob_type) ) {
	len2 = PyFFCvt_Length(value);
	is_cvt2 = true;
    } else if ( PySequence_Check(value)) {
	is_cvt2 = false;
	len2 = PySequence_Size(value);
    } else {
	PyErr_Format(PyExc_TypeError, "The value must be either another cvt or a tuple of integers");
return( -1 );
    }

    cvt = SFFindTable(sf,CHR('c','v','t',' '));
    if ( cvt==NULL )
	cvt = BuildCvt(sf,len2*2);
    if ( len2*2>=cvt->maxlen )
	cvt->data = grealloc(cvt->data,cvt->maxlen = sizeof(uint16)*len2+10 );
    if ( is_cvt2 ) {
	if ( len2!=0 )
	    memcpy(cvt->data,c2->cvt->data,2*len2 );
    } else {
	for( i=0; i<len2; ++i ) {
	    memputshort(cvt->data,2*i,PyInt_AsLong(PySequence_GetItem(value,i)));
	    if ( PyErr_Occurred())
return( -1 );
	}
    }
    cvt->len = 2*len2;
return( 0 );
}

static PyObject *PyFF_Font_get_userdata(PyFF_Font *self,void *closure) {
    Py_XINCREF( (PyObject *) (self->fv->python_data) );
return( self->fv->python_data );
}

static int PyFF_Font_set_userdata(PyFF_Font *self,PyObject *value,void *closure) {
    PyObject *old = self->fv->python_data;

    Py_INCREF(value);
    self->fv->python_data = value;
    Py_XDECREF(old);
return( 0 );
}

static int PyFF_Font_set_str_null(PyFF_Font *self,PyObject *value,
	char *str,int offset) {
    char *newv, **oldpos;
    PyObject *temp;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    if ( value == Py_None )
	newv = NULL;
    else if ( PyUnicode_Check(value)) {
	/* Need to force utf8 encoding rather than accepting the "default" */
	/*  which would happen if we treated unicode as a string */
	temp = PyUnicode_AsUTF8String(value);
	newv = copy( PyString_AsString(temp));
	Py_DECREF(temp);
    } else
	newv = copy( PyString_AsString(value));
    if ( newv==NULL && value!=Py_None )
return( -1 );
    oldpos = (char **) (((char *) (self->fv->sf)) + offset );
    free( *oldpos );
    *oldpos = newv;
return( 0 );
}

static int PyFF_Font_set_str(PyFF_Font *self,PyObject *value,
	char *str,int offset) {
    char *newv, **oldpos;
    PyObject *temp;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    if ( PyUnicode_Check(value)) {
	/* Need to force utf8 encoding rather than accepting the "default" */
	/*  which would happen if we treated unicode as a string */
	temp = PyUnicode_AsUTF8String(value);
	newv = copy( PyString_AsString(temp));
	Py_DECREF(temp);
    } else
	newv = copy( PyString_AsString(value));
    if ( newv==NULL )
return( -1 );
    oldpos = (char **) (((char *) (self->fv->sf)) + offset );
    free( *oldpos );
    *oldpos = newv;
return( 0 );
}

static int PyFF_Font_set_real(PyFF_Font *self,PyObject *value,
	char *str,int offset) {
    double temp;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    temp = PyFloat_AsDouble(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    * (real *) (((char *) (self->fv->sf)) + offset ) = temp;
return( 0 );
}

static int PyFF_Font_set_int(PyFF_Font *self,PyObject *value,
	char *str,int offset) {
    long temp;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    temp = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    * (int *) (((char *) (self->fv->sf)) + offset ) = temp;
return( 0 );
}

static int PyFF_Font_set_int2(PyFF_Font *self,PyObject *value,
	char *str,int offset) {
    long temp;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete the %s", str);
return( -1 );
    }
    temp = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    * (int16 *) (((char *) (self->fv->sf)) + offset ) = temp;
return( 0 );
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_str(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self,void *closure) { \
return( Py_BuildValue("s", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value,void *closure) {\
return( PyFF_Font_set_str(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_strnull(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self,void *closure) { \
    if ( self->fv->sf->name==NULL )			\
Py_RETURN_NONE;						\
    else						\
return( Py_BuildValue("s", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value,void *closure) {\
return( PyFF_Font_set_str_null(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_real(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self,void *closure) { \
return( Py_BuildValue("d", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value,void *closure) {\
return( PyFF_Font_set_real(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_int(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self,void *closure) { \
return( Py_BuildValue("i", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value,void *closure) {\
return( PyFF_Font_set_int(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_int2(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self,void *closure) { \
return( Py_BuildValue("i", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value,void *closure) {\
return( PyFF_Font_set_int2(self,value,#name,offsetof(SplineFont,name)) );\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_ro_bit(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self,void *closure) { \
return( Py_BuildValue("i", self->fv->sf->name ));	\
}

/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_bit(name) \
static PyObject *PyFF_Font_get_##name(PyFF_Font *self,void *closure) { \
return( Py_BuildValue("i", self->fv->sf->name ));	\
}							\
							\
static int PyFF_Font_set_##name(PyFF_Font *self,PyObject *value,void *closure) { \
    long temp;						\
							\
    if ( value==NULL ) {				\
	PyErr_SetString(PyExc_TypeError, "Cannot delete the " #name ); \
return( -1 );						\
    }							\
    temp = PyInt_AsLong(value);				\
    if ( PyErr_Occurred()!=NULL )			\
return( -1 );						\
    self->fv->sf->name = temp;				\
return( 0 );						\
}
/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
static void SFDefaultOS2(SplineFont *sf) {
    if ( !sf->pfminfo.pfmset ) {
	SFDefaultOS2Info(&sf->pfminfo,sf,sf->fontname);
	sf->pfminfo.pfmset = sf->pfminfo.subsuper_set = sf->pfminfo.panose_set =
	    sf->pfminfo.hheadset = sf->pfminfo.vheadset = true;
    }
}

#define ff_gs_os2int2(name) \
static PyObject *PyFF_Font_get_OS2_##name(PyFF_Font *self,void *closure) { \
    SplineFont *sf = self->fv->sf;			\
    SFDefaultOS2(sf);					\
return( Py_BuildValue("i", sf->pfminfo.name ));		\
}							\
							\
static int PyFF_Font_set_OS2_##name(PyFF_Font *self,PyObject *value,void *closure) {\
    SplineFont *sf = self->fv->sf;			\
    SFDefaultOS2(sf);					\
return( PyFF_Font_set_int2(self,value,#name,offsetof(SplineFont,pfminfo)+offsetof(struct pfminfo,name)) );\
}
/* *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */
#define ff_gs_os2bit(name) \
static PyObject *PyFF_Font_get_OS2_##name(PyFF_Font *self,void *closure) { \
    SplineFont *sf = self->fv->sf;			\
    SFDefaultOS2(sf);					\
return( Py_BuildValue("i", self->fv->sf->pfminfo.name ));	\
}							\
							\
static int PyFF_Font_set_OS2_##name(PyFF_Font *self,PyObject *value,void *closure) { \
    SplineFont *sf = self->fv->sf;			\
    long temp;						\
							\
    if ( value==NULL ) {				\
	PyErr_SetString(PyExc_TypeError, "Cannot delete the " #name ); \
return( -1 );						\
    }							\
    temp = PyInt_AsLong(value);				\
    if ( PyErr_Occurred()!=NULL )			\
return( -1 );						\
    SFDefaultOS2(sf);					\
    sf->pfminfo.name = temp;				\
return( 0 );						\
}

ff_gs_str(fontname)
ff_gs_str(fullname)
ff_gs_str(familyname)
ff_gs_str(weight)
ff_gs_str(version)

ff_gs_strnull(copyright)
ff_gs_strnull(xuid)
ff_gs_strnull(fondname)

ff_gs_strnull(cidregistry)
ff_gs_strnull(ordering)

ff_gs_real(italicangle)
ff_gs_real(upos)
ff_gs_real(uwidth)
ff_gs_real(cidversion)
ff_gs_real(strokewidth)

ff_gs_int(ascent)
ff_gs_int(descent)
ff_gs_int(vertical_origin)
ff_gs_int(uniqueid)
ff_gs_int(supplement)
ff_gs_int2(macstyle)
ff_gs_int2(os2_version)
ff_gs_int2(gasp_version)

ff_gs_os2int2(weight)
ff_gs_os2int2(width)
ff_gs_os2int2(fstype)
ff_gs_os2int2(linegap)
ff_gs_os2int2(vlinegap)
ff_gs_os2int2(hhead_ascent)
ff_gs_os2int2(hhead_descent)
ff_gs_os2int2(os2_typoascent)
ff_gs_os2int2(os2_typodescent)
ff_gs_os2int2(os2_typolinegap)
ff_gs_os2int2(os2_winascent)
ff_gs_os2int2(os2_windescent)
ff_gs_os2int2(os2_subxsize)
ff_gs_os2int2(os2_subxoff)
ff_gs_os2int2(os2_subysize)
ff_gs_os2int2(os2_subyoff)
ff_gs_os2int2(os2_supxsize)
ff_gs_os2int2(os2_supxoff)
ff_gs_os2int2(os2_supysize)
ff_gs_os2int2(os2_supyoff)
ff_gs_os2int2(os2_strikeysize)
ff_gs_os2int2(os2_strikeypos)
ff_gs_os2int2(os2_family_class)

ff_gs_os2bit(winascent_add)
ff_gs_os2bit(windescent_add)
ff_gs_os2bit(hheadascent_add)
ff_gs_os2bit(hheaddescent_add)
ff_gs_os2bit(typoascent_add)
ff_gs_os2bit(typodescent_add)

ff_gs_bit(changed)
ff_gs_ro_bit(multilayer)
ff_gs_bit(strokedfont)
ff_gs_ro_bit(new)

ff_gs_bit(use_typo_metrics)
ff_gs_bit(weight_width_slope_only)
ff_gs_bit(onlybitmaps)
ff_gs_bit(hasvmetrics)

static PyObject *PyFF_Font_get_path(PyFF_Font *self,void *closure) { \
    if ( self->fv->sf->origname==NULL )
Py_RETURN_NONE;
    else
return( Py_BuildValue("s", self->fv->sf->origname ));
}

static PyObject *PyFF_Font_get_sfd_path(PyFF_Font *self,void *closure) { \
    if ( self->fv->sf->filename==NULL )
Py_RETURN_NONE;
    else
return( Py_BuildValue("s", self->fv->sf->filename ));
}

static PyObject *PyFF_Font_get_OS2_panose(PyFF_Font *self,void *closure) {
    int i;
    PyObject *tuple;
    SplineFont *sf = self->fv->sf;
    if ( !sf->pfminfo.panose_set )
	SFDefaultOS2(sf);
    tuple = PyTuple_New(10);
    for ( i=0; i<10; ++i )
	PyTuple_SET_ITEM(tuple,i,Py_BuildValue("i",self->fv->sf->pfminfo.panose[i]));
return( tuple );
}

static int PyFF_Font_set_OS2_panose(PyFF_Font *self,PyObject *value,void *closure) {
    int panose[10], i;
    SplineFont *sf = self->fv->sf;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete panose");
return( -1 );
    }
    if ( !PyArg_ParseTuple(value,"iiiiiiiiii", &panose[0], &panose[1], &panose[2], &panose[3],
	    &panose[4], &panose[5], &panose[6], &panose[7], &panose[8], &panose[9] ))
return( -1 );

    if ( !sf->pfminfo.panose_set )
	SFDefaultOS2(sf);
    for ( i=0; i<10; ++i )
	sf->pfminfo.panose[i] = panose[i];
    sf->pfminfo.panose_set = true;
return( 0 );
}

static PyObject *PyFF_Font_get_OS2_vendor(PyFF_Font *self,void *closure) {
    char buf[8];
    SplineFont *sf = self->fv->sf;

    SFDefaultOS2(sf);
    buf[0] = sf->pfminfo.os2_vendor[0];
    buf[1] = sf->pfminfo.os2_vendor[1];
    buf[2] = sf->pfminfo.os2_vendor[2];
    buf[3] = sf->pfminfo.os2_vendor[3];
    buf[4] = '\0';
return( Py_BuildValue("s",buf));
}

static int PyFF_Font_set_OS2_vendor(PyFF_Font *self,PyObject *value,void *closure) {
    SplineFont *sf = self->fv->sf;
    char *newv;
    PyObject *temp;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete vendor" );
return( -1 );
    }
    if ( PyUnicode_Check(value)) {
	/* Need to force utf8 encoding rather than accepting the "default" */
	/*  which would happen if we treated unicode as a string */
	temp = PyUnicode_AsUTF8String(value);
	newv = PyString_AsString(temp);
	Py_DECREF(temp);
    } else
	newv = PyString_AsString(value);
    if ( newv==NULL )
return( -1 );

    if ( strlen( newv )>4 ) {
	PyErr_Format(PyExc_TypeError, "OS2 vendor is limited to 4 characters" );
return( -1 );
    }
    SFDefaultOS2(sf);
    sf->pfminfo.os2_vendor[0] = newv[0];
    sf->pfminfo.os2_vendor[1] = newv[1];
    sf->pfminfo.os2_vendor[2] = newv[2];
    sf->pfminfo.os2_vendor[3] = newv[3];
    sf->pfminfo.panose_set = true;
return( 0 );
}

static PyObject *PyFF_Font_get_design_size(PyFF_Font *self,void *closure) {
    /* Design size is expressed in tenths of points */
return( Py_BuildValue("d", self->fv->sf->design_size/10.0));
}

static int PyFF_Font_set_design_size(PyFF_Font *self,PyObject *value,
	void *closure) {
    double temp;

    if ( value==NULL )
	self->fv->sf->design_size = 0;
    else if ( PyFloat_Check(value)) {
	temp = PyFloat_AsDouble(value);
	if ( PyErr_Occurred()!=NULL )
return( -1 );
	self->fv->sf->design_size = rint(10.0*temp);
    } else if ( PyInt_Check(value)) {
	int t = PyInt_AsLong(value);
	if ( PyErr_Occurred()!=NULL )
return( -1 );
	self->fv->sf->design_size = 10*t;
    }
return( 0 );
}

static PyObject *PyFF_Font_get_size_feature(PyFF_Font *self,void *closure) {
    /* Size feature has two formats: Just a design size, or a design size */
    /*  and size bounds, id & name */
    /* First case means a tuple of one element, second a tuple of 5 */
    /* design size & bounds are measured in tenths of points */
    struct otfname *names;
    int i,cnt;
    PyObject *tuple;
    SplineFont *sf = self->fv->sf;

    if ( sf->design_size==0 )
Py_RETURN_NONE;

    for ( names=sf->fontstyle_name, cnt=0; names!=NULL; names=names->next, ++cnt );
    if ( cnt==0 )
return( Py_BuildValue("(d)", sf->design_size/10.0));
    tuple = PyTuple_New(cnt);
    FontInfoInit();
    for ( names=sf->fontstyle_name, cnt=0; names!=NULL; names=names->next, ++cnt ) {
	for ( i=0; sfnt_name_mslangs[i].name!=NULL ; ++i )
	    if ( sfnt_name_mslangs[i].flag == names->lang )
	break;
	if ( sfnt_name_mslangs[i].flag == names->lang )
	    PyTuple_SetItem(tuple,i,Py_BuildValue("ss", sfnt_name_mslangs[i].name, names->name));
	else
	    PyTuple_SetItem(tuple,i,Py_BuildValue("is", names->lang, names->name));
    }
return( Py_BuildValue("(dddiO)", sf->design_size/10.0,
	sf->design_range_bottom/10.0, sf->design_range_top/10.,
	sf->fontstyle_id, tuple ));
}

static int PyFF_Font_set_size_feature(PyFF_Font *self,PyObject *value,
	void *closure) {
    double temp, top=0, bot=0;
    int id=0;
    PyObject *names=NULL;
    SplineFont *sf = self->fv->sf;
    int i;
    struct otfname *head, *last, *cur;
    char *string;

    if ( value==NULL ) {
	sf->design_size = 0;
	/* Setting the design size to zero means there will be no 'size' feature in the output */
return( 0 );
    }
    /* If they only specify a design size, we won't require that it be a tuple */
    /*  if it is in a tuple, remove it and treat as a single value */
    if ( PySequence_Check(value) && PySequence_Size(value)==1 )
	value = PySequence_GetItem(value,0);
    if ( PyFloat_Check(value) || PyInt_Check(value)) {
	if ( PyFloat_Check(value))
	    temp = PyFloat_AsDouble(value);
	else
	    temp = PyInt_AsLong(value);
	if ( PyErr_Occurred()!=NULL )
return( -1 );
	sf->design_size = rint(10.0*temp);
	sf->design_range_bottom = 0;
	sf->design_range_top = 0;
	sf->fontstyle_id = 0;
	OtfNameListFree(sf->fontstyle_name);
	sf->fontstyle_name = NULL;
return( 0 );
    }

    if ( !PyArg_ParseTuple(value,"dddiO", &temp, &bot, &top, &id, &names ))
return( -1 );
    sf->design_size = rint(10.0*temp);
    sf->design_range_bottom = rint(10.0*bot);
    sf->design_range_top = rint(10.0*top);
    sf->fontstyle_id = id;

    if ( !PySequence_Check(names)) {
	PyErr_Format(PyExc_TypeError,"Final argument must be a tuple of tuples");
return( -1 );
    }
    head = last = NULL;
    for ( i=0; i<PySequence_Size(names); ++i ) {
	PyObject *subtuple = PySequence_GetItem(names,i);
	PyObject *val;
	int lang;

	if ( !PySequence_Check(subtuple)) {
	    PyErr_Format(PyExc_TypeError, "Value must be a tuple" );
    return(0);
	}

	val = PySequence_GetItem(subtuple,0);
	if ( PyString_Check(val) ) {
	    char *lang_str = PyString_AsString(val);
	    lang = FlagsFromString(lang_str,sfnt_name_mslangs);
	    if ( lang==0x80000000 ) {
		PyErr_Format(PyExc_TypeError, "Unknown language" );
return( 0 );
	    }
	} else if ( PyInt_Check(val))
	    lang = PyInt_AsLong(val);
	else {
	    PyErr_Format(PyExc_TypeError, "Unknown language" );
return( 0 );
	}

	string = PyString_AsString(PySequence_GetItem(subtuple,2));
	if ( string==NULL )
return( 0 );
	cur = chunkalloc(sizeof( struct otfname ));
	cur->name = copy(string);
	cur->lang = lang;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
    OtfNameListFree(sf->fontstyle_name);
    sf->fontstyle_name = head;
    
return( 0 );
}

static PyObject *PyFF_Font_get_cidsubfontcnt(PyFF_Font *self,void *closure) {
return( Py_BuildValue("i", self->fv->sf->subfontcnt));
}

static PyObject *PyFF_Font_get_cidsubfontnames(PyFF_Font *self,void *closure) {
    PyObject *tuple;
    SplineFont *sf = self->fv->sf;
    int i;

    tuple = PyTuple_New(sf->subfontcnt);
    for ( i=0; i<sf->subfontcnt; ++i )
	PyTuple_SET_ITEM(tuple,i,Py_BuildValue("s",sf->subfonts[i]->fontname));
return( tuple );
}

static PyObject *PyFF_Font_get_encoding(PyFF_Font *self,void *closure) {
return( Py_BuildValue("s", self->fv->map->enc->enc_name));
}

static int PyFF_Font_set_encoding(PyFF_Font *self,PyObject *value,void *closure) {
    FontView *fv = self->fv;
    char *encname;
    Encoding *new_enc;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete encoding field" );
return( -1 );
    }
    encname = PyString_AsString(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    if ( strmatch(encname,"compacted")==0 ) {
	fv->normal = EncMapCopy(fv->map);
	CompactEncMap(fv->map,fv->sf);
    } else {
	new_enc = FindOrMakeEncoding(encname);
	if ( new_enc==NULL ) {
	    PyErr_Format(PyExc_NameError, "Unknown encoding %s", encname);
return -1;
	}
	if ( new_enc==&custom )
	    fv->map->enc = &custom;
	else {
	    EncMap *map = EncMapFromEncoding(fv->sf,new_enc);
	    EncMapFree(fv->map);
	    fv->map = map;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    if ( !no_windowing_ui )
		FVSetTitle(fv);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	}
	if ( fv->normal!=NULL ) {
	    EncMapFree(fv->normal);
	    fv->normal = NULL;
	}
	SFReplaceEncodingBDFProps(fv->sf,fv->map);
    }
    free(fv->selected);
    fv->selected = gcalloc(fv->map->enccount,sizeof(char));
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( !no_windowing_ui )
	FontViewReformatAll(fv->sf);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    
return(0);
}

static PyObject *PyFF_Font_get_is_quadratic(PyFF_Font *self,void *closure) {
return( Py_BuildValue("i", self->fv->sf->order2));
}

static int PyFF_Font_set_is_quadratic(PyFF_Font *self,PyObject *value,void *closure) {
    SplineFont *sf = self->fv->sf;
    int order2;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete is_quadratic field" );
return( -1 );
    }
    order2 = PyInt_AsLong(value);
    if ( PyErr_Occurred()!=NULL )
return( -1 );
    if ( sf->order2==order2 )
	/* Do Nothing */;
    else if ( order2 ) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	SFCloseAllInstrs(sf);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	SFConvertToOrder2(sf);
    } else
	SFConvertToOrder3(sf);
return(0);
}

static PyObject *PyFF_Font_get_guide(PyFF_Font *self,void *closure) {
return( (PyObject *) LayerFromLayer(&self->fv->sf->grid,NULL));
}

static int PyFF_Font_set_guide(PyFF_Font *self,PyObject *value,void *closure) {
    SplineSet *ss = NULL, *newss;
    int isquad = false;
    SplineFont *sf;
    Layer *guide;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete guide field" );
return( -1 );
    }
    if ( PyType_IsSubtype(&PyFF_LayerType,value->ob_type) ) {
	isquad = ((PyFF_Layer *) value)->is_quadratic;
	ss = SSFromLayer( (PyFF_Layer *) value);
    } else if ( PyType_IsSubtype(&PyFF_ContourType,value->ob_type) ) {
	isquad = ((PyFF_Contour *) value)->is_quadratic;
	ss = SSFromContour( (PyFF_Contour *) value,NULL);
    } else {
	PyErr_Format( PyExc_TypeError, "Unexpected type" );
return( -1 );
    }
    sf = self->fv->sf;
    guide = &sf->grid;
    SplinePointListsFree(guide->splines);
    if ( sf->order2!=isquad ) {
	if ( sf->order2 )
	    newss = SplineSetsTTFApprox(ss);
	else
	    newss = SplineSetsPSApprox(ss);
	SplinePointListsFree(ss);
	ss = newss;
    }
    guide->splines = ss;
return( 0 );
}

static PyObject *PyFF_Font_get_mark_classes(PyFF_Font *self,void *closure) {
    PyObject *tuple, *nametuple;
    SplineFont *sf = self->fv->sf;
    int i;

    if ( sf->mark_class_cnt==0 )
Py_RETURN_NONE;

    tuple = PyTuple_New(sf->mark_class_cnt-1);
    for ( i=1; i<sf->mark_class_cnt; ++i ) {
	nametuple = TupleOfGlyphNames(sf->mark_classes[i],0);
	PyTuple_SetItem(tuple,i-1,Py_BuildValue("(sO)", sf->mark_class_names[i], nametuple));
    }
return( tuple );
}

static int PyFF_Font_set_mark_classes(PyFF_Font *self,PyObject *value,void *closure) {
    SplineFont *sf = self->fv->sf;
    int i, cnt;
    char **names, **classes;
    char *nm;
    PyObject *subtuple;

    if ( value==NULL || value==Py_None )
	cnt = 0;
    else {
	cnt = PySequence_Size(value);
	if ( cnt==-1 )
return( -1 );
	if ( cnt>=256 ) {
	    PyErr_Format(PyExc_ValueError, "There may be at most 255 mark classes" );
return( -1 );
	}
    }
    if ( cnt==0 ) {
	MarkClassFree(sf->mark_class_cnt,sf->mark_classes,sf->mark_class_names);
	sf->mark_class_cnt = 0;
	sf->mark_classes = NULL;
	sf->mark_class_names = NULL;
return( 0 );
    }

    names = galloc((cnt+1)*sizeof(char *));
    classes = galloc((cnt+1)*sizeof(char *));
    names[0] = classes[0] = NULL;
    for ( i=0; i<cnt; ++i ) {
	if ( !PyArg_ParseTuple(PySequence_GetItem(value,i),"(sO)", &nm, &subtuple))
return( -1 );
	classes[i+1] = GlyphNamesFromTuple(subtuple);
	if ( classes[i+1]==NULL )
return( -1 );
	names[i+1] = copy(nm);
    }

    MarkClassFree(sf->mark_class_cnt,sf->mark_classes,sf->mark_class_names);
    sf->mark_class_cnt = cnt+1;
    sf->mark_classes = classes;
    sf->mark_class_names = names;
    
return( 0 );
}

static PyObject *PyFF_Font_get_em(PyFF_Font *self,void *closure) {
return( Py_BuildValue("i", self->fv->sf->ascent + self->fv->sf->descent ));
}

static int PyFF_Font_set_em(PyFF_Font *self,PyObject *value,void *closure) {
    int newem, as, ds, oldem;
    SplineFont *sf;

    if ( value==NULL ) {
	PyErr_Format(PyExc_TypeError, "Cannot delete em field" );
return( -1 );
    }
    if ( !PyInt_Check(value)) {
	PyErr_Format( PyExc_TypeError, "Unexpected type" );
return( -1 );
    }
    newem = PyInt_AsLong(value);
    if ( newem<10 || newem>=16*1024 ) {
	PyErr_Format(PyExc_ValueError, "Em size too big or too small" );
return( -1 );
    }
    sf = self->fv->sf;
    if ( (oldem = sf->ascent+sf->descent)<=0 ) oldem = 1;
    ds = newem * sf->descent /oldem;
    as = newem - ds;
    SFScaleToEm(sf,as,ds);
return( 0 );
}

static int PyFF_Font_SetMaxpValue(PyFF_Font *self,PyObject *value,char *str) {
    SplineFont *sf = self->fv->sf;
    struct ttf_table *tab;
    int val;

    val = PyInt_AsLong(value);
    if ( PyErr_Occurred())
return( -1 );

    tab = SFFindTable(sf,CHR('m','a','x','p'));
    if ( tab==NULL ) {
	tab = chunkalloc(sizeof(struct ttf_table));
	tab->next = sf->ttf_tables;
	sf->ttf_tables = tab;
	tab->tag = CHR('m','a','x','p');
    }
    if ( tab->len<32 ) {
	tab->data = grealloc(tab->data,32);
	memset(tab->data+tab->len,0,32-tab->len);
	if ( tab->len<16 )
	    tab->data[15] = 2;			/* Default zones to 2 */
	tab->len = tab->maxlen = 32;
    }
    if ( strmatch(str,"Zones")==0 )
	memputshort(tab->data,7*sizeof(uint16),val);
    else if ( strmatch(str,"TwilightPntCnt")==0 )
	memputshort(tab->data,8*sizeof(uint16),val);
    else if ( strmatch(str,"StorageCnt")==0 )
	memputshort(tab->data,9*sizeof(uint16),val);
    else if ( strmatch(str,"MaxStackDepth")==0 )
	memputshort(tab->data,12*sizeof(uint16),val);
    else if ( strmatch(str,"FDEFs")==0 )
	memputshort(tab->data,10*sizeof(uint16),val);
    else if ( strmatch(str,"IDEFs")==0 )
	memputshort(tab->data,11*sizeof(uint16),val);
return( 0 );
}

static PyObject *PyFF_Font_GetMaxpValue(PyFF_Font *self,char *str) {
    SplineFont *sf = self->fv->sf;
    struct ttf_table *tab;
    uint8 *data, dummy[32];
    int val;

    memset(dummy,0,32);
    dummy[15] = 2;
    tab = SFFindTable(sf,CHR('m','a','x','p'));
    if ( tab==NULL )
	data = dummy;
    else if ( tab->len<32 ) {
	memcpy(dummy,tab->data,tab->len);
	data = dummy;
    } else
	data = tab->data;

    if ( strmatch(str,"Zones")==0 )
	val = memushort(data,32,7*sizeof(uint16));
    else if ( strmatch(str,"TwilightPntCnt")==0 )
	val = memushort(data,32,8*sizeof(uint16));
    else if ( strmatch(str,"StorageCnt")==0 )
	val = memushort(data,32,9*sizeof(uint16));
    else if ( strmatch(str,"MaxStackDepth")==0 )
	val = memushort(data,32,12*sizeof(uint16));
    else if ( strmatch(str,"FDEFs")==0 )
	val = memushort(data,32,10*sizeof(uint16));
    else if ( strmatch(str,"IDEFs")==0 )
	val = memushort(data,32,11*sizeof(uint16));
    else
	val = -1;
return( Py_BuildValue("i",val));
}

static PyObject *PyFF_Font_get_maxp_IDEFs(PyFF_Font *self,void *closure) {
return( PyFF_Font_GetMaxpValue(self,"IDEFs"));
}

static int PyFF_Font_set_maxp_IDEFs(PyFF_Font *self,PyObject *value,void *closure) {
return( PyFF_Font_SetMaxpValue(self,value,"IDEFs"));
}

static PyObject *PyFF_Font_get_maxp_FDEFs(PyFF_Font *self,void *closure) {
return( PyFF_Font_GetMaxpValue(self,"FDEFs"));
}

static int PyFF_Font_set_maxp_FDEFs(PyFF_Font *self,PyObject *value,void *closure) {
return( PyFF_Font_SetMaxpValue(self,value,"FDEFs"));
}

static PyObject *PyFF_Font_get_maxp_maxStackDepth(PyFF_Font *self,void *closure) {
return( PyFF_Font_GetMaxpValue(self,"MaxStackDepth"));
}

static int PyFF_Font_set_maxp_maxStackDepth(PyFF_Font *self,PyObject *value,void *closure) {
return( PyFF_Font_SetMaxpValue(self,value,"MaxStackDepth"));
}

static PyObject *PyFF_Font_get_maxp_storageCnt(PyFF_Font *self,void *closure) {
return( PyFF_Font_GetMaxpValue(self,"StorageCnt"));
}

static int PyFF_Font_set_maxp_storageCnt(PyFF_Font *self,PyObject *value,void *closure) {
return( PyFF_Font_SetMaxpValue(self,value,"StorageCnt"));
}

static PyObject *PyFF_Font_get_maxp_twilightPtCnt(PyFF_Font *self,void *closure) {
return( PyFF_Font_GetMaxpValue(self,"TwilightPntCnt"));
}

static int PyFF_Font_set_maxp_twilightPtCnt(PyFF_Font *self,PyObject *value,void *closure) {
return( PyFF_Font_SetMaxpValue(self,value,"TwilightPntCnt"));
}

static PyObject *PyFF_Font_get_maxp_zones(PyFF_Font *self,void *closure) {
return( PyFF_Font_GetMaxpValue(self,"Zones"));
}

static int PyFF_Font_set_maxp_zones(PyFF_Font *self,PyObject *value,void *closure) {
return( PyFF_Font_SetMaxpValue(self,value,"Zones"));
}

static PyGetSetDef PyFF_Font_getset[] = {
    {"userdata",
	 (getter)PyFF_Font_get_userdata, (setter)PyFF_Font_set_userdata,
	 "arbetrary user data", NULL},
    {"sfnt_names",
	 (getter)PyFF_Font_get_sfntnames, (setter)PyFF_Font_set_sfntnames,
	 "The sfnt 'name' table. A tuple of all ms names.\nEach name is itself a tuple of strings (language,strid,name)\nMac names will be automagically created from ms names", NULL},
    {"bitmapSizes",
	 (getter)PyFF_Font_get_bitmapSizes, (setter)PyFF_Font_set_bitmapSizes,
	 "A tuple of sizes of all bitmaps associated with the font", NULL},
    {"gpos_lookups",
	 (getter)PyFF_Font_get_gpos_lookups, (setter)PyFF_cant_set,
	 "The names of all lookups in the font's GPOS table", NULL},
    {"gsub_lookups",
	 (getter)PyFF_Font_get_gsub_lookups, (setter)PyFF_cant_set,
	 "The names of all lookups in the font's GSUB table", NULL},
    {"private",
	 (getter)PyFF_Font_get_private, (setter)PyFF_cant_set,
	 "The font's PostScript private dictionary", NULL},
    {"selection",
	 (getter)PyFF_Font_get_selection, (setter)PyFF_Font_set_selection,
	 "The font's PostScript private dictionary", NULL},
    {"cvt",
	 (getter)PyFF_Font_get_cvt, (setter)PyFF_Font_set_cvt,
	 "The font's TrueType cvt table", NULL},
    {"path",
	 (getter)PyFF_Font_get_path, (setter)PyFF_cant_set,
	 "filename of the original font file loaded", NULL},
    {"sfd_path",
	 (getter)PyFF_Font_get_sfd_path, (setter)PyFF_cant_set,
	 "filename of the sfd file containing this font (if any)", NULL},
    {"fontname",
	 (getter)PyFF_Font_get_fontname, (setter)PyFF_Font_set_fontname,
	 "font name", NULL},
    {"fullname",
	 (getter)PyFF_Font_get_fullname, (setter)PyFF_Font_set_fullname,
	 "full name", NULL},
    {"familyname",
	 (getter)PyFF_Font_get_familyname, (setter)PyFF_Font_set_familyname,
	 "family name", NULL},
    {"weight",
	 (getter)PyFF_Font_get_weight, (setter)PyFF_Font_set_weight,
	 "weight (PS)", NULL},
    {"copyright",
	 (getter)PyFF_Font_get_copyright, (setter)PyFF_Font_set_copyright,
	 "copyright (PS)", NULL},
    {"version",
	 (getter)PyFF_Font_get_version, (setter)PyFF_Font_set_version,
	 "font version (PS)", NULL},
    {"xuid",
	 (getter)PyFF_Font_get_xuid, (setter)PyFF_Font_set_xuid,
	 "PostScript eXtended Unique ID", NULL},
    {"fondname",
	 (getter)PyFF_Font_get_fondname, (setter)PyFF_Font_set_fondname,
	 "Mac FOND resource name", NULL},
    {"cidregistry",
	 (getter)PyFF_Font_get_cidregistry, (setter)PyFF_Font_set_cidregistry,
	 "CID Registry", NULL},
    {"cidordering",
	 (getter)PyFF_Font_get_ordering, (setter)PyFF_Font_set_ordering,
	 "CID Ordering", NULL},
    {"cidsubfontcnt",
	 (getter)PyFF_Font_get_cidsubfontcnt, (setter)PyFF_cant_set,
	 "The number of sub fonts that make up a CID keyed font", NULL},
    {"cidsubfontnames",
	 (getter)PyFF_Font_get_cidsubfontnames, (setter)PyFF_cant_set,
	 "The names of all the sub fonts that make up a CID keyed font", NULL},
    {"italicangle",
	 (getter)PyFF_Font_get_italicangle, (setter)PyFF_Font_set_italicangle,
	 "The Italic angle (skewedness) of the font", NULL},
    {"upos",
	 (getter)PyFF_Font_get_upos, (setter)PyFF_Font_set_upos,
	 "Underline Position", NULL},
    {"uwidth",
	 (getter)PyFF_Font_get_uwidth, (setter)PyFF_Font_set_uwidth,
	 "Underline Width", NULL},
    {"cidversion",
	 (getter)PyFF_Font_get_cidversion, (setter)PyFF_Font_set_cidversion,
	 "CID Version", NULL},
    {"strokewidth",
	 (getter)PyFF_Font_get_strokewidth, (setter)PyFF_Font_set_strokewidth,
	 "Stroke Width", NULL},
    {"ascent",
	 (getter)PyFF_Font_get_ascent, (setter)PyFF_Font_set_ascent,
	 "Font Ascent", NULL},
    {"descent",
	 (getter)PyFF_Font_get_descent, (setter)PyFF_Font_set_descent,
	 "Font Descent", NULL},
    {"em",
	 (getter)PyFF_Font_get_em, (setter)PyFF_Font_set_em,
	 "Em size", NULL},
    {"vertical_origin",
	 (getter)PyFF_Font_get_vertical_origin, (setter)PyFF_Font_set_vertical_origin,
	 "Vertical Origin", NULL},
    {"uniqueid",
	 (getter)PyFF_Font_get_uniqueid, (setter)PyFF_Font_set_uniqueid,
	 "PostScript Unique ID", NULL},
    {"cidsupplement",
	 (getter)PyFF_Font_get_supplement, (setter)PyFF_Font_set_supplement,
	 "CID Supplement", NULL},
    {"macstyle",
	 (getter)PyFF_Font_get_macstyle, (setter)PyFF_Font_set_macstyle,
	 "Mac Style Bits", NULL},
    {"design_size",
	 (getter)PyFF_Font_get_design_size, (setter)PyFF_Font_set_design_size,
	 "Point size for which this font was designed", NULL},
    {"size_feature",
	 (getter)PyFF_Font_get_size_feature, (setter)PyFF_Font_set_size_feature,
	 "A tuple containing the info needed for the 'size' feature", NULL},
    {"gasp_version",
	 (getter)PyFF_Font_get_gasp_version, (setter)PyFF_Font_set_gasp_version,
	 "Gasp table version number", NULL},
    {"gasp",
	 (getter)PyFF_Font_get_gasp, (setter)PyFF_Font_set_gasp,
	 "Gasp table as a tuple", NULL},
    {"maxp_zones",
	 (getter)PyFF_Font_get_maxp_zones, (setter)PyFF_Font_set_maxp_zones,
	 "The number of zones used in the tt program", NULL},
    {"maxp_twilightPtCnt",
	 (getter)PyFF_Font_get_maxp_twilightPtCnt, (setter)PyFF_Font_set_maxp_twilightPtCnt,
	 "The number of points in the twilight zone of the tt program", NULL},
    {"maxp_storageCnt",
	 (getter)PyFF_Font_get_maxp_storageCnt, (setter)PyFF_Font_set_maxp_storageCnt,
	 "The number of storage locations used by the tt program", NULL},
    {"maxp_maxStackDepth",
	 (getter)PyFF_Font_get_maxp_maxStackDepth, (setter)PyFF_Font_set_maxp_maxStackDepth,
	 "The maximum stack depth used by the tt program", NULL},
    {"maxp_FDEFs",
	 (getter)PyFF_Font_get_maxp_FDEFs, (setter)PyFF_Font_set_maxp_FDEFs,
	 "The number of function definitions used by the tt program", NULL},
    {"maxp_IDEFs",
	 (getter)PyFF_Font_get_maxp_IDEFs, (setter)PyFF_Font_set_maxp_IDEFs,
	 "The number of instruction definitions used by the tt program", NULL},
    {"os2_version",
	 (getter)PyFF_Font_get_os2_version, (setter)PyFF_Font_set_os2_version,
	 "OS/2 table version number", NULL},
    {"os2_weight",
	 (getter)PyFF_Font_get_OS2_weight, (setter)PyFF_Font_set_OS2_weight,
	 "OS/2 weight", NULL},
    {"os2_width",
	 (getter)PyFF_Font_get_OS2_width, (setter)PyFF_Font_set_OS2_width,
	 "OS/2 width", NULL},
    {"os2_fstype",
	 (getter)PyFF_Font_get_OS2_fstype, (setter)PyFF_Font_set_OS2_fstype,
	 "OS/2 fstype", NULL},
    {"hhea_linegap",
	 (getter)PyFF_Font_get_OS2_linegap, (setter)PyFF_Font_set_OS2_linegap,
	 "hhea linegap", NULL},
    {"hhea_ascent",
	 (getter)PyFF_Font_get_OS2_hhead_ascent, (setter)PyFF_Font_set_OS2_hhead_ascent,
	 "hhea ascent", NULL},
    {"hhea_descent",
	 (getter)PyFF_Font_get_OS2_hhead_descent, (setter)PyFF_Font_set_OS2_hhead_descent,
	 "hhea descent", NULL},
    {"hhea_ascent_add",
	 (getter)PyFF_Font_get_OS2_hheadascent_add, (setter)PyFF_Font_set_OS2_hheadascent_add,
	 "Whether the hhea_ascent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {"hhea_descent_add",
	 (getter)PyFF_Font_get_OS2_hheaddescent_add, (setter)PyFF_Font_set_OS2_hheaddescent_add,
	 "Whether the hhea_descent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {"vhea_linegap",
	 (getter)PyFF_Font_get_OS2_vlinegap, (setter)PyFF_Font_set_OS2_vlinegap,
	 "vhea linegap", NULL},
    {"os2_typoascent",
	 (getter)PyFF_Font_get_OS2_os2_typoascent, (setter)PyFF_Font_set_OS2_os2_typoascent,
	 "OS/2 Typographic Ascent", NULL},
    {"os2_typodescent",
	 (getter)PyFF_Font_get_OS2_os2_typodescent, (setter)PyFF_Font_set_OS2_os2_typodescent,
	 "OS/2 Typographic Descent", NULL},
    {"os2_typolinegap",
	 (getter)PyFF_Font_get_OS2_os2_typolinegap, (setter)PyFF_Font_set_OS2_os2_typolinegap,
	 "OS/2 Typographic Linegap", NULL},
    {"os2_typoascent_add",
	 (getter)PyFF_Font_get_OS2_typoascent_add, (setter)PyFF_Font_set_OS2_typoascent_add,
	 "Whether the os2_typoascent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {"os2_typodescent_add",
	 (getter)PyFF_Font_get_OS2_typodescent_add, (setter)PyFF_Font_set_OS2_typodescent_add,
	 "Whether the os2_typodescent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {"os2_winascent",
	 (getter)PyFF_Font_get_OS2_os2_winascent, (setter)PyFF_Font_set_OS2_os2_winascent,
	 "OS/2 Windows Ascent", NULL},
    {"os2_windescent",
	 (getter)PyFF_Font_get_OS2_os2_windescent, (setter)PyFF_Font_set_OS2_os2_windescent,
	 "OS/2 Windows Descent", NULL},
    {"os2_winascent_add",
	 (getter)PyFF_Font_get_OS2_winascent_add, (setter)PyFF_Font_set_OS2_winascent_add,
	 "Whether the os2_winascent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {"os2_windescent_add",
	 (getter)PyFF_Font_get_OS2_windescent_add, (setter)PyFF_Font_set_OS2_windescent_add,
	 "Whether the os2_windescent field is used as is, or as an offset applied to the value FontForge thinks appropriate", NULL},
    {"os2_subxsize",
	 (getter)PyFF_Font_get_OS2_os2_subxsize, (setter)PyFF_Font_set_OS2_os2_subxsize,
	 "OS/2 Subscript XSize", NULL},
    {"os2_subxoff",
	 (getter)PyFF_Font_get_OS2_os2_subxoff, (setter)PyFF_Font_set_OS2_os2_subxoff,
	 "OS/2 Subscript XOffset", NULL},
    {"os2_subysize",
	 (getter)PyFF_Font_get_OS2_os2_subysize, (setter)PyFF_Font_set_OS2_os2_subysize,
	 "OS/2 Subscript YSize", NULL},
    {"os2_subyoff",
	 (getter)PyFF_Font_get_OS2_os2_subyoff, (setter)PyFF_Font_set_OS2_os2_subyoff,
	 "OS/2 Subscript YOffset", NULL},
    {"os2_supxsize",
	 (getter)PyFF_Font_get_OS2_os2_supxsize, (setter)PyFF_Font_set_OS2_os2_supxsize,
	 "OS/2 Superscript XSize", NULL},
    {"os2_supxoff",
	 (getter)PyFF_Font_get_OS2_os2_supxoff, (setter)PyFF_Font_set_OS2_os2_supxoff,
	 "OS/2 Superscript XOffset", NULL},
    {"os2_supysize",
	 (getter)PyFF_Font_get_OS2_os2_supysize, (setter)PyFF_Font_set_OS2_os2_supysize,
	 "OS/2 Superscript YSize", NULL},
    {"os2_supyoff",
	 (getter)PyFF_Font_get_OS2_os2_supyoff, (setter)PyFF_Font_set_OS2_os2_supyoff,
	 "OS/2 Superscript YOffset", NULL},
    {"os2_strikeysize",
	 (getter)PyFF_Font_get_OS2_os2_strikeysize, (setter)PyFF_Font_set_OS2_os2_strikeysize,
	 "OS/2 Strikethrough YSize", NULL},
    {"os2_strikeypos",
	 (getter)PyFF_Font_get_OS2_os2_strikeypos, (setter)PyFF_Font_set_OS2_os2_strikeypos,
	 "OS/2 Strikethrough YPosition", NULL},
    {"os2_family_class",
	 (getter)PyFF_Font_get_OS2_os2_family_class, (setter)PyFF_Font_set_OS2_os2_family_class,
	 "OS/2 Family Class", NULL},
    {"os2_use_typo_metrics",
	 (getter)PyFF_Font_get_use_typo_metrics, (setter)PyFF_Font_set_use_typo_metrics,
	 "OS/2 Flag MS thinks is necessary to encourage people to follow the standard and use typographic metrics", NULL},
    {"os2_weight_width_slope_only",
	 (getter)PyFF_Font_get_weight_width_slope_only, (setter)PyFF_Font_set_weight_width_slope_only,
	 "OS/2 Flag MS thinks is necessary", NULL},
    {"os2_panose",
	 (getter)PyFF_Font_get_OS2_panose, (setter)PyFF_Font_set_OS2_panose,
	 "The 10 element OS/2 Panose tuple", NULL},
    {"os2_vendor",
	 (getter)PyFF_Font_get_OS2_vendor, (setter)PyFF_Font_set_OS2_vendor,
	 "The 4 character OS/2 vendor string", NULL},
    {"changed",
	 (getter)PyFF_Font_get_changed, (setter)PyFF_Font_set_changed,
	 "Flag indicating whether the font has been changed since it was loaded (read only)", NULL},
    {"isnew",
	 (getter)PyFF_Font_get_new, (setter)PyFF_cant_set,
	 "Flag indicating whether the font is new (read only)", NULL},
    {"hasvmetrics",
	 (getter)PyFF_Font_get_hasvmetrics, (setter)PyFF_Font_set_hasvmetrics,
	 "Flag indicating whether the font contains vertical metrics", NULL},
    {"onlybitmaps",
	 (getter)PyFF_Font_get_onlybitmaps, (setter)PyFF_Font_set_onlybitmaps,
	 "Flag indicating whether the font contains bitmap strikes but no outlines", NULL},
    {"encoding",
	 (getter)PyFF_Font_get_encoding, (setter)PyFF_Font_set_encoding,
	 "The encoding used for indexing the font", NULL },
    {"is_quadratic",
	 (getter)PyFF_Font_get_is_quadratic, (setter)PyFF_Font_set_is_quadratic,
	 "Flag indicating whether the font contains quadratic splines (truetype) or cubic (postscript)", NULL},
    {"multilayer",
	 (getter)PyFF_Font_get_multilayer, (setter)PyFF_cant_set,
	 "Flag indicating whether the font is multilayered (type3) or not", NULL},
    {"strokedfont",
	 (getter)PyFF_Font_get_strokedfont, (setter)PyFF_Font_set_strokedfont,
	 "Flag indicating whether the font is a stroked font or not", NULL},
    {"guide",
	 (getter)PyFF_Font_get_guide, (setter)PyFF_Font_set_guide,
	 "The Contours that make up the guide layer of the font", NULL},
    {"markClasses",
	 (getter)PyFF_Font_get_mark_classes, (setter)PyFF_Font_set_mark_classes,
	 "A tuple each entry of which is itself a tuple containing a mark-class-name and a tuple of glyph-names", NULL},
    {NULL}  /* Sentinel */
};

/* ************************************************************************** */
/* Font Methods */
/* ************************************************************************** */

static uint32 StrToTag(char *tag_name, int *was_mac) {
    uint8 foo[4];
    int feat, set;

    if ( was_mac!=NULL && sscanf(tag_name,"<%d,%d>", &feat, &set )==2 ) {
	*was_mac = true;
return( (feat<<16) | set );
    }

    if ( was_mac ) *was_mac = false;
    foo[0] = foo[1] = foo[2] = foo[3] = ' ';
    if ( *tag_name!='\0' ) {
	foo[0] = tag_name[0];
	if ( tag_name[1]!='\0' ) {
	    foo[1] = tag_name[1];
	    if ( tag_name[2]!='\0' ) {
		foo[2] = tag_name[2];
		if ( tag_name[3]!='\0' ) {
		    foo[3] = tag_name[3];
		    if ( tag_name[4]!='\0' ) {
			PyErr_Format(PyExc_TypeError, "OpenType tags are limited to 4 characters: %s", tag_name);
return( 0xffffffff );
		    }
		}
	    }
	}
    }
return( (foo[0]<<24) | (foo[1]<<16) | (foo[2]<<8) | foo[3] );
}

static PyObject *TagToPyString(uint32 tag,int ismac) {
    char foo[30];

    if ( ismac ) {
	sprintf( foo,"<%d,%d>", tag>>16, tag&0xffff );
    } else {
	foo[0] = tag>>24;
	foo[1] = tag>>16;
	foo[2] = tag>>8;
	foo[3] = tag;
	foo[4] = '\0';
    }
return( PyString_FromString(foo));
}
		    
static PyObject *PyFFFont_GetTableData(PyObject *self, PyObject *args) {
    char *table_name;
    uint32 tag;
    struct ttf_table *tab;
    PyObject *binstr;

    if ( !PyArg_ParseTuple(args,"s",&table_name) )
return( NULL );
    tag = StrToTag(table_name,NULL);
    if ( tag==0xffffffff )
return( NULL );

    for ( tab=((PyFF_Font *) self)->fv->sf->ttf_tables; tab!=NULL && tab->tag!=tag; tab=tab->next );
    if ( tab==NULL )
	for ( tab=((PyFF_Font *) self)->fv->sf->ttf_tab_saved; tab!=NULL && tab->tag!=tag; tab=tab->next );

    if ( tab==NULL )
Py_RETURN_NONE;

    binstr = PyString_FromStringAndSize((char *) tab->data,tab->len);
return( binstr );
}

static void TableAddInstrs(SplineFont *sf, uint32 tag,int replace,
	uint8 *instrs,int icnt) {
    struct ttf_table *tab;

    for ( tab=sf->ttf_tables; tab!=NULL && tab->tag!=tag; tab=tab->next );
    if ( tab==NULL )
	for ( tab=sf->ttf_tab_saved; tab!=NULL && tab->tag!=tag; tab=tab->next );

    if ( replace && tab!=NULL ) {
	free(tab->data);
	tab->data = NULL;
	tab->len = tab->maxlen = 0;
    }
    if ( icnt==0 )
return;
    if ( tab==NULL ) {
	tab = chunkalloc(sizeof( struct ttf_table ));
	tab->tag = tag;
	if ( tag==CHR('p','r','e','p') || tag==CHR('f','p','g','m') ||
		tag==CHR('c','v','t',' ') || tag==CHR('m','a','x','p') ) {
	    tab->next = sf->ttf_tables;
	    sf->ttf_tables = tab;
	} else {
	    tab->next = sf->ttf_tab_saved;
	    sf->ttf_tab_saved = tab;
	}
    }
    if ( tab->data==NULL ) {
	tab->data = galloc(icnt);
	memcpy(tab->data,instrs,icnt);
	tab->len = icnt;
    } else {
	uint8 *newi = galloc(icnt+tab->len);
	memcpy(newi,tab->data,tab->len);
	memcpy(newi+tab->len,instrs,icnt);
	free(tab->data);
	tab->data = newi;
	tab->len += icnt;
    }
    tab->maxlen = tab->len;
}

static PyObject *PyFFFont_SetTableData(PyObject *self, PyObject *args) {
    char *table_name;
    uint32 tag;
    PyObject *tuple;
    uint8 *instrs;
    int icnt, i;

    if ( !PyArg_ParseTuple(args,"sO",&table_name,&tuple) )
return( NULL );
    tag = StrToTag(table_name,NULL);
    if ( tag==0xffffffff )
return( NULL );

    if ( !PySequence_Check(tuple)) {
	PyErr_Format(PyExc_TypeError, "Argument must be a tuple" );
return( NULL );
    }
    if ( PyString_Check(tuple)) {
	char *space; int len;
	PyString_AsStringAndSize(tuple,&space,&len);
	instrs = gcalloc(len,sizeof(uint8));
	icnt = len;
	memcpy(instrs,space,len);
    } else {
	icnt = PySequence_Size(tuple);
	instrs = galloc(icnt);
	for ( i=0; i<icnt; ++i ) {
	    instrs[i] = PyInt_AsLong(PySequence_GetItem(tuple,i));
	    if ( PyErr_Occurred())
return( NULL );
	}
    }
    TableAddInstrs(((PyFF_Font *) self)->fv->sf,tag,true,instrs,icnt);
    free(instrs);
Py_RETURN(self);
}

static PyObject *PyFFFont_regenBitmaps(PyFF_Font *self,PyObject *args) {
    if ( bitmapper(self,args,false)==-1 )
return( NULL );

Py_RETURN(self);
}

static struct flaglist compflags[] = {
    { "outlines",		  1 },
    { "outlines-exactly",	  2 },
    { "warn-outlines-mismatch",	  4 },
    { "hints",			  8 },
    { "warn-refs-unlink",	  0x40 },
    { "strikes",		  0x80 },
    { "fontnames",		  0x100 },
    { "gpos",			  0x200 },
    { "gsub",			  0x400 },
    { "add-outlines",		  0x800 },
    { "create-glyphs",		  0x1000 },
    NULL };

static PyObject *PyFFFont_compareFonts(PyFF_Font *self,PyObject *args) {
    /* Compare the current font against the named one	     */
    /* output to a file (used /dev/null if no output wanted) */
    /* flags control what tests are done		     */
    PyFF_Font *other;
    PyObject *flagstuple, *ret;
    FILE *diffs;
    int flags;
    char *filename, *locfilename;

    if ( !PyArg_ParseTuple(args,"OesO", &other, "UTF-8", &filename, &flagstuple ))
return( NULL );
    locfilename = utf82def_copy(filename);
    free(filename);

    if ( !PyType_IsSubtype(&PyFF_FontType,other->ob_type) ) {
	PyErr_Format(PyExc_TypeError,"First argument must be a fontforge font");
return( NULL );
    }
    flags = FlagsFromTuple(flagstuple,compflags);
    if ( flags==0x80000000 )
return( NULL );

    if ( strcmp(filename,"-")==0 )
	diffs = stdout;
    else
	diffs = fopen(filename,"w");
    if ( diffs==NULL ) {
	PyErr_Format(PyExc_EnvironmentError,"Failed to open output file: %s", locfilename);
return( NULL );
    }

    free( locfilename );

    ret = Py_BuildValue("i", CompareFonts(self->fv->sf, self->fv->map, other->fv->sf, diffs, flags ));
    if ( diffs!=stdout )
	fclose( diffs );
return( ret );
}

static PyObject *PyFFFont_appendSFNTName(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    struct ttflangname dummy;
    int i;

    memset(&dummy,0,sizeof(dummy));
    DefaultTTFEnglishNames(&dummy, sf);

    if ( !SetSFNTName(sf,args,&dummy) )
return( NULL );

    for ( i=0; i<ttf_namemax; ++i )
	free( dummy.names[i]);

Py_RETURN( self );
}

static struct lookup_subtable *addLookupSubtable(SplineFont *sf, char *lookup,
	char *new_subtable, char *after_str) {
    OTLookup *otl;
    struct lookup_subtable *sub, *after=NULL;
    int is_v;

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }
    if ( after_str!=NULL ) {
	after = SFFindLookupSubtable(sf,after_str);
	if ( after==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No lookup subtable named %s", after_str );
return( NULL );
	} else if ( after->lookup!=otl ) {
	    PyErr_Format(PyExc_EnvironmentError, "Subtable, %s, is not in lookup %s.", after_str, lookup );
return( NULL );
	}
    }

    if ( sf->cidmaster ) sf = sf->cidmaster;

    if ( SFFindLookupSubtable(sf,new_subtable)!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "A lookup subtable named %s already exists", new_subtable);
return( NULL );
    }

    sub = chunkalloc(sizeof(struct lookup_subtable));
    sub->lookup = otl;
    sub->subtable_name = copy(new_subtable);
    if ( after!=NULL ) {
	sub->next = after->next;
	after->next = sub;
    } else {
	sub->next = otl->subtables;
	otl->subtables = sub;
    }
    switch ( otl->lookup_type ) {
      case gpos_cursive: case gpos_mark2base: case gpos_mark2ligature: case gpos_mark2mark:
	sub->anchor_classes = true;
      break;
      case gpos_pair:
	is_v = VerticalKernFeature(sf,otl,false);
	if ( is_v==-1 ) is_v = false;
	sub->vertical_kerning = is_v;
	sub->per_glyph_pst_or_kern = true;
      break;
      case gpos_single:
      case gsub_single: case gsub_multiple: case gsub_alternate: case gsub_ligature:
	sub->per_glyph_pst_or_kern = true;
      break;
    }

return( sub );
}

static PyObject *PyFFFont_addAnchorClass(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *subtable, *anchor_name;
    struct lookup_subtable *sub;
    AnchorClass *ac;
    int lookup_type;

    if ( !PyArg_ParseTuple(args,"ss", &subtable, &anchor_name ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s", subtable );
return( NULL );
    }
    lookup_type = sub->lookup->lookup_type;
    if ( lookup_type<gpos_cursive || lookup_type>gpos_mark2mark ) {
	PyErr_Format(PyExc_EnvironmentError, "Cannot add an anchor class to %s, it has the wrong lookup type", subtable );
return( NULL );
    }
    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( strcmp(ac->name,anchor_name)==0 )
    break;
    }
    if ( ac!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "An anchor class named %s already exists", anchor_name );
return( NULL );
    }
    ac = chunkalloc(sizeof(AnchorClass));
    ac->name = copy( anchor_name );
    ac->subtable = sub;
    ac->type = lookup_type==gpos_cursive        ? act_curs :
		lookup_type==gpos_mark2base     ? act_mark :
		lookup_type==gpos_mark2ligature ? act_mklg :
						  act_mkmk ;
    ac->next = sf->anchor;
    sf->anchor = ac;

Py_RETURN( self );
}

static PyObject *PyFFFont_removeAnchorClass(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *anchor_name;
    AnchorClass *ac;

    if ( !PyArg_ParseTuple(args,"s", &anchor_name ))
return( NULL );

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next ) {
	if ( strcmp(ac->name,anchor_name)==0 )
    break;
    }
    if ( ac==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No anchor class named %s exists", anchor_name );
return( NULL );
    }
    SFRemoveAnchorClass(sf,ac);
Py_RETURN( self );
}

static int ParseClassNames(PyObject *classes,char ***class_strs) {
    int cnt, i;
    char **cls;

    *class_strs = NULL;
    cnt = PySequence_Size(classes);
    if ( cnt==-1 )
return( -1 );
    *class_strs = cls = galloc(cnt*sizeof(char *));
    for ( i=0; i<cnt; ++i ) {
	PyObject *thingy = PySequence_GetItem(classes,i);
	if ( i==0 && thingy==Py_None )
	    cls[i] = NULL;
	else {
	    cls[i] = GlyphNamesFromTuple(thingy);
	    if ( cls[i]==NULL )
return( -1 );
	}
    }
return( cnt );
}

static PyObject *MakeClassNameTuple(int cnt, char**classes) {
    PyObject *tuple;
    int i;

    tuple = PyTuple_New(cnt);
    for ( i=0; i<cnt; ++i ) {
	if ( classes[i]==NULL ) {
	    PyTuple_SetItem(tuple,i,Py_None);
	    Py_INCREF(Py_None);
	} else
	    PyTuple_SetItem(tuple,i,TupleOfGlyphNames(classes[i],0));
    }
return( tuple );
}

static PyObject *PyFFFont_addKerningClass(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *lookup, *subtable, *after_str=NULL;
    int i;
    struct lookup_subtable *sub;
    PyObject *class1s, *class2s, *offsets;
    char **class1_strs, **class2_strs;
    int cnt1, cnt2;
    int16 *offs;

    if ( !PyArg_ParseTuple(args,"sOOO", &subtable, &class1s, &class2s,
	    &offsets ))
return( NULL );

    cnt1 = ParseClassNames(class1s,&class1_strs);
    cnt2 = ParseClassNames(class2s,&class2_strs);
    if ( cnt1*cnt2 != PySequence_Size(offsets) ) {
	PyErr_Format(PyExc_ValueError, "There aren't enough kerning offsets for the number of kerning classes. Should be %d", cnt1*cnt2 );
return( NULL );
    }
    offs = galloc(cnt1*cnt2*sizeof(int16));
    for ( i=0 ; i<cnt1*cnt2; ++i ) {
	offs[i] = PyInt_AsLong(PySequence_GetItem(offsets,i));
	if ( PyErr_Occurred())
return( NULL );
    }

    sub = addLookupSubtable(sf, lookup, subtable, after_str);
    if ( sub==NULL )
return( NULL );
    if ( sub->lookup->lookup_type!=gpos_pair ) {
	PyErr_Format(PyExc_EnvironmentError, "Cannot add kerning data to %s, it has the wrong lookup type", lookup );
return( NULL );
    }
    sub->per_glyph_pst_or_kern = false;
    sub->kc = chunkalloc(sizeof(KernClass));
    sub->kc->subtable = sub;
    sub->kc->first_cnt = cnt1;
    sub->kc->second_cnt = cnt2;
    sub->kc->firsts = class1_strs;
    sub->kc->seconds = class2_strs;
    sub->kc->offsets = offs;

    if ( sub->vertical_kerning ) {
	sub->kc->next = sf->vkerns;
	sf->vkerns = sub->kc;
    } else {
	sub->kc->next = sf->kerns;
	sf->kerns = sub->kc;
    }

Py_RETURN( self );
}

static PyObject *PyFFFont_alterKerningClass(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *lookup, *subtable, *after_str=NULL;
    int i;
    struct lookup_subtable *sub;
    PyObject *class1s, *class2s, *offsets;
    char **class1_strs, **class2_strs;
    int cnt1, cnt2;
    int16 *offs;

    if ( !PyArg_ParseTuple(args,"ssOOO|s", &lookup, &subtable, &class1s, &class2s,
	    &offsets, &after_str ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s", subtable );
return( NULL );
    }
    if ( sub->kc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "This subtable, %s, does not contain not a kerning class", subtable );
return( NULL );
    }

    cnt1 = ParseClassNames(class1s,&class1_strs);
    cnt2 = ParseClassNames(class2s,&class2_strs);
    if ( cnt1*cnt2 != PySequence_Size(offsets) ) {
	PyErr_Format(PyExc_ValueError, "There aren't enough kerning offsets for the number of kerning classes. Should be %d", cnt1*cnt2 );
return( NULL );
    }
    offs = galloc(cnt1*cnt2*sizeof(int16));
    for ( i=0 ; i<cnt1*cnt2; ++i ) {
	offs[i] = PyInt_AsLong(PySequence_GetItem(offsets,i));
	if ( PyErr_Occurred())
return( NULL );
    }

    KernClassFreeContents(sub->kc);
    sub->kc->first_cnt = cnt1;
    sub->kc->second_cnt = cnt2;
    sub->kc->firsts = class1_strs;
    sub->kc->seconds = class2_strs;
    sub->kc->offsets = offs;

Py_RETURN( self );
}

static PyObject *PyFFFont_getKerningClass(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *subtable;
    struct lookup_subtable *sub;
    PyObject *offsets;
    int i;

    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s", subtable );
return( NULL );
    }
    if ( sub->kc==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "This subtable, %s, does not contain not a kerning class", subtable );
return( NULL );
    }
    offsets = PyTuple_New(sub->kc->first_cnt*sub->kc->second_cnt);
    for ( i=0; i<sub->kc->first_cnt*sub->kc->second_cnt; ++i )
	PyTuple_SetItem(offsets,i,PyInt_FromLong(sub->kc->offsets[i]));

return( Py_BuildValue("(OOO)",
	MakeClassNameTuple(sub->kc->first_cnt,sub->kc->firsts),
	MakeClassNameTuple(sub->kc->second_cnt,sub->kc->seconds),
	offsets));
}

static PyObject *PyFFFont_isKerningClass(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *subtable;
    struct lookup_subtable *sub;
    PyObject *ret;

    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL || sub->kc==NULL )
	ret = Py_False;
    else
	ret = Py_True;
    Py_INCREF(ret);
return( ret );
}

static PyObject *PyFFFont_isVerticalKerning(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *subtable;
    struct lookup_subtable *sub;
    PyObject *ret;

    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL || !sub->vertical_kerning )
	ret = Py_False;
    else
	ret = Py_True;
    Py_INCREF(ret);
return( ret );
}

static PyObject *PyFFFont_removeLookup(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *lookup;
    OTLookup *otl;

    if ( !PyArg_ParseTuple(args,"s", &lookup ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s exists", lookup );
return( NULL );
    }
    SFRemoveLookup(sf,otl);
Py_RETURN( self );
}

static PyObject *PyFFFont_mergeLookups(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *lookup1, *lookup2;
    OTLookup *otl1, *otl2;
    struct lookup_subtable *sub;

    if ( !PyArg_ParseTuple(args,"ss", &lookup1, &lookup2 ))
return( NULL );

    otl1 = SFFindLookup(sf,lookup1);
    if ( otl1==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s exists", lookup1 );
return( NULL );
    }
    otl2 = SFFindLookup(sf,lookup2);
    if ( otl2==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s exists", lookup2 );
return( NULL );
    }
    if ( otl1->lookup_type != otl2->lookup_type ) {
	PyErr_Format(PyExc_EnvironmentError, "When merging two lookups they must be of the same type, but %s and %s are not", lookup1, lookup2);
return( NULL );
    }
    FLMerge(otl1,otl2);

    for ( sub = otl2->subtables; sub!=NULL; sub=sub->next )
	sub->lookup = otl1;
    if ( otl1->subtables==NULL )
	otl1->subtables = otl2->subtables;
    else {
	for ( sub=otl1->subtables; sub->next!=NULL; sub=sub->next );
	sub->next = otl2->subtables;
    }
    otl2->subtables = NULL;
    SFRemoveLookup(sf,otl2);
Py_RETURN( self );
}

static PyObject *PyFFFont_removeLookupSubtable(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *subtable;
    struct lookup_subtable *sub;

    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s exists", subtable );
return( NULL );
    }
    SFRemoveLookupSubTable(sf,sub);
Py_RETURN( self );
}

static PyObject *PyFFFont_mergeLookupSubtables(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *subtable1, *subtable2;
    struct lookup_subtable *sub1, *sub2;

    if ( !PyArg_ParseTuple(args,"ss", &subtable1, &subtable2 ))
return( NULL );

    sub1 = SFFindLookupSubtable(sf,subtable1);
    if ( sub1==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s exists", subtable1 );
return( NULL );
    }
    sub2 = SFFindLookupSubtable(sf,subtable2);
    if ( sub2==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No subtable named %s exists", subtable2 );
return( NULL );
    }
    if ( sub1->lookup!=sub2->lookup ) {
	PyErr_Format(PyExc_EnvironmentError, "When merging two lookup subtables they must be in the same lookup, but %s and %s are not", subtable1, subtable2);
return( NULL );
    }
    SFSubTablesMerge(sf,sub1,sub2);
    SFRemoveLookupSubTable(sf,sub2);
Py_RETURN( self );
}

static struct flaglist lookup_types[] = {
    { "gsub_single", gsub_single },
    { "gsub_multiple", gsub_multiple },
    { "gsub_alternate", gsub_alternate },
    { "gsub_ligature", gsub_ligature },
    { "gsub_context", gsub_context },
    { "gsub_contextchain", gsub_contextchain },
    { "gsub_reversecchain", gsub_reversecchain },
    { "morx_indic", morx_indic },
    { "morx_context", morx_context },
    { "morx_insert", morx_insert },
    { "gpos_single", gpos_single },
    { "gpos_pair", gpos_pair },
    { "gpos_cursive", gpos_cursive },
    { "gpos_mark2base", gpos_mark2base },
    { "gpos_mark2ligature", gpos_mark2ligature },
    { "gpos_mark2mark", gpos_mark2mark },
    { "gpos_context", gpos_context },
    { "gpos_contextchain", gpos_contextchain },
    { "kern_statemachine", kern_statemachine },
    NULL };
static struct flaglist lookup_flags[] = {
    { "right_to_left", pst_r2l },
    { "ignore_bases", pst_ignorebaseglyphs },
    { "ignore_ligatures", pst_ignoreligatures },
    { "ignore_marks", pst_ignorecombiningmarks },
    { "right_2_left", pst_r2l },
    { "right2left", pst_r2l },
    NULL };

static int ParseLookupFlagsItem(SplineFont *sf,PyObject *flagstr) {
    char *str = PyString_AsString(flagstr);
    int i;

    if ( str==NULL )
return( -1 );
    for ( i=0; lookup_flags[i].name!=NULL; ++i )
	if ( strcmp(lookup_flags[i].name,str)==0 )
return( lookup_flags[i].flag );

    for ( i=1; i<sf->mark_class_cnt; ++i )
	if ( strcmp(sf->mark_class_names[i],str)==0 )
return( i<<8 );

    PyErr_Format(PyExc_ValueError, "Unknown lookup flag %s", str );
return( -1 );
}

static int ParseLookupFlags(SplineFont *sf,PyObject *flagtuple) {
    int i, flags=0, cnt, temp;

    if ( PyString_Check(flagtuple))
return( ParseLookupFlagsItem(sf,flagtuple));
    cnt = PySequence_Size(flagtuple);
    if ( cnt==-1 )
return( -1 );
    for ( i=0; i<cnt; ++i ) {
	temp = ParseLookupFlagsItem(sf,PySequence_GetItem(flagtuple,i));
	if ( temp==-1 )
return( -1 );
	flags |= temp;
    }
return( flags );
}

static FeatureScriptLangList *PyParseFeatureList(PyObject *tuple) {
    FeatureScriptLangList *flhead=NULL, *fltail, *fl;
    struct scriptlanglist *sltail, *sl;
    int f,s,l, cnt;
    int wasmac;
    PyObject *scripts, *langs;

    if ( !PySequence_Check(tuple)) {
	PyErr_Format(PyExc_TypeError, "A feature list is composed of a tuple of tuples" );
return( (FeatureScriptLangList *) -1 );
    }
    cnt = PySequence_Size(tuple);

    for ( f=0; f<cnt; ++f ) {
	PyObject *subs = PySequence_GetItem(tuple,f);
	if ( !PySequence_Check(subs)) {
	    PyErr_Format(PyExc_TypeError, "A feature list is composed of a tuple of tuples" );
return( (FeatureScriptLangList *) -1 );
	} else if ( PySequence_Size(subs)!=2 ) {
	    PyErr_Format(PyExc_TypeError, "A feature list is composed of a tuple of tuples each containing two elements");
return( (FeatureScriptLangList *) -1 );
	} else if ( !PyString_Check(PySequence_GetItem(subs,0)) ||
		!PySequence_Check(PySequence_GetItem(subs,1))) {
	    PyErr_Format(PyExc_TypeError, "Bad type for argument");
return( (FeatureScriptLangList *) -1 );
	}
	fl = chunkalloc(sizeof(FeatureScriptLangList));
	fl->featuretag = StrToTag(PyString_AsString(PySequence_GetItem(subs,0)),&wasmac);
	if ( fl->featuretag == 0xffffffff )
return( (FeatureScriptLangList *) -1 );
	fl->ismac = wasmac;
	if ( flhead==NULL )
	    flhead = fl;
	else
	    fltail->next = fl;
	fltail = fl;
	scripts = PySequence_GetItem(subs,1);
	if ( !PySequence_Check(scripts)) {
	    PyErr_Format(PyExc_TypeError, "A script list is composed of a tuple of tuples" );
return( (FeatureScriptLangList *) -1 );
	} else if ( PySequence_Size(scripts)==0 ) {
	    PyErr_Format(PyExc_TypeError, "No scripts specified for feature %s", PyString_AsString(PySequence_GetItem(subs,0)));
return( (FeatureScriptLangList *) -1 );
	}
	sltail = NULL;
	for ( s=0; s<PySequence_Size(scripts); ++s ) {
	    PyObject *subs = PySequence_GetItem(tuple,s);
	    if ( !PySequence_Check(subs)) {
		PyErr_Format(PyExc_TypeError, "A script list is composed of a tuple of tuples" );
return( (FeatureScriptLangList *) -1 );
	    } else if ( PySequence_Size(subs)!=2 ) {
		PyErr_Format(PyExc_TypeError, "A script list is composed of a tuple of tuples each containing two elements");
return( (FeatureScriptLangList *) -1 );
	    } else if ( !PyString_Check(PySequence_GetItem(subs,0)) ||
		    !PySequence_Check(PySequence_GetItem(subs,1))) {
		PyErr_Format(PyExc_TypeError, "Bad type for argument");
return( (FeatureScriptLangList *) -1 );
	    }
	    sl = chunkalloc(sizeof(struct scriptlanglist));
	    sl->script = StrToTag(PyString_AsString(PySequence_GetItem(subs,0)),NULL);
	    if ( sl->script==0xffffffff )
return( (FeatureScriptLangList *) -1 );
	    if ( sltail==NULL )
		fl->scripts = sl;
	    else
		sltail->next = sl;
	    sltail = sl;
	    langs = PySequence_GetItem(subs,1);
	    if ( !PySequence_Check(langs)) {
		PyErr_Format(PyExc_TypeError, "A language list is composed of a tuple of strings" );
return( (FeatureScriptLangList *) -1 );
	    } else if ( PySequence_Size(langs)==0 ) {
		sl->lang_cnt = 1;
		sl->langs[0] = DEFAULT_LANG;
	    } else {
		sl->lang_cnt = PySequence_Size(langs);
		if ( sl->lang_cnt>MAX_LANG )
		    sl->morelangs = galloc((sl->lang_cnt-MAX_LANG)*sizeof(uint32));
		for ( l=0; l<sl->lang_cnt; ++l ) {
		    uint32 lang = StrToTag(PyString_AsString(PySequence_GetItem(langs,l)),NULL);
		    if ( lang==0xffffffff )
return( (FeatureScriptLangList *) -1 );
		    if ( l<MAX_LANG )
			sl->langs[l] = lang;
		    else
			sl->morelangs[l-MAX_LANG] = lang;
		}
	    }
	}
    }
return( flhead );
}

static PyObject *PyFFFont_addLookup(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    OTLookup *otl, *after = NULL;
    int itype;
    char *lookup_str, *type, *after_str=NULL;
    PyObject *flagtuple, *featlist;
    int flags;
    FeatureScriptLangList *fl;

    if ( !PyArg_ParseTuple(args,"ssOO|s", &lookup_str, &type, &flags, &featlist, &after_str ))
return( NULL );

    otl = SFFindLookup(sf,lookup_str);
    if ( otl!=NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "A lookup named %s already exists", lookup_str );
return( NULL );
    }
    if ( after_str!=NULL ) {
	after = SFFindLookup(sf,after_str);
	if ( after!=NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", after_str );
return( NULL );
	}
    }

    itype = FlagsFromString(type,lookup_types);
    if ( itype==0x8000000 )
return( NULL );

    flags = ParseLookupFlags(sf,flagtuple);
    if ( flags==-1 )
return( NULL );

    fl = PyParseFeatureList(featlist);
    if ( fl==(FeatureScriptLangList *) -1 )
return( NULL );

    if ( after!=NULL && (after->lookup_type>=gpos_start)!=(itype>=gpos_start) ) {
	PyErr_Format(PyExc_EnvironmentError, "After lookup, %s, is in a different table", after_str );
return( NULL );
    }

    if ( sf->cidmaster ) sf = sf->cidmaster;

    otl = chunkalloc(sizeof(OTLookup));
    if ( after!=NULL ) {
	otl->next = after->next;
	after->next = otl;
    } else if ( itype>=gpos_start ) {
	otl->next = sf->gpos_lookups;
	sf->gpos_lookups = otl;
    } else {
	otl->next = sf->gsub_lookups;
	sf->gsub_lookups = otl;
    }
    otl->lookup_type = itype;
    otl->lookup_flags = flags;
    otl->lookup_name = copy(lookup_str);
    otl->features = fl;
    if ( fl!=NULL && (fl->featuretag==CHR('l','i','g','a') || fl->featuretag==CHR('r','l','i','g')))
	otl->store_in_afm = true;
Py_RETURN( self );
}

static PyObject *PyFFFont_lookupSetFeatureList(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    OTLookup *otl;
    char *lookup;
    PyObject *featlist;
    FeatureScriptLangList *fl;

    if ( !PyArg_ParseTuple(args,"sO", &lookup, &featlist ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }

    fl = PyParseFeatureList(featlist);
    if ( fl==(FeatureScriptLangList *) -1 )
return( NULL );

    FeatureScriptLangListFree(otl->features);
    otl->features = fl;
Py_RETURN( self );
}

static PyObject *PyFFFont_lookupSetFlags(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    OTLookup *otl;
    char *lookup;
    PyObject *flagtuple;
    int flags;

    if ( !PyArg_ParseTuple(args,"sO", &lookup, &flagtuple ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }

    flags = ParseLookupFlags(sf,flagtuple);
    if ( flags==-1 )
return( NULL );

    otl->lookup_flags = flags;
Py_RETURN( self );
}

static PyObject *PyFFFont_lookupSetStoreLigatureInAfm(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    OTLookup *otl;
    char *lookup;
    int store_it;

    if ( !PyArg_ParseTuple(args,"si", &lookup, &store_it ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }
    otl->store_in_afm = store_it;
Py_RETURN( self );
}

static PyObject *PyFFFont_getLookupInfo(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    OTLookup *otl;
    char *lookup, *type;
    int i, cnt;
    PyObject *flags_tuple;
    FeatureScriptLangList *fl;
    struct scriptlanglist *sl;
    int fcnt, scnt, l;
    PyObject *farray, *sarray, *larray;

    if ( !PyArg_ParseTuple(args,"s", &lookup ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }

    for ( i=0; lookup_types[i].name!=NULL ; ++i )
	if ( lookup_types[i].flag == otl->lookup_type )
    break;
    type = lookup_types[i].name;

    cnt = ( otl->lookup_flags&0xff00 )!=0;
    for ( i=0; i<4; ++i )
	if ( otl->lookup_flags&(1<<i) )
	    ++cnt;
    flags_tuple = PyTuple_New(cnt);
    cnt = 0;
    if ( otl->lookup_flags&0xff00 )
	PyTuple_SetItem(flags_tuple,cnt++,Py_BuildValue("s",sf->mark_class_names[ (otl->lookup_flags&0xff00)>>8 ]));
    for ( i=0; i<4; ++i )
	if ( otl->lookup_flags&(1<<i) )
	    PyTuple_SetItem(flags_tuple,cnt++,Py_BuildValue("s",lookup_flags[i].name));

    for ( fl=otl->features, fcnt=0; fl!=NULL; fl=fl->next, ++fcnt );
    farray = PyTuple_New(fcnt);
    for ( fl=otl->features, fcnt=0; fl!=NULL; fl=fl->next, ++fcnt ) {
	for ( sl=fl->scripts, scnt=0; sl!=NULL; sl=sl->next, ++scnt );
	sarray = PyTuple_New(scnt);
	for ( sl=fl->scripts, scnt=0; sl!=NULL; sl=sl->next, ++scnt ) {
	    larray = PyTuple_New(sl->lang_cnt);
	    for ( l=0; l<sl->lang_cnt; ++l )
		PyTuple_SetItem(larray,l,TagToPyString(l<MAX_LANG?sl->langs[l]:sl->morelangs[l-MAX_LANG],false));
	    PyTuple_SetItem(sarray,scnt,Py_BuildValue("(sO)",
		    TagToPyString(sl->script,false),larray));
	}
	PyTuple_SetItem(farray,fcnt,Py_BuildValue("(sO)",
		TagToPyString(fl->featuretag,fl->ismac),sarray));
    }
return( Py_BuildValue("(sOO)",type,flags_tuple,farray) );
}

static PyObject *PyFFFont_addLookupSubtable(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *lookup, *subtable, *after_str=NULL;

    if ( !PyArg_ParseTuple(args,"ss|s", &lookup, &subtable, &after_str ))
return( NULL );

    if ( addLookupSubtable(sf, lookup, subtable, after_str)==NULL )
return( NULL );

Py_RETURN( self );
}

static PyObject *PyFFFont_getLookupSubtables(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *lookup;
    OTLookup *otl;
    struct lookup_subtable *sub;
    int cnt;
    PyObject *tuple;

    if ( !PyArg_ParseTuple(args,"s", &lookup ))
return( NULL );

    otl = SFFindLookup(sf,lookup);
    if ( otl==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup );
return( NULL );
    }
    for ( sub = otl->subtables, cnt=0; sub!=NULL; sub=sub->next, ++cnt );
    tuple = PyTuple_New(cnt);
    for ( sub = otl->subtables, cnt=0; sub!=NULL; sub=sub->next, ++cnt )
	PyTuple_SetItem(tuple,cnt,Py_BuildValue("s",sub->subtable_name));
return( tuple );
}

static PyObject *PyFFFont_getLookupSubtableAnchorClasses(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *subtable;
    struct lookup_subtable *sub;
    AnchorClass *ac;
    int cnt;
    PyObject *tuple;

    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup subtable named %s", subtable );
return( NULL );
    }
    for ( ac = sf->anchor, cnt=0; ac!=NULL; ac=ac->next )
	if ( ac->subtable == sub )
	    ++cnt;
    tuple = PyTuple_New(cnt);
    for ( ac = sf->anchor, cnt=0; ac!=NULL; ac=ac->next )
	if ( ac->subtable == sub )
	    PyTuple_SetItem(tuple,cnt++,Py_BuildValue("s",ac->name));
return( tuple );
}

static PyObject *PyFFFont_getLookupOfSubtable(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *subtable;
    struct lookup_subtable *sub;

    if ( !PyArg_ParseTuple(args,"s", &subtable ))
return( NULL );

    sub = SFFindLookupSubtable(sf,subtable);
    if ( sub==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No lookup subtable named %s", subtable );
return( NULL );
    }
return( Py_BuildValue("s", sub->lookup->lookup_name ));
}

static PyObject *PyFFFont_getSubtableOfAnchor(PyObject *self, PyObject *args) {
    SplineFont *sf = ((PyFF_Font *) self)->fv->sf;
    char *anchorclass;
    AnchorClass *ac;

    if ( !PyArg_ParseTuple(args,"s", &anchorclass ))
return( NULL );

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next )
	if ( strcmp(ac->name,anchorclass)==0 )
return( Py_BuildValue("s", ac->subtable->subtable_name ));

    PyErr_Format(PyExc_EnvironmentError, "No anchor class named %s", anchorclass );
return( NULL );
}

static PyObject *PyFFFont_Save(PyObject *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL, *pt;
    FontView *fv = ((PyFF_Font *) self)->fv;
    int s2d=false;

    if ( PySequence_Size(args)==1 ) {
	if ( !PyArg_ParseTuple(args,"es","UTF-8",&filename) )
return( NULL );
	locfilename = utf82def_copy(filename);
	free(filename);
#ifdef VMS
	pt = strrchr(locfilename,'_');
	if ( pt!=NULL && strmatch(pt,"_sfdir")==0 )
	    s2d = true;
#else
	pt = strrchr(locfilename,'.');
	if ( pt!=NULL && strmatch(pt,".sfdir")==0 )
	    s2d = true;
#endif
	if ( !SFDWrite(locfilename,fv->sf,fv->map,fv->normal,s2d))
	    PyErr_Format(PyExc_EnvironmentError, "Save As failed");
	/* Hmmm. We don't set the filename, nor the save_to_dir bit */
	free(locfilename);
    } else {
	if ( fv->sf->filename==NULL )
	    PyErr_Format(PyExc_TypeError, "This font has no associated sfd file yet, you must specify a filename" );
	if ( !SFDWriteBak(fv->sf,fv->map,fv->normal) )
	    PyErr_Format(PyExc_EnvironmentError, "Save failed");
    }
Py_RETURN( self );
}

/* filename, bitmaptype,flags,resolution,mult-sfd-file,namelist */
static char *gen_keywords[] = { "bitmap_type", "flags", "bitmap_resolution",
	"subfont_directory", "namelist", NULL };
struct flaglist gen_flags[] = {
    { "afm", 0x0001 },
    { "pfm", 0x0002 },
    { "short-post", 0x0004 },
    { "omit-instructions", 0x0008 },
    { "apple", 0x0010 },
    { "opentype", 0x0080 },
    { "glyph-comments", 0x0020 },
    { "glyph-colors", 0x0040 },
    { "glyph-map-file", 0x0100 },
    { "TeX-table", 0x0200 },
    { "ofm", 0x0400 },
    { "old-kern", 0x0800 },
    { "broken-size", 0x1000 },
    { "tfm", 0x10000 },
    { "no-flex", 0x40000 },
    { "no-hints", 0x80000 },
    { "round", 0x200000 },
    { "composites-in-afm", 0x400000 },
    NULL
};

static PyObject *PyFFFont_Generate(PyObject *self, PyObject *args, PyObject *keywds) {
    char *filename;
    char *locfilename = NULL;
    FontView *fv = ((PyFF_Font *) self)->fv;
    PyObject *flags=NULL;
    int iflags = -1;
    int resolution = -1;
    char *bitmaptype="", *subfontdirectory=NULL, *namelist=NULL;
    NameList *rename_to = NULL;

    if ( !PyArg_ParseTupleAndKeywords(args, keywds, "es|sOiss", gen_keywords,
	    "UTF-8",&filename, &bitmaptype, &flags, &resolution, &subfontdirectory, &namelist) )
return( NULL );
    if ( flags!=NULL ) {
	iflags = FlagsFromTuple(flags,gen_flags);
	if ( iflags==0x80000000 ) {
	    PyErr_Format(PyExc_TypeError, "Unknown flag");
return( NULL );
	}
	/* Legacy screw ups mean that opentype & apple bits don't mean what */
	/*  I want them to. Python users should not see that, but fix it up */
	/*  here */
	if ( (iflags&0x80) && (iflags&0x10) )	/* Both */
	    iflags &= ~0x10;
	else if ( (iflags&0x80) && !(iflags&0x10)) /* Just opentype */
	    iflags &= ~0x80;
	else if ( !(iflags&0x80) && (iflags&0x10)) /* Just apple */
	    /* This one's set already */;
	else
	    iflags |= 0x90;
    }
    if ( namelist!=NULL ) {
	rename_to = NameListByName(namelist);
	if ( rename_to==NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "Unknown namelist");
return( NULL );
	}
    }
    locfilename = utf82def_copy(filename);
    free(filename);
    if ( !GenerateScript(fv->sf,locfilename,bitmaptype,iflags,resolution,subfontdirectory,
	    NULL,fv->normal==NULL?fv->map:fv->normal,rename_to) ) {
	PyErr_Format(PyExc_EnvironmentError, "Font generation failed");
return( NULL );
    }
    free(locfilename);
Py_RETURN( self );
}

static PyObject *PyFFFont_GenerateFeature(PyObject *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    char *lookup_name = NULL;
    FontView *fv = ((PyFF_Font *) self)->fv;
    FILE *out;
    OTLookup *otl = NULL;
    int err;

    if ( !PyArg_ParseTuple(args,"es|s","UTF-8",&filename,&lookup_name) )
return( NULL );
    locfilename = utf82def_copy(filename);
    free(filename);

    if ( lookup_name!=NULL ) {
	otl = SFFindLookup(fv->sf,lookup_name);
	if ( otl == NULL ) {
	    PyErr_Format(PyExc_EnvironmentError, "No lookup named %s", lookup_name );
return( NULL );
	}
    }
    out = fopen(locfilename,"w");
    if ( out==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "Failed to open file, %s, for writing", locfilename );
return( NULL );
    }
    if ( otl!=NULL )
	FeatDumpOneLookup(out,fv->sf,otl);
    else
	FeatDumpFontLookups(out,fv->sf);
    err = ferror(out);
    if ( fclose(out)!=0 || err ) {
	PyErr_Format(PyExc_EnvironmentError, "IO error on file %s", locfilename);
return( NULL );
    }
    free(locfilename);
Py_RETURN( self );
}

static PyObject *PyFFFont_MergeKern(PyObject *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    FontView *fv = ((PyFF_Font *) self)->fv;

    if ( !PyArg_ParseTuple(args,"es","UTF-8",&filename) )
return( NULL );
    locfilename = utf82def_copy(filename);
    free(filename);
    if ( !LoadKerningDataFromMetricsFile(fv->sf,locfilename,fv->map)) {
	PyErr_Format(PyExc_EnvironmentError, "No metrics data found");
return( NULL );
    }
    free(locfilename);
Py_RETURN( self );
}

static PyObject *PyFFFont_MergeFonts(PyObject *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    FontView *fv = ((PyFF_Font *) self)->fv;
    SplineFont *sf;
    int openflags=0;

    if ( !PyArg_ParseTuple(args,"es|i","UTF-8",&filename, &openflags) )
return( NULL );
    locfilename = utf82def_copy(filename);
    free(filename);
    sf = LoadSplineFont(locfilename,openflags);
    free(locfilename);
    if ( sf==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No font found in file");
return( NULL );
    }
    if ( sf->fv==NULL )
	EncMapFree(sf->map);
    MergeFont(fv,sf);
Py_RETURN( self );
}

static PyObject *PyFFFont_InterpolateFonts(PyObject *self, PyObject *args) {
    char *filename;
    char *locfilename = NULL;
    FontView *fv = ((PyFF_Font *) self)->fv, *newfv;
    SplineFont *sf;
    int openflags=0;
    double fraction;

    if ( !PyArg_ParseTuple(args,"des|i",&fraction,"UTF-8",&filename, &openflags) )
return( NULL );
    locfilename = utf82def_copy(filename);
    free(filename);
    sf = LoadSplineFont(locfilename,openflags);
    free(locfilename);
    if ( sf==NULL ) {
	PyErr_Format(PyExc_EnvironmentError, "No font found in file");
return( NULL );
    }
    if ( sf->fv==NULL )
	EncMapFree(sf->map);
    newfv = SFAdd(InterpolateFont(fv->sf,sf,fraction, fv->map->enc ));
return( PyFV_From_FV_I(newfv));
}

static PyObject *PyFFFont_CreateMappedChar(PyObject *self, PyObject *args) {
    int enc;
    char *str;
    FontView *fv = ((PyFF_Font *) self)->fv;
    SplineChar *sc;

    if ( !PyArg_ParseTuple(args,"s", &str ) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"i", &enc ) )
return( NULL );
	if ( enc<0 || enc>fv->map->enccount ) {
	    PyErr_Format(PyExc_ValueError, "Encoding is out of range" );
return( NULL );
	}
    } else {
	enc = SFFindSlot(fv->sf, fv->map, -1, str );
	if ( enc==-1 ) {
	    PyErr_Format(PyExc_ValueError, "Glyph name, %s, not in current encoding", str );
return( NULL );
	}
    }
    sc = SFMakeChar(fv->sf,fv->map,enc);
return( PySC_From_SC_I( sc ));
}

static PyObject *PyFFFont_CreateUnicodeChar(PyObject *self, PyObject *args) {
    int uni, enc;
    char *name=NULL;
    FontView *fv = ((PyFF_Font *) self)->fv;
    SplineChar *sc;

    if ( !PyArg_ParseTuple(args,"i|s", &uni, &name ) )
return( NULL );
    if ( uni<-1 || uni>=unicode4_size ) {
	PyErr_Format(PyExc_ValueError, "Unicode codepoint, %d, out of range, must be either -1 or between 0 and 0x10ffff", uni );
return( NULL );
    } else if ( uni==-1 && name==NULL ) {
	PyErr_Format(PyExc_ValueError, "If you do not specify a code point, you must specify a name.");
return( NULL );
    }

    enc = SFFindSlot(fv->sf, fv->map, uni, name );
    if ( enc!=-1 ) {
	sc = SFMakeChar(fv->sf,fv->map,enc);
	if ( name!=NULL ) {
	    free(sc->name);
	    sc->name = copy(name);
	}
    } else {
	sc = SFGetOrMakeChar(fv->sf,uni,name);
	/* does not add to current map. But since we didn't find a slot it's */
	/*  not in the encoding. We could add it in the unencoded area */
    }
return( PySC_From_SC_I( sc ));
}

static PyObject *PyFFFont_findEncodingSlot(PyObject *self, PyObject *args) {
    int uni= -1;
    char *name=NULL;
    FontView *fv = ((PyFF_Font *) self)->fv;

    if ( !PyArg_ParseTuple(args,"s", &name ) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"i", &uni ) )
return( NULL );
    }
    if ( uni<-1 || uni>=unicode4_size ) {
	PyErr_Format(PyExc_ValueError, "Unicode codepoint, %d, out of range, must be either -1 or between 0 and 0x10ffff", uni );
return( NULL );
    }

return( Py_BuildValue("i",SFFindSlot(fv->sf, fv->map, uni, name )) );
}

static PyObject *PyFFFont_removeGlyph(PyObject *self, PyObject *args) {
    int uni, enc;
    char *name=NULL;
    FontView *fv = ((PyFF_Font *) self)->fv;
    SplineChar *sc;
    int flags = 0;	/* Currently unused */

    if ( PyTuple_Size(args)==1 && PyType_IsSubtype(&PyFF_GlyphType,PyTuple_GetItem(args,0)->ob_type)) {
	sc = ((PyFF_Glyph *) PyTuple_GetItem(args,0))->sc;
	if ( sc->parent!=fv->sf ) {
	    PyErr_Format(PyExc_ValueError, "This glyph is not in the font");
return( NULL );
	}
    } else {
	if ( !PyArg_ParseTuple(args,"i|s", &enc, &name ) )
return( NULL );
	if ( uni<-1 || uni>=unicode4_size ) {
	    PyErr_Format(PyExc_ValueError, "Unicode codepoint, %d, out of range, must be either -1 or between 0 and 0x10ffff", uni );
return( NULL );
	} else if ( uni==-1 && name==NULL ) {
	    PyErr_Format(PyExc_ValueError, "If you do not specify a code point, you must specify a name.");
return( NULL );
	}
	sc = SFGetChar(fv->sf,uni,name);
	if ( sc==NULL ) {
	    PyErr_Format(PyExc_ValueError, "This glyph is not in the font");
return( NULL );
	}
    }
    SFRemoveGlyph(fv->sf,sc,&flags);
Py_RETURN( self );
}

static struct flaglist printflags[] = {
    { "fontdisplay", 0 },
    { "chars", 1 },
    { "waterfall", 2 },
    { "fontsample", 3 },
    { "fontsampleinfile", 4 },
    { "multisize", 2 },
    NULL };

static PyObject *PyFFFont_print(PyObject *self, PyObject *args) {
    int type, i, inlinesample = true;
    int32 *pointsizes=NULL;
    char *samplefile=NULL, *output=NULL;
    unichar_t *sample=NULL;
    char *locfilename=NULL;
    int arg_cnt;
    PyObject *arg;
    char *ptype;

    arg_cnt = PyTuple_Size(args);
    if ( arg_cnt<1 || arg_cnt>4 ) {
	PyErr_Format(PyExc_ValueError, "Wrong number of arguments" );
return( NULL );
    }

    ptype = PyString_AsString(PyTuple_GetItem(args,0));
    if ( ptype==NULL )
return( NULL );
    type = FlagsFromString(ptype,printflags);
    if ( type==0x80000000 ) {
	PyErr_Format(PyExc_TypeError, "Unknown printing type" );
return( NULL );
    }
    if ( type==4 ) {
	type=3;
	inlinesample = false;
    }
    if ( arg_cnt>1 ) {
	arg = PyTuple_GetItem(args,1);
	if ( PyInt_Check(arg)) {
	    int val = PyInt_AsLong(arg);
	    if ( val>0 ) { 
		pointsizes = gcalloc(2,sizeof(int32));
		pointsizes[0] = val;
	    }
	} else if ( PySequence_Check(arg) ) {
	    int subcnt = PySequence_Size(arg);
	    pointsizes = galloc((subcnt+1)*sizeof(int32));
	    for ( i=0; i<subcnt; ++i ) {
		pointsizes[i] = PyInt_AsLong(PySequence_GetItem(arg,i));
		if ( PyErr_Occurred())
return( NULL );
	    }
	    pointsizes[i] = 0;
	} else {
	    PyErr_Format(PyExc_TypeError, "Unexpect type for pointsize" );
return( NULL );
	}
    }
    if ( arg_cnt>2 ) {
	char *str = PyString_AsString(PyTuple_GetItem(args,2));
	if ( str==NULL )
return( NULL );
	if ( inlinesample ) {
	    sample = utf82u_copy(str);
	    samplefile = NULL;
	} else {
	    samplefile = locfilename = utf82def_copy(str);
	    sample = NULL;
	}
    }
    if ( arg_cnt>3 ) {
	output = PyString_AsString(PyTuple_GetItem(args,3));
	if ( output==NULL )
return( NULL );
    }
    ScriptPrint(((PyFF_Font *) self)->fv,type,pointsizes,samplefile,sample,output);
    free(pointsizes);
    free(locfilename);
    /* ScriptPrint frees sample for us */
Py_RETURN(self);
}

static PyObject *PyFFFont_clear(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    FVFakeMenus(fv,5);
Py_RETURN(self);
}

static PyObject *PyFFFont_cut(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    FVFakeMenus(fv,0);
Py_RETURN(self);
}

static PyObject *PyFFFont_copy(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    FVFakeMenus(fv,1);
Py_RETURN(self);
}

static PyObject *PyFFFont_copyReference(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    FVFakeMenus(fv,2);
Py_RETURN(self);
}

static PyObject *PyFFFont_paste(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    FVFakeMenus(fv,4);
Py_RETURN(self);
}

static PyObject *PyFFFont_pasteInto(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    FVFakeMenus(fv,9);
Py_RETURN(self);
}

static PyObject *PyFFFont_unlinkReferences(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    FVFakeMenus(fv,8);
Py_RETURN(self);
}


static PyObject *PyFFFont_Build(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;

    FVBuildAccent(fv,false);

Py_RETURN( self );
}

static PyObject *PyFFFont_canonicalContours(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;
    int i,gid;

    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && fv->selected[i] )
	CanonicalContours(sf->glyphs[gid]);

Py_RETURN( self );
}

static PyObject *PyFFFont_canonicalStart(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    EncMap *map = fv->map;
    SplineFont *sf = fv->sf;
    int i,gid;

    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && fv->selected[i] )
	SPLsStartToLeftmost(sf->glyphs[gid]);

Py_RETURN( self );
}

static PyObject *PyFFFont_autoHint(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;

    FVFakeMenus(fv,200);
Py_RETURN( self );
}

static PyObject *PyFFFont_autoInstr(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;

    FVFakeMenus(fv,202);
Py_RETURN( self );
}

static PyObject *PyFFFont_autoTrace(PyObject *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;

    FVAutoTrace(fv,false);
Py_RETURN( self );
}

static PyObject *PyFFFont_Transform(PyFF_Layer *self, PyObject *args) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    int i;
    double m[6];
    real t[6];
    BVTFunc bvts[1];

    if ( !PyArg_ParseTuple(args,"(dddddd)",&m[0], &m[1], &m[2], &m[3], &m[4], &m[5] ) )
return( NULL );
    for ( i=0; i<6; ++i )
	t[i] = m[i];
    bvts[0].func = bvt_none;
    FVTransFunc(fv,t,0,bvts,true);
Py_RETURN( self );
}

static PyObject *PyFFFont_Simplify(PyFF_Font *self, PyObject *args) {
    static struct simplifyinfo smpl = { sf_normal,.75,.2,10 };
    FontView *fv = self->fv;

    smpl.err = (fv->sf->ascent+fv->sf->descent)/1000.;
    smpl.linefixup = (fv->sf->ascent+fv->sf->descent)/500.;
    smpl.linelenmax = (fv->sf->ascent+fv->sf->descent)/100.;

    if ( PySequence_Size(args)>=1 )
	smpl.err = PyFloat_AsDouble(PySequence_GetItem(args,0));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=2 )
	smpl.flags = FlagsFromTuple( PySequence_GetItem(args,1),simplifyflags);
    if ( !PyErr_Occurred() && PySequence_Size(args)>=3 )
	smpl.tan_bounds = PyFloat_AsDouble( PySequence_GetItem(args,2));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=4 )
	smpl.linefixup = PyFloat_AsDouble( PySequence_GetItem(args,3));
    if ( !PyErr_Occurred() && PySequence_Size(args)>=5 )
	smpl.linelenmax = PyFloat_AsDouble( PySequence_GetItem(args,4));
    if ( PyErr_Occurred() )
return( NULL );
    _FVSimplify(self->fv,&smpl);
Py_RETURN( self );
}

static PyObject *PyFFFont_Round(PyFF_Font *self, PyObject *args) {
    double factor=1;
    FontView *fv = self->fv;
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    int i, gid;

    if ( !PyArg_ParseTuple(args,"|d",&factor ) )
return( NULL );
    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && fv->selected[i] ) {
	SplineChar *sc = sf->glyphs[gid];
	SCRound2Int( sc,factor);
    }
Py_RETURN( self );
}

static PyObject *PyFFFont_Cluster(PyFF_Font *self, PyObject *args) {
    double within = .1, max = .5;
    int i, gid;
    FontView *fv = self->fv;
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;

    if ( !PyArg_ParseTuple(args,"|dd", &within, &max ) )
return( NULL );

    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL && fv->selected[i] ) {
	SplineChar *sc = sf->glyphs[gid];
	SCRoundToCluster( sc,-2,false,within,max);
    }
Py_RETURN( self );
}

static PyObject *PyFFFont_AddExtrema(PyFF_Font *self, PyObject *args) {
    FontView *fv = self->fv;

    FVFakeMenus(fv,102);
Py_RETURN( self );
}

static PyObject *PyFFFont_Stroke(PyFF_Font *self, PyObject *args) {
    StrokeInfo si;

    if ( Stroke_Parse(&si,args)==-1 )
return( NULL );

    FVStrokeItScript(self->fv, &si);
Py_RETURN( self );
}

static PyObject *PyFFFont_Correct(PyFF_Font *self, PyObject *args) {
    int i, gid;
    FontView *fv = self->fv;
    SplineFont *sf = fv->sf;
    EncMap *map = fv->map;
    int changed, refchanged;
    int checkrefs = true;
    RefChar *ref;
    SplineChar *sc;

    for ( i=0; i<map->enccount; ++i ) if ( (gid=map->map[i])!=-1 && (sc=sf->glyphs[gid])!=NULL && fv->selected[i] ) {
	changed = refchanged = false;
	if ( checkrefs ) {
	    for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
		if ( ref->transform[0]*ref->transform[3]<0 ||
			(ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		    if ( !refchanged ) {
			refchanged = true;
			SCPreserveState(sc,false);
		    }
		    SCRefToSplines(sc,ref);
		}
	    }
	}
	if ( !refchanged )
	    SCPreserveState(sc,false);
	sc->layers[ly_fore].splines = SplineSetsCorrect(sc->layers[ly_fore].splines,&changed);
	if ( changed || refchanged )
	    SCCharChangedUpdate(sc);
    }
Py_RETURN( self );
}

static PyObject *PyFFFont_RemoveOverlap(PyFF_Font *self, PyObject *args) {

    FVFakeMenus(self->fv,100);
Py_RETURN( self );
}

static PyObject *PyFFFont_Intersect(PyFF_Font *self, PyObject *args) {

    FVFakeMenus(self->fv,104);
Py_RETURN( self );
}

static PyObject *PyFFFont_replaceWithReference(PyFF_Font *self, PyObject *args) {
    double fudge = .01;

    if ( !PyArg_ParseTuple(args,"|d",&fudge) )
return( NULL );

    FVReplaceOutlineWithReference(self->fv,fudge);
Py_RETURN( self );
}

static PyObject *PyFFFont_compareGlyphs(PyFF_Font *self, PyObject *args) {
    /* Compare selected glyphs against the contents of the clipboard	*/
    /* Three comparisons:						*/
    /*	1) Compare the points (base & control) of all contours		*/
    /*  2) Compare that the splines themselves don't get too far appart */
    /*  3) Compare bitmaps						*/
    /*  4) Compare hints/hintmasks					*/
    double pt_err = .5, spline_err = 1, bitmaps = -1;
    int bb_err=2, comp_hints=false, report_errors = true;
    int ret;

    if ( !PyArg_ParseTuple(args,"|dddiii",&pt_err,&spline_err,&bitmaps,
	    &bb_err,&comp_hints,&report_errors) )
return( NULL );
    ret = CompareGlyphs(NULL, pt_err, spline_err, bitmaps, bb_err,
		    comp_hints, report_errors );
    if ( ret==-1 )
return( NULL );

return( Py_BuildValue("i", ret ));
}

static PyMethodDef PyFF_Font_methods[] = {
    { "appendSFNTName", PyFFFont_appendSFNTName, METH_VARARGS, "Adds or replaces a name in the sfnt 'name' table. Takes three arguments, a language, a string id, and the string value" },
    { "close", PyFFFont_close, METH_NOARGS, "Frees up memory for the current font. Any python pointers to it will become invalid." },
    { "compareFonts", (PyCFunction) PyFFFont_compareFonts, METH_VARARGS, "Compares two fonts and stores the result into a file"},
    { "save", PyFFFont_Save, METH_VARARGS, "Save the current font to a sfd file" },
    { "generate", (PyCFunction) PyFFFont_Generate, METH_VARARGS | METH_KEYWORDS, "Save the current font to a standard font file" },
    { "generateFeatureFile", (PyCFunction) PyFFFont_GenerateFeature, METH_VARARGS, "Creates an adobe feature file containing all features and lookups" },
    { "mergeKern", PyFFFont_MergeKern, METH_VARARGS, "Merge feature data into the current font from an external file" },
    { "mergeFeature", PyFFFont_MergeKern, METH_VARARGS, "Merge feature data into the current font from an external file" },
    { "mergeFonts", PyFFFont_MergeFonts, METH_VARARGS, "Merge two fonts" },
    { "interpolateFonts", PyFFFont_InterpolateFonts, METH_VARARGS, "Interpolate between two fonts returning a new one" },
    { "createChar", PyFFFont_CreateUnicodeChar, METH_VARARGS, "Creates a (blank) glyph at the specified unicode codepoint" },
    { "createMappedChar", PyFFFont_CreateMappedChar, METH_VARARGS, "Creates a (blank) glyph at the specified encoding" },
    { "getTableData", PyFFFont_GetTableData, METH_VARARGS, "Returns a tuple, one entry per byte (as unsigned integers) of the table"},
    { "setTableData", PyFFFont_SetTableData, METH_VARARGS, "Sets the table to a tuple of bytes"},
    { "addLookup", PyFFFont_addLookup, METH_VARARGS, "Add a new lookup"},
    { "addLookupSubtable", PyFFFont_addLookupSubtable, METH_VARARGS, "Add a new lookup-subtable"},
    { "addAnchorClass", PyFFFont_addAnchorClass, METH_VARARGS, "Add a new anchor class to the subtable"},
    { "addKerningClass", PyFFFont_addKerningClass, METH_VARARGS, "Add a new subtable with a new kerning class to a lookup"},
    { "alterKerningClass", PyFFFont_alterKerningClass, METH_VARARGS, "Changes the existing kerning class in the named subtable"},
    { "findEncodingSlot", PyFFFont_findEncodingSlot, METH_VARARGS, "Returns the encoding of a unicode code point or glyph name if they are in the current encoding. Else returns -1" },
    { "getKerningClass", PyFFFont_getKerningClass, METH_VARARGS, "Returns the contents of the kerning class in the named subtable"},
    { "getLookupInfo", PyFFFont_getLookupInfo, METH_VARARGS, "Get info about the named lookup" },
    { "getLookupSubtables", PyFFFont_getLookupSubtables, METH_VARARGS, "Get a tuple of subtable names in a lookup" },
    { "getLookupSubtableAnchorClasses", PyFFFont_getLookupSubtableAnchorClasses, METH_VARARGS, "Get a tuple of all anchor classes in a subtable" },
    { "getLookupOfSubtable", PyFFFont_getLookupOfSubtable, METH_VARARGS, "Returns the name of the lookup containing this subtable" },
    { "getSubtableOfAnchor", PyFFFont_getSubtableOfAnchor, METH_VARARGS, "Returns the name of the lookup subtable containing this anchor class" },
    { "isKerningClass", PyFFFont_isKerningClass, METH_VARARGS, "Returns whether the named subtable contains a kerning class"},
    { "isVerticalKerning", PyFFFont_isVerticalKerning, METH_VARARGS, "Returns whether the named subtable contains vertical kerning data"},
    { "lookupSetFeatureList", PyFFFont_lookupSetFeatureList, METH_VARARGS, "Sets the feature, script, language list on a lookup" },
    { "lookupSetFlags", PyFFFont_lookupSetFlags, METH_VARARGS, "Sets the lookup flags on a lookup" },
    { "lookupSetStoreLigatureInAfm", PyFFFont_lookupSetStoreLigatureInAfm, METH_VARARGS, "Sets whether this ligature lookup contains data which should live in the afm file"},
    { "mergeLookups", PyFFFont_mergeLookups, METH_VARARGS, "Merges two lookups" },
    { "mergeLookupSubtables", PyFFFont_mergeLookupSubtables, METH_VARARGS, "Merges two lookup subtables" },
    { "print", PyFFFont_print, METH_VARARGS, "Produces a font sample printout" },
    { "regenBitmaps", (PyCFunction) PyFFFont_regenBitmaps, METH_VARARGS, "Rerasterize the bitmap fonts specified in the argument tuple" },
    { "removeAnchorClass", PyFFFont_removeAnchorClass, METH_VARARGS, "Removes the named anchor class" },
    { "removeGlyph", PyFFFont_removeGlyph, METH_VARARGS, "Removes the glyph from the font" },
    { "removeLookup", PyFFFont_removeLookup, METH_VARARGS, "Removes the named lookup" },
    { "removeLookupSubtable", PyFFFont_removeLookupSubtable, METH_VARARGS, "Removes the named lookup subtable" },
/* Selection based */
    { "clear", PyFFFont_clear, METH_NOARGS, "Clears all selected glyphs" },
    { "cut", PyFFFont_cut, METH_NOARGS, "Cuts all selected glyphs" },
    { "copy", PyFFFont_copy, METH_NOARGS, "Copies all selected glyphs" },
    { "copyReference", PyFFFont_copyReference, METH_NOARGS, "Copies all selected glyphs as references" },
    { "paste", PyFFFont_paste, METH_NOARGS, "Pastes the clipboard into the selected glyphs (clearing them first)" },
    { "pasteInto", PyFFFont_pasteInto, METH_NOARGS, "Pastes the clipboard into the selected glyphs (merging with what's there)" },
    { "unlinkReferences", PyFFFont_unlinkReferences, METH_NOARGS, "Unlinks all references in the selected glyphs" },
    { "replaceWithReference", (PyCFunction) PyFFFont_replaceWithReference, METH_VARARGS, "Replaces any inline copies of any of the selected glyphs with a reference" },

    { "addExtrema", (PyCFunction) PyFFFont_AddExtrema, METH_NOARGS, "Add extrema to the contours of the glyph"},
    { "autoHint", PyFFFont_autoHint, METH_NOARGS, "Guess at postscript hints"},
    { "autoInstr", PyFFFont_autoInstr, METH_NOARGS, "Guess (badly) at truetype instructions"},
    { "autoTrace", PyFFFont_autoTrace, METH_NOARGS, "Autotrace any background images"},
    { "build", PyFFFont_Build, METH_NOARGS, "If the current glyph is an accented character\nand all components are in the font\nthen build it out of references" },
    { "canonicalContours", (PyCFunction) PyFFFont_canonicalContours, METH_NOARGS, "Orders the contours in the current glyph by the x coordinate of their leftmost point. (This can reduce the size of the postscript charstring needed to describe the glyph(s)."},
    { "canonicalStart", (PyCFunction) PyFFFont_canonicalStart, METH_NOARGS, "Sets the start point of all the contours of the current glyph to be the leftmost point on the contour."},
    { "cluster", (PyCFunction) PyFFFont_Cluster, METH_VARARGS, "Cluster the points of a glyph towards common values" },
    { "compareGlyphs", (PyCFunction) PyFFFont_compareGlyphs, METH_VARARGS, "Compares two sets of glyphs"},
    { "correctDirection", (PyCFunction) PyFFFont_Correct, METH_NOARGS, "Orient a layer so that external contours are clockwise and internal counter clockwise." },
    { "intersect", (PyCFunction) PyFFFont_Intersect, METH_NOARGS, "Leaves the areas where the contours of a glyph overlap."},
    { "removeOverlap", (PyCFunction) PyFFFont_RemoveOverlap, METH_NOARGS, "Remove overlapping areas from a glyph"},
    { "round", (PyCFunction)PyFFFont_Round, METH_VARARGS, "Rounds point coordinates (and reference translations) to integers"},
    { "simplify", (PyCFunction)PyFFFont_Simplify, METH_VARARGS, "Simplifies a glyph" },
    { "stroke", (PyCFunction)PyFFFont_Stroke, METH_VARARGS, "Strokes the countours in a glyph"},
    { "transform", (PyCFunction)PyFFFont_Transform, METH_VARARGS, "Transform a glyph by a 6 element matrix." },

    NULL
};

/* ************************************************************************** */
/* *********************** Font as glyph dictionary ************************* */
/* ************************************************************************** */
static int PyFF_FontLength( PyObject *self ) {
return( ((PyFF_Font *) self)->fv->map->enccount );
}

static PyObject *PyFF_FontIndex( PyObject *self, PyObject *index ) {
    FontView *fv = ((PyFF_Font *) self)->fv;
    SplineFont *sf = fv->sf;
    SplineChar *sc = NULL;

    if ( PyString_Check(index)) {
	char *name = PyString_AsString(index);
	sc = SFGetChar(sf,-1,name);
    } else if ( PyInt_Check(index)) {
	int pos = PyInt_AsLong(index), gid;
	if ( pos<0 || pos>=fv->map->enccount ) {
	    PyErr_Format(PyExc_TypeError, "Index out of bounds");
return( NULL );
	}
	gid = fv->map->map[pos];
	sc = gid==-1 ? NULL : sf->glyphs[gid];
    } else {
	PyErr_Format(PyExc_TypeError, "Index must be an integer or a string" );
return( NULL );
    }
    if ( sc==NULL ) {
	PyErr_Format(PyExc_TypeError, "No such glyph" );
return( NULL );
    }
return( PySC_From_SC_I(sc));
}

static PyMappingMethods PyFF_FontMapping = {
    PyFF_FontLength,		/* length */
    PyFF_FontIndex,		/* subscript */
    NULL			/* subscript assign */
};
/* ************************************************************************** */
/* ************************* initializer routines *************************** */
/* ************************************************************************** */

static PyTypeObject PyFF_FontType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "fontforge.font",          /*tp_name*/
    sizeof(PyFF_Font), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor) PyFF_Font_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    &PyFF_FontMapping,         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    (reprfunc) PyFFFont_Str,   /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "FontForge Font object",   /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    fontiter_new,              /* tp_iter */
    0,		               /* tp_iternext */
    PyFF_Font_methods,         /* tp_methods */
    0,			       /* tp_members */
    PyFF_Font_getset,          /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    /*(initproc)PyFF_Font_init*/0,  /* tp_init */
    0,                         /* tp_alloc */
    PyFF_Font_new,             /* tp_new */
};

static PyMethodDef FontForge_methods[] = {
    { "getPrefs", PyFF_GetPrefs, METH_VARARGS, "Get FontForge preference items" },
    { "setPrefs", PyFF_SetPrefs, METH_VARARGS, "Set FontForge preference items" },
    { "savePrefs", PyFF_SavePrefs, METH_NOARGS, "Save FontForge preference items" },
    { "loadPrefs", PyFF_LoadPrefs, METH_NOARGS, "Load FontForge preference items" },
    { "defaultOtherSubrs", PyFF_DefaultOtherSubrs, METH_NOARGS, "Use FontForge's default \"othersubrs\" functions for Type1 fonts" },
    { "readOtherSubrsFile", PyFF_ReadOtherSubrsFile, METH_VARARGS, "Read from a file, \"othersubrs\" functions for Type1 fonts" },
    { "loadEncodingFile", PyFF_LoadEncodingFile, METH_VARARGS, "Load an encoding file into the list of encodings" },
    { "loadNamelist", PyFF_LoadNamelist, METH_VARARGS, "Load a namelist into the list of namelists" },
    { "loadNamelistDir", PyFF_LoadNamelistDir, METH_VARARGS, "Load a directory of namelist files into the list of namelists" },
    { "loadPlugin", PyFF_LoadPlugin, METH_VARARGS, "Load a FontForge plugin" },
    { "loadPluginDir", PyFF_LoadPluginDir, METH_VARARGS, "Load a directory of FontForge plugin files" },
    { "preloadCidmap", PyFF_PreloadCidmap, METH_VARARGS, "Load a cidmap file" },
    { "unicodeFromName", PyFF_UnicodeFromName, METH_VARARGS, "Given a name, look it up in the namelists and find what unicode code point it maps to (returns -1 if not found)" },
    { "version", PyFF_Version, METH_NOARGS, "Returns a string containing the current version of FontForge, as 20061116" },
    { "fonts", PyFF_FontTuple, METH_NOARGS, "Returns a tuple of all loaded fonts" },
    { "fontsInFile", PyFF_FontsInFile, METH_VARARGS, "Returns a tuple containing the names of any fonts in an external file"},
    { "open", PyFF_OpenFont, METH_VARARGS, "Opens a font and returns it" },
    { "printSetup", PyFF_printSetup, METH_VARARGS, "Prepare to print a font sample (select default printer or file, page size, etc.)" },
    { "parseTTInstrs", PyFF_ParseTTFInstrs, METH_VARARGS, "Takes a string and parses it into a tuple of truetype instruction bytes"},
    { "unParseTTInstrs", PyFF_UnParseTTFInstrs, METH_VARARGS, "Takes a tuple of truetype instruction bytes and converts to a human readable string"},
    { "activeFontInUI", PyFF_ActiveFont, METH_NOARGS, "If invoked from the UI, this returns the currently active font"},
    {NULL}  /* Sentinel */
};

static PyObject *PyPS_Identity(PyObject *noself, PyObject *args) {
return( Py_BuildValue("(dddddd)",  1.0, 0.0, 0.0,  0.0, 1.0, 0.0));
}

static PyObject *PyPS_Translate(PyObject *noself, PyObject *args) {
    double x,y=0;

    if ( !PyArg_ParseTuple(args,"d|d",&x,&y) ) {
	PyErr_Clear();
	if ( !PyArg_ParseTuple(args,"(dd)",&x,&y) )
return( NULL );
    }

return( Py_BuildValue("(dddddd)",  1.0, 0.0, 0.0, 1.0,  x, y));
}

static PyObject *PyPS_Scale(PyObject *noself, PyObject *args) {
    double x,y=-99999;

    if ( !PyArg_ParseTuple(args,"d|d",&x,&y) )
return( NULL );
    if ( y== -99999 )
	y = x;

return( Py_BuildValue("(dddddd)",  x, 0.0, 0.0, y,  0.0, 0.0));
}

static PyObject *PyPS_Rotate(PyObject *noself, PyObject *args) {
    double theta, c, s;

    if ( !PyArg_ParseTuple(args,"d",&theta) )
return( NULL );
    c = cos(theta); s = sin(theta);

return( Py_BuildValue("(dddddd)",  c, s, -s, c,  0.0, 0.0));
}

static PyObject *PyPS_Skew(PyObject *noself, PyObject *args) {
    double theta, s;

    if ( !PyArg_ParseTuple(args,"d",&theta) )
return( NULL );
    s = sin(theta);

return( Py_BuildValue("(dddddd)",  1.0, s, 0.0, 1.0,  0.0, 0.0));
}

static PyObject *PyPS_Compose(PyObject *noself, PyObject *args) {
    double m1[6], m2[6];
    real r1[6], r2[6], r3[6];
    int i;
    PyObject *tuple;

    if ( !PyArg_ParseTuple(args,"(dddddd)(dddddd)",
	    &m1[0], &m1[1], &m1[2], &m1[3], &m1[4], &m1[5],
	    &m2[0], &m2[1], &m2[2], &m2[3], &m2[4], &m2[5] ))
return( NULL );
    for ( i=0; i<6; ++i ) {
	r1[i] = m1[i]; r2[i] = m2[i];
    }
    MatMultiply(r1,r2,r3);
    tuple = PyTuple_New(6);
    for ( i=0; i<6; ++i )
	PyTuple_SetItem(tuple,i,Py_BuildValue("d",(double) r3[i]));
return( tuple );
}

static PyObject *PyPS_Inverse(PyObject *noself, PyObject *args) {
    double m1[6];
    real r1[6], r3[6];
    int i;
    PyObject *tuple;

    if ( !PyArg_ParseTuple(args,"(dddddd)",
	    &m1[0], &m1[1], &m1[2], &m1[3], &m1[4], &m1[5] ))
return( NULL );
    for ( i=0; i<6; ++i )
	r1[i] = m1[i];
    MatInverse(r3,r1);
    tuple = PyTuple_New(6);
    for ( i=0; i<6; ++i )
	PyTuple_SetItem(tuple,i,Py_BuildValue("d",(double) r3[i]));
return( tuple );
}

static PyMethodDef psMat_methods[] = {
    { "identity", PyPS_Identity, METH_NOARGS, "Identity transformation" },
    { "translate", PyPS_Translate, METH_VARARGS, "Translation transformation" },
    { "rotate", PyPS_Rotate, METH_VARARGS, "Rotation transformation" },
    { "skew", PyPS_Skew, METH_VARARGS, "Skew transformation (for making a oblique font)" },
    { "scale", PyPS_Scale, METH_VARARGS, "Scale transformation" },
    { "compose", PyPS_Compose, METH_VARARGS, "Compose two transformations (matrix multiplication)" },
    { "inverse", PyPS_Inverse, METH_VARARGS, "Provide an inverse transformations (not always possible)" },
    NULL
};

void PyFF_ErrorString(const char *msg,const char *str) {
    char *cond = (char *) msg;
    if ( str!=NULL )
	cond = strconcat3(msg, " ", str);
    PyErr_SetString(PyExc_ValueError, cond );
    if ( cond!=msg )
	free(cond);
}

void PyFF_ErrorF3(const char *frmt, const char *str, int size, int depth) {
    PyErr_Format(PyExc_ValueError, frmt, str, size, depth );
}

static PyMODINIT_FUNC initPyFontForge(void) {
    PyObject* m;
    int i;
    static PyTypeObject *types[] = { &PyFF_PointType, &PyFF_ContourType,
	    &PyFF_LayerType, &PyFF_GlyphPenType, &PyFF_GlyphType,
	    &PyFF_CvtType, &PyFF_PrivateIterType, &PyFF_PrivateType,
	    &PyFF_FontIterType, &PyFF_SelectionType, &PyFF_FontType,
	    &PyFF_ContourIterType, &PyFF_LayerIterType,
	    NULL };
    static char *names[] = { "point", "contour", "layer", "glyph", "glyphPen",
	    "cvt", "privateiter", "private", "fontiter", "selection", "font",
	    "contouriter", "layeriter",
	    NULL };

    for ( i=0; types[i]!=NULL; ++i ) {
	types[i]->ob_type = &PyType_Type;		/* Or does Type_Ready do this??? */
	if (PyType_Ready(types[i]) < 0)
return;
    }

    m = Py_InitModule3("fontforge", FontForge_methods,
                       "FontForge font manipulation module.");

    for ( i=0; types[i]!=NULL; ++i ) {
	Py_INCREF(types[i]);
	PyModule_AddObject(m, names[i], (PyObject *)types[i]);
    }

    m = Py_InitModule3("psMat", psMat_methods,
                       "PostScript Matrix manipulation");
    /* No types, just tuples */
}

void FontForge_PythonInit(void) {
    PyImport_AppendInittab("fontforge", initPyFontForge);
#ifdef MAC
    PyMac_Initialize();
#else
    Py_Initialize();
#endif
}

extern int no_windowing_ui, running_script;

void PyFF_Stdin(void) {
    no_windowing_ui = running_script = true;
    if ( isatty(fileno(stdin)))
	PyRun_InteractiveLoop(stdin,"<stdin>");
    else
	PyRun_SimpleFile(stdin,"<stdin>");
    exit(0);
}

void PyFF_Main(int argc,char **argv,int start) {
    char **newargv, *arg;
    int i;

    no_windowing_ui = running_script = true;
    newargv= gcalloc(argc+1,sizeof(char *));
    arg = argv[start];
    if ( *arg=='-' && arg[1]=='-' ) ++arg;
    if ( strcmp(arg,"-script")==0 )
	++start;
    newargv[0] = argv[0];
    for ( i=start; i<argc; ++i )
	newargv[i-start+1] = argv[i];
    newargv[i-start+1] = NULL;
    exit( Py_Main( i-start+1,newargv ));
}

void PyFF_ScriptFile(FontView *fv,char *filename) {
    FILE *fp = fopen(filename,"r");

    fv_active_in_ui = fv;		/* Make fv known to interpreter */
    if ( fp==NULL )
	LogError( "Can't open %s", filename );
    else {
	PyRun_SimpleFile(fp,filename);
	fclose(fp);
    }
}

void PyFF_ScriptString(FontView *fv,char *str) {

    fv_active_in_ui = fv;		/* Make fv known to interpreter */
    PyRun_SimpleString(str);
}

void PyFF_FreeFV(FontView *fv) {
    if ( fv->python_fv_object!=NULL ) {
	((PyFF_Font *) (fv->python_fv_object))->fv = NULL;
	Py_DECREF( (PyObject *) (fv->python_fv_object));
    }
    Py_XDECREF( (PyObject *) (fv->python_data));
}

void PyFF_FreeSC(SplineChar *sc) {
    if ( sc->python_sc_object!=NULL ) {
	((PyFF_Glyph *) (sc->python_sc_object))->sc = NULL;
	Py_DECREF( (PyObject *) (sc->python_sc_object));
    }
    Py_XDECREF( (PyObject *) (sc->python_data));
}
#endif		/* _NO_PYTHON */
