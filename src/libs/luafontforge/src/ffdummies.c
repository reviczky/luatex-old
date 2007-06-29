
/* some dummy functions and variables so that a few ff source files can be ignored */

#define FONTFORGE_CONFIG_NO_WINDOWING_UI 1

#include "pfaeditui.h"
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <utype.h>
#include <chardata.h>
#include <ustring.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sd.h"

/* a gettext miss */

char *sgettext(const char *msgid) {
    const char *msgval = msgid;
    char *found;
    if ( (found = strrchr (msgid, '|'))!=NULL )
      msgval = found+1;
    return (char *) msgval;
}


/* some error and process reporting stuff */

void gwwv_post_error(char *a, char *b) { }
void gwwv_ask (char *a) { }
void gwwv_post_notice (char *a) { }
void gwwv_progress_change_line1 (char *a) { }
void gwwv_progress_change_line2 (char *b) { }
void gwwv_progress_next (void){ }
void gwwv_progress_start_indicator (void) { }
void gwwv_progress_end_indicator (void) { }
void gwwv_progress_enable_stop (void) { }
void gwwv_progress_resume_timer (void) { }
void gwwv_progress_pause_timer (void) { }
void gwwv_progress_next_stage (void) { }
void gwwv_progress_change_stages (void) { }
void gwwv_progress_change_total (void) { }
void gwwv_progress_increment (void) { }

void GWidgetError8(char *a, char *b) { }

void GProgressChangeLine1_8 (char *a) { }
void GProgressChangeLine2_8 (char *b) { }
void GProgressChangeTotal (void) { }
void GProgressNext (void){ }
void GProgressStartIndicator (void) { }
void GProgressEndIndicator (void) { }
void GProgressEnableStop (void) { }
void GProgressResumeTimer (void) { }
void GProgressPauseTimer (void) { }
void GProgressNextStage (void) { }
void GProgressChangeStages (void) { }
void GProgressIncrement (void) { }

/* from plugins.c */

void LoadPluginDir(void *a) { }

/* from scripting.c */

int no_windowing_ui = 1;
int running_script = 1;
int use_utf8_in_script = 1;

void ScriptError( void *c, const char *msg ) { }

void ScriptErrorString( void *c, const char *msg, const char *name) { }

void ScriptErrorF( void *c, const char *format, ... ) { }

void CheckIsScript(int argc, char *argv[]) { }

void ExecuteScriptFile(FontView *fv, char *filename) { }

void ScriptDlg(FontView *fv) { }

struct dictentry {
    char *name;
    Val val;
};

struct dictionary {
    struct dictentry *entries;
    int cnt, max;
};


typedef struct array {
    int argc;
    Val *vals;
} Array;

static void arrayfree(Array *a) {
    int i;

    for ( i=0; i<a->argc; ++i ) {
	if ( a->vals[i].type==v_str )
	    free(a->vals[i].u.sval);
	else if ( a->vals[i].type==v_arr )
	    arrayfree(a->vals[i].u.aval);
    }
    free(a->vals);
    free(a);
}


void DictionaryFree(struct dictionary *dica) {
    int i;

    if ( dica==NULL )
return;

    for ( i=0; i<dica->cnt; ++i ) {
	free(dica->entries[i].name );
	if ( dica->entries[i].val.type == v_str )
	    free( dica->entries[i].val.u.sval );
	if ( dica->entries[i].val.type == v_arr )
	    arrayfree( dica->entries[i].val.u.aval );
    }
    free( dica->entries );
}


