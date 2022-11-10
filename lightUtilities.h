#ifndef lightUtilities_H_
#define lightUtilities_H_

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  10-02-2007 - TRK - Created.
//
//

extern FILE * errOut;
extern FILE * fStdOut;
extern FILE * fStdErr;

enum {
  lightLOG_DEBUG = 0,
  lightLOG_INFO = 1,
  lightLOG_STATUS = 2,
  lightLOG_WARNING = 3,
  lightLOG_ERROR = 4,
  lightLOG_FATAL = 5,
  lightLOG_INTERNAL_ERROR = 6,
  lightLOG_PANIC = 7,
  lightLOG_LOGONLY = 0x800,
  lightLOG_LOGONLYMASK = 0x0F
};

extern void logTo(FILE * logFile, int minConsoleLevel, FILE * console, bool errorsToStdErr = true);

extern void addToLogFile(int msgType, pdfXString message);

extern void fatalError(int msgType, pdfXString message);

extern int isHexDigit(char c);
extern int hextoi(LPCTSTR cp);
extern bool parseHexValue(LPCTSTR& ptr, int& val, LPCTSTR stopChars = NULL);

extern bool parseDoubleValue(LPCTSTR& ptr, double& val, LPCTSTR stopChars = NULL);
extern bool lookingAt(LPCTSTR& ptr, LPCTSTR someStr, bool autoAdvanceOnMatch = false);
extern bool lookingAtWithCase(LPCTSTR& ptr, LPCTSTR someStr, bool autoAdvanceOnMatch = false);
extern bool advanceToNext(LPCTSTR& ptr, LPCTSTR skipping, LPCTSTR match);
extern bool parseFileNameValue(LPCTSTR& ptr, pdfXString& fileName, pdfXString& error, LPCTSTR stopChars = NULL);
extern bool parseRegexStringValue(LPCTSTR& ptr, pdfXString& regex, pdfXString& error);
extern bool parseIntegerValue(LPCTSTR& ptr, int& val, LPCTSTR stopChars = NULL);

#endif // lightUtilities_H_


