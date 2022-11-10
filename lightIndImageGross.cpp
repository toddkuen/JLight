#include "stdafx.h"

// Copyright (C) 2007 Todd R. Kueny, Inc.  All Rights Reserved.
// All Rights Reserved.
//
// Change Log:
//
//	  09-14-2007 - TRK - Created
//
//

#ifdef __PORTABLE
#include "Support/pdfXPortability.h"
#endif // __PORTABLE

#include "Common/pdfdatatypes.h"
#include "Filters/pdfCodecFilter.h"
#include "New/pdfCUtility.h"

#include "imageScaler.h"
#include "lightIndImageGross.h"
#include "indConvolve.h"

#include <time.h>

void indImagePreviewBaseC::initializeCodec(pindJLTCodecState state) {
  state->rawData = (char *)imageBuffer;
  state->oHeight = height;
  state->currentRow = 0;
  state->endRowExclusive = height;
  state->currentColumn = width - 1;
  state->startColumnInclusive = 0;
  state->endColumnExclusive = width - 1;
  state->bytesPerStep = colorsPerEntry;
  state->totalBytesWritten = 0;
  state->bytesWritten = 0;  
}

// Note: For JLYT we exchange height and width because width acts like a windows-world height...

bool indImagePreviewBaseC::deriveFromBaseImageFiltered(unsigned char * baseIb, 
                                                       int pixelHeight, 
                                                       int paddedHeightBytes,
                                                       int pixelWidth) {
  C2PassScale <CBilinearFilter> ScaleEngine;
  
  ScaleEngine.Scale (baseIb,                                  // A pointer to the source bitmap bits
                     colorsPerEntry,                          // The size of a single pixel in bytes (both source and scaled image)
                     pixelHeight,                             // The width of a line of the source image to scale in pixels
                     paddedHeightBytes,                       // The width of a single line of the source image in bytes (to allow for padding, etc.)
                     pixelWidth,                              // The height of the source image to scale in pixels.
                     imageBuffer,                             // A pointer to a buffer to hold the ecaled image
                     height,                                  // The desired width of a line of the scaled image in pixels
                     height * colorsPerEntry,                 // The width of a single line of the scaled image in bytes (to allow for padding, etc.)
                     width);                                  // The desired height of the scaled image in pixels.
  
#ifdef PRODUCT_DEMO_BUILD
  int tick, i;
  int iSecretStep, iSecretC, iSecretM, iSecretY, iSecretK;
  
  srand ( time(NULL) );
  
  iSecretStep = (rand() % 60) + 10;
  iSecretC = 100 + 35 - (rand() % 39);
  iSecretM = 110 + 55 - (rand() % 45);
  iSecretY = 95 + 62 - (rand() % 51);
  iSecretK = 106 + 39 - (rand() % 29);
  
  tick = 17 + (10 - rand() % 10);
  for (i = 0; i < width * height * colorsPerEntry; i++) {
    if ((i % iSecretStep == 0) || (i % iSecretStep) == 1 || (i % iSecretStep) == 2 || (i % iSecretStep) == 3) {
      tick -= 1;
      
      if (tick <= 0) {
        tick = 13 + (8 - rand() % 9);
        iSecretC = 128 + 50 - (rand() % 50);
        iSecretM = 128 + 50 - (rand() % 50);
        iSecretY = 128 + 50 - (rand() % 50);
        iSecretK = 128 + 50 - (rand() % 50);
      }
      if (i % 4 == 0) {
        if (imageBuffer[i] < iSecretC) {
          imageBuffer[i] = iSecretC;
          imageBuffer[i + 4] += iSecretC / 2;
        } else {
          imageBuffer[i] += (unsigned char)(.30*imageBuffer[i]);
          imageBuffer[i - 4] += (unsigned char)(.15*imageBuffer[i - 4]);
        }
      }
      if (i % 4 == 1) {
        if (imageBuffer[i] < iSecretM) {
          imageBuffer[i] = iSecretM;
          imageBuffer[i - 16] += iSecretY / 5;
          imageBuffer[i + 16] -= iSecretY / 5;
        } else {
          imageBuffer[i] += (unsigned char)(.35*imageBuffer[i]);
        }
      }
      if (i % 4 == 2) {
        if (imageBuffer[i] < iSecretY) {
          imageBuffer[i] = iSecretY;
          imageBuffer[i - 8] += iSecretY;
          imageBuffer[i + 8] -= iSecretY;
        } else {
          imageBuffer[i] -= (unsigned char)(.35*imageBuffer[i]);
          imageBuffer[i + 4] += (unsigned char)(.5*imageBuffer[i + 4]);
          imageBuffer[i - 4] += (unsigned char)(.5*imageBuffer[i - 4]);
        }
      }
      if (i % 4 == 3) {
        if (imageBuffer[i] < iSecretK) {
          imageBuffer[i] = iSecretK;
        } else {
          imageBuffer[i - 3] = 0;
          imageBuffer[i - 2] = 0;
          imageBuffer[i - 1] = 0;
          imageBuffer[i] = 0; //-= (unsigned char)(.15*outBase[i]);
          imageBuffer[i + 4] += (unsigned char)(.25*imageBuffer[i + 4]);
          imageBuffer[i - 4] -= (unsigned char)(.25*imageBuffer[i - 4]);
        }
      }
    }
  }
#endif // PRODUCT_DEMO_BUILD 
  return true;
}

//////////////////////////

bool indImageDumperC::dumpImage(void) {
  indImageLoaderC * aLoader;
  
  aLoader = destination->getLoader();
  fprintf(stderr, "!!!!!!! -> dumping sub image %d from %s\n", destination->getSubImageId(), (LPCTSTR)aLoader->inputId);
  return true;
}

pdfXString indImageDumperC::stringPerformSubstitutions(pdfXString input) {
  int i;
  pdfXString result, tag;
  
  for (i = 0; i < input.GetLength(); i++) {
    if (input.GetAt(i) == '@') {
      if (i + 1 < input.GetLength() && input.GetAt(i + 1) == '@') {
        result += (TCHAR)'@';
        i++;
      } else if (i + 1 < input.GetLength() && input.GetAt(i + 1) == '{') {
        i += 2;
        tag = "";
        while (i < input.GetLength() && input.GetAt(i) != '}') {
          tag += (TCHAR)input.GetAt(i);
          i++;
        }
        
        if (i < input.GetLength() && input.GetAt(i) == '}') {
          if (tag.CompareNoCase("groupNo") == 0) {
            tag.Format("%03d", groupNo);
            result += tag;
          } else if (tag.CompareNoCase("fileRoot") == 0) {
            tag.Format("%s", (LPCTSTR)fileRoot);
            result += tag;
          } else if (tag.CompareNoCase("filePath") == 0) {
            tag.Format("%s", (LPCTSTR)filePath);
            result += tag;
          } else if (tag.CompareNoCase("fileExt") == 0) {
            tag.Format("%s", (LPCTSTR)fileExt);
            result += tag;
          } else {
            result += "?-" + tag + "-?";
          }
        } else {
          result += "?-badtag-?";
        }
      } else {
        i++;
      }
    } else {
      result += (TCHAR)input.GetAt(i);
    }
  }
  
  return result;
}

bool indImageDumperC::dumpMasterFile(int groupSize,
                                     pDumperElementPathT dmp, 
                                     pDumperElementPathT pageControlInfo, 
                                     pdfXString path, 
                                     pdfXString otemplate,
                                     bool breakOnGroup,
                                     bool aggregate,
                                     bool cleanup) {
  int j;
  
  if (dmp != NULL) {
    for (j = 0; j < dmp->GetSize(); j++) {
      fprintf(stdout, "GENERIC DUMP (PATH='%s' TEMPLATE=%s'): %d: %s (CTRL='%s')\n", (LPCTSTR)path, (LPCTSTR)otemplate, j, (LPCTSTR)dmp->GetAt(j), j < pageControlInfo->GetSize()? (LPCTSTR)pageControlInfo->GetAt(j) : "**NONE**");
    }
  } else {
    fprintf(stdout, "GENERIC DUMP - DMP NULL - NO OUTPUT!!!!\n");
  }
  
  return true;
}


indImageLoaderC::indImageLoaderC(indImageGrossC * parent) {
  destination = parent;
}

indImageGrossC::indImageGrossC() {
  clear();
}

void indImageGrossC::clear(void) {
  int i;
  
  imageBuffer = NULL;
  imageBufferCurrent = NULL;
  imageBufferSize = -1;
  
  outputBuffer = NULL;
  outputBufferCurrent = NULL;
  outputBufferSize = -1;
  colorsPerEntry = -1;
  
  loader = NULL;
  for (i = 0; i < MAX_IMAGE_DUMPERS; i++) {
    dumpers[i] = NULL;
    dumperPaths[i] = NULL;
  }
  dumperCount = 0;
  
  subImageCounter = 0;
  subImageGroupCounter = 0;
  subWithinImageCounter = 0;

  counts = NULL;
  starts = NULL;
  
  performAutoLevel = true;
  levelMargin = .05;
  
  icolors[indColorC] = NULL;
  icolors[indColorM] = NULL;
  icolors[indColorY] = NULL;
  icolors[indColorK] = NULL;
  
  plane = indColorFirst;
  lastPlane = false;
  dumpTextRaster = false;
  applyConvolution = false;
  convolutionMixAmount = 0.0;
  
  renderPlane[indColorC] = true;
  renderPlane[indColorM] = true;
  renderPlane[indColorY] = true;
  renderPlane[indColorK] = true;
  textRenditionPath = "/bugs/pkgraphics/";
  textRenditionTemplate = "pdf_jlt.%d%s.txt";
  
  minRenderColumn = 0;
  maxRenderColumn = 999999999;
  
  indVersion = indIC96FileFormat;
  pageModulator = -1;
  pressSim = indPressSimDropStrokeWhiteBlack;
}

