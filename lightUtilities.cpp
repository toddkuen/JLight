#include "stdafx.h"

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  10-02-2007 - TRK - Created.
//
//

#ifdef __PORTABLE
#include "Support/pdfXPortability.h"
#endif // __PORTABLE

#include <stdio.h>
#include <ctype.h>

#include "lightUtilities.h"

FILE * errOut = NULL;
FILE * fStdOut = stdout;
FILE * fStdErr = stderr;

static FILE * loggingFile = NULL;
FILE * loggingConsole = stderr;

static bool copyToStdError = false;

static int minConsoleLogLevel = lightLOG_INFO;

static const char * errorTypes[] = {
  "DEBUG",
  "INFO",
  "STATUS",
  "WARNING",
  "ERROR",
  "FATAL",
  "INTERNAL ERROR",
  "PANIC"
};

void logTo(FILE * logFile, int minConsoleLevel, FILE * console, bool errorsToStdErr /* = true */) {
  minConsoleLogLevel = minConsoleLevel;
  loggingFile = logFile;
  loggingConsole = console;
  copyToStdError = errorsToStdErr;
}

void addToLogFile(int msgType, pdfXString message) {
  pdfXString error;
  extern LPCTSTR appName(void);
  
  error.Format("%s: %s: %s\n", appName(), errorTypes[(msgType & lightLOG_LOGONLYMASK)], (LPCTSTR)message);
  
  if (loggingFile != NULL) {
    fputs((LPCTSTR)error, loggingFile);
    fflush(loggingFile);
  }
  
  if (loggingConsole != fStdErr && msgType >= lightLOG_ERROR && copyToStdError) {
    fputs((LPCTSTR)error, fStdErr);
    fflush(fStdErr);
  }
  
  if ((msgType & lightLOG_LOGONLY) != 0) {
    return;
  }
  
  if (loggingConsole && (msgType & lightLOG_LOGONLYMASK) >= minConsoleLogLevel) {
    if ((loggingConsole != fStdErr && loggingConsole != fStdOut) || !(msgType >= lightLOG_ERROR && copyToStdError)) {
      fputs((LPCTSTR)error, loggingConsole);
      fflush(loggingConsole);
    }
  }
}

void fatalError(int msgType, pdfXString message) {
  addToLogFile(msgType, message);
  exit(1);
}

//////////////////////////////////
// begin pdfMStitcherParserC - support

const int MAX_SOURCE_FILE_LIST_LENGTH = 1000;

int advanceChar(LPCTSTR& ptr) {
  int result;
  
  result = *ptr & 0xFF;
  ptr += 1;
  return result;
}

bool parseMatchChar(LPCTSTR& ptr, char matchChar) {
  if (*ptr == matchChar) {
    ptr++;
    return true;
  }
  return false;
}

typedef bool (*parseMatchCharComparitor)(char c);

static bool AlnumComparitor(char c) {
	return isalnum(c)? true : false;
}

static bool IntegerDigitsComparitor(char c) {
	return isdigit(c)? true : false;
}

int isHexDigit(char c) 
{
	if (isdigit(c))
  {
    return c - '0';
  }
  else if (c >= 'A' && c <= 'F')
  {
    return (c - 'A') + 10;
  }
  else if (c >= 'a' && c <= 'f')
  {
    return (c - 'a') + 10;
  }
  else
  {
    return -1;
  }
}

int hextoi(LPCTSTR cp)
{
  int val;
  
  while (*cp != '\0')
  {
    int dig;
    
    dig = isHexDigit(*cp);
    if (dig == -1)
    {
      break;
    }
    val = (val << 4) | (0xF & dig); 
    cp++;
  }
  
  return val;
}

static bool HexDigitsComparitor(char c) {
	return isHexDigit(c) != -1? true : false;
}

static bool FileNameComparitor(char c) {
  switch (c) {
#ifdef GCC_UNIX
#ifdef MAC_OSX
    case ':':
#else
    case 01:
#endif // MAC_OSX
#else
    case '"':
    case '*':
    case '?':
    case '<':
    case '>':
    case '|':
#endif // GCC_UNIX
      return false;
    default:
      return true;
  }
}

bool parseComparitorValue(LPCTSTR& ptr, parseMatchCharComparitor f, pdfXString& cmdName, LPCTSTR stopChars = NULL) {
  cmdName = "";
  
  while ((*f)(*ptr) && *ptr != '\0') {
    
    if (stopChars != NULL) {
      int i;
      
      for (i = 0; stopChars[i] != '\0'; i++) {
        if (*ptr == stopChars[i]) {
          return true;
        }
      }
    }
    
    cmdName += (TCHAR)*ptr;
    ptr++;
  }
  
  if (cmdName.GetLength() == 0) {
    return false;
  }
  
  return true;
}

