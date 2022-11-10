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

#ifdef GCC_UNIX
#include <unistd.h>
#endif // GCC_UNIX

#include "New/pdfCUtility.h"
#include "New/pdfCMemory.h"
#include "New/pdfCMergeEngine.h"
#include "Common/pdfdatatypes.h"
#include "Filters/pdfCodecFilter.h"
#include "TIFF/pdfCTiffInput.h"
#include "JPEG/pdfCJPEGInput.h"

#include <stdio.h>
#include <stdlib.h>			/* for atof */
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "lightGenericDumpers.h"

indHexDumperC::indHexDumperC(indImageGrossC * parent) : indImageDumperC(parent) {
  
}
  
void indHexDumperC::setFileTemplate(pdfXString templt) {
  fileTemplate = templt;
}

bool indHexDumperC::dumpImage(void) {
  int i, width, j, iColorMax;
  pdfXString realFileName;
  unsigned int * counts;
  FILE * f;
  unsigned char ** starts, * icolors;
    
  realFileName = destination->stringPerformSubstitutions(fileTemplate);

  if (skipWritingFile) {
    return true;
  }
  
  f = fopen((LPCTSTR)realFileName, "w");
  if (f != NULL) {
    width = destination->getWidth();
    counts = destination->getCounts();
    starts = destination->getStarts();
    icolors = destination->getIColors();
    iColorMax = destination->getIColorMax();
    
    fprintf(f, "IMAGE (PLANE %s):\n", destination->getPlaneAsString());
    
    for (i = 0; i < width; i++) {
      fprintf(f, "%05d: ", i);
      for (j = 0; j < (int)counts[i]; j++) {
        fprintf(f, "%02X", starts[i][j] & 0xFF);
      }
      fprintf(f, "\n");
    }
    
    fprintf(f, "COLORS (PLANE %s):\n", destination->getPlaneAsString());
    
    for (i = 0; i < iColorMax; i++) {
      fprintf(f, "%05d(%03X|%04X|%04X|%04X): %02X %02X %02X %02X\n", i, i, i*4, (4096 - i), (4096*4) - (i*4), icolors[i*4 + 0], icolors[i*4 + 1], icolors[i*4 + 2], icolors[i*4 + 3]);
    }
    
    fprintf(f, "\n");
    fclose(f);
    return true;
  }
  
  return false;
}


indMathematicaDumperC::indMathematicaDumperC(indImageGrossC * parent) : indImageDumperC(parent) {
  isPre = false;
}

void indMathematicaDumperC::setFileTemplate(pdfXString templt) {
  fileTemplate = templt;
}

bool indMathematicaDumperC::dumpImage(void) {
  int fileKey, width;
  char * realFileName, * outputBuffer;
  FILE * f;
  int rowBase, col, oHeight, cpe;
  
  outputBuffer = destination->getOutputBuffer();
  
  realFileName = new char[fileTemplate.GetLength() + 5];
  fileKey = destination->getSubImageId();
  sprintf(realFileName, fileTemplate, fileKey + 1);
  
  if (skipWritingFile) {
    return true;
  }
  
  f = fopen(realFileName, "w");
  if (f != NULL) {

    oHeight = destination->getOHeight();
    cpe = destination->getColorsPerEntry();
    
    fprintf(f, "Graphics[Raster[{\n");
    for (rowBase = yOffset; rowBase < (yOffset + height); rowBase += 1) {
      fprintf(f, "(*%d*){", rowBase);
      for (col = xOffset; col < xOffset + width; col += 1) {
        fprintf(f, "{%d,%d,%d}", 0xFF & outputBuffer[(rowBase*cpe) + (col*oHeight*cpe)], 0xFF & outputBuffer[(rowBase*cpe) + 1 + (col*oHeight*cpe)], 0xFF & outputBuffer[(rowBase*cpe) + 2 + (col*oHeight*cpe)]);
        if (col != (xOffset + width) - 1) {
          fprintf(f, ",");
        }
      }
      fprintf(f, "}");
      if (rowBase != (yOffset + height - 1)) {
        fprintf(f, ",\n");
      }
    }
    fprintf(f, "  },\n");
    fprintf(f, "  {{0,0},{%d,%d}}, \n", width, height);
    fprintf(f, "  {0,255}, ColorFunction->RGBColor], ImageSize -> {%d,%d}, PlotRange -> {{0, %d}, {0, %d}}]\n", width, height, width, height);
    
    fflush(f);
    fclose(f);
    return true;
  }
  
  return false;
}

///////// PDF //////////  (Most shared with PS dumper)

int indJLTFillOutputBuffer(pindJLTCodecState state, char * buffer, int bufferLength) {
  char * op, * srcp;
	int bytesWritten, i;
  
	bytesWritten = 0;
	op = (char *)buffer;
  
  while (state->currentRow < state->endRowExclusive && bytesWritten  < bufferLength - 6) {
    while (bytesWritten  < bufferLength - 6 && state->currentColumn >= state->startColumnInclusive) {
      srcp = &state->rawData[state->bytesPerStep * ((state->currentRow) + (state->currentColumn * state->oHeight))];
      
      for (i = 0; i < state->bytesPerStep; i++) {
        *op++ = *srcp++;
      }
      bytesWritten += state->bytesPerStep;
      state->totalBytesWritten += state->bytesPerStep;
      state->currentColumn -= 1;
    }
    
    if (bytesWritten < bufferLength - 6) {
      state->currentColumn = state->endColumnExclusive;
      state->currentRow += 1;
    
      if (state->currentRow >= state->endRowExclusive) {
        break;
      }
    } else {
      break;
    }
  }
  
	return bytesWritten;
}

class PDFCodecINDJLYTContentFilterC : public PDFCodecFilterC {
private:
  
  pindJLTCodecState state;
  
public:
    PDFCodecINDJLYTContentFilterC(pindJLTCodecState st) : PDFCodecFilterC() {
      state = st;
    };
  
  bool needsSource(void) { return false; };
  
  int process(void);
};

int PDFCodecINDJLYTContentFilterC::process(void) {
  char * destBuf;
	int maxToWrite, actuallyWritten;
  
	dataDestination->getFillInfo(maxToWrite, destBuf);
  if (maxToWrite < 12) {
    return CODEC_OUTPUT_FULL;
  }
	
	actuallyWritten = indJLTFillOutputBuffer(state, destBuf, maxToWrite);
  
  if (actuallyWritten > 0) {
    totalBytesWritten += actuallyWritten;
  }
  
  dataDestination->setFillComplete(actuallyWritten > 0? actuallyWritten: 0);
  
  return actuallyWritten == 0? CODEC_COMPLETE: CODEC_CONTINUE;
}

typedef struct {
  int maj;
  int op;
  const char * text;
  const char * stream;
} IxPDFEntry, * pIxPDFEntry;

