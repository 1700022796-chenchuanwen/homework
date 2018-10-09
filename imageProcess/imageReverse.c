#include "stdio.h"
#include "image.h"

#define be16_to_cpu(x) (((x)[0]<<8)|(x)[1])
/* Global variable to return the last error found while deconding */
static char error_string[256];

#define error(fmt, args...) do { \
   snprintf(error_string, sizeof(error_string), fmt, ## args); \
   return -1; \
} while(0)

static const unsigned char zigzag[64] = 
{
   0,  1,  5,  6, 14, 15, 27, 28,
   2,  4,  7, 13, 16, 26, 29, 42,
   3,  8, 12, 17, 25, 30, 41, 43,
   9, 11, 18, 24, 31, 40, 44, 53,
  10, 19, 23, 32, 39, 45, 52, 54,
  20, 22, 33, 38, 46, 51, 55, 60,
  21, 34, 37, 47, 50, 56, 59, 61,
  35, 36, 48, 49, 57, 58, 62, 63
};
void exitmessage(char *msg){
    printf("%s", msg);
    exit(0);
}

char* tinyjpeg_get_errorstring(struct jdec_private *priv){
    return "error";
}

/*******************************************************************************
 *
 * Colorspace conversion routine
 *
 *
 * Note:
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *      R = Y                + 1.40200 * Cr
 *      G = Y - 0.34414 * Cb - 0.71414 * Cr
 *      B = Y + 1.77200 * Cb
 * 
 ******************************************************************************/

static unsigned char clamp(int i)
{
  if (i<0)
    return 0;
  else if (i>255)
    return 255;
  else
    return i;
}   


/**
 *  YCrCb -> YUV420P (1x1)
 *  .---.
 *  | 1 |
 *  `---'
 */
static void YCrCB_to_YUV420P_1x1(struct jdec_private *priv)
{
  const unsigned char *s, *y;
  unsigned char *p;
  int i,j;

  p = priv->plane[0];
  y = priv->Y;
  for (i=0; i<8; i++)
   {
     memcpy(p, y, 8);
     p+=priv->width;
     y+=8;
   }

  p = priv->plane[1];
  s = priv->Cb;
  for (i=0; i<8; i+=2)
   {
     for (j=0; j<8; j+=2, s+=2)
       *p++ = *s;
     s += 8; /* Skip one line */
     p += priv->width/2 - 4;
   }

  p = priv->plane[2];
  s = priv->Cr;
  for (i=0; i<8; i+=2)
   {
     for (j=0; j<8; j+=2, s+=2)
       *p++ = *s;
     s += 8; /* Skip one line */
     p += priv->width/2 - 4;
   }
}

/**
 *  YCrCb -> YUV420P (2x1)
 *  .-------.
 *  | 1 | 2 |
 *  `-------'
 */
static void YCrCB_to_YUV420P_2x1(struct jdec_private *priv)
{
  unsigned char *p;
  const unsigned char *s, *y1;
  unsigned int i;

  p = priv->plane[0];
  y1 = priv->Y;
  for (i=0; i<8; i++)
   {
     memcpy(p, y1, 16);
     p += priv->width;
     y1 += 16;
   }

  p = priv->plane[1];
  s = priv->Cb;
  for (i=0; i<8; i+=2)
   {
     memcpy(p, s, 8);
     s += 16; /* Skip one line */
     p += priv->width/2;
   }

  p = priv->plane[2];
  s = priv->Cr;
  for (i=0; i<8; i+=2)
   {
     memcpy(p, s, 8);
     s += 16; /* Skip one line */
     p += priv->width/2;
   }
}


/**
 *  YCrCb -> YUV420P (1x2)
 *  .---.
 *  | 1 |
 *  |---|
 *  | 2 |
 *  `---'
 */
static void YCrCB_to_YUV420P_1x2(struct jdec_private *priv)
{
  const unsigned char *s, *y;
  unsigned char *p;
  int i,j;

  p = priv->plane[0];
  y = priv->Y;
  for (i=0; i<16; i++)
   {
     memcpy(p, y, 8);
     p+=priv->width;
     y+=8;
   }

  p = priv->plane[1];
  s = priv->Cb;
  for (i=0; i<8; i++)
   {
     for (j=0; j<8; j+=2, s+=2)
       *p++ = *s;
     p += priv->width/2 - 4;
   }

  p = priv->plane[2];
  s = priv->Cr;
  for (i=0; i<8; i++)
   {
     for (j=0; j<8; j+=2, s+=2)
       *p++ = *s;
     p += priv->width/2 - 4;
   }
}

/**
 *  YCrCb -> YUV420P (2x2)
 *  .-------.
 *  | 1 | 2 |
 *  |---+---|
 *  | 3 | 4 |
 *  `-------'
 */
static void YCrCB_to_YUV420P_2x2(struct jdec_private *priv)
{
  unsigned char *p;
  const unsigned char *s, *y1;
  unsigned int i;

  p = priv->plane[0];
  y1 = priv->Y;
  for (i=0; i<16; i++)
   {
     memcpy(p, y1, 16);
     p += priv->width;
     y1 += 16;
   }

  p = priv->plane[1];
  s = priv->Cb;
  for (i=0; i<8; i++)
   {
     memcpy(p, s, 8);
     s += 8;
     p += priv->width/2;
   }

  p = priv->plane[2];
  s = priv->Cr;
  for (i=0; i<8; i++)
   {
     memcpy(p, s, 8);
     s += 8;
     p += priv->width/2;
   }
}

/**
 *  YCrCb -> RGB24 (1x1)
 *  .---.
 *  | 1 |
 *  `---'
 */
