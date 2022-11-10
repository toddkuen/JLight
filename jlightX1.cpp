#include "stdafx.h"

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
//
// Change Log:
//
//	  09-15-2007 - TRK - Created.
//
//

#ifdef __PORTABLE
#include "Support/pdfXPortability.h"
#endif // __PORTABLE

#include "New/pdfCEngine1.h"
#include "New/pdfCCommonMerge.h"
#include "New/pdfCFastMergeEngine.h"
#include "New/pdfCMemory.h"
#include "New/pdfCPSStorage.h"
#include "New/pdfCColorSpec.h"
#include "XML/pdfXML.h"
#include "Common/pdfdatatypes.h"

#include <stdio.h>
#include <ctype.h>

#include "lightPKLoader.h"
#include "lightConversionEngine.h"
#include "lightUtilities.h"
#include "genericUtilities.h"

#define MAJOR_VERSION (1)
#define MINOR_VERSION (0)

extern FILE * errOut;
extern FILE * fStdOut;
extern FILE * fStdErr;

pdfXString * gVersionID = NULL;
pdfXString * gProductID = NULL;

extern char engineVersionBuffer[];
char * platformDemoKey = NULL;

int platformLoadStringFromId(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf) {
	return 0;
}

int jpegMemoryLimitToUse = -1;	

pdfXString buildId() {
  return "1.0";
}

void addInputListToDict(pdfDictC * defines) {
}

#include "version_.h"