#define Ix_NONE									(0x00000000)
#define Ix_PIXEL_WIDTH					(0x01000000)
#define Ix_PIXEL_HEIGHT					(0x02000000)
#define Ix_XREF_START						(0x03000000)
#define Ix_XREF_OFFSET					(0x04000000)
#define Ix_STREAM_LENGTH				(0x05000000)
#define Ix_POINTS_WIDTH					(0x06000000)
#define Ix_POINTS_HEIGHT				(0x07000000)
#define Ix_OBJECT_START					(0x08000000)
#define Ix_OBJECT_COUNT					(0x09000000)
#define Ix_STREAM								(0x0A000000)
#define Ix_CTM									(0x0B000000)
#define Ix_BITSPERCOMP					(0x0C000000)
#define Ix_FILTER								(0x0D000000)
#define Ix_COLORSPACE						(0x0E000000)
#define Ix_MASK_STREAM					(0x0F000000)
#define Ix_MASK_WIDTH						(0x10000000)
#define Ix_MASK_HEIGHT					(0x11000000)
#define Ix_MASK_STREAM_LENGTH		(0x12000000)
#define Ix_INDEX_HIVAL					(0x13000000)
#define Ix_TINT_STREAM					(0x14000000)
#define Ix_TINT_STREAM_LENGTH		(0x15000000)
#define Ix_DATETIME							(0x16000000)
#define Ix_TINT_RANGE						(0x17000000)
#define Ix_TINT_SIZE						(0x18000000)
#define Ix_TINT_VECTOR					(0x19000000)
#define Ix_TINT_COLOR_REF				(0x20000000)
#define Ix_CSREF_1							(0x21000000)
#define Ix_CSREF_2							(0x22000000)
#define Ix_CSREF_INIMAGE				(0x23000000)
#define Ix_CSREF_ASMASK					(0x24000000)
#define Ix_CSREF_IN_RENAME			(0x25000000)
#define Ix_RENAME               (0x26000000)
#define Ix_STRMSZ_RENAME        (0x27000000)
#define Ix_CSREF_3							(0x28000000)
#define Ix_ISCALE               (0x29000000)
#define Ix_IGNORE								(0xFE000000)
#define Ix_END									(0xFF000000)
#define Ix_OBJECT_SMASK(x)			(0x00FFFFFF & (x))

#define MAX_OBJS (17)