static void YCrCB_to_RGB24_1x1(struct jdec_private *priv)
{
  const unsigned char *Y, *Cb, *Cr;
  unsigned char *p;
  int i,j;
  int offset_to_next_row;

#define SCALEBITS       10
#define ONE_HALF        (1UL << (SCALEBITS-1))
#define FIX(x)          ((int)((x) * (1UL<<SCALEBITS) + 0.5))

  p = priv->plane[0];
  Y = priv->Y;
  Cb = priv->Cb;
  Cr = priv->Cr;
  offset_to_next_row = priv->width*3 - 8*3;
  for (i=0; i<8; i++) {

    for (j=0; j<8; j++) {

       int y, cb, cr;
       int add_r, add_g, add_b;
       int r, g , b;

       y  = (*Y++) << SCALEBITS;
       cb = *Cb++ - 128;
       cr = *Cr++ - 128;
       add_r = FIX(1.40200) * cr + ONE_HALF;
       add_g = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;
       add_b = FIX(1.77200) * cb + ONE_HALF;

       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);

    }

    p += offset_to_next_row;
  }

#undef SCALEBITS
#undef ONE_HALF
#undef FIX

}

/**
 *  YCrCb -> BGR24 (1x1)
 *  .---.
 *  | 1 |
 *  `---'
 */
static void YCrCB_to_BGR24_1x1(struct jdec_private *priv)
{
  const unsigned char *Y, *Cb, *Cr;
  unsigned char *p;
  int i,j;
  int offset_to_next_row;

#define SCALEBITS       10
#define ONE_HALF        (1UL << (SCALEBITS-1))
#define FIX(x)          ((int)((x) * (1UL<<SCALEBITS) + 0.5))

  p = priv->plane[0];
  Y = priv->Y;
  Cb = priv->Cb;
  Cr = priv->Cr;
  offset_to_next_row = priv->width*3 - 8*3;
  for (i=0; i<8; i++) {

    for (j=0; j<8; j++) {

       int y, cb, cr;
       int add_r, add_g, add_b;
       int r, g , b;

       y  = (*Y++) << SCALEBITS;
       cb = *Cb++ - 128;
       cr = *Cr++ - 128;
       add_r = FIX(1.40200) * cr + ONE_HALF;
       add_g = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;
       add_b = FIX(1.77200) * cb + ONE_HALF;

       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);

    }

    p += offset_to_next_row;
  }

#undef SCALEBITS
#undef ONE_HALF
#undef FIX

}


/**
 *  YCrCb -> RGB24 (2x1)
 *  .-------.
 *  | 1 | 2 |
 *  `-------'
 */
static void YCrCB_to_RGB24_2x1(struct jdec_private *priv)
{
  const unsigned char *Y, *Cb, *Cr;
  unsigned char *p;
  int i,j;
  int offset_to_next_row;

#define SCALEBITS       10
#define ONE_HALF        (1UL << (SCALEBITS-1))
#define FIX(x)          ((int)((x) * (1UL<<SCALEBITS) + 0.5))

  p = priv->plane[0];
  Y = priv->Y;
  Cb = priv->Cb;
  Cr = priv->Cr;
  offset_to_next_row = priv->width*3 - 16*3;
  for (i=0; i<8; i++) {

    for (j=0; j<8; j++) {

       int y, cb, cr;
       int add_r, add_g, add_b;
       int r, g , b;

       y  = (*Y++) << SCALEBITS;
       cb = *Cb++ - 128;
       cr = *Cr++ - 128;
       add_r = FIX(1.40200) * cr + ONE_HALF;
       add_g = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;
       add_b = FIX(1.77200) * cb + ONE_HALF;

       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);

       y  = (*Y++) << SCALEBITS;
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);

    }

    p += offset_to_next_row;
  }

#undef SCALEBITS
#undef ONE_HALF
#undef FIX

}

/*
 *  YCrCb -> BGR24 (2x1)
 *  .-------.
 *  | 1 | 2 |
 *  `-------'
 */
static void YCrCB_to_BGR24_2x1(struct jdec_private *priv)
{
  const unsigned char *Y, *Cb, *Cr;
  unsigned char *p;
  int i,j;
  int offset_to_next_row;

#define SCALEBITS       10
#define ONE_HALF        (1UL << (SCALEBITS-1))
#define FIX(x)          ((int)((x) * (1UL<<SCALEBITS) + 0.5))

  p = priv->plane[0];
  Y = priv->Y;
  Cb = priv->Cb;
  Cr = priv->Cr;
  offset_to_next_row = priv->width*3 - 16*3;
  for (i=0; i<8; i++) {

    for (j=0; j<8; j++) {

       int y, cb, cr;
       int add_r, add_g, add_b;
       int r, g , b;

       cb = *Cb++ - 128;
       cr = *Cr++ - 128;
       add_r = FIX(1.40200) * cr + ONE_HALF;
       add_g = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;
       add_b = FIX(1.77200) * cb + ONE_HALF;

       y  = (*Y++) << SCALEBITS;
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);

       y  = (*Y++) << SCALEBITS;
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);

    }

    p += offset_to_next_row;
  }

#undef SCALEBITS
#undef ONE_HALF
#undef FIX

}

/**
 *  YCrCb -> RGB24 (1x2)
 *  .---.
 *  | 1 |
 *  |---|
 *  | 2 |
 *  `---'
 */