pdfXString indImageGrossC::stringPerformSubstitutions(pdfXString input) {
  int i;
  pdfXString result, tag;
  
  for (i = 0; i < input.GetLength(); i++) {
    if (input.GetAt(i) == '@') {
      if (i + 1 < input.GetLength() && input.GetAt(i + 1) == '@') {
        result += (TCHAR)'@';
        i++;
      } else if (i + 1 < input.GetLength() && input.GetAt(i + 1) == '{') {
        i += 2;
        tag = "";
        while (i < input.GetLength() && input.GetAt(i) != '}') {
          tag += (TCHAR)input.GetAt(i);
          i++;
        }
        
        if (i < input.GetLength() && input.GetAt(i) == '}') {
          if (tag.CompareNoCase("imageNo") == 0) {
            tag.Format("%d", getSubImageId() + 1);
            result += tag;
          } else if (tag.CompareNoCase("fileRoot") == 0) {
            tag.Format("%s", (LPCTSTR)fileRoot);
            result += tag;
          } else if (tag.CompareNoCase("fileExt") == 0) {
            tag.Format("%s", (LPCTSTR)fileExt);
            result += tag;
          } else if (tag.CompareNoCase("filePath") == 0) {
            tag.Format("%s", (LPCTSTR)filePath);
            result += tag;
          } else if (tag.CompareNoCase("planeString") == 0) {
            tag.Format("%s", getPlaneAsString());
            result += tag;
          } else {
            result += "?-" + tag + "-?";
          }
        } else {
          result += "?-badtag-?";
        }
      } else {
        i++;
      }
    } else {
      result += (TCHAR)input.GetAt(i);
    }
  }
  
  return result;
}

void indImageGrossC::nextImage(void) {
  int i;
  
  plane = indColorFirst;
  lastPlane = false;

  if (counts != NULL) {
    delete counts;
    counts = NULL;
  }
  
  if (starts != NULL) {
    delete starts;
    starts = NULL;
  }
  
  for (i = 0; i < indColorMax; i++) {
    delete icolors[i];
    icolors[i] = NULL;
  }
  
}

bool indImageGrossC::dumpFileList(void * me, int groupSize, pDumperElementPathT control, pdfXString path, pdfXString otemplate, bool breakOnGroup, bool aggregate, bool cleanup) {
  int i;
  bool status;
  
  status = true;
  for (i = 0; i < dumperCount; i++) {
    if (me == dumpers[i] && me != NULL) {
      bool thisStatus;
      
      thisStatus = dumpers[i]->dumpMasterFile(groupSize,
                                              dumperPaths[i], 
                                              control, 
                                              path, 
                                              otemplate,
                                              breakOnGroup,
                                              aggregate,
                                              cleanup);
      if (!thisStatus) {
        status = false;
        break;
      }
    }
  }
  
  return status;
}


void indImageGrossC::nextPlane(void) {
  plane++;
  lastPlane = (plane == indColorK);
}

void indImageGrossC::initialize(int inputSize, int outputSize) {
  int s;
  
  s = 4;    // assume maximum resolution for memory allocation.
  
  colorsPerEntry = 4;

  imageBuffer = new char[inputSize];
  imageBufferCurrent = imageBuffer;
  imageBufferSize = inputSize;
  
  levelMargin = 0.05;;
  performAutoLevel = true;
  
  outputBuffer = new char[colorsPerEntry*s*s*outputSize];
  outputBufferCurrent = outputBuffer;
  outputBufferSize = outputSize;
  
  inputScanLineLength = -1;
  inputScanLine = NULL;
  maskScanLineLength = -1;
  maskScanLine = NULL;
  outputScanLineLength = -1;
  outputScanLine = NULL;
  
  if (outputBuffer == NULL) {
    fatalError(lightLOG_INTERNAL_ERROR, "Insufficient memory to handle images.");
  }
}

indImageGrossC::~indImageGrossC() {
  int i;
  
  if (imageBuffer != NULL) {
    delete imageBuffer;
  }
  
  if (outputBuffer != NULL) {
    delete outputBuffer;
  }
  
  if (inputScanLine != NULL) {
    delete inputScanLine;
  }
  
  if (maskScanLine != NULL) {
    delete maskScanLine;
  }
  
  if (outputScanLine != NULL) {
    delete outputScanLine;
  }
  
  if (loader != NULL) {
    delete loader;
  }
  
  for (i = 0; i < MAX_IMAGE_DUMPERS; i++) {
    if (dumpers[i] != NULL) {
      delete dumpers[i];
    }
    if (dumperPaths[i] != NULL) {
      delete dumperPaths[i];
    }
  }
  
  if (starts != NULL) {
    delete starts;
  }
  
  if (counts != NULL) {
    delete counts;
  }
  
  for (i = 0; i < indColorMax; i++) {
    delete icolors[i];
  }
  
  clear();
}

bool indImageGrossC::loadImages(int pageLimit) {
  if (loader != NULL) {
    return loader->loadImages(pageLimit);
  }
  
  return false;
}

bool indImageGrossC::buildIndicies(bool& handlesFullImage, int s, int presSim, bool skipLoad) {
  unsigned char * cp;
  IC96T * hp;
  int pl;
  bool inLater;
  unsigned short int count;
  int chunk, col, colorCount, wwidth;
  
  handlesFullImage = false;
  iColorMax[plane] = 4096 * 4;
  chunk = 0;
  col = 0;
  hp = (IC96T *)imageBuffer;
  
  if (hp->iid[2] == '0' && hp->iid[3] == '5') {
    indVersion = indIC05FileFormat;
  }
  
  pl = 0;
  colorCount = 0;
  icolors[plane] = new unsigned char[iColorMax[plane]];
  
  if (plane == indColorFirst) {
    width = hp->width;
    
    if (minRenderColumn < 0 || minRenderColumn > width) {
      minRenderColumn = 0;
    }
    if (maxRenderColumn < minRenderColumn || maxRenderColumn > width) {
      maxRenderColumn = width;
    }
    
    height = hp->height;
    wwidth = hp->width;
    starts = new unsigned char *[wwidth + 1];
    counts = new unsigned int[wwidth + 1];
  } else {
    if (width != hp->width || height != hp->height) {
      pdfXString error;
      
      error.Format("Plane dimensions out of sync with header: width (%d != %d) height (%d != %d).", width, hp->width, height, hp->height);
      fatalError(lightLOG_ERROR, error);

      return false;
    }
  }
  
  count = 0;
  inLater = false;
nextPlane:
  col = 0;
  cp = (unsigned char *)(hp + 1);
  while (cp < (unsigned char *)imageBufferCurrent) {
    if (*cp == 0375 && !inLater && (pl == 0 || pl == 1 || pl == 2 || pl == 3)) {
      count = 0;
      count = (*(cp + 1) & 0xFF) << 8;
      count |= *(cp + 2) & 0xFF;
      cp += 3;
      
      starts[chunk] = cp;
      counts[chunk] = (count & 0x7FFF) - 3;
      
      chunk += 1;
      if (chunk > wwidth) {
        return false;
      }
      
      cp += (count & 0x7FFF) - 3;
      
      if ((count & 0x8000) != 0) {
        hp = (IC96T *)cp;
        
        // Advanced in nextPlane...
        hp += 1;
        cp = (unsigned char *)hp;

        count &= 0x7FFF;
        count *= 2;
        //delete icolors[pl + 1];
        //icolors[pl] = new unsigned char[ (4096 * 4) ];
        //iColorMax[pl] = count;
        
        inLater = true;
        col = 0;
        
        {
          unsigned char * scp;
          int mscount;
          
          scp = cp;
          mscount = 0;
          while (scp < (unsigned char *)imageBufferCurrent) 
          {
            if (*scp == 'I' && *(scp+1) == 'C' && *(scp+2) == '9' && *(scp+3) == '6')
            {
              break;
            }
            scp++;
            mscount += 1;
          }
          count = mscount;
        }

        if (cp < (unsigned char *)imageBufferCurrent) {
          goto nextPlane;
        }
        break;
      }
    } else {
      if (col > iColorMax[plane]) {
        return false;
      }
      
      if (*cp == 'I' && *(cp+1) == 'C' && *(cp+2) == '9' && *(cp+3) == '6')
      {
        count = 0;
        goto countIsZero;
      }
      
      icolors[plane][col] = (*cp & 0xFF);
      colorCount = col / 4;
      cp++;
      col++;
      
      // Added to handle when all planes are run together...
      count--;
      
countIsZero:

      if ((short int)count <= 0)
      {
        hp = (IC96T *)cp;
        inLater = false;
        chunk = 0;
        if (plane == indColorK || (cp + 1 >= (unsigned char *)imageBufferCurrent))
        {
          break; //goto nextPlane;
        }
        else
        {
          
          if (!(hp->iid[0] == 'I' && hp->iid[1] == 'C' && hp->iid[2] == '9' && hp->iid[3] == '6'))
          {
            continue;
          }
          
          iColorMax[plane] = colorCount;

          //       
          handlesFullImage = true;
          dumpImage(true);
          processImage(s, presSim, skipLoad);
          if (!skipLoad) {
            dumpImage(false);
          }
          
          plane += 1;
          pl += 1;
          iColorMax[plane] = 4096 * 4;
          icolors[plane] = new unsigned char[iColorMax[plane]];

          goto nextPlane;
        }
      }
      //
    }
  }
  
  if (handlesFullImage && plane == indColorK)
  {
    lastPlane = true;
    if (handlesFullImage)
    {
      dumpImage(true);
      processImage(s, presSim, skipLoad);
      if (!skipLoad) {
        dumpImage(false);
      }
    }
  }
  
  iColorMax[plane] = colorCount;
  
  return true;
}

