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

#include "Common/pdfdatatypes.h"
#include "Filters/pdfCodecFilter.h"
#include "New/pdfCUtility.h"
#include "New/pdfCEngine1.h"

#include <ctype.h>

#include "lightPKLoader.h"

enum {
  stINITIAL = 0,
  stATHEAD = 1,
  stATSTREAMHEAD = 2,
  stATSTREAMNEXT = 3,
  stATSTREAMEND = 4,
  stERROR = 5,
  stATFIRSTHEAD = 6,
  stEOF = 7
};

char * stNameStrs[] = {
  (char *)"stINITIAL",
  (char *)"stATHEAD",
  (char *)"stATSTREAMHEAD",
  (char *)"stATSTREAMNEXT",
  (char *)"stATSTREAMEND",
  (char *)"stERROR",
  (char *)"stATFIRSTHEAD",
  (char *)"stEOF"
};

lightPKLoaderC::lightPKLoaderC(indImageGrossC * parent) : indImageLoaderC(parent) {
  clear();
  produceImages = true;
  traceStates = false;
}

lightPKLoaderC::~lightPKLoaderC() {
  if (input != NULL) {
    fclose(input);
  }
  clear();
}

void lightPKLoaderC::advanceState(void) {
  char * rbfr, buf[4096], * cp;
  int len, pg, sb, img;
  long ipos;
  bool firstHead;
  pdfXString error;
  
  ipos = ftell(input);
  firstHead = false;
  
  if (traceStates) {
    error.Format("advanceState (pos=%ld) %s", ipos, stNameStrs[state]);
    addToLogFile(lightLOG_DEBUG, error);
  }
  
  switch (state) {
    case stINITIAL:
    case stATSTREAMNEXT:
      //   "<<				% s 1.1.2"
      //
      length = -1;
      for (;;) {
        rbfr = fgets(buf, sizeof(buf) - 1, input);
        if (rbfr == NULL) {
          state = stEOF;
          if (traceStates) {
            error.Format(" -> stEOF (pos=%ld,count=%ld)\n", ftell(input), ftell(input) - ipos);
            addToLogFile(lightLOG_DEBUG, error);
          }
          return;
        }
        cp = buf;
        buf[sizeof(buf) - 1] = '\0';
        len = strlen(cp);
        
        if (len > 9 && *cp == '<' && *(cp + 1) == '<') {
          cp += 2;
          while (isspace(*cp)) {
            cp++;
          }
          if (*cp == '%' && *(cp + 2) == 's' && isdigit(*(cp + 4))) {
            sscanf(cp + 4, "%d.%d.%d", &pg, &sb, &img);
            firstHead = ( thisState.page != pg || thisState.sub != sb || thisState.image != img);
            thisState.page = pg;
            thisState.sub = sb;
            thisState.image = img;
            state = stATHEAD;
            if (traceStates) {
              error.Format("-> stATHEAD %d.%d.%d (pos=%ld,count=%ld)\n", pg, sb, img, ftell(input), ftell(input) - ipos);
              addToLogFile(lightLOG_DEBUG, error);
            }
            return;
          } else {
            continue;
          }
        } else {
          continue;
        }
      }
      break;
      
    case stATFIRSTHEAD:
    case stATHEAD:
      // "/Length 32736"
      // ...
      // stream
      //
      length = -1;
      for (;;) {
        rbfr = fgets(buf, sizeof(buf) - 1, input);
        if (rbfr == NULL) {
          state = stEOF;
          if (traceStates) {
            error.Format("-> stEOF (pos=%ld,count=%ld)\n", ftell(input), ftell(input) - ipos);
            addToLogFile(lightLOG_DEBUG, error);
          }
          return;
        }
        cp = buf;
        buf[sizeof(buf) - 1] = '\0';
        len = strlen(cp);
        
        if (*cp == '/' && *(cp + 1) == 'L' && *(cp + 2) == 'e' && *(cp + 3) == 'n' && *(cp + 4) == 'g' && *(cp + 5) == 't' && *(cp + 6) == 'h') {
          sscanf(cp + 8, "%d", &pg);
          length = pg;
        } else if (length != -1 && *cp == 's' && *(cp + 1) == 't' && *(cp + 2) == 'r' && *(cp + 3) == 'e' && *(cp + 4) == 'a' && *(cp + 5) == 'm') {
          if (traceStates) {
            error.Format("-> stATSTREAMHEAD [strmlength=%d] (pos=%ld,count=%ld)\n", length, ftell(input), ftell(input) - ipos);
            addToLogFile(lightLOG_DEBUG, error);
          }
          state = stATSTREAMHEAD;
          return;
        } else {
          continue;
        }
      }
      break;
      
    case stATSTREAMHEAD:
      if (!destination->imageBufferHasRoom(length)) {
        state = stERROR;
        if (traceStates) {
          error.Format("-> stERROR(OVERFLOW) (pos=%ld,count=%ld)\n", ftell(input), ftell(input) - ipos);
          addToLogFile(lightLOG_DEBUG, error);
        }
        return;
      }
        
      if ((pg = fread(destination->getImageBufferCurrent(), 1, length, input)) != length) {
        state = stERROR;
        if (traceStates) {
          error.Format("-> stERROR(EOF) (pos=%ld,count=%ld, read=%d of %d)\n", ftell(input), ftell(input) - ipos, pg, length);
          addToLogFile(lightLOG_DEBUG, error);
        }
        return;
      }
      destination->advanceImageBuffer(length);
        
      state = stATSTREAMEND;
      if (traceStates) {
        error.Format("-> stATSTREAMEND(+%d) (pos=%ld,count=%ld)\n", length, ftell(input), ftell(input) - ipos);
        addToLogFile(lightLOG_DEBUG, error);
      }
      return;
      
    case stATSTREAMEND:
      // endstream
      //
      length = -1;
      for (;;) {
        rbfr = fgets(buf, sizeof(buf) - 1, input);
        if (rbfr == NULL) {
          state = stERROR;
          if (traceStates) {
            error.Format("-> stERROR (pos=%ld,count=%ld)\n", ftell(input), ftell(input) - ipos);
            addToLogFile(lightLOG_DEBUG, error);
          }
          return;
        }
        cp = buf;
        buf[sizeof(buf) - 1] = '\0';
        len = strlen(cp);
        
        if (*cp == 'e' && *(cp + 1) == 'n' && *(cp + 2) == 'd' && *(cp + 3) == 's' && *(cp + 4) == 't' && *(cp + 5) == 'r' &&
            *(cp + 6) == 'e' && *(cp + 7) == 'a' && *(cp + 8) == 'm') {
          if (traceStates) { 
            error.Format("-> stATSTREAMNEXT (pos=%ld,count=%ld)\n", ftell(input), ftell(input) - ipos);
            addToLogFile(lightLOG_DEBUG, error);
          }
          state = stATSTREAMNEXT;
          return;
        } else {
          continue;
        }
      }
        break;
  }
  
}