static void YCrCB_to_RGB24_1x2(struct jdec_private *priv)
{
  const unsigned char *Y, *Cb, *Cr;
  unsigned char *p, *p2;
  int i,j;
  int offset_to_next_row;

#define SCALEBITS       10
#define ONE_HALF        (1UL << (SCALEBITS-1))
#define FIX(x)          ((int)((x) * (1UL<<SCALEBITS) + 0.5))

  p = priv->plane[0];
  p2 = priv->plane[0] + priv->width*3;
  Y = priv->Y;
  Cb = priv->Cb;
  Cr = priv->Cr;
  offset_to_next_row = 2*priv->width*3 - 8*3;
  for (i=0; i<8; i++) {

    for (j=0; j<8; j++) {

       int y, cb, cr;
       int add_r, add_g, add_b;
       int r, g , b;

       cb = *Cb++ - 128;
       cr = *Cr++ - 128;
       add_r = FIX(1.40200) * cr + ONE_HALF;
       add_g = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;
       add_b = FIX(1.77200) * cb + ONE_HALF;

       y  = (*Y++) << SCALEBITS;
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);

       y  = (Y[8-1]) << SCALEBITS;
       r = (y + add_r) >> SCALEBITS;
       *p2++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p2++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p2++ = clamp(b);

    }
    Y += 8;
    p += offset_to_next_row;
    p2 += offset_to_next_row;
  }

#undef SCALEBITS
#undef ONE_HALF
#undef FIX

}

/*
 *  YCrCb -> BGR24 (1x2)
 *  .---.
 *  | 1 |
 *  |---|
 *  | 2 |
 *  `---'
 */
static void YCrCB_to_BGR24_1x2(struct jdec_private *priv)
{
  const unsigned char *Y, *Cb, *Cr;
  unsigned char *p, *p2;
  int i,j;
  int offset_to_next_row;

#define SCALEBITS       10
#define ONE_HALF        (1UL << (SCALEBITS-1))
#define FIX(x)          ((int)((x) * (1UL<<SCALEBITS) + 0.5))

  p = priv->plane[0];
  p2 = priv->plane[0] + priv->width*3;
  Y = priv->Y;
  Cb = priv->Cb;
  Cr = priv->Cr;
  offset_to_next_row = 2*priv->width*3 - 8*3;
  for (i=0; i<8; i++) {

    for (j=0; j<8; j++) {

       int y, cb, cr;
       int add_r, add_g, add_b;
       int r, g , b;

       cb = *Cb++ - 128;
       cr = *Cr++ - 128;
       add_r = FIX(1.40200) * cr + ONE_HALF;
       add_g = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;
       add_b = FIX(1.77200) * cb + ONE_HALF;

       y  = (*Y++) << SCALEBITS;
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);

       y  = (Y[8-1]) << SCALEBITS;
       b = (y + add_b) >> SCALEBITS;
       *p2++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p2++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p2++ = clamp(r);

    }
    Y += 8;
    p += offset_to_next_row;
    p2 += offset_to_next_row;
  }

#undef SCALEBITS
#undef ONE_HALF
#undef FIX

}


/**
 *  YCrCb -> RGB24 (2x2)
 *  .-------.
 *  | 1 | 2 |
 *  |---+---|
 *  | 3 | 4 |
 *  `-------'
 */
static void YCrCB_to_RGB24_2x2(struct jdec_private *priv)
{
  const unsigned char *Y, *Cb, *Cr;
  unsigned char *p, *p2;
  int i,j;
  int offset_to_next_row;

#define SCALEBITS       10
#define ONE_HALF        (1UL << (SCALEBITS-1))
#define FIX(x)          ((int)((x) * (1UL<<SCALEBITS) + 0.5))

  p = priv->plane[0];
  p2 = priv->plane[0] + priv->width*3;
  Y = priv->Y;
  Cb = priv->Cb;
  Cr = priv->Cr;
  offset_to_next_row = (priv->width*3*2) - 16*3;
  for (i=0; i<8; i++) {

    for (j=0; j<8; j++) {

       int y, cb, cr;
       int add_r, add_g, add_b;
       int r, g , b;

       cb = *Cb++ - 128;
       cr = *Cr++ - 128;
       add_r = FIX(1.40200) * cr + ONE_HALF;
       add_g = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;
       add_b = FIX(1.77200) * cb + ONE_HALF;

       y  = (*Y++) << SCALEBITS;
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);

       y  = (*Y++) << SCALEBITS;
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);

       y  = (Y[16-2]) << SCALEBITS;
       r = (y + add_r) >> SCALEBITS;
       *p2++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p2++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p2++ = clamp(b);

       y  = (Y[16-1]) << SCALEBITS;
       r = (y + add_r) >> SCALEBITS;
       *p2++ = clamp(r);
       g = (y + add_g) >> SCALEBITS;
       *p2++ = clamp(g);
       b = (y + add_b) >> SCALEBITS;
       *p2++ = clamp(b);
    }
    Y  += 16;
    p  += offset_to_next_row;
    p2 += offset_to_next_row;
  }

#undef SCALEBITS
#undef ONE_HALF
#undef FIX

}


/*
 *  YCrCb -> BGR24 (2x2)
 *  .-------.
 *  | 1 | 2 |
 *  |---+---|
 *  | 3 | 4 |
 *  `-------'
 */
