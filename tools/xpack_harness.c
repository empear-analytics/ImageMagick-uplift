/*
  xpack_harness.c: run the X11 image packers of MagickCore/xwindow-image.c without an X
  server, for the LSB-first and the MSB-first byte orders alike.

  The file under test is included whole (-DXWINDOW_SOURCE='"path/xwindow-image.c"'), so
  its static functions XMakeImageLSBFirst and XMakeImageMSBFirst are called directly on a
  hand-made XImage (ZPixmap or XYBitmap, 1 to 32 bits per pixel) and a hand-made window,
  standard colormap and pixel table. Nothing talks to a display. No case of the oracle
  reaches the MSB-first packers (the X server the oracle uses is LSB-first), so this is how
  both byte orders are compared between two revisions of the file and timed.

    xpack_harness check            one line per configuration and image size: the name
                                   and the FNV-1a hashes of the packed bytes and of the
                                   matte bytes (each buffer with 64 guard bytes)
    xpack_harness list             the configuration names
    xpack_harness time NAME REPS   pack configuration NAME on a 1600x1200 image REPS
                                   times; prints the seconds the packing took

  tools/xpack_compare.sh builds it for two revisions and compares or times them.
*/
#include XWINDOW_SOURCE
#include "MagickCore/channel.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

typedef enum
{
  BitmapKind,
  ColormappedKind,
  ContinuousKind
} HarnessKind;

typedef struct _HarnessMap
{
  int
    bits_per_pixel;

  unsigned long
    base_pixel,
    red_max,
    green_max,
    blue_max,
    red_mult,
    green_mult,
    blue_mult;
} HarnessMap;

typedef struct _HarnessConfig
{
  char
    name[64];

  int
    order,
    bits_per_pixel,
    gamma,
    immutable;

  HarnessKind
    kind;

  const HarnessMap
    *map;
} HarnessConfig;

typedef struct _HarnessRun
{
  ExceptionInfo
    *exception;

  Image
    *image;

  MakeImageContext
    context;

  QuantizeInfo
    *quantize_info;

  XImage
    matte,
    ximage;

  XPixelInfo
    pixel_info;

  XResourceInfo
    resource_info;

  XStandardColormap
    map;

  XWindowInfo
    window;

  size_t
    data_length,
    matte_length;
} HarnessRun;

/* Standard colormaps: a generic one per depth, then red-high and blue-high 32 bit. */
static const HarnessMap
  harness_maps[] =
  {
    { 1, 0, 1, 1, 0, 2, 1, 1 },
    { 2, 0, 1, 1, 0, 2, 1, 1 },
    { 4, 0, 1, 3, 1, 8, 2, 1 },
    { 8, 3, 7, 7, 3, 32, 4, 1 },
    { 16, 1, 31, 63, 31, 2048, 32, 1 },
    { 24, 1, 255, 255, 255, 1, 256, 65536L },
    { 32, 1, 255, 255, 255, 1, 256, 65536L },
    { 32, 0, 255, 255, 255, 65536L, 256, 1 },
    { 32, 0, 255, 255, 255, 1, 256, 65536L }
  };

static unsigned long
  harness_seed = 12345UL;

static unsigned long HarnessRandom(void)
{
  harness_seed=harness_seed*1103515245UL+12345UL;
  return((harness_seed >> 8) & 0xffffffUL);
}

static void HarnessColormap(Image *image,const size_t colors,
  ExceptionInfo *exception)
{
  ssize_t
    i;

  (void) AcquireImageColormap(image,colors,exception);
  for (i=0; i < (ssize_t) colors; i++)
  {
    image->colormap[i].red=(double) (HarnessRandom() % 65536);
    image->colormap[i].green=(double) (HarnessRandom() % 65536);
    image->colormap[i].blue=(double) (HarnessRandom() % 65536);
  }
}

static void HarnessPixel(Image *image,const size_t colors,Quantum *q)
{
  ssize_t
    i;

  if (colors == 0)
    {
      SetPixelRed(image,(Quantum) (HarnessRandom() % 65536),q);
      SetPixelGreen(image,(Quantum) (HarnessRandom() % 65536),q);
      SetPixelBlue(image,(Quantum) (HarnessRandom() % 65536),q);
    }
  else
    {
      i=(ssize_t) (HarnessRandom() % colors);
      SetPixelIndex(image,(Quantum) i,q);
      SetPixelRed(image,ClampToQuantum(image->colormap[i].red),q);
      SetPixelGreen(image,ClampToQuantum(image->colormap[i].green),q);
      SetPixelBlue(image,ClampToQuantum(image->colormap[i].blue),q);
    }
  SetPixelAlpha(image,(Quantum) (HarnessRandom() % 65536),q);
}

