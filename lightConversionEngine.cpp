#include "stdafx.h"
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

#include "lightConversionEngine.h"


indConversionEngineC::indConversionEngineC() {
  imgProcessor = NULL;
  aLoader = NULL;
}

indConversionEngineC::~indConversionEngineC() {
  if (imgProcessor != NULL) {
    delete imgProcessor;
  }
  
  if (aLoader != NULL) {
    delete aLoader;
  }
}

bool indConversionEngineC::attachSourceFile(pdfXString src) {
  if (imgProcessor != NULL) {
    delete imgProcessor;
  }
  
  if (aLoader != NULL) {
    delete aLoader;
  }
  
  imgProcessor = new indImageGrossC();
  
  aLoader = new lightPKLoaderC(imgProcessor);
  
  imgProcessor->setImageLoader(aLoader);
  
  if (aLoader->init(src)) {
    imgProcessor->initialize(indIMAGE_INPUT_BUFFER_MAX, 
                             indALIGNFORPRESS(aLoader->header.pixelWidth) * indALIGNFORPRESS(aLoader->header.pixelHeight));
    return true;
  }
  
  return false;
}

void indConversionEngineC::attachDumper(indImageDumperC * dumper) {
  if (imgProcessor != NULL) {
    imgProcessor->addImageDumper(dumper);
  }
}

bool indConversionEngineC::processImagesFromSourceFile(int pageLimit) {
  if (imgProcessor != NULL) {
    return imgProcessor->loadImages(pageLimit);
  }
  
  return false;
}

bool indConversionEngineC::processImageResults(void * dumper, int groupSize, pDumperElementPathT control, pdfXString path, pdfXString otemplate, bool breakOnGroup, bool aggregate, bool cleanup) {
  return imgProcessor->dumpFileList(dumper, groupSize, control, path, otemplate, breakOnGroup, aggregate, cleanup);
}