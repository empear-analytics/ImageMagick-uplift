/*
  xannotate_harness.c: run the per-pixel transfer of MagickCore/xwindow-annotate.c without
  an X server, for every stencil of the annotate and of the draw path.

  The file under test is included whole (-DXANNOTATE_SOURCE='"path/xwindow-annotate.c"'),
  so its static functions XTransferAnnotatePixels and XTransferDrawPixels are called
  directly, as XAnnotateImage and XComposeDrawImage call them, on a hand-made XImage (the
  text or the element the X server would have rendered, as random zero and non-zero
  pixels; XInitImage sets its access functions, so no display is opened) and an image
  prepared as those callers prepare theirs (and, as annotate-alpha, an annotate image
  with an alpha channel for every stencil, so that the alpha the transfer writes for the
  other stencils is compared too; XAnnotateImage adds the channel only for the
  foreground stencil). The transfer writes the pen, box or
  background colour and an alpha value into every pixel; the harness hashes every channel
  of every pixel afterwards.

    xannotate_harness check            one line per configuration and image size: the
                                       name, the row count the transfer returned and the
                                       FNV-1a hash of the image's pixels
    xannotate_harness list             the configuration names
    xannotate_harness time NAME REPS   transfer configuration NAME on a 1600x1200 image
                                       REPS times; prints the seconds the transfers took

  tools/xannotate_compare.sh builds it for two revisions and compares or times them.
*/
#include XANNOTATE_SOURCE
#include "MagickCore/channel.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

typedef enum
{
  AnnotatePath,
  DrawPath
} HarnessPath;

typedef struct _HarnessFormat
{
  int
    depth,
    bits_per_pixel;
} HarnessFormat;

typedef struct _HarnessConfig
{
  char
    name[64];

  HarnessPath
    path;

  AnnotationStencil
    stencil;

  int
    alpha;

  const HarnessFormat
    *format;
} HarnessConfig;

/* What XGetImage returns for a ZPixmap of the default visual: 24, 16 and 8 bit. */
static const HarnessFormat
  harness_formats[] =
  {
    { 24, 32 },
    { 16, 16 },
    { 8, 8 }
  };

static const char
  *harness_paths[] = { "annotate", "annotate-alpha", "draw" },
  *harness_stencils[] = { "fg", "bg", "opaque", "transparent" };

static unsigned long
  harness_seed = 12345UL;

static XPixelInfo
  harness_pixel;

static unsigned long HarnessRandom(void)
{
  harness_seed=harness_seed*1103515245UL+12345UL;
  return((harness_seed >> 8) & 0xffffffUL);
}

static XImage *HarnessXImage(const HarnessFormat *format,const size_t columns,
  const size_t rows)
{
  int
    x,
    y;

  unsigned long
    on;

  XImage
    *ximage;

  ximage=(XImage *) calloc(1,sizeof(*ximage));
  ximage->width=(int) columns;
  ximage->height=(int) rows;
  ximage->format=ZPixmap;
  ximage->byte_order=LSBFirst;
  ximage->bitmap_unit=32;
  ximage->bitmap_bit_order=LSBFirst;
  ximage->bitmap_pad=32;
  ximage->depth=format->depth;
  ximage->bits_per_pixel=format->bits_per_pixel;
  ximage->bytes_per_line=(int) (((columns*(size_t) format->bits_per_pixel+31)/32)*4);
  ximage->red_mask=0;
  ximage->green_mask=0;
  ximage->blue_mask=0;
  ximage->data=(char *) calloc((size_t) ximage->bytes_per_line*rows,1);
  (void) XInitImage(ximage);
  on=(1UL << format->depth)-1UL;
  for (y=0; y < (int) rows; y++)
    for (x=0; x < (int) columns; x++)
      (void) XPutPixel(ximage,x,y,(HarnessRandom() % 3) == 0 ? on : 0UL);
  return(ximage);
}

static void HarnessPixelInfo(XPixelInfo *pixel)
{
  pixel->box_color.red=1000;
  pixel->box_color.green=22000;
  pixel->box_color.blue=43000;
  pixel->pen_color.red=61000;
  pixel->pen_color.green=37000;
  pixel->pen_color.blue=5000;
}

/* The image XAnnotateImage or XComposeDrawImage hands to the transfer. */
static Image *HarnessImage(const HarnessConfig *config,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
  Image
    *image;

  image=AcquireImage((ImageInfo *) NULL,exception);
  image->columns=columns;
  image->rows=rows;
  image->background_color.red=9000.0;
  image->background_color.green=31000.0;
  image->background_color.blue=52000.0;
  image->background_color.alpha=47000.0;
  if (config->path == AnnotatePath)
    {
      if ((config->stencil == ForegroundStencil) || (config->alpha != 0))
        image->alpha_trait=BlendPixelTrait;
    }
  else
    {
      (void) SetImageStorageClass(image,DirectClass,exception);
      image->alpha_trait=BlendPixelTrait;
    }
  return(image);
}

static unsigned long long HarnessHash(const Image *image,ExceptionInfo *exception)
{
  const Quantum
    *p;

  size_t
    i,
    n;

  ssize_t
    y;

  unsigned long long
    h = 1469598103934665603ULL;

  for (y=0; y < (ssize_t) image->rows; y++)
  {
    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    n=image->columns*GetPixelChannels(image)*sizeof(*p);
    for (i=0; i < n; i++)
    {
      h^=((const unsigned char *) p)[i];
      h*=1099511628211ULL;
    }
  }
  return(h^(unsigned long long) GetPixelChannels(image));
}

