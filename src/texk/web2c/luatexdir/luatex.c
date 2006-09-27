/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include <zlib.h>

/* do this aleph stuff here, for now */

void
btestin(void)
{
  string fname = kpse_find_file ((char *)(nameoffile + 1), 
				   kpse_program_binary_format, true);

    if (fname) {
      libcfree(nameoffile);
      nameoffile = xmalloc(2+strlen(fname));
      namelength = strlen(fname);
      strcpy((char *)nameoffile+1, fname);
    }
    else {
      libcfree(nameoffile);
      nameoffile = xmalloc(2);
      namelength = 0;
      nameoffile[0] = 0;
      nameoffile[1] = 0;
    }
}
 
memoryword **fonttables;
static int font_entries = 0;

void
allocatefonttable P2C(int, font_number, int, font_size)
{
    int i;
    if (font_entries==0) {
      fonttables = (memoryword **) xmalloc(256*sizeof(memoryword**));
      font_entries=256;
    } else if ((font_number==256)&&(font_entries==256)) {
      fonttables = xrealloc(fonttables, 65536);
      font_entries=65536;
    }
    fonttables[font_number] =
       (memoryword *) xmalloc((font_size+1)*sizeof(memoryword));
    fonttables[font_number][0].cint = font_size;
    fonttables[font_number][0].cint1 = 0;
    for (i=1; i<=font_size; i++) {
        fonttables[font_number][i].cint  = 0;
        fonttables[font_number][i].cint1 = 0;
    }
}

void
dumpfonttable P2C(int, font_number, int, words)
{
    fonttables[font_number][0].cint=words;
    dumpthings(fonttables[font_number][0], fonttables[font_number][0].cint+1);
}

void
undumpfonttable(font_number)
int font_number;
{
    memoryword sizeword;
    if (font_entries==0) {
      fonttables = (memoryword **) xmalloc(256*sizeof(memoryword**));
      font_entries=256;
    } else if ((font_number==256)&&(font_entries==256)) {
      fonttables = xrealloc(fonttables, 65536);
      font_entries=65536;
    }

    undumpthings(sizeword,1);
    fonttables[font_number] =
        (memoryword *) xmalloc((sizeword.cint+1)*sizeof(memoryword));
    fonttables[font_number][0].cint = sizeword.cint;
    undumpthings(fonttables[font_number][1], sizeword.cint);
}


int **ocptables;
static int ocp_entries = 0;

void
allocateocptable P2C(int, ocp_number, int, ocp_size)
{
    int i;
    if (ocp_entries==0) {
      ocptables = (int **) xmalloc(256*sizeof(int**));
      ocp_entries=256;
    } else if ((ocp_number==256)&&(ocp_entries==256)) {
      ocptables = xrealloc(ocptables, 65536);
      ocp_entries=65536;
    }
    ocptables[ocp_number] =
       (int *) xmalloc((1+ocp_size)*sizeof(int));
    ocptables[ocp_number][0] = ocp_size;
    for (i=1; i<=ocp_size; i++) {
        ocptables[ocp_number][i]  = 0;
    }
}

void
dumpocptable P1C(int, ocp_number)
{
    dumpthings(ocptables[ocp_number][0], ocptables[ocp_number][0]+1);
}

void
undumpocptable P1C(int, ocp_number)
{
    int sizeword;
    if (ocp_entries==0) {
      ocptables = (int **) xmalloc(256*sizeof(int**));
      ocp_entries=256;
    } else if ((ocp_number==256)&&(ocp_entries==256)) {
      ocptables = xrealloc(ocptables, 65536);
      ocp_entries=65536;
    }
    undumpthings(sizeword,1);
    ocptables[ocp_number] =
        (int *) xmalloc((1+sizeword)*sizeof(int));
    ocptables[ocp_number][0] = sizeword;
    undumpthings(ocptables[ocp_number][1], sizeword);
}


void
runexternalocp P1C(string, external_ocp_name)
{
  char *in_file_name;
  char *out_file_name;
  FILE *in_file;
  FILE *out_file;
  char command_line[400];
  int i;
  unsigned c;
  int c_in;
#ifdef WIN32
  char *tempenv;

#define null_string(s) ((s == NULL) || (*s == '\0'))

  tempenv = getenv("TMPDIR");
  if (null_string(tempenv))
    tempenv = getenv("TEMP");
  if (null_string(tempenv))
    tempenv = getenv("TMP");
  if (null_string(tempenv))
    tempenv = "c:/tmp";	/* "/tmp" is not good if we are on a CD-ROM */
  in_file_name = concat(tempenv, "/__aleph__in__XXXXXX");
  mktemp(in_file_name);
#else
  in_file_name = strdup("/tmp/__aleph__in__XXXXXX");
  mkstemp(in_file_name);
#endif /* WIN32 */

  in_file = fopen(in_file_name, FOPEN_WBIN_MODE);
  
  for (i=1; i<=otpinputend; i++) {
      c = otpinputbuf[i];
      if (c>0xffff) {
          fprintf(stderr, "Aleph does not currently support 31-bit chars\n");
          exit(1);
      }
      if (c>0x4000000) {
          fputc(0xfc | ((c>>30) & 0x1), in_file);
          fputc(0x80 | ((c>>24) & 0x3f), in_file);
          fputc(0x80 | ((c>>18) & 0x3f), in_file);
          fputc(0x80 | ((c>>12) & 0x3f), in_file);
          fputc(0x80 | ((c>>6) & 0x3f), in_file);
          fputc(0x80 | (c & 0x3f), in_file);
      } else if (c>0x200000) {
          fputc(0xf8 | ((c>>24) & 0x3), in_file);
          fputc(0x80 | ((c>>18) & 0x3f), in_file);
          fputc(0x80 | ((c>>12) & 0x3f), in_file);
          fputc(0x80 | ((c>>6) & 0x3f), in_file);
          fputc(0x80 | (c & 0x3f), in_file);
      } else if (c>0x10000) {
          fputc(0xf0 | ((c>>18) & 0x7), in_file);
          fputc(0x80 | ((c>>12) & 0x3f), in_file);
          fputc(0x80 | ((c>>6) & 0x3f), in_file);
          fputc(0x80 | (c & 0x3f), in_file);
      } else if (c>0x800) {
          fputc(0xe0 | ((c>>12) & 0xf), in_file);
          fputc(0x80 | ((c>>6) & 0x3f), in_file);
          fputc(0x80 | (c & 0x3f), in_file);
      } else if (c>0x80) {
          fputc(0xc0 | ((c>>6) & 0x1f), in_file);
          fputc(0x80 | (c & 0x3f), in_file);
      } else {
          fputc(c & 0x7f, in_file);
      }
  }
  fclose(in_file);
  
#define advance_cin if ((c_in = fgetc(out_file)) == -1) { \
                         fprintf(stderr, "File contains bad char\n"); \
                         goto end_of_while; \
                    }
                     