const char * jlightX1Version(void) {

#ifdef _DEBUG
	sprintf(engineVersionBuffer, "J-Light (DEBUG Build) Build %ld.%ld.%s %s (%s)\nCopyright (C) 2007 by Todd R. Kueny\nAll Rights Reserved. US Patents 6,547,831 7,020,837\n",
#else
  sprintf(engineVersionBuffer, "J-Light (PRODUCTION Build) Build %ld.%ld.%s %s (%s)\nCopyright (C) 2007 by Todd R. Kueny\nAll Rights Reserved. US Patents 6,547,831 7,020,837\n",
#endif // _DEBUG

    (long)MAJOR_VERSION, (long)MINOR_VERSION, __autoBuildId, __autoBuildDate, 
#ifndef GCC_UNIX
#ifdef MSWIN
    platformDemoKey = "Win"
#endif // WIN_PLATFORM
#ifdef MAC_PLATFORM
    platformDemoKey = "Mac OS9"
#endif // MAC_PLATFORM
#else
#ifdef MAC_OSX
    platformDemoKey = (char *)"Mac OS X"
#endif // MAC_OSX
#ifdef HP_UX
    platformDemoKey = "HP-UX PA Risc"
#endif // HP_UX
#ifdef SOLARIS
    platformDemoKey = "Solaris sparc"
#endif // SOLARIS
#ifdef PCLINUX
    platformDemoKey = "PC Linux"
#endif // PCLINUX
#endif // GCC_UNIX
);

  return engineVersionBuffer;
}

////////////////////////////////////////////////////////////////

#define MAX_PARAM_LENGTH (1024)

typedef pdfXMap<int, int, int, int> PageResMaskT;
typedef pdfXMap<int, int, pdfXString, pdfXString> PageMediaExceptionT;

char gpJLYTInPath[MAX_PARAM_LENGTH];
char gpPDFResolution[MAX_PARAM_LENGTH];
char gpPSResolution[MAX_PARAM_LENGTH];
char gpCreateTextRaster[2];
char gpCreateHexDump[2];
char gpLogLevel[2];
char gpWorkPath[MAX_PARAM_LENGTH];
char gpElementPath[MAX_PARAM_LENGTH];
char gpOutputPath[MAX_PARAM_LENGTH];
char gpPreviewPath[MAX_PARAM_LENGTH];
char gpPDFTemplate[MAX_PARAM_LENGTH];
char gpPreviewTemplate[MAX_PARAM_LENGTH];
char gpPDFJobTemplate[MAX_PARAM_LENGTH];
char gpPSTemplate[MAX_PARAM_LENGTH];
char gpPSJobTemplate[MAX_PARAM_LENGTH];
char gpHexTemplate[MAX_PARAM_LENGTH];
char gpTextRasterTemplate[MAX_PARAM_LENGTH];
char gpHexPath[MAX_PARAM_LENGTH];
char gpTextRasterPath[MAX_PARAM_LENGTH];
char gpDefaultMediaType[MAX_PARAM_LENGTH];
char gpLogPath[MAX_PARAM_LENGTH];
bool gDisablePlane[indColorMax];
PageMediaExceptionT * gMediaExceptions = NULL;
int gPageGroupSize;
int gHardPageLimit;

PageMaskT * gIncludePages = NULL;
PageMaskT * gDontLoadPages = NULL;
PageMaskT * gTransferPages = NULL;
PageResMaskT * gResolutionPages = NULL;

int gOutputImageResolution;
double gAutoLevelMargin;
bool gAutoLevels;
bool gPrintAllPages;
bool skipAllOutput = false;
bool gBreakOnGroup;
bool gProvidePreview;
bool gAppendStdError;
bool gAggregate;
bool gCleanup;
double previewScale;

void initParameters(void) {
  gpJLYTInPath[0] = '\0';
  gpPDFResolution[0] = '\0';
  gpCreateTextRaster[0] = 'F';
  gpCreateHexDump[0] = 'F';
  gpLogLevel[0] = 'A';
  gpWorkPath[0] = '\0';
  gpElementPath[0] = '\0';
  gpOutputPath[0] = '\0';
  gpHexPath[0] = '\0';
  gpTextRasterPath[0] = '\0';
  gpPreviewPath[0] = '\0';
  gpLogPath[0] = '\0';
  gpDefaultMediaType[0] = '\0';
  gpPreviewTemplate[0] = '\0';
  
  strcpy(gpPreviewTemplate, "preview.@{imageNo}.pdf");
  strcpy(gpPDFTemplate, "pdf_jlt.@{imageNo}.pdf");
  strcpy(gpPSTemplate, "pdf_jlt.@{imageNo}.ps");
  strcpy(gpPDFJobTemplate, "FinalJob.@{groupNo}.pdf");
  strcpy(gpPSJobTemplate, "FinalJob.@{groupNo}.ps");
  strcpy(gpHexTemplate, "pdf_jlt.@{imageNo}@{planeString}.hex");
  strcpy(gpTextRasterTemplate, "pdf_jlt.@{imageNo}@{planeString}.txt");
  
  gDisablePlane[indColorC] = false;
  gDisablePlane[indColorM] = false;
  gDisablePlane[indColorY] = false;
  gDisablePlane[indColorK] = false;
  
  gIncludePages = new PageMaskT;
  gDontLoadPages = new PageMaskT;
  gTransferPages = new PageMaskT;
  gResolutionPages = new PageResMaskT;
  
  gMediaExceptions = new PageMediaExceptionT;
  
  gPageGroupSize = -1;
  gOutputImageResolution = 1;
  gAutoLevelMargin = .15;
  previewScale = .25;
  gAutoLevels = false;
  gPrintAllPages = false;
  gBreakOnGroup = false;
  gAggregate = false;
  gCleanup = true;
  gHardPageLimit = -1;
  gAppendStdError = false;
  gProvidePreview = false;
}

void xmlConvertConfigAttribute(CExtensibleMarkupLanguageElement * element_p, pdfXString& error) {
  pdfXString eName;
  
  element_p->GetTag(eName);
  
  if (element_p->GetType() == CExtensibleMarkupLanguageElement::typeElement) {
    
		if (eName.CompareNoCase("attribute") == 0) {
			pdfXString fVal, fName;
      
			if (element_p->GetAttributeByName( TEXT( "name" ), fName ) != FALSE && element_p->GetAttributeByName( TEXT( "value" ), fVal ) != FALSE) {
				if (fName.CompareNoCase("jlyt") == 0) {
          safeAttrCopy(gpJLYTInPath, fVal);
        } else {
          error.Format("unrecognized <attribute> name='%s' in tag", (LPCTSTR)fName);
        }
      } else {
				error = "couldn't find 'name' and/or 'value' attributes in <attribute> tag";
			}
    } else if (eName.CompareNoCase("generate") == 0) {
			pdfXString fVal, fName;
      
			if (element_p->GetAttributeByName( TEXT( "textraster" ), fName ) != FALSE) {
        if (fName.CompareNoCase("true") == 0) {
          gpCreateTextRaster[0] = 'T';
        }
      } else if (element_p->GetAttributeByName( TEXT( "hex" ), fName ) != FALSE) {
        if (fName.CompareNoCase("true") == 0) {
          gpCreateHexDump[0] = 'T';
        }
      } else if (element_p->GetAttributeByName( TEXT( "pdf" ), fName ) != FALSE) {
          safeAttrCopy(gpPDFResolution, fName);
      } else if (element_p->GetAttributeByName( TEXT( "log" ), fName ) != FALSE) {
        if (fName.CompareNoCase("all") == 0) {
          gpLogLevel[0] = 'A';
        } else if (fName.CompareNoCase("error") == 0) {
          gpLogLevel[0] = 'E';
        } else if (fName.CompareNoCase("warning") == 0) {
          gpLogLevel[0] = 'W';
        } else if (fName.CompareNoCase("status") == 0) {
          gpLogLevel[0] = 'S';
        } else if (fName.CompareNoCase("info") == 0) {
          gpLogLevel[0] = 'I';
        } else if (fName.CompareNoCase("debug") == 0) {
          gpLogLevel[0] = 'D';
        } else {
          error.Format("unrecognized <generate log='%s'> log option", (LPCTSTR)fName);
        }
      } else {
        error.Format("unrecognized <generate> name='%s' in tag", (LPCTSTR)fName);
      }
    } else if (eName.CompareNoCase("paths") == 0) {
			pdfXString fVal, fName;
      
			if (element_p->GetAttributeByName( TEXT( "work" ), fName ) != FALSE) {
        safeAttrCopy(gpWorkPath, fName);
      } else if (element_p->GetAttributeByName( TEXT( "output" ), fName ) != FALSE) {
        safeAttrCopy(gpOutputPath, fName);
      } else if (element_p->GetAttributeByName( TEXT( "hex" ), fName ) != FALSE) {
        safeAttrCopy(gpHexPath, fName);
      } else if (element_p->GetAttributeByName( TEXT( "log" ), fName ) != FALSE) {
        safeAttrCopy(gpLogPath, fName);
      } else if (element_p->GetAttributeByName( TEXT( "textraster" ), fName ) != FALSE) {
        safeAttrCopy(gpTextRasterPath, fName);
      } else {
        error.Format("unrecognized <generate> name='%s' in tag", (LPCTSTR)fName);
      }
		} else {
      error.Format("invalid <config> tag '%s'", (LPCTSTR)eName);
		}
  } else {
    error = "couldn't find 'attribute' element in <config>";
	}
}

void xmlConvertConfigElement(CExtensibleMarkupLanguageElement * element_p, 
                             pdfXString& error) {
  pdfXString eName;
  DWORD pos;
  CExtensibleMarkupLanguageElement * selement_p;
  
  if ( element_p == NULL ) {
    return;
  }
  
  element_p->GetTag( eName );  
  
  if (element_p->GetType() == CExtensibleMarkupLanguageElement::typeElement) {
		if (eName.CompareNoCase("config") == 0) {
			pdfXString tmp;
      
			if ( element_p->EnumerateChildren( pos ) != FALSE ) {
        while(element_p->GetNextChild( pos, selement_p ) != FALSE && error == "") {
          xmlConvertConfigAttribute(selement_p, error);
				}
			} else {
        error = "<config> tag contains no <attribute> tags";
      }
    } else {
      error = "missing <config> tag";
    }
	} else {
    error = "invalid config file structure";
  }
}

bool isValidXMLConfig(pdfXString path, pdfXString& translation) {
  CExtensibleMarkupLanguageDocument xml;
  CDataParser par;
  pdfXByteArray bytes;
  FILE * f;
  long cnt;
  translation = "";
  
  if ((f = fopen((LPCTSTR)RESOLVETOFULLPATH(path), "rb")) != NULL) {
    
		fseek(f, 0, SEEK_END);
		cnt = ftell(f);
    
		bytes.SetSize(cnt);
    
		fseek(f, 0, SEEK_SET);
		if ((cnt = fread((void *)bytes.GetData(), 1, cnt, f)) != cnt) {
		  fclose(f);
		  return false;
		}
		fclose(f);
    
		par.Initialize(&bytes, FALSE );
    
    xml.SetParseOptions(xml.GetParseOptions() | WFC_XML_LOOSE_COMMENT_PARSING);
    
    if (xml.Parse(par) == TRUE) {
			CExtensibleMarkupLanguageElement * msp = xml.GetElement("config");
      
			if (msp != NULL) {
				pdfXString error;
        
        error = "";
        
				xmlConvertConfigElement(msp, error);
        
        if (error != "") {
          translation = error;
				  return false;
				}
			}
      
			return true;
		} else {
			CParsePoint beg, end;
			pdfXString tag, * msg;
      
			msg = new pdfXString("");
			xml.GetParsingErrorInformation(tag, beg, end, msg);
      translation = "XML File Error: " + (*msg);
			delete msg;
		}
	}
  
  return false;
}

////////////////////////////////////////////////////////////////

LPCTSTR appName(void) {
  return "J-Light";
}


#ifdef GCC_UNIX
// Provide resolution to ~ in path names internal to pdfc/bimp.  Note that paths presented on the command line are
// assumed to have be processed by the shell to expand ~ in advance.
//
#include <unistd.h>

static char * loginPath = NULL;
extern char** environ;

const char * unixResolveToFullPath(const char * x) {
  int i;
  static char buf[4096];

  if (loginPath == NULL) {
    for (i = 0; environ[i] != NULL; i++) {
      if (!strncmp(environ[i], "HOME", 4)) {
        loginPath = &environ[i][5];
        break;
      }
    }
  }

  if (loginPath != NULL && x[0] == '~' && strlen(loginPath) + strlen(x) + 2 < sizeof(buf)) {
    strcpy(buf, loginPath);
    if (x[1] != '/') {
      strcat(buf, "/");
    }
    strcat(buf, &x[1]);

    return buf;
  }
  return x;
}

#endif // GCC_UNIX

LPCTSTR gTmpPathPrefix = 
#ifdef WIN_PLATFORM
"C:\\";
#else
#ifndef GCC_UNIX
"C:\\";
#else
"/tmp/";
#endif // GCC_UNIX
#endif // WIN_PLATFORM

static time_t gStartTime, gEndTime;

int main( int argc, char * argv[] ) {
  int i, result, logLevel;
  pdfXString thisArg;
  pdfXString srcFile;
  pdfXString error;
  LPCTSTR cp;
  FILE * ilogFile;
  
  initParameters();
  
	fprintf(stdout, "%s\n", jlightX1Version());

  result = 1;
  
  ilogFile = NULL;
  
  logTo(ilogFile, lightLOG_DEBUG, stdout);
    
  for (i = 1; i < argc; i++) {
		thisArg = (LPCTSTR)argv[i];

    error.Format("processing[%d]='%s'", i, argv[i]);
    addToLogFile(lightLOG_INFO, error);

    if (thisArg.CompareNoCase("-debugargs") == 0) {
			int k;

			for (k = 1; k < argc; k++) {
        error.Format("Parameter[%d]='%s'", k, argv[k]);
        addToLogFile(lightLOG_ERROR, error);
			}
    } else if (thisArg.CompareNoCase("-skipoutput") == 0) {
      error.Format("%s: SKIPOUTPUT mode...\n", appName());
      addToLogFile(lightLOG_STATUS, error);
      skipAllOutput = true;
    } else if (thisArg.CompareNoCase("-version") == 0) {
      // -version
      goto usage;
    } else if (thisArg.CompareNoCase("-jlyt") == 0) {
      // -jlyt file
      i++;
      if (i >= argc) {
        error.Format("%s: Expected source JLYT file name after '-jlyt'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      safeAttrCopy(gpJLYTInPath, argv[i]);
 
    } else if (thisArg.CompareNoCase("-hardpagelimit") == 0) {
      i++;
      if (i >= argc) {
        error.Format("%s: Expected source parameter after '-hardpagelimit'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      
      cp = (LPCTSTR)argv[i];
      extractInteger(cp, gHardPageLimit); 
      
    } else if (thisArg.CompareNoCase("-generate") == 0) {
      // -generate pdf=12in/18in,textraster,hex,log=level
      i++;
      if (i >= argc) {
        error.Format("%s: Expected source parameters after '-generate'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      
      cp = argv[i];
      for (;;) {
        if (first(cp, 10).CompareNoCase("textraster") == 0) {
          gpCreateTextRaster[0] = 'T';
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 3).CompareNoCase("hex") == 0) {
          gpCreateHexDump[0] = 'T';
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 4).CompareNoCase("log=") == 0) {
          pdfXString fName;
          
          cp += 4;
          extractTo(cp, fName, ',');
          if (fName.CompareNoCase("all") == 0) {
            gpLogLevel[0] = 'A';
          } else if (fName.CompareNoCase("error") == 0) {
            gpLogLevel[0] = 'E';
          } else if (fName.CompareNoCase("warning") == 0) {
            gpLogLevel[0] = 'W';
          } else if (fName.CompareNoCase("status") == 0) {
            gpLogLevel[0] = 'S';
          } else if (fName.CompareNoCase("info") == 0) {
            gpLogLevel[0] = 'I';
          } else if (fName.CompareNoCase("debug") == 0) {
            gpLogLevel[0] = 'D';
          } else {
            error.Format("%s: unrecognized <generate log='%s' > option.\n", appName(), (LPCTSTR)fName);
            addToLogFile(lightLOG_ERROR, error);
            goto usage;
          }
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 4).CompareNoCase("pdf=") == 0) {
          pdfXString fName;
          
          cp += 4;
          if (*cp == '[') {
            extractTo(cp, fName, ']');
            fName += "]";
          } else {
            extractTo(cp, fName, ',');
          }
          safeAttrCopy(gpPDFResolution, (LPCTSTR)fName);
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 3).CompareNoCase("ps=") == 0) {
          pdfXString fName;
          
          cp += 3;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpPSResolution, (LPCTSTR)fName);
          if (skipPast(cp, ',')) {
            continue;
          }
        } else {
          error.Format("%s: unrecognized <generate %s ...> option.\n", appName(), cp);
          addToLogFile(lightLOG_ERROR, error);
          goto usage;
        }
        
        break;
      }
      
    } else if (thisArg.CompareNoCase("-cleanup") == 0 || thisArg.CompareNoCase("-cleanup=yes") == 0) {
      gCleanup = true;
    } else if (thisArg.CompareNoCase("-cleanup=no") == 0) {
      gCleanup = false;
    } else if (thisArg.CompareNoCase("-aggregate") == 0 || thisArg.CompareNoCase("-aggregate=yes") == 0) {
      gAggregate = true;
    } else if (thisArg.CompareNoCase("-aggregate=no") == 0) {
      gAggregate = false;
    } else if (thisArg.CompareNoCase("-preview") == 0) {
      gProvidePreview = true;

      i++;
      if (i >= argc) {
        error.Format("%s: Expected percentage scale factor after '-preview'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      
      cp = argv[i];
      previewScale = atof(cp);
      
      if (previewScale > 1.0) {
        if (previewScale >= 1.0 && previewScale <= 100.0) {
          previewScale *= .01;
        } else {
          goto previewScaleError;
        }
      } else if (previewScale >= .01 && previewScale <= 1.0) {
        // do nothing
      } else {
previewScaleError:
        error.Format("%s: '-preview' parameter must be in decimal range of (.01 .. 1.0) or integer range of (1 .. 100)  .\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
    } else if (thisArg.CompareNoCase("-break=group") == 0 || thisArg.CompareNoCase("-break=none") == 0) {
      pdfXString fName;

      cp = argv[i];
      cp += 7;
      extractTo(cp, fName, ',');
      if (fName.CompareNoCase("group") == 0) {
        gBreakOnGroup = true;
      } else if (fName.CompareNoCase("none") == 0) {
        gBreakOnGroup = false;
      } else {
        error.Format("%s: Expected { group | none } after '-break='.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
    } else if (thisArg.CompareNoCase("-disable") == 0) {
      // -disable colorplane=C,M,Y,K 
      i++;
      if (i >= argc) {
        error.Format("%s: Expected parameters after '-disable'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      
      cp = argv[i];
      for (;;) {
        if (first(cp, 11).CompareNoCase("colorplane=") == 0) {
          pdfXString fName;
          
          cp += 11;
          while (*cp != '\0') {
            extractTo(cp, fName, ',');
            if (fName.CompareNoCase("c") == 0) {
              gDisablePlane[indColorC] = true;
            } else if (fName.CompareNoCase("m") == 0) {
              gDisablePlane[indColorM] = true;
            } else if (fName.CompareNoCase("y") == 0) {
              gDisablePlane[indColorY] = true;
            } else if (fName.CompareNoCase("k") == 0) {
              gDisablePlane[indColorK] = true;
            } else {
              
            }
            
            if (*cp == ',') {
              cp++;
            }
          }
          
          if (skipPast(cp, ',')) {
            continue;
          }
        } else {
          error.Format("%s: unrecognized <disable %s ...> option.\n", appName(), cp);
          addToLogFile(lightLOG_ERROR, error);
          goto usage;
        }
        
        break;
      }
      
    } else if (thisArg.CompareNoCase("-paths") == 0) {
      // -paths work=..,output=..,textraster=..,hex=..,log=..
      i++;
      if (i >= argc) {
        error.Format("%s: Expected source parameters after '-paths'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      
      cp = argv[i];
      for (;;) {
        if (first(cp, 5).CompareNoCase("work=") == 0) {
          pdfXString fName;
          
          cp += 5;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpWorkPath, (LPCTSTR)fName);
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 7).CompareNoCase("output=") == 0) {
          pdfXString fName;
          
          cp += 7;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpOutputPath, (LPCTSTR)fName);
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 4).CompareNoCase("log=") == 0) {
          pdfXString fName;
          
          cp += 4;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpLogPath, (LPCTSTR)fName);
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 8).CompareNoCase("preview=") == 0) {
          pdfXString fName;
          
          cp += 8;
          extractTo(cp, fName, ',');
          appendPathSepIfNeeded(fName);
          safeAttrCopy(gpPreviewPath, (LPCTSTR)fName);
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 7).CompareNoCase("hex=") == 0) {
          pdfXString fName;
          
          cp += 7;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpHexPath, (LPCTSTR)fName);
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 7).CompareNoCase("textraster=") == 0) {
          pdfXString fName;
          
          cp += 7;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpTextRasterPath, (LPCTSTR)fName);
          if (skipPast(cp, ',')) {
            continue;
          }
        } else {
          error.Format("%s: unrecognized <generate %s ...> option.\n", appName(), cp);
          addToLogFile(lightLOG_ERROR, error);
          goto usage;
        }
        
        break;
      }
            
    } else if (thisArg.CompareNoCase("-templates") == 0) {
      pdfXString fName;
 
      // -templates textraster=..,hex=..,pdf=..,group=..
      i++;
      if (i >= argc) {
        error.Format("%s: Expected source parameters after '-templates'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      
      cp = argv[i];
      while (*cp != '\0') {
        if (first(cp, 4).CompareNoCase("pdf=") == 0) {
          cp += 4;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpPDFTemplate, (LPCTSTR)fName); 
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 7).CompareNoCase("pdfjob=") == 0) {
          cp += 7;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpPDFJobTemplate, (LPCTSTR)fName); 
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 3).CompareNoCase("ps=") == 0) {
          cp += 3;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpPSTemplate, (LPCTSTR)fName); 
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 6).CompareNoCase("psjob=") == 0) {
          cp += 6;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpPSJobTemplate, (LPCTSTR)fName); 
          if (skipPast(cp, ',')) {
            continue;
          }
        } else {
          error.Format("%s: unrecognized <template %s ...> option.\n", appName(), cp);
          addToLogFile(lightLOG_ERROR, error);
          goto usage;
        }

        if (*cp == ',') {
          cp++;
        }
      }
      
    } else if (thisArg.CompareNoCase("-defines") == 0) {
      // [ -define transfer=presssim1,convolve=.1,kernel=..,level=auto ] - 
      
    } else if (thisArg.CompareNoCase("-pages") == 0) {
      // -pages group=28,include=1..28,applytransfer=5,7,9,11,13,15,17,19,21,23,25,27,res1=2
      i++;
      if (i >= argc) {
        error.Format("%s: Expected source parameters after '-pages'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      
      cp = argv[i];
      while (*cp != '\0') {
        if (first(cp, 6).CompareNoCase("group=") == 0) {
          pdfXString fName;
          
          cp += 6;
          extractInteger(cp, gPageGroupSize); 
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 7).CompareNoCase("ignore=") == 0) {
          pdfXString fName;
          int a, b;
          
          cp += 7;
          while (*cp != '\0') {
            extractInteger(cp, a); 
          
            if (*cp == '.' && *(cp + 1) == '.') {
              // range
              cp += 2;
              extractInteger(cp, b); 
              
              while (a <= b) {
                gDontLoadPages->SetAt(a, true);
                a += 1;
              }
            } else if (*cp == ',' && isdigit(*(cp + 1))) {
              // another individual page or range..
              gDontLoadPages->SetAt(a, true);
            } else {
              break;
            }
          
            if (*cp == ',') {
              cp++;
            }
            if (!isdigit(*cp)) {
              break;
            }
          }
          if (*cp == ',') {
            cp++;
          }
        } else if (first(cp, 8).CompareNoCase("include=") == 0) {
          pdfXString fName;
          int a, b;

          cp += 8;
          while (*cp != '\0') {
            extractInteger(cp, a); 

            if (*cp == '.' && *(cp + 1) == '.') {
              // range
              cp += 2;
              extractInteger(cp, b); 
              while (a <= b) {
                gIncludePages->SetAt(a, true);
                a += 1;
              }
            } else if (*cp == ',' && isdigit(*(cp + 1))) {
              // another individual page or range..
              gIncludePages->SetAt(a, true);
            } else {
              break;
            }
            
            if (*cp == ',') {
              cp++;
            }
            if (!isdigit(*cp)) {
              break;
            }
          }
          if (*cp == ',') {
            cp++;
          }
        } else if (first(cp, 14).CompareNoCase("applytransfer=") == 0) {
          pdfXString fName;
          int a, b;
                        
          cp += 14;
          for (;;) {
            extractInteger(cp, a); 
              
            if (*cp == '.' && *(cp + 1) == '.') {
              // range
              cp += 2;
              extractInteger(cp, b); 
              while (a <= b) {
                gTransferPages->SetAt(a, true);
                a += 1;
              }
            } else if (*cp == ',' && isdigit(*(cp + 1))) {
              // another individual page or range..
              gTransferPages->SetAt(a, true);
            } else {
              gTransferPages->SetAt(a, true);
              break;
            }
              
            if (*cp == ',') {
              cp++;
            }
            if (!isdigit(*cp)) {
              break;
            }
          }
          if (*cp == ',') {
            cp++;
          }
        } else if (first(cp, 3).CompareNoCase("res") == 0) {
          pdfXString fName;
          int a, b;
          double reso;
          
          cp += 3;
          extractDouble(cp, reso); 
          if (*cp == '=') {
            cp++;

            for (;;) {
              extractInteger(cp, a); 
                
              if (*cp == '.' && *(cp + 1) == '.') {
                // range
                cp += 2;
                extractInteger(cp, b); 
                while (a <= b) {
                  gResolutionPages->SetAt(a, (int)reso);
                  a += 1;
                }
              } else if (*cp == ',' && isdigit(*(cp + 1))) {
                // another individual page or range..
                gResolutionPages->SetAt(a, (int)reso);
              } else {
                gResolutionPages->SetAt(a, (int)reso);
                break;
              }
                
              if (*cp == ',') {
                cp++;
              }
              if (!isdigit(*cp)) {
                break;
              }
            }
          }
          
          if (*cp == ',') {
            cp++;
          }
        } else if (first(cp, 5).CompareNoCase("media") == 0) {
          pdfXString fName;
          int a, b;
          
          cp += 5;
          extractTo(cp, fName, '=');
          if (*cp == '=') {
            cp++;
            
            for (;;) {
              extractInteger(cp, a); 
              
              if (*cp == '.' && *(cp + 1) == '.') {
                // range
                cp += 2;
                extractInteger(cp, b); 
                while (a <= b) {
                  gMediaExceptions->SetAt(a, fName);
                  a += 1;
                }
              } else if (*cp == ',' && isdigit(*(cp + 1))) {
                // another individual page or range..
                gMediaExceptions->SetAt(a, fName);
              } else {
                gMediaExceptions->SetAt(a, fName);
                break;
              }
              
              if (*cp == ',') {
                cp++;
              }
              if (!isdigit(*cp)) {
                break;
              }
            }
          }
          
          if (*cp == ',') {
            cp++;
          }
        } else {
          error.Format("%s: unrecognized <pages %s ...> option.\n", appName(), cp);
          addToLogFile(lightLOG_ERROR, error);
          goto usage;
        }
      }
    } else if (thisArg.CompareNoCase("-defaults") == 0) {
      // -defaults levelmargin=.15,pageres=2,printpages=none,transfer=none,level=auto,delete=none
      double res;
      
      i++;
      if (i >= argc) {
        error.Format("%s: Expected source parameters after '-defaults'.\n", appName());
        addToLogFile(lightLOG_ERROR, error);
        goto usage;
      }
      
      cp = argv[i];
      while (*cp != '\0') {
        if (first(cp, 8).CompareNoCase("pageres=") == 0) {
          cp += 8;
          extractDouble(cp, res);
          gOutputImageResolution = (int)res;
          if (gOutputImageResolution != 1 && gOutputImageResolution != 2 && gOutputImageResolution != 4) {
            error.Format("%s: Invalid output resolution pageres=%d' - must be 1, 2, or 4.\n", appName(), gOutputImageResolution);
            addToLogFile(lightLOG_ERROR, error);
            goto usage;
          }
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 12).CompareNoCase("levelmargin=") == 0) {
          cp += 12;
          extractDouble(cp, res);
          gAutoLevelMargin = res;
          if (gAutoLevelMargin <= 0.0 || gAutoLevelMargin > 1.0) {
            error.Format("%s: Invalid output automatic level margin %s' - must be greater than 0 and less than 1.\n", appName(), (LPCTSTR)FixedToCString(gAutoLevelMargin));
            addToLogFile(lightLOG_ERROR, error);
            goto usage;
          }
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 10).CompareNoCase("mediatype=") == 0) {
          pdfXString fName;
          
          cp += 10;
          extractTo(cp, fName, ',');
          safeAttrCopy(gpDefaultMediaType, (LPCTSTR)fName);
        } else if (first(cp, 11).CompareNoCase("printpages=") == 0) {
          pdfXString fName;
          
          cp += 11;
          extractTo(cp, fName, ',');
          if (fName.CompareNoCase("none") == 0) {
            gPrintAllPages = false;
          } else if (fName.CompareNoCase("all") == 0) {
            gPrintAllPages = true;
          } else {
            error.Format("%s: Invalid -defaults printpages=%s - must be one of {none, all}.\n", appName(), (LPCTSTR)fName);
            addToLogFile(lightLOG_ERROR, error);
            goto usage;
          }
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 8).CompareNoCase("transfer=") == 0) {
          pdfXString fName;
          
          cp += 8;
          extractTo(cp, fName, ',');
          if (fName.CompareNoCase("none") == 0) {
            
          } else {
            error.Format("%s: Invalid -defaults transfer=%s - must be one of {none}.\n", appName(), (LPCTSTR)fName);
            addToLogFile(lightLOG_ERROR, error);
            goto usage;
          }
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 6).CompareNoCase("level=") == 0) {
          pdfXString fName;
          
          cp += 6;
          extractTo(cp, fName, ',');
          if (fName.CompareNoCase("auto") == 0) {
            gAutoLevels = true;
          } else {
            error.Format("%s: Invalid -defaults level=%s - must be one of {auto}.\n", appName(), (LPCTSTR)fName);
            addToLogFile(lightLOG_ERROR, error);
            goto usage;
          }
          if (skipPast(cp, ',')) {
            continue;
          }
        } else if (first(cp, 7).CompareNoCase("delete=") == 0) {
          cp += 7;
        } else {
          error.Format("%s: unrecognized <defaults %s ...> option.\n", appName(), cp);
          addToLogFile(lightLOG_ERROR, error);
          goto usage;
        }
      }
    } else {

usage:
			fprintf(stderr, "%s\n", jlightX1Version());
			fprintf(stderr, "usage: jlightX1 -jlyt [src.jlyt] [ -levelmargin .15 ] [ -autolevel ] \n");

			goto errorExit;
		}
	}
  
  // Validate any parameters we can...
  //
  if (gpWorkPath[0] != '\0') {
    int len;
    
    len = strlen(gpWorkPath);
    if (gpWorkPath[len - 1] != gpPathSep[0]) {
      strcat(gpWorkPath, gpPathSep);
    }
  } else {
    strcat(gpWorkPath, ".");
    strcat(gpWorkPath, gpPathSep);
  }
  if (gpPreviewPath[0] == '\0') {
    strcat(gpPreviewPath, ".");
    strcat(gpPreviewPath, gpPathSep);
  }
  if (gpOutputPath[0] != '\0') {
    int len;
    
    len = strlen(gpOutputPath);
    if (gpOutputPath[len - 1] != gpOutputPath[0]) {
      strcat(gpOutputPath, gpPathSep);
    }
  } else {
    strcat(gpOutputPath, ".");
    strcat(gpOutputPath, gpPathSep);
  }
  
  if (gPrintAllPages) {
    error.Format("Imaging all pages.");
    addToLogFile(lightLOG_STATUS, error);
  } else {
    if (gPageGroupSize != -1) {
      error.Format("Imaging file as groups of %d pages.", gPageGroupSize);
      addToLogFile(lightLOG_STATUS, error);
    } else {
      error.Format("Either <default printpages=all ..> or set <pages group= ..>");
      addToLogFile(lightLOG_STATUS, error);
			goto errorExit;
    }
  }
  
  if (gpLogLevel[0] == 'A' || gpLogLevel[0] == 'D') {
    logLevel = lightLOG_DEBUG;
  } else if (gpLogLevel[0] == 'E') {
    logLevel = lightLOG_ERROR;
  } else if (gpLogLevel[0] == 'W') {
    logLevel = lightLOG_WARNING;
  } else if (gpLogLevel[0] == 'S') {
    logLevel = lightLOG_STATUS;
  } else if (gpLogLevel[0] == 'I') {
    logLevel = lightLOG_INFO;
  }  
  
  if (gpLogPath[0] != '\0') {
    pdfXString logPath;
    
    logPath = gpLogPath;
    if (logPath.CompareNoCase("-stdout-")) {
      logTo(NULL, logLevel, stdout);
    } else if (logPath.CompareNoCase("-stderr-")) {
      logTo(stderr, logLevel, stdout);
    } else {
      ilogFile = fopen(gpLogPath, "w");
      if (ilogFile == NULL) {
        error.Format("%s: unable to open log file '%s' - using stdout instead.", jlightX1Version(), gpLogPath);
        addToLogFile(lightLOG_WARNING, error);
        logTo(NULL, logLevel, stdout);
      } else {
        logTo(ilogFile, logLevel, stdout);
      }
    }
  }

  /////////////////////////////////
  
  error.Format("%s\n", jlightX1Version());
  addToLogFile(lightLOG_INFO + lightLOG_LOGONLY, error);
  
  srcFile = gpJLYTInPath;
  
  time( &gStartTime );    

  indConversionEngineC * conveng;
  
  conveng = new indConversionEngineC();
  
  error.Format("Begin processing JLYT file '%s'", (LPCTSTR)srcFile);
  addToLogFile(lightLOG_STATUS, error);

  conveng->setOutputResolution(gOutputImageResolution);
  error.Format("Output file resolution set to %d", gOutputImageResolution);
  addToLogFile(lightLOG_INFO, error);
  
  if (conveng->attachSourceFile(srcFile)) {
    indPDF1DumperC * pdfDump;
    indPS1DumperC * psDump;
    indHexDumperC * hexDump;
    int workingPixelHeight, workingPixelWidth, workingPixelWidthOffset, workingPixelHeightOffset;
    double workingSheetHeightPoints, workingSheetWidthPoints;
    
    workingPixelHeight = -1;
    workingPixelWidth = -1;
    workingPixelWidthOffset = -1;
    workingPixelHeightOffset = -1;
    workingSheetHeightPoints = -1.0;
    workingSheetWidthPoints = -1.0;
    
    pdfDump = NULL;
    hexDump = NULL;
    psDump = NULL;
    
    if (gpPDFResolution[0] != '\0') {
      pdfDump = new indPDF1DumperC(conveng->getImageProcessor());
      pdfDump->setSkipWritingFile(skipAllOutput);
    }
    
    if (gpPSResolution[0] != '\0') {
      psDump = new indPS1DumperC(conveng->getImageProcessor());
      psDump->setSkipWritingFile(skipAllOutput);
    }
    
    if (gpCreateHexDump[0] == 'T') {
      hexDump = new indHexDumperC(conveng->getImageProcessor());
      hexDump->setSkipWritingFile(skipAllOutput);
    }
      
    error.Format("JLYT PDF Version: %%PDF-%s", (LPCTSTR)conveng->getLoaderHeader()->pdfVersion);
    addToLogFile(lightLOG_INFO, error);
    error.Format("JLYT Version: %s", (LPCTSTR)conveng->getLoaderHeader()->jlytVersion);
    addToLogFile(lightLOG_INFO, error);
    error.Format("RIP Format Version: %s", (LPCTSTR)conveng->getLoaderHeader()->version);
    addToLogFile(lightLOG_INFO, error);
    error.Format("Job Type: %s", (LPCTSTR)conveng->getLoaderHeader()->JTM);
    addToLogFile(lightLOG_INFO, error);
    error.Format("Creation Time Stamp: %s", (LPCTSTR)conveng->getLoaderHeader()->timeStamp);
    addToLogFile(lightLOG_INFO, error);
    error.Format("Job Name: %s", (LPCTSTR)conveng->getLoaderHeader()->jobName);
    addToLogFile(lightLOG_INFO, error);
    error.Format("RIP Info: %s", (LPCTSTR)conveng->getLoaderHeader()->ripInfo);
    addToLogFile(lightLOG_INFO, error);

    error.Format("Paper Info: %s", (LPCTSTR)conveng->getLoaderHeader()->paperInfo);
    addToLogFile(lightLOG_INFO, error);
    
    workingPixelHeight = conveng->getLoaderHeader()->pixelHeight;
    workingPixelHeightOffset = 0;
    error.Format("Job Raster Height: %s", (LPCTSTR)FixedToCString(workingPixelHeight));
    addToLogFile(lightLOG_INFO, error);
    workingPixelWidth = conveng->getLoaderHeader()->pixelWidth;
    workingPixelWidthOffset = 0;
    error.Format("Job Raster Width: %s", (LPCTSTR)FixedToCString(workingPixelWidth));
    addToLogFile(lightLOG_INFO, error);
          
    workingSheetHeightPoints = conveng->getLoaderHeader()->frontSheetSize.height;
    error.Format("Front media height: %s", (LPCTSTR)FixedToCString(workingSheetHeightPoints));
    addToLogFile(lightLOG_INFO, error);
    workingSheetWidthPoints = conveng->getLoaderHeader()->frontSheetSize.width;
    error.Format("Front media width: %s", (LPCTSTR)FixedToCString(workingSheetWidthPoints));
    addToLogFile(lightLOG_INFO, error);
    
    
    if (pdfDump != NULL) {
      LPCTSTR cp;
      double pdfHeight, pdfWidth;
      pdfXString units;
      
      pdfHeight = -1.0;
      pdfWidth = -1.0;
      cp = gpPDFResolution;
            
      if (*cp == '[' && pdfXString(gpPDFResolution).CompareNoCase("[jlyt]") != 0) {
        // pdf=[offX [px|in|pt(s)],offY [px|in|pt(s)],h [px|in|pt(s)],w [px|in|pt(s)]]
        // If pdf= is followed by a '[', then we describe a rectangle out of the larger image.
        //
        double zparts[4];
        int j;
        double lhUnits, lwUnits;    // points per pixel, some fractional value like .35XX for a 5000
          
        lhUnits = workingSheetHeightPoints / workingPixelHeight;
        lwUnits = workingSheetWidthPoints / workingPixelWidth;
          
        for (j = 0; j < 4; j++) {
          cp++;

          while (*cp != '\0' && isspace(*cp)) {
            cp++;
          }
          
          extractDouble(cp, zparts[j]);

          while (*cp != '\0' && isspace(*cp)) {
            cp++;
          }
            
          if (isalpha(*cp)) {
            pdfXString text;
              
            text = "";
            while (*cp != '\0' && isalpha(*cp)) {
              text += (TCHAR)*cp;
              cp++;
            }
            if (text.CompareNoCase("px") == 0) {
              zparts[j] = (int)zparts[j];
            } else if (text.CompareNoCase("pt") == 0 || text.CompareNoCase("pts") == 0) {
              zparts[j] = (double)((int)(zparts[j] / ((j == 0 || j == 3) ? lwUnits : lhUnits)));   // convert points to pixels truncated
            } else if (text.CompareNoCase("in") == 0) {
              zparts[j] = (int)((72.0 * zparts[j]) / ((j == 0 || j == 3) ? lwUnits : lhUnits));   // convert inches to points to pixels truncated
            } else {
              error.Format("-generate pdf=[...] requires valid measurement type (px, pt, in) for each value.");
              addToLogFile(lightLOG_WARNING, error);
            }
          } else {
            zparts[j] = (int)zparts[j];
          }
            
          if (zparts[j] < 0) {
            zparts[j] = 0.0;
          }
            
          if (zparts[j] > ((j == 0 || j == 3)? workingSheetWidthPoints / lwUnits : workingSheetHeightPoints / lhUnits)) {
            if (j == 0 || j == 3) {
              zparts[j] = workingSheetWidthPoints / lwUnits;
            } else {
              zparts[j] = workingSheetHeightPoints / lhUnits;
            }
          }
            
          while (*cp != '\0' && isspace(*cp)) {
            cp++;
          }

          if (*cp == ',') {
            continue;
          }
          break;
        }
          
        cp++;
        
        if (j != 3) {
          error.Format("-generate pdf=[...] requires four measurement values (xOff, yOff, Height, Width) with a type of (px, pt, in) for each value.");
          addToLogFile(lightLOG_ERROR, error);
          goto usage;
        }
          
        if (zparts[0] + zparts[3] > 4 * workingSheetWidthPoints / lwUnits) {
          zparts[3] = (4* workingSheetWidthPoints / lwUnits) - zparts[0];
        }
          
        if (zparts[1] + zparts[2] > 4 * workingSheetHeightPoints / lhUnits) {
          zparts[2] = (4 * workingSheetHeightPoints / lhUnits) - zparts[1 ];
        }
          
        workingPixelHeight = zparts[2];
        workingPixelWidth = zparts[3];
        workingPixelWidthOffset = zparts[0];
        workingPixelHeightOffset = zparts[1];
        pdfHeight = ((int)(workingPixelHeight * lhUnits)) / 72.0;
        pdfWidth = ((int)(workingPixelWidth * lwUnits)) / 72.0;
      } else if (pdfXString(gpPDFResolution).CompareNoCase("[jlyt]") == 0) {
        pdfHeight = workingSheetHeightPoints / 72.0;
        pdfWidth = workingSheetWidthPoints / 72.0;
        pdfDump->setCyclesPerSize(conveng->getLoaderHeader()->cyclesPerSide);
      } else {
        workingPixelWidthOffset = 0.0;
        workingPixelHeightOffset = 0.0;
        
        extractDouble(cp, pdfHeight);
        if (pdfHeight > 0.0 && *cp == '/') {
          units = "in";
          error.Format("PDF height assumes unit type as inches.");
          addToLogFile(lightLOG_INFO, error);
        } else if (pdfHeight > 0.0 && *cp != '\0') {
          extractTo(cp, units, '/');
        } else {
          pdfHeight = workingSheetHeightPoints / 12.0;
          units = "in";
          error.Format("No PDF height or units supplied (%s %s) assumed.", (LPCTSTR)FixedToCString(pdfHeight), (LPCTSTR)units);
          addToLogFile(lightLOG_WARNING, error);
        }
        if (units.CompareNoCase("in") == 0) {
          // already inches...
        } else if (units.CompareNoCase("pt") == 0 || units.CompareNoCase("pts") == 0) {
          pdfHeight /= 72.0;
        } else {
          error.Format("Unknown PDF height unit type supplied (%s), inches assumed.", (LPCTSTR)units);
          addToLogFile(lightLOG_WARNING, error);
        }
          
        if (*cp != '\0') {
          if (*cp == '/') {
            cp++;
          }
          
          extractDouble(cp, pdfWidth);
          if (pdfWidth > 0.0 && *cp == '/') {
            units = "in";
            error.Format("PDF width assumes unit type as inches.");
            addToLogFile(lightLOG_INFO, error);
          } else if (pdfWidth > 0.0 && *cp != '\0') {
            extractTo(cp, units, '/');
          } else {
            pdfWidth = workingSheetWidthPoints / 12.0;
            units = "in";
            error.Format("No PDF width or units supplied (%s %s) assumed.", (LPCTSTR)FixedToCString(pdfWidth), (LPCTSTR)units);
            addToLogFile(lightLOG_WARNING, error);
          }
          if (units.CompareNoCase("in") == 0) {
            // already inches...
          } else if (units.CompareNoCase("pt") == 0 || units.CompareNoCase("pts") == 0) {
            pdfWidth /= 72.0;
          } else {
            error.Format("Unknown PDF width unit type supplied (%s), inches assumed.", (LPCTSTR)units);
            addToLogFile(lightLOG_WARNING, error);
          }
        } else {
          pdfHeight = workingSheetHeightPoints / 12.0;
          units = "in";
          error.Format("Only PDF width supplied, no PDF height or units supplied (%s %s) assumed.", (LPCTSTR)FixedToCString(pdfHeight), (LPCTSTR)units);
          addToLogFile(lightLOG_WARNING, error);
        }
      }
      
      pdfDump->setSheetSize(pdfHeight * 72.0, pdfWidth * 72.0);
      error.Format("PDF Sheet size set to (%sh in x %sw in).", (LPCTSTR)FixedToCString(pdfHeight), (LPCTSTR)FixedToCString(pdfWidth));
      addToLogFile(lightLOG_STATUS, error);
      
      while (*cp != '\0') {
        if (*cp == '/') {
          cp++;
        }
        extractTo(cp, units, '/');
        if (units.CompareNoCase("flate") == 0) {
          pdfDump->setFlateCompress(true);
          error.Format("PDF output file flate compression: enabled.");
          addToLogFile(lightLOG_STATUS, error);
        } else if (units.CompareNoCase("noflate") == 0) {
          pdfDump->setFlateCompress(false);
          error.Format("PDF output file flate compression: disabled.");
          addToLogFile(lightLOG_STATUS, error);
        } else if (units.CompareNoCase("ascii85") == 0) { 
          pdfDump->setAscii85Encode(true);
          error.Format("PDF output file ascii85 encoding: enabled.");
          addToLogFile(lightLOG_STATUS, error);
        } else if (units.CompareNoCase("noascii85") == 0) { 
          pdfDump->setAscii85Encode(false);
          error.Format("PDF output file ascii85 encoding: disabled.");
          addToLogFile(lightLOG_WARNING, error);
        } else {
          error.Format("Unknown PDF option (%s) ignored.", (LPCTSTR)units);
          addToLogFile(lightLOG_WARNING, error);
        }
      }
    }
    
    if (psDump != NULL) {
      LPCTSTR cp;
      double psHeight, psWidth;
      pdfXString units;
      
      psHeight = -1.0;
      psWidth = -1.0;
      cp = gpPSResolution;
      
      
      extractDouble(cp, psHeight);
      if (psHeight > 0.0 && *cp == '/') {
        units = "in";
        error.Format("PS height assumes unit type as inches.");
        addToLogFile(lightLOG_INFO, error);
      } else if (psHeight > 0.0 && *cp != '\0') {
        extractTo(cp, units, '/');
      } else {
        psHeight = 12.0;
        units = "in";
        error.Format("No PS height or units supplied (%s %s) assumed.", (LPCTSTR)FixedToCString(psHeight), (LPCTSTR)units);
        addToLogFile(lightLOG_WARNING, error);
      }
      if (units.CompareNoCase("in") == 0) {
        // already inches...
      } else if (units.CompareNoCase("pt") == 0 || units.CompareNoCase("pts") == 0) {
        psHeight *= 72.0;
      } else {
        error.Format("Unknown PS height unit type supplied (%s), inches assumed.", (LPCTSTR)units);
        addToLogFile(lightLOG_WARNING, error);
      }
      
      if (*cp != '\0') {
        if (*cp == '/') {
          cp++;
        }
        
        extractDouble(cp, psWidth);
        if (psWidth > 0.0 && *cp == '/') {
          units = "in";
          error.Format("PS width assumes unit type as inches.");
          addToLogFile(lightLOG_INFO, error);
        } else if (psWidth > 0.0 && *cp != '\0') {
          extractTo(cp, units, '/');
        } else {
          psWidth = 18.0;
          units = "in";
          error.Format("No PS width or units supplied (%s %s) assumed.", (LPCTSTR)FixedToCString(psWidth), (LPCTSTR)units);
          addToLogFile(lightLOG_WARNING, error);
        }
        if (units.CompareNoCase("in") == 0) {
          // already inches...
        } else if (units.CompareNoCase("pt") == 0 || units.CompareNoCase("pts") == 0) {
          psWidth /= 72.0;
        } else {
          error.Format("Unknown PS width unit type supplied (%s), inches assumed.", (LPCTSTR)units);
          addToLogFile(lightLOG_WARNING, error);
        }
      } else {
        psHeight = 18.0;
        units = "in";
        error.Format("Only PS width supplied, no PS height or units supplied (%s %s) assumed.", (LPCTSTR)FixedToCString(psHeight), (LPCTSTR)units);
        addToLogFile(lightLOG_WARNING, error);
      }
      
      psDump->setSheetSize(psHeight * 72.0, psWidth * 72.0);
      error.Format("PS Sheet size set to (%sh x %sw).", (LPCTSTR)FixedToCString(psHeight), (LPCTSTR)FixedToCString(psWidth));
      addToLogFile(lightLOG_STATUS, error);
      
      while (*cp != '\0') {
        if (*cp == '/') {
          cp++;
        }
        extractTo(cp, units, '/');
        if (units.CompareNoCase("lzw") == 0) {
          psDump->setLZWCompress(true);
          error.Format("PS output file lzw compression: enabled.");
          addToLogFile(lightLOG_STATUS, error);
        } else if (units.CompareNoCase("nolzw") == 0) {
          psDump->setLZWCompress(false);
          error.Format("PS output file lzw compression: disabled.");
          addToLogFile(lightLOG_STATUS, error);
        } else if (units.CompareNoCase("ascii85") == 0) { 
          psDump->setAscii85Encode(true);
          error.Format("PS output file ascii85 encoding: enabled.");
          addToLogFile(lightLOG_STATUS, error);
        } else if (units.CompareNoCase("noascii85") == 0) { 
          psDump->setAscii85Encode(false);
          error.Format("PS output file ascii85 encoding: disabled.");
          addToLogFile(lightLOG_WARNING, error);
        } else {
          error.Format("Unknown PS option (%s) ignored.", (LPCTSTR)units);
          addToLogFile(lightLOG_WARNING, error);
        }
      }
    }
    
    if (conveng->getLoaderHeader()->outputType == lightJLYTDuplex) {
      error.Format("Back media height: %s", (LPCTSTR)FixedToCString(conveng->getLoaderHeader()->backSheetSize.height));
      addToLogFile(lightLOG_INFO, error);
      error.Format("Back media width: %s", (LPCTSTR)FixedToCString(conveng->getLoaderHeader()->backSheetSize.width));
      addToLogFile(lightLOG_INFO, error);
      if (conveng->getLoaderHeader()->tumble) {
        error.Format("Sheet processing: Duplex/Tumble");
        addToLogFile(lightLOG_INFO, error);
      } else {
        error.Format("Sheet processing: Duplex");
        addToLogFile(lightLOG_INFO, error);
      }
    } else {
      error.Format("Sheet processing: Simplex");
      addToLogFile(lightLOG_INFO, error);
    }

    if (pdfDump != NULL) {
      pdfXString tmp;
      
      tmp.Format("%s%s", gpWorkPath, gpPDFTemplate);
      pdfDump->setFileTemplate(tmp);

      // Code was
      //    pdfDump->setImageArea(conveng->getLoaderHeader()->pixelWidth, conveng->getLoaderHeader()->pixelHeight, 0, 0);
      // but now we use the pdf=[...] notation to extract a portion of the page...
      //
      pdfDump->setImageArea(workingPixelWidth, workingPixelHeight, workingPixelWidthOffset, workingPixelHeightOffset);
      conveng->attachDumper(pdfDump);
    }
    
    if (psDump != NULL) {
      pdfXString tmp;
      
      tmp.Format("%s%s", gpWorkPath, gpPSTemplate);
      psDump->setFileTemplate(tmp);
      psDump->setImageArea(conveng->getLoaderHeader()->pixelWidth, conveng->getLoaderHeader()->pixelHeight, 0, 0);
      conveng->attachDumper(psDump);
    }
    
    if (hexDump != NULL) {
      pdfXString tmp;
      
      tmp.Format("%s%s", gpWorkPath, gpHexTemplate);
      hexDump->setFileTemplate(tmp);
      conveng->attachDumper(hexDump);
    }
    
    if (gpCreateTextRaster[0] == 'T') {
      conveng->setDumpTextRaster(true);
    }
    
    conveng->disablePlaneRendering(&gDisablePlane[indColorC]);
    conveng->setMetaData(gpJLYTInPath);
    conveng->setDefaultResolution(gOutputImageResolution);
    conveng->setDefaultPressSim(indPressSimNone);
    
    if (gPrintAllPages) {
      
    } else {
      if (gPageGroupSize != -1) {
        int loop, bI;
        bool bV;
        
        conveng->setPageModulator(gPageGroupSize);
        for (loop = 1; loop <= gPageGroupSize; loop++) {
          if (gTransferPages->Lookup(loop, bV) && bV) {
            conveng->setPagePressSim(loop, indPressSimDropStrokeWhiteBlack);
          }
          if (gResolutionPages->Lookup(loop, bI) && bI != gOutputImageResolution) {
            conveng->setPageResolution(loop, bI);
          }
        }
      } else {
        // Never happens because of check above...
      }
    }

    if (gProvidePreview && pdfDump != NULL) {
      pdfDump->setPreviewTemplate(gpPreviewTemplate);
      pdfDump->setPreviewPath(gpPreviewPath);
        
      if (!pdfDump->addPreview(.25, 0, error)) {
        error.Format("Preview setup error: %s", (LPCTSTR)error);
        addToLogFile(lightLOG_ERROR, error);
      }
    }
    
    if (conveng->processImagesFromSourceFile(gHardPageLimit)) {
      DumperElementPathT pageControl;
      pdfXString control;
      
      if (gPrintAllPages) {
        for (i = 1; i <= conveng->getImageCount(); i++) {
          if (gMediaExceptions->Lookup(i, control)) {
            pageControl.SetAtGrow(i - 1, control);
          } else {
            pageControl.SetAtGrow(i - 1, gpDefaultMediaType);
          }
        }
     } else {
        if (gPageGroupSize != -1) {
          for (i = 1; i <= gPageGroupSize; i++) {
            if (gMediaExceptions->Lookup(i, control)) {
              pageControl.SetAtGrow(i - 1, control);
            } else {
              pageControl.SetAtGrow(i - 1, gpDefaultMediaType);
            }
          }
        } else {
          // Never happens... checked above.
        }
      }
      
      if (!skipAllOutput) {
        if (pdfDump != NULL) {
          pdfDump->setOutputMetaData(gpJLYTInPath);
          if (!conveng->processImageResults(pdfDump, gPageGroupSize, &pageControl, gpOutputPath, gpPDFJobTemplate, gBreakOnGroup, gAggregate, gCleanup)) {
            error.Format("Output file processing failed for PDF.", (LPCTSTR)srcFile);
            addToLogFile(lightLOG_FATAL, error);
            goto errorExit;
          }
        }
        if (psDump != NULL) {
          psDump->setOutputMetaData(gpJLYTInPath);
          if (!conveng->processImageResults(psDump, gPageGroupSize, &pageControl, gpOutputPath, gpPSJobTemplate, gBreakOnGroup, gAggregate, gCleanup)) {
            error.Format("Output file processing failed for PostScript.", (LPCTSTR)srcFile);
            addToLogFile(lightLOG_FATAL, error);
            goto errorExit;
          }
        }
      }
      
      result = 0;
    } else {
      error.Format("Unable to process images from JLYT file '%s'", (LPCTSTR)srcFile);
      addToLogFile(lightLOG_FATAL, error);
    }
  } else {
    error.Format("Unable to open input JLYT file '%s'", (LPCTSTR)srcFile);
    addToLogFile(lightLOG_FATAL, error);
  }

  time( &gEndTime );    
  
  if (result == 0) {
    error.Format("JLYT file '%s' processed successfully in %d seconds.", (LPCTSTR)srcFile, gEndTime - gStartTime);
    addToLogFile(lightLOG_STATUS, error);
  } else {
    error.Format("JLYT file '%s' not processed due to previous errors.", (LPCTSTR)srcFile);
    addToLogFile(lightLOG_WARNING, error);
  }

errorExit:
    
  if (ilogFile != NULL) {
    fclose(ilogFile);
  }

	pdfXStringCleanUp();
	releaseBufferPool();

  exit( result );
	return 0;
}

bool debugNotify(char * msg) {
  fputs(msg, stderr);
	return false;
}

void fatalError(const char * msg) {
	fprintf(stderr, "FATAL: %s", msg);

	exit( 1 );
}