static void YCrCB_to_BGR24_2x2(struct jdec_private *priv)
{
  const unsigned char *Y, *Cb, *Cr;
  unsigned char *p, *p2;
  int i,j;
  int offset_to_next_row;

#define SCALEBITS       10
#define ONE_HALF        (1UL << (SCALEBITS-1))
#define FIX(x)          ((int)((x) * (1UL<<SCALEBITS) + 0.5))

  p = priv->plane[0];
  p2 = priv->plane[0] + priv->width*3;
  Y = priv->Y;
  Cb = priv->Cb;
  Cr = priv->Cr;
  offset_to_next_row = (priv->width*3*2) - 16*3;
  for (i=0; i<8; i++) {

    for (j=0; j<8; j++) {

       int y, cb, cr;
       int add_r, add_g, add_b;
       int r, g , b;

       cb = *Cb++ - 128;
       cr = *Cr++ - 128;
       add_r = FIX(1.40200) * cr + ONE_HALF;
       add_g = - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;
       add_b = FIX(1.77200) * cb + ONE_HALF;

       y  = (*Y++) << SCALEBITS;
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);

       y  = (*Y++) << SCALEBITS;
       b = (y + add_b) >> SCALEBITS;
       *p++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p++ = clamp(r);

       y  = (Y[16-2]) << SCALEBITS;
       b = (y + add_b) >> SCALEBITS;
       *p2++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p2++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p2++ = clamp(r);

       y  = (Y[16-1]) << SCALEBITS;
       b = (y + add_b) >> SCALEBITS;
       *p2++ = clamp(b);
       g = (y + add_g) >> SCALEBITS;
       *p2++ = clamp(g);
       r = (y + add_r) >> SCALEBITS;
       *p2++ = clamp(r);
    }
    Y  += 16;
    p  += offset_to_next_row;
    p2 += offset_to_next_row;
  }

#undef SCALEBITS
#undef ONE_HALF
#undef FIX

}



/**
 *  YCrCb -> Grey (1x1)
 *  .---.
 *  | 1 |
 *  `---'
 */
static void YCrCB_to_Grey_1x1(struct jdec_private *priv)
{
  const unsigned char *y;
  unsigned char *p;
  unsigned int i;
  int offset_to_next_row;

  p = priv->plane[0];
  y = priv->Y;
  offset_to_next_row = priv->width;

  for (i=0; i<8; i++) {
     memcpy(p, y, 8);
     y+=8;
     p += offset_to_next_row;
  }
}

/**
 *  YCrCb -> Grey (2x1)
 *  .-------.
 *  | 1 | 2 |
 *  `-------'
 */
static void YCrCB_to_Grey_2x1(struct jdec_private *priv)
{
  const unsigned char *y;
  unsigned char *p;
  unsigned int i;

  p = priv->plane[0];
  y = priv->Y;

  for (i=0; i<8; i++) {
     memcpy(p, y, 16);
     y += 16;
     p += priv->width;
  }
}


/**
 *  YCrCb -> Grey (1x2)
 *  .---.
 *  | 1 |
 *  |---|
 *  | 2 |
 *  `---'
 */
static void YCrCB_to_Grey_1x2(struct jdec_private *priv)
{
  const unsigned char *y;
  unsigned char *p;
  unsigned int i;

  p = priv->plane[0];
  y = priv->Y;

  for (i=0; i<16; i++) {
     memcpy(p, y, 8);
     y += 8;
     p += priv->width;
  }
}

/**
 *  YCrCb -> Grey (2x2)
 *  .-------.
 *  | 1 | 2 |
 *  |---+---|
 *  | 3 | 4 |
 *  `-------'
 */
static void YCrCB_to_Grey_2x2(struct jdec_private *priv)
{
  const unsigned char *y;
  unsigned char *p;
  unsigned int i;

  p = priv->plane[0];
  y = priv->Y;

  for (i=0; i<16; i++) {
     memcpy(p, y, 16);
     y += 16;
     p += priv->width;
  }
}


/*
 * Decode all the 3 components for 1x1 
 */
static void decode_MCU_1x1_3planes(struct jdec_private *priv)
{
  // Y
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y, 8);
  
  // Cb
  process_Huffman_data_unit(priv, cCb);
  IDCT(&priv->component_infos[cCb], priv->Cb, 8);

  // Cr
  process_Huffman_data_unit(priv, cCr);
  IDCT(&priv->component_infos[cCr], priv->Cr, 8);
}

/*
 * Decode a 1x1 directly in 1 color
 */
static void decode_MCU_1x1_1plane(struct jdec_private *priv)
{
  // Y
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y, 8);
  
  // Cb
  process_Huffman_data_unit(priv, cCb);
  IDCT(&priv->component_infos[cCb], priv->Cb, 8);

  // Cr
  process_Huffman_data_unit(priv, cCr);
  IDCT(&priv->component_infos[cCr], priv->Cr, 8);
}


/*
 * Decode a 2x1
 *  .-------.
 *  | 1 | 2 |
 *  `-------'
 */
static void decode_MCU_2x1_3planes(struct jdec_private *priv)
{
  // Y
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y, 16);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+8, 16);

  // Cb
  process_Huffman_data_unit(priv, cCb);
  IDCT(&priv->component_infos[cCb], priv->Cb, 8);

  // Cr
  process_Huffman_data_unit(priv, cCr);
  IDCT(&priv->component_infos[cCr], priv->Cr, 8);
}

/*
 * Decode a 2x1
 *  .-------.
 *  | 1 | 2 |
 *  `-------'
 */
static void decode_MCU_2x1_1plane(struct jdec_private *priv)
{
  // Y
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y, 16);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+8, 16);

  // Cb
  process_Huffman_data_unit(priv, cCb);

  // Cr
  process_Huffman_data_unit(priv, cCr);
}


/*
 * Decode a 2x2
 *  .-------.
 *  | 1 | 2 |
 *  |---+---|
 *  | 3 | 4 |
 *  `-------'
 */
static void decode_MCU_2x2_3planes(struct jdec_private *priv)
{
  // Y
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y, 16);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+8, 16);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+64*2, 16);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+64*2+8, 16);

  // Cb
  process_Huffman_data_unit(priv, cCb);
  IDCT(&priv->component_infos[cCb], priv->Cb, 8);

  // Cr
  process_Huffman_data_unit(priv, cCr);
  IDCT(&priv->component_infos[cCr], priv->Cr, 8);
}

