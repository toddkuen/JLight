#ifndef genericUtilities_H_
#define genericUtilities_H_

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  10-09-2007 - TRK - Created.
//
//

#define MAX_PARAM_LENGTH (1024)

extern bool fileExists(pdfXString path);

extern void appendPathSepIfNeeded(pdfXString& aPath);

extern void safeAttrCopy(char * dst, pdfXString src);

extern void splitFilePath(pdfXString root, pdfXString& prefix, pdfXString& fileRoot, pdfXString& ext, bool adSep = true);

extern pdfXString repathToPDF(pdfXString somePath);

extern char gpPathSep[];

extern pdfXString first(pdfXString str, int len);

extern bool skipPast(LPCTSTR& ptr, char c);

extern void extractTo(LPCTSTR& ptr, pdfXString& n, char c);

extern void extractDouble(LPCTSTR& ptr, double& d);

extern void extractInteger(LPCTSTR& ptr, int& d);

#endif // genericUtilities_H_