enum {
  opNONE = 0,
  opFF = 1,
  opFFFF = 2,
  opZRO = 3, 
  opFC = 4,
  opSHORT_GB = 5,
  opSHORT_SCR = 6
};

void writeUnderPixelFromBlack(unsigned char * outBase, int outP, int C, int M, int Y, int s, int colorsPerEntry, int pixelStep) {
  switch (s) {
    case 1:
      if (C != -1) {
        outBase[outP - 3] = C & 0xFF;
      }
      if (M != -1) {
        outBase[outP - 2] = M & 0xFF;
      }
      if (Y != -1) {
        outBase[outP - 1] = Y & 0xFF;
      }
      break;
      
    case 2:
      writeUnderPixelFromBlack(outBase, outP, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + colorsPerEntry, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + colorsPerEntry + pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      break;

    case 4:
      writeUnderPixelFromBlack(outBase, outP + 0*colorsPerEntry + 0*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 1*colorsPerEntry + 0*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 2*colorsPerEntry + 0*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 3*colorsPerEntry + 0*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 0*colorsPerEntry + 1*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 1*colorsPerEntry + 1*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 2*colorsPerEntry + 1*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 3*colorsPerEntry + 1*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 0*colorsPerEntry + 2*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 1*colorsPerEntry + 2*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 2*colorsPerEntry + 2*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 3*colorsPerEntry + 2*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 0*colorsPerEntry + 3*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 1*colorsPerEntry + 3*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 2*colorsPerEntry + 3*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      writeUnderPixelFromBlack(outBase, outP + 3*colorsPerEntry + 3*pixelStep, C, M, Y, 1, colorsPerEntry, pixelStep);
      break;
  }
}

int indImageGrossC::writePixel(unsigned char * outBase, int& outP, int value, int s, int pixelStep, int rMask, bool isGB, int lastV) {
  /*
  if (pass == 0) {
    inputScanLine[outP] = value;
    maskScanLine[outP] = lastV;
    outP += s * colorsPerEntry;
    return 0;
  } else {    
    value = inputScanLine[outP];
  }
  */
  
  switch (s) {
  case 1:
    outBase[outP] = value;
    outP += colorsPerEntry;
    break;
  case 2:
    if (rMask != 0x10000) {
/*
      int v1, v0;
      int cells[4], n;
      
      v1 = value;
      v0 = lastV;
      //if (v1 < v0) {
      //  //unsigned char x = (unsigned char)v0;
      //  
      //  v0 = v1;
      //  v1 = v0;
      //}
      //v1 = value;
      //v0 = lastV;
      n = 1;//outputScanLine[outP / (2 * colorsPerEntry) ];
      rMask = ((rMask & 0xFF) << 8) | ((rMask & 0xFF00) >> 8); // Swap bytes
      
      cells[0] = 0;
      cells[1] = 0;
      cells[2] = 0;
      cells[3] = 0;
      
      cells[0] += (((rMask & 0x0008) != 0)? 1 : 0); // 0 * colorsPerEntry + 2,3 * pixelStep
      cells[0] += (((rMask & 0x0080) != 0)? 1 : 0);
      cells[1] += (((rMask & 0x0800) != 0)? 1 : 0); // 0 * colorsPerEntry + 0,1 * pixelStep
      cells[1] += (((rMask & 0x8000) != 0)? 1 : 0);
      
      cells[0] += (((rMask & 0x0004) != 0)? 1 : 0); // 1 * colorsPerEntry + 2,3 * pixelStep
      cells[0] += (((rMask & 0x0040) != 0)? 1 : 0);
      cells[1] += (((rMask & 0x0400) != 0)? 1 : 0); // 1 * colorsPerEntry + 0,1 * pixelStep
      cells[1] += (((rMask & 0x4000) != 0)? 1 : 0);
      
      cells[2] += (((rMask & 0x0002) != 0)? 1 : 0); // 2 * colorsPerEntry + 2,3 * pixelStep
      cells[2] += (((rMask & 0x0020) != 0)? 1 : 0);
      cells[3] += (((rMask & 0x0200) != 0)? 1 : 0); // 2 * colorsPerEntry + 0,1 * pixelStep
      cells[3] += (((rMask & 0x2000) != 0)? 1 : 0);
      
      cells[2] += (((rMask & 0x0001) != 0)? 1 : 0); // 3 * colorsPerEntry + 2,3 * pixelStep
      cells[2] += (((rMask & 0x0010) != 0)? 1 : 0);
      cells[3] += (((rMask & 0x0100) != 0)? 1 : 0); // 3 * colorsPerEntry + 0,1 * pixelStep
      cells[3] += (((rMask & 0x1000) != 0)? 1 : 0);
      
      outBase[outP + (0 * colorsPerEntry) + (1 * pixelStep)] = (cells[0] >= n)? v1*(((float)cells[0])/4.0) : v0;
      outBase[outP + (0 * colorsPerEntry) + (0 * pixelStep)] = (cells[1] >= n)? v1*(((float)cells[1])/4.0) : v0;
        
      outBase[outP + (1 * colorsPerEntry) + (1 * pixelStep)] = (cells[2] >= n)? v1*(((float)cells[2])/4.0) : v0;
      outBase[outP + (1 * colorsPerEntry) + (0 * pixelStep)] = (cells[3] >= n)? v1*(((float)cells[3])/4.0) : v0;
*/
      int v1, v0;
      float cells[4], n;
      
      v1 = value;
      v0 = lastV;
      //if (v1 < v0) {
      //  //unsigned char x = (unsigned char)v0;
      //  
      //  v0 = v1;
      //  v1 = v0;
      //}
      //v1 = value;
      //v0 = lastV;
      n = 1;//outputScanLine[outP / (2 * colorsPerEntry) ];
      rMask = ((rMask & 0xFF) << 8) | ((rMask & 0xFF00) >> 8); // Swap bytes
      
      cells[0] = 0.0;
      cells[1] = 0.0;
      cells[2] = 0.0;
      cells[3] = 0.0;
      
      cells[0] += (((rMask & 0x0008) != 0)? 1.0 : 0.0); // 0 * colorsPerEntry + 2,3 * pixelStep
      cells[0] += (((rMask & 0x0080) != 0)? 1.0 : 0.0);
      cells[1] += (((rMask & 0x0800) != 0)? 1.0 : 0.0); // 0 * colorsPerEntry + 0,1 * pixelStep
      cells[1] += (((rMask & 0x8000) != 0)? 1.0 : 0.0);
      
      cells[0] += (((rMask & 0x0004) != 0)? 1.0 : 0.0); // 1 * colorsPerEntry + 2,3 * pixelStep
      cells[0] += (((rMask & 0x0040) != 0)? 1.0 : 0.0);
      cells[1] += (((rMask & 0x0400) != 0)? 1.0 : 0.0); // 1 * colorsPerEntry + 0,1 * pixelStep
      cells[1] += (((rMask & 0x4000) != 0)? 1.0 : 0.0);
      
      cells[2] += (((rMask & 0x0002) != 0)? 1.0 : 0.0); // 2 * colorsPerEntry + 2,3 * pixelStep
      cells[2] += (((rMask & 0x0020) != 0)? 1.0 : 0.0);
      cells[3] += (((rMask & 0x0200) != 0)? 1.0 : 0.0); // 2 * colorsPerEntry + 0,1 * pixelStep
      cells[3] += (((rMask & 0x2000) != 0)? 1.0 : 0.0);
      
      cells[2] += (((rMask & 0x0001) != 0)? 1.0 : 0.0); // 3 * colorsPerEntry + 2,3 * pixelStep
      cells[2] += (((rMask & 0x0010) != 0)? 1.0 : 0.0);
      cells[3] += (((rMask & 0x0100) != 0)? 1.0 : 0.0); // 3 * colorsPerEntry + 0,1 * pixelStep
      cells[3] += (((rMask & 0x1000) != 0)? 1.0 : 0.0);
      
      if (cells[0] == 1.0) {
        cells[0] = 0.25;
      }
      if (cells[1] == 1.0) {
        cells[1] = 0.25;
      }
      if (cells[2] == 1.0) {
        cells[2] = 0.25;
      }
      if (cells[3] == 1.0) {
        cells[3] = 0.25;
      }

      outBase[outP + (0 * colorsPerEntry) + (1 * pixelStep)] = (cells[0] >= n)? v1*(cells[0]/4.0) : v0;
      outBase[outP + (0 * colorsPerEntry) + (0 * pixelStep)] = (cells[1] >= n)? v1*(cells[1]/4.0) : v0;
      
      outBase[outP + (1 * colorsPerEntry) + (1 * pixelStep)] = (cells[2] >= n)? v1*(cells[2]/4.0) : v0;
      outBase[outP + (1 * colorsPerEntry) + (0 * pixelStep)] = (cells[3] >= n)? v1*(cells[3]/4.0) : v0;
      
      
      outP += 2 * colorsPerEntry;        
      break;
    }
    outBase[outP] = value;
    outBase[outP + pixelStep] = value;
    outP += colorsPerEntry;
    outBase[outP] = value;
    outBase[outP + pixelStep] = value;
    outP += colorsPerEntry;
    break;
  
  case 4:
    
    if (rMask < 0x10000) {
      int v1, v0;
      
      // Original
      v1 = value;
      v0 = lastV;
      //if (v1 < v0) {
        //unsigned char x = (unsigned char)v0;
        
        //v0 = v1;
        //v1 = v0;
      //}
      
      // Swaping bytes orients the stair step left and right... 
      //
      rMask = ((rMask & 0xFF) << 8) | ((rMask & 0xFF00) >> 8); // Swap bytes
     
      //if (isGB) {
      /*
      if ((rMask & 0x0008) != 0) {  outBase[outP + (0 * colorsPerEntry) + (3 * pixelStep)] = v1; }
      if ((rMask & 0x0080) != 0) {  outBase[outP + (0 * colorsPerEntry) + (2 * pixelStep)] = v1; }
      if ((rMask & 0x0800) != 0) {  outBase[outP + (0 * colorsPerEntry) + (1 * pixelStep)] = v1; }
      if ((rMask & 0x8000) != 0) {  outBase[outP + (0 * colorsPerEntry) + (0 * pixelStep)] = v1; }
      
      if ((rMask & 0x0004) != 0) {  outBase[outP + (1 * colorsPerEntry) + (3 * pixelStep)] = v1; }
      if ((rMask & 0x0040) != 0) {  outBase[outP + (1 * colorsPerEntry) + (2 * pixelStep)] = v1; }
      if ((rMask & 0x0400) != 0) {  outBase[outP + (1 * colorsPerEntry) + (1 * pixelStep)] = v1; }
      if ((rMask & 0x4000) != 0) {  outBase[outP + (1 * colorsPerEntry) + (0 * pixelStep)] = v1; }
              
      if ((rMask & 0x0002) != 0) {  outBase[outP + (2 * colorsPerEntry) + (3 * pixelStep)] = v1; }
      if ((rMask & 0x0020) != 0) {  outBase[outP + (2 * colorsPerEntry) + (2 * pixelStep)] = v1; }
      if ((rMask & 0x0200) != 0) {  outBase[outP + (2 * colorsPerEntry) + (1 * pixelStep)] = v1; }
      if ((rMask & 0x2000) != 0) {  outBase[outP + (2 * colorsPerEntry) + (0 * pixelStep)] = v1; }
        
      if ((rMask & 0x0001) != 0) {  outBase[outP + (3 * colorsPerEntry) + (3 * pixelStep)] = v1; }
      if ((rMask & 0x0010) != 0) {  outBase[outP + (3 * colorsPerEntry) + (2 * pixelStep)] = v1; }
      if ((rMask & 0x0100) != 0) {  outBase[outP + (3 * colorsPerEntry) + (1 * pixelStep)] = v1; }
      if ((rMask & 0x1000) != 0) {  outBase[outP + (3 * colorsPerEntry) + (0 * pixelStep)] = v1; }
/*
      } else {
        //v0 = 0;
              rMask = ((rMask & 0xFF) << 8) | ((rMask & 0xFF00) >> 8); // Swap bytes
              rows[0][0] = ((rMask & 0x0008) != 0)? 1 : 0
*/
      // top to bottom across page
      // 8 = leftmost ... 8000 = rightmost
      // [ 8 80 800 8000 ] -> left to right across page
      // [ 4 40 400 4000 ] 
      // [ 2 20 200 2000 ] 
      // [ 1 10 100 1000 ] 

        outBase[outP + (0 * colorsPerEntry) + (3 * pixelStep)] = (((rMask & 0x0008) != 0)? v1 : v0);
        outBase[outP + (0 * colorsPerEntry) + (2 * pixelStep)] = (((rMask & 0x0080) != 0)? v1 : v0);
        outBase[outP + (0 * colorsPerEntry) + (1 * pixelStep)] = (((rMask & 0x0800) != 0)? v1 : v0);
        outBase[outP + (0 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x8000) != 0)? v1 : v0);

      if (isGB) {
         outBase[outP + (0 * colorsPerEntry) + (3 * pixelStep)] = (((rMask & 0x0008) != 0)? (isGB? 255 : 0) : (isGB? 255 : 0));
      }
      //outBase[outP + (0 * colorsPerEntry) + (2 * pixelStep)] = (((rMask & 0x0080) != 0)? 64 : v0);
      //outBase[outP + (0 * colorsPerEntry) + (1 * pixelStep)] = (((rMask & 0x0800) != 0)? 32 : v0);
      //outBase[outP + (0 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x8000) != 0)? 16 : v0);
      
        
        outBase[outP + (1 * colorsPerEntry) + (3 * pixelStep)] = (((rMask & 0x0004) != 0)? v1 : v0);
        outBase[outP + (1 * colorsPerEntry) + (2 * pixelStep)] = (((rMask & 0x0040) != 0)? v1 : v0);
        outBase[outP + (1 * colorsPerEntry) + (1 * pixelStep)] = (((rMask & 0x0400) != 0)? v1 : v0);
        outBase[outP + (1 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x4000) != 0)? v1 : v0);
      //outBase[outP + (1 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x4000) != 0)? 32 : v0);
        
        outBase[outP + (2 * colorsPerEntry) + (3 * pixelStep)] = (((rMask & 0x0002) != 0)? v1 : v0);
        outBase[outP + (2 * colorsPerEntry) + (2 * pixelStep)] = (((rMask & 0x0020) != 0)? v1 : v0);
        outBase[outP + (2 * colorsPerEntry) + (1 * pixelStep)] = (((rMask & 0x0200) != 0)? v1 : v0);
        outBase[outP + (2 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x2000) != 0)? v1 : v0);
      //outBase[outP + (2 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x2000) != 0)? 64 : v0);
        
        outBase[outP + (3 * colorsPerEntry) + (3 * pixelStep)] = (((rMask & 0x0001) != 0)? v1 : v0);
        outBase[outP + (3 * colorsPerEntry) + (2 * pixelStep)] = (((rMask & 0x0010) != 0)? v1 : v0);
        outBase[outP + (3 * colorsPerEntry) + (1 * pixelStep)] = (((rMask & 0x0100) != 0)? v1 : v0);
        outBase[outP + (3 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x1000) != 0)? v1 : v0);
      //outBase[outP + (3 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x1000) != 0)? 127 : v0);
      
      //outBase[outP + (3 * colorsPerEntry) + (3 * pixelStep)] = (((rMask & 0x0001) != 0)? 64 : v0);
      //outBase[outP + (3 * colorsPerEntry) + (2 * pixelStep)] = (((rMask & 0x0010) != 0)? 64 : v0);
      //outBase[outP + (3 * colorsPerEntry) + (1 * pixelStep)] = (((rMask & 0x0100) != 0)? 64 : v0);
      //outBase[outP + (3 * colorsPerEntry) + (0 * pixelStep)] = (((rMask & 0x1000) != 0)? 64 : v0);
      /*    }
      */
      
      outP += 4 * colorsPerEntry;    
      
      return 0;
    }
    //value = 0xFF & ((rMask & 0xFF0000) >> 16);
    outBase[outP] = value;
    outBase[outP + pixelStep] = value;
    outBase[outP + (2 * pixelStep)] = value;
    outBase[outP + (3 * pixelStep)] = value;
    outP += colorsPerEntry;
    outBase[outP] = value;
    outBase[outP + pixelStep] = value;
    outBase[outP + (2 * pixelStep)] = value;
    outBase[outP + (3 * pixelStep)] = value;
    outP += colorsPerEntry;
    outBase[outP] = value;
    outBase[outP + pixelStep] = value;
    outBase[outP + (2 * pixelStep)] = value;
    outBase[outP + (3 * pixelStep)] = value;
    outP += colorsPerEntry;
    outBase[outP] = value;
    outBase[outP + pixelStep] = value;
    outBase[outP + (2 * pixelStep)] = value;
    outBase[outP + (3 * pixelStep)] = value;
    outP += colorsPerEntry;
    break;
  }
  
  return 0;
}