/*
 * Decode a 2x2 directly in GREY format (8bits)
 *  .-------.
 *  | 1 | 2 |
 *  |---+---|
 *  | 3 | 4 |
 *  `-------'
 */
static void decode_MCU_2x2_1plane(struct jdec_private *priv)
{
  // Y
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y, 16);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+8, 16);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+64*2, 16);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+64*2+8, 16);

  // Cb
  process_Huffman_data_unit(priv, cCb);

  // Cr
  process_Huffman_data_unit(priv, cCr);
}

/*
 * Decode a 1x2 mcu
 *  .---.
 *  | 1 |
 *  |---|
 *  | 2 |
 *  `---'
 */
static void decode_MCU_1x2_3planes(struct jdec_private *priv)
{
  // Y
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y, 8);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+64, 8);

  // Cb
  process_Huffman_data_unit(priv, cCb);
  IDCT(&priv->component_infos[cCb], priv->Cb, 8);

  // Cr
  process_Huffman_data_unit(priv, cCr);
  IDCT(&priv->component_infos[cCr], priv->Cr, 8);
}

/*
 * Decode a 1x2 mcu
 *  .---.
 *  | 1 |
 *  |---|
 *  | 2 |
 *  `---'
 */
static void decode_MCU_1x2_1plane(struct jdec_private *priv)
{
  // Y
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y, 8);
  process_Huffman_data_unit(priv, cY);
  IDCT(&priv->component_infos[cY], priv->Y+64, 8);

  // Cb
  process_Huffman_data_unit(priv, cCb);

  // Cr
  process_Huffman_data_unit(priv, cCr);
}

void build_huffman_table(const unsigned char* bits, const  unsigned char* vals, struct huffman_table *table){
    
}


void build_default_huffman_tables(struct jdec_private *priv){
    if( (priv->flags & TINYJPEG_FLAGS_MJPEG_TABLE) && priv->default_huffman_table_initialized){
        return;
    }

    build_huffman_table(bits_dc_luminance, val_dc_luminance, &priv->HTDC[0]);
    build_huffman_table(bits_dc_luminance, val_ac_luminance, &priv->HTAC[0]);

    build_huffman_table(bits_dc_chrominance, val_dc_chrominance, &priv->HTDC[1]);
    build_huffman_table(bits_dc_chrominance, val_ac_chrominance, &priv->HTAC[1]);

    priv->default_huffman_table_initialized =1;
}






void tinyjpeg_get_size(struct jdec_private* jdec, int* width, int* length){

}


static int find_next_rst_marker(struct jdec_private *priv)
{
  int rst_marker_found = 0;
  int marker;
  const unsigned char *stream = priv->stream;

  /* Parse marker */
  while (!rst_marker_found)
   {
     while (*stream++ != 0xff)
      {
	if (stream >= priv->stream_end)
	  error("EOF while search for a RST marker.");
      }
     /* Skip any padding ff byte (this is normal) */
     while (*stream == 0xff)
       stream++;

     marker = *stream++;
     if ((RST+priv->last_rst_marker_seen) == marker)
       rst_marker_found = 1;
     else if (marker >= RST && marker <= RST7)
       error("Wrong Reset marker found, abording");
     else if (marker == EOI)
       return 0;
   }
  trace("RST Marker %d found at offset %d\n", priv->last_rst_marker_seen, stream - priv->stream_begin);

  priv->stream = stream;
  priv->last_rst_marker_seen++;
  priv->last_rst_marker_seen &= 7;

  return 0;
}
static const decode_MCU_fct decode_mcu_1comp_table[4] = {
   decode_MCU_1x1_1plane,
   decode_MCU_1x2_1plane,
   decode_MCU_2x1_1plane,
   decode_MCU_2x2_1plane,
};
static const convert_colorspace_fct convert_colorspace_yuv420p[4] = {
   YCrCB_to_YUV420P_1x1,
   YCrCB_to_YUV420P_1x2,
   YCrCB_to_YUV420P_2x1,
   YCrCB_to_YUV420P_2x2,
};

static const convert_colorspace_fct convert_colorspace_rgb24[4] = {
   YCrCB_to_RGB24_1x1,
   YCrCB_to_RGB24_1x2,
   YCrCB_to_RGB24_2x1,
   YCrCB_to_RGB24_2x2,
};

static const convert_colorspace_fct convert_colorspace_bgr24[4] = {
   YCrCB_to_BGR24_1x1,
   YCrCB_to_BGR24_1x2,
   YCrCB_to_BGR24_2x1,
   YCrCB_to_BGR24_2x2,
};

static const convert_colorspace_fct convert_colorspace_grey[4] = {
   YCrCB_to_Grey_1x1,
   YCrCB_to_Grey_1x2,
   YCrCB_to_Grey_2x1,
   YCrCB_to_Grey_2x2,
};

static void resync(struct jdec_private *priv)
{
  int i;

  /* Init DC coefficients */
  for (i=0; i<COMPONENTS; i++)
     priv->component_infos[i].previous_DC = 0;

  priv->reservior = 0;
  priv->nbits_in_reservoir = 0;
  if (priv->restart_interval > 0)
    priv->restarts_to_go = priv->restart_interval;
  else
    priv->restarts_to_go = -1;
}