static Image *HarnessImage(const HarnessKind kind,const size_t columns,
  const size_t rows,ExceptionInfo *exception)
{
  Image
    *image;

  Quantum
    *q;

  ssize_t
    x,
    y;

  size_t
    colors;

  image=AcquireImage((ImageInfo *) NULL,exception);
  (void) SetImageExtent(image,columns,rows,exception);
  (void) SetImageAlphaChannel(image,ActivateAlphaChannel,exception);
  colors=(kind == ContinuousKind) ? 0 : ((kind == BitmapKind) ? 2 : 16);
  if (colors != 0)
    HarnessColormap(image,colors,exception);
  for (y=0; y < (ssize_t) rows; y++)
  {
    q=GetAuthenticPixels(image,0,y,columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) columns; x++)
    {
      HarnessPixel(image,colors,q);
      q+=GetPixelChannels(image);
    }
    (void) SyncAuthenticPixels(image,exception);
  }
  return(image);
}

static void HarnessXImage(XImage *ximage,const int format,const int order,
  const int bits_per_pixel,const size_t columns,const size_t rows)
{
  (void) memset(ximage,0,sizeof(*ximage));
  ximage->width=(int) columns;
  ximage->height=(int) rows;
  ximage->format=format;
  ximage->byte_order=order;
  ximage->bitmap_bit_order=order;
  ximage->bits_per_pixel=bits_per_pixel;
  ximage->bytes_per_line=(int) (((columns*(size_t) bits_per_pixel+31)/32)*4);
  ximage->data=(char *) calloc((size_t) ximage->bytes_per_line*rows+64,1);
}

static void HarnessWindow(const HarnessConfig *config,HarnessRun *run)
{
  static unsigned long
    pixels[256];

  int
    i;

  for (i=0; i < 256; i++)
    pixels[i]=HarnessRandom();
  (void) memset(&run->map,0,sizeof(run->map));
  run->map.base_pixel=config->map->base_pixel;
  run->map.red_max=config->map->red_max;
  run->map.green_max=config->map->green_max;
  run->map.blue_max=config->map->blue_max;
  run->map.red_mult=config->map->red_mult;
  run->map.green_mult=config->map->green_mult;
  run->map.blue_mult=config->map->blue_mult;
  (void) memset(&run->pixel_info,0,sizeof(run->pixel_info));
  run->pixel_info.pixels=pixels;
  run->pixel_info.colors=(config->kind == ColormappedKind) ? 16 : 0;
  run->pixel_info.foreground_color.red=60000;
  run->pixel_info.foreground_color.green=50000;
  run->pixel_info.foreground_color.blue=40000;
  run->pixel_info.background_color.red=1000;
  run->pixel_info.background_color.green=2000;
  run->pixel_info.background_color.blue=3000;
  (void) memset(&run->window,0,sizeof(run->window));
  run->window.immutable=config->immutable ? MagickTrue : MagickFalse;
  run->window.map_info=(&run->map);
  run->window.pixel_info=(&run->pixel_info);
}

static void HarnessSetup(const HarnessConfig *config,const size_t columns,
  const size_t rows,HarnessRun *run)
{
  run->exception=AcquireExceptionInfo();
  harness_seed=12345UL+(unsigned long) config->bits_per_pixel;
  HarnessWindow(config,run);
  run->image=HarnessImage(config->kind,columns,rows,run->exception);
  run->quantize_info=AcquireQuantizeInfo((ImageInfo *) NULL);
  run->quantize_info->dither_method=NoDitherMethod;
  (void) memset(&run->resource_info,0,sizeof(run->resource_info));
  run->resource_info.image_info=AcquireImageInfo();
  run->resource_info.quantize_info=run->quantize_info;
  run->resource_info.color_recovery=MagickFalse;
  run->context.resource_info=(&run->resource_info);
  run->context.window=(&run->window);
  run->context.exception=run->exception;
  red_gamma=config->gamma ? 2.2 : 1.0;
  green_gamma=config->gamma ? 1.7 : 1.0;
  blue_gamma=config->gamma ? 0.8 : 1.0;
  HarnessXImage(&run->ximage,(config->kind == BitmapKind) ? XYBitmap : ZPixmap,
    config->order,config->bits_per_pixel,columns,rows);
  HarnessXImage(&run->matte,XYBitmap,config->order,1,columns,rows);
  run->data_length=(size_t) run->ximage.bytes_per_line*rows+64;
  run->matte_length=(size_t) run->matte.bytes_per_line*rows+64;
}