void lightPKLoaderC::clear(void) {
  thisState.page = -1;
  thisState.sub = -1;
  thisState.image = -1;
  lastState.page = -1;
  lastState.sub = -1;
  lastState.image = -1;
  
  input = NULL;
  state = stINITIAL;
}
  
bool lightPKLoaderC::init(pdfXString imagePath) {
  char buf[4096], * rbfr;
  pdfXString objData;
  pdfTypeC * info, * root, * pages, * root2;
  pdfCEngineC * eng;
  int idx;
  pdfCObjectIndexC * pindx;
  pdfXString rootS;
  int robj, rgen;
  pdfCObjectEntryC * rootObj, * pRootObj, * pPageObj, * aAnnotObj;
  long endsAt;
  double realHeight, realWidth;
  
  realHeight = -1.0;
  realWidth = -1.0;
  
  inputId = imagePath;
  
  eng = new pdfCEngineC();
  
  idx = eng->loadFile(imagePath, false, false, false);
  
  if ((pindx = eng->getObjectIndex(idx)) != NULL)
  {
    root = pindx->getRoot();
    
    pindx->activateInputFile(NULL);
    
    rootS = root->aspdfXString();
    
    sscanf((LPCTSTR)rootS, "%d %d", &robj, &rgen);
    
    if (pindx->Lookup(robj, rgen, rootObj))
    {
      root2 = rootObj->parseAsPDF(&endsAt);
      
      if (root2->isDict() && ((pages = root2->asDict()->lookup("Pages")) != NULL))
      {
        pdfTypeC * proot;
        bool deletePages;
        
        deletePages = false;
        
        if (pages->isObjectRef())
        {
          int obj, gen;
          
          pages->asObjectRef(obj, gen);
          pages = NULL;
          if (pindx->Lookup(obj, gen, pRootObj))
          {
            pages = pRootObj->parseAsPDF(&endsAt);
            deletePages = true;
          }
        }
        
        if (pages != NULL && pages->isDict())
        {
          int ix;
          pdfTypeC * kids;
          
          if ((kids = pages->asDict()->lookup("Kids")) != NULL && kids->isArray())
          {
            for (ix = 0; ix < kids->GetLength(); ix++)
            {
              proot = kids->GetAt(ix);
              
              if (proot->isObjectRef())
              {
                int pobj, pgen;
                pdfTypeC * apage, * page;
                  
                proot->asObjectRef(pobj, pgen);
                apage = NULL;
                if (pindx->Lookup(pobj, pgen, pPageObj))
                {
                    page = pPageObj->parseAsPDF(&endsAt);
                    
                    if (page->isDict())
                    {
                      pdfDictC * pDict;
                      int aobj, agen;
                      pdfTypeC * apage, * aanot, * annot, * rect;
                      
                      pDict = page->asDict();
                      
                      if ((apage = pDict->lookup("Annots")) != NULL && apage->isArray())
                      {
                        int ax;
                        double rectR[4];
                        
                        header.cyclesPerSide = apage->GetLength();
                                                          
                        for (ax = 0; ax < apage->GetLength(); ax++)
                        {
                          aanot = apage->GetAt(ax);
                          if (aanot->isObjectRef())
                          {
                            aanot->asObjectRef(aobj, agen);
                            if (pindx->Lookup(aobj, agen, aAnnotObj))
                            {
                              annot = aAnnotObj->parseAsPDF(&endsAt);
                              
                              if (annot->isDict() && (rect = annot->asDict()->lookup("Rect")) != NULL)
                              {
                                if (rect->isArray() && rect->GetLength() == 4)
                                {
                                  rectR[0] = rect->GetAt(0)->asDouble();
                                  rectR[1] = rect->GetAt(1)->asDouble();
                                  rectR[2] = rect->GetAt(2)->asDouble();
                                  rectR[3] = rect->GetAt(3)->asDouble();
                                  
                                  fprintf(stdout, "Cycle Size: Page %d, Segment %d, Rect=[%f, %f, %f, %f], Height=%f, Width=%f\n", ix, ax, rectR[0], rectR[1], rectR[2], rectR[3],
                                    rectR[2] - rectR[0], rectR[3] - rectR[1]);
                                    
                                  realHeight = rectR[2] - rectR[0];
                                  realWidth = rectR[3] - rectR[1];
                                }
                              }
                              
                              delete annot;
                            }
                          }
                        }
                      }
                    }
                    
                    delete page;
                }
              }
            }
          }
          
        }
        
        if (deletePages)
        {
          delete pages;
        }
      }
      
      
      delete root2;
    }
  }
  
    
  if ((input = fopen((LPCTSTR)imagePath, "rb")) != NULL) {
    
    // %pdf
    rbfr = fgets(buf, sizeof(buf) - 1, input);
    if (rbfr != NULL) {
      buf[8] = '\0';
      if (buf[0] == '%' && buf[1] == 'P' && buf[2] == 'D' && buf[3] == 'F') {
        header.pdfVersion = &buf[5];
      }
    } else {
      goto fail;
    }
    
    // %jlyt-ver-7.4
    rbfr = fgets(buf, sizeof(buf) - 1, input);
    if (rbfr != NULL) {
      buf[13] = '\0';
      if (buf[0] == '%' && buf[1] == 'j' && buf[2] == 'l' && buf[3] == 'y' && buf[4] == 't') {
        header.jlytVersion = &buf[10];
      }
    } else {
      goto fail;
    }
    
    // load object 1
    while ((rbfr = fgets(buf, sizeof(buf) - 1, input)) != NULL) {
      if (buf[0] == '1' && isspace(buf[1]) && buf[2] == '0' && isspace(buf[3]) && buf[4] == 'o') {
        break;
      }
    }
    
    while ((rbfr = fgets(buf, sizeof(buf) - 1, input)) != NULL) {
      if (buf[0] == 'e' && buf[1] == 'n' && buf[2] == 'd' && buf[3] == 'o' && buf[4] == 'b' && buf[5] == 'j') {
        break;
      }
      objData += buf;
    }
    
    header.tumble = false;
    header.frontSheetSize.height = -1.0;
    header.frontSheetSize.width = -1.0;
    header.backSheetSize.height = -1.0;
    header.backSheetSize.width = -1.0;  
    header.outputType = lightJLYTUnknown;
    
    info = new pdfTypeC(objData);
    if (info->isDict()) {
      pdfTypeC * tag, * jt, * v;
      
      tag = info->asDict()->lookup("Type");
      if (tag->isTag() && tag->asTagNoSlash() == "Catalog") {
        jt = info->asDict()->lookup("JT");
        if (jt->isDict()) {
          v = jt->asDict()->lookup("V");
          if (v->isNumeric()) {
            header.version.Format("%3.1f", v->asDouble());
          }

          v = jt->asDict()->lookup("A");
          if (v->isArray()) {
            pdfTypeC * elem;
            
            elem = v->GetAt(0);
            if (elem->isDict()) {
              pdfTypeC * dat;
              
              if ((dat = elem->asDict()->lookup("JTM")) != NULL && dat->isString()) {
                header.JTM = dat->asRawString();
              }
              if ((dat = elem->asDict()->lookup("Dt")) != NULL && dat->isString()) {
                header.timeStamp = dat->asRawString();
              }
            }
          }
          
          v = jt->asDict()->lookup("Cn");
          if (v->isArray()) {
            pdfTypeC * elem;
            
            elem = v->GetAt(0);
            if (elem->isDict()) {
              pdfTypeC * dat;
              
              if ((dat = elem->asDict()->lookup("JN")) != NULL && dat->isString()) {
                header.jobName = dat->asRawString();
              }
              if ((dat = elem->asDict()->lookup("Cm")) != NULL && dat->isString()) {
                header.ripInfo = dat->asRawString();
              }
              
              if ((dat = elem->asDict()->lookup("MS")) != NULL && dat->isDict()) {
                pdfTypeC * med;
                
                med = dat->asDict()->lookup("Me");
                if (med->isDict()) {
                  pdfTypeC * ent;
                  
                  if ((ent = med->asDict()->lookup("Dm")) != NULL && ent->isArray()) {
                    if (ent->GetLength() >= 2) {
                      header.outputType = lightJLYTSimplex;
                      header.frontSheetSize.height = ent->GetAt(0)->asDouble();
                      header.frontSheetSize.width = ent->GetAt(1)->asDouble();
                      header.backSheetSize.height = ent->GetAt(0)->asDouble();
                      header.backSheetSize.width = ent->GetAt(1)->asDouble();
                    } 
                    if (ent->GetLength() >= 4) {
                      header.outputType = lightJLYTDuplex;
                      header.backSheetSize.height = ent->GetAt(2)->asDouble();
                      header.backSheetSize.width = ent->GetAt(3)->asDouble();
                    } 
                    if (ent->GetLength() >= 5) {
                      header.outputType = lightJLYTUnknown;
                    }
                  }

                  if ((ent = med->asDict()->lookup("Ct")) != NULL && ent->isString()) {
                    header.paperInfo = ent->asRawString();
                  }
                }
              }

              if ((dat = elem->asDict()->lookup("F")) != NULL && dat->isArray()) {
                pdfTypeC * med;
                
                if ((med = dat->GetAt(0)) != NULL && med->isDict()) {
                  pdfTypeC * ent;
                  
                  if ((ent = med->asDict()->lookup("O")) != NULL && ent->isTag()) {
                    if (ent->asTagNoSlash() == "Duplex") {
                       header.outputType = lightJLYTDuplex;
                    }
                  }
                }
                
                if ((med = dat->GetAt(1)) != NULL && med->isDict()) {
                  pdfTypeC * ent;
                  
                  if ((ent = med->asDict()->lookup("O")) != NULL && ent->isTag()) {
                    if (ent->asTagNoSlash() == "DuplexTumble") {
                      header.outputType = lightJLYTDuplex;
                      header.tumble = true;
                    }
                  }
                }
                
              }
            }
          }
        }
      }
    }
    delete info;
    
    if (realHeight != -1.0 && realWidth != -1.0)
    {
      header.frontSheetSize.height = realHeight;
      header.frontSheetSize.width = realWidth;
      header.backSheetSize.height = realHeight;
      header.backSheetSize.width = realWidth;
    }
    
    while ((rbfr = fgets(buf, sizeof(buf) - 1, input)) != NULL) {
      if (buf[6] == '%' && buf[7] == ' ' && buf[8] == 'e' && buf[9] == ' ' && buf[10] == '1' && buf[11] == '.' && buf[12] == '1') {
        break;
      }
    }
    
    {
      char * cpx;
      
      cpx = &buf[13];
      while (*cpx != '\0' && !isspace(*cpx))
      {
        cpx++;
      }
    
      sscanf(cpx, "%dx%d", &header.pixelHeight, &header.pixelWidth);
    }

    fseek(input, 0, SEEK_SET);    // rewind for loading images...
    
    return true;
    
fail:
    fclose(input);
  }
  return false;
}