int nextOpType(unsigned char * bufBase, int nextRoot) {
  int nextOp;
  
  if (bufBase[nextRoot] == 0xFF) {
    if (bufBase[nextRoot + 1] == 0xFF) {
      nextOp = opFFFF;
    } else {
      nextOp = opFF;
    }
  } else if (bufBase[nextRoot] == 0xFC) {
    nextOp = opFC;
  } else if ((bufBase[nextRoot] & 0x80) == 0x80) {
    nextOp = opSHORT_GB;
  } else if (bufBase[nextRoot] == 0x00) {
    nextOp = opZRO;
  } else {
    nextOp = opNONE;        
  }
  
  return nextOp;
}

typedef char shitT[4][5];

void hackery1(shitT& rows, int rMask) {
  rMask = ((rMask & 0xFF) << 8) | ((rMask & 0xFF00) >> 8); // Swap bytes
  rows[0][0] = ((rMask & 0x0008) != 0)? 'X' : '0';
  rows[0][1] = ((rMask & 0x0080) != 0)? 'X' : '0';
  rows[0][2] = ((rMask & 0x0800) != 0)? 'X' : '0';
  rows[0][3] = ((rMask & 0x8000) != 0)? 'X' : '0';
  rows[0][4] = 0;
  
  rows[1][0] = ((rMask & 0x0004) != 0)? 'X' : '0';
  rows[1][1] = ((rMask & 0x0040) != 0)? 'X' : '0';
  rows[1][2] = ((rMask & 0x0400) != 0)? 'X' : '0';
  rows[1][3] = ((rMask & 0x4000) != 0)? 'X' : '0';
  rows[1][4] = 0;
  
  rows[2][0] = ((rMask & 0x0002) != 0)? 'X' : '0';
  rows[2][1] = ((rMask & 0x0020) != 0)? 'X' : '0';
  rows[2][2] = ((rMask & 0x0200) != 0)? 'X' : '0';
  rows[2][3] = ((rMask & 0x2000) != 0)? 'X' : '0';
  rows[2][4] = 0;
  
  rows[3][0] = ((rMask & 0x0001) != 0)? 'X' : '0';
  rows[3][1] = ((rMask & 0x0010) != 0)? 'X' : '0';
  rows[3][2] = ((rMask & 0x0100) != 0)? 'X' : '0';
  rows[3][3] = ((rMask & 0x1000) != 0)? 'X' : '0';
  rows[3][4] = 0;
}