char **GetFontNames(char *filename) {
    FILE *foo;
    char **ret = NULL;

    if ( GFileIsDir(filename)) {
#if 0
	char *temp = galloc(strlen(filename)+strlen("/glyphs/contents.plist")+1);
	strcpy(temp,filename);
	strcat(temp,"/glyphs/contents.plist");
	if ( GFileExists(temp))
	    ret = NamesReadUFO(filename);
	else
	  {
	    strcpy(temp,filename);
	    strcat(temp,"/font.props");
	    if ( GFileExists(temp))
		ret = NamesReadSFD(temp);
		/* The fonts.prop file will look just like an sfd file as far */
		/* as fontnames are concerned, we don't need a separate routine*/
	}
	free(temp);
#endif 
    } else {
	foo = fopen(filename,"rb");
	if ( foo!=NULL ) {
	    /* Try to guess the file type from the first few characters... */
	    int ch1 = getc(foo);
	    int ch2 = getc(foo);
	    int ch3 = getc(foo);
	    int ch4 = getc(foo);
	    int ch5, ch6;
	    fseek(foo, 98, SEEK_SET);
	    ch5 = getc(foo);
	    ch6 = getc(foo);
	    fclose(foo);
	    if (( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
		    (ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
		    (ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e') ||
		    (ch1=='t' && ch2=='t' && ch3=='c' && ch4=='f') ) {
		ret = NamesReadTTF(filename);
	    } else if (( ch1=='%' && ch2=='!' ) ||
			( ch1==0x80 && ch2=='\01' ) ) {	/* PFB header */
		ret = NamesReadPostscript(filename);
#if 0
	    } else if ( ch1=='<' && ch2=='?' && (ch3=='x'||ch3=='X') && (ch4=='m'||ch4=='M') ) {
		ret = NamesReadSVG(filename);
	    } else if ( ch1=='S' && ch2=='p' && ch3=='l' && ch4=='i' ) {
		ret = NamesReadSFD(filename);
#endif
	    } else if ( ch1==1 && ch2==0 && ch3==4 ) {
		ret = NamesReadCFF(filename);
	    } else /* Too hard to figure out a valid mark for a mac resource file */
		ret = NamesReadMacBinary(filename);
	}
    }
return( ret );
}


/* from svg.c */

SplineChar *SCHasSubs(SplineChar *sc, uint32 tag) {
    PST *pst;

    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
        if ( pst->type==pst_substitution &&
                FeatureTagInFeatureScriptList(tag,pst->subtable->lookup->features) )
return( SFGetChar(sc->parent,-1,pst->u.subs.variant));
    }
return( NULL );
}

int _ExportSVG(FILE *svg,SplineChar *sc) {
  return 0;
}

int WriteSVGFont(char *fontname,SplineFont *sf,enum fontformat format,int flags,
	EncMap *map) {
  return 1;
}

int HasSVG(void) { return 0; }


SplineFont *SFReadSVG(char *filename, int flags) {
return( NULL );
}

SplineSet *SplinePointListInterpretSVG(char *filename,char *memory, int memlen,
	int em_size,int ascent,int is_stroked) {
return( NULL );
}

/* from ufo.c */

int HasUFO(void) {
return( false );
}

SplineFont *SFReadUFO(char *filename, int flags) {
return( NULL );
}

SplineSet *SplinePointListInterpretGlif(char *filename,char *memory, int memlen,
	int em_size,int ascent,int is_stroked) {
return( NULL );
}

int WriteUFOFont(char *basedir,SplineFont *sf,enum fontformat ff,int flags,
	EncMap *map) {
  return 1;
}

int _ExportGlif(FILE *glif,SplineChar *sc) {
return 0;
}

/* some Gdraw routines for filesystem access */


int GFileIsDir(const char *file) {
    char buffer[1000];
    sprintf(buffer,"%s/.",file);
return( access(buffer,0)==0 );
}

int GFileExists(const char *file) {
return( access(file,0)==0 );
}

char *GFileNameTail(const char *oldname) {
    char *pt;

    pt = strrchr(oldname,'/');
    if ( pt !=NULL )
return( pt+1);
    else
return( (char *)oldname );
}


char *GFileAppendFile(char *dir,char *name,int isdir) {
    char *ret, *pt;

    ret = galloc((strlen(dir)+strlen(name)+3));
    strcpy(ret,dir);
    pt = ret+strlen(ret);
    if ( pt>ret && pt[-1]!='/' )
        *pt++ = '/';
    strcpy(pt,name);
    if ( isdir ) {
        pt += strlen(pt);
        if ( pt>ret && pt[-1]!='/' ) {
            *pt++ = '/';
            *pt = '\0';
        }
    }
return(ret);
}

int GFileReadable(char *file) {
return( access(file,04)==0 );
}

static char dirname_[1024];


char *GFileGetAbsoluteName(char *name, char *result, int rsiz) {
    /* result may be the same as name */
    char buffer[1000];

    if ( *name!='/' ) {
        char *pt, *spt, *rpt, *bpt;

        if ( dirname_[0]=='\0' ) {
            getcwd(dirname_,sizeof(dirname_));
        }
        strcpy(buffer,dirname_);
        if ( buffer[strlen(buffer)-1]!='/' )
            strcat(buffer,"/");
        strcat(buffer,name);

        /* Normalize out any .. */
        spt = rpt = buffer;
        while ( *spt!='\0' ) {
            if ( *spt=='/' ) ++spt;
            for ( pt = spt; *pt!='\0' && *pt!='/'; ++pt );
            if ( pt==spt )      /* Found // in a path spec, reduce to / (we've*/
                strcpy(spt,pt); /*  skipped past the :// of the machine name) */
            else if ( pt==spt+1 && spt[0]=='.' )        /* Noop */
                strcpy(spt,pt);
            else if ( pt==spt+2 && spt[0]=='.' && spt[1]=='.' ) {
                for ( bpt=spt-2 ; bpt>rpt && *bpt!='/'; --bpt );
                if ( bpt>=rpt && *bpt=='/' ) {
                    strcpy(bpt,pt);
                    spt = bpt;
                } else {
                    rpt = pt;
                    spt = pt;
                }
            } else
                spt = pt;
        }
        name = buffer;
        if ( rsiz>sizeof(buffer)) rsiz = sizeof(buffer);        /* Else valgrind gets unhappy */
    }
    if (result!=name) {
        strncpy(result,name,rsiz);
        result[rsiz-1]='\0';
    }
return(result);
}

/* from autosave.c, but much shortened */

char *getPfaEditDir(char *buffer) {
  return( NULL );
}

void DoAutoSaves (void ) {}

/* from dumppfa.c, but much shortened */

const char *GetAuthor(void) {
  static char author[200] = { '\0' };
  const char *ret = NULL, *pt;

  if ((pt=getenv("USER"))!=NULL ) {
    strncpy(author,pt,sizeof(author));
    author[sizeof(author)-1] = '\0';
    ret = author;
  }
  return ret;
}

double BlueScaleFigure(struct psdict *private,real bluevalues[], real otherblues[]) {
    double max_diff=0, p1, p2;
    char *pt, *end;
    int i;

    if ( PSDictHasEntry(private,"BlueScale")!=NULL )
return( -1 );

    pt = PSDictHasEntry(private,"BlueValues");
    if ( pt!=NULL ) {
	while ( *pt==' ' || *pt=='[' ) ++pt;
	forever {
	    p1 = strtod(pt,&end);
	    if ( end==pt )
	break;
	    pt = end;
	    p2 = strtod(pt,&end);
	    if ( end==pt )
	break;
	    if ( p2-p1 >max_diff ) max_diff = p2-p1;
	    pt = end;
	}
    } else {
	for ( i=0; i<14 && (bluevalues[i]!=0 || bluevalues[i+1])!=0; i+=2 ) {
	    if ( bluevalues[i+1] - bluevalues[i]>=max_diff )
		max_diff = bluevalues[i+1] - bluevalues[i];
	}
    }

    pt = PSDictHasEntry(private,"OtherBlues");
    if ( pt!=NULL ) {
	while ( *pt==' ' || *pt=='[' ) ++pt;
	forever {
	    p1 = strtod(pt,&end);
	    if ( end==pt )
	break;
	    pt = end;
	    p2 = strtod(pt,&end);
	    if ( end==pt )
	break;
	    if ( p2-p1 >max_diff ) max_diff = p2-p1;
	    pt = end;
	}
    } else {
	for ( i=0; i<10 && (otherblues[i]!=0 || otherblues[i+1]!=0); i+=2 ) {
	    if ( otherblues[i+1] - otherblues[i]>=max_diff )
		max_diff = otherblues[i+1] - otherblues[i];
	}
    }
    if ( max_diff<=0 )
return( -1 );
    if ( 1/max_diff > .039625 )
return( -1 );

return( .99/max_diff );
}

/* from dumpbdf.c */

int IsntBDFChar(BDFChar *bdfc) {
    if ( bdfc==NULL )
return( true );

return( !SCWorthOutputting(bdfc->sc));
}

