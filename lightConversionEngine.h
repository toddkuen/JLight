#ifndef lightConversionEngine_H_
#define lightConversionEngine_H_

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

#include "New/pdfCEngine1.h"
#include "New/pdfCCommonMerge.h"
#include "New/pdfCFastMergeEngine.h"
#include "New/pdfCMemory.h"
#include "New/pdfCPSStorage.h"
#include "New/pdfCColorSpec.h"

#include <stdio.h>
#include <ctype.h>

#include "lightPKLoader.h"
#include "lightGenericDumpers.h"

class indConversionEngineC {
protected:
  
  pdfXString sourceFile;
  pdfXString destinationRoot;
  pdfXString pathSep;
  pdfXString segmentTemplate;
  
  pdfXString workFolderRoot;
  
  indImageGrossC * imgProcessor;
  
  lightPKLoaderC * aLoader;
  
  int outputRes;
  
public:
  
  bool attachSourceFile(pdfXString src);
  
  void attachDumper(indImageDumperC * dumper);
  
  bool processImagesFromSourceFile(int pageLimit);
  
  inline int getImageCount(void) {
    if (imgProcessor != NULL) {
      return imgProcessor->getSubImageId();
    }
    
    return -1;
  }
  
  inline indImageGrossC * getImageProcessor(void) {
    return imgProcessor;
  }
  
  inline void setDumpTextRaster(bool dtr) {
    if (imgProcessor != NULL) {
      imgProcessor->setDumpTextRaster(dtr);
    }
  }
  
  inline void disablePlaneRendering(bool * bp) {
    if (imgProcessor != NULL) {
      imgProcessor->disablePlaneRendering(bp);
    }
  }
  
  inline lightJLYTInfoC * getLoaderHeader(void) {
    if (aLoader != NULL) {
      return &aLoader->header;
    }
    
    return NULL;
  }
  
  inline void setLevelMargin(double marg) {
    if (imgProcessor != NULL) {
      imgProcessor->setLevelMargin(marg);
    }
  }
  
  inline void setPerformAutoLevel(bool pal) {
    if (imgProcessor != NULL) {
      imgProcessor->setPerformAutoLevel(pal);
    }
  }
  
  void setDefaultResolution(int page) {
    if (imgProcessor != NULL) {
      imgProcessor->setDefaultResolution(page);
    }
  }
  
  void setPageResolution(int page, int s) {
    if (imgProcessor != NULL) {
      imgProcessor->setPageResolution(page, s);
    }
  }
  
  virtual void setMetaData(pdfXString source) {
    if (imgProcessor != NULL) {
      imgProcessor->setMetaData(source);
    }
  }
  
  void setPagePressSim(int page, int s) {
    if (imgProcessor != NULL) {
      imgProcessor->setPagePressSim(page, s);
    }
  }
  
  void setDefaultPressSim(int s) {
    if (imgProcessor != NULL) {
      imgProcessor->setDefaultPressSim(s);
    }
  }
  
  void setPageToExtract(int page) {
    if (imgProcessor != NULL) {
      imgProcessor->setPageToExtract(page);
    }
  }
  
  void setPageModulator(int pm) {
    if (imgProcessor != NULL) {
      imgProcessor->setPageModulator(pm);
    }
  }
  
  inline void setOutputResolution(int res) {
    outputRes = res;
  }
  
  bool processImageResults(void * dumper, int groupSize, pDumperElementPathT control, pdfXString path, pdfXString otemplate, bool breakOnGroup, bool aggregate, bool cleanup);
  
  indConversionEngineC();
  ~indConversionEngineC();
};

#endif // lightConversionEngine_H_