int tinyjpeg_decode(struct jdec_private* priv, int pixfmt){
  unsigned int x, y, xstride_by_mcu, ystride_by_mcu;
  unsigned int bytes_per_blocklines[3], bytes_per_mcu[3];
  decode_MCU_fct decode_MCU;
  const decode_MCU_fct *decode_mcu_table;
  const convert_colorspace_fct *colorspace_array_conv;
  convert_colorspace_fct convert_to_pixfmt;

  if (setjmp(priv->jump_state))
    return -1;

  /* To keep gcc happy initialize some array */
  bytes_per_mcu[1] = 0;
  bytes_per_mcu[2] = 0;
  bytes_per_blocklines[1] = 0;
  bytes_per_blocklines[2] = 0;

  decode_mcu_table = decode_mcu_3comp_table;
  switch (pixfmt) {
     case TINYJPEG_FMT_YUV420P:
       colorspace_array_conv = convert_colorspace_yuv420p;
       if (priv->components[0] == NULL)
	 priv->components[0] = (uint8_t *)malloc(priv->width * priv->height);
       if (priv->components[1] == NULL)
	 priv->components[1] = (uint8_t *)malloc(priv->width * priv->height/4);
       if (priv->components[2] == NULL)
	 priv->components[2] = (uint8_t *)malloc(priv->width * priv->height/4);
       bytes_per_blocklines[0] = priv->width;
       bytes_per_blocklines[1] = priv->width/4;
       bytes_per_blocklines[2] = priv->width/4;
       bytes_per_mcu[0] = 8;
       bytes_per_mcu[1] = 4;
       bytes_per_mcu[2] = 4;
       break;

     case TINYJPEG_FMT_RGB24:
       colorspace_array_conv = convert_colorspace_rgb24;
       if (priv->components[0] == NULL)
	 priv->components[0] = (uint8_t *)malloc(priv->width * priv->height * 3);
       bytes_per_blocklines[0] = priv->width * 3;
       bytes_per_mcu[0] = 3*8;
       break;

     case TINYJPEG_FMT_BGR24:
       colorspace_array_conv = convert_colorspace_bgr24;
       if (priv->components[0] == NULL)
	 priv->components[0] = (uint8_t *)malloc(priv->width * priv->height * 3);
       bytes_per_blocklines[0] = priv->width * 3;
       bytes_per_mcu[0] = 3*8;
       break;

     case TINYJPEG_FMT_GREY:
       decode_mcu_table = decode_mcu_1comp_table;
       colorspace_array_conv = convert_colorspace_grey;
       if (priv->components[0] == NULL)
	 priv->components[0] = (uint8_t *)malloc(priv->width * priv->height);
       bytes_per_blocklines[0] = priv->width;
       bytes_per_mcu[0] = 8;
       break;

     default:
       trace("Bad pixel format\n");
       return -1;
  }

  xstride_by_mcu = ystride_by_mcu = 8;
  if ((priv->component_infos[cY].Hfactor | priv->component_infos[cY].Vfactor) == 1) {
     decode_MCU = decode_mcu_table[0];
     convert_to_pixfmt = colorspace_array_conv[0];
     trace("Use decode 1x1 sampling\n");
  } else if (priv->component_infos[cY].Hfactor == 1) {
     decode_MCU = decode_mcu_table[1];
     convert_to_pixfmt = colorspace_array_conv[1];
     ystride_by_mcu = 16;
     trace("Use decode 1x2 sampling (not supported)\n");
  } else if (priv->component_infos[cY].Vfactor == 2) {
     decode_MCU = decode_mcu_table[3];
     convert_to_pixfmt = colorspace_array_conv[3];
     xstride_by_mcu = 16;
     ystride_by_mcu = 16;
     trace("Use decode 2x2 sampling\n");
  } else {
     decode_MCU = decode_mcu_table[2];
     convert_to_pixfmt = colorspace_array_conv[2];
     xstride_by_mcu = 16;
     trace("Use decode 2x1 sampling\n");
  }

  resync(priv);

  /* Don't forget to that block can be either 8 or 16 lines */
  bytes_per_blocklines[0] *= ystride_by_mcu;
  bytes_per_blocklines[1] *= ystride_by_mcu;
  bytes_per_blocklines[2] *= ystride_by_mcu;

  bytes_per_mcu[0] *= xstride_by_mcu/8;
  bytes_per_mcu[1] *= xstride_by_mcu/8;
  bytes_per_mcu[2] *= xstride_by_mcu/8;

  /* Just the decode the image by macroblock (size is 8x8, 8x16, or 16x16) */
  for (y=0; y < priv->height/ystride_by_mcu; y++)
   {
     //trace("Decoding row %d\n", y);
     priv->plane[0] = priv->components[0] + (y * bytes_per_blocklines[0]);
     priv->plane[1] = priv->components[1] + (y * bytes_per_blocklines[1]);
     priv->plane[2] = priv->components[2] + (y * bytes_per_blocklines[2]);
     for (x=0; x < priv->width; x+=xstride_by_mcu)
      {
	decode_MCU(priv);
	convert_to_pixfmt(priv);
	priv->plane[0] += bytes_per_mcu[0];
	priv->plane[1] += bytes_per_mcu[1];
	priv->plane[2] += bytes_per_mcu[2];
	if (priv->restarts_to_go>0)
	 {
	   priv->restarts_to_go--;
	   if (priv->restarts_to_go == 0)
	    {
	      priv->stream -= (priv->nbits_in_reservoir/8);
	      resync(priv);
	      if (find_next_rst_marker(priv) < 0)
		return -1;
	    }
	 }
      }
   }

  trace("Input file size: %d\n", priv->stream_length+2);
  trace("Input bytes actually read: %d\n", priv->stream - priv->stream_begin + 2);

  return 0;
}

void tinyjpeg_get_components(struct jdec_private* jdec,  unsigned char** components){

}

void tinyjpeg_free(struct jdec_private* jdec){

}
/*******************************************************************************
 *
 * JPEG/JFIF Parsing functions
 *
 * Note: only a small subset of the jpeg file format is supported. No markers,
 * nor progressive stream is supported.
 *
 ******************************************************************************/

