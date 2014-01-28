/*
 * Copyright (c) 2014 Christian Hansen <chansen@cpan.org>
 * <https://github.com/chansen/c-timestamp>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "timestamp.h"

static const uint16_t DayOffset[13] = {
    0, 306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275
};

static void
rdn_to_ymd(uint32_t rdn, uint16_t *yp, uint16_t *mp, uint16_t *dp) {
    uint16_t n100, n1, y, m;
    uint32_t d;

    d = rdn + 305;
    y = 400 * (d / 146097);
    d %= 146097;

    n100 = d / 36524;
    y += 100 * n100;
    d %= 36524;

    y += 4 * (d / 1461);
    d %= 1461;

    n1 = d / 365;
    y += n1;
    d %= 365;

    if (n100 == 4 || n1 == 4)
        d = 366;
    else
        y++, d++;

    m = (5 * d + 456) / 153;
    if   (m > 12) m -= 12;
    else          y -= 1;

    *yp = y;
    *mp = m;
    *dp = d - DayOffset[m];
}

#define MIN_SEC  INT64_C(-62135596800) /* 0001-01-01T00:00:00 */
#define MAX_SEC  INT64_C(253402300799) /* 9999-12-31T23:59:59 */
#define EPOCH    INT64_C(62135683200)  /* 1970-01-01T00:00:00 */

static int
timestamp_valid(const timestamp_t *tsp) {
    const int64_t sec = tsp->sec + tsp->offset * 60;
    if (sec < MIN_SEC || sec > MAX_SEC ||
        tsp->nsec < 0 || tsp->nsec > 999999999 ||
        tsp->offset < -1439 || tsp->offset > 1439)
        return 0;
    return 1;
}

size_t
timestamp_format(char *dst, size_t len, const timestamp_t *tsp) {
    unsigned char *p;
    uint64_t sec;
    uint32_t rdn, f, v;
    uint16_t y, m, d;
    size_t need, flen;

    if (!timestamp_valid(tsp))
        return 0;

    need = sizeof("YYYY-MM-DDThh:mm:ssZ");
    if (tsp->offset)
        need += 5; /* hh:mm */

    f = tsp->nsec;
    if (!f)
        flen = 0;
    else {
        if      ((f % 1000000) == 0) f /= 1000000, flen =  4; /* .milli */
        else if ((f % 1000)    == 0) f /=    1000, flen =  7; /* .micro */
        else                                       flen = 10; /* .nano  */
        need += flen;
    }

    if (need > len)
        return 0;

    sec = tsp->sec + tsp->offset * 60 + EPOCH;
    rdn = sec / 86400;

    rdn_to_ymd(rdn, &y, &m, &d);

    p = (unsigned char *)dst;
    p[3] = '0' + (y % 10); y /= 10;
    p[2] = '0' + (y % 10); y /= 10;
    p[1] = '0' + (y % 10); y /= 10;
    p[0] = '0' + (y % 10);
    p += 4;

    p[2] = '0' + (m % 10); m /= 10;
    p[1] = '0' + (m % 10);
    p[0] = '-';
    p += 3;

    p[2] = '0' + (d % 10); d /= 10;
    p[1] = '0' + (d % 10);
    p[0] = '-';
    p += 3;

    v = sec % 86400;
    p[8] = '0' + (v % 10); v /= 10;
    p[7] = '0' + (v %  6); v /=  6;
    p[6] = ':';
    p[5] = '0' + (v % 10); v /= 10;
    p[4] = '0' + (v %  6); v /=  6;
    p[3] = ':';
    p[2] = '0' + (v % 10); v /= 10;
    p[1] = '0' + (v % 10);
    p[0] = 'T';
    p += 9;

    if (flen) {
        if (flen > 7) {
            p[9] = '0' + (f % 10); f /= 10;
            p[8] = '0' + (f % 10); f /= 10;
            p[7] = '0' + (f % 10); f /= 10;
        }
        if (flen > 4) {
            p[6] = '0' + (f % 10); f /= 10;
            p[5] = '0' + (f % 10); f /= 10;
            p[4] = '0' + (f % 10); f /= 10;
        }
        p[3] = '0' + (f % 10); f /= 10;
        p[2] = '0' + (f % 10); f /= 10;
        p[1] = '0' + (f % 10);
        p[0] = '.';
        p += flen;
    }

    if (!tsp->offset)
        *p++ = 'Z';
    else {
        if (tsp->offset < 0)
            p[0] = '-', v = -tsp->offset;
        else
            p[0] = '+', v = tsp->offset;

        p[5] = '0' + (v % 10); v /= 10;
        p[4] = '0' + (v %  6); v /=  6;
        p[3] = ':';
        p[2] = '0' + (v % 10); v /= 10;
        p[1] = '0' + (v % 10);
        p += 6;
    }
    *p = 0;
    return p - (unsigned char *)dst;
}

