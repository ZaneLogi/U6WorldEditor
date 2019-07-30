#pragma once

#include <stdint.h>

bool ClipRect(int& xdst, int& ydst, int wdst, int hdst,
              int& xsrc, int& ysrc, int wsrc, int hsrc,
              int& wrect, int& hrect);

void CopyBits(void* dst, int dst_yptich,
              void* src, int src_yPitch,
              int w, int h, int bitcount);

inline
void CopyBits(void* pDst, int xDst, int yDst, int xDstPitch, int yDstPitch,
    void* pSrc, int xSrc, int ySrc, int xSrcPitch, int ySrcPitch,
    int w, int h, int bitcount)
{
    pDst = ((char*)pDst) + yDst * yDstPitch + xDst * xDstPitch;
    pSrc = ((char*)pSrc) + ySrc * ySrcPitch + xSrc * xSrcPitch;
    CopyBits(pDst, yDstPitch, pSrc, ySrcPitch, w, h, bitcount);
}


void FillRect(void* dst, int w, int h, uint32_t color, int xpitch, int ypitch);