static void build_quantization_table(float *qtable, const unsigned char *ref_table)
{
  /* Taken from libjpeg. Copyright Independent JPEG Group's LLM idct.
   * For float AA&N IDCT method, divisors are equal to quantization
   * coefficients scaled by scalefactor[row]*scalefactor[col], where
   *   scalefactor[0] = 1
   *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
   * We apply a further scale factor of 8.
   * What's actually stored is 1/divisor so that the inner loop can
   * use a multiplication rather than a division.
   */
  int i, j;
  static const double aanscalefactor[8] = {
     1.0, 1.387039845, 1.306562965, 1.175875602,
     1.0, 0.785694958, 0.541196100, 0.275899379
  };
  const unsigned char *zz = zigzag;

  for (i=0; i<8; i++) {
     for (j=0; j<8; j++) {
       *qtable++ = ref_table[*zz++] * aanscalefactor[i] * aanscalefactor[j];
     }
   }

}

static void print_SOF(const unsigned char *stream)
{
  int width, height, nr_components, precision;
#if DEBUG
  const char *nr_components_to_string[] = {
     "????",
     "Grayscale",
     "????",
     "YCbCr",
     "CYMK"
  };
#endif

  precision = stream[2];
  height = be16_to_cpu(stream+3);
  width  = be16_to_cpu(stream+5);
  nr_components = stream[7];

  trace("> SOF marker\n");
  trace("Size:%dx%d nr_components:%d (%s)  precision:%d\n", 
      width, height,
      nr_components, nr_components_to_string[nr_components],
      precision);
}
static int parse_SOF(struct jdec_private *priv, const unsigned char *stream)
{
  int i, width, height, nr_components, cid, sampling_factor;
  int Q_table;
  struct component *c;

  trace("> SOF marker\n");
  print_SOF(stream);

  height = be16_to_cpu(stream+3);
  width  = be16_to_cpu(stream+5);
  nr_components = stream[7];
#if SANITY_CHECK
  if (stream[2] != 8)
    error("Precision other than 8 is not supported\n");
  if (width>JPEG_MAX_WIDTH || height>JPEG_MAX_HEIGHT)
    error("Width and Height (%dx%d) seems suspicious\n", width, height);
  if (nr_components != 3)
    error("We only support YUV images\n");
  if (height%16)
    error("Height need to be a multiple of 16 (current height is %d)\n", height);
  if (width%16)
    error("Width need to be a multiple of 16 (current Width is %d)\n", width);
#endif
  stream += 8;
  for (i=0; i<nr_components; i++) {
     cid = *stream++;
     sampling_factor = *stream++;
     Q_table = *stream++;
     c = &priv->component_infos[i];
#if SANITY_CHECK
     c->cid = cid;
     if (Q_table >= COMPONENTS)
       error("Bad Quantization table index (got %d, max allowed %d)\n", Q_table, COMPONENTS-1);
#endif
     c->Vfactor = sampling_factor&0xf;
     c->Hfactor = sampling_factor>>4;
     c->Q_table = priv->Q_tables[Q_table];
     trace("Component:%d  factor:%dx%d  Quantization table:%d\n",
           cid, c->Hfactor, c->Hfactor, Q_table );

  }
  priv->width = width;
  priv->height = height;

  trace("< SOF marker\n");

  return 0;
}
static int parse_DQT(struct jdec_private *priv, const unsigned char *stream)
{
  int qi;
  float *table;
  const unsigned char *dqt_block_end;

  trace("> DQT marker\n");
  dqt_block_end = stream + be16_to_cpu(stream);
  stream += 2;	/* Skip length */

  while (stream < dqt_block_end)
   {
     qi = *stream++;
#if SANITY_CHECK
     if (qi>>4)
       error("16 bits quantization table is not supported\n");
     if (qi>4)
       error("No more 4 quantization table is supported (got %d)\n", qi);
#endif
     table = priv->Q_tables[qi];
     build_quantization_table(table, stream);
     stream += 64;
   }
  trace("< DQT marker\n");
  return 0;
}