int nextRealColorValue(unsigned char * bufBase, int nextRoot, int prevColor, int maxDepth) {
  int nextOp;
  
  if (bufBase[nextRoot] == 0xFF) {
    if (bufBase[nextRoot + 1] == 0xFF) {
      nextOp = opFFFF;
    } else {
      nextOp = opFF;
    }
  } else if (bufBase[nextRoot] == 0xFC) {
    nextOp = opFC;
  } else if ((bufBase[nextRoot] & 0x80) == 0x80) {
    nextOp = opSHORT_GB;
  } else if (bufBase[nextRoot] == 0x00) {
    nextOp = opZRO;
  } else {
    nextOp = opNONE;        
  }
  
  switch (nextOp) {
    case opFFFF: 
    case opZRO:
      return 0;
      
    case opSHORT_GB:
      if (maxDepth - 1 == 0) {
        return 32 + 256 - ((unsigned char)(bufBase[nextRoot] & 0xFF));//prevColor;//
      }
      return nextRealColorValue(bufBase, nextRoot + 3, prevColor, maxDepth - 1);
        
    case opFC:
      return nextRealColorValue(bufBase, nextRoot + 2, prevColor, maxDepth - 1);

    default:
    case opFF:
      //ASSERT(false);
      return bufBase[nextRoot] & 0xFF;
      
    case opNONE:
      return bufBase[nextRoot] & 0xFF;
  }
}

bool approximatelyEqual(int delta, unsigned char ax, unsigned char bx, unsigned char cx) {
  int d1, d2, d3, a, b, c;
  
  a = ax & 0xFF;
  b = bx & 0xFF;
  c = cx & 0xFF;
  d1 = (a > b? a - b : b - a);
  d2 = (a > c? a - c : a - c);
  d3 = (b > c? b - c : b - c);
  
  return d1 < delta && d2 < delta && d3 < delta;
}

int colorRXHack(int i, int j, int actual) {
  if (i >= 2285 && i <= 2285 && j > 44) {
    return actual;
  }
  return actual;
}

