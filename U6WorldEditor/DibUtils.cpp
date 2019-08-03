#include "stdafx.h"
#include <cassert>
#include "DibUtils.h"

// source rectangle      ( xsrc, ysrc, xsrc + wrect, ysrc + hrect )
// destination rectangle ( xdst, ydst, xdst + wrect, ydst + hrect )
// source boundary       ( 0, 0, wsrc, hsrc )
// destination boundary  ( 0, 0, wdst, hdst )
bool ClipRect(int& xdst, int& ydst, int wdst, int hdst,
              int& xsrc, int& ysrc, int wsrc, int hsrc,
              int& wrect, int& hrect)
{
    assert(wdst >= 0 && hdst >= 0);
    assert(wsrc >= 0 && hsrc >= 0);
    assert(wrect >= 0 && hrect >= 0);

    // check source rectangle and source boundary
    if (xsrc >= wsrc || ysrc >= hsrc || xsrc + wrect <= 0 || ysrc + hrect <= 0)
        return false;

    // clip source rectangle
    if (xsrc < 0)
    {
        wrect += xsrc; // shorten
        xdst -= xsrc;
        xsrc = 0;
    }

    if (xsrc + wrect >= wsrc)
    {
        wrect = wsrc - xsrc; // shorten
    }

    if (ysrc < 0)
    {
        hrect += ysrc;
        ydst -= ysrc;
        ysrc = 0;
    }

    if (ysrc + hrect >= hsrc)
    {
        hrect = hsrc - ysrc;
    }

    // check destination rectangle and destination boundary
    if (xdst >= wdst || ydst >= hdst || xdst + wrect <= 0 || ydst + hrect <= 0)
        return false;

    // clip destination rectangle
    if (xdst < 0)
    {
        wrect += xdst;
        xsrc -= xdst;
        xdst = 0;
    }

    if (xdst + wrect >= wdst)
    {
        wrect = wdst - xdst;
    }

    if (ydst < 0)
    {
        hrect += ydst;
        ysrc -= ydst;
        ydst = 0;
    }

    if (ydst + hrect >= hdst)
    {
        hrect = hdst - ydst;
    }

    return true;
}

// dst     start bits of the destination
// src     start bits of the source
// w        width
// h        height
void CopyBits(void* dst, int dst_yptich, void* src, int src_yPitch, int w, int h, int bitcount)
{
    int bytes = (w * bitcount + 7) >> 3;
    if (bytes <= 0)
        return;

    char* pd = (char*)dst;
    char* ps = (char*)src;

    while (h-- > 0)
    {
        memcpy(pd, ps, bytes);
        pd += dst_yptich;
        ps += src_yPitch;
    }
}

void FillRect(void* dst, int w, int h, uint32_t color, int xpitch, int ypitch)
{
    if (h <= 0)
        return;

    // first line
    char* p = (char*)dst;
    if (xpitch == 1)
    {
        memset(p, color, w);
    }
    else
    {
        for (int i = 0; i < w; i++)
        {
            memcpy(p, &color, xpitch);
            p += xpitch;
        }
    }

    --h;
    dst = (char*)dst + ypitch;

    // other lines
    int bytes = w * xpitch;
    while (h-- > 0)
    {
        memcpy(dst, (char*)dst - ypitch, bytes);
        dst = (char*)dst + ypitch;
    }
}