static int parse_DHT(struct jdec_private *priv, const unsigned char *stream)
{
  unsigned int count, i;
  unsigned char huff_bits[17];
  int length, index;

  length = be16_to_cpu(stream) - 2;
  stream += 2;	/* Skip length */

  trace("> DHT marker (length=%d)\n", length);

  while (length>0) {
     index = *stream++;

     /* We need to calculate the number of bytes 'vals' will takes */
     huff_bits[0] = 0;
     count = 0;
     for (i=1; i<17; i++) {
	huff_bits[i] = *stream++;
	count += huff_bits[i];
     }
#if SANITY_CHECK
     if (count >= HUFFMAN_BITS_SIZE)
       error("No more than %d bytes is allowed to describe a huffman table", HUFFMAN_BITS_SIZE);
     if ( (index &0xf) >= HUFFMAN_TABLES)
       error("No more than %d Huffman tables is supported (got %d)\n", HUFFMAN_TABLES, index&0xf);
     trace("Huffman table %s[%d] length=%d\n", (index&0xf0)?"AC":"DC", index&0xf, count);
#endif

     if (index & 0xf0 )
       build_huffman_table(huff_bits, stream, &priv->HTAC[index&0xf]);
     else
       build_huffman_table(huff_bits, stream, &priv->HTDC[index&0xf]);

     length -= 1;
     length -= 16;
     length -= count;
     stream += count;
  }
  trace("< DHT marker\n");
  return 0;
}
static int parse_SOS(struct jdec_private *priv, const unsigned char *stream)
{
  unsigned int i, cid, table;
  unsigned int nr_components = stream[2];

  trace("> SOS marker\n");

#if SANITY_CHECK
  if (nr_components != 3)
    error("We only support YCbCr image\n");
#endif

  stream += 3;
  for (i=0;i<nr_components;i++) {
     cid = *stream++;
     table = *stream++;
#if SANITY_CHECK
     if ((table&0xf)>=4)
	error("We do not support more than 2 AC Huffman table\n");
     if ((table>>4)>=4)
	error("We do not support more than 2 DC Huffman table\n");
     if (cid != priv->component_infos[i].cid)
        error("SOS cid order (%d:%d) isn't compatible with the SOF marker (%d:%d)\n",
	      i, cid, i, priv->component_infos[i].cid);
     trace("ComponentId:%d  tableAC:%d tableDC:%d\n", cid, table&0xf, table>>4);
#endif
     priv->component_infos[i].AC_table = &priv->HTAC[table&0xf];
     priv->component_infos[i].DC_table = &priv->HTDC[table>>4];
  }
  priv->stream = stream+3;
  trace("< SOS marker\n");
  return 0;
}
int parse_JFIF(struct jdec_private  *priv, const unsigned char *stream){
    int chuck_len;
    int marker;
    int sos_marker_found = 0;
    int dht_marker_found = 0;
    const unsigned char *next_chunck;
    char* param_trace;

    trace("parsing_JFIF……\n");

    while(!sos_marker_found){
        if(*stream++ !=0xff){
            goto bogus_jpeg_format;
        }
        while(*stream ==0xff){
            stream++;
        }
        marker = *stream++;

        chuck_len = be16_to_cpu(stream);
       next_chunck = stream +chuck_len;

       trace("marker:%x chuck_len:%d\n", marker, chuck_len);
       switch(marker){
           case SOF:
               if(parse_SOF(priv, stream)<0){
                   trace("%s", "parse_JFIF……");
                   return -1;
               }
            break;
           case DQT:
            if (parse_DQT(priv, stream) < 0)
            return -1;
            break;
           case SOS:
            if (parse_SOS(priv, stream) < 0)
                return -1;
            sos_marker_found = 1;
            break;
           case DHT:
            if (parse_DHT(priv, stream) < 0)
                return -1;
            dht_marker_found = 1;
            break;
           default:
            trace("> Unknown marker %2.2x\n", marker);
            break;
       }; 

       //解下一个segment
       stream  = next_chunck;
    }

    if(!dht_marker_found){
#if TRACE_PARAM
        fprintf(param_trace, "No Huffman table loaded, using the default one\n");
        fflush(param_trace);
#endif
        build_default_huffman_tables(priv);

        return 0;

bogus_jpeg_format:
        //fprintf(param_trace, "Bogus jpeg format\n");
       // fflush(param_trace);
        trace("bogus_jpeg_format%s\n", "t");
        return -1;
    }

}

int tinyjpeg_parse_header(struct jdec_private *priv, const unsigned char *buf, unsigned int size){
    int ret;

    //是否是JPEG文件
    /* Identify the file */
    if ((buf[0] != 0xFF) || (buf[1] != SOI))
        error("Not a JPG file ?\n");

    priv->stream_begin = buf+2;
    priv->stream_length = size -2;
    priv->stream_end = priv->stream_begin + priv->stream_length; //定位到文件后
    trace("> parsing header:%x%x\n", buf[0],buf[1]); 
    ret = parse_JFIF(priv, priv->stream_begin);

    trace("< parse header\n"); 
    return ret;
}





// convert image

int convert_one_image(const char *infilename, const char *outfilename){
    FILE *fp;
    unsigned int length_of_file;
    unsigned int width, height;
    unsigned char *buf;
    struct jdec_private *jdec;
    unsigned char *components[3];

    char* error_string;
    int  output_format;

   //load the Jpeg to memory
   fp = fopen(infilename, "rb");
   if(fp == NULL){
       exitmessage("Cannot open filename\n");
   }

   fseek(fp,0,SEEK_END);
   length_of_file = ftell(fp); //得到文件大小
   buf = (unsigned char*)malloc(length_of_file +4);
   if(buf ==NULL){
       exitmessage("Not enough memory for loading file\n");
   }

   fseek(fp,0,SEEK_SET);
   fread(buf,sizeof(unsigned char), length_of_file, fp); //将文件里的jpg数据读到buf中
   fclose(fp);

   //解压
   jdec = tinyjpeg_init(); 
   if(jdec == NULL){
       exitmessage("Not enough meory to alloc the structure need for decompressing\n");
   }

   trace("parse_header:%d buf:%d\n",length_of_file, sizeof(*buf));
   if(tinyjpeg_parse_header(jdec, buf, length_of_file) <0){
       exitmessage(tinyjpeg_get_errorstring(jdec));
   }

   //Get the size of the image
   trace("> tinyjpeg_get_size\n");
   tinyjpeg_get_size(jdec, &width, &height);
   trace("< tinyjpeg_get_size\n");

   trace("> tinyjpeg_decode:Decoding JPEG image...\n");
   if(tinyjpeg_decode(jdec, output_format)<0){
       exitmessage(tinyjpeg_get_errorstring(jdec));
   }
   trace("< tinyjpeg_decode\n");

   trace("> tinyjpeg_get_components\n");
   tinyjpeg_get_components(jdec, components);
   trace("< tinyjpeg_get_components\n");


   //------------
   tinyjpeg_free(jdec);
   free(buf);
   return 0;
}



void main(){
    char* infilename="/home/chen/sandbox/image/test.jpg";
    char* outfilename="/home/chen/sandbox/image/test_reverse.jpg";

    convert_one_image(infilename, outfilename);

    printf("\n%s", "ending...");
}