bool indImageGrossC::processImage(int s, int presSim, bool skipLoad) {
  unsigned char * outBase, currentPixInput[1], * lastValue;
  int i, j, r, outP, op, count, lastOp, cspan, columnStep, pixelStep, lastMask, lastVX, lastGValue;
  unsigned char top, bottom;
  FILE * mF;
  LPCTSTR lastMF;
  bool lastGB;
  unsigned char * gBmask;
  unsigned char baseColorMax[4];
  int iSecretStep, iSecretC, iSecretM, iSecretY, iSecretK;
  
  srand ( time(NULL) );
  
  iSecretStep = (rand() % 100) + 75;
  iSecretC = 128 + 50 - (rand() % 50);
  iSecretM = 128 + 50 - (rand() % 50);
  iSecretY = 128 + 50 - (rand() % 50);
  iSecretK = 128 + 50 - (rand() % 50);
  
  setOutputBufferScale(s);
  setPressSim(presSim);
  
  oHeight = 16 * (((outputBufferScale * height) + 15) / 16);
  inputScanLineLength = colorsPerEntry * oHeight;
  maskScanLineLength = inputScanLineLength;
  outputScanLineLength = oHeight;
  
  if (inputScanLine == NULL) {
    int maxOHeight;
    
    maxOHeight = 16 * (((4 * height) + 15) / 16);
    inputScanLine = new char[colorsPerEntry * maxOHeight];
    maskScanLine = new char[colorsPerEntry * maxOHeight];

    outputScanLine = new char[maxOHeight];
  }
  
  if (outputBufferSize == 0)
  {
    delete outputBuffer;
    outputBuffer = new char[colorsPerEntry * (outputBufferScale * (width + outputBufferScale)) * (oHeight + outputBufferScale)];
  }

  outBase = (unsigned char *)outputBuffer;
  
  pixelStep = colorsPerEntry * oHeight;
  columnStep = outputBufferScale * pixelStep;
  
  if (plane == indColorFirst) {
    colorValueMin = 255;
    colorValueMax = 0;
    
    memset(outBase, 0, colorsPerEntry * (outputBufferScale * width) * oHeight);
  }
    
  if (plane == indColorK) {
    gBmask = new unsigned char[inputScanLineLength];  
  }
  
  colorPlaneValueMin[plane] = 255;
  colorPlaneValueMax[plane] = 0;
  for (i = 0; i < 256; i++) {
    colorValueRange[plane][i] = 0;
  }
  
  cspan = 0;
  lastVX = 0;
  lastGValue = 0;
  
  if (!renderPlane[plane]) {
    if (plane == indColorK) {
      delete gBmask;
    }
    return true;
  }
  
  mF = NULL;
  if (dumpTextRaster && mF == NULL) {
    pdfXString path;
     
    path.Format((LPCTSTR)(textRenditionPath + textRenditionTemplate), subImageCounter + 1, getPlaneAsString());
    mF = fopen((LPCTSTR)path, "w");
    if (mF != NULL) {
      int col, mm, tens, huns, thous;
      
      ///
      mm = 0;
      for (col = 0; col < oHeight; col++) {
        fprintf(mF, "%d", mm);
        mm += 1;
        if (mm > 9) {
          mm = 0;
        }
      }
      fprintf(mF, "\n");

      ///
      mm = 0;
      tens = 1;
      for (col = 0; col < oHeight; col++) {
        if (mm != 0 && (mm % 10 == 0)) {
          fprintf(mF, "%d", tens);
          tens += 1;
          if (tens > 9) {
            tens = 0;
          }
          if (mm > 9) {
            mm = 0;
          }
        } else {
          fprintf(mF, " ");
        }
        mm += 1;
      }
      fprintf(mF, "\n");
      
      ///
      mm = 0;
      huns = 1;
      for (col = 0; col < oHeight; col++) {
        if (mm != 0 && (mm % 100 == 0)) {
          fprintf(mF, "%d", huns);
          huns += 1;
          if (huns > 9) {
            huns = 0;
          }
          if (mm > 9) {
            mm = 0;
          }
        } else {
          fprintf(mF, " ");
        }
        mm += 1;
      }
      fprintf(mF, "\n");

      ///
      mm = 0;
      thous = 1;
      for (col = 0; col < oHeight; col++) {
        if (mm != 0 && (mm % 1000 == 0)) {
          fprintf(mF, "%d", thous);
          thous += 1;
          if (thous > 9) {
            thous = 0;
          }
          if (mm > 9) {
            mm = 0;
          }
        } else {
          fprintf(mF, " ");
        }
        mm += 1;
      }
      fprintf(mF, "\n");
      
    }
  }
  
  if (skipLoad) {
    goto skipLoading;
  }
  
  for (i = 0; i < width; i++) {

    if (i < minRenderColumn || i > maxRenderColumn) {
      continue;
    }
    
    lastVX = 0;
    lastGValue = 0;
    pass = 0;
    memset(inputScanLine, 0, inputScanLineLength);
    memset(maskScanLine, 0, maskScanLineLength);
    memset(outputScanLine, 2, outputScanLineLength);
    if (plane == indColorK) {
      memset(gBmask, 0, inputScanLineLength);
    }
    
nextPass:

    workingCol = i;
    colStarted = true;
    outP = plane;
    
    lastOp = opNONE;
    lastMask = 0xFFFF;
    lastGB = false;
    
    if (mF != NULL && pass == 0) {
      fprintf(mF, "%05d: ", i);
    }

    if (indVersion == indIC05FileFormat) {
      for (j = 0; j < (int)counts[i]; j++) {
        
        if (starts[i][j] == 0xFF) {
          if (starts[i][j + 1] == 0xFF) {
            op = opFFFF;
          } else {
            op = opFF;
          }
        } else if (starts[i][j] == 0xFC) {
          op = opFC;
        } else if ((starts[i][j] & 0x80) == 0x80) {
          op = opSHORT_GB;
        } else if (starts[i][j] == 0x00) {
          op = opZRO;
        } else {
          op = opNONE;        
        }
        
        switch( op ) {
          case opFFFF:      
            if (mF != NULL && pass == 0) {
              lastMF = "F";
              fprintf(mF, "%s", lastMF);
            }
            
            j += 1; 
            currentPixInput[0] = 0x00;
            lastValue = &currentPixInput[0];
            writePixel(outBase, outP, 0x00, outputBufferScale, pixelStep, lastMask = 0xFFFF, lastGB = false, lastVX = 0x00);
            colorValueRange[plane][currentPixInput[0]] += 1;
            break;
            
          case opZRO:
            if (mF != NULL && pass == 0) {
              lastMF = " ";
              fprintf(mF, "%s", lastMF);
            }
            
            currentPixInput[0] = 0x00;
            lastValue = &currentPixInput[0];
            writePixel(outBase, outP, 0x00, outputBufferScale, pixelStep, lastMask = 0xFFFF, lastGB = false, lastVX = 0x00);
            colorValueRange[plane][outBase[outP]] += 1;
            lastMask = 0xFFFF;
            break;
            
          case opFC:
            // FC [count] - pick up the count, if count = 0, its 0x100, else its count + 1
            //
            j += 1;
            count = starts[i][j] & 0xFF;
            
            if (count == 0x00) {
              count = 0x100;
            } else {
              count += 1;
            }
              
            // if lastOp was a special op, reduce count by 1
            //
            if (lastOp == opFFFF || lastOp == opZRO || lastOp == opNONE || lastOp == opFF || lastOp == opSHORT_GB || lastOp == opSHORT_SCR) {
              count -= 1;
            }
              
            colorValueRange[plane][lastValue[0]] += count;
            
            for (r = 0; r < count; r++) {
              writePixel(outBase, outP, lastValue[0], outputBufferScale, pixelStep, lastMask, lastGB, lastVX);
              colorValueRange[plane][lastValue[0]] += 1;  
              
              if (mF != NULL && pass == 0) {
                fprintf(mF, "%s", r == 0? "R" : lastMF);
              }
            }
            break;
            
          case opFF:
            if (mF != NULL && pass == 0) {
              lastMF = (starts[i][j] & 0xFF) == 0? "Z" : "Q";
              fprintf(mF, "%s", lastMF);
            }
            
            j += 1;
            
            currentPixInput[0] = (starts[i][j] & 0xFF);
            lastValue = &currentPixInput[0];
            
            writePixel(outBase, 
                       outP, 
                       (starts[i][j] & 0xFF), 
                       outputBufferScale, 
                       pixelStep, 
                       lastMask = 0xFFFF & (*(unsigned short int *)&starts[i][j + 1]), 
                       lastGB = false,
                       lastVX);
            colorValueRange[plane][lastValue[0]] += 1;  
            
            j += 2;
            //if (indVersion == indIC05FileFormat) {
            //  j += 1;
            //}
            break;
            
          case opSHORT_GB:
            if (mF != NULL && pass == 0) {
              lastMF = "G";
              //if (indVersion == indIC05FileFormat && (starts[i][j] & 0xFF) >= 0xD0 && (starts[i][j] & 0xFF) <= 0xDF) {
              //  lastMF = "D";
              //}
              //if (indVersion == indIC05FileFormat && (starts[i][j] & 0xFF) >= 0xE0 && (starts[i][j] & 0xFF) <= 0xEF) {
              //  lastMF = "'";
              //}
              fprintf(mF, "%s", lastMF);
            }
            
            
            //if ((pressSim & indPressSimDropStrokeWhiteBlack) != 0) {
            //  currentPixInput[0] = ((plane == indColorK? 0 : 32) +  256 - ((unsigned char)(starts[i][j] & 0xFF)));
            //} else {
              currentPixInput[0] = 32 +  256 - ((unsigned char)(starts[i][j] & 0xFF));
            //}
            
            top = currentPixInput[0];
            bottom = lastVX;
            lastValue = &currentPixInput[0];
            
            //if (plane == indColorK && pass == 0 && (pressSim & indPressSimDropStrokeWhiteBlack) != 0) {
            //  gBmask[outP] = 1;
            //}
              writePixel(outBase, 
                         outP, 
                         top,
                         outputBufferScale, 
                         pixelStep, 
                         lastMask = 0xFFFF & (*(unsigned short int *)&starts[i][j + 1]),  
//                         indVersion == indIC05FileFormat? lastMask = 0xFFFF : lastMask = 0xFFFF & (*(unsigned short int *)&starts[i][j + 1]),  
                         lastGB = true,
                         currentPixInput[0]); // originally bottom
            colorValueRange[plane][lastValue[0]] += 1;  
            lastGValue = top; 
            
            j += 2;
            
            //if (indVersion == indIC05FileFormat && (starts[i][j - 2] & 0xFF) >= 0xD0 && (starts[i][j - 2] & 0xFF) <= 0xDF) {
            //  j -= 1;
            //} else if (indVersion == indIC05FileFormat && (starts[i][j - 2] & 0xFF) >= 0xE0 && (starts[i][j - 2] & 0xFF) <= 0xE7) {
            //  int chunky;
            //  
            //  chunky = (starts[i][j - 2] & 0x7) + 1;
            //  j -= 1;
            //  
            //  
            //  for (r = 0; r < chunky; r++) {
            //    writePixel(outBase, outP, lastValue[0], outputBufferScale, pixelStep, lastMask, lastGB, lastVX);
            ///    colorValueRange[plane][lastValue[0]] += 1;  
            //    
            //    if (mF != NULL && pass == 0) {
            //      fprintf(mF, "%s", r == 0? "R" : lastMF);
            //    }
            //  }
            //  op = opSHORT_SCR;
            //} else if (indVersion == indIC05FileFormat && (starts[i][j - 2] & 0xFF) >= 0xF0 && (starts[i][j - 2] & 0xFF) <= 0xF7) {
            //  j -= 1;
            //}
              break;
            
          default:
          case opNONE:
            if (mF != NULL && pass == 0) {
              lastMF = ".";
              fprintf(mF, "%s", lastMF);
            }
            currentPixInput[0] = starts[i][j];
            lastValue = &currentPixInput[0];
            
            writePixel(outBase, outP, starts[i][j] & 0xFF, outputBufferScale, pixelStep, lastMask = 0xFFFF, lastGB = false, lastVX = starts[i][j] & 0xFF);
            colorValueRange[plane][lastValue[0]] += 1;  
            break;
        }
        
        lastOp = op;
      }
    } else {
      int maskedStarts, previousColor, currentColor;
      
      previousColor = -1;
      currentColor = -1;
      //
      // Begin: Optimized for 800 DPI Indigo
      //
      for (j = 0; j < (int)counts[i]; j++) {
        
        if (starts[i][j] == 0xFF) {
          if (starts[i][j + 1] == 0xFF) {
            op = opFFFF;
          } else {
            op = opFF;
          }
        } else if (starts[i][j] == 0xFC) {
          op = opFC;
        } else if ((starts[i][j] & 0x80) == 0x80) {
          op = opSHORT_GB;
        } else if (starts[i][j] == 0x00) {
          op = opZRO;
        } else {
          op = opNONE;        
        }
                
        maskedStarts = starts[i][j] & 0xFF;
        
        switch( op ) {
          case opFFFF:      
#ifdef NON_PRODUCTION
            if (mF != NULL && pass == 0) {
              lastMF = "F";
              fprintf(mF, "%s", lastMF);
            }
#endif // NON_PRODUCTION
            
            j += 1; 
            currentPixInput[0] = colorRXHack(i, j, 0x00);
            
            previousColor = currentColor;
            currentColor = currentPixInput[0];
            
            lastValue = &currentPixInput[0];
            writePixel(outBase, outP, currentPixInput[0], outputBufferScale, pixelStep, lastMask = 0x0F0000, lastGB = false, lastVX = currentPixInput[0]);
            colorValueRange[plane][currentPixInput[0]] += 1;
            break;
            
          case opZRO:
#ifdef NON_PRODUCTION
            if (mF != NULL && pass == 0) {
              lastMF = " ";
              fprintf(mF, "%s", lastMF);
            }
#endif // NON_PRODUCTION
            
            currentPixInput[0] = colorRXHack(i, j, 0x00);

            previousColor = currentColor;
            currentColor = currentPixInput[0];

            lastValue = &currentPixInput[0];
            writePixel(outBase, outP, currentPixInput[0], outputBufferScale, pixelStep, lastMask = 0x1F0000, lastGB = false, lastVX = currentPixInput[0]);
            colorValueRange[plane][outBase[outP]] += 1;
            break;
            
          case opFC:
            j += 1;
            
            count = starts[i][j] & 0xFF;
            
            if (count == 0x00) {
              count = 0x100;
            } else {
              count += 1;
            }
              
            if (lastOp == opFFFF || lastOp == opZRO || lastOp == opNONE || lastOp == opFF || lastOp == opSHORT_GB || lastOp == opSHORT_SCR) {
              count -= 1;
            }
              
            colorValueRange[plane][lastValue[0]] += count;
            
            for (r = 0; r < count; r++) {
              writePixel(outBase, outP, lastValue[0], outputBufferScale, pixelStep, lastMask, lastGB, lastVX/*starts[i][j] & 0xFF*/); ///// ? ? /////
              colorValueRange[plane][lastValue[0]] += 1;  
              
#ifdef NON_PRODUCTION
              if (mF != NULL && pass == 0) {
                fprintf(mF, "%s", r == 0? "R" : lastMF);
              }
#endif // NON_PRODUCTION
            }
            break;
            
          case opFF:

#ifdef NON_PRODUCTION
            if (mF != NULL && pass == 0) {
              lastMF = (starts[i][j] & 0xFF) == 0? "Z" : "Q";
              fprintf(mF, "%s", lastMF);
            }
#endif // NON_PRODUCTION
            
            //ASSERT(false);
            j += 1;
            
            currentPixInput[0] = colorRXHack(i, j, starts[i][j] & 0xFF);
            
            //currentPixInput[0] = (starts[i][j] & 0xFF);

            lastValue = &currentPixInput[0];
            
            writePixel(outBase, 
                       outP, 
                       currentPixInput[0],//32 + 256 - (unsigned char)(starts[i][j] & 0xFF), 
                       outputBufferScale, 
                       pixelStep, 
                       lastMask = 0xFFFF & (*(unsigned short int *)&starts[i][j + 1]), 
                       lastGB = false,
                       lastVX = 0 /*starts[i][j] & 0xFF*/); ///////
            colorValueRange[plane][lastValue[0]] += 1;  
            
            j += 2;
            break;
            
          case opSHORT_GB:
#ifdef NON_PRODUCTION
            if (mF != NULL && pass == 0) {
              lastMF = "G";
              fprintf(mF, "%s", lastMF);
            }
#endif // NON_PRODUCTION
          
            {
              int nextColor, xtmp, rMask, prevXC, dumpColor;
              bool trigger;
              
              xtmp = lastValue[0];
              currentPixInput[0] = currentColor;            
              lastValue = &currentPixInput[0];
              
              trigger = false;
              lastMask = 0xFFFF & (*(unsigned short int *)&starts[i][j + 1]);

              prevXC = currentColor;

              if (plane != indColorK) {
                bool topIn, bottomOut, runLeft, runRight;
                int oNextColor;
              
                nextColor = 256 - (starts[i][j] & 0xFF);
                oNextColor = nextColor;
                            
                if (currentColor <= nextColor) {
                  int tmp;
                
                  tmp = currentColor;
                  currentColor = nextColor + 32;
                  nextColor = tmp;
                } else {
                  trigger = true;
                }
              
                //shitT rows;
                //hackery1(rows, lastMask);
              
                dumpColor = nextColor;
              
                rMask = ((lastMask & 0xFF) << 8) | ((lastMask & 0xFF00) >> 8); // Swap bytes
                topIn = (((rMask & 0x8000) == 0) && ((rMask & 0x0800) == 0) && ((rMask & 0x0080) == 0) && ((rMask & 0x0008) == 0));
                bottomOut = (((rMask & 0x1000) == 0) && ((rMask & 0x0100) == 0) && ((rMask & 0x0010) == 0) && ((rMask & 0x0001) == 0));
                
                runLeft = (((rMask & 0x8000) == 0) && ((rMask & 0x4000) == 0) && ((rMask & 0x2000) == 0) && ((rMask & 0x1000) == 0));
                runRight = (((rMask & 0x0008) == 0) && ((rMask & 0x0004) == 0) && ((rMask & 0x0002) == 0) && ((rMask & 0x0001) == 0));
                
                if ((nextOpType(starts[i], j + 2) == opSHORT_GB && (bottomOut || topIn) && currentColor > dumpColor)) {
                  if (topIn) {
                    dumpColor = prevXC;
                  } 
                  
                  if (bottomOut && (nextOpType(starts[i], j + 2) != opSHORT_GB)) {
                    dumpColor = nextRealColorValue(starts[i], j + 3, 0, 1);
                  }
                }
                 
                if (lastOp == opSHORT_GB && runLeft != runRight && !topIn && !bottomOut) {
                  writePixel(outBase, 
                             outP, 
                             prevXC,
                             outputBufferScale, 
                             pixelStep, 
                             lastMask,  
                             lastGB /*= (nextOpType(starts[i], j + 2) == opSHORT_GB && (!bottomOut || !topIn)) */,
                             lastVX = ((nextOpType(starts[i], j + 2) == opSHORT_GB && (!bottomOut || !topIn)) ? oNextColor : lastVX)
                             );
                  currentColor = prevXC;
                  lastValue[0] = currentColor;
                } else {
                  if (lastOp == opSHORT_GB) {
                    writePixel(outBase, 
                               outP, 
                               currentColor,
                               outputBufferScale, 
                               pixelStep, 
                               lastMask,  
                               lastGB = (nextOpType(starts[i], j + 2) != opSHORT_GB),/*= (nextOpType(starts[i], j + 2) == opSHORT_GB && (bottomOut || topIn) && currentColor < dumpColor), */
                               dumpColor// = (nextOpType(starts[i], j + 2) != opSHORT_GB)? lastVX/*nextRealColorValue(starts[i], j + 3, 0, 1)*/ : dumpColor
                               );
                  } else {
                    writePixel(outBase, 
                               outP, 
                               currentColor,
                               outputBufferScale, 
                               pixelStep, 
                               lastMask,  
                               lastGB,/*= (nextOpType(starts[i], j + 2) == opSHORT_GB && (bottomOut || topIn) && currentColor < dumpColor), */
                               dumpColor = (nextOpType(starts[i], j + 2) == opSHORT_GB && (bottomOut || topIn) /* && currentColor > dumpColor */)? nextRealColorValue(starts[i], j + 3, 0, 1) : dumpColor
                               );
                  }
                  if (nextOpType(starts[i], j + 2) == opSHORT_GB && bottomOut) {
                    lastVX = currentColor;
                    currentColor = dumpColor;                    
                  } else {
                    lastVX = dumpColor;
                  }
                  lastValue[0] = currentColor;
                }
                
              } else {
                int nextColor, foo;
                  
                trigger = false;
                  
                foo = starts[i][j] & 0xFF;
                if (starts[i][j] & 0x80) {
                  foo |= 0xFFFFFF00;
                }
                  
                nextColor = 0xFF & ((currentColor + foo) - 16);   // K == 16
                  
                if (nextColor & 0x80) {
                  nextColor = 128 - (0x7F & nextColor);
                }
                  
                lastMask = 0xFFFF & (*(unsigned short int *)&starts[i][j + 1]);
                  
                if (currentColor < nextColor) {
                  lastMask = ~lastMask;
                }
                  
                dumpColor = nextColor;
                  
                writePixel(outBase, 
                           outP, 
                           currentColor,
                           outputBufferScale, 
                           pixelStep, 
                           lastMask,  
                           lastGB = trigger,
                           dumpColor);
                  
                lastVX = nextColor;
                lastValue[0] = currentColor;
              }
            }
            
            colorValueRange[plane][lastValue[0]] += 1;  
            lastGValue = currentPixInput[0]; 
            
            j += 2;
            break;
            
          default:
          case opNONE:
#ifdef NON_PRODUCTION
            if (mF != NULL && pass == 0) {
              lastMF = ".";
              fprintf(mF, "%s", lastMF);
            }
#endif // NON_PRODUCTION

            currentPixInput[0] = colorRXHack(i, j, starts[i][j] & 0xFF);
            
            previousColor = currentColor;
            currentColor = currentPixInput[0];
            
            lastValue = &currentPixInput[0];
            
            writePixel(outBase, outP, currentPixInput[0], outputBufferScale, pixelStep, lastMask = 0x3F0000, lastGB = false, lastVX = currentPixInput[0]);
            colorValueRange[plane][lastValue[0]] += 1;  
            break;
        }

        lastOp = op;
      }
      
      //
      // End: Optimized for 800 DPI Indigo
      //
    }
    

    pass += 1;

    outBase += columnStep;  
    
    if (mF != NULL) {
      fprintf(mF, "\n");
    }
  }
  
skipLoading:
  
  baseColorMax[indColorC] = 0;
  baseColorMax[indColorM] = 0;
  baseColorMax[indColorY] = 0;
  baseColorMax[indColorK] = 0;
  
  //if ((pressSim & indPressSimDropStrokeWhiteBlack) != 0 && plane == indColorK) {
  if (applyConvolution && plane == indColorK) {
    convolve1((unsigned char *)outputBuffer, 
              colorsPerEntry,
              outputBufferScale,
              oHeight, 
              width, 
              0.0,
              baseColorMax,
              convolutionMixAmount);
  }
         
  for (colorPlaneValueMin[plane] = 0; colorPlaneValueMin[plane] < 256; colorPlaneValueMin[plane]++) {
    if (colorValueRange[plane][colorPlaneValueMin[plane]] != 0) {
      break;
    }
  }
  
  if (colorPlaneValueMin[plane] < colorValueMin) {
    colorValueMin = colorPlaneValueMin[plane];
  }
  
  for (colorPlaneValueMax[plane] = 255; colorPlaneValueMax[plane] >= 0; colorPlaneValueMax[plane]--) {
    if (colorValueRange[plane][colorPlaneValueMax[plane]] != 0) {
      break;
    }
  }
  
  if (colorPlaneValueMax[plane] > colorValueMax) {
    colorValueMax = colorPlaneValueMax[plane];
  }
  
  if (true && !skipLoad && plane == indColorK) {
    float levelComp;
    pdfXString error;
    int jx, max, min;
    
    max = 0;
    min = 1000;
    for (jx = 0; jx <= plane; jx++) {
      if (colorPlaneValueMax[jx] > max) {
        max = colorPlaneValueMax[jx];
      }
      if (colorPlaneValueMin[jx] < min) {
        min = colorPlaneValueMin[jx];
      }
    }
    
    levelComp = (float)(255.0 / ((float)max));
    levelComp -= (float)(levelComp * levelMargin * 5.0);
      
    error.Format("Image %d has color range: %d..%d", subImageCounter + 1, min, max);
    addToLogFile(lightLOG_STATUS, error);
  
    outBase = (unsigned char *)outputBuffer;
    if (performAutoLevel) {
      error.Format("Adjusting image levels.");
      addToLogFile(lightLOG_STATUS, error);

      for (i = 0; i < colorsPerEntry * (outputBufferScale * width) * oHeight; i++) {
        if (outBase[i] > 10) {
          outBase[i] = (unsigned char)(((float)outBase[i]) * levelComp);
        }
      }
    }
    /*
#ifdef PRODUCT_DEMO_BUILD
    int tick;
    
    tick = 40 + (10 - rand() % 20);
    for (i = 0; i < colorsPerEntry * (outputBufferScale * width) * oHeight; i++) {
      if ((i % iSecretStep == 0) || (i % iSecretStep) == 1 || (i % iSecretStep) == 2 || (i % iSecretStep) == 3) {
        tick -= 1;
        
        if (tick <= 0) {
          tick = 40 + (10 - rand() % 20);
          iSecretC = 128 + 50 - (rand() % 50);
          iSecretM = 128 + 50 - (rand() % 50);
          iSecretY = 128 + 50 - (rand() % 50);
          iSecretK = 128 + 50 - (rand() % 50);
        }
        if (i % 4 == 0) {
          if (outBase[i] < iSecretC) {
            outBase[i] = iSecretC;
            outBase[i + 4] += iSecretC / 2;
          } else {
            outBase[i] += (unsigned char)(.30*outBase[i]);
            outBase[i - 4] += (unsigned char)(.15*outBase[i - 4]);
          }
        }
        if (i % 4 == 1) {
          if (outBase[i] < iSecretM) {
            outBase[i] = iSecretM;
            outBase[i - 16] += iSecretY / 5;
            outBase[i + 16] -= iSecretY / 5;
          } else {
            outBase[i] += (unsigned char)(.35*outBase[i]);
            outBase[i + 8 - 3] = 0;
            outBase[i + 8 - 2] = 0;
            outBase[i + 8 - 1] = 0;
            outBase[i+ 8] = 0;
            outBase[i + 4 - 3] = 0;
            outBase[i + 4 - 2] = 0;
            outBase[i + 4 - 1] = 0;
            outBase[i + 4] = 0;
          }
        }
        if (i % 4 == 2) {
          if (outBase[i] < iSecretY) {
            outBase[i] = iSecretY;
            outBase[i - 8] += iSecretY;
            outBase[i + 8] -= iSecretY;
          } else {
            outBase[i] -= (unsigned char)(.35*outBase[i]);
            outBase[i + 4] += (unsigned char)(.5*outBase[i + 4]);
            outBase[i - 8] += (unsigned char)(.5*outBase[i - 4]);
            outBase[i + 4 - 3] = 0;
            outBase[i + 4 - 2] = 0;
            outBase[i + 4 - 1] = 0;
            outBase[i + 4] = 0;
          }
        }
        if (i % 4 == 3) {
          if (outBase[i] < iSecretK) {
            outBase[i] = iSecretK;
          } else {
            outBase[i - 3] = 0;
            outBase[i - 2] = 0;
            outBase[i - 1] = 0;
            outBase[i] = 0;
            outBase[i + 8 - 3] = 0;
            outBase[i + 8 - 2] = 0;
            outBase[i + 8 - 1] = 0;
            outBase[i+ 8] = 0;
            outBase[i - 8 - 3] = 0;
            outBase[i - 8 - 2] = 0;
            outBase[i - 8 - 1] = 0;
            outBase[i - 8] = 0;
            outBase[i + 4] += (unsigned char)(.25*outBase[i + 4]);
            outBase[i - 4] -= (unsigned char)(.25*outBase[i - 4]);
          }
        }
      }
    }
#endif // PRODUCT_DEMO_BUILD 
    */
  }  

  if (mF != NULL) {
    fprintf(mF, "Layer %s\n", getPlaneAsString());
    for (i = 0; i < 256; i++) {
      char buf[4];
      
      buf[0] = ' ';
      buf[1] = ' ';
      buf[2] = ' ';
      buf[3] = '\0';
      if (i == colorPlaneValueMin[plane]) {
        buf[0] = '-';
      }
      if (i == colorPlaneValueMax[plane]) {
        buf[0] = '+';
      }
      fprintf(mF, "%03d: %3s %d\n", i, buf, colorValueRange[plane][i]);
    }
    fclose(mF);
  }
  
  if (plane == indColorK) {
    delete gBmask;
  }
  
  return true;
}

