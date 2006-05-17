#$Id: syntaxform.awk,v 1.13 2006/02/10 21:27:48 hhenkel Exp $
# pdftex-t.tex --> pdftex-syntax.txt
# requires gawk due to gensub() function

BEGIN{
  buffer = "";
}

/\015*$/ {
  gsub(/\015*$/, "");
}

/^%%S NL/ {
  print "";
  next;
}

/^%%S/ {
  gsub (/^%%S/, "%%" );
  print;
  next;
}

/^\\Syntax/ {
  printing = 1;
  indentation = 0;
  next;
}

// {
  if (buffer != "") {
    gsub(/^ */, "");
    $0 = buffer $0;
  }
  buffer = "";
}

/\|\|/ {
  gsub(/\|\|/, "-");
}

/^  */{
  gsub(/^  */, indspaces);
}

/\\Something/ {
  $0 = gensub(/\\Something *{([^}]*)}/, "<\\1>", "g");
}

/\\Literal/ {
  $0 = gensub(/\\Literal *{([^}]*)}/, "\\1", "g");
}

/\\Tex/ {
  $0 = gensub(/\\Tex *{([^}]*)}/, "\\1", "g");
}

/\\Optional/ {
  $0 = gensub(/\\Optional *{([^}]*)}/, "[\\1]", "g");
}

/\\Means/ {
  gsub(/\\Means/, "-->");
  indentation = match($0, /-->/);
  indspaces = " ";
  for(i=0; i < indentation; i++)
    indspaces = indspaces " ";
}

/\\Lbrace/ {
  gsub(/\\Lbrace/, "{");
}

/\\Rbrace/ {
  gsub(/\\Rbrace/, "}");
}

/\\Or/ {
  gsub(/\\Or/, "|");
}

/\\Next/ {
  gsub(/\\Next /, "");
}

/\\Whatever/ {
  whatind = 57;
  whatpos = match($0, /\\Whatever/);
  b = substr($0, 0, whatpos - 1)
  c = substr($0, whatpos)
  c = gensub(/\\Whatever *{([^}]*)}/, "(\\1)", "g", c);
  $0 = b;
  for(i = whatpos; i < whatind; i++)
    $0 = $0 " ";
  $0 = $0 c;
}

/^}/ {printing = 0;}

/% *$/ {
  gsub(/% *$/, "");
  buffer = $0;
  next;
}

/^ *$/ {
  $0 = "================= ERROR";
}

/  *$/ {
  gsub(/  *$/, "");
}

{ if (printing) print; }