static IxPDFEntry IxMaskedDevice[] = {
	{ -1, Ix_NONE, "%PDF-1.3\n", NULL },
	{ -1, Ix_NONE, "%\xE2\xE3\xCF\xD3\n", NULL },
	{ -1, Ix_IGNORE, NULL, NULL },
	{  2, Ix_NONE, "2 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_RENAME, "/%s 1 0 R\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{  3, Ix_NONE, "3 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_NONE, "/Type /ExtGState\n", NULL },
	{ -1, Ix_NONE, "/SM 0.0200\n", NULL },
	{ -1, Ix_NONE, "/TR /Identity\n", NULL },
	{ -1, Ix_NONE, "/SA false\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{  4, Ix_NONE, "4 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_NONE, "/GS1 3 0 R\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{  5, Ix_NONE, "5 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_NONE, "/ProcSet [ /PDF /ImageC ]\n", NULL },
	{ -1, Ix_NONE, "/ExtGState 4 0 R\n", NULL },
	{ -1, Ix_NONE, "/XObject 2 0 R\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{  6, Ix_NONE, "6 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_STRMSZ_RENAME, "/Length %d\n", (const char *)76 },                          // 76 + len(name)
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "stream\n", NULL },
	{ -1, Ix_CTM, "/GS1 gs q %12.4f 0 0 %12.4f %12.4f %12.4f cm ", NULL }, // 21 + (4 * 12) +
	{ -1, Ix_RENAME, "/%s Do Q\n", NULL },                                 // 7 + len(name) = streamlen
	{ -1, Ix_NONE, "endstream\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{  7, Ix_NONE, "7 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_NONE, "/Parent 8 0 R\n", NULL },
	{ -1, Ix_NONE, "/Type /Page\n", NULL },
	{ -1, Ix_NONE, "/MediaBox [ 0 0 ", NULL },
	{ -1, Ix_POINTS_WIDTH, "%12.4f ", NULL },
	{ -1, Ix_POINTS_HEIGHT, "%12.4f ]\n", NULL },
	{ -1, Ix_NONE, "/Resources 5 0 R\n", NULL },
	{ -1, Ix_NONE, "/Rotate 0\n", NULL },
	{ -1, Ix_NONE, "/Hid false\n", NULL },
	{ -1, Ix_NONE, "/Contents 6 0 R\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{  8, Ix_NONE, "8 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_NONE, "/Type /Pages\n", NULL },
	{ -1, Ix_NONE, "/Kids [ 7 0 R ]\n", NULL },
	{ -1, Ix_NONE, "/Count 1\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{  9, Ix_NONE, "9 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_DATETIME, "/CreationDate (%s)\n", NULL },
	{ -1, Ix_DATETIME, "/ModDate (%s)\n", NULL },
	{ -1, Ix_NONE, "/Creator (jaylight image compiler)\n", NULL },
	{ -1, Ix_NONE, "/Producer (jaylight image compiler)\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{ 10, Ix_NONE, "10 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_NONE, "/Type /Catalog\n", NULL },
	{ -1, Ix_NONE, "/Pages 8 0 R\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{  1, Ix_NONE, "1 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_FILTER, "  /Filter [ %s %s ]\n", NULL },
	{ -1, Ix_PIXEL_WIDTH, "  /Width %9d\n", NULL },
	{ -1, Ix_BITSPERCOMP, "  /BitsPerComponent %d\n", NULL },
	{ -1, Ix_PIXEL_HEIGHT, "  /Height %9d\n", NULL },
	{ -1, Ix_NONE, " /Type /XObject\n", NULL },
	{ -1, Ix_COLORSPACE, "  /ColorSpace %s\n", NULL },
	{ -1, Ix_NONE, "  /Subtype /Image\n", NULL },
	{ -1, Ix_RENAME, "  /Name /%s\n", NULL },
	{ -1, Ix_NONE, "  %%%%/Mask 12 0 R\n", NULL },
	{ -1, Ix_NONE, "  /Length 11 0 R\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "stream\n", NULL },
	{ -1, Ix_STREAM, NULL, NULL },
	{ -1, Ix_NONE, "endstream\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{ 11, Ix_NONE, "11 0 obj\n", NULL },
	{ -1, Ix_STREAM_LENGTH, "%d\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{ 12, Ix_NONE, "12 0 obj\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
#ifdef DEBUG_MASK
	{ -1, Ix_NONE, "%%/Filter [ /ASCII85Decode /FlateDecode ]\n", NULL },
#else
	{ -1, Ix_NONE, "/Filter [ /ASCII85Decode /FlateDecode ]\n", NULL },
#endif // DEBUG_MASK
	{ -1, Ix_MASK_WIDTH, "/Width %9d\n", NULL },
	{ -1, Ix_NONE, "/BitsPerComponent 1\n", NULL },
	{ -1, Ix_MASK_HEIGHT, "/Height %9d\n", NULL },
	{ -1, Ix_NONE, "/Type /XObject\n", NULL },
	{ -1, Ix_NONE, "/ImageMask true\n", NULL },
	{ -1, Ix_NONE, "/Subtype /Image\n", NULL },
	{ -1, Ix_NONE, "/Name /DEF\n", NULL },
	{ -1, Ix_NONE, "/Length 13 0 R\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "stream\n", NULL },
	{ -1, Ix_MASK_STREAM, NULL, NULL },
	{ -1, Ix_NONE, "endstream\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{ 13, Ix_NONE, "13 0 obj\n", NULL },
	{ -1, Ix_MASK_STREAM_LENGTH, "%d\n", NULL },
	{ -1, Ix_NONE, "endobj\n", NULL },
	{ -1, Ix_XREF_START, "xref\n", NULL },
	{ -1, Ix_OBJECT_COUNT, "0 %d\n", NULL },
	{ -1, Ix_OBJECT_START + 0,  "0000000000 65536 f \n", NULL },
	{ -1, Ix_OBJECT_START + 1,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 2,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 3,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 4,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 5,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 6,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 7,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 8,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 9,  "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 10, "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 11, "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 12, "%010d 00000 n \n", NULL },
	{ -1, Ix_OBJECT_START + 13, "%010d 00000 n \n", NULL },
	{ -1, Ix_NONE, "trailer\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_OBJECT_COUNT, "/Size %d\n", NULL },
	{ -1, Ix_NONE, "/Root 10 0 R\n", NULL },
	{ -1, Ix_NONE, "/Info 9 0 R\n", NULL },
	{ -1, Ix_NONE, ">>\n", NULL },
	{ -1, Ix_NONE, "startxref\n", NULL },
	{ -1, Ix_XREF_OFFSET, "%d\n", NULL },
	{ -1, Ix_NONE, "%%EOF\n", NULL },
	{ -1, Ix_END, NULL, NULL }
};

static IxPDFEntry IxMaskedPSDevice[] = {
	{ -1, Ix_NONE, "%PS\n", NULL },
	{ -1, Ix_NONE, "gsave\n", NULL },
	{ -1, Ix_NONE, "<< /PageSize [ ", NULL },
	{ -1, Ix_POINTS_WIDTH, "%12.4f ", NULL },
	{ -1, Ix_POINTS_HEIGHT, "%12.4f ", NULL },
	{ -1, Ix_NONE, "] >> setpagedevice\n", NULL },
	{ -1, Ix_NONE, "/DeviceCMYK setcolorspace\n", NULL },
	{ -1, Ix_NONE, "0 0 translate\n", NULL },
  { -1, Ix_PIXEL_WIDTH, "%d ", NULL },
  { -1, Ix_ISCALE, "%f mul 2.82 div ", NULL },
  { -1, Ix_PIXEL_HEIGHT, "%d ", NULL },
  { -1, Ix_ISCALE, "%f mul 2.82 div scale\n", NULL },
	{ -1, Ix_NONE, "<<\n", NULL },
	{ -1, Ix_NONE, "  /ImageType 1\n", NULL },
	{ -1, Ix_FILTER, "  /DataSource currentfile %s filter \n", NULL },
	{ -1, Ix_PIXEL_WIDTH, "  /Width %d\n", NULL },
	{ -1, Ix_BITSPERCOMP, "  /BitsPerComponent %d\n", NULL },
	{ -1, Ix_PIXEL_HEIGHT, "  /Height %d\n", NULL },
	{ -1, Ix_PIXEL_WIDTH, "  /ImageMatrix [ %d 0 0 ", NULL },
	{ -1, Ix_PIXEL_HEIGHT, "-%d 0 ", NULL },
	{ -1, Ix_PIXEL_HEIGHT, "%d ]\n", NULL },
	{ -1, Ix_NONE, "  /Decode [ 0 1 0 1 0 1 0 1 ]\n", NULL },
	{ -1, Ix_NONE, ">> image\n", NULL },
  { -1, Ix_STREAM, NULL, NULL },
	{ -1, Ix_NONE, "\n", NULL },
	{ -1, Ix_NONE, "showpage\n", NULL },
	{ -1, Ix_NONE, "grestore\n", NULL },
	{ -1, Ix_END, NULL, NULL }
};

indPDF1DumperC::indPDF1DumperC(indImageGrossC * parent) : indImageDumperC(parent) {
  isPre = false;
  isPlane = false;
  sheetWidth = 1.0;
  sheetHeight = 1.0;
  flateCompress = true;
  ascii85encode = true;
  cyclesPerSide = 1;
}

indPDF1DumperC::~indPDF1DumperC() {
  if (previews.GetSize() > 0) {
    int i;
    
    for (i = 0; i < previews.GetSize(); i++) {
      delete previews.GetAt(i);
    }
    
    previews.RemoveAll();
  }
}

void indPDF1DumperC::setFileTemplate(pdfXString templt) {
  fileTemplate = templt;
}

enum {
  CMYKColorSpace = 1,
  RGBColorSpace = 2,
  GrayColorSpace = 3
};

bool dumpCore(pIxPDFEntry ep, 
              FILE * out,
              int imageWidth,
              int imageHeight,
              double widthInPoints,
              double heightInPoints,
              int bitsPerComponent,
              PDFCodecBufferC * inContent, 
              int rotation,
              double iscale,
              bool flate,
              bool lzw,
              bool ascii85,
              int colorSpace,
              pdfXString& error,
              int maskWidth,
              int maskHeight,
              PDFCodecBufferC * inMask) {
  int i, maxObj, count;
  char * p;
  bool eof;
  long objMajor[MAX_OBJS], point, xrefStart, streamLength, maskStreamLength;
	pdfXString buf, tmp;
	const char * pbuf;
	pdfXString tmpPath;
	time_t creationTime;
  struct tm * localTime;
  pdfXString resourceName;
  
  maxObj = 0;
  point = 0;
  resourceName = "ABCD";
    
  for (i = 0; (unsigned int)ep[i].op != Ix_END; i++) {
    if (ep[i].maj != -1) {
      if (ep[i].maj > maxObj) {
        maxObj = ep[i].maj;
      }
      objMajor[ep[i].maj] = point;
    }
      
    switch (ep[i].op & Ix_END) {
        
    case Ix_NONE:
      pbuf = ep[i].text;
      break;
          
    case Ix_IGNORE:
      pbuf = "";
      break;
          
    case Ix_INDEX_HIVAL:
      buf.Format(ep[i].text, maskWidth);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_MASK_WIDTH:
      buf.Format(ep[i].text, maskWidth);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_PIXEL_WIDTH:
      buf.Format(ep[i].text, imageWidth);
      pbuf = (LPCTSTR)buf;
      break;
      
    case Ix_ISCALE:
      buf.Format(ep[i].text, iscale);
      pbuf = (LPCTSTR)buf;
      break;
      
    case Ix_MASK_HEIGHT:
      buf.Format(ep[i].text, maskHeight);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_PIXEL_HEIGHT:
      buf.Format(ep[i].text, imageHeight);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_XREF_START:
      pbuf = ep[i].text;
      xrefStart = point;
      break;
          
    case Ix_XREF_OFFSET:
      buf.Format(ep[i].text, xrefStart);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_STREAM_LENGTH:
      buf.Format(ep[i].text, streamLength);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_MASK_STREAM_LENGTH:
      buf.Format(ep[i].text, maskStreamLength);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_POINTS_WIDTH:
      buf.Format(ep[i].text, widthInPoints);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_POINTS_HEIGHT:
      buf.Format(ep[i].text, heightInPoints);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_BITSPERCOMP:
      buf.Format(ep[i].text, bitsPerComponent);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_COLORSPACE:
      switch (colorSpace) {
      case CMYKColorSpace:
        buf.Format(ep[i].text, (LPCTSTR)"/DeviceCMYK");
        break;
      case RGBColorSpace:
        buf.Format(ep[i].text, (LPCTSTR)"/DeviceRGB");
        break;
      case GrayColorSpace:
        buf.Format(ep[i].text, (LPCTSTR)"/DeviceGray");
        break;
      }
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_FILTER:
      if (!ascii85 && !flate && !lzw) {
        buf = "";
      } else {
        buf.Format(ep[i].text, (LPCTSTR)(ascii85? "/ASCII85Decode": ""), (LPCTSTR)(flate? "/FlateDecode" : ""),  (LPCTSTR)(lzw? "/LZWDecode" : ""));
      }
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_CTM:
      if (rotation == 0) {
        buf.Format(ep[i].text, widthInPoints, heightInPoints, 0.0, 0.0);
      } else {
        buf.Format(ep[i].text, -widthInPoints, -heightInPoints, widthInPoints, heightInPoints);
      }
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_STRMSZ_RENAME:
      buf.Format(ep[i].text, ((int)ep[i].stream) + resourceName.GetLength());
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_RENAME:
      buf.Format(ep[i].text, (LPCTSTR)resourceName);
      pbuf = (LPCTSTR)buf;
      break;
          
   case Ix_OBJECT_START:
      buf.Format(ep[i].text, objMajor[Ix_OBJECT_SMASK(ep[i].op)]);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_OBJECT_COUNT:
      buf.Format(ep[i].text, maxObj + 1);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_MASK_STREAM:
      fflush(out);
      maskStreamLength = 0;
      if (inMask != NULL) {
        while (inMask->fillBuffer()) {
          inMask->getDrainInfo(p, count, eof);
          maskStreamLength += fwrite(p, 1, count, out);
          
          inMask->setDrainComplete(count);
        }
        
        inMask->getDrainInfo(p, count, eof);
        maskStreamLength += fwrite(p, 1, count, out);
      }
      point += maskStreamLength;
      break;
          
    case Ix_STREAM:
      fflush(out);
      streamLength = 0;
      while (inContent->fillBuffer()) {
        inContent->getDrainInfo(p, count, eof);
        streamLength += fwrite(p, 1, count, out);
            
        inContent->setDrainComplete(count);            
      }
            
      inContent->getDrainInfo(p, count, eof);
      streamLength += fwrite(p, 1, count, out);
      point += streamLength;
      break;
          
    //case Ix_TINT_STREAM_LENGTH:
      //buf.Format(ep[i].text, tintStreamLength);
      //pbuf = (LPCTSTR)buf;
      //break;
          
    case Ix_DATETIME:
      time( &creationTime );    
          
      localTime = localtime(&creationTime);
          
      tmp.Format("D:%04d%02d%02d%02d%02d%02d",
                     localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
                     localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
          
      buf.Format(ep[i].text, (LPCTSTR)tmp);
      pbuf = (LPCTSTR)buf;
      break;
          
    case Ix_TINT_SIZE:
      buf.Format(ep[i].text, 255);
      pbuf = (LPCTSTR)buf;
      break;      
    }
      
    if ((ep[i].op & Ix_END) != Ix_STREAM && (ep[i].op & Ix_END) != Ix_MASK_STREAM && 
          (ep[i].op & Ix_END) != Ix_TINT_STREAM) {
      point += strlen(pbuf);
      fputs(pbuf, out);
    }
  }
    
  fclose(out);
    
  return true;
}

static pdfXString fixForpdfX(pdfXString s) {
  pdfXString result;
  int i;

  for (i = 0; i < s.GetLength(); i++) {
	if (s.GetAt(i) == '\\' || s.GetAt(i) == ')' || s.GetAt(i) == '(') {
	  result += TCHAR('\\');
	} 
	result += s.GetAt(i);
  }

  return result;
}

bool indPDF1DumperC::addPreview(double scale, int filterControl, pdfXString& error) {
  indPDFPreviewDumperC * previewer;
  
  previewer = new indPDFPreviewDumperC(scale);
  previews.SetAtGrow(previews.GetSize(), previewer);
  
  return true;
}

bool indPDF1DumperC::dumpMasterFile(int groupSize,
                                   pDumperElementPathT dmp, 
                                   pDumperElementPathT pageControlInfo, 
                                   pdfXString path, 
                                   pdfXString otemplate,
                                   bool breakOnGroup,
                                   bool aggregate,
                                   bool cleanup) {
  int j;
  pdfXString serror, element, element2, fName;
  FILE * f;
  bool status;
  
  serror.Format("Beginning PDF output.");
  addToLogFile(lightLOG_STATUS, serror);
  
  groupNo = 0;
  
  if (dmp != NULL) {
    if (groupSize == -1) 
    {
      if (cyclesPerSide > 1)
      {
        fName.Format("%s", (LPCTSTR)stringPerformSubstitutions(path + otemplate + ".lis"));        
        f = fopen((LPCTSTR)fName, "w");
        
        if (f != NULL) {
          fprintf(f, "[ /begin << /ExpandPathMacros true >> ] merge\n");
          fprintf(f, "[ /createCarousel (c1) << /Page << /MediaBox [ 0 0 %f %f ] >> >> ] merge\n", sheetHeight, sheetWidth * 2);
          fprintf(f, "[ /createOutputStream (s1) << /OutputPath (%s) /FileName (%s) >> ] merge\n", 
                  (LPCTSTR)stringPerformSubstitutions(path),
                  (LPCTSTR)fixForpdfX(stringPerformSubstitutions(otemplate)));
          
          for (j = 0; j < dmp->GetSize(); j += 2) {
            
            if (dmp->GetAt(j).GetLength() >= 13 && dmp->GetAt(j).Left(13) == "PDFELEMENT#$:" &&
                dmp->GetAt(j + 1).GetLength() >= 13 && dmp->GetAt(j + 1).Left(13) == "PDFELEMENT#$:") {
              
              fprintf(f, "%%%% Page: %d\n", j + 1);
              
              element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 13);
              element2 = dmp->GetAt(j+1).Right(dmp->GetAt(j + 1).GetLength() - 13);
              
              fprintf(f, "[ /addPage (s1) [(c1:1)] << /$Page.MediaBoxTop [ << /Value (:%s:1:)  /CTM [ 1 0 0 1 %f 0 ] >> << /Value (:%s:1:) >> ] >> ] merge\n", (LPCTSTR)fixForpdfX(element), sheetWidth, (LPCTSTR)fixForpdfX(element2));
              
            } else if (dmp->GetAt(j).GetLength() >= 17 && dmp->GetAt(j).Left(17) == "GROUPSEPERATOR#$:") {
              element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 17);
              if (j + 2 < dmp->GetSize()) {
                fprintf(f, "%%%% Group (%s)\n", (LPCTSTR)element);
              }
            } else {
              fprintf(f, "%%%% Unknown tag '%s' ignored.\n", (LPCTSTR)dmp->GetAt(j));
            }
          }
          
          fprintf(f, "[ /closeAndDeleteOutputStream (s1) ] merge\n");
          fprintf(f, "[ /deleteCarousel (c1) ] merge\n");
          fprintf(f, "[ /end ] merge\n");
          fclose(f);
          
          status = runScript(fName);
          
          if (cleanup) {
            
            serror.Format("PDF - Beginning clean up of PDF page components.");
            addToLogFile(lightLOG_INFO, serror);
            
            for (j = 0; j < dmp->GetSize(); j++) {
              
              if (dmp->GetAt(j).GetLength() >= 13 && dmp->GetAt(j).Left(13) == "PDFELEMENT#$:") {            
                element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 13);
#ifndef GCC_UNIX
                _unlink((LPCTSTR)element);
#else
                unlink((LPCTSTR)element);
#endif // GCC_UNIX
              }
            }
            
#ifndef GCC_UNIX
            _unlink((LPCTSTR)fName);
#else
            unlink((LPCTSTR)fName);
#endif // GCC_UNIX
            
            serror.Format("PDF - Clean up of PDF page components completed.");
            addToLogFile(lightLOG_INFO, serror);
          }
          
          if (!status) {
            serror.Format("PDF I/O - Unable to combine PDF pages.");
            addToLogFile(lightLOG_ERROR, serror);
            return false;
          }          
        }
      }
      else 
      {
        // Process all pages - if there is control info, apply it.  If we run out of control info - keep going using the last control info...
        //  
        fName.Format("%s", (LPCTSTR)stringPerformSubstitutions(path + otemplate + ".lis"));        
        f = fopen((LPCTSTR)fName, "w");
        
        if (f != NULL) {
          fprintf(f, "[ /begin << /ExpandPathMacros true >> ] merge\n");
          fprintf(f, "[ /createOutputStream (s1) << /OutputPath (%s) /FileName (%s) >> ] merge\n", 
                  (LPCTSTR)stringPerformSubstitutions(path),
                  (LPCTSTR)fixForpdfX(stringPerformSubstitutions(otemplate)));
          
          for (j = 0; j < dmp->GetSize(); j++) {
            
            if (dmp->GetAt(j).GetLength() >= 13 && dmp->GetAt(j).Left(13) == "PDFELEMENT#$:") {
              
              fprintf(f, "%%%% Page: %d\n", j + 1);
              
              element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 13);
              
              fprintf(f, "[ /addPage (s1) [(:%s:1:)] << >> ] merge\n", (LPCTSTR)fixForpdfX(element));
              
            } else if (dmp->GetAt(j).GetLength() >= 17 && dmp->GetAt(j).Left(17) == "GROUPSEPERATOR#$:") {
              element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 17);
              if (j + 2 < dmp->GetSize()) {
                fprintf(f, "%%%% Group (%s)\n", (LPCTSTR)element);
              }
            } else {
              fprintf(f, "%%%% Unknown tag '%s' ignored.\n", (LPCTSTR)dmp->GetAt(j));
            }
          }
          
          fprintf(f, "[ /closeAndDeleteOutputStream (s1) ] merge\n");
          fprintf(f, "[ /end ] merge\n");
          fclose(f);
          
        runScript:
          status = runScript(fName);
          
          if (cleanup) {
            
            serror.Format("PDF - Beginning clean up of PDF page components.");
            addToLogFile(lightLOG_INFO, serror);
            
            for (j = 0; j < dmp->GetSize(); j++) {
              
              if (dmp->GetAt(j).GetLength() >= 13 && dmp->GetAt(j).Left(13) == "PDFELEMENT#$:") {            
                element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 13);
#ifndef GCC_UNIX
                _unlink((LPCTSTR)element);
#else
                unlink((LPCTSTR)element);
#endif // GCC_UNIX
              }
            }
            
#ifndef GCC_UNIX
            _unlink((LPCTSTR)fName);
#else
            unlink((LPCTSTR)fName);
#endif // GCC_UNIX
            
            serror.Format("PDF - Clean up of PDF page components completed.");
            addToLogFile(lightLOG_INFO, serror);
          }
          
          if (!status) {
            serror.Format("PDF I/O - Unable to combine PDF pages.");
            addToLogFile(lightLOG_ERROR, serror);
            return false;
          }          
        }
      
        else 
        {
scriptRunFailed1:
          serror.Format("PDF I/O - Unable to open script file '%s' to combine pages.", (LPCTSTR)fName);
          addToLogFile(lightLOG_ERROR, serror);
          return false;
        }
      }
    } else {
      
      if (breakOnGroup) {
        int groupCount, bStart;
        
        groupCount = 0;
        for (j = 0; j < dmp->GetSize(); j++) {
          if (dmp->GetAt(j).GetLength() >= 17 && dmp->GetAt(j).Left(17) == "GROUPSEPERATOR#$:") {
            groupCount += 1;
          }
        }
        
        fName.Format("%s", (LPCTSTR)stringPerformSubstitutions(path + otemplate + ".lis"));        
        f = fopen((LPCTSTR)fName, "w");
        
        if (f != NULL) {
          
          fprintf(f, "[ /begin << /ExpandPathMacros true >> ] merge\n");

          for (j = 0; j < dmp->GetSize(); j++) {
            
            bStart = j;
            
            for (; j < dmp->GetSize(); j++) {
              if (dmp->GetAt(j).GetLength() >= 17 && dmp->GetAt(j).Left(17) == "GROUPSEPERATOR#$:") {
                break;
              }
            }
            
            if (bStart < j) {
              fprintf(f, "[ /createOutputStream (s1) << /OutputPath (%s) /FileName (%s) >> ] merge\n", 
                      (LPCTSTR)stringPerformSubstitutions(path),
                      (LPCTSTR)fixForpdfX(stringPerformSubstitutions(otemplate)));
              
              for (; bStart < j; bStart++) {
                
                if (dmp->GetAt(bStart).GetLength() >= 13 && dmp->GetAt(bStart).Left(13) == "PDFELEMENT#$:") {
                  
                  fprintf(f, "%%%% Page: %d\n", bStart + 1);
                  
                  element = dmp->GetAt(bStart).Right(dmp->GetAt(bStart).GetLength() - 13);
                  
                  fprintf(f, "[ /addPage (s1) [(:%s:1:)] << >> ] merge\n", (LPCTSTR)fixForpdfX(element));
                  
                } else if (dmp->GetAt(bStart).GetLength() >= 17 && dmp->GetAt(bStart).Left(17) == "GROUPSEPERATOR#$:") {
                  element = dmp->GetAt(bStart).Right(dmp->GetAt(bStart).GetLength() - 17);
                  if (j + 2 < dmp->GetSize()) {
                    fprintf(f, "%%%% Group (%s)\n", (LPCTSTR)element);
                  }
                } else {
                  fprintf(f, "%%%% Unknown tag '%s' ignored.\n", (LPCTSTR)dmp->GetAt(bStart));
                }
              }
              fprintf(f, "[ /closeAndDeleteOutputStream (s1) ] merge\n");
            }

            groupNo += 1;
          }
          
          fprintf(f, "[ /end ] merge\n");
          fclose(f);
          
          goto runScript;
          
        } else {
          goto scriptRunFailed1;
        }
        
      } else {
        
        fName.Format("%s", (LPCTSTR)stringPerformSubstitutions(path + otemplate + ".lis"));        
        f = fopen((LPCTSTR)fName, "w");
        
        if (f != NULL) {
          fprintf(f, "[ /begin << /ExpandPathMacros true >> ] merge\n");
          fprintf(f, "[ /createOutputStream (s1) << /OutputPath (%s) /FileName (%s) >> ] merge\n", 
                  (LPCTSTR)stringPerformSubstitutions(path),
                  (LPCTSTR)fixForpdfX(stringPerformSubstitutions(otemplate)));
          
            for (j = 0; j < dmp->GetSize(); j++) {
              
              if (dmp->GetAt(j).GetLength() >= 13 && dmp->GetAt(j).Left(13) == "PDFELEMENT#$:") {
                
                fprintf(f, "%%%% Page: %d\n", j + 1);
                
                element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 13);
                
                fprintf(f, "[ /addPage (s1) [(:%s:1:)] << >> ] merge\n", (LPCTSTR)fixForpdfX(element));
                
              } else if (dmp->GetAt(j).GetLength() >= 17 && dmp->GetAt(j).Left(17) == "GROUPSEPERATOR#$:") {
                element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 17);
                if (j + 2 < dmp->GetSize()) {
                  fprintf(f, "%%%% Group (%s)\n", (LPCTSTR)element);
                }
              } else {
                fprintf(f, "%%%% Unknown tag '%s' ignored.\n", (LPCTSTR)dmp->GetAt(j));
              }
            }
          
          fprintf(f, "[ /closeAndDeleteOutputStream (s1) ] merge\n");
          fprintf(f, "[ /end ] merge\n");
          fclose(f);
          
          goto runScript;
        } else {
          serror.Format("PDF I/O - Unable to open output file '%s'.", (LPCTSTR)fName);
          addToLogFile(lightLOG_ERROR, serror);
          return false;
        }
      }
    }
    
    serror.Format("End PDF output.");
    addToLogFile(lightLOG_STATUS, serror);

    return true;
  } else {
    serror.Format("PDF I/O - No pages produced.");
    addToLogFile(lightLOG_WARNING, serror);
  }

  return false;
}

bool indPDF1DumperC::dumpImage(void) {
  pdfXString realFileName; 
  char * outputBuffer;
  FILE * f;
  int oHeight, cpe, dScale;
  indJLTCodecState state;
  pdfXString serror;
  bool dumpStatus;
    
  outputBuffer = destination->getOutputBuffer();
  oHeight = destination->getOHeight();
  cpe = destination->getColorsPerEntry();
  dScale = destination->getOutputBufferScale();
  
  realFileName = destination->stringPerformSubstitutions(fileTemplate);
  
  if (previews.GetSize() > 0) {
    int i;
    
    for (i = 0; i < previews.GetSize(); i++) {
      indPDFPreviewDumperC * previewer;
      int pHeight, pWidth;
      pdfXString previewFile;

      previewer = previews.GetAt(i);

      if (!previewer->isInitialized()) {
        pHeight = (int)(((double)height) * previewer->getScale());
        pWidth = (int)(((double)(width)) * previewer->getScale());
        
        if (pHeight < MIN_PREVIEW_PIXELS || pWidth < MIN_PREVIEW_PIXELS) {
          serror.Format("Scale %s results in a preview with a height or width less than %d pixels - no preview or output produced.", 
                       (LPCTSTR)FixedToCString(previewer->getScale()), MIN_PREVIEW_PIXELS);
          addToLogFile(lightLOG_ERROR, serror);
          return false;
        }

        previewer->init(cpe, pHeight, pWidth);
      }
      
      previewer->getInfo(pHeight, pWidth);
      previewer->deriveFromBaseImageFiltered((unsigned char *)outputBuffer, oHeight, oHeight * cpe, dScale * width);  // swapped...
      previewer->initializeCodec(&state);
      
      previewFile.Format("%s%s", (LPCTSTR)previewPath, (LPCTSTR)previewTemplate);
      previewFile = destination->stringPerformSubstitutions(previewFile);
      f = fopen((LPCTSTR)previewFile, "wb");
      
      if (f != NULL) {
        PDFCodecStackC * contentStack;
        pdfXString error;
        PDFCodecBufferC * topContent;
        
        contentStack = new PDFCodecStackC();
        
        topContent = contentStack->push(new PDFCodecINDJLYTContentFilterC(&state));
        if (flateCompress) {
          topContent = contentStack->push(new PDFCodecFlateEncodeFilterC());
        }
        if (ascii85encode) {
          topContent = contentStack->push(new PDFCodecASCII85EncodeFilterC());
        }
        
        dumpStatus = dumpCore(IxMaskedDevice,
                              f,
                              pWidth,
                              pHeight,
                              sheetWidth * previewer->getScale(),   // widthInPoints,
                              sheetHeight * previewer->getScale(),  // heightInPoints,
                              8,    // bitsPerComponent,
                              topContent, 
                              0,     // rotation,
                              1.0,
                              flateCompress, // flate,
                              false, // lzw
                              ascii85encode, // ascii85,
                              CMYKColorSpace,     // colorSpace
                              error,
                              0,
                              0,
                              NULL);
        
        delete contentStack;
        
        if (!dumpStatus) {
          
          serror.Format("PDF I/O Error processing '%s': %s", (LPCTSTR)previewFile, (LPCTSTR)error);
          addToLogFile(lightLOG_ERROR, serror);
          
          fclose(f);
          return false;
        } else {
          destination->addDumpElement(this, "PDFPROOF", previewFile);      
          
          serror.Format("PDF file '%s' written successfully", (LPCTSTR)previewFile);
          addToLogFile(lightLOG_INFO, serror);
        }
        
        fflush(f);
        fclose(f);
      }  
    }
  }
  
  if (skipWritingFile) {
    serror.Format("PDF output skipped: '%s'.", (LPCTSTR)realFileName);
    addToLogFile(lightLOG_STATUS, serror);
    return true;
  }
  
  state.rawData = outputBuffer;
  state.oHeight = oHeight;
  state.currentRow = yOffset;
  state.endRowExclusive = yOffset + (dScale * height);
  state.currentColumn = xOffset + (dScale * width) - 1;
  state.startColumnInclusive = xOffset;
  state.endColumnExclusive = xOffset + (dScale * width) - 1;
  state.bytesPerStep = cpe;
  state.totalBytesWritten = 0;
  state.bytesWritten = 0;

  f = fopen((LPCTSTR)realFileName, "wb");
  if (f != NULL) {
    PDFCodecStackC * contentStack;
    pdfXString error;
    PDFCodecBufferC * topContent;
    
    contentStack = new PDFCodecStackC();
    
    topContent = contentStack->push(new PDFCodecINDJLYTContentFilterC(&state));
    if (flateCompress) {
      topContent = contentStack->push(new PDFCodecFlateEncodeFilterC());
    }
    if (ascii85encode) {
      topContent = contentStack->push(new PDFCodecASCII85EncodeFilterC());
    }
    
    dumpStatus = dumpCore(IxMaskedDevice,
                          f,
                          dScale * width,
                          dScale * height,
                          sheetWidth,   // widthInPoints,
                          sheetHeight,  // heightInPoints,
                          8,    // bitsPerComponent,
                          topContent, 
                          0,     // rotation,
                          1.0,
                          flateCompress, // flate,
                          false, // lzw
                          ascii85encode, // ascii85,
                          CMYKColorSpace,     // colorSpace
                          error,
                          0,
                          0,
                          NULL);
    
    fflush(f);
    fclose(f);
    
    delete contentStack;
    
    if (!dumpStatus) {    
      serror.Format("PDF I/O Error processing '%s': %s", (LPCTSTR)realFileName, (LPCTSTR)error);
      addToLogFile(lightLOG_ERROR, serror);
    } else {
      destination->addDumpElement(this, "PDFELEMENT", realFileName);      

      serror.Format("PDF file '%s' written successfully", (LPCTSTR)realFileName);
      addToLogFile(lightLOG_INFO, serror);
    }
        
    return dumpStatus;
  }
  
  serror.Format("PDF I/O: unable to write file '%s'", (LPCTSTR)realFileName);
  addToLogFile(lightLOG_ERROR, serror);
  
  return false;
}

///////////////// PS ///////////////////

indPS1DumperC::indPS1DumperC(indImageGrossC * parent) : indImageDumperC(parent) {
  isPre = false;
  isPlane = false;
  sheetWidth = 1.0;
  sheetHeight = 1.0;
  lzwCompress = false;
  ascii85encode = true;
}

void indPS1DumperC::setFileTemplate(pdfXString templt) {
  fileTemplate = templt;
}

static void writePSHeader(FILE * f, 
                          pdfXString jobName,
                          pdfXString version,
                          int pageCount,
                          float width,
                          float height) {
	time_t creationTime;
  struct tm * localTime;
  pdfXString tmp;

  time( &creationTime );    
  
  localTime = localtime(&creationTime);

  tmp.Format("%04d-%02d-%02dT%02d:%02d:%02d", localTime->tm_year + 1900, localTime->tm_mon + 1,
													localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

  fprintf(f, "%%!PS-Adobe-3.0\n");
  fprintf(f, "%%APL_DSC_Encoding: UTF8\n");
  fprintf(f, "%%%%Title: (%s)\n", (LPCTSTR)jobName);
  fprintf(f, "%%%%Creator: (jaylight version %s)\n", (LPCTSTR)version);
  fprintf(f, "%%%%CreationDate: (%s)\n", (LPCSTR)tmp);
  fprintf(f, "%%%%For: (OPERATIONS)\n");
  fprintf(f, "%%%%Pages: %d\n", pageCount);
  fprintf(f, "%%%%DocumentFonts: \n");
  fprintf(f, "%%%%DocumentNeededFonts: \n");
  fprintf(f, "%%%%DocumentSuppliedFonts: \n");
  fprintf(f, "%%%%DocumentData: Clean7Bit\n");
  fprintf(f, "%%%%PageOrder: Ascend\n");
  fprintf(f, "%%%%Orientation: Portrait\n");
  fprintf(f, "%%%%DocumentMedia: (Default) %s %s 0 () ()\n", (LPCTSTR)FixedToCString(width), (LPCTSTR)FixedToCString(height));
  fprintf(f, "%%ADO_ImageableArea: 18 18 %s %s\n", (LPCTSTR)FixedToCString(width - 18.0), (LPCTSTR)FixedToCString(height - 18.0));
  fprintf(f, "%%RBINumCopies: 1\n");
  fprintf(f, "%%%%LanguageLevel: 2\n");
  fprintf(f, "%%%%DocumentProcessColors: Cyan Magenta Yellow Black\n");
  fprintf(f, "%%%%EndComments\n");
}

bool indPS1DumperC::dumpMasterFile(int groupSize,
                                   pDumperElementPathT dmp, 
                                   pDumperElementPathT pageControlInfo, 
                                   pdfXString path, 
                                   pdfXString otemplate,
                                   bool breakOnGroup,
                                   bool aggregate,
                                   bool cleanup) {
  int j;
  pdfXString serror, element, fName;
  FILE * f;
  
  serror.Format("Beginning PostScript output.");
  addToLogFile(lightLOG_STATUS, serror);

  groupNo = 0;

  if (dmp != NULL) {
    if (groupSize == -1) {
      // Process all pages - if there is control info, apply it.  If we run out of control info - keep going using the last control info...
      //  
      
      fName.Format("%s", (LPCTSTR)stringPerformSubstitutions(path + otemplate));
      f = fopen((LPCTSTR)fName, "w");
      
      if (f != NULL) {
        int pageCount;
        
        pageCount = 0;
        
        for (j = 0; j < dmp->GetSize(); j++) {
          if (dmp->GetAt(j).GetLength() >= 12 && dmp->GetAt(j).Left(12) == "PSELEMENT#$:") {
            pageCount++;
          }
        }
        
        writePSHeader(f, 
                      "NONE",
                      "1.0",
                      pageCount,
                      (float)sheetWidth,
                      (float)sheetHeight);
        
        fprintf(f, "\n%%%% JOB START\n");
        fprintf(f, "%%%% No page groups specified - output is a single large job...\n");
        
        for (j = 0; j < dmp->GetSize(); j++) {
          
          fprintf(f, "\n%%%%Page %d %d\n", j + 1, j + 1);
          
          if (j < pageControlInfo->GetSize()) {
            fprintf(f, "<< /MediaType (%s) >> setpagedevice\n", (LPCTSTR)pageControlInfo->GetAt(j));
          } else {
            fprintf(f, "%%%% No media type specified for this page.\n");
          }
          
          if (dmp->GetAt(j).GetLength() >= 12 && dmp->GetAt(j).Left(12) == "PSELEMENT#$:") {
            element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 12);
            fprintf(f, "(%s) run\n", (LPCTSTR)element);
          } else {
            fprintf(f, "%%%% Unknown tag '%s' ignored.\n", (LPCTSTR)dmp->GetAt(j));
          }
          
          fprintf(f, "%%%%PageTrailer\n\n");
        }
        
        fprintf(f, "%%%%Trailer\n%%EOF\n");

        fclose(f);
      } else {
        serror.Format("PS I/O - Unable to open output file '%s'.", (LPCTSTR)fName);
        addToLogFile(lightLOG_ERROR, serror);
        return false;
      }

    } else {
           
      if (breakOnGroup) {
        int groupCount, bStart, i;
        int pageCount;
                
        groupCount = 0;
        for (j = 0; j < dmp->GetSize(); j++) {
          if (dmp->GetAt(j).GetLength() >= 17 && dmp->GetAt(j).Left(17) == "GROUPSEPERATOR#$:") {
            groupCount += 1;
          }
        }
        
        if (groupCount > 0) {
          
          //for (j = 0; j < dmp->GetSize(); j++) {
          //  fprintf(stdout, "GENERIC DUMP (PATH='%s' TEMPLATE=%s'): %d: %s (CTRL='%s')\n", (LPCTSTR)path, (LPCTSTR)otemplate, j, (LPCTSTR)dmp->GetAt(j), j < pageControlInfo->GetSize()? (LPCTSTR)pageControlInfo->GetAt(j) : "**NONE**");
          //}
         

          for (j = 0; j < dmp->GetSize(); j++) {

            bStart = j;
            pageCount = 0;        
            
            for (; j < dmp->GetSize(); j++) {
              if (dmp->GetAt(j).GetLength() >= 17 && dmp->GetAt(j).Left(17) == "GROUPSEPERATOR#$:") {
                break;
              } else if (dmp->GetAt(j).GetLength() >= 12 && dmp->GetAt(j).Left(12) == "PSELEMENT#$:") {
                pageCount++;
              }
            }
            
            fName.Format("%s", (LPCTSTR)stringPerformSubstitutions(path + otemplate));

            if ((f = fopen((LPCTSTR)fName, "w")) != NULL) {
              int base;
              pdfXString element;
              FILE * elem;
              
              writePSHeader(f, 
                            "NONE",
                            "1.0",
                            pageCount,
                            (float)sheetWidth,
                            (float)sheetHeight);
              
              i = 1;
              base = 0;
              while (bStart < j) {

                if (dmp->GetAt(bStart).GetLength() >= 12 && dmp->GetAt(bStart).Left(12) == "PSELEMENT#$:") {
                  
                  fprintf(f, "%%%% Page %d %d\n", i, i);
                  i += 1;
                  
                  if (base < pageControlInfo->GetSize()) {
                    fprintf(f, "<< /MediaType (%s) >> setpagedevice\n", (LPCTSTR)pageControlInfo->GetAt(base));
                  } else {
                    fprintf(f, "%%%% No media type specified for this page.\n");
                  }
                  element = dmp->GetAt(bStart).Right(dmp->GetAt(bStart).GetLength() - 12);

                  if (aggregate) {
                    if ((elem = fopen((LPCTSTR)element, "r")) != NULL) {
                      int cnt, length;
#ifdef SMALL_LOCAL
                      char * buf;
                      pdfCIOBuffer tmp(PDFCIOBUFFER_SIZE);
                      int copySize = PDFCIOBUFFER_SIZE;
                      
                      buf = tmp.getBuffer();
#else
                      char buf[10 * PDFCIOBUFFER_SIZE];
                      int copySize = 10 * PDFCIOBUFFER_SIZE;
#endif // SMALL_LOCAL
                      
                      fseek(elem, 0, SEEK_END);
                      length = ftell(elem);
                      fseek(elem, 0, SEEK_SET);
                      
                      while ((cnt = fread(buf, 1, copySize, elem)) > 0) {
                        length -= fwrite(buf, 1, cnt, f);
                      }
                      
                      if (length != 0) {
                        serror.Format("PS I/O - Error copying PostScript component file '%s' (len=%d).", (LPCTSTR)element, length);
                        addToLogFile(lightLOG_ERROR, serror);
                        return false;
                      }
                      
                      fclose(elem);
                      
                      if (cleanup) {
#ifndef GCC_UNIX
                        _unlink((LPCTSTR)element);
#else
                        unlink((LPCTSTR)element);
#endif // GCC_UNIX
                        
                      }
                    } else {
                      serror.Format("PS I/O - Unable to open output PostScript component file '%s'.", (LPCTSTR)element);
                      addToLogFile(lightLOG_ERROR, serror);
                      return false;
                    }
                  } else {
                    fprintf(f, "(%s) run\n", (LPCTSTR)element);
                  }
                } else {
                  fprintf(f, "%%%% Unknown tag '%s' ignored.\n", (LPCTSTR)dmp->GetAt(bStart));
                }
                
                fprintf(f, "%%%%PageTrailer\n\n");
                
                base += 1;
                bStart += 1;
              }

              fprintf(f, "%%%%Trailer\n%%EOF\n");
              fclose(f);
            } else {
              serror.Format("PS I/O - Unable to open output file '%s'.", (LPCTSTR)fName);
              addToLogFile(lightLOG_ERROR, serror);
              return false;
            }
            
            groupNo += 1;
          }
          
        } else {
          
          fName.Format("%s", (LPCTSTR)stringPerformSubstitutions(path + otemplate));
          f = fopen((LPCTSTR)fName, "w");
          
          if (f != NULL) {
            fprintf(f, "%%%% JOB START\n");
            fprintf(f, "%%%% Page groups (count=1)\n");
            goto noGroups;
          } else {
            serror.Format("PS I/O - Unable to open output file '%s'.", (LPCTSTR)fName);
            addToLogFile(lightLOG_ERROR, serror);
            return false;
          }
        }
        
      } else {
        
        fName.Format("%s", (LPCTSTR)stringPerformSubstitutions(path + otemplate));        
        f = fopen((LPCTSTR)fName, "w");

        if (f != NULL) {
          int pageCount;

          pageCount = 0;
          
          for (j = 0; j < dmp->GetSize(); j++) {
            if (dmp->GetAt(j).GetLength() >= 12 && dmp->GetAt(j).Left(12) == "PSELEMENT#$:") {
              pageCount++;
            }
          }
          
          writePSHeader(f, 
                        "NONE",
                        "1.0",
                        pageCount,
                        (float)sheetWidth,
                        (float)sheetHeight);

          fprintf(f, "\n%%%% JOB START\n");
          fprintf(f, "%%%% Page groups ignored on output - output is a single large job...\n");
          
noGroups:
            
          for (j = 0; j < dmp->GetSize(); j++) {
                        
            if (dmp->GetAt(j).GetLength() >= 12 && dmp->GetAt(j).Left(12) == "PSELEMENT#$:") {
              FILE * elem;

              fprintf(f, "%%%% Page %d %d\n", j + 1, j + 1);
              
              if (j < pageControlInfo->GetSize()) {
                fprintf(f, "<< /MediaType (%s) >> setpagedevice\n", (LPCTSTR)pageControlInfo->GetAt(j));
              } else {
                fprintf(f, "%%%% No media type specified for this page.\n");
              }
              element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 12);

              if (aggregate) {
                if ((elem = fopen((LPCTSTR)element, "r")) != NULL) {
                  int cnt, length;
#ifdef SMALL_LOCAL
                  char * buf;
                  pdfCIOBuffer tmp(PDFCIOBUFFER_SIZE);
                  int copySize = PDFCIOBUFFER_SIZE;
                  
                  buf = tmp.getBuffer();
#else
                  char buf[10 * PDFCIOBUFFER_SIZE];
                  int copySize = 10 * PDFCIOBUFFER_SIZE;
#endif // SMALL_LOCAL
                  
                  fseek(elem, 0, SEEK_END);
                  length = ftell(elem);
                  fseek(elem, 0, SEEK_SET);
                  
                  while ((cnt = fread(buf, 1, copySize, elem)) > 0) {
                    length -= fwrite(buf, 1, cnt, f);
                  }
                  
                  if (length != 0) {
                    serror.Format("PS I/O - Error copying PostScript component file '%s' (len=%d).", (LPCTSTR)element, length);
                    addToLogFile(lightLOG_ERROR, serror);
                    return false;
                  }
                  
                  fclose(elem);
                  
                  if (cleanup) {
#ifndef GCC_UNIX
                    _unlink((LPCTSTR)element);
#else
                    unlink((LPCTSTR)element);
#endif // GCC_UNIX
                    
                  }
                } else {
                  serror.Format("PS I/O - Unable to open output PostScript component file '%s'.", (LPCTSTR)element);
                  addToLogFile(lightLOG_ERROR, serror);
                  return false;
                }
                
              } else {
                fprintf(f, "(%s) run\n", (LPCTSTR)element);
              }
            
              fprintf(f, "%%%%PageTrailer\n\n");

            } else if (dmp->GetAt(j).GetLength() >= 17 && dmp->GetAt(j).Left(17) == "GROUPSEPERATOR#$:") {
              element = dmp->GetAt(j).Right(dmp->GetAt(j).GetLength() - 17);
              if (j + 2 < dmp->GetSize()) {
                fprintf(f, "%%%% Group (%s)\n", (LPCTSTR)element);
              }
            } else {
              fprintf(f, "%%%% Unknown tag '%s' ignored.\n", (LPCTSTR)dmp->GetAt(j));
            }
          }

          fprintf(f, "%%%%Trailer\n%%EOF\n");
          fclose(f);
        } else {
          serror.Format("PS I/O - Unable to open output file '%s'.", (LPCTSTR)fName);
          addToLogFile(lightLOG_ERROR, serror);
          return false;
        }
      }
    }

    serror.Format("End PostScript output.");
    addToLogFile(lightLOG_STATUS, serror);

    return true;
  } else {
    serror.Format("PS I/O - No pages produced.");
    addToLogFile(lightLOG_WARNING, serror);
  }
  
  return false;
}

