#include "stdafx.h"

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  10-07-2007 - TRK - Created.
//
//

#ifdef __PORTABLE
#include "Support/pdfXPortability.h"
#endif // __PORTABLE

#include "genericUtilities.h"
#include <ctype.h>

#ifdef MSWIN
#define TPATHSEP '\\'
#else
#define TPATHSEP '/'
#endif // MSWIN

char gpPathSep[2] = {
  TPATHSEP, '\0'
};

bool fileExists(pdfXString path) {
  FILE * f;
  
  if ((f = fopen((LPCTSTR)path, "rb")) != NULL) {
    fclose(f);
    return true;
  }
  return false;
}

void safeAttrCopy(char * dst, pdfXString src) {
  int i;
  
  for (i = 0; i < (src.GetLength() < (MAX_PARAM_LENGTH - 1)? src.GetLength() : (MAX_PARAM_LENGTH - 1)); i++) {
    dst[i] = src.GetAt(i);
  }
  dst[i] = 0;
}

void extractDouble(LPCTSTR& ptr, double& d) {
  int i;
  char buf[20];
  float x;
  
  i = 0;
  while (i < (int)sizeof(buf) - 1 && (isdigit(*ptr) || *ptr == '.' || *ptr == '-' || *ptr == '+')) {
    buf[i] = *ptr;
    i++;
    ptr++;
  }
  buf[i] = 0;
  
  sscanf(buf, "%f", &x);
  d = x;
}

void extractInteger(LPCTSTR& ptr, int& d) {
  int i;
  char buf[20];
  int x;
  
  i = 0;
  while (i < (int)sizeof(buf) - 1 && (isdigit(*ptr) || *ptr == '-' || *ptr == '+')) {
    buf[i] = *ptr;
    i++;
    ptr++;
  }
  buf[i] = 0;
  
  sscanf(buf, "%d", &x);
  d = x;
}

void extractTo(LPCTSTR& ptr, pdfXString& n, char c) {
  n = "";
  while (*ptr != '\0' && *ptr != c) {
    n += (TCHAR)*ptr;
    ptr++;
  }
}

pdfXString first(pdfXString str, int len) {
  if (str.GetLength() >= len) {
    return str.Left(len);
  } else {
    return str;
  }
}

bool skipPast(LPCTSTR& ptr, char c) {
  while (*ptr != '\0' && *ptr != c) {
    ptr++;
  }
  
  if (*ptr == c) {
    ptr++;
    return true;
  }
  
  return false;
}


void splitFilePath(pdfXString root, pdfXString& prefix, pdfXString& fileRoot, pdfXString& ext, bool adSep /* = true */) {
  int i;
  
  for (i = root.GetLength() - 1; i >= 0; i--) {
    if (root.GetAt(i) == '.' || root.GetAt(i) == '/' || root.GetAt(i) == '\\' || root.GetAt(i) == ':') {
      break;
    }
  }
  
  if (i >= 0 && root.GetAt(i) == '.') {
    ext = root.Right(root.GetLength() - (i + 1));
    root = root.Left(i); 
    i -= 1;
  } else {
    ext = "";
  }

  for (; i >= 0; i--) {
    if (root.GetAt(i) == '/' || root.GetAt(i) == '\\' || root.GetAt(i) == ':') {
      break;
    }
  }
  
  if (i >= 0) {
    fileRoot = root.Right(root.GetLength() - (i + 1));
    prefix = root.Left(i + 1);     
  } else {
    fileRoot = root;
    prefix = "";
  }
  
  if (adSep && prefix.GetLength() > 0 && prefix.GetAt(prefix.GetLength() - 1) != TPATHSEP) {
    prefix += (TCHAR)TPATHSEP;
  }
}

void appendPathSepIfNeeded(pdfXString& aPath) {
  if (aPath.GetLength() > 0 && aPath.GetAt(aPath.GetLength() - 1) != TPATHSEP) {
    aPath += (TCHAR)TPATHSEP;
  }
}


pdfXString repathToPDF(pdfXString somePath) {
  pdfXString prefix, root, ext, newRoot;
  int i;
  
  splitFilePath(somePath, prefix, root, ext, false);
  newRoot = "";
  for (i = 0; i < prefix.GetLength(); i++) {
    if (prefix.GetAt(i) == '\\' || prefix.GetAt(i) == '/') {
      newRoot += (TCHAR)gpPathSep[0];
    } else {
      newRoot += prefix.GetAt(i);
    }
  }
  
  if (prefix.GetLength() > 0 && prefix.GetAt(prefix.GetLength() - 1) != '\\' && prefix.GetAt(prefix.GetLength() - 1) != '/') {
      newRoot += (TCHAR)gpPathSep[0];
  }
  
  newRoot += root + ".pdf";
  
  return newRoot;
}