#ifdef WIN32
  out_file_name = concat(tempenv, "/__aleph__out__XXXXXX");
  mktemp(out_file_name);
#else
  out_file_name = strdup("/tmp/__aleph__out__XXXXXX");
  mkstemp(out_file_name);
#endif

  sprintf(command_line, "%s <%s >%s\n",
          external_ocp_name+1, in_file_name, out_file_name);
  system(command_line);
  out_file = fopen(out_file_name, FOPEN_RBIN_MODE);
  otpoutputend = 0;
  otpoutputbuf[otpoutputend] = 0;
  while ((c_in = fgetc(out_file)) != -1) {
     if (c_in>=0xfc) {
         c = (c_in & 0x1)   << 30;
         {advance_cin}
         c |= (c_in & 0x3f) << 24;
         {advance_cin}
         c |= (c_in & 0x3f) << 18;
         {advance_cin}
         c |= (c_in & 0x3f) << 12;
         {advance_cin}
         c |= (c_in & 0x3f) << 6;
         {advance_cin}
         c |= c_in & 0x3f;
     } else if (c_in>=0xf8) {
         c = (c_in & 0x3) << 24;
         {advance_cin}
         c |= (c_in & 0x3f) << 18;
         {advance_cin}
         c |= (c_in & 0x3f) << 12;
         {advance_cin}
         c |= (c_in & 0x3f) << 6;
         {advance_cin}
         c |= c_in & 0x3f;
     } else if (c_in>=0xf0) {
         c = (c_in & 0x7) << 18;
         {advance_cin}
         c |= (c_in & 0x3f) << 12;
         {advance_cin}
         c |= (c_in & 0x3f) << 6;
         {advance_cin}
         c |= c_in & 0x3f;
     } else if (c_in>=0xe0) {
         c = (c_in & 0xf) << 12;
         {advance_cin}
         c |= (c_in & 0x3f) << 6;
         {advance_cin}
         c |= c_in & 0x3f;
     } else if (c_in>=0x80) {
         c = (c_in & 0x1f) << 6;
         {advance_cin}
         c |= c_in & 0x3f;
     } else {
         c = c_in & 0x7f;
     }
     otpoutputbuf[++otpoutputend] = c;
  }

end_of_while:
  remove(in_file_name);
  remove(out_file_name);
}

/* Read and write dump files through zlib */

static int totalout = 0;
static int totalin = 0;

void
do_zdump (char *p,  int item_size,  int nitems, FILE *out_file)
{
  int err;
  if (nitems==0)
	return;
  //  fprintf(stderr,"*%dx%d->%d",item_size,nitems,totalout);
  //  totalout += item_size*nitems;
  if (gzwrite ((gzFile)out_file,(void *)p, item_size*nitems) != item_size*nitems)
    {
      fprintf (stderr, "! Could not write %d %d-byte item(s): %s.\n",
               nitems, item_size, gzerror((gzFile)out_file,&err));
      uexit (1);
    }
}

void
do_zundump (char *p,  int item_size,  int nitems, FILE *in_file)
{
  int err;
  if (nitems==0)
	return;
  //totalin += item_size*nitems;
  //  fprintf(stderr,"*%dx%d->%d",item_size,nitems,totalin);
  if (gzread ((gzFile)in_file,(void *)p, item_size*nitems) <= 0) 
	{
	  fprintf (stderr, "Could not undump %d %d-byte item(s): %s.\n",
			   nitems, item_size, gzerror((gzFile)in_file,&err));
	  uexit (1);
	}
}

boolean 
zopen_w_input (FILE **f, int format, const_string fopen_mode) {
  int res = open_input(f,format,fopen_mode);
  if (res) {
	*f = (FILE *)gzdopen(fileno(*f),"rb3");
  }
  return res;
}

boolean 
zopen_w_output (FILE **f, const_string fopen_mode) {
  int res =  open_output(f,fopen_mode);
  if (res) {
	*f = (FILE *)gzdopen(fileno(*f),"wb3");
  }
  return res;
}

void 
zwclose (FILE *f) { 
  //  fprintf (stderr, "Uncompressed sizes: in=%d,out=%d\n",totalin,totalout);
  gzclose((gzFile)f); 
}
