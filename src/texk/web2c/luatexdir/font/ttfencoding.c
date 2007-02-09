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

/* 
   The truetype and opentype specifications are a bit of a mess.  I
   had no desire to spend months studying the sometimes conflicting
   standards by Adobe and Microsoft, so I have deduced the internal
   file stucture of ttf&otf from George Williams' font editor,
   FontForge. 
*/

#include "ttf.h"

#undef strstart
#include "ustring.h"
#include "chardata.h"

#include "pua.c"

struct cidmap {
    char *registry, *ordering;
    int supplement, maxsupple;
    int cidmax;                 /* Max cid found in the charset */
    int namemax;                /* Max cid with useful info */
    uint32 *unicode;
    char **name;
    struct cidmap *next;
};

static struct cidmap *cidmaps = NULL;

static const int unicode4_size = 17*65536;

int strmatch(const char *str1, const char *str2) {
    int ch1, ch2;
    for (;;) {
        ch1 = *str1++; ch2 = *str2++ ;
        ch1 = tolower(ch1);
        ch2 = tolower(ch2);
        if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
}

int strnmatch(const char *str1, const char *str2, int n) {
    int ch1, ch2;
    for (;n-->0;) {
        ch1 = *str1++; ch2 = *str2++ ;
        ch1 = tolower(ch1);
        ch2 = tolower(ch2);
        if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
return(0);
}


char *strstrmatch(const char *longer, const char *substr) {
    int ch1, ch2;
    const char *lpt, *str1, *str2;

    for ( lpt=longer; *lpt!='\0'; ++lpt ) {
        str1 = lpt; str2 = substr;
        for (;;) {
            ch1 = *str1++; ch2 = *str2++ ;
            ch1 = tolower(ch1);
            ch2 = tolower(ch2);
            if ( ch2=='\0' )
			  return((char *) lpt);
            if ( ch1!=ch2 )
        break;
        }
    }
	return( NULL );
}

char *AdobeExpertEncoding[] = {
/* 0000 */	".notdef",
/* 0001 */	".notdef",
/* 0002 */	".notdef",
/* 0003 */	".notdef",
/* 0004 */	".notdef",
/* 0005 */	".notdef",
/* 0006 */	".notdef",
/* 0007 */	".notdef",
/* 0008 */	".notdef",
/* 0009 */	".notdef",
/* 000a */	".notdef",
/* 000b */	".notdef",
/* 000c */	".notdef",
/* 000d */	".notdef",
/* 000e */	".notdef",
/* 000f */	".notdef",
/* 0010 */	".notdef",
/* 0011 */	".notdef",
/* 0012 */	".notdef",
/* 0013 */	".notdef",
/* 0014 */	".notdef",
/* 0015 */	".notdef",
/* 0016 */	".notdef",
/* 0017 */	".notdef",
/* 0018 */	".notdef",
/* 0019 */	".notdef",
/* 001a */	".notdef",
/* 001b */	".notdef",
/* 001c */	".notdef",
/* 001d */	".notdef",
/* 001e */	".notdef",
/* 001f */	".notdef",
/* 0020 */	"space",
/* 0021 */	"exclamsmall",
/* 0022 */	"Hungarumlautsmal",
/* 0023 */	".notdef",
/* 0024 */	"dollaroldstyle",
/* 0025 */	"dollarsuperior",
/* 0026 */	"ampersandsmall",
/* 0027 */	"Acutesmall",
/* 0028 */	"parenleftsuperior",
/* 0029 */	"parenrightsuperior",
/* 002a */	"twodotenleader",
/* 002b */	"onedotenleader",
/* 002c */	"comma",
/* 002d */	"hyphen",
/* 002e */	"period",
/* 002f */	"fraction",
/* 0030 */	"zerooldstyle",
/* 0031 */	"oneoldstyle",
/* 0032 */	"twooldstyle",
/* 0033 */	"threeoldstyle",
/* 0034 */	"fouroldstyle",
/* 0035 */	"fiveoldstyle",
/* 0036 */	"sixoldstyle",
/* 0037 */	"sevenoldstyle",
/* 0038 */	"eightoldstyle",
/* 0039 */	"nineoldstyle",
/* 003a */	"colon",
/* 003b */	"semicolon",
/* 003c */	"commasuperior",
/* 003d */	"threequartersemdash",
/* 003e */	"periodsuperior",
/* 003f */	"questionsmall",
/* 0040 */	".notdef",
/* 0041 */	"asuperior",
/* 0042 */	"bsuperior",
/* 0043 */	"centsuperior",
/* 0044 */	"dsuperior",
/* 0045 */	"esuperior",
/* 0046 */	".notdef",
/* 0047 */	".notdef",
/* 0048 */	".notdef",
/* 0049 */	"isuperior",
/* 004a */	".notdef",
/* 004b */	".notdef",
/* 004c */	"lsuperior",
/* 004d */	"msuperior",
/* 004e */	"nsuperior",
/* 004f */	"osuperior",
/* 0050 */	".notdef",
/* 0051 */	".notdef",
/* 0052 */	"rsuperior",
/* 0053 */	"ssuperior",
/* 0054 */	"tsuperior",
/* 0055 */	".notdef",
/* 0056 */	"ff",
/* 0057 */	"fi",
/* 0058 */	"fl",
/* 0059 */	"ffi",
/* 005a */	"ffl",
/* 005b */	"parenleftinferior",
/* 005c */	".notdef",
/* 005d */	"parenrightinferior",
/* 005e */	"Circumflexsmall",
/* 005f */	"hyphensuperior",
/* 0060 */	"Gravesmall",
/* 0061 */	"Asmall",
/* 0062 */	"Bsmall",
/* 0063 */	"Csmall",
/* 0064 */	"Dsmall",
/* 0065 */	"Esmall",
/* 0066 */	"Fsmall",
/* 0067 */	"Gsmall",
/* 0068 */	"Hsmall",
/* 0069 */	"Ismall",
/* 006a */	"Jsmall",
/* 006b */	"Ksmall",
/* 006c */	"Lsmall",
/* 006d */	"Msmall",
/* 006e */	"Nsmall",
/* 006f */	"Osmall",
/* 0070 */	"Psmall",
/* 0071 */	"Qsmall",
/* 0072 */	"Rsmall",
/* 0073 */	"Ssmall",
/* 0074 */	"Tsmall",
/* 0075 */	"Usmall",
/* 0076 */	"Vsmall",
/* 0077 */	"Wsmall",
/* 0078 */	"Xsmall",
/* 0079 */	"Ysmall",
/* 007a */	"Zsmall",
/* 007b */	"colonmonetary",
/* 007c */	"onefitted",
/* 007d */	"rupiah",
/* 007e */	"Tildesmall",
/* 007f */	".notdef",
/* 0080 */	".notdef",
/* 0081 */	".notdef",
/* 0082 */	".notdef",
/* 0083 */	".notdef",
/* 0084 */	".notdef",
/* 0085 */	".notdef",
/* 0086 */	".notdef",
/* 0087 */	".notdef",
/* 0088 */	".notdef",
/* 0089 */	".notdef",
/* 008a */	".notdef",
/* 008b */	".notdef",
/* 008c */	".notdef",
/* 008d */	".notdef",
/* 008e */	".notdef",
/* 008f */	".notdef",
/* 0090 */	".notdef",
/* 0091 */	".notdef",
/* 0092 */	".notdef",
/* 0093 */	".notdef",
/* 0094 */	".notdef",
/* 0095 */	".notdef",
/* 0096 */	".notdef",
/* 0097 */	".notdef",
/* 0098 */	".notdef",
/* 0099 */	".notdef",
/* 009a */	".notdef",
/* 009b */	".notdef",
/* 009c */	".notdef",
/* 009d */	".notdef",
/* 009e */	".notdef",
/* 009f */	".notdef",
/* 00a0 */	".notdef",
/* 00a1 */	"exclamdownsmall",
/* 00a2 */	"centoldstyle",
/* 00a3 */	"Lslashsmall",
/* 00a4 */	".notdef",
/* 00a5 */	".notdef",
/* 00a6 */	"Scaronsmall",
/* 00a7 */	"Zcaronsmall",
/* 00a8 */	"Dieresissmall",
/* 00a9 */	"Brevesmall",
/* 00aa */	"Caronsmall",
/* 00ab */	".notdef",
/* 00ac */	"Dotaccentsmall",
/* 00ad */	".notdef",
/* 00ae */	".notdef",
/* 00af */	"Macronsmall",
/* 00b0 */	".notdef",
/* 00b1 */	".notdef",
/* 00b2 */	"figuredash",
/* 00b3 */	"hypheninferior",
/* 00b4 */	".notdef",
/* 00b5 */	".notdef",
/* 00b6 */	"Ogoneksmall",
/* 00b7 */	"Ringsmall",
/* 00b8 */	"Cedillasmall",
/* 00b9 */	".notdef",
/* 00ba */	".notdef",
/* 00bb */	".notdef",
/* 00bc */	"onequarter",
/* 00bd */	"onehalf",
/* 00be */	"threequarters",
/* 00bf */	"questiondownsmall",
/* 00c0 */	"oneeighth",
/* 00c1 */	"threeeighths",
/* 00c2 */	"fiveeighths",
/* 00c3 */	"seveneighths",
/* 00c4 */	"onethird",
/* 00c5 */	"twothirds",
/* 00c6 */	".notdef",
/* 00c7 */	".notdef",
/* 00c8 */	"zerosuperior",
/* 00c9 */	"onesuperior",
/* 00ca */	"twosuperior",
/* 00cb */	"threesuperior",
/* 00cc */	"foursuperior",
/* 00cd */	"fivesuperior",
/* 00ce */	"sixsuperior",
/* 00cf */	"sevensuperior",
/* 00d0 */	"eightsuperior",
/* 00d1 */	"ninesuperior",
/* 00d2 */	"zeroinferior",
/* 00d3 */	"oneinferior",
/* 00d4 */	"twoinferior",
/* 00d5 */	"threeinferior",
/* 00d6 */	"fourinferior",
/* 00d7 */	"fiveinferior",
/* 00d8 */	"sixinferior",
/* 00d9 */	"seveninferior",
/* 00da */	"eightinferior",
/* 00db */	"nineinferior",
/* 00dc */	"centinferior",
/* 00dd */	"dollarinferior",
/* 00de */	"periodinferior",
/* 00df */	"commainferior",
/* 00e0 */	"Agravesmall",
/* 00e1 */	"Aacutesmall",
/* 00e2 */	"Acircumflexsmall",
/* 00e3 */	"Atildesmall",
/* 00e4 */	"Adieresissmall",
/* 00e5 */	"Aringsmall",
/* 00e6 */	"AEsmall",
/* 00e7 */	"Ccedillasmall",
/* 00e8 */	"Egravesmall",
/* 00e9 */	"Eacutesmall",
/* 00ea */	"Ecircumflexsmall",
/* 00eb */	"Edieresissmall",
/* 00ec */	"Igravesmall",
/* 00ed */	"Iacutesmall",
/* 00ee */	"Icircumflexsmall",
/* 00ef */	"Idieresissmall",
/* 00f0 */	"Ethsmall",
/* 00f1 */	"Ntildesmall",
/* 00f2 */	"Ogravesmall",
/* 00f3 */	"Oacutesmall",
/* 00f4 */	"Ocircumflexsmall",
/* 00f5 */	"Otildesmall",
/* 00f6 */	"Odieresissmall",
/* 00f7 */	"OEsmall",
/* 00f8 */	"Oslashsmall",
/* 00f9 */	"Ugravesmall",
/* 00fa */	"Uacutesmall",
/* 00fb */	"Ucircumflexsmall",
/* 00fc */	"Udieresissmall",
/* 00fd */	"Yacutesmall",
/* 00fe */	"Thornsmall",
/* 00ff */	"Ydieresissmall"
};


char *AdobeStandardEncoding[] = {
/* 0000 */	".notdef",
/* 0001 */	".notdef",
/* 0002 */	".notdef",
/* 0003 */	".notdef",
/* 0004 */	".notdef",
/* 0005 */	".notdef",
/* 0006 */	".notdef",
/* 0007 */	".notdef",
/* 0008 */	".notdef",
/* 0009 */	".notdef",
/* 000a */	".notdef",
/* 000b */	".notdef",
/* 000c */	".notdef",
/* 000d */	".notdef",
/* 000e */	".notdef",
/* 000f */	".notdef",
/* 0010 */	".notdef",
/* 0011 */	".notdef",
/* 0012 */	".notdef",
/* 0013 */	".notdef",
/* 0014 */	".notdef",
/* 0015 */	".notdef",
/* 0016 */	".notdef",
/* 0017 */	".notdef",
/* 0018 */	".notdef",
/* 0019 */	".notdef",
/* 001a */	".notdef",
/* 001b */	".notdef",
/* 001c */	".notdef",
/* 001d */	".notdef",
/* 001e */	".notdef",
/* 001f */	".notdef",
/* 0020 */	"space",
/* 0021 */	"exclam",
/* 0022 */	"quotedbl",
/* 0023 */	"numbersign",
/* 0024 */	"dollar",
/* 0025 */	"percent",
/* 0026 */	"ampersand",
/* 0027 */	"quoteright",
/* 0028 */	"parenleft",
/* 0029 */	"parenright",
/* 002a */	"asterisk",
/* 002b */	"plus",
/* 002c */	"comma",
/* 002d */	"hyphen",
/* 002e */	"period",
/* 002f */	"slash",
/* 0030 */	"zero",
/* 0031 */	"one",
/* 0032 */	"two",
/* 0033 */	"three",
/* 0034 */	"four",
/* 0035 */	"five",
/* 0036 */	"six",
/* 0037 */	"seven",
/* 0038 */	"eight",
/* 0039 */	"nine",
/* 003a */	"colon",
/* 003b */	"semicolon",
/* 003c */	"less",
/* 003d */	"equal",
/* 003e */	"greater",
/* 003f */	"question",
/* 0040 */	"at",
/* 0041 */	"A",
/* 0042 */	"B",
/* 0043 */	"C",
/* 0044 */	"D",
/* 0045 */	"E",
/* 0046 */	"F",
/* 0047 */	"G",
/* 0048 */	"H",
/* 0049 */	"I",
/* 004a */	"J",
/* 004b */	"K",
/* 004c */	"L",
/* 004d */	"M",
/* 004e */	"N",
/* 004f */	"O",
/* 0050 */	"P",
/* 0051 */	"Q",
/* 0052 */	"R",
/* 0053 */	"S",
/* 0054 */	"T",
/* 0055 */	"U",
/* 0056 */	"V",
/* 0057 */	"W",
/* 0058 */	"X",
/* 0059 */	"Y",
/* 005a */	"Z",
/* 005b */	"bracketleft",
/* 005c */	"backslash",
/* 005d */	"bracketright",
/* 005e */	"asciicircum",
/* 005f */	"underscore",
/* 0060 */	"quoteleft",
/* 0061 */	"a",
/* 0062 */	"b",
/* 0063 */	"c",
/* 0064 */	"d",
/* 0065 */	"e",
/* 0066 */	"f",
/* 0067 */	"g",
/* 0068 */	"h",
/* 0069 */	"i",
/* 006a */	"j",
/* 006b */	"k",
/* 006c */	"l",
/* 006d */	"m",
/* 006e */	"n",
/* 006f */	"o",
/* 0070 */	"p",
/* 0071 */	"q",
/* 0072 */	"r",
/* 0073 */	"s",
/* 0074 */	"t",
/* 0075 */	"u",
/* 0076 */	"v",
/* 0077 */	"w",
/* 0078 */	"x",
/* 0079 */	"y",
/* 007a */	"z",
/* 007b */	"braceleft",
/* 007c */	"bar",
/* 007d */	"braceright",
/* 007e */	"asciitilde",
/* 007f */	".notdef",
/* 0080 */	".notdef",
/* 0081 */	".notdef",
/* 0082 */	".notdef",
/* 0083 */	".notdef",
/* 0084 */	".notdef",
/* 0085 */	".notdef",
/* 0086 */	".notdef",
/* 0087 */	".notdef",
/* 0088 */	".notdef",
/* 0089 */	".notdef",
/* 008a */	".notdef",
/* 008b */	".notdef",
/* 008c */	".notdef",
/* 008d */	".notdef",
/* 008e */	".notdef",
/* 008f */	".notdef",
/* 0090 */	".notdef",
/* 0091 */	".notdef",
/* 0092 */	".notdef",
/* 0093 */	".notdef",
/* 0094 */	".notdef",
/* 0095 */	".notdef",
/* 0096 */	".notdef",
/* 0097 */	".notdef",
/* 0098 */	".notdef",
/* 0099 */	".notdef",
/* 009a */	".notdef",
/* 009b */	".notdef",
/* 009c */	".notdef",
/* 009d */	".notdef",
/* 009e */	".notdef",
/* 009f */	".notdef",
/* 00a0 */	".notdef",
/* 00a1 */	"exclamdown",
/* 00a2 */	"cent",
/* 00a3 */	"sterling",
/* 00a4 */	"fraction",
/* 00a5 */	"yen",
/* 00a6 */	"florin",
/* 00a7 */	"section",
/* 00a8 */	"currency",
/* 00a9 */	"quotesingle",
/* 00aa */	"quotedblleft",
/* 00ab */	"guillemotleft",
/* 00ac */	"guilsinglleft",
/* 00ad */	"guilsinglright",
/* 00ae */	"fi",
/* 00af */	"fl",
/* 00b0 */	".notdef",
/* 00b1 */	"endash",
/* 00b2 */	"dagger",
/* 00b3 */	"daggerdbl",
/* 00b4 */	"periodcentered",
/* 00b5 */	".notdef",
/* 00b6 */	"paragraph",
/* 00b7 */	"bullet",
/* 00b8 */	"quotesinglbase",
/* 00b9 */	"quotedblbase",
/* 00ba */	"quotedblright",
/* 00bb */	"guillemotright",
/* 00bc */	"ellipsis",
/* 00bd */	"perthousand",
/* 00be */	".notdef",
/* 00bf */	"questiondown",
/* 00c0 */	".notdef",
/* 00c1 */	"grave",
/* 00c2 */	"acute",
/* 00c3 */	"circumflex",
/* 00c4 */	"tilde",
/* 00c5 */	"macron",
/* 00c6 */	"breve",
/* 00c7 */	"dotaccent",
/* 00c8 */	"dieresis",
/* 00c9 */	".notdef",
/* 00ca */	"ring",
/* 00cb */	"cedilla",
/* 00cc */	".notdef",
/* 00cd */	"hungarumlaut",
/* 00ce */	"ogonek",
/* 00cf */	"caron",
/* 00d0 */	"emdash",
/* 00d1 */	".notdef",
/* 00d2 */	".notdef",
/* 00d3 */	".notdef",
/* 00d4 */	".notdef",
/* 00d5 */	".notdef",
/* 00d6 */	".notdef",
/* 00d7 */	".notdef",
/* 00d8 */	".notdef",
/* 00d9 */	".notdef",
/* 00da */	".notdef",
/* 00db */	".notdef",
/* 00dc */	".notdef",
/* 00dd */	".notdef",
/* 00de */	".notdef",
/* 00df */	".notdef",
/* 00e0 */	".notdef",
/* 00e1 */	"AE",
/* 00e2 */	".notdef",
/* 00e3 */	"ordfeminine",
/* 00e4 */	".notdef",
/* 00e5 */	".notdef",
/* 00e6 */	".notdef",
/* 00e7 */	".notdef",
/* 00e8 */	"Lslash",
/* 00e9 */	"Oslash",
/* 00ea */	"OE",
/* 00eb */	"ordmasculine",
/* 00ec */	".notdef",
/* 00ed */	".notdef",
/* 00ee */	".notdef",
/* 00ef */	".notdef",
/* 00f0 */	".notdef",
/* 00f1 */	"ae",
/* 00f2 */	".notdef",
/* 00f3 */	".notdef",
/* 00f4 */	".notdef",
/* 00f5 */	"dotlessi",
/* 00f6 */	".notdef",
/* 00f7 */	".notdef",
/* 00f8 */	"lslash",
/* 00f9 */	"oslash",
/* 00fa */	"oe",
/* 00fb */	"germandbls",
/* 00fc */	".notdef",
/* 00fd */	".notdef",
/* 00fe */	".notdef",
/* 00ff */	".notdef"
};

const char *ttfstandardnames[258] = { 
".notdef", ".null", "nonmarkingreturn", "space", "exclam", "quotedbl",
"numbersign", "dollar", "percent", "ampersand", "quotesingle",
"parenleft", "parenright", "asterisk", "plus", "comma", "hyphen",
"period", "slash", "zero", "one", "two", "three", "four", "five",
"six", "seven", "eight", "nine", "colon", "semicolon", "less",
"equal", "greater", "question", "at", "A", "B", "C", "D", "E", "F",
"G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
"U", "V", "W", "X", "Y", "Z", "bracketleft", "backslash",
"bracketright", "asciicircum", "underscore", "grave", "a", "b", "c",
"d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q",
"r", "s", "t", "u", "v", "w", "x", "y", "z", "braceleft", "bar",
"braceright", "asciitilde", "Adieresis", "Aring", "Ccedilla",
"Eacute", "Ntilde", "Odieresis", "Udieresis", "aacute", "agrave",
"acircumflex", "adieresis", "atilde", "aring", "ccedilla", "eacute",
"egrave", "ecircumflex", "edieresis", "iacute", "igrave",
"icircumflex", "idieresis", "ntilde", "oacute", "ograve",
"ocircumflex", "odieresis", "otilde", "uacute", "ugrave",
"ucircumflex", "udieresis", "dagger", "degree", "cent", "sterling",
"section", "bullet", "paragraph", "germandbls", "registered",
"copyright", "trademark", "acute", "dieresis", "notequal", "AE",
"Oslash", "infinity", "plusminus", "lessequal", "greaterequal", "yen",
"mu", "partialdiff", "summation", "product", "pi", "integral",
"ordfeminine", "ordmasculine", "Omega", "ae", "oslash",
"questiondown", "exclamdown", "logicalnot", "radical", "florin",
"approxequal", "Delta", "guillemotleft", "guillemotright", "ellipsis",
"nonbreakingspace", "Agrave", "Atilde", "Otilde", "OE", "oe",
"endash", "emdash", "quotedblleft", "quotedblright", "quoteleft",
"quoteright", "divide", "lozenge", "ydieresis", "Ydieresis",
"fraction", "currency", "guilsinglleft", "guilsinglright", "fi", "fl",
"daggerdbl", "periodcentered", "quotesinglbase", "quotedblbase",
"perthousand", "Acircumflex", "Ecircumflex", "Aacute", "Edieresis",
"Egrave", "Iacute", "Icircumflex", "Idieresis", "Igrave", "Oacute",
"Ocircumflex", "apple", "Ograve", "Uacute", "Ucircumflex", "Ugrave",
"dotlessi", "circumflex", "tilde", "macron", "breve", "dotaccent",
"ring", "cedilla", "hungarumlaut", "ogonek", "caron", "Lslash",
"lslash", "Scaron", "scaron", "Zcaron", "zcaron", "brokenbar", "Eth",
"eth", "Yacute", "yacute", "Thorn", "thorn", "minus", "multiply",
"onesuperior", "twosuperior", "threesuperior", "onehalf",
"onequarter", "threequarters", "franc", "Gbreve", "gbreve",
"Idotaccent", "Scedilla", "scedilla", "Cacute", "cacute", "Ccaron",
"ccaron", "dcroat" };

/* some encoding stuff */
static int32 tex_base_encoding[] = {
    0x0000, 0x02d9, 0xfb01, 0xfb02, 0x2044, 0x02dd, 0x0141, 0x0142,
    0x02db, 0x02da, 0x000a, 0x02d8, 0x2212, 0x000d, 0x017d, 0x017e,
    0x02c7, 0x0131, 0xf6be, 0xfb00, 0xfb03, 0xfb04, 0x2260, 0x221e,
    0x2264, 0x2265, 0x2202, 0x2211, 0x220f, 0x03c0, 0x0060, 0x0027,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x2019,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x2018, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
    0x20ac, 0x222b, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x2126, 0x221a, 0x2248,
    0x0090, 0x0091, 0x0092, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x2206, 0x25ca, 0x0178,
    0x0000, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x002d, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};

int32 unicode_from_adobestd[256];


static Encoding texbase = { "TeX-Base-Encoding", 256, tex_base_encoding, NULL, NULL, 1, 1, 1, 1 };
       Encoding custom = { "Custom", 0, NULL, NULL, &texbase,			  1, 1, 0, 0, 0, 0, 0, 1, 0, 0 };
static Encoding original = { "Original", 0, NULL, NULL, &custom,		  1, 1, 0, 0, 0, 0, 0, 0, 1, 0 };
static Encoding unicodebmp = { "UnicodeBmp", 65536, NULL, NULL, &original, 	  1, 1, 0, 0, 1, 1, 0, 0, 0, 0 };
static Encoding unicodefull = { "UnicodeFull", 17*65536, NULL, NULL, &unicodebmp, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0 };
static Encoding adobestd = { "AdobeStandard", 256, unicode_from_adobestd, AdobeStandardEncoding, &unicodefull,
										  1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
static Encoding symbol = { "Symbol", 256, unicode_from_MacSymbol, NULL, &adobestd,1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };

Encoding *enclist = &symbol;


const char *FindUCS2Name(void) {
  return( "UCS-2");
}


static int TryEscape( Encoding *enc,char *escape_sequence ) {
    char from[20], ucs2[20];
    size_t fromlen, tolen;
    ICONV_CONST char *fpt;
    char *upt;
    int i, j, low;
    int esc_len = strlen(escape_sequence);

    strcpy(from,escape_sequence);

    enc->has_2byte = false;
    low = -1;
    for ( i=0; i<256; ++i ) if ( i!=escape_sequence[0] ) {
	for ( j=0; j<256; ++j ) {
	    from[esc_len] = i; from[esc_len+1] = j; from[esc_len+2] = 0;
	    fromlen = esc_len+2;
	    fpt = from;
	    upt = ucs2;
	    tolen = sizeof(ucs2);
	    if ( iconv( enc->tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1) &&
		    upt-ucs2==2 /* Exactly one character */ ) {
		if ( low==-1 ) {
		    enc->low_page = low = i;
		    enc->has_2byte = true;
		}
		enc->high_page = i;
	break;
	    }
	}
    }
    if ( enc->low_page==enc->high_page )
	enc->has_2byte = false;
    if ( enc->has_2byte ) {
	strcpy(enc->iso_2022_escape, escape_sequence);
	enc->iso_2022_escape_len = esc_len;
    }
	return( enc->has_2byte );
}


Encoding *_FindOrMakeEncoding(const char *name,int make_it) {
    Encoding *enc;
    char buffer[20];
    const char *iconv_name;
    Encoding temp;
    uint8 good[256];
    int i, j, any, all;
    char from[8], ucs2[20];
    size_t fromlen, tolen;
    ICONV_CONST char *fpt;
    char *upt;
    /* iconv is not case sensitive */

    if ( strncasecmp(name,"iso8859_",8)==0 || strncasecmp(name,"koi8_",5)==0 ) {
	  /* Fixup for old naming conventions */
	  strncpy(buffer,name,sizeof(buffer));
	  *strchr(buffer,'_') = '-';
	  name = buffer;
    } else if ( strcasecmp(name,"iso-8859")==0 ) {
	/* Fixup for old naming conventions */
	  strncpy(buffer,name,3);
	  strncpy(buffer+3,name+4,sizeof(buffer)-3);
	  name = buffer;
    } else if ( strcasecmp(name,"isolatin1")==0 ) {
	  name = "iso8859-1";
    } else if ( strcasecmp(name,"isocyrillic")==0 ) {
	  name = "iso8859-5";
    } else if ( strcasecmp(name,"isoarabic")==0 ) {
	  name = "iso8859-6";
    } else if ( strcasecmp(name,"isogreek")==0 ) {
	  name = "iso8859-7";
    } else if ( strcasecmp(name,"isohebrew")==0 ) {
	  name = "iso8859-8";
    } else if ( strcasecmp(name,"isothai")==0 ) {
	  name = "tis-620";	/* TIS doesn't define non-breaking space in 0xA0 */ 
    } else if ( strcasecmp(name,"latin0")==0 || strcasecmp(name,"latin9")==0 ) {
	  name = "iso8859-15";	/* "latin-9" is supported (libiconv bug?) */ 
    } else if ( strcasecmp(name,"koi8r")==0 ) {
	  name = "koi8-r";
    } else if ( strncasecmp(name,"jis201",6)==0 || strncasecmp(name,"jisx0201",8)==0 ) {
	  name = "jis_x0201";
    } else if ( strcasecmp(name,"AdobeStandardEncoding")==0 || strcasecmp(name,"Adobe")==0 )
	  name = "AdobeStandard";
    for ( enc=enclist; enc!=NULL; enc=enc->next )
	  if ( strmatch(name,enc->enc_name)==0 ||
		   (enc->iconv_name!=NULL && strmatch(name,enc->iconv_name)==0))
		return( enc );
    if ( strmatch(name,"unicode")==0 || strmatch(name,"iso10646")==0 || strmatch(name,"iso10646-1")==0 )
	  return( &unicodebmp );
    if ( strmatch(name,"unicode4")==0 || strmatch(name,"ucs4")==0 )
	  return( &unicodefull );
	
    iconv_name = name;
    /* Mac seems to work ok */
    if ( strcasecmp(name,"win")==0 || strcasecmp(name,"ansi")==0 )
	  iconv_name = "MS-ANSI";		/* "WINDOWS-1252";*/
    else if ( strncasecmp(name,"jis208",6)==0 || strncasecmp(name,"jisx0208",8)==0 )
	  iconv_name = "ISO-2022-JP";
    else if ( strncasecmp(name,"jis212",6)==0 || strncasecmp(name,"jisx0212",8)==0 )
	  iconv_name = "ISO-2022-JP-2";
    else if ( strncasecmp(name,"ksc5601",7)==0 )
	  iconv_name = "ISO-2022-KR";
    else if ( strcasecmp(name,"gb2312pk")==0 || strcasecmp(name,"gb2312packed")==0 )
	  iconv_name = "EUC-CN";
    else if ( strncasecmp(name,"gb2312",6)==0 )
	  iconv_name = "ISO-2022-CN";
    else if ( strcasecmp(name,"wansung")==0 )
	  iconv_name = "EUC-KR";
    else if ( strcasecmp(name,"EUC-CN")==0 ) {
	  iconv_name = name;
	  name = "gb2312pk";
    } else if ( strcasecmp(name,"EUC-KR")==0 ) {
	  iconv_name = name;
	  name = "wansung";
    }
	
	/* Escape sequences:					*/
	/*	ISO-2022-CN:     \e $ ) A ^N			*/
	/*	ISO-2022-KR:     \e $ ) C ^N			*/
	/*	ISO-2022-JP:     \e $ B				*/
	/*	ISO-2022-JP-2:   \e $ ( D			*/
	/*	ISO-2022-JP-3:   \e $ ( O			*/ /* Capital "O", not zero */
	/*	ISO-2022-CN-EXT: \e $ ) E ^N			*/ /* Not sure about this, also uses CN escape */

    memset(&temp,0,sizeof(temp));
    temp.builtin = true;
    temp.tounicode = iconv_open(FindUCS2Name(),iconv_name);
    if ( temp.tounicode==(iconv_t) -1 || temp.tounicode==NULL )
	  return( NULL );			/* Iconv doesn't recognize this name */
    temp.fromunicode = iconv_open(iconv_name,FindUCS2Name());
    if ( temp.fromunicode==(iconv_t) -1 || temp.fromunicode==NULL ) {
	  /* This should never happen, but if it does... */
	  iconv_close(temp.tounicode);
	  return( NULL );
    }

    memset(good,0,sizeof(good));
    any = false; all = true;
    for ( i=1; i<256; ++i ) {
	from[0] = i; from[1] = 0;
	fromlen = 1;
	fpt = from;
	upt = ucs2;
	tolen = sizeof(ucs2);
	if ( iconv( temp.tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1)) {
	  good[i] = true;
	  any = true;
	} else
	  all = false;
    }
    if ( any )
	  temp.has_1byte = true;
    if ( all )
	  temp.only_1byte = true;

    if ( !all ) {
	if ( strstr(iconv_name,"2022")==NULL ) {
	  for ( i=temp.has_1byte; i<256; ++i ) if ( !good[i] ) {
		for ( j=0; j<256; ++j ) {
		  from[0] = i; from[1] = j; from[2] = 0;
		  fromlen = 2;
		  fpt = from;
		  upt = ucs2;
		  tolen = sizeof(ucs2);
		  if ( iconv( temp.tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1) &&
			   upt-ucs2==2 /* Exactly one character */ ) {
			if ( temp.low_page==-1 )
			  temp.low_page = i;
			temp.high_page = i;
			temp.has_2byte = true;
			break;
		  }
		}
	  }
	  if ( temp.low_page==temp.high_page ) {
		temp.has_2byte = false;
		temp.low_page = temp.high_page = -1;
	  }
	}
	if ( !temp.has_2byte && !good[033]/* escape */ ) {
	    if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP3")!=NULL &&
		    TryEscape( &temp,"\33$(O" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP2")!=NULL &&
		    TryEscape( &temp,"\33$(D" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP")!=NULL &&
		    TryEscape( &temp,"\33$B" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"KR")!=NULL &&
		    TryEscape( &temp,"\33$)C\16" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"CN")!=NULL &&
		    TryEscape( &temp,"\33$)A\16" ))
		;
	}
    }
    if ( !temp.has_1byte && !temp.has_2byte )
	  return( NULL );
    if ( !make_it )
	  return( NULL );
	
    enc = chunkalloc(sizeof(Encoding));
    *enc = temp;
    enc->enc_name = copy(name);
    if ( iconv_name!=name )
	enc->iconv_name = copy(iconv_name);
    enc->next = enclist;
    enc->builtin = true;
    enclist = enc;
    if ( enc->has_2byte )
	enc->char_cnt = (enc->high_page<<8) + 256;
    else {
	enc->char_cnt = 256;
	enc->only_1byte = true;
    }
    if ( strstrmatch(iconv_name,"JP")!=NULL ||
	    strstrmatch(iconv_name,"sjis")!=NULL ||
	    strstrmatch(iconv_name,"cp932")!=NULL )
	enc->is_japanese = true;
    else if ( strstrmatch(iconv_name,"KR")!=NULL )
	enc->is_korean = true;
    else if ( strstrmatch(iconv_name,"CN")!=NULL )
	enc->is_simplechinese = true;
    else if ( strstrmatch(iconv_name,"BIG")!=NULL && strstrmatch(iconv_name,"5")!=NULL )
	enc->is_tradchinese = true;

    if ( strstrmatch(name,"ISO8859")!=NULL &&
	    strtol(name+strlen(name)-2,NULL,10)>=16 )
	/* Not in our menu, don't hide */;
    else if ( iconv_name!=name || strmatch(name,"mac")==0 || strstrmatch(name,"ISO8859")!=NULL ||
	    strmatch(name,"koi8-r")==0 || strmatch(name,"sjis")==0 ||
	    strmatch(name,"big5")==0 || strmatch(name,"big5hkscs")==0 )
	enc->hidden = true;

	return( enc );
}

Encoding *FindOrMakeEncoding(const char *name) {
return( _FindOrMakeEncoding(name,true));
}

int32 UniFromEnc(int enc, Encoding *encname) {
    char from[20];
    unsigned short to[20];
    ICONV_CONST char *fpt;
    char *tpt;
    size_t fromlen, tolen;

    if ( encname->is_custom || encname->is_original )
return( -1 );
    if ( enc>=encname->char_cnt )
return( -1 );
    if ( encname->is_unicodebmp || encname->is_unicodefull )
return( enc );
    if ( encname->unicode!=NULL )
return( encname->unicode[enc] );
    else if ( encname->tounicode ) {
	/* To my surprise, on RH9, doing a reset on conversion of CP1258->UCS2 */
	/* causes subsequent calls to return garbage */
	if ( encname->iso_2022_escape_len ) {
	    tolen = sizeof(to); fromlen = 0;
	    iconv(encname->tounicode,NULL,&fromlen,NULL,&tolen);	/* Reset state */
	}
	fpt = from; tpt = (char *) to; tolen = sizeof(to);
	if ( encname->has_1byte && enc<256 ) {
	    *(char *) fpt = enc;
	    fromlen = 1;
	} else if ( encname->has_2byte ) {
	    if ( encname->iso_2022_escape_len )
		strncpy(from,encname->iso_2022_escape,encname->iso_2022_escape_len );
	    fromlen = encname->iso_2022_escape_len;
	    from[fromlen++] = enc>>8;
	    from[fromlen++] = enc&0xff;
	}
	if ( iconv(encname->tounicode,&fpt,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	if ( tpt-(char *) to == 0 ) {
	    /* This strange call appears to be what we need to make CP1258->UCS2 */
	    /*  work.  It's supposed to reset the state and give us the shift */
	    /*  out. As there is no state, and no shift out I have no idea why*/
	    /*  this works, but it does. */
	    if ( iconv(encname->tounicode,NULL,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	}
	if ( tpt-(char *) to == 2 )
return( to[0] );
	else if ( tpt-(char *) to == 4 && to[0]>=0xd800 && to[0]<0xdc00 && to[1]>=0xdc00 )
return( ((to[0]-0xd800)<<10) + (to[1]-0xdc00) + 0x10000 );
    } else if ( encname->tounicode_func!=NULL ) {
return( (encname->tounicode_func)(enc) );
    }
return( -1 );
}


int32 EncFromUni(int32 uni, Encoding *enc) {
    short from[20];
    unsigned char to[20];
    ICONV_CONST char *fpt;
    char *tpt;
    size_t fromlen, tolen;
    int i;

    if ( enc->is_custom || enc->is_original || enc->is_compact )
return( -1 );
    if ( enc->is_unicodebmp || enc->is_unicodefull )
return( uni<enc->char_cnt ? uni : -1 );

    if ( enc->unicode!=NULL ) {
	for ( i=0; i<enc->char_cnt; ++i ) {
	    if ( enc->unicode[i]==uni )
return( i );
	}
return( -1 );
    } else if ( enc->fromunicode!=NULL ) {
	/* I don't see how there can be any state to reset in this direction */
	/*  So I don't reset it */
	if ( uni<0x10000 ) {
	    from[0] = uni;
	    fromlen = 2;
	} else {
	    uni -= 0x10000;
	    from[0] = 0xd800 + (uni>>10);
	    from[1] = 0xdc00 + (uni&0x3ff);
	    fromlen = 4;
	}
	fpt = (char *) from; tpt = (char *) to; tolen = sizeof(to);
	iconv(enc->fromunicode,NULL,NULL,NULL,NULL);	/* reset shift in/out, etc. */
	if ( iconv(enc->fromunicode,&fpt,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	if ( tpt-(char *) to == 1 )
return( to[0] );
	if ( enc->iso_2022_escape_len!=0 ) {
	    if ( tpt-(char *) to == enc->iso_2022_escape_len+2 &&
		    strncmp((char *) to,enc->iso_2022_escape,enc->iso_2022_escape_len)==0 )
return( (to[enc->iso_2022_escape_len]<<8) | to[enc->iso_2022_escape_len+1] );
	} else {
	    if ( tpt-(char *) to == 2 )
return( (to[0]<<8) | to[1] );
	}
    } else if ( enc->fromunicode_func!=NULL ) {
return( (enc->fromunicode_func)(uni) );
    }
return( -1 );
}

int32 EncFromName(const char *name,enum uni_interp interp,Encoding *encname) {
    int i;
    if ( encname->psnames!=NULL ) {
	for ( i=0; i<encname->char_cnt; ++i )
	    if ( encname->psnames[i]!=NULL && strcmp(name,encname->psnames[i])==0 )
return( i );
    }
    i = UniFromName(name,interp,encname);
    if ( i==-1 && strlen(name)==4 ) {
	/* MS says use this kind of name, Adobe says use the one above */
	char *end;
	i = strtol(name,&end,16);
	if ( i<0 || i>0xffff || *end!='\0' )
return( -1 );
    }
return( EncFromUni(i,encname));
}



EncMap *EncMapFromEncoding(SplineFont *sf,Encoding *enc) {
    int i,j, extras, found, base, unmax;
    int *encoded, *unencoded;
    EncMap *map;
    struct altuni *altuni;
    SplineChar *sc;

    if ( enc==NULL )
return( NULL );

    base = enc->char_cnt;
    if ( enc->is_original )
	base = 0;
    else if ( enc->char_cnt<=256 )
	base = 256;
    else if ( enc->char_cnt<=0x10000 )
	base = 0x10000;
    encoded = galloc(base*sizeof(int));
    memset(encoded,-1,base*sizeof(int));
    unencoded = galloc(sf->glyphcnt*sizeof(int));
    unmax = sf->glyphcnt;

    for ( i=extras=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	found = false;
	if ( enc->psnames!=NULL ) {
	    for ( j=enc->char_cnt-1; j>=0; --j ) {
		if ( enc->psnames[j]!=NULL &&
			strcmp(enc->psnames[j],sc->name)==0 ) {
		    found = true;
		    encoded[j] = i;
		}
	    }
	}
	if ( !found ) {
	    if ( sc->unicodeenc!=-1 &&
		     sc->unicodeenc<unicode4_size &&
		     (j = EncFromUni(sc->unicodeenc,enc))!= -1 )
		encoded[j] = i;
	    else {
		/* I don't think extras can surpass unmax now, but it doesn't */
		/*  hurt to leave the code (it's from when we encoded duplicates see below) */
		if ( extras>=unmax ) unencoded = grealloc(unencoded,(unmax+=300)*sizeof(int));
		unencoded[extras++] = i;
	    }
	    for ( altuni=sc->altuni; altuni!=NULL; altuni=altuni->next ) {
		if ( altuni->unienc!=-1 &&
			 altuni->unienc<unicode4_size &&
			 (j = EncFromUni(altuni->unienc,enc))!= -1 )
		    encoded[j] = i;
		/* I used to have code here to add these unencoded duplicates */
		/*  but I don't really see any reason to do so. The main unicode */
		/*  will occur, and any encoded duplicates so the glyph won't */
		/*  vanish */
	    }
	}
    }

    /* Some glyphs have both a pua encoding and an encoding in a non-bmp */
    /*  plane. Big5HK does and the AMS glyphs do */
    if ( enc->is_unicodefull && (sf->uni_interp == ui_trad_chinese ||
				 sf->uni_interp == ui_ams )) {
#if 0
	extern const int cns14pua[], amspua[];
#endif
	const int *pua = sf->uni_interp == ui_ams? amspua : cns14pua;
	for ( i=0xe000; i<0xf8ff; ++i ) {
	    if ( pua[i-0xe000]!=0 )
		encoded[pua[i-0xe000]] = encoded[i];
	}
    }

    if ( enc->psnames != NULL ) {
	/* Names are more important than unicode code points for some encodings */
	/*  AdobeStandard for instance which won't work if you have a glyph  */
	/*  named "f_i" (must be "fi") even though the code point is correct */
	/* The code above would match f_i where AS requires fi, so force the */
	/*  names to be correct. */
	for ( j=0; j<enc->char_cnt; ++j ) {
	    if ( encoded[j]!=-1 && enc->psnames[j]!=NULL &&
		    strcmp(sf->glyphs[encoded[j]]->name,enc->psnames[j])!=0 ) {
		free(sf->glyphs[encoded[j]]->name);
		sf->glyphs[encoded[j]]->name = copy(enc->psnames[j]);
	    }
	}
    }

    map = chunkalloc(sizeof(EncMap));
    map->enccount = map->encmax = base + extras;
    map->map = galloc(map->enccount*sizeof(int));
    memcpy(map->map,encoded,base*sizeof(int));
    memcpy(map->map+base,unencoded,extras*sizeof(int));
    map->backmax = sf->glyphcnt;
    map->backmap = galloc(sf->glyphcnt*sizeof(int));
    memset(map->backmap,-1,sf->glyphcnt*sizeof(int));	/* Just in case there are some unencoded glyphs (duplicates perhaps) */
    for ( i = map->enccount-1; i>=0; --i ) if ( map->map[i]!=-1 )
	map->backmap[map->map[i]] = i;
    map->enc = enc;

    free(encoded);
    free(unencoded);

return( map );
}


EncMap *EncMapNew(int enccount,int backmax,Encoding *enc) {
    EncMap *map = chunkalloc(sizeof(EncMap));

    map->enccount = map->encmax = enccount;
    map->backmax = backmax;
    map->map = galloc(enccount*sizeof(int));
    memset(map->map,-1,enccount*sizeof(int));
    map->backmap = galloc(backmax*sizeof(int));
    memset(map->backmap,-1,backmax*sizeof(int));
    map->enc = enc;
return(map);
}

static struct cidmap *MakeDummyMap(char *registry,char *ordering,int supplement) {
  struct cidmap *ret = galloc(sizeof(struct cidmap));

  ret->registry = copy(registry);
  ret->ordering = copy(ordering);
  ret->supplement = ret->maxsupple = supplement;
  ret->cidmax = ret->namemax = 0;
  ret->unicode = NULL; 
  ret->name = NULL;
  ret->next = cidmaps;
  cidmaps = ret;
  return( ret );
}


struct cidmap *FindCidMap(char *registry,char *ordering,int supplement,SplineFont *sf) {
  return( MakeDummyMap(registry,ordering,supplement));
}


int NameUni2CID(struct cidmap *map,int uni, const char *name) {
    int i;

    if ( map==NULL )
return( -1 );
    if ( uni!=-1 ) {
        for ( i=0; i<map->namemax; ++i )
            if ( map->unicode[i]==uni )
return( i );
    } else {
        for ( i=0; i<map->namemax; ++i )
            if ( map->name[i]!=NULL && strcmp(map->name[i],name)==0 )
return( i );
    }
return( -1 );
}


void initadobeenc(void) {
  int i,j;

  for ( i=0; i<0x100; ++i ) {
	if ( strcmp(AdobeStandardEncoding[i],".notdef")==0 )
	  unicode_from_adobestd[i] = 0xfffd;
	else {
	  j = UniFromName(AdobeStandardEncoding[i],ui_none,&custom);
	  if ( j==-1 ) j = 0xfffd;
	  unicode_from_adobestd[i] = j;
	}
  }
}