static double indColorMin(double x, double y) {
  return x < y ? x : y;
} 

void indRGB_2_CMYK(double * RGB, double * CMYK, int styleCMYKRGB /* = indColorStyleAdobeCMYK2RGB */) {
  double C, M, Y, K, BG, UCR, NOT_UCR, denom, v;
  
  C = 1.0f - RGB[indColorR];
  M = 1.0f - RGB[indColorG];
  Y = 1.0f - RGB[indColorB];
  K = C < M ? indColorMin(C, Y) : indColorMin(M, Y);
  BG = K;
  UCR = K;
  
  if (UCR == 1.0f) {
    CMYK[indColorC] = 0.0f;
    CMYK[indColorM] = 0.0f;
    CMYK[indColorY] = 0.0f;
  } else if (UCR == 0.0f) {
    CMYK[indColorC] = C;
    CMYK[indColorM] = M;
    CMYK[indColorY] = Y;
  } else {
    switch (styleCMYKRGB) {
      
      case indColorStyleAdobeCMYK2RGB:
        NOT_UCR = (UCR < 0.0f ? 1.0f + UCR : 1.0f);
        
        CMYK[indColorC] = (C < UCR ? 0.0f : C > NOT_UCR ? 1.0f : C - UCR);
        CMYK[indColorM] = (M < UCR ? 0.0f : M > NOT_UCR ? 1.0f : M - UCR);
        CMYK[indColorY] = (Y < UCR ? 0.0f : Y > NOT_UCR ? 1.0f : Y - UCR);
        break;
        
      case indColorStyleGhostscriptCMYK2RGB:
        denom = 1.0f - UCR;
        
        v = 1.0f - (RGB[indColorR] / denom);
        CMYK[indColorC] = (v < 0.0f ? 0.0f : v >= 1.0 ? 1.0f : v);
        v = 1.0f - (RGB[indColorG] / denom);
        CMYK[indColorM] = (v < 0.0f ? 0.0f : v >= 1.0f ? 1.0f : v);
        v = 1.0f - (RGB[indColorB] / denom);
        CMYK[indColorY] = (v < 0.0f ? 0.0f : v >= 1.0f ? 1.0f : v);
        break;
        
      default:
        break;
    }
  }
  
  CMYK[indColorK] = BG;
}