static int HarnessTransfer(const HarnessConfig *config,Image *image,
  XImage *ximage,ExceptionInfo *exception)
{
  const XPixelInfo
    *pixel = (&harness_pixel);

  if (config->path == AnnotatePath)
    {
      AnnotatePixels
        annotate_pixels;

      XAnnotateInfo
        annotate_info;

      (void) memset(&annotate_info,0,sizeof(annotate_info));
      annotate_info.stencil=config->stencil;
      annotate_pixels.annotate_image=image;
      annotate_pixels.annotate_ximage=ximage;
      annotate_pixels.pixel=pixel;
      annotate_pixels.annotate_info=(&annotate_info);
      annotate_pixels.exception=exception;
      return(XTransferAnnotatePixels(&annotate_pixels));
    }
  else
    {
      DrawPixels
        draw_pixels;

      XDrawInfo
        draw_info;

      (void) memset(&draw_info,0,sizeof(draw_info));
      draw_info.stencil=config->stencil;
      draw_pixels.draw_image=image;
      draw_pixels.draw_ximage=ximage;
      draw_pixels.pixel=pixel;
      draw_pixels.draw_info=(&draw_info);
      draw_pixels.exception=exception;
      return(XTransferDrawPixels(&draw_pixels));
    }
}

static double HarnessRun(const HarnessConfig *config,const size_t columns,
  const size_t rows,const int reps)
{
  double
    seconds;

  ExceptionInfo
    *exception;

  Image
    *image;

  int
    i,
    y;

  struct timespec
    start,
    stop;

  unsigned long long
    hash;

  XImage
    *ximage;

  exception=AcquireExceptionInfo();
  seconds=0.0;
  hash=0;
  y=0;
  for (i=0; i < reps; i++)
  {
    harness_seed=12345UL+(unsigned long) config->format->depth*7UL+
      (unsigned long) config->stencil;
    ximage=HarnessXImage(config->format,columns,rows);
    image=HarnessImage(config,columns,rows,exception);
    clock_gettime(CLOCK_MONOTONIC,&start);
    y=HarnessTransfer(config,image,ximage,exception);
    clock_gettime(CLOCK_MONOTONIC,&stop);
    seconds+=(double) (stop.tv_sec-start.tv_sec)+1.0e-9*(double)
      (stop.tv_nsec-start.tv_nsec);
    hash=HarnessHash(image,exception);
    image=DestroyImage(image);
  }
  (void) printf("%-28s %4dx%-4d rows %4d %016llx\n",config->name,(int) columns,
    (int) rows,y,hash);
  exception=DestroyExceptionInfo(exception);
  return(seconds);
}

static int HarnessConfigs(HarnessConfig *configs)
{
  int
    f,
    n,
    path,
    stencil;

  n=0;
  for (path=0; path < 3; path++)
    for (stencil=0; stencil < 4; stencil++)
      for (f=0; f < (int) (sizeof(harness_formats)/sizeof(*harness_formats)); f++)
      {
        HarnessConfig
          *c = configs+n;

        (void) memset(c,0,sizeof(*c));
        c->path=(path == 2) ? DrawPath : AnnotatePath;
        c->alpha=(path == 1) ? 1 : 0;
        c->stencil=(AnnotationStencil) stencil;
        c->format=harness_formats+f;
        (void) snprintf(c->name,sizeof(c->name),"%s-%s-z%d",harness_paths[path],
          harness_stencils[stencil],c->format->depth);
        n++;
      }
  return(n);
}

static int HarnessTime(const HarnessConfig *configs,const int n,
  const char *name,const int reps)
{
  int
    i;

  for (i=0; i < n; i++)
    if (strcmp(configs[i].name,name) == 0)
      {
        (void) printf("%.6f\n",HarnessRun(configs+i,1600,1200,reps));
        return(0);
      }
  (void) fprintf(stderr,"no configuration %s\n",name);
  return(2);
}

static void HarnessCheck(const HarnessConfig *configs,const int n)
{
  int
    i;

  for (i=0; i < n; i++)
  {
    (void) HarnessRun(configs+i,37,23,1);
    (void) HarnessRun(configs+i,203,9,2);
  }
}

static int HarnessCommand(const HarnessConfig *configs,const int n,
  const int argc,char **argv)
{
  int
    i;

  if (strcmp(argv[1],"list") == 0)
    {
      for (i=0; i < n; i++)
        (void) printf("%s\n",configs[i].name);
      return(0);
    }
  if (strcmp(argv[1],"check") == 0)
    {
      HarnessCheck(configs,n);
      return(0);
    }
  if ((strcmp(argv[1],"time") == 0) && (argc > 3))
    return(HarnessTime(configs,n,argv[2],atoi(argv[3])));
  return(2);
}

int main(int argc,char **argv)
{
  HarnessConfig
    configs[64];

  int
    n,
    status;

  if (argc < 2)
    {
      (void) fprintf(stderr,"usage: %s check|list|time NAME REPS\n",argv[0]);
      return(2);
    }
  MagickCoreGenesis(argv[0],MagickFalse);
  HarnessPixelInfo(&harness_pixel);
  n=HarnessConfigs(configs);
  status=HarnessCommand(configs,n,argc,argv);
  MagickCoreTerminus();
  return(status);
}