bool indPS1DumperC::dumpImage(void) {
  pdfXString realFileName; 
  char * outputBuffer;
  FILE * f;
  int oHeight, cpe, dScale;
  indJLTCodecState state;
  pdfXString serror;
  bool dumpStatus;
  
  outputBuffer = destination->getOutputBuffer();
  oHeight = destination->getOHeight();
  cpe = destination->getColorsPerEntry();
  dScale = destination->getOutputBufferScale();
  
  realFileName = destination->stringPerformSubstitutions(fileTemplate);
  
  state.rawData = outputBuffer;
  state.oHeight = oHeight;
  state.currentRow = yOffset;
  state.endRowExclusive = yOffset + (dScale * height);
  state.currentColumn = xOffset + (dScale * width) - 1;
  state.startColumnInclusive = xOffset;
  state.endColumnExclusive = xOffset + (dScale * width) - 1;
  state.bytesPerStep = cpe;
  state.totalBytesWritten = 0;
  state.bytesWritten = 0;
  
  f = fopen((LPCTSTR)realFileName, "wb");
  if (f != NULL) {
    PDFCodecStackC * contentStack;
    pdfXString error;
    PDFCodecBufferC * topContent;
    
    contentStack = new PDFCodecStackC();
    
    topContent = contentStack->push(new PDFCodecINDJLYTContentFilterC(&state));
    if (lzwCompress) {
      //topContent = contentStack->push(new PDFCodecFlateEncodeFilterC());
    }
    if (ascii85encode) {
      topContent = contentStack->push(new PDFCodecASCII85EncodeFilterC());
    }
    
    if (!skipWritingFile) {
      dumpStatus = dumpCore(IxMaskedPSDevice,
                            f,
                            dScale * width,
                            dScale * height,
                            sheetWidth,   // widthInPoints,
                            sheetHeight,   // heightInPoints,
                            8,    // bitsPerComponent,
                            topContent, 
                            0,     // rotation,
                            1.0 / (double)destination->getOutputBufferScale(),
                            false, // flate,
                            lzwCompress, 
                            ascii85encode, // ascii85,
                            CMYKColorSpace,     // colorSpace
                            error,
                            0,
                            0,
                            NULL);
    } else {
      dumpStatus = true;
      serror.Format("PS FILE OUTPUT SKIPPED - file is empty! '%s'", (LPCTSTR)realFileName);
      addToLogFile(lightLOG_WARNING, serror);
    }
    
    delete contentStack;
      
    if (!dumpStatus) {
      serror.Format("PS I/O Error processing '%s': %s", (LPCTSTR)realFileName, (LPCTSTR)error);
      addToLogFile(lightLOG_ERROR, serror);
      
      fclose(f);
      return false;
    } else {
      destination->addDumpElement(this, "PSELEMENT", realFileName);      
      
      serror.Format("PS file '%s' written successfully", (LPCTSTR)realFileName);
      addToLogFile(lightLOG_INFO, serror);
    }
     
    fflush(f);
    fclose(f);
    
    return true;
  }
  
  serror.Format("PS I/O: unable to write file '%s'", (LPCTSTR)realFileName);
  addToLogFile(lightLOG_ERROR, serror);
  
  delete realFileName;
  return false;
}

///////////////////////////////////////////////

bool indPDFPreviewDumperC::dumpFilteredPreviewImage(pdfXString somePath) {
  return true;
}