void indCMYK_2_RGB(double * CMYK, double * RGB, int styleCMYKRGB /* = indColorStyleAdobeCMYK2RGB */) {
  double not_k;
  
  if (CMYK[indColorK] == 0.0f) {
    RGB[indColorR] = 1.0f - CMYK[indColorC];
    RGB[indColorG] = 1.0f - CMYK[indColorM];
    RGB[indColorB] = 1.0f - CMYK[indColorY];
  } else if (CMYK[indColorK] == 1.0f) {
    RGB[indColorR] = 0.0f;
    RGB[indColorG] = 0.0f;
    RGB[indColorB] = 0.0f;
  } else {
    not_k = 1.0f - CMYK[indColorK];
    
    switch (styleCMYKRGB) {
      
      case indColorStyleAdobeCMYK2RGB:
        
        RGB[indColorR] = (CMYK[indColorC] > not_k) ? 0.0f : not_k - CMYK[indColorC];
        RGB[indColorG] = (CMYK[indColorM] > not_k) ? 0.0f : not_k - CMYK[indColorM];
        RGB[indColorB] = (CMYK[indColorY] > not_k) ? 0.0f : not_k - CMYK[indColorY];
        break;
        
      case indColorStyleGhostscriptCMYK2RGB:
        RGB[indColorR] = (1.0f - CMYK[indColorC]) * not_k;
        RGB[indColorG] = (1.0f - CMYK[indColorM]) * not_k;
        RGB[indColorB] = (1.0f - CMYK[indColorY]) * not_k;
        break;
        
      default:
        break;
    }
  }
}

void indSafeCMYK_2_RGB(unsigned char * CMYK, unsigned char * RGB, int styleCMYKRGB /* = indColorStyleAdobeCMYK2RGB */) {
  unsigned char not_k, sCMYK[4];
  
  sCMYK[indColorC] = CMYK[indColorC];
  sCMYK[indColorM] = CMYK[indColorM];
  sCMYK[indColorY] = CMYK[indColorY];
  sCMYK[indColorK] = CMYK[indColorK];
  
  if (CMYK[indColorK] == 0) {
    RGB[indColorR] = 255 - sCMYK[indColorC];
    RGB[indColorG] = 255 - sCMYK[indColorM];
    RGB[indColorB] = 255 - sCMYK[indColorY];
  } else if (sCMYK[indColorK] == 255) {
    RGB[indColorR] = 0;
    RGB[indColorG] = 0;
    RGB[indColorB] = 0;
  } else {
    not_k = 255 - sCMYK[indColorK];
    
    switch (styleCMYKRGB) {
      
      case indColorStyleAdobeCMYK2RGB:
        
        RGB[indColorR] = (sCMYK[indColorC] > not_k) ? 0 : not_k - sCMYK[indColorC];
        RGB[indColorG] = (sCMYK[indColorM] > not_k) ? 0 : not_k - sCMYK[indColorM];
        RGB[indColorB] = (sCMYK[indColorY] > not_k) ? 0 : not_k - sCMYK[indColorY];
        break;
        
      case indColorStyleGhostscriptCMYK2RGB:
        RGB[indColorR] = (255 - sCMYK[indColorC]) * not_k;
        RGB[indColorG] = (255 - sCMYK[indColorM]) * not_k;
        RGB[indColorB] = (255 - sCMYK[indColorY]) * not_k;
        break;
        
      default:
        break;
    }
  }
}