static void HarnessTeardown(HarnessRun *run)
{
  free(run->ximage.data);
  free(run->matte.data);
  run->resource_info.image_info=DestroyImageInfo(run->resource_info.image_info);
  run->quantize_info=DestroyQuantizeInfo(run->quantize_info);
  run->image=DestroyImage(run->image);
  run->exception=DestroyExceptionInfo(run->exception);
}

static unsigned long long HarnessHash(const char *p,const size_t n)
{
  size_t
    i;

  unsigned long long
    h = 1469598103934665603ULL;

  for (i=0; i < n; i++)
  {
    h^=(unsigned char) p[i];
    h*=1099511628211ULL;
  }
  return(h);
}

static double HarnessPack(const HarnessConfig *config,const size_t columns,
  const size_t rows,const int reps)
{
  HarnessRun
    run;

  int
    i;

  struct timespec
    start,
    stop;

  HarnessSetup(config,columns,rows,&run);
  clock_gettime(CLOCK_MONOTONIC,&start);
  for (i=0; i < reps; i++)
  {
    if (config->order == LSBFirst)
      XMakeImageLSBFirst(&run.context,run.image,&run.ximage,&run.matte);
    else
      XMakeImageMSBFirst(&run.context,run.image,&run.ximage,&run.matte);
  }
  clock_gettime(CLOCK_MONOTONIC,&stop);
  (void) printf("%-28s %4dx%-4d %016llx %016llx\n",config->name,(int) columns,
    (int) rows,HarnessHash(run.ximage.data,run.data_length),
    HarnessHash(run.matte.data,run.matte_length));
  HarnessTeardown(&run);
  return((double) (stop.tv_sec-start.tv_sec)+1.0e-9*(double)
    (stop.tv_nsec-start.tv_nsec));
}

static int HarnessConfigs(HarnessConfig *configs)
{
  static const char
    *kinds[] = { "bitmap", "colormapped", "continuous" };

  int
    m,
    n,
    variant;

  n=0;
  for (variant=0; variant < 8; variant++)
    for (m=0; m < (int) (sizeof(harness_maps)/sizeof(*harness_maps)); m++)
    {
      HarnessConfig
        *c = configs+n;

      (void) memset(c,0,sizeof(*c));
      c->order=(variant & 4) ? MSBFirst : LSBFirst;
      c->gamma=(variant & 2) ? 1 : 0;
      c->immutable=(variant & 1) ? 1 : 0;
      c->map=harness_maps+m;
      c->bits_per_pixel=c->map->bits_per_pixel;
      c->kind=(m == 0) ? BitmapKind : ColormappedKind;
      n++;
      if (m > 0)
        {
          configs[n]=(*c);
          configs[n].kind=ContinuousKind;
          n++;
        }
    }
  for (m=0; m < n; m++)
    (void) snprintf(configs[m].name,sizeof(configs[m].name),"%s-%s%d-m%d-g%d-i%d",
      configs[m].order == LSBFirst ? "lsb" : "msb",kinds[configs[m].kind],
      configs[m].bits_per_pixel,(int) (configs[m].map-harness_maps),
      configs[m].gamma,configs[m].immutable);
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
        (void) printf("%.6f\n",HarnessPack(configs+i,1600,1200,reps));
        return(0);
      }
  (void) fprintf(stderr,"no configuration %s\n",name);
  return(2);
}

int main(int argc,char **argv)
{
  HarnessConfig
    configs[256];

  int
    i,
    n,
    status;

  if (argc < 2)
    {
      (void) fprintf(stderr,"usage: %s check|list|time NAME REPS\n",argv[0]);
      return(2);
    }
  MagickCoreGenesis(argv[0],MagickFalse);
  n=HarnessConfigs(configs);
  status=0;
  if (strcmp(argv[1],"list") == 0)
    for (i=0; i < n; i++)
      (void) printf("%s\n",configs[i].name);
  else if (strcmp(argv[1],"check") == 0)
    for (i=0; i < n; i++)
    {
      (void) HarnessPack(configs+i,37,23,1);
      (void) HarnessPack(configs+i,203,9,2);
    }
  else if ((strcmp(argv[1],"time") == 0) && (argc > 3))
    status=HarnessTime(configs,n,argv[2],atoi(argv[3]));
  else
    status=2;
  MagickCoreTerminus();
  return(status);
}