bool parseFileNameValue(LPCTSTR& ptr, pdfXString& fileName, pdfXString& error, LPCTSTR stopChars /* = NULL */) {
  char initialQuote;
  
  fileName = "";
  error = "";
  
  if (*ptr == '"' || *ptr == '\'') {
    initialQuote = *ptr;
    ptr++;
  } else {
    initialQuote = '\0';
  }
  
  while (*ptr != '\0' && *ptr != initialQuote) {
    
    if (stopChars != NULL) {
      int i;
      
      for (i = 0; stopChars[i] != '\0'; i++) {
        if (*ptr == stopChars[i]) {
          goto done;
        }
      }
    }
    
#ifdef GCC_UNIX
    if (*ptr == '\\') {
      ptr++;
      fileName += (TCHAR)*ptr;
      ptr++;
      continue;
    }
#endif // GCC_UNIX
    
    if (!(*FileNameComparitor)(*ptr)) {
      error.Format("Illegal character (%c/#%d) in file name.", *ptr & 0xFF, *ptr & 0xFF);
      return false;
    }
    
    fileName += (TCHAR)*ptr;
    
    ptr++;
  }
  
done:
  
  if (initialQuote != '\0') {
    if (*ptr != initialQuote) {
      error.Format("Missing match quote character (%c/#%d) in file name.", initialQuote & 0xFF, initialQuote & 0xFF);
      return false;
    }
    ptr++;
  }
  
  return true;
}

bool parseRegexStringValue(LPCTSTR& ptr, pdfXString& regex, pdfXString& error) {
  char initialQuote;
  
  regex = "";
  error = "";
  
  if (*ptr == '"' || *ptr == '\'') {
    initialQuote = *ptr;
    ptr++;
  } else {
    initialQuote = '\0';
  }
  
  while (*ptr != '\0' && *ptr != initialQuote) {
    regex += (TCHAR)*ptr;
    ptr++;
  }
  
  if (initialQuote != '\0') {
    if (*ptr != initialQuote) {
      error.Format("Missing match quote character (%c/#%d) in regex string.", initialQuote & 0xFF, initialQuote & 0xFF);
      return false;
    }
    ptr++;
  }
  
  return true;
}

bool parseIntegerValue(LPCTSTR& ptr, int& val, LPCTSTR stopChars /* = NULL */) {
  bool neg;
  pdfXString n;
  
  neg = false;
  val = 0;
  if (*ptr == '-') {
    neg = true;
    ptr++;
  }
  
  if (!parseComparitorValue(ptr, IntegerDigitsComparitor, n, stopChars)) {
    return false;
  }
  
  val = atoi((LPCTSTR)n);
  return true;
}

bool parseDoubleValue(LPCTSTR& ptr, double& val, LPCTSTR stopChars /* = NULL */) {
  bool neg;
  pdfXString before, after;
  pdfXString stops;
  
  if (stopChars == NULL) {
    stops = ".";
  } else {
    stops = pdfXString(stopChars) + ".";
  }
  
  neg = false;
  val = 0;
  if (*ptr == '-') {
    neg = true;
    ptr++;
  }
  
  if (*ptr != '.') {
    if (!parseComparitorValue(ptr, IntegerDigitsComparitor, before, (LPCTSTR)stops)) {
      return false;
    }
  } else {
    before = "0";
  }
  
  if (*ptr == '.') {
    after = "";
    ptr++;
    if (!parseComparitorValue(ptr, IntegerDigitsComparitor, after, stopChars)) {
      return false;
    }
  }
  
  val = (neg? -1.0 : 1.0) * atof((LPCTSTR)(before + "." + after));
  return true;
}

bool parseHexValue(LPCTSTR& ptr, int& val, LPCTSTR stopChars /* = NULL */) {
  bool neg;
  pdfXString stops, hex;

  neg = false;
  val = 0;
  if (*ptr == '-') {
    neg = true;
    ptr++;
  }
  
  if (!parseComparitorValue(ptr, HexDigitsComparitor, hex, (LPCTSTR)stops)) {
    return false;
  }
  
  val = (int)((neg? -1.0 : 1.0) * hextoi((LPCTSTR)(hex)));
  return true;
}

bool advanceToNext(LPCTSTR& ptr, LPCTSTR skipping, LPCTSTR match) {
  int i;
  
  if (*ptr == '\0') {
    return false;
  }
  
  for (;;) {
  restart:
    
    if (skipping != NULL) {
      for (i = 0; skipping[i] != '\0'; i++) {
        if (*ptr == skipping[i]) {
          ptr++;
          goto restart;
        }
      }
    }
    
    if (match == NULL || match[0] == '\0') {
      // Just skip...
      return true;
    }
    
    for (i = 0; match[i] != '\0'; i++) {
      if (*ptr == match[i]) {
        ptr++;
        return true;
      }
    }
    
    return false;
  }
}

bool lookingAt(LPCTSTR& ptr, LPCTSTR someStr, bool autoAdvanceOnMatch /* = false */) {
  int i;
  bool result;
  LPCTSTR save;
  
  save = ptr;
  
  if (*ptr == '\0') {
    return false;
  }
  
  i = 0;
  while ((isupper(ptr[i])? tolower(ptr[i]) : ptr[i]) == someStr[i] && someStr[i] != '\0') {
    i++;
  }
  
  result = (someStr[i] == '\0');
  if (autoAdvanceOnMatch) {
    ptr += i;
  }
  
  if (!result) {
    ptr = save;
  }
  
  return result;
}

bool lookingAtWithCase(LPCTSTR& ptr, LPCTSTR someStr, bool autoAdvanceOnMatch /* = false */) {
  int i;
  bool result;
  LPCTSTR save;
  
  save = ptr;
  
  if (*ptr == '\0') {
    return false;
  }
  
  i = 0;
  while (ptr[i] == someStr[i] && someStr[i] != '\0') {
    i++;
  }
  
  result = (someStr[i] == '\0');
  if (autoAdvanceOnMatch) {
    ptr += i;
  }
  
  if (!result) {
    ptr = save;
  }
  
  return result;
}

// begin pdfMStitcherParserC - support
//////////////////////////////////