bool lightPKLoaderC::loadImages(int pageLimit) {
  bool first;
  int accum;
  int plane;
  pdfXString error;
  extern PageMaskT * gDontLoadPages;
  
  first = true;
  accum = 0;
  plane = indColorFirst;
  destination->nextImage();
  
  error.Format("Begining first image.");
  addToLogFile(lightLOG_STATUS, error);

  for (;;) {
    lastState.page = thisState.page;
    lastState.sub = thisState.sub;
    lastState.image = thisState.image;
    
    advanceState();
    
    if (state == stERROR) {
      error.Format("Parse of JLYT file terminates on image %d with error (state = stERROR).", thisState.page);
      addToLogFile(lightLOG_ERROR, error);
      return false;
    }
    
    switch (state) {
      case stATHEAD:
      case stEOF:
        if (state == stEOF || lastState.page != thisState.page || lastState.sub != thisState.sub || lastState.image != thisState.image) {
          if (!first) {
            
            // Have complete image in a buffer
            //
            if (produceImages && destination->canExtractPage(lastState.page)) {
              error.Format("Image %d plane %s (%d.%d.%d) length=%d resolution=%dx simcode=0x%04X.", lastState.page, destination->getPlaneAsString(), lastState.page, lastState.sub, lastState.image, 
                           destination->getImageBufferSize(), destination->getPageResolution(lastState.page), destination->getPagePressSim(lastState.page));
              addToLogFile(lightLOG_STATUS, error);
                 
              destination->imageProcessSequence(destination->getPageResolution(lastState.page), destination->getPagePressSim(lastState.page), gDontLoadPages->ContainsKey(lastState.page));
              
              if (destination->isLastPlane() && !destination->extractAll()) {
                destination->setPageToExtract(lastState.page, false);
                if (destination->allPagesExtracted()) {
                  goto allDone;
                }
              }
            }
              
            // Start over...
            //
            if (destination->isLastPlane()) {
              destination->advanceSubImageCount();
              destination->resetImageBuffer();
              destination->nextImage();

              error.Format("Image %d complete, begining next image.", lastState.page);
              addToLogFile(lightLOG_STATUS, error);
              
              if (pageLimit != -1 && lastState.page >= pageLimit) {
                goto allDone;
              }

          } else {
              destination->resetImageBuffer();
              destination->nextPlane();
            }

            accum = 0;
                        
          }
          first = false;
        }
        
        if (state == stEOF) {
          return true;
        }
        break;
        
      case stATSTREAMHEAD:
        if (traceStates) { 
          error.Format("###### lightPKLoaderC::loadNextImage: Length accumulator (%d adds %d, offset = %d).", accum, length, destination->getImageBufferSize());
          addToLogFile(lightLOG_DEBUG, error);
        }
        accum += length;
        break;
        
      default:
        break;
    }
  }
  
allDone:

  return true;
}

