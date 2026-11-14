/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               DDDD   IIIII  SSSSS  PPPP   L       AAA   Y   Y               %
%               D   D    I    SS     P   P  L      A   A   Y Y                %
%               D   D    I     SSS   PPPP   L      AAAAA    Y                 %
%               D   D    I       SS  P      L      A   A    Y                 %
%               DDDD   IIIII  SSSSS  P      LLLLL  A   A    Y                 %
%                                                                             %
%                      IIIII  M   M   AAA    GGGG  EEEEE                      %
%                        I    MM MM  A   A  G      E                          %
%                        I    M M M  AAAAA  G  GG  EEE                        %
%                        I    M   M  A   A  G   G  E                          %
%                      IIIII  M   M  A   A   GGG   EEEEE                      %
%                                                                             %
%                                                                             %
%                  MagickCore X Window Image Display Methods                  %
%                                                                             %
%                             Software Design                                 %
%                                  Cristy                                     %
%                                July 1992                                    %
%                                                                             %
%                                                                             %
%  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization         %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/license/                                         %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "MagickCore/studio.h"
#include "MagickCore/artifact.h"
#include "MagickCore/attribute.h"
#include "MagickCore/blob.h"
#include "MagickCore/cache.h"
#include "MagickCore/cache-private.h"
#include "MagickCore/channel.h"
#include "MagickCore/client.h"
#include "MagickCore/color.h"
#include "MagickCore/colorspace.h"
#include "MagickCore/composite.h"
#include "MagickCore/constitute.h"
#include "MagickCore/decorate.h"
#include "MagickCore/delegate.h"
#include "MagickCore/display.h"
#include "MagickCore/display-private.h"
#include "MagickCore/distort.h"
#include "MagickCore/draw.h"
#include "MagickCore/effect.h"
#include "MagickCore/enhance.h"
#include "MagickCore/exception.h"
#include "MagickCore/exception-private.h"
#include "MagickCore/fx.h"
#include "MagickCore/geometry.h"
#include "MagickCore/image.h"
#include "MagickCore/image-private.h"
#include "MagickCore/list.h"
#include "MagickCore/locale-private.h"
#include "MagickCore/log.h"
#include "MagickCore/magick.h"
#include "MagickCore/memory_.h"
#include "MagickCore/monitor.h"
#include "MagickCore/monitor-private.h"
#include "MagickCore/montage.h"
#include "MagickCore/nt-base-private.h"
#include "MagickCore/option.h"
#include "MagickCore/paint.h"
#include "MagickCore/pixel.h"
#include "MagickCore/pixel-accessor.h"
#include "MagickCore/property.h"
#include "MagickCore/quantum.h"
#include "MagickCore/quantum-private.h"
#include "MagickCore/resize.h"
#include "MagickCore/resource_.h"
#include "MagickCore/shear.h"
#include "MagickCore/segment.h"
#include "MagickCore/statistic.h"
#include "MagickCore/string_.h"
#include "MagickCore/string-private.h"
#include "MagickCore/timer-private.h"
#include "MagickCore/transform.h"
#include "MagickCore/transform-private.h"
#include "MagickCore/threshold.h"
#include "MagickCore/utility.h"
#include "MagickCore/utility-private.h"
#include "MagickCore/version.h"
#include "MagickCore/visual-effects.h"
#include "MagickCore/widget.h"
#include "MagickCore/widget-private.h"
#include "MagickCore/xwindow.h"
#include "MagickCore/xwindow-private.h"
#if defined(MAGICKCORE_X11_DELEGATE)

/*
  Typedef declarations.
*/
typedef enum
{
  CopyMode,
  CropMode,
  CutMode
} ClipboardMode;

typedef enum
{
  OpenCommand,
  NextCommand,
  FormerCommand,
  SelectCommand,
  SaveCommand,
  PrintCommand,
  DeleteCommand,
  NewCommand,
  VisualDirectoryCommand,
  QuitCommand,
  UndoCommand,
  RedoCommand,
  CutCommand,
  CopyCommand,
  PasteCommand,
  HalfSizeCommand,
  OriginalSizeCommand,
  DoubleSizeCommand,
  ResizeCommand,
  ApplyCommand,
  RefreshCommand,
  RestoreCommand,
  CropCommand,
  ChopCommand,
  FlopCommand,
  FlipCommand,
  RotateRightCommand,
  RotateLeftCommand,
  RotateCommand,
  ShearCommand,
  RollCommand,
  TrimCommand,
  HueCommand,
  SaturationCommand,
  BrightnessCommand,
  GammaCommand,
  SpiffCommand,
  DullCommand,
  ContrastStretchCommand,
  SigmoidalContrastCommand,
  NormalizeCommand,
  EqualizeCommand,
  NegateCommand,
  GrayscaleCommand,
  MapCommand,
  QuantizeCommand,
  DespeckleCommand,
  EmbossCommand,
  ReduceNoiseCommand,
  AddNoiseCommand,
  SharpenCommand,
  BlurCommand,
  ThresholdCommand,
  EdgeDetectCommand,
  SpreadCommand,
  ShadeCommand,
  RaiseCommand,
  SegmentCommand,
  SolarizeCommand,
  SepiaToneCommand,
  SwirlCommand,
  ImplodeCommand,
  VignetteCommand,
  WaveCommand,
  OilPaintCommand,
  CharcoalDrawCommand,
  AnnotateCommand,
  DrawCommand,
  ColorCommand,
  MatteCommand,
  CompositeCommand,
  AddBorderCommand,
  AddFrameCommand,
  CommentCommand,
  LaunchCommand,
  RegionOfInterestCommand,
  ROIHelpCommand,
  ROIDismissCommand,
  InfoCommand,
  ZoomCommand,
  ShowPreviewCommand,
  ShowHistogramCommand,
  ShowMatteCommand,
  BackgroundCommand,
  SlideShowCommand,
  PreferencesCommand,
  HelpCommand,
  BrowseDocumentationCommand,
  VersionCommand,
  SaveToUndoBufferCommand,
  FreeBuffersCommand,
  NullCommand
} DisplayCommand;

typedef enum
{
  AnnotateNameCommand,
  AnnotateFontColorCommand,
  AnnotateBackgroundColorCommand,
  AnnotateRotateCommand,
  AnnotateHelpCommand,
  AnnotateDismissCommand,
  TextHelpCommand,
  TextApplyCommand,
  ChopDirectionCommand,
  ChopHelpCommand,
  ChopDismissCommand,
  HorizontalChopCommand,
  VerticalChopCommand,
  ColorEditMethodCommand,
  ColorEditColorCommand,
  ColorEditBorderCommand,
  ColorEditFuzzCommand,
  ColorEditUndoCommand,
  ColorEditHelpCommand,
  ColorEditDismissCommand,
  CompositeOperatorsCommand,
  CompositeDissolveCommand,
  CompositeDisplaceCommand,
  CompositeHelpCommand,
  CompositeDismissCommand,
  CropHelpCommand,
  CropDismissCommand,
  RectifyCopyCommand,
  RectifyHelpCommand,
  RectifyDismissCommand,
  DrawElementCommand,
  DrawColorCommand,
  DrawStippleCommand,
  DrawWidthCommand,
  DrawUndoCommand,
  DrawHelpCommand,
  DrawDismissCommand,
  MatteEditMethod,
  MatteEditBorderCommand,
  MatteEditFuzzCommand,
  MatteEditValueCommand,
  MatteEditUndoCommand,
  MatteEditHelpCommand,
  MatteEditDismissCommand,
  PasteOperatorsCommand,
  PasteHelpCommand,
  PasteDismissCommand,
  RotateColorCommand,
  RotateDirectionCommand,
  RotateCropCommand,
  RotateSharpenCommand,
  RotateHelpCommand,
  RotateDismissCommand,
  HorizontalRotateCommand,
  VerticalRotateCommand,
  TileLoadCommand,
  TileNextCommand,
  TileFormerCommand,
  TileDeleteCommand,
  TileUpdateCommand
} ModeType;

/*
  Define declarations.
*/
#define MaxColors  MagickMin((ssize_t) windows->visual_info->colormap_size,256L)
#define BricksWidth  20
#define BricksHeight  20
#define DiagonalWidth  16
#define DiagonalHeight  16
#define HighlightWidth  8
#define HighlightHeight  8
#define OpaqueWidth  8
#define OpaqueHeight  8
#define ScalesWidth  16
#define ScalesHeight  16
#define ShadowWidth  8
#define ShadowHeight  8
#define VerticalWidth  16
#define VerticalHeight  16
#define WavyWidth  16
#define WavyHeight  16

/*
  Constant declarations.
*/
static const unsigned char
  HighlightBitmap[8] =
  {
    0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55
  },
  OpaqueBitmap[8] =
  {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  },
  ShadowBitmap[8] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

static const char
  ImageAnnotateHelp[] =
  {
    "In annotate mode, the Command widget has these options:\n"
    "\n"
    "    Font Name\n"
    "      fixed\n"
    "      variable\n"
    "      5x8\n"
    "      6x10\n"
    "      7x13bold\n"
    "      8x13bold\n"
    "      9x15bold\n"
    "      10x20\n"
    "      12x24\n"
    "      Browser...\n"
    "    Font Color\n"
    "      black\n"
    "      blue\n"
    "      cyan\n"
    "      green\n"
    "      gray\n"
    "      red\n"
    "      magenta\n"
    "      yellow\n"
    "      white\n"
    "      transparent\n"
    "      Browser...\n"
    "    Font Color\n"
    "      black\n"
    "      blue\n"
    "      cyan\n"
    "      green\n"
    "      gray\n"
    "      red\n"
    "      magenta\n"
    "      yellow\n"
    "      white\n"
    "      transparent\n"
    "      Browser...\n"
    "    Rotate Text\n"
    "      -90\n"
    "      -45\n"
    "      -30\n"
    "      0\n"
    "      30\n"
    "      45\n"
    "      90\n"
    "      180\n"
    "      Dialog...\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "Choose a font name from the Font Name sub-menu.  Additional\n"
    "font names can be specified with the font browser.  You can\n"
    "change the menu names by setting the X resources font1\n"
    "through font9.\n"
    "\n"
    "Choose a font color from the Font Color sub-menu.\n"
    "Additional font colors can be specified with the color\n"
    "browser.  You can change the menu colors by setting the X\n"
    "resources pen1 through pen9.\n"
    "\n"
    "If you select the color browser and press Grab, you can\n"
    "choose the font color by moving the pointer to the desired\n"
    "color on the screen and press any button.\n"
    "\n"
    "If you choose to rotate the text, choose Rotate Text from the\n"
    "menu and select an angle.  Typically you will only want to\n"
    "rotate one line of text at a time.  Depending on the angle you\n"
    "choose, subsequent lines may end up overwriting each other.\n"
    "\n"
    "Choosing a font and its color is optional.  The default font\n"
    "is fixed and the default color is black.  However, you must\n"
    "choose a location to begin entering text and press button 1.\n"
    "An underscore character will appear at the location of the\n"
    "pointer.  The cursor changes to a pencil to indicate you are\n"
    "in text mode.  To exit immediately, press Dismiss.\n"
    "\n"
    "In text mode, any key presses will display the character at\n"
    "the location of the underscore and advance the underscore\n"
    "cursor.  Enter your text and once completed press Apply to\n"
    "finish your image annotation.  To correct errors press BACK\n"
    "SPACE.  To delete an entire line of text, press DELETE.  Any\n"
    "text that exceeds the boundaries of the image window is\n"
    "automagically continued onto the next line.\n"
    "\n"
    "The actual color you request for the font is saved in the\n"
    "image.  However, the color that appears in your image window\n"
    "may be different.  For example, on a monochrome screen the\n"
    "text will appear black or white even if you choose the color\n"
    "red as the font color.  However, the image saved to a file\n"
    "with -write is written with red lettering.  To assure the\n"
    "correct color text in the final image, any PseudoClass image\n"
    "is promoted to DirectClass (see miff(5)).  To force a\n"
    "PseudoClass image to remain PseudoClass, use -colors.\n"
  },
  ImageChopHelp[] =
  {
    "In chop mode, the Command widget has these options:\n"
    "\n"
    "    Direction\n"
    "      horizontal\n"
    "      vertical\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "If the you choose the horizontal direction (this the\n"
    "default), the area of the image between the two horizontal\n"
    "endpoints of the chop line is removed.  Otherwise, the area\n"
    "of the image between the two vertical endpoints of the chop\n"
    "line is removed.\n"
    "\n"
    "Select a location within the image window to begin your chop,\n"
    "press and hold any button.  Next, move the pointer to\n"
    "another location in the image.  As you move a line will\n"
    "connect the initial location and the pointer.  When you\n"
    "release the button, the area within the image to chop is\n"
    "determined by which direction you choose from the Command\n"
    "widget.\n"
    "\n"
    "To cancel the image chopping, move the pointer back to the\n"
    "starting point of the line and release the button.\n"
  },
  ImageColorEditHelp[] =
  {
    "In color edit mode, the Command widget has these options:\n"
    "\n"
    "    Method\n"
    "      point\n"
    "      replace\n"
    "      floodfill\n"
    "      filltoborder\n"
    "      reset\n"
    "    Pixel Color\n"
    "      black\n"
    "      blue\n"
    "      cyan\n"
    "      green\n"
    "      gray\n"
    "      red\n"
    "      magenta\n"
    "      yellow\n"
    "      white\n"
    "      Browser...\n"
    "    Border Color\n"
    "      black\n"
    "      blue\n"
    "      cyan\n"
    "      green\n"
    "      gray\n"
    "      red\n"
    "      magenta\n"
    "      yellow\n"
    "      white\n"
    "      Browser...\n"
    "    Fuzz\n"
    "      0%\n"
    "      2%\n"
    "      5%\n"
    "      10%\n"
    "      15%\n"
    "      Dialog...\n"
    "    Undo\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "Choose a color editing method from the Method sub-menu\n"
    "of the Command widget.  The point method recolors any pixel\n"
    "selected with the pointer until the button is released.  The\n"
    "replace method recolors any pixel that matches the color of\n"
    "the pixel you select with a button press.  Floodfill recolors\n"
    "any pixel that matches the color of the pixel you select with\n"
    "a button press and is a neighbor.  Whereas filltoborder recolors\n"
    "any neighbor pixel that is not the border color.  Finally reset\n"
    "changes the entire image to the designated color.\n"
    "\n"
    "Next, choose a pixel color from the Pixel Color sub-menu.\n"
    "Additional pixel colors can be specified with the color\n"
    "browser.  You can change the menu colors by setting the X\n"
    "resources pen1 through pen9.\n"
    "\n"
    "Now press button 1 to select a pixel within the image window\n"
    "to change its color.  Additional pixels may be recolored as\n"
    "prescribed by the method you choose.\n"
    "\n"
    "If the Magnify widget is mapped, it can be helpful in positioning\n"
    "your pointer within the image (refer to button 2).\n"
    "\n"
    "The actual color you request for the pixels is saved in the\n"
    "image.  However, the color that appears in your image window\n"
    "may be different.  For example, on a monochrome screen the\n"
    "pixel will appear black or white even if you choose the\n"
    "color red as the pixel color.  However, the image saved to a\n"
    "file with -write is written with red pixels.  To assure the\n"
    "correct color text in the final image, any PseudoClass image\n"
    "is promoted to DirectClass (see miff(5)).  To force a\n"
    "PseudoClass image to remain PseudoClass, use -colors.\n"
  },
  ImageCompositeHelp[] =
  {
    "First a widget window is displayed requesting you to enter an\n"
    "image name. Press Composite, Grab or type a file name.\n"
    "Press Cancel if you choose not to create a composite image.\n"
    "When you choose Grab, move the pointer to the desired window\n"
    "and press any button.\n"
    "\n"
    "If the Composite image does not have any matte information,\n"
    "you are informed and the file browser is displayed again.\n"
    "Enter the name of a mask image.  The image is typically\n"
    "grayscale and the same size as the composite image.  If the\n"
    "image is not grayscale, it is converted to grayscale and the\n"
    "resulting intensities are used as matte information.\n"
    "\n"
    "A small window appears showing the location of the cursor in\n"
    "the image window. You are now in composite mode.  To exit\n"
    "immediately, press Dismiss.  In composite mode, the Command\n"
    "widget has these options:\n"
    "\n"
    "    Operators\n"
    "      Over\n"
    "      In\n"
    "      Out\n"
    "      Atop\n"
    "      Xor\n"
    "      Plus\n"
    "      Minus\n"
    "      Add\n"
    "      Subtract\n"
    "      Difference\n"
    "      Multiply\n"
    "      Bumpmap\n"
    "      Copy\n"
    "      CopyRed\n"
    "      CopyGreen\n"
    "      CopyBlue\n"
    "      CopyOpacity\n"
    "      Clear\n"
    "    Dissolve\n"
    "    Displace\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "Choose a composite operation from the Operators sub-menu of\n"
    "the Command widget.  How each operator behaves is described\n"
    "below.  Image window is the image currently displayed on\n"
    "your X server and image is the image obtained with the File\n"
    "Browser widget.\n"
    "\n"
    "Over     The result is the union of the two image shapes,\n"
    "         with image obscuring image window in the region of\n"
    "         overlap.\n"
    "\n"
    "In       The result is simply image cut by the shape of\n"
    "         image window.  None of the image data of image\n"
    "         window is in the result.\n"
    "\n"
    "Out      The resulting image is image with the shape of\n"
    "         image window cut out.\n"
    "\n"
    "Atop     The result is the same shape as the image window,\n"
    "         with image obscuring image window where the image\n"
    "         shapes overlap.  Note this differs from over\n"
    "         because the portion of image outside image window's\n"
    "         shape does not appear in the result.\n"
    "\n"
    "Xor      The result is the image data from both image and\n"
    "         image window that is outside the overlap region.\n"
    "         The overlap region is blank.\n"
    "\n"
    "Plus     The result is just the sum of the image data.\n"
    "         Output values are cropped to QuantumRange (no overflow).\n"
    "\n"
    "Minus    The result of image - image window, with underflow\n"
    "         cropped to zero.\n"
    "\n"
    "Add      The result of image + image window, with overflow\n"
    "         wrapping around (mod 256).\n"
    "\n"
    "Subtract The result of image - image window, with underflow\n"
    "         wrapping around (mod 256).  The add and subtract\n"
    "         operators can be used to perform reversible\n"
    "         transformations.\n"
    "\n"
    "Difference\n"
    "         The result of abs(image - image window).  This\n"
    "         useful for comparing two very similar images.\n"
    "\n"
    "Multiply\n"
    "         The result of image * image window.  This\n"
    "         useful for the creation of drop-shadows.\n"
    "\n"
    "Bumpmap  The result of surface normals from image * image\n"
    "         window.\n"
    "\n"
    "Copy     The resulting image is image window replaced with\n"
    "         image.  Here the matte information is ignored.\n"
    "\n"
    "CopyRed  The red layer of the image window is replace with\n"
    "         the red layer of the image.  The other layers are\n"
    "         untouched.\n"
    "\n"
    "CopyGreen\n"
    "         The green layer of the image window is replace with\n"
    "         the green layer of the image.  The other layers are\n"
    "         untouched.\n"
    "\n"
    "CopyBlue The blue layer of the image window is replace with\n"
    "         the blue layer of the image.  The other layers are\n"
    "         untouched.\n"
    "\n"
    "CopyOpacity\n"
    "         The matte layer of the image window is replace with\n"
    "         the matte layer of the image.  The other layers are\n"
    "         untouched.\n"
    "\n"
    "The image compositor requires a matte, or alpha channel in\n"
    "the image for some operations.  This extra channel usually\n"
    "defines a mask which represents a sort of a cookie-cutter\n"
    "for the image.  This the case when matte is opaque (full\n"
    "coverage) for pixels inside the shape, zero outside, and\n"
    "between 0 and QuantumRange on the boundary.  If image does not\n"
    "have a matte channel, it is initialized with 0 for any pixel\n"
    "matching in color to pixel location (0,0), otherwise QuantumRange.\n"
    "\n"
    "If you choose Dissolve, the composite operator becomes Over.  The\n"
    "image matte channel percent transparency is initialized to factor.\n"
    "The image window is initialized to (100-factor). Where factor is the\n"
    "value you specify in the Dialog widget.\n"
    "\n"
    "Displace shifts the image pixels as defined by a displacement\n"
    "map.  With this option, image is used as a displacement map.\n"
    "Black, within the displacement map, is a maximum positive\n"
    "displacement.  White is a maximum negative displacement and\n"
    "middle gray is neutral.  The displacement is scaled to determine\n"
    "the pixel shift.  By default, the displacement applies in both the\n"
    "horizontal and vertical directions.  However, if you specify a mask,\n"
    "image is the horizontal X displacement and mask the vertical Y\n"
    "displacement.\n"
    "\n"
    "Note that matte information for image window is not retained\n"
    "for colormapped X server visuals (e.g. StaticColor,\n"
    "StaticColor, GrayScale, PseudoColor).  Correct compositing\n"
    "behavior may require a TrueColor or DirectColor visual or a\n"
    "Standard Colormap.\n"
    "\n"
    "Choosing a composite operator is optional.  The default\n"
    "operator is replace.  However, you must choose a location to\n"
    "composite your image and press button 1.  Press and hold the\n"
    "button before releasing and an outline of the image will\n"
    "appear to help you identify your location.\n"
    "\n"
    "The actual colors of the composite image is saved.  However,\n"
    "the color that appears in image window may be different.\n"
    "For example, on a monochrome screen image window will appear\n"
    "black or white even though your composited image may have\n"
    "many colors.  If the image is saved to a file it is written\n"
    "with the correct colors.  To assure the correct colors are\n"
    "saved in the final image, any PseudoClass image is promoted\n"
    "to DirectClass (see miff(5)).  To force a PseudoClass image\n"
    "to remain PseudoClass, use -colors.\n"
  },
  ImageCutHelp[] =
  {
    "In cut mode, the Command widget has these options:\n"
    "\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "To define a cut region, press button 1 and drag.  The\n"
    "cut region is defined by a highlighted rectangle that\n"
    "expands or contracts as it follows the pointer.  Once you\n"
    "are satisfied with the cut region, release the button.\n"
    "You are now in rectify mode.  In rectify mode, the Command\n"
    "widget has these options:\n"
    "\n"
    "    Cut\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "You can make adjustments by moving the pointer to one of the\n"
    "cut rectangle corners, pressing a button, and dragging.\n"
    "Finally, press Cut to commit your copy region.  To\n"
    "exit without cutting the image, press Dismiss.\n"
  },
  ImageCopyHelp[] =
  {
    "In copy mode, the Command widget has these options:\n"
    "\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "To define a copy region, press button 1 and drag.  The\n"
    "copy region is defined by a highlighted rectangle that\n"
    "expands or contracts as it follows the pointer.  Once you\n"
    "are satisfied with the copy region, release the button.\n"
    "You are now in rectify mode.  In rectify mode, the Command\n"
    "widget has these options:\n"
    "\n"
    "    Copy\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "You can make adjustments by moving the pointer to one of the\n"
    "copy rectangle corners, pressing a button, and dragging.\n"
    "Finally, press Copy to commit your copy region.  To\n"
    "exit without copying the image, press Dismiss.\n"
  },
  ImageCropHelp[] =
  {
    "In crop mode, the Command widget has these options:\n"
    "\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "To define a cropping region, press button 1 and drag.  The\n"
    "cropping region is defined by a highlighted rectangle that\n"
    "expands or contracts as it follows the pointer.  Once you\n"
    "are satisfied with the cropping region, release the button.\n"
    "You are now in rectify mode.  In rectify mode, the Command\n"
    "widget has these options:\n"
    "\n"
    "    Crop\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "You can make adjustments by moving the pointer to one of the\n"
    "cropping rectangle corners, pressing a button, and dragging.\n"
    "Finally, press Crop to commit your cropping region.  To\n"
    "exit without cropping the image, press Dismiss.\n"
  },
  ImageDrawHelp[] =
  {
    "The cursor changes to a crosshair to indicate you are in\n"
    "draw mode.  To exit immediately, press Dismiss.  In draw mode,\n"
    "the Command widget has these options:\n"
    "\n"
    "    Element\n"
    "      point\n"
    "      line\n"
    "      rectangle\n"
    "      fill rectangle\n"
    "      circle\n"
    "      fill circle\n"
    "      ellipse\n"
    "      fill ellipse\n"
    "      polygon\n"
    "      fill polygon\n"
    "    Color\n"
    "      black\n"
    "      blue\n"
    "      cyan\n"
    "      green\n"
    "      gray\n"
    "      red\n"
    "      magenta\n"
    "      yellow\n"
    "      white\n"
    "      transparent\n"
    "      Browser...\n"
    "    Stipple\n"
    "      Brick\n"
    "      Diagonal\n"
    "      Scales\n"
    "      Vertical\n"
    "      Wavy\n"
    "      Translucent\n"
    "      Opaque\n"
    "      Open...\n"
    "    Width\n"
    "      1\n"
    "      2\n"
    "      4\n"
    "      8\n"
    "      16\n"
    "      Dialog...\n"
    "    Undo\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "Choose a drawing primitive from the Element sub-menu.\n"
    "\n"
    "Choose a color from the Color sub-menu.  Additional\n"
    "colors can be specified with the color browser.\n"
    "\n"
    "If you choose the color browser and press Grab, you can\n"
    "select the color by moving the pointer to the desired\n"
    "color on the screen and press any button.  The transparent\n"
    "color updates the image matte channel and is useful for\n"
    "image compositing.\n"
    "\n"
    "Choose a stipple, if appropriate, from the Stipple sub-menu.\n"
    "Additional stipples can be specified with the file browser.\n"
    "Stipples obtained from the file browser must be on disk in the\n"
    "X11 bitmap format.\n"
    "\n"
    "Choose a width, if appropriate, from the Width sub-menu.  To\n"
    "choose a specific width select the Dialog widget.\n"
    "\n"
    "Choose a point in the Image window and press button 1 and\n"
    "hold.  Next, move the pointer to another location in the\n"
    "image.  As you move, a line connects the initial location and\n"
    "the pointer.  When you release the button, the image is\n"
    "updated with the primitive you just drew.  For polygons, the\n"
    "image is updated when you press and release the button without\n"
    "moving the pointer.\n"
    "\n"
    "To cancel image drawing, move the pointer back to the\n"
    "starting point of the line and release the button.\n"
  },
  DisplayHelp[] =
  {
    "BUTTONS\n"
    "  The effects of each button press is described below.  Three\n"
    "  buttons are required.  If you have a two button mouse,\n"
    "  button 1 and 3 are returned.  Press ALT and button 3 to\n"
    "  simulate button 2.\n"
    "\n"
    "  1    Press this button to map or unmap the Command widget.\n"
    "\n"
    "  2    Press and drag to define a region of the image to\n"
    "       magnify.\n"
    "\n"
    "  3    Press and drag to choose from a select set of commands.\n"
    "       This button behaves differently if the image being\n"
    "       displayed is a visual image directory.  Here, choose a\n"
    "       particular tile of the directory and press this button and\n"
    "       drag to select a command from a pop-up menu.  Choose from\n"
    "       these menu items:\n"
    "\n"
    "           Open\n"
    "           Next\n"
    "           Former\n"
    "           Delete\n"
    "           Update\n"
    "\n"
    "       If you choose Open, the image represented by the tile is\n"
    "       displayed.  To return to the visual image directory, choose\n"
    "       Next from the Command widget.  Next and Former moves to the\n"
    "       next or former image respectively.  Choose Delete to delete\n"
    "       a particular image tile.  Finally, choose Update to\n"
    "       synchronize all the image tiles with their respective\n"
    "       images.\n"
    "\n"
    "COMMAND WIDGET\n"
    "  The Command widget lists a number of sub-menus and commands.\n"
    "  They are\n"
    "\n"
    "      File\n"
    "        Open...\n"
    "        Next\n"
    "        Former\n"
    "        Select...\n"
    "        Save...\n"
    "        Print...\n"
    "        Delete...\n"
    "        New...\n"
    "        Visual Directory...\n"
    "        Quit\n"
    "      Edit\n"
    "        Undo\n"
    "        Redo\n"
    "        Cut\n"
    "        Copy\n"
    "        Paste\n"
    "      View\n"
    "        Half Size\n"
    "        Original Size\n"
    "        Double Size\n"
    "        Resize...\n"
    "        Apply\n"
    "        Refresh\n"
    "        Restore\n"
    "      Transform\n"
    "        Crop\n"
    "        Chop\n"
    "        Flop\n"
    "        Flip\n"
    "        Rotate Right\n"
    "        Rotate Left\n"
    "        Rotate...\n"
    "        Shear...\n"
    "        Roll...\n"
    "        Trim Edges\n"
    "      Enhance\n"
    "        Brightness...\n"
    "        Saturation...\n"
    "        Hue...\n"
    "        Gamma...\n"
    "        Sharpen...\n"
    "        Dull\n"
    "        Contrast Stretch...\n"
    "        Sigmoidal Contrast...\n"
    "        Normalize\n"
    "        Equalize\n"
    "        Negate\n"
    "        Grayscale\n"
    "        Map...\n"
    "        Quantize...\n"
    "      Effects\n"
    "        Despeckle\n"
    "        Emboss\n"
    "        Reduce Noise\n"
    "        Add Noise\n"
    "        Sharpen...\n"
    "        Blur...\n"
    "        Threshold...\n"
    "        Edge Detect...\n"
    "        Spread...\n"
    "        Shade...\n"
    "        Painting...\n"
    "        Segment...\n"
    "      F/X\n"
    "        Solarize...\n"
    "        Sepia Tone...\n"
    "        Swirl...\n"
    "        Implode...\n"
    "        Vignette...\n"
    "        Wave...\n"
    "        Oil Painting...\n"
    "        Charcoal Drawing...\n"
    "      Image Edit\n"
    "        Annotate...\n"
    "        Draw...\n"
    "        Color...\n"
    "        Matte...\n"
    "        Composite...\n"
    "        Add Border...\n"
    "        Add Frame...\n"
    "        Comment...\n"
    "        Launch...\n"
    "        Region of Interest...\n"
    "      Miscellany\n"
    "        Image Info\n"
    "        Zoom Image\n"
    "        Show Preview...\n"
    "        Show Histogram\n"
    "        Show Matte\n"
    "        Background...\n"
    "        Slide Show\n"
    "        Preferences...\n"
    "      Help\n"
    "        Overview\n"
    "        Browse Documentation\n"
    "        About Display\n"
    "\n"
    "  Menu items with a indented triangle have a sub-menu.  They\n"
    "  are represented above as the indented items.  To access a\n"
    "  sub-menu item, move the pointer to the appropriate menu and\n"
    "  press a button and drag.  When you find the desired sub-menu\n"
    "  item, release the button and the command is executed.  Move\n"
    "  the pointer away from the sub-menu if you decide not to\n"
    "  execute a particular command.\n"
    "\n"
    "KEYBOARD ACCELERATORS\n"
    "  Accelerators are one or two key presses that effect a\n"
    "  particular command.  The keyboard accelerators that\n"
    "  display(1) understands is:\n"
    "\n"
    "  Ctl+O     Press to open an image from a file.\n"
    "\n"
    "  space     Press to display the next image.\n"
    "\n"
    "            If the image is a multi-paged document such as a Postscript\n"
    "            document, you can skip ahead several pages by preceding\n"
    "            this command with a number.  For example to display the\n"
    "            third page beyond the current page, press 3<space>.\n"
    "\n"
    "  backspace Press to display the former image.\n"
    "\n"
    "            If the image is a multi-paged document such as a Postscript\n"
    "            document, you can skip behind several pages by preceding\n"
    "            this command with a number.  For example to display the\n"
    "            third page preceding the current page, press 3<backspace>.\n"
    "\n"
    "  Ctl+S     Press to write the image to a file.\n"
    "\n"
    "  Ctl+P     Press to print the image to a Postscript printer.\n"
    "\n"
    "  Ctl+D     Press to delete an image file.\n"
    "\n"
    "  Ctl+N     Press to create a blank canvas.\n"
    "\n"
    "  Ctl+Q     Press to discard all images and exit program.\n"
    "\n"
    "  Ctl+Z     Press to undo last image transformation.\n"
    "\n"
    "  Ctl+R     Press to redo last image transformation.\n"
    "\n"
    "  Ctl+X     Press to cut a region of the image.\n"
    "\n"
    "  Ctl+C     Press to copy a region of the image.\n"
    "\n"
    "  Ctl+V     Press to paste a region to the image.\n"
    "\n"
    "  <         Press to half the image size.\n"
    "\n"
    "  -         Press to return to the original image size.\n"
    "\n"
    "  >         Press to double the image size.\n"
    "\n"
    "  %         Press to resize the image to a width and height you\n"
    "            specify.\n"
    "\n"
    "Cmd-A       Press to make any image transformations permanent."
    "\n"
    "            By default, any image size transformations are applied\n"
    "            to the original image to create the image displayed on\n"
    "            the X server.  However, the transformations are not\n"
    "            permanent (i.e. the original image does not change\n"
    "            size only the X image does).  For example, if you\n"
    "            press > the X image will appear to double in size,\n"
    "            but the original image will in fact remain the same size.\n"
    "            To force the original image to double in size, press >\n"
    "            followed by Cmd-A.\n"
    "\n"
    "  @         Press to refresh the image window.\n"
    "\n"
    "  C         Press to cut out a rectangular region of the image.\n"
    "\n"
    "  [         Press to chop the image.\n"
    "\n"
    "  H         Press to flop image in the horizontal direction.\n"
    "\n"
    "  V         Press to flip image in the vertical direction.\n"
    "\n"
    "  /         Press to rotate the image 90 degrees clockwise.\n"
    "\n"
    " \\         Press to rotate the image 90 degrees counter-clockwise.\n"
    "\n"
    "  *         Press to rotate the image the number of degrees you\n"
    "            specify.\n"
    "\n"
    "  S         Press to shear the image the number of degrees you\n"
    "            specify.\n"
    "\n"
    "  R         Press to roll the image.\n"
    "\n"
    "  T         Press to trim the image edges.\n"
    "\n"
    "  Shft-H    Press to vary the image hue.\n"
    "\n"
    "  Shft-S    Press to vary the color saturation.\n"
    "\n"
    "  Shft-L    Press to vary the color brightness.\n"
    "\n"
    "  Shft-G    Press to gamma correct the image.\n"
    "\n"
    "  Shft-C    Press to sharpen the image contrast.\n"
    "\n"
    "  Shft-Z    Press to dull the image contrast.\n"
    "\n"
    "  =         Press to perform histogram equalization on the image.\n"
    "\n"
    "  Shft-N    Press to perform histogram normalization on the image.\n"
    "\n"
    "  Shft-~    Press to negate the colors of the image.\n"
    "\n"
    "  .         Press to convert the image colors to gray.\n"
    "\n"
    "  Shft-#    Press to set the maximum number of unique colors in the\n"
    "            image.\n"
    "\n"
    "  F2        Press to reduce the speckles in an image.\n"
    "\n"
    "  F3        Press to eliminate peak noise from an image.\n"
    "\n"
    "  F4        Press to add noise to an image.\n"
    "\n"
    "  F5        Press to sharpen an image.\n"
    "\n"
    "  F6        Press to delete an image file.\n"
    "\n"
    "  F7        Press to threshold the image.\n"
    "\n"
    "  F8        Press to detect edges within an image.\n"
    "\n"
    "  F9        Press to emboss an image.\n"
    "\n"
    "  F10       Press to displace pixels by a random amount.\n"
    "\n"
    "  F11       Press to negate all pixels above the threshold level.\n"
    "\n"
    "  F12       Press to shade the image using a distant light source.\n"
    "\n"
    "  F13       Press to lighten or darken image edges to create a 3-D effect.\n"
    "\n"
    "  F14       Press to segment the image by color.\n"
    "\n"
    "  Meta-S    Press to swirl image pixels about the center.\n"
    "\n"
    "  Meta-I    Press to implode image pixels about the center.\n"
    "\n"
    "  Meta-W    Press to alter an image along a sine wave.\n"
    "\n"
    "  Meta-P    Press to simulate an oil painting.\n"
    "\n"
    "  Meta-C    Press to simulate a charcoal drawing.\n"
    "\n"
    "  Alt-A     Press to annotate the image with text.\n"
    "\n"
    "  Alt-D     Press to draw on an image.\n"
    "\n"
    "  Alt-P     Press to edit an image pixel color.\n"
    "\n"
    "  Alt-M     Press to edit the image matte information.\n"
    "\n"
    "  Alt-V     Press to composite the image with another.\n"
    "\n"
    "  Alt-B     Press to add a border to the image.\n"
    "\n"
    "  Alt-F     Press to add an ornamental border to the image.\n"
    "\n"
    "  Alt-Shft-!\n"
    "            Press to add an image comment.\n"
    "\n"
    "  Ctl-A     Press to apply image processing techniques to a region\n"
    "            of interest.\n"
    "\n"
    "  Shft-?    Press to display information about the image.\n"
    "\n"
    "  Shft-+    Press to map the zoom image window.\n"
    "\n"
    "  Shft-P    Press to preview an image enhancement, effect, or f/x.\n"
    "\n"
    "  F1        Press to display helpful information about display(1).\n"
    "\n"
    "  Find      Press to browse documentation about ImageMagick.\n"
    "\n"
    "  1-9       Press to change the level of magnification.\n"
    "\n"
    "  Use the arrow keys to move the image one pixel up, down,\n"
    "  left, or right within the magnify window.  Be sure to first\n"
    "  map the magnify window by pressing button 2.\n"
    "\n"
    "  Press ALT and one of the arrow keys to trim off one pixel\n"
    "  from any side of the image.\n"
  },
  ImageMatteEditHelp[] =
  {
    "Matte information within an image is useful for some\n"
    "operations such as image compositing (See IMAGE\n"
    "COMPOSITING).  This extra channel usually defines a mask\n"
    "which represents a sort of a cookie-cutter for the image.\n"
    "This the case when matte is opaque (full coverage) for\n"
    "pixels inside the shape, zero outside, and between 0 and\n"
    "QuantumRange on the boundary.\n"
    "\n"
    "A small window appears showing the location of the cursor in\n"
    "the image window. You are now in matte edit mode.  To exit\n"
    "immediately, press Dismiss.  In matte edit mode, the Command\n"
    "widget has these options:\n"
    "\n"
    "    Method\n"
    "      point\n"
    "      replace\n"
    "      floodfill\n"
    "      filltoborder\n"
    "      reset\n"
    "    Border Color\n"
    "      black\n"
    "      blue\n"
    "      cyan\n"
    "      green\n"
    "      gray\n"
    "      red\n"
    "      magenta\n"
    "      yellow\n"
    "      white\n"
    "      Browser...\n"
    "    Fuzz\n"
    "      0%\n"
    "      2%\n"
    "      5%\n"
    "      10%\n"
    "      15%\n"
    "      Dialog...\n"
    "    Matte\n"
    "      Opaque\n"
    "      Transparent\n"
    "      Dialog...\n"
    "    Undo\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "Choose a matte editing method from the Method sub-menu of\n"
    "the Command widget.  The point method changes the matte value\n"
    "of any pixel selected with the pointer until the button is\n"
    "is released.  The replace method changes the matte value of\n"
    "any pixel that matches the color of the pixel you select with\n"
    "a button press.  Floodfill changes the matte value of any pixel\n"
    "that matches the color of the pixel you select with a button\n"
    "press and is a neighbor.  Whereas filltoborder changes the matte\n"
    "value any neighbor pixel that is not the border color.  Finally\n"
    "reset changes the entire image to the designated matte value.\n"
    "\n"
    "Choose Matte Value and pick Opaque or Transparent.  For other values\n"
    "select the Dialog entry.  Here a dialog appears requesting a matte\n"
    "value.  The value you select is assigned as the opacity value of the\n"
    "selected pixel or pixels.\n"
    "\n"
    "Now, press any button to select a pixel within the image\n"
    "window to change its matte value.\n"
    "\n"
    "If the Magnify widget is mapped, it can be helpful in positioning\n"
    "your pointer within the image (refer to button 2).\n"
    "\n"
    "Matte information is only valid in a DirectClass image.\n"
    "Therefore, any PseudoClass image is promoted to DirectClass\n"
    "(see miff(5)).  Note that matte information for PseudoClass\n"
    "is not retained for colormapped X server visuals (e.g.\n"
    "StaticColor, StaticColor, GrayScale, PseudoColor) unless you\n"
    "immediately save your image to a file (refer to Write).\n"
    "Correct matte editing behavior may require a TrueColor or\n"
    "DirectColor visual or a Standard Colormap.\n"
  },
  ImagePanHelp[] =
  {
    "When an image exceeds the width or height of the X server\n"
    "screen, display maps a small panning icon.  The rectangle\n"
    "within the panning icon shows the area that is currently\n"
    "displayed in the image window.  To pan about the image,\n"
    "press any button and drag the pointer within the panning\n"
    "icon.  The pan rectangle moves with the pointer and the\n"
    "image window is updated to reflect the location of the\n"
    "rectangle within the panning icon.  When you have selected\n"
    "the area of the image you wish to view, release the button.\n"
    "\n"
    "Use the arrow keys to pan the image one pixel up, down,\n"
    "left, or right within the image window.\n"
    "\n"
    "The panning icon is withdrawn if the image becomes smaller\n"
    "than the dimensions of the X server screen.\n"
  },
  ImagePasteHelp[] =
  {
    "A small window appears showing the location of the cursor in\n"
    "the image window. You are now in paste mode.  To exit\n"
    "immediately, press Dismiss.  In paste mode, the Command\n"
    "widget has these options:\n"
    "\n"
    "    Operators\n"
    "      over\n"
    "      in\n"
    "      out\n"
    "      atop\n"
    "      xor\n"
    "      plus\n"
    "      minus\n"
    "      add\n"
    "      subtract\n"
    "      difference\n"
    "      replace\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "Choose a composite operation from the Operators sub-menu of\n"
    "the Command widget.  How each operator behaves is described\n"
    "below.  Image window is the image currently displayed on\n"
    "your X server and image is the image obtained with the File\n"
    "Browser widget.\n"
    "\n"
    "Over     The result is the union of the two image shapes,\n"
    "         with image obscuring image window in the region of\n"
    "         overlap.\n"
    "\n"
    "In       The result is simply image cut by the shape of\n"
    "         image window.  None of the image data of image\n"
    "         window is in the result.\n"
    "\n"
    "Out      The resulting image is image with the shape of\n"
    "         image window cut out.\n"
    "\n"
    "Atop     The result is the same shape as the image window,\n"
    "         with image obscuring image window where the image\n"
    "         shapes overlap.  Note this differs from over\n"
    "         because the portion of image outside image window's\n"
    "         shape does not appear in the result.\n"
    "\n"
    "Xor      The result is the image data from both image and\n"
    "         image window that is outside the overlap region.\n"
    "         The overlap region is blank.\n"
    "\n"
    "Plus     The result is just the sum of the image data.\n"
    "         Output values are cropped to QuantumRange (no overflow).\n"
    "         This operation is independent of the matte\n"
    "         channels.\n"
    "\n"
    "Minus    The result of image - image window, with underflow\n"
    "         cropped to zero.\n"
    "\n"
    "Add      The result of image + image window, with overflow\n"
    "         wrapping around (mod 256).\n"
    "\n"
    "Subtract The result of image - image window, with underflow\n"
    "         wrapping around (mod 256).  The add and subtract\n"
    "         operators can be used to perform reversible\n"
    "         transformations.\n"
    "\n"
    "Difference\n"
    "         The result of abs(image - image window).  This\n"
    "         useful for comparing two very similar images.\n"
    "\n"
    "Copy     The resulting image is image window replaced with\n"
    "         image.  Here the matte information is ignored.\n"
    "\n"
    "CopyRed  The red layer of the image window is replace with\n"
    "         the red layer of the image.  The other layers are\n"
    "         untouched.\n"
    "\n"
    "CopyGreen\n"
    "         The green layer of the image window is replace with\n"
    "         the green layer of the image.  The other layers are\n"
    "         untouched.\n"
    "\n"
    "CopyBlue The blue layer of the image window is replace with\n"
    "         the blue layer of the image.  The other layers are\n"
    "         untouched.\n"
    "\n"
    "CopyOpacity\n"
    "         The matte layer of the image window is replace with\n"
    "         the matte layer of the image.  The other layers are\n"
    "         untouched.\n"
    "\n"
    "The image compositor requires a matte, or alpha channel in\n"
    "the image for some operations.  This extra channel usually\n"
    "defines a mask which represents a sort of a cookie-cutter\n"
    "for the image.  This the case when matte is opaque (full\n"
    "coverage) for pixels inside the shape, zero outside, and\n"
    "between 0 and QuantumRange on the boundary.  If image does not\n"
    "have a matte channel, it is initialized with 0 for any pixel\n"
    "matching in color to pixel location (0,0), otherwise QuantumRange.\n"
    "\n"
    "Note that matte information for image window is not retained\n"
    "for colormapped X server visuals (e.g. StaticColor,\n"
    "StaticColor, GrayScale, PseudoColor).  Correct compositing\n"
    "behavior may require a TrueColor or DirectColor visual or a\n"
    "Standard Colormap.\n"
    "\n"
    "Choosing a composite operator is optional.  The default\n"
    "operator is replace.  However, you must choose a location to\n"
    "paste your image and press button 1.  Press and hold the\n"
    "button before releasing and an outline of the image will\n"
    "appear to help you identify your location.\n"
    "\n"
    "The actual colors of the pasted image is saved.  However,\n"
    "the color that appears in image window may be different.\n"
    "For example, on a monochrome screen image window will appear\n"
    "black or white even though your pasted image may have\n"
    "many colors.  If the image is saved to a file it is written\n"
    "with the correct colors.  To assure the correct colors are\n"
    "saved in the final image, any PseudoClass image is promoted\n"
    "to DirectClass (see miff(5)).  To force a PseudoClass image\n"
    "to remain PseudoClass, use -colors.\n"
  },
  ImageROIHelp[] =
  {
    "In region of interest mode, the Command widget has these\n"
    "options:\n"
    "\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "To define a region of interest, press button 1 and drag.\n"
    "The region of interest is defined by a highlighted rectangle\n"
    "that expands or contracts as it follows the pointer.  Once\n"
    "you are satisfied with the region of interest, release the\n"
    "button.  You are now in apply mode.  In apply mode the\n"
    "Command widget has these options:\n"
    "\n"
    "      File\n"
    "        Save...\n"
    "        Print...\n"
    "      Edit\n"
    "        Undo\n"
    "        Redo\n"
    "      Transform\n"
    "        Flop\n"
    "        Flip\n"
    "        Rotate Right\n"
    "        Rotate Left\n"
    "      Enhance\n"
    "        Hue...\n"
    "        Saturation...\n"
    "        Brightness...\n"
    "        Gamma...\n"
    "        Spiff\n"
    "        Dull\n"
    "        Contrast Stretch\n"
    "        Sigmoidal Contrast...\n"
    "        Normalize\n"
    "        Equalize\n"
    "        Negate\n"
    "        Grayscale\n"
    "        Map...\n"
    "        Quantize...\n"
    "      Effects\n"
    "        Despeckle\n"
    "        Emboss\n"
    "        Reduce Noise\n"
    "        Sharpen...\n"
    "        Blur...\n"
    "        Threshold...\n"
    "        Edge Detect...\n"
    "        Spread...\n"
    "        Shade...\n"
    "        Raise...\n"
    "        Segment...\n"
    "      F/X\n"
    "        Solarize...\n"
    "        Sepia Tone...\n"
    "        Swirl...\n"
    "        Implode...\n"
    "        Vignette...\n"
    "        Wave...\n"
    "        Oil Painting...\n"
    "        Charcoal Drawing...\n"
    "      Miscellany\n"
    "        Image Info\n"
    "        Zoom Image\n"
    "        Show Preview...\n"
    "        Show Histogram\n"
    "        Show Matte\n"
    "      Help\n"
    "      Dismiss\n"
    "\n"
    "You can make adjustments to the region of interest by moving\n"
    "the pointer to one of the rectangle corners, pressing a\n"
    "button, and dragging.  Finally, choose an image processing\n"
    "technique from the Command widget.  You can choose more than\n"
    "one image processing technique to apply to an area.\n"
    "Alternatively, you can move the region of interest before\n"
    "applying another image processing technique.  To exit, press\n"
    "Dismiss.\n"
  },
  ImageRotateHelp[] =
  {
    "In rotate mode, the Command widget has these options:\n"
    "\n"
    "    Pixel Color\n"
    "      black\n"
    "      blue\n"
    "      cyan\n"
    "      green\n"
    "      gray\n"
    "      red\n"
    "      magenta\n"
    "      yellow\n"
    "      white\n"
    "      Browser...\n"
    "    Direction\n"
    "      horizontal\n"
    "      vertical\n"
    "    Help\n"
    "    Dismiss\n"
    "\n"
    "Choose a background color from the Pixel Color sub-menu.\n"
    "Additional background colors can be specified with the color\n"
    "browser.  You can change the menu colors by setting the X\n"
    "resources pen1 through pen9.\n"
    "\n"
    "If you choose the color browser and press Grab, you can\n"
    "select the background color by moving the pointer to the\n"
    "desired color on the screen and press any button.\n"
    "\n"
    "Choose a point in the image window and press this button and\n"
    "hold.  Next, move the pointer to another location in the\n"
    "image.  As you move a line connects the initial location and\n"
    "the pointer.  When you release the button, the degree of\n"
    "image rotation is determined by the slope of the line you\n"
    "just drew.  The slope is relative to the direction you\n"
    "choose from the Direction sub-menu of the Command widget.\n"
    "\n"
    "To cancel the image rotation, move the pointer back to the\n"
    "starting point of the line and release the button.\n"
  };

static const int
  RoiDelta = 8;

static const unsigned char
  BricksBitmap[] =
  {
    0xff, 0xff, 0x0f, 0x03, 0x0c, 0x00, 0x03, 0x0c, 0x00, 0x03, 0x0c, 0x00,
    0x03, 0x0c, 0x00, 0xff, 0xff, 0x0f, 0x60, 0x80, 0x01, 0x60, 0x80, 0x01,
    0x60, 0x80, 0x01, 0x60, 0x80, 0x01, 0xff, 0xff, 0x0f, 0x03, 0x0c, 0x00,
    0x03, 0x0c, 0x00, 0x03, 0x0c, 0x00, 0x03, 0x0c, 0x00, 0xff, 0xff, 0x0f,
    0x60, 0x80, 0x01, 0x60, 0x80, 0x01, 0x60, 0x80, 0x01, 0x60, 0x80, 0x01
  },
  DiagonalBitmap[] =
  {
    0x44, 0x44, 0x88, 0x88, 0x11, 0x11, 0x22, 0x22, 0x44, 0x44, 0x88, 0x88,
    0x11, 0x11, 0x22, 0x22, 0x44, 0x44, 0x88, 0x88, 0x11, 0x11, 0x22, 0x22,
    0x44, 0x44, 0x88, 0x88, 0x11, 0x11, 0x22, 0x22
  },
  ScalesBitmap[] =
  {
    0x08, 0x08, 0x08, 0x08, 0x14, 0x14, 0xe3, 0xe3, 0x80, 0x80, 0x80, 0x80,
    0x41, 0x41, 0x3e, 0x3e, 0x08, 0x08, 0x08, 0x08, 0x14, 0x14, 0xe3, 0xe3,
    0x80, 0x80, 0x80, 0x80, 0x41, 0x41, 0x3e, 0x3e
  },
  VerticalBitmap[] =
  {
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
  },
  WavyBitmap[] =
  {
    0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfd, 0xff, 0xfd, 0xff, 0xfb, 0xff,
    0xe7, 0xff, 0x1f, 0xff, 0xff, 0xf8, 0xff, 0xe7, 0xff, 0xdf, 0xff, 0xbf,
    0xff, 0xbf, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f
  };

/*
  Forward declarations.
*/
static DisplayCommand
  XImageWindowCommand(Display *,XResourceInfo *,XWindows *,
    const MagickStatusType,KeySym,Image **,ExceptionInfo *);
static Image
  *XMagickCommand(Display *,XResourceInfo *,XWindows *,const DisplayCommand,
    Image **,ExceptionInfo *),
  *XOpenImage(Display *,XResourceInfo *,XWindows *,const MagickBooleanType),
  *XTileImage(Display *,XResourceInfo *,XWindows *,Image *,XEvent *,
    ExceptionInfo *),
  *XVisualDirectoryImage(Display *,XResourceInfo *,XWindows *,
    ExceptionInfo *);
static MagickBooleanType
  XAnnotateEditImage(Display *,XResourceInfo *,XWindows *,Image *,
    ExceptionInfo *),
  XBackgroundImage(Display *,XResourceInfo *,XWindows *,Image **,
    ExceptionInfo *),
  XChopImage(Display *,XResourceInfo *,XWindows *,Image **,
    ExceptionInfo *),
  XCropImage(Display *,XResourceInfo *,XWindows *,Image *,const ClipboardMode,
    ExceptionInfo *),
  XColorEditImage(Display *,XResourceInfo *,XWindows *,Image **,
    ExceptionInfo *),
  XCompositeImage(Display *,XResourceInfo *,XWindows *,Image *,
    ExceptionInfo *),
  XConfigureImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *),
  XDrawEditImage(Display *,XResourceInfo *,XWindows *,Image **,
    ExceptionInfo *),
  XMatteEditImage(Display *,XResourceInfo *,XWindows *,Image **,
    ExceptionInfo *),
  XPasteImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *),
  XPrintImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *),
  XRotateImage(Display *,XResourceInfo *,XWindows *,double,Image **,
    ExceptionInfo *),
  XROIImage(Display *,XResourceInfo *,XWindows *,Image **,ExceptionInfo *),
  XSaveImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *),
  XTrimImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *);
static void
  XDrawPanRectangle(Display *,XWindows *),
  XImageCache(Display *,XResourceInfo *,XWindows *,const DisplayCommand,Image **,
    ExceptionInfo *),
  XMagnifyImage(Display *,XWindows *,XEvent *,ExceptionInfo *),
  XMakePanImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *),
  XPanImage(Display *,XWindows *,XEvent *,ExceptionInfo *),
  XMagnifyWindowCommand(Display *,XWindows *,const MagickStatusType,
    const KeySym,ExceptionInfo *),
  XSetCropGeometry(Display *,XWindows *,RectangleInfo *,Image *),
  XScreenEvent(Display *,XWindows *,XEvent *,ExceptionInfo *),
  XTranslateImage(Display *,XWindows *,Image *,const KeySym);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X A n n o t a t e E d i t I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XAnnotateEditImage() annotates the image with text.
%
%  The format of the XAnnotateEditImage method is:
%
%      MagickBooleanType XAnnotateEditImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
*/

static inline MagickBooleanType IsSelectionUnusable(const int status,
  const Atom type,const int format,const unsigned long length)
{
  return(((status != Success) || (type != XA_STRING) || (format == 32) ||
    (length == 0)) ? MagickTrue : MagickFalse);
}

typedef struct _AnnotateTargets
{
  XAnnotateInfo
    **annotate_info;

  char
    **p;

  int
    *x,
    *y;

  XEvent
    *text_event;

  size_t
    *state;

  XEvent
    *event;
} AnnotateTargets;

static void XGoToPreviousTextLine(const AnnotateTargets *targets)
{
  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  char
    **const p = targets->p;

  int
    *const x = targets->x,
    *const y = targets->y;

  /*
    Go to end of the previous line of text.
  */
  (*annotate_info)=(*annotate_info)->previous;
  (*p)=(*annotate_info)->text;
  (*x)=(*annotate_info)->x+(int) (*annotate_info)->width;
  (*y)=(*annotate_info)->y;
  if ((*annotate_info)->width != 0)
    (*p)+=(ptrdiff_t) strlen((*annotate_info)->text);
}

typedef struct _AnnotateMenuInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *AnnotateMenu;

  int
    id;

  char
    *command;
} AnnotateMenuInput;

typedef struct _AnnotateFontTargets
{
  unsigned int
    *font_id;
} AnnotateFontTargets;

static void XSelectAnnotateFont(const AnnotateMenuInput *menu,
  const AnnotateFontTargets *targets)
{
  Display
    *const display = menu->display;

  XResourceInfo
    *const resource_info = menu->resource_info;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const AnnotateMenu = menu->AnnotateMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  unsigned int
    *const font_id = targets->font_id;

  const char
    *FontMenu[MaxNumberFonts];

  int
    font_number;

  ssize_t
    i;

  XFontStruct
    *font_info;

  /*
    Initialize menu selections.
  */
  for (i=0; i < MaxNumberFonts; i++)
    FontMenu[i]=resource_info->font_name[i];
  FontMenu[MaxNumberFonts-2]="Browser...";
  FontMenu[MaxNumberFonts-1]=(const char *) NULL;
  /*
    Select a font name from the pop-up menu.
  */
  font_number=XMenuWidget(display,windows,AnnotateMenu[id],
    (const char **) FontMenu,command);
  if (font_number < 0)
    return;
  if (font_number == (MaxNumberFonts-2))
    {
      static char
        font_name[MagickPathExtent] = "fixed";

      /*
        Select a font name from a browser.
      */
      resource_info->font_name[font_number]=font_name;
      XFontBrowserWidget(display,windows,"Select",font_name);
      if (*font_name == '\0')
        return;
    }
  /*
    Initialize font info.
  */
  font_info=XLoadQueryFont(display,resource_info->font_name[
    font_number]);
  if (font_info == (XFontStruct *) NULL)
    {
      XNoticeWidget(display,windows,"Unable to load font:",
        resource_info->font_name[font_number]);
      return;
    }
  (*font_id)=(unsigned int) font_number;
  (void) XFreeFont(display,font_info);
}

typedef struct _AnnotateFontColorTargets
{
  MagickBooleanType
    *transparent_pen;

  XColor
    *color;

  unsigned int
    *pen_id;
} AnnotateFontColorTargets;

static void XSelectAnnotateFontColor(const AnnotateMenuInput *menu,
  const AnnotateFontColorTargets *targets)
{
  Display
    *const display = menu->display;

  XResourceInfo
    *const resource_info = menu->resource_info;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const AnnotateMenu = menu->AnnotateMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  MagickBooleanType
    *const transparent_pen = targets->transparent_pen;

  XColor
    *const color = targets->color;

  unsigned int
    *const pen_id = targets->pen_id;

  const char
    *ColorMenu[MaxNumberPens+1];

  int
    pen_number;

  ssize_t
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="transparent";
  ColorMenu[MaxNumberPens-1]="Browser...";
  ColorMenu[MaxNumberPens]=(const char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  pen_number=XMenuWidget(display,windows,AnnotateMenu[id],
    (const char **) ColorMenu,command);
  if (pen_number < 0)
    return;
  (*transparent_pen)=pen_number == (MaxNumberPens-2) ? MagickTrue :
    MagickFalse;
  if ((*transparent_pen) != MagickFalse)
    return;
  if (pen_number == (MaxNumberPens-1))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set pen color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],color);
  XBestPixel(display,windows->map_info->colormap,(XColor *) NULL,
    (unsigned int) MaxColors,color);
  windows->pixel_info->pen_colors[pen_number]=(*color);
  (*pen_id)=(unsigned int) pen_number;
}

typedef struct _AnnotateBoxColorTargets
{
  MagickBooleanType
    *transparent_box;

  XColor
    *color;

  unsigned int
    *box_id;
} AnnotateBoxColorTargets;

static void XSelectAnnotateBoxColor(const AnnotateMenuInput *menu,
  const AnnotateBoxColorTargets *targets)
{
  Display
    *const display = menu->display;

  XResourceInfo
    *const resource_info = menu->resource_info;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const AnnotateMenu = menu->AnnotateMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  MagickBooleanType
    *const transparent_box = targets->transparent_box;

  XColor
    *const color = targets->color;

  unsigned int
    *const box_id = targets->box_id;

  const char
    *ColorMenu[MaxNumberPens+1];

  int
    pen_number;

  ssize_t
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="transparent";
  ColorMenu[MaxNumberPens-1]="Browser...";
  ColorMenu[MaxNumberPens]=(const char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  pen_number=XMenuWidget(display,windows,AnnotateMenu[id],
    (const char **) ColorMenu,command);
  if (pen_number < 0)
    return;
  (*transparent_box)=pen_number == (MaxNumberPens-2) ? MagickTrue :
    MagickFalse;
  if ((*transparent_box) != MagickFalse)
    return;
  if (pen_number == (MaxNumberPens-1))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set pen color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],color);
  XBestPixel(display,windows->map_info->colormap,(XColor *) NULL,
    (unsigned int) MaxColors,color);
  windows->pixel_info->pen_colors[pen_number]=(*color);
  (*box_id)=(unsigned int) pen_number;
}

typedef struct _AnnotateRotateTargets
{
  double
    *degrees;
} AnnotateRotateTargets;

static void XSelectAnnotateRotation(const AnnotateMenuInput *menu,
  const AnnotateRotateTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const AnnotateMenu = menu->AnnotateMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  double
    *const degrees = targets->degrees;

  int
    entry;

  const char
    *const RotateMenu[] =
    {
      "-90",
      "-45",
      "-30",
      "0",
      "30",
      "45",
      "90",
      "180",
      "Dialog...",
      (char *) NULL,
    };

  static char
    angle[MagickPathExtent] = "30.0";

  /*
    Select a command from the pop-up menu.
  */
  entry=XMenuWidget(display,windows,AnnotateMenu[id],RotateMenu,
    command);
  if (entry < 0)
    return;
  if (entry != 8)
    {
      (*degrees)=StringToDouble(RotateMenu[entry],(char **) NULL);
      return;
    }
  (void) XDialogWidget(display,windows,"OK","Enter rotation angle:",
    angle);
  if (*angle == '\0')
    return;
  (*degrees)=StringToDouble(angle,(char **) NULL);
}

static void XRedrawTextLines(Display *display,XWindows *windows,
  XAnnotateInfo *annotate_info,GC annotate_context)
{
  XAnnotateInfo
    *text_info;

  text_info=annotate_info;
  while (text_info != (XAnnotateInfo *) NULL)
  {
    if (annotate_info->stencil == ForegroundStencil)
      (void) XDrawString(display,windows->image.id,annotate_context,
        text_info->x,text_info->y,text_info->text,
        (int) strlen(text_info->text));
    else
      (void) XDrawImageString(display,windows->image.id,
        annotate_context,text_info->x,text_info->y,text_info->text,
        (int) strlen(text_info->text));
    text_info=text_info->previous;
  }
}

static void XEraseTextCharacter(Display *display,XWindows *windows,
  XFontStruct *font_info,const AnnotateTargets *targets)
{
  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  char
    **const p = targets->p;

  int
    *const x = targets->x,
    *const y = targets->y;

  XEvent
    *const text_event = targets->text_event;

  /*
    Erase one character.
  */
  if ((*p) == (*annotate_info)->text)
    {
      if ((*annotate_info)->previous == (XAnnotateInfo *) NULL)
        return;
      else
        {
          XGoToPreviousTextLine(targets);
          return;
        }
    }
  (*p)--;
  (*x)-=XTextWidth(font_info,(*p),1);
  (*text_event).xexpose.x=(*x);
  (*text_event).xexpose.y=(*y)-font_info->max_bounds.ascent;
  XRefreshWindow(display,&windows->image,text_event);
}

static void XEraseTextLine(Display *display,XWindows *windows,
  XFontStruct *font_info,const AnnotateTargets *targets)
{
  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  char
    **const p = targets->p;

  int
    *const x = targets->x;

  XEvent
    *const text_event = targets->text_event;

  /*
    Erase the entire line of text.
  */
  while ((*p) != (*annotate_info)->text)
  {
    (*p)--;
    (*x)-=XTextWidth(font_info,(*p),1);
    (*text_event).xexpose.x=(*x);
    XRefreshWindow(display,&windows->image,text_event);
  }
}

static MagickBooleanType XAdvanceTextLine(XWindows *windows,
  XFontStruct *font_info,const AnnotateTargets *targets)
{
  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  char
    **const p = targets->p;

  int
    *const x = targets->x,
    *const y = targets->y;

  /*
    Advance to the next line of text.
  */
  *(*p)='\0';
  (*annotate_info)->width=(unsigned int) XTextWidth(font_info,
    (*annotate_info)->text,(int) strlen((*annotate_info)->text));
  if ((*annotate_info)->next != (XAnnotateInfo *) NULL)
    {
      /*
        Line of text already exists.
      */
      (*annotate_info)=(*annotate_info)->next;
      (*x)=(*annotate_info)->x;
      (*y)=(*annotate_info)->y;
      (*p)=(*annotate_info)->text;
      return(MagickTrue);
    }
  (*annotate_info)->next=(XAnnotateInfo *) AcquireQuantumMemory(1,
    sizeof(*(*annotate_info)->next));
  if ((*annotate_info)->next == (XAnnotateInfo *) NULL)
    return(MagickFalse);
  *(*annotate_info)->next=(*(*annotate_info));
  (*annotate_info)->next->previous=(*annotate_info);
  (*annotate_info)=(*annotate_info)->next;
  (*annotate_info)->text=(char *) AcquireQuantumMemory((size_t) (
   (ssize_t)  windows->image.width/MagickMax((ssize_t)
   font_info->min_bounds.width,1)+2L),sizeof(*(*annotate_info)->text));
  if ((*annotate_info)->text == (char *) NULL)
    return(MagickFalse);
  (*annotate_info)->y+=(ssize_t) (*annotate_info)->height;
  if ((*annotate_info)->y > (int) windows->image.height)
    (*annotate_info)->y=(int) (*annotate_info)->height;
  (*annotate_info)->next=(XAnnotateInfo *) NULL;
  (*x)=(*annotate_info)->x;
  (*y)=(*annotate_info)->y;
  (*p)=(*annotate_info)->text;
  return(MagickTrue);
}

typedef struct _AnnotateTextInput
{
  Display
    *display;

  XWindows
    *windows;

  GC
    annotate_context;

  XFontStruct
    *font_info;

  char
    *command;

  size_t
    state;
} AnnotateTextInput;

static MagickBooleanType XInsertTextCharacter(
  const AnnotateTextInput *text_input,const AnnotateTargets *targets)
{
  Display
    *const display = text_input->display;

  XWindows
    *const windows = text_input->windows;

  const GC
    annotate_context = text_input->annotate_context;

  XFontStruct
    *const font_info = text_input->font_info;

  char
    *const command = text_input->command;

  const size_t
    state = text_input->state;

  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  char
    **const p = targets->p;

  int
    *const x = targets->x,
    *const y = targets->y;

  /*
    Draw a single character on the Image window.
  */
  if ((state & ModifierState) != 0)
    return(MagickTrue);
  if (*command == '\0')
    return(MagickTrue);
  *(*p)=(*command);
  if ((*annotate_info)->stencil == ForegroundStencil)
    (void) XDrawString(display,windows->image.id,annotate_context,
      (*x),(*y),(*p),1);
  else
    (void) XDrawImageString(display,windows->image.id,
      annotate_context,(*x),(*y),(*p),1);
  (*x)+=XTextWidth(font_info,(*p),1);
  (*p)++;
  if (((*x)+font_info->max_bounds.width) < (int) windows->image.width)
    return(MagickTrue);
  if (XAdvanceTextLine(windows,font_info,targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static KeySym XModifyKeySymbol(const size_t state,KeySym key_symbol)
{
  if ((state & ModifierState) != 0)
    switch ((int) key_symbol)
    {
      case XK_u:
      case XK_U:
      {
        key_symbol=DeleteCommand;
        break;
      }
      default:
        break;
    }
  return(key_symbol);
}

static MagickBooleanType XTypeTextKey(const AnnotateTextInput *text_input,
  const AnnotateTargets *targets,const KeySym key_symbol)
{
  XWindows
    *const windows = text_input->windows;

  XFontStruct
    *const font_info = text_input->font_info;

  switch ((int) key_symbol)
  {
    default:
    {
      if (XInsertTextCharacter(text_input,targets) == MagickFalse)
        return(MagickFalse);
      break;
    }
    case XK_Return:
    case XK_KP_Enter:
    {
      if (XAdvanceTextLine(windows,font_info,targets) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType XAnnotateKeyPress(
  const AnnotateTextInput *text_input,const AnnotateTargets *targets,
  KeySym key_symbol)
{
  Display
    *const display = text_input->display;

  XWindows
    *const windows = text_input->windows;

  XFontStruct
    *const font_info = text_input->font_info;

  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  XEvent
    *const text_event = targets->text_event;

  size_t
    *const state = targets->state;

  switch ((int) key_symbol)
  {
    case XK_BackSpace:
    {
      XEraseTextCharacter(display,windows,font_info,targets);
      break;
    }
    case XK_bracketleft:
    {
      key_symbol=XK_Escape;
      break;
    }
    case DeleteCommand:
    {
      XEraseTextLine(display,windows,font_info,targets);
      break;
    }
    case XK_Escape:
    case XK_F20:
    {
      /*
        Finished annotating.
      */
      (*annotate_info)->width=(unsigned int) XTextWidth(font_info,
        (*annotate_info)->text,(int) strlen((*annotate_info)->text));
      XRefreshWindow(display,&windows->image,text_event);
      (*state)|=ExitState;
      break;
    }
    default:
    {
      if (XTypeTextKey(text_input,targets,key_symbol) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType XPasteSelectionCharacter(
  const AnnotateTextInput *text_input,const AnnotateTargets *targets,
  unsigned char *data,const ssize_t i)
{
  Display
    *const display = text_input->display;

  XWindows
    *const windows = text_input->windows;

  const GC
    annotate_context = text_input->annotate_context;

  XFontStruct
    *const font_info = text_input->font_info;

  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  char
    **const p = targets->p;

  int
    *const x = targets->x,
    *const y = targets->y;

  if ((char) data[i] != '\n')
    {
      /*
        Draw a single character on the Image window.
      */
      *(*p)=(char) data[i];
      (void) XDrawString(display,windows->image.id,annotate_context,
        (*x),(*y),(*p),1);
      (*x)+=XTextWidth(font_info,(*p),1);
      (*p)++;
      if (((*x)+font_info->max_bounds.width) < (int) windows->image.width)
        return(MagickTrue);
    }
  /*
    Advance to the next line of text.
  */
  *(*p)='\0';
  (*annotate_info)->width=(unsigned int) XTextWidth(font_info,
    (*annotate_info)->text,(int) strlen((*annotate_info)->text));
  if ((*annotate_info)->next != (XAnnotateInfo *) NULL)
    {
      /*
        Line of text already exists.
      */
      (*annotate_info)=(*annotate_info)->next;
      (*x)=(*annotate_info)->x;
      (*y)=(*annotate_info)->y;
      (*p)=(*annotate_info)->text;
      return(MagickTrue);
    }
  (*annotate_info)->next=(XAnnotateInfo *) AcquireQuantumMemory(1,
    sizeof(*(*annotate_info)->next));
  if ((*annotate_info)->next == (XAnnotateInfo *) NULL)
    return(MagickFalse);
  *(*annotate_info)->next=(*(*annotate_info));
  (*annotate_info)->next->previous=(*annotate_info);
  (*annotate_info)=(*annotate_info)->next;
  (*annotate_info)->text=(char *) AcquireQuantumMemory((size_t)
    (windows->image.width/MagickMax((ssize_t)
    font_info->min_bounds.width,1)+2L),sizeof(*(*annotate_info)->text));
  if ((*annotate_info)->text == (char *) NULL)
    return(MagickFalse);
  (*annotate_info)->y+=(ssize_t) (*annotate_info)->height;
  if ((*annotate_info)->y > (int) windows->image.height)
    (*annotate_info)->y=(int) (*annotate_info)->height;
  (*annotate_info)->next=(XAnnotateInfo *) NULL;
  (*x)=(*annotate_info)->x;
  (*y)=(*annotate_info)->y;
  (*p)=(*annotate_info)->text;
  return(MagickTrue);
}

static MagickBooleanType XPasteSelectionText(
  const AnnotateTextInput *text_input,const XEvent *event,
  const AnnotateTargets *targets)
{
  Display
    *const display = text_input->display;

  Atom
    type;

  int
    format;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  int
    status;

  ssize_t
    i;

  /*
    Obtain response from primary selection.
  */
  if ((*event).xselection.property == (Atom) None)
    return(MagickTrue);
  status=XGetWindowProperty(display,(*event).xselection.requestor,
    (*event).xselection.property,0L,(long) MagickPathExtent,True,XA_STRING,
    &type,&format,&length,&after,&data);
  if (IsSelectionUnusable(status,type,format,length) != MagickFalse)
    return(MagickTrue);
  /*
    Annotate Image window with primary selection.
  */
  for (i=0; i < (ssize_t) length; i++)
  {
    if (XPasteSelectionCharacter(text_input,targets,data,i) ==
        MagickFalse)
      return(MagickFalse);
  }
  (void) XFree((void *) data);
  return(MagickTrue);
}

static void XSetBoxPixelIndex(XWindows *windows,const unsigned int box_id)
{
  ssize_t
    i;

  windows->pixel_info->box_color=windows->pixel_info->pen_colors[box_id];
  if (windows->pixel_info->colors != 0)
    for (i=0; i < (ssize_t) windows->pixel_info->colors; i++)
      if (windows->pixel_info->pixels[i] ==
          windows->pixel_info->pen_colors[box_id].pixel)
        {
          windows->pixel_info->box_index=(unsigned short) i;
          break;
        }
}

static void XSetPenPixelIndex(XWindows *windows,const unsigned int pen_id)
{
  ssize_t
    i;

  windows->pixel_info->pen_color=windows->pixel_info->pen_colors[pen_id];
  if (windows->pixel_info->colors != 0)
    for (i=0; i < (ssize_t) windows->pixel_info->colors; i++)
      if (windows->pixel_info->pixels[i] ==
          windows->pixel_info->pen_colors[pen_id].pixel)
        {
          windows->pixel_info->pen_index=(unsigned short) i;
          break;
        }
}

static void
  XTrackInfoWidget(Display *,XWindows *,const int,const int);

static const char
  *const AnnotateMenu[] =
  {
    "Font Name",
    "Font Color",
    "Box Color",
    "Rotate Text",
    "Help",
    "Dismiss",
    (char *) NULL
  },
  *const TextMenu[] =
  {
    "Help",
    "Apply",
    (char *) NULL
  };

static const ModeType
  AnnotateCommands[] =
  {
    AnnotateNameCommand,
    AnnotateFontColorCommand,
    AnnotateBackgroundColorCommand,
    AnnotateRotateCommand,
    AnnotateHelpCommand,
    AnnotateDismissCommand
  },
  TextCommands[] =
  {
    TextHelpCommand,
    TextApplyCommand
  };

typedef struct _AnnotateContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  char
    *command;

  const AnnotateFontTargets
    *font_targets;

  const AnnotateFontColorTargets
    *font_color_targets;

  const AnnotateBoxColorTargets
    *box_color_targets;

  const AnnotateRotateTargets
    *rotate_targets;

  Cursor
    cursor;

  ExceptionInfo
    *exception;

  char
    *text;

  GC
    annotate_context;

  XFontStruct
    *font_info;

  unsigned int
    box_id;

  unsigned int
    pen_id;
} AnnotateContext;

static void XSetAnnotateMenuInput(const AnnotateContext *context,const int id,
  AnnotateMenuInput *annotate_input)
{
  (*annotate_input).display=context->display;
  (*annotate_input).resource_info=context->resource_info;
  (*annotate_input).windows=context->windows;
  (*annotate_input).AnnotateMenu=AnnotateMenu;
  (*annotate_input).id=id;
  (*annotate_input).command=context->command;
}

static void XAnnotatePointerCommand(const AnnotateContext *context,
  const AnnotateTargets *targets,const int id)
{
  size_t
    *const state = targets->state;

  switch (AnnotateCommands[id])
  {
    case AnnotateNameCommand:
    {
      AnnotateMenuInput
        annotate_input;

      XSetAnnotateMenuInput(context,id,&annotate_input);
      XSelectAnnotateFont(&annotate_input,context->font_targets);
      break;
    }
    case AnnotateFontColorCommand:
    {
      AnnotateMenuInput
        annotate_input;

      XSetAnnotateMenuInput(context,id,&annotate_input);
      XSelectAnnotateFontColor(&annotate_input,context->font_color_targets);
      break;
    }
    case AnnotateBackgroundColorCommand:
    {
      AnnotateMenuInput
        annotate_input;

      XSetAnnotateMenuInput(context,id,&annotate_input);
      XSelectAnnotateBoxColor(&annotate_input,context->box_color_targets);
      break;
    }
    case AnnotateRotateCommand:
    {
      AnnotateMenuInput
        annotate_input;

      XSetAnnotateMenuInput(context,id,&annotate_input);
      XSelectAnnotateRotation(&annotate_input,context->rotate_targets);
      break;
    }
    case AnnotateHelpCommand:
    {
      XTextViewHelp(context->display,context->resource_info,context->windows,
        MagickFalse,
        "Help Viewer - Image Annotation",ImageAnnotateHelp);
      break;
    }
    case AnnotateDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static void XAnnotatePointerPress(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != context->windows->image.id)
    return;
  /*
    Change to text entering mode.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  (*state)|=ExitState;
}

static void XAnnotatePointerKey(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  if ((*event).xkey.window != context->windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XTextViewHelp(context->display,context->resource_info,context->windows,
        MagickFalse,
        "Help Viewer - Image Annotation",ImageAnnotateHelp);
      break;
    }
    default:
    {
      (void) XBell(context->display,0);
      break;
    }
  }
}

static void XAnnotatePointerEvent(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XAnnotatePointerPress(context,targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      XAnnotatePointerKey(context,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(context->display,context->windows,(*x),(*y));
      break;
    }
    default:
      break;
  }
}

static void XAnnotateCommandBranch(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  int
    id;

  XEvent
    *const event = targets->event;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(context->display,context->windows,AnnotateMenu,event);
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
  if (id < 0)
    return;
  XAnnotatePointerCommand(context,targets,id);
}

static void XAnnotateTrackPointer(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  do
  {
    if (context->windows->info.mapped != MagickFalse)
      {
        /*
          Display pointer position.
        */
        (void) FormatLocaleString(context->text,MagickPathExtent," %+d%+d ",
          (*x)+context->windows->image.x,(*y)+context->windows->image.y);
        XInfoWidget(context->display,context->windows,context->text);
      }
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XAnnotateCommandBranch(context,targets);
        continue;
      }
    XAnnotatePointerEvent(context,targets);
  } while (((*state) & ExitState) == 0);
}

static void XAnnotateTextCommand(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  int
    id;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  XEvent
    *const text_event = targets->text_event;

  /*
    Select a command from the Command widget.
  */
  (void) XSetBackground(context->display,context->annotate_context,
    context->windows->pixel_info->background_color.pixel);
  (void) XSetForeground(context->display,context->annotate_context,
    context->windows->pixel_info->foreground_color.pixel);
  id=XCommandWidget(context->display,context->windows,AnnotateMenu,event);
  (void) XSetBackground(context->display,context->annotate_context,
    context->windows->pixel_info->pen_colors[context->box_id].pixel);
  (void) XSetForeground(context->display,context->annotate_context,
    context->windows->pixel_info->pen_colors[context->pen_id].pixel);
  if (id < 0)
    return;
  switch (TextCommands[id])
  {
    case TextHelpCommand:
    {
      XTextViewHelp(context->display,context->resource_info,context->windows,
        MagickFalse,
        "Help Viewer - Image Annotation",ImageAnnotateHelp);
      (void) XCheckDefineCursor(context->display,context->windows->image.id,
        context->cursor);
      break;
    }
    case TextApplyCommand:
    {
      /*
        Finished annotating.
      */
      (*annotate_info)->width=(unsigned int) XTextWidth(context->font_info,
        (*annotate_info)->text,(int) strlen((*annotate_info)->text));
      XRefreshWindow(context->display,&context->windows->image,text_event);
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static MagickBooleanType XAnnotateTextKey(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  AnnotateTextInput
    text_input;

  char
    command[MagickPathExtent];

  int
    length;

  KeySym
    key_symbol;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  /*
    Respond to a user key press.
  */
  length=XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if ((((*event).xkey.state & ControlMask) != 0) ||
      (((*event).xkey.state & Mod1Mask) != 0))
    (*state)|=ModifierState;
  key_symbol=XModifyKeySymbol((*state),key_symbol);
  text_input.display=context->display;
  text_input.windows=context->windows;
  text_input.annotate_context=context->annotate_context;
  text_input.font_info=context->font_info;
  text_input.command=command;
  text_input.state=(*state);
  if (XAnnotateKeyPress(&text_input,targets,key_symbol) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType XAnnotateTextPaste(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  AnnotateTextInput
    text_input;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  text_input.display=context->display;
  text_input.windows=context->windows;
  text_input.annotate_context=context->annotate_context;
  text_input.font_info=context->font_info;
  text_input.command=context->command;
  text_input.state=(*state);
  if (XPasteSelectionText(&text_input,event,targets) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void XAnnotateTextPress(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  XEvent
    *const event = targets->event;

  if ((*event).xbutton.window != context->windows->image.id)
    return;
  if ((*event).xbutton.button == Button2)
    {
      /*
        Request primary selection.
      */
      (void) XConvertSelection(context->display,XA_PRIMARY,XA_STRING,XA_STRING,
        context->windows->image.id,CurrentTime);
      return;
    }
}

static MagickBooleanType XAnnotateEditImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  static MagickBooleanType
    transparent_box = MagickTrue,
    transparent_pen = MagickFalse;

  static double
    degrees = 0.0;

  static unsigned int
    box_id = MaxNumberPens-2,
    font_id = 0,
    pen_id = 0;

  char
    command[MagickPathExtent],
    *p,
    text[MagickPathExtent];

  Cursor
    cursor;

  GC
    annotate_context;

  int
    status,
    x,
    y;

  KeySym
    key_symbol;

  size_t
    state;

  unsigned int
    height,
    width;

  XAnnotateInfo
    *annotate_info,
    *previous_info;

  XColor
    color;

  XFontStruct
    *font_info;

  XEvent
    event,
    text_event;

  AnnotateContext
    context;

  const AnnotateTargets
    targets = { &annotate_info, &p, &x, &y, &text_event, &state, &event };

  const AnnotateRotateTargets
    rotate_targets = { &degrees };

  const AnnotateBoxColorTargets
    box_color_targets = { &transparent_box, &color, &box_id };

  const AnnotateFontColorTargets
    font_color_targets = { &transparent_pen, &color, &pen_id };

  const AnnotateFontTargets
    font_targets = { &font_id };

  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Annotate");
  windows->command.data=4;
  (void) XCommandWidget(display,windows,AnnotateMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  cursor=XCreateFontCursor(display,XC_left_side);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  state=DefaultState;
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.command=command;
  context.font_targets=&font_targets;
  context.font_color_targets=&font_color_targets;
  context.box_color_targets=&box_color_targets;
  context.rotate_targets=&rotate_targets;
  context.exception=exception;
  context.text=text;
  context.cursor=cursor;
  XAnnotateTrackPointer(&context,&targets);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  /*
    Set font info and check boundary conditions.
  */
  font_info=XLoadQueryFont(display,resource_info->font_name[font_id]);
  if (font_info == (XFontStruct *) NULL)
    {
      XNoticeWidget(display,windows,"Unable to load font:",
        resource_info->font_name[font_id]);
      font_info=windows->font_info;
    }
  if ((x+font_info->max_bounds.width) >= (int) windows->image.width)
    x=(int) windows->image.width-font_info->max_bounds.width;
  if (y < (int) (font_info->ascent+font_info->descent))
    y=(int) font_info->ascent+font_info->descent;
  if (((int) font_info->max_bounds.width > (int) windows->image.width) ||
      ((font_info->ascent+font_info->descent) >= (int) windows->image.height))
    return(MagickFalse);
  /*
    Initialize annotate structure.
  */
  annotate_info=(XAnnotateInfo *) AcquireMagickMemory(sizeof(*annotate_info));
  if (annotate_info == (XAnnotateInfo *) NULL)
    return(MagickFalse);
  XGetAnnotateInfo(annotate_info);
  annotate_info->x=x;
  annotate_info->y=y;
  if ((transparent_box == MagickFalse) && (transparent_pen == MagickFalse))
    annotate_info->stencil=OpaqueStencil;
  else
    if (transparent_box == MagickFalse)
      annotate_info->stencil=BackgroundStencil;
    else
      annotate_info->stencil=ForegroundStencil;
  annotate_info->height=(unsigned int) (font_info->ascent+font_info->descent);
  annotate_info->degrees=degrees;
  annotate_info->font_info=font_info;
  annotate_info->text=(char *) AcquireQuantumMemory((size_t) 
    windows->image.width/(size_t) MagickMax(font_info->min_bounds.width,1)+2UL,
    sizeof(*annotate_info->text));
  if (annotate_info->text == (char *) NULL)
    return(MagickFalse);
  /*
    Create cursor and set graphic context.
  */
  cursor=XCreateFontCursor(display,XC_pencil);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  annotate_context=windows->image.annotate_context;
  (void) XSetFont(display,annotate_context,font_info->fid);
  (void) XSetBackground(display,annotate_context,
    windows->pixel_info->pen_colors[box_id].pixel);
  (void) XSetForeground(display,annotate_context,
    windows->pixel_info->pen_colors[pen_id].pixel);
  /*
    Begin annotating the image with text.
  */
  (void) CloneString(&windows->command.name,"Text");
  windows->command.data=0;
  (void) XCommandWidget(display,windows,TextMenu,(XEvent *) NULL);
  state=DefaultState;
  (void) XDrawString(display,windows->image.id,annotate_context,x,y,"_",1);
  text_event.xexpose.width=(int) font_info->max_bounds.width;
  text_event.xexpose.height=font_info->max_bounds.ascent+
    font_info->max_bounds.descent;
  p=annotate_info->text;
  context.cursor=cursor;
  context.annotate_context=annotate_context;
  context.font_info=font_info;
  context.box_id=box_id;
  context.pen_id=pen_id;
  do
  {
    /*
      Display text cursor.
    */
    *p='\0';
    (void) XDrawString(display,windows->image.id,annotate_context,x,y,"_",1);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,&event,exception);
    if (event.xany.window == windows->command.id)
      {
        XAnnotateTextCommand(&context,&targets);
        continue;
      }
    /*
      Erase text cursor.
    */
    text_event.xexpose.x=x;
    text_event.xexpose.y=y-font_info->max_bounds.ascent;
    (void) XClearArea(display,windows->image.id,x,text_event.xexpose.y,
      (unsigned int) text_event.xexpose.width,(unsigned int)
      text_event.xexpose.height,MagickFalse);
    XRefreshWindow(display,&windows->image,&text_event);
    switch (event.type)
    {
      case ButtonPress:
      {
        XAnnotateTextPress(&context,&targets);
        break;
      }
      case Expose:
      {
        if (event.xexpose.count == 0)
          {
            /*
              Refresh Image window.
            */
            XRefreshWindow(display,&windows->image,(XEvent *) NULL);
            XRedrawTextLines(display,windows,annotate_info,annotate_context);
            (void) XDrawString(display,windows->image.id,annotate_context,
              x,y,"_",1);
          }
        break;
      }
      case KeyPress:
      {
        if (event.xkey.window != windows->image.id)
          break;
        if (XAnnotateTextKey(&context,&targets) == MagickFalse)
          return(MagickFalse);
        break;
      }
      case KeyRelease:
      {
        /*
          Respond to a user key release.
        */
        (void) XLookupString((XKeyEvent *) &event.xkey,command,(int)
          sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        state&=(size_t) (~ModifierState);
        break;
      }
      case SelectionNotify:
      {
        if (XAnnotateTextPaste(&context,&targets) == MagickFalse)
          return(MagickFalse);
        break;
      }
      default:
        break;
    }
  } while ((state & ExitState) == 0);
  (void) XFreeCursor(display,cursor);
  /*
    Annotation is relative to image configuration.
  */
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  /*
    Initialize annotated image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  while (annotate_info != (XAnnotateInfo *) NULL)
  {
    if (annotate_info->width == 0)
      {
        /*
          No text on this line--  go to the next line of text.
        */
        previous_info=annotate_info->previous;
        annotate_info->text=(char *)
          RelinquishMagickMemory(annotate_info->text);
        annotate_info=(XAnnotateInfo *) RelinquishMagickMemory(annotate_info);
        annotate_info=previous_info;
        continue;
      }
    /*
      Determine pixel index for box and pen color.
    */
    XSetBoxPixelIndex(windows,box_id);
    XSetPenPixelIndex(windows,pen_id);
    /*
      Define the annotate geometry string.
    */
    annotate_info->x=(int)
      width*(annotate_info->x+windows->image.x)/windows->image.ximage->width;
    annotate_info->y=(int) height*(annotate_info->y-font_info->ascent+
      windows->image.y)/windows->image.ximage->height;
    (void) FormatLocaleString(annotate_info->geometry,MagickPathExtent,
      "%gx%g%+g%+g",(double) width*annotate_info->width/
      windows->image.ximage->width,(double) height*annotate_info->height/
      windows->image.ximage->height,(double) annotate_info->x+x,(double)
      annotate_info->y+y);
    /*
      Annotate image with text.
    */
    status=XAnnotateImage(display,windows->pixel_info,annotate_info,image,
      exception) == MagickFalse ? 0 : 1;
    if (status == 0)
      return(MagickFalse);
    /*
      Free up memory.
    */
    previous_info=annotate_info->previous;
    annotate_info->text=DestroyString(annotate_info->text);
    annotate_info=(XAnnotateInfo *) RelinquishMagickMemory(annotate_info);
    annotate_info=previous_info;
  }
  (void) XSetForeground(display,annotate_context,
    windows->pixel_info->foreground_color.pixel);
  (void) XSetBackground(display,annotate_context,
    windows->pixel_info->background_color.pixel);
  (void) XSetFont(display,annotate_context,windows->font_info->fid);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeFont(display,font_info);
  /*
    Update image configuration.
  */
  XConfigureImageColormap(display,resource_info,windows,image,exception);
  (void) XConfigureImage(display,resource_info,windows,image,exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X B a c k g r o u n d I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XBackgroundImage() displays the image in the background of a window.
%
%  The format of the XBackgroundImage method is:
%
%      MagickBooleanType XBackgroundImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType XBackgroundImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
#define BackgroundImageTag  "Background/Image"

  int
    status;

  static char
    window_id[MagickPathExtent] = "root";

  XResourceInfo
    background_resources;

  /*
    Put image in background.
  */
  status=XDialogWidget(display,windows,"Background",
    "Enter window id (id 0x00 selects window with pointer):",window_id);
  if (*window_id == '\0')
    return(MagickFalse);
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XInfoWidget(display,windows,BackgroundImageTag);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  background_resources=(*resource_info);
  background_resources.window_id=window_id;
  background_resources.backdrop=status != 0 ? MagickTrue : MagickFalse;
  status=XDisplayBackgroundImage(display,&background_resources,*image,
    exception) == MagickFalse ? 0 : 1;
  if (status != MagickFalse)
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_retain_colors,CurrentTime);
  XSetCursorState(display,windows,MagickFalse);
  (void) XMagickCommand(display,resource_info,windows,UndoCommand,image,
    exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C h o p I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XChopImage() chops the X image.
%
%  The format of the XChopImage method is:
%
%    MagickBooleanType XChopImage(Display *display,XResourceInfo *resource_info,
%      XWindows *windows,Image **image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static ModeType XSelectChopDirection(const int id,
  const ModeType *DirectionCommands,ModeType direction)
{
  if (id >= 0)
    direction=DirectionCommands[id];
  return(direction);
}

static void XTrackInfoWidget(Display *display,XWindows *windows,const int x,
  const int y)
{
  if (windows->info.mapped != MagickFalse)
    {
      if ((x < (windows->info.x+(int) windows->info.width)) &&
          (y < (windows->info.y+(int) windows->info.height)))
        (void) XWithdrawWindow(display,windows->info.id,
          windows->info.screen);
    }
  else
    if ((x > (windows->info.x+(int) windows->info.width)) ||
        (y > (windows->info.y+(int) windows->info.height)))
      (void) XMapWindow(display,windows->info.id);
}

static void XClampChopSegment(const XWindows *windows,XSegment *segment_info)
{
  /*
    Check boundary conditions.
  */
  if ((*segment_info).x2 < 0)
    (*segment_info).x2=0;
  else
    if ((*segment_info).x2 > windows->image.ximage->width)
      (*segment_info).x2=windows->image.ximage->width;
  if ((*segment_info).y2 < 0)
    (*segment_info).y2=0;
  else
    if ((*segment_info).y2 > windows->image.ximage->height)
      (*segment_info).y2=windows->image.ximage->height;
}

static void XChopHorizontalGeometry(const XWindows *windows,
  const XSegment *segment_info,RectangleInfo *chop_info)
{
  (*chop_info).width=(size_t) ((*segment_info).x2-(*segment_info).x1+1);
  (*chop_info).height=0;
  (*chop_info).x=(ssize_t) windows->image.x+(*segment_info).x1;
  (*chop_info).y=0;
  if ((*segment_info).x1 > (*segment_info).x2)
    {
      (*chop_info).width=(size_t) ((*segment_info).x1-(*segment_info).x2+1);
      (*chop_info).x=(ssize_t) windows->image.x+(*segment_info).x2;
    }
}

static void XChopVerticalGeometry(const XWindows *windows,
  const XSegment *segment_info,RectangleInfo *chop_info)
{
  (*chop_info).width=0;
  (*chop_info).height=(size_t) ((*segment_info).y2-(*segment_info).y1+1);
  (*chop_info).x=0;
  (*chop_info).y=(ssize_t) windows->image.y+(*segment_info).y1;
  if ((*segment_info).y1 > (*segment_info).y2)
    {
      (*chop_info).height=(size_t) ((*segment_info).y1-(*segment_info).y2+1);
      (*chop_info).y=(ssize_t) windows->image.y+(*segment_info).y2;
    }
}

static void XScaleChopInfo(const XWindows *windows,Image **image,
  RectangleInfo *chop_info)
{
  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  width=(unsigned int) (*image)->columns;
  height=(unsigned int) (*image)->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  (*chop_info).x+=x;
  (*chop_info).x=(ssize_t) (scale_factor*(*chop_info).x+0.5);
  (*chop_info).width=(unsigned int) (scale_factor*(*chop_info).width+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  (*chop_info).y+=y;
  (*chop_info).y=(ssize_t) (scale_factor*(*chop_info).y+0.5);
  (*chop_info).height=(unsigned int) (scale_factor*(*chop_info).height+0.5);
}

typedef struct _ChopEventInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *ChopMenu;

  const ModeType
    *ChopCommands,
    *DirectionCommands;

  ExceptionInfo
    *exception;
} ChopEventInput;

static size_t XChopKeyPress(const ChopEventInput *input,XEvent *event,
  size_t state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  if ((*event).xkey.window != windows->image.id)
    return(state);
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      state|=EscapeState;
      state|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Chop",ImageChopHelp);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
  return(state);
}

typedef struct _ChopTargets
{
  XSegment
    *segment_info;

  size_t
    *state;

  ModeType
    *direction;
} ChopTargets;

static void XChopStartPress(const ChopEventInput *input,const XEvent *event,
  const ChopTargets *targets)
{
  XWindows
    *const windows = input->windows;

  XSegment
    *const segment_info = targets->segment_info;

  size_t
    *const state = targets->state;

  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  /*
    User has committed to start point of chopping line.
  */
  (*segment_info).x1=(short int) (*event).xbutton.x;
  (*segment_info).x2=(short int) (*event).xbutton.x;
  (*segment_info).y1=(short int) (*event).xbutton.y;
  (*segment_info).y2=(short int) (*event).xbutton.y;
  (*state)|=ExitState;
}

static void XShowChopPointer(Display *display,XWindows *windows,const int x,
  const int y)
{
  char
    text[MagickPathExtent];

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+d%+d ",
        x+windows->image.x,y+windows->image.y);
      XInfoWidget(display,windows,text);
    }
}

static MagickBooleanType XChopCommand(const ChopEventInput *input,XEvent *event,
  ModeType *direction,size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  const char
    *const *const ChopMenu = input->ChopMenu;

  const ModeType
    *const ChopCommands = input->ChopCommands,
    *const DirectionCommands = input->DirectionCommands;

  int
    id;

  if ((*event).xany.window == windows->command.id)
    {
      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,ChopMenu,event);
      if (id < 0)
        return(MagickTrue);
      switch (ChopCommands[id])
      {
        case ChopDirectionCommand:
        {
          char
            command[MagickPathExtent];

          const char
            *const Directions[] =
            {
              "horizontal",
              "vertical",
              (char *) NULL,
            };

          /*
            Select a command from the pop-up menu.
          */
          id=XMenuWidget(display,windows,ChopMenu[id],Directions,command);
          (*direction)=XSelectChopDirection(id,DirectionCommands,(*direction));
          break;
        }
        case ChopHelpCommand:
        {
          XTextViewHelp(display,resource_info,windows,MagickFalse,
            "Help Viewer - Image Chop",ImageChopHelp);
          break;
        }
        case ChopDismissCommand:
        {
          /*
            Prematurely exit.
          */
          (*state)|=EscapeState;
          (*state)|=ExitState;
          break;
        }
        default:
          break;
      }
      return(MagickTrue);
    }
  return(MagickFalse);
}

static void XChopSelectStep(const ChopEventInput *input,
  const ChopTargets *targets,int *x,int *y)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  ModeType
    *const direction = targets->direction;

  size_t
    *const state = targets->state;

  XEvent
    event;

  XShowChopPointer(display,windows,(*x),(*y));
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  if (XChopCommand(input,&event,direction,state) != MagickFalse)
    return;
  switch (event.type)
  {
    case ButtonPress:
    {
      XChopStartPress(input,&event,targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      (*state)=XChopKeyPress(input,&event,(*state));
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      (*x)=event.xmotion.x;
      (*y)=event.xmotion.y;
      XTrackInfoWidget(display,windows,(*x),(*y));
    }
  }
}

static MagickBooleanType XChopApply(const ChopEventInput *input,Image **image,
  RectangleInfo chop_info)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  ExceptionInfo
    *const exception = input->exception;

  Image
    *chop_image;

  /*
    Image chopping is relative to image configuration.
  */
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  windows->image.window_changes.width=windows->image.ximage->width-
    (int) chop_info.width;
  windows->image.window_changes.height=windows->image.ximage->height-
    (int) chop_info.height;
  XScaleChopInfo(windows,image,&chop_info);
  /*
    Chop image.
  */
  chop_image=ChopImage(*image,&chop_info,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (chop_image == (Image *) NULL)
    return(MagickFalse);
  *image=DestroyImage(*image);
  *image=chop_image;
  /*
    Update image configuration.
  */
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
  return(MagickTrue);
}

static void XShowChopSelection(const ChopEventInput *input,
  const unsigned int distance,const RectangleInfo chop_info,
  const XSegment *segment_info)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  char
    text[MagickPathExtent];

  if (distance > 9)
    {
      /*
        Display info and draw chopping line.
      */
      if (windows->info.mapped == MagickFalse)
        (void) XMapWindow(display,windows->info.id);
      (void) FormatLocaleString(text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) chop_info.width,(double)
        chop_info.height,(double) chop_info.x,(double) chop_info.y);
      XInfoWidget(display,windows,text);
      XHighlightLine(display,windows->image.id,
        windows->image.highlight_context,segment_info);
    }
  else
    if (windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

static void XChopDragEvent(const XEvent *event,const ChopTargets *targets)
{
  XSegment
    *const segment_info = targets->segment_info;

  size_t
    *const state = targets->state;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      (*segment_info).x2=(short int) (*event).xmotion.x;
      (*segment_info).y2=(short int) (*event).xmotion.y;
      break;
    }
    case ButtonRelease:
    {
      /*
        User has committed to chopping line.
      */
      (*segment_info).x2=(short int) (*event).xbutton.x;
      (*segment_info).y2=(short int) (*event).xbutton.y;
      (*state)|=ExitState;
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      (*segment_info).x2=(short int) (*event).xmotion.x;
      (*segment_info).y2=(short int) (*event).xmotion.y;
    }
    default:
      break;
  }
}

static void XChopDragLine(const ChopEventInput *input,
  const ChopTargets *targets,RectangleInfo *chop_info,unsigned int *distance)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  XSegment
    *const segment_info = targets->segment_info;

  size_t
    *const state = targets->state;

  ModeType
    *const direction = targets->direction;

  XEvent
    event;

  (*chop_info).width=0;
  (*chop_info).height=0;
  (*chop_info).x=0;
  (*chop_info).y=0;
  (*distance)=0;
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  (*state)=DefaultState;
  do
  {
    XShowChopSelection(input,(*distance),(*chop_info),segment_info);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,&event,exception);
    if ((*distance) > 9)
      XHighlightLine(display,windows->image.id,
        windows->image.highlight_context,segment_info);
    XChopDragEvent(&event,targets);
    XClampChopSegment(windows,segment_info);
    (*distance)=(unsigned int)
      ((((*segment_info).x2-(*segment_info).x1)*
        ((*segment_info).x2-(*segment_info).x1))+
       (((*segment_info).y2-(*segment_info).y1)*
        ((*segment_info).y2-(*segment_info).y1)));
    /*
      Compute chopping geometry.
    */
    if ((*direction) == HorizontalChopCommand)
      XChopHorizontalGeometry(windows,segment_info,chop_info);
    else
      XChopVerticalGeometry(windows,segment_info,chop_info);
  } while (((*state) & ExitState) == 0);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

static MagickBooleanType XChopImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  const char
    *const ChopMenu[] =
    {
      "Direction",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static ModeType
    direction = HorizontalChopCommand;

  static const ModeType
    ChopCommands[] =
    {
      ChopDirectionCommand,
      ChopHelpCommand,
      ChopDismissCommand
    },
    DirectionCommands[] =
    {
      HorizontalChopCommand,
      VerticalChopCommand
    };

  int
    x,
    y;

  RectangleInfo
    chop_info;

  size_t
    state;

  unsigned int
    distance;

  XSegment
    segment_info;

  const ChopEventInput
    input = { display, resource_info, windows, ChopMenu, ChopCommands,
      DirectionCommands, exception };

  const ChopTargets
    targets = { &segment_info, &state, &direction };

  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Chop");
  windows->command.data=1;
  (void) XCommandWidget(display,windows,ChopMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  state=DefaultState;
  (void) memset(&segment_info,0,sizeof(segment_info));
  do
  {
    XChopSelectStep(&input,&targets,&x,&y);
  } while ((state & ExitState) == 0);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  /*
    Draw line as pointer moves until the mouse button is released.
  */
  XChopDragLine(&input,&targets,&chop_info,&distance);
  if (distance <= 9)
    return(MagickTrue);
  return(XChopApply(&input,image,chop_info));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C o l o r E d i t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XColorEditImage() allows the user to interactively change the color of one
%  pixel for a DirectColor image or one colormap entry for a PseudoClass image.
%
%  The format of the XColorEditImage method is:
%
%      MagickBooleanType XColorEditImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image **image,
%          ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XReplaceRowColor(Image **image,Quantum *q,const PixelInfo target,
  const XColor color)
{
  int
    x;

  PixelInfo
    pixel;

  for (x=0; x < (int) (*image)->columns; x++)
  {
    GetPixelInfoPixel(*image,q,&pixel);
    if (IsFuzzyEquivalencePixelInfo(&pixel,&target))
      {
        SetPixelRed(*image,ScaleShortToQuantum(
          color.red),q);
        SetPixelGreen(*image,ScaleShortToQuantum(
          color.green),q);
        SetPixelBlue(*image,ScaleShortToQuantum(
          color.blue),q);
      }
    q+=(ptrdiff_t) GetPixelChannels(*image);
  }
}

static void XReplaceColormapColor(Image **image,const PixelInfo target,
  const XColor color,ExceptionInfo *exception)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) (*image)->colors; i++)
    if (IsFuzzyEquivalencePixelInfo((*image)->colormap+i,&target))
      {
        (*image)->colormap[i].red=(double) ScaleShortToQuantum(
          color.red);
        (*image)->colormap[i].green=(double) ScaleShortToQuantum(
          color.green);
        (*image)->colormap[i].blue=(double) ScaleShortToQuantum(
          color.blue);
      }
  (void) SyncImage(*image,exception);
}

typedef struct _XReplaceColorInput
{
  Image
    **image;

  CacheView
    *image_view;

  PixelInfo
    target;

  XColor
    color;

  ExceptionInfo
    *exception;
} XReplaceColorInput;

static void XReplaceImageColor(const XReplaceColorInput *replace)
{
  Image
    **const image = replace->image;

  CacheView
    *const image_view = replace->image_view;

  const PixelInfo
    target = replace->target;

  const XColor
    color = replace->color;

  ExceptionInfo
    *const exception = replace->exception;

  int
    y;

  Quantum
    *q;

  if ((*image)->storage_class == DirectClass)
    {
      for (y=0; y < (int) (*image)->rows; y++)
      {
        q=GetCacheViewAuthenticPixels(image_view,0,(ssize_t) y,
          (*image)->columns,1,exception);
        if (q == (Quantum *) NULL)
          break;
        XReplaceRowColor(image,q,target,color);
        if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
          break;
      }
    }
  else
    {
      XReplaceColormapColor(image,target,color,exception);
    }
}

typedef struct _EditMenuInput
{
  Display
    *display;

  XWindows
    *windows;

  const char
    *const *MatteEditMenu;

  int
    id;

  char
    *command;
} EditMenuInput;

typedef struct _PaintMethodTargets
{
  PaintMethod
    *method;
} PaintMethodTargets;

typedef struct _ColorEditPixelInput
{
  Image
    **image;

  CacheView
    *image_view;

  int
    x_offset,
    y_offset;

  ExceptionInfo
    *exception;

  XResourceInfo
    *resource_info;

  PaintMethod
    method;

  XColor
    color,
    border_color;

  unsigned int
    pen_id;
} ColorEditPixelInput;

static void XColorFloodfill(const ColorEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  ExceptionInfo
    *const exception = pixel_input->exception;

  XResourceInfo
    *const resource_info = pixel_input->resource_info;

  const PaintMethod
    method = pixel_input->method;

  const XColor
    border_color = pixel_input->border_color;

  const unsigned int
    pen_id = pixel_input->pen_id;

  DrawInfo
    *draw_info;

  PixelInfo
    target;

  /*
    Update color information using floodfill algorithm.
  */
  (void) GetOneVirtualPixelInfo(*image,
    GetPixelCacheVirtualMethod(*image),(ssize_t) x_offset,(ssize_t)
    y_offset,&target,exception);
  if (method == FillToBorderMethod)
    {
      target.red=(double)
        ScaleShortToQuantum(border_color.red);
      target.green=(double)
        ScaleShortToQuantum(border_color.green);
      target.blue=(double)
        ScaleShortToQuantum(border_color.blue);
    }
  draw_info=CloneDrawInfo(resource_info->image_info,
    (DrawInfo *) NULL);
  (void) QueryColorCompliance(resource_info->pen_colors[pen_id],
    AllCompliance,&draw_info->fill,exception);
  (void) FloodfillPaintImage(*image,draw_info,&target,
    (ssize_t)x_offset,(ssize_t)y_offset,
    method != FloodfillMethod ? MagickTrue : MagickFalse,exception);
  draw_info=DestroyDrawInfo(draw_info);
}

static void XColorReset(const ColorEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  ExceptionInfo
    *const exception = pixel_input->exception;

  const XColor
    color = pixel_input->color;

  int
    x,
    y;

  Quantum
    *q;

  for (y=0; y < (int) (*image)->rows; y++)
  {
    q=QueueCacheViewAuthenticPixels(image_view,0,(ssize_t) y,
      (*image)->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) (*image)->columns; x++)
    {
      SetPixelRed(*image,ScaleShortToQuantum(color.red),q);
      SetPixelGreen(*image,ScaleShortToQuantum(color.green),q);
      SetPixelBlue(*image,ScaleShortToQuantum(color.blue),q);
      q+=(ptrdiff_t) GetPixelChannels(*image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

static MagickBooleanType XColorDirectPixels(
  const ColorEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  ExceptionInfo
    *const exception = pixel_input->exception;

  const PaintMethod
    method = pixel_input->method;

  const XColor
    color = pixel_input->color;

  Quantum
    *q;

  switch (method)
  {
    case PointMethod:
    default:
    {
      /*
        Update color information using point algorithm.
      */
      if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
      q=GetCacheViewAuthenticPixels(image_view,(ssize_t)x_offset,
        (ssize_t) y_offset,1,1,exception);
      if (q == (Quantum *) NULL)
        break;
      SetPixelRed(*image,ScaleShortToQuantum(color.red),q);
      SetPixelGreen(*image,ScaleShortToQuantum(color.green),q);
      SetPixelBlue(*image,ScaleShortToQuantum(color.blue),q);
      (void) SyncCacheViewAuthenticPixels(image_view,exception);
      break;
    }
    case ResetMethod:
    {
      /*
        Update color information using reset algorithm.
      */
      if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
      XColorReset(pixel_input);
      break;
    }
  }
  return(MagickTrue);
}

static MagickBooleanType XColorEditPixels(
  const ColorEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  ExceptionInfo
    *const exception = pixel_input->exception;

  const PaintMethod
    method = pixel_input->method;

  const XColor
    color = pixel_input->color;

  switch (method)
  {
    default:
    {
      if (XColorDirectPixels(pixel_input) == MagickFalse)
        return(MagickFalse);
      break;
    }
    case ReplaceMethod:
    {
      PixelInfo
        target;

      XReplaceColorInput
        replace;

      /*
        Update color information using replace algorithm.
      */
      (void) GetOneCacheViewVirtualPixelInfo(image_view,(ssize_t)
        x_offset,(ssize_t) y_offset,&target,exception);
      replace.image=image;
      replace.image_view=image_view;
      replace.target=target;
      replace.color=color;
      replace.exception=exception;
      XReplaceImageColor(&replace);
      break;
    }
    case FloodfillMethod:
    case FillToBorderMethod:
    {
      XColorFloodfill(pixel_input);
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _ColorBorderTargets
{
  XColor
    *border_color;
} ColorBorderTargets;

static void XSelectColorBorder(const EditMenuInput *menu,
  XResourceInfo *resource_info,const ColorBorderTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const ColorEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  XColor
    *const border_color = targets->border_color;

  const char
    *ColorMenu[MaxNumberPens];

  int
    pen_number;

  ssize_t
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="Browser...";
  ColorMenu[MaxNumberPens-1]=(const char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  pen_number=XMenuWidget(display,windows,ColorEditMenu[id],
    (const char **) ColorMenu,command);
  if (pen_number < 0)
    return;
  if (pen_number == (MaxNumberPens-2))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set border color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],border_color);
}

typedef struct _PenColorTargets
{
  XColor
    *color;

  unsigned int
    *pen_id;
} PenColorTargets;

static void XSelectPenColor(const EditMenuInput *menu,
  XResourceInfo *resource_info,const PenColorTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const ColorEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  XColor
    *const color = targets->color;

  unsigned int
    *const pen_id = targets->pen_id;

  const char
    *ColorMenu[MaxNumberPens];

  int
    pen_number;

  ssize_t
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="Browser...";
  ColorMenu[MaxNumberPens-1]=(const char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  pen_number=XMenuWidget(display,windows,ColorEditMenu[id],
    (const char **) ColorMenu,command);
  if (pen_number < 0)
    return;
  if (pen_number == (MaxNumberPens-2))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set pen color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],color);
  XBestPixel(display,windows->map_info->colormap,(XColor *) NULL,
    (unsigned int) MaxColors,color);
  windows->pixel_info->pen_colors[pen_number]=(*color);
  (*pen_id)=(unsigned int) pen_number;
}

static void
  XSelectPaintMethod(const EditMenuInput *,const PaintMethodTargets *);

static void
  XSelectFuzzFactor(const EditMenuInput *,const char *const *,char *,
    Image **);

static void
  XDiscardMagnifyKeys(Display *,XWindows *,XEvent *);

static const char
  *const ColorEditMenu[] =
  {
    "Method",
    "Pixel Color",
    "Border Color",
    "Fuzz",
    "Undo",
    "Help",
    "Dismiss",
    (char *) NULL
  };

static const ModeType
  ColorEditCommands[] =
  {
    ColorEditMethodCommand,
    ColorEditColorCommand,
    ColorEditBorderCommand,
    ColorEditFuzzCommand,
    ColorEditUndoCommand,
    ColorEditHelpCommand,
    ColorEditDismissCommand
  };

typedef struct _ColorEditContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  ExceptionInfo
    *exception;

  const ModeType
    *ColorEditCommands;

  const char
    *const *ColorEditMenu;

  char
    *command;

  Cursor
    cursor;

  PaintMethodTargets
    method_targets;

  PenColorTargets
    pen_targets;

  ColorBorderTargets
    border_targets;

  char
    *text;
} ColorEditContext;

static void XColorEditCommandAction(const ColorEditContext *context,
  const int id,size_t *state)
{
  switch (context->ColorEditCommands[id])
  {
    case ColorEditUndoCommand:
    {
      (void) XMagickCommand(context->display,context->resource_info,
        context->windows,UndoCommand,context->image,context->exception);
      break;
    }
    case ColorEditHelpCommand:
    default:
    {
      XTextViewHelp(context->display,context->resource_info,context->windows,
        MagickFalse,"Help Viewer - Image Annotation",ImageColorEditHelp);
      break;
    }
    case ColorEditDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
  }
}

static void XColorEditMenuAction(const ColorEditContext *context,
  const int id,size_t *state)
{
  EditMenuInput
    menu_input;

  switch (context->ColorEditCommands[id])
  {
    case ColorEditBorderCommand:
    {
      menu_input.display=context->display;
      menu_input.windows=context->windows;
      menu_input.MatteEditMenu=context->ColorEditMenu;
      menu_input.id=id;
      menu_input.command=context->command;
      XSelectColorBorder(&menu_input,context->resource_info,
        &context->border_targets);
      break;
    }
    case ColorEditFuzzCommand:
    {
      const char
        *const FuzzMenu[] =
        {
          "0%",
          "2%",
          "5%",
          "10%",
          "15%",
          "Dialog...",
          (char *) NULL,
        };

      static char
        fuzz[MagickPathExtent];

      menu_input.display=context->display;
      menu_input.windows=context->windows;
      menu_input.MatteEditMenu=context->ColorEditMenu;
      menu_input.id=id;
      menu_input.command=context->command;
      XSelectFuzzFactor(&menu_input,FuzzMenu,fuzz,context->image);
      break;
    }
    default:
    {
      XColorEditCommandAction(context,id,state);
      break;
    }
  }
}

typedef struct _ColorEditTargets
{
  XEvent
    *event;

  size_t
    *state;

  int
    *x,
    *y;

  PaintMethod
    *method;

  unsigned int
    *pen_id;

  XColor
    *border_color,
    *color;
} ColorEditTargets;

static void XColorEditCommand(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  EditMenuInput
    menu_input;

  int
    id;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(context->display,context->windows,context->ColorEditMenu,
    targets->event);
  if (id < 0)
    {
      (void) XCheckDefineCursor(context->display,context->windows->image.id,
        context->cursor);
      return;
    }
  switch (context->ColorEditCommands[id])
  {
    case ColorEditMethodCommand:
    {
      menu_input.display=context->display;
      menu_input.windows=context->windows;
      menu_input.MatteEditMenu=context->ColorEditMenu;
      menu_input.id=id;
      menu_input.command=context->command;
      XSelectPaintMethod(&menu_input,&context->method_targets);
      break;
    }
    case ColorEditColorCommand:
    {
      menu_input.display=context->display;
      menu_input.windows=context->windows;
      menu_input.MatteEditMenu=context->ColorEditMenu;
      menu_input.id=id;
      menu_input.command=context->command;
      XSelectPenColor(&menu_input,context->resource_info,&context->pen_targets);
      break;
    }
    default:
    {
      XColorEditMenuAction(context,id,targets->state);
      break;
    }
  }
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
}

static void XColorEditPress(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  if ((*event).xbutton.button != Button1)
    return;
  if (((*event).xbutton.window != context->windows->image.id) &&
      ((*event).xbutton.window != context->windows->magnify.id))
    return;
  /*
    exit loop.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  (void) XMagickCommand(context->display,context->resource_info,
    context->windows,
    SaveToUndoBufferCommand,context->image,context->exception);
  (*state)|=UpdateConfigurationState;
}

static void XColorEditRelease(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  if ((*event).xbutton.button != Button1)
    return;
  if (((*event).xbutton.window != context->windows->image.id) &&
      ((*event).xbutton.window != context->windows->magnify.id))
    return;
  /*
    Update colormap information.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  XConfigureImageColormap(context->display,context->resource_info,
    context->windows,*context->image,context->exception);
  (void) XConfigureImage(context->display,context->resource_info,
    context->windows,*context->image,context->exception);
  XInfoWidget(context->display,context->windows,context->text);
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
  (*state)&=(size_t) (~UpdateConfigurationState);
}

static void XColorEditKey(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  char
    command[MagickPathExtent];

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  KeySym
    key_symbol;

  XDiscardMagnifyKeys(context->display,context->windows,event);
  if ((*event).xkey.window != context->windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XTextViewHelp(context->display,context->resource_info,context->windows,
        MagickFalse,"Help Viewer - Image Annotation",ImageColorEditHelp);
      break;
    }
    default:
    {
      (void) XBell(context->display,0);
      break;
    }
  }
}

static void XColorEditEvent(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XColorEditPress(context,targets);
      break;
    }
    case ButtonRelease:
    {
      XColorEditRelease(context,targets);
      break;
    }
    case Expose:
      break;
    case KeyPress:
    {
      XColorEditKey(context,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(context->display,context->windows,(*x),(*y));
      break;
    }
    default:
      break;
  }
}

static MagickBooleanType XColorEditUpdate(const ColorEditContext *context,
  const ColorEditTargets *targets,int x_offset,int y_offset)
{
  PaintMethod
    *const method = targets->method;

  size_t
    *const state = targets->state;

  unsigned int
    *const pen_id = targets->pen_id;

  unsigned int
    height,
    width;

  XColor
    *const border_color = targets->border_color;

  XColor
    *const color = targets->color;

  if (((*state) & UpdateConfigurationState) != 0)
    {
      CacheView
        *image_view;

      ColorEditPixelInput
        pixel_input;

      int
        x,
        y;

      /*
        Pixel edit is relative to image configuration.
      */
      (void) XClearArea(context->display,context->windows->image.id,x_offset,
        y_offset,1,1,MagickTrue);
      (*color)=context->windows->pixel_info->pen_colors[(*pen_id)];
      XPutPixel(context->windows->image.ximage,x_offset,y_offset,
        (*color).pixel);
      width=(unsigned int) (*context->image)->columns;
      height=(unsigned int) (*context->image)->rows;
      x=0;
      y=0;
      if (context->windows->image.crop_geometry != (char *) NULL)
        (void) XParseGeometry(context->windows->image.crop_geometry,&x,&y,
          &width,&height);
      x_offset=((int) width*(context->windows->image.x+x_offset)/(int)
        context->windows->image.ximage->width+x);
      y_offset=((int) height*(context->windows->image.y+y_offset)/(int)
        context->windows->image.ximage->height+y);
      if ((x_offset < 0) || (y_offset < 0))
        return(MagickTrue);
      if ((x_offset >= (int) (*context->image)->columns) ||
          (y_offset >= (int) (*context->image)->rows))
        return(MagickTrue);
      image_view=AcquireAuthenticCacheView(*context->image,context->exception);
      pixel_input.image=context->image;
      pixel_input.image_view=image_view;
      pixel_input.x_offset=x_offset;
      pixel_input.y_offset=y_offset;
      pixel_input.exception=context->exception;
      pixel_input.resource_info=context->resource_info;
      pixel_input.method=(*method);
      pixel_input.color=(*color);
      pixel_input.border_color=(*border_color);
      pixel_input.pen_id=(*pen_id);
      if (XColorEditPixels(&pixel_input) == MagickFalse)
        return(MagickFalse);
      image_view=DestroyCacheView(image_view);
      (*state)&=(~0x0080U);
    }
  return(MagickTrue);
}

static MagickBooleanType XColorEditStep(const ColorEditContext *context,
  const ColorEditTargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  int
    x_offset,
    y_offset;

  if (context->windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(context->text,MagickPathExtent," %+d%+d ",
        (*x)+context->windows->image.x,(*y)+context->windows->image.y);
      XInfoWidget(context->display,context->windows,context->text);
    }
  /*
    Wait for next event.
  */
  XScreenEvent(context->display,context->windows,event,context->exception);
  if ((*event).xany.window == context->windows->command.id)
    {
      XColorEditCommand(context,targets);
      return(MagickTrue);
    }
  XColorEditEvent(context,targets);
  if ((*event).xany.window == context->windows->magnify.id)
    {
      (*x)=context->windows->magnify.x-context->windows->image.x;
      (*y)=context->windows->magnify.y-context->windows->image.y;
    }
  x_offset=(*x);
  y_offset=(*y);
  if (XColorEditUpdate(context,targets,x_offset,y_offset) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static void XMapColorEditCommand(Display *display,XWindows *windows)
{
  (void) CloneString(&windows->command.name,"Color Edit");
  windows->command.data=4;
  (void) XCommandWidget(display,windows,ColorEditMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
}

static MagickBooleanType XColorEditImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  static PaintMethod
    method = PointMethod;

  static unsigned int
    pen_id = 0;

  static XColor
    border_color = { 0, 0, 0, 0, 0, 0 };

  char
    command[MagickPathExtent],
    text[MagickPathExtent];

  Cursor
    cursor;

  int
    x,
    y;

  size_t
    state;

  XColor
    color;

  XEvent
    event;

  ColorEditContext
    context;

  const PaintMethodTargets
    method_targets = { &method };

  const PenColorTargets
    pen_targets = { &color, &pen_id };

  const ColorBorderTargets
    border_targets = { &border_color };

  const ColorEditTargets
    targets = { &event, &state, &x, &y, &method, &pen_id, &border_color,
      &color };

  /*
    Map Command widget.
  */
  XMapColorEditCommand(display,windows);
  /*
    Make cursor.
  */
  cursor=XMakeCursor(display,windows->image.id,windows->map_info->colormap,
    resource_info->background_color,resource_info->foreground_color);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  state=DefaultState;
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.image=image;
  context.exception=exception;
  context.ColorEditCommands=ColorEditCommands;
  context.ColorEditMenu=ColorEditMenu;
  context.command=command;
  context.cursor=cursor;
  context.method_targets=method_targets;
  context.pen_targets=pen_targets;
  context.border_targets=border_targets;
  context.text=text;
  do
  {
    if (XColorEditStep(&context,&targets) == MagickFalse)
      return(MagickFalse);
  } while ((state & ExitState) == 0);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeCursor(display,cursor);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C o m p o s i t e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XCompositeImage() requests an image name from the user, reads the image and
%  composites it with the X window image at a location the user chooses with
%  the pointer.
%
%  The format of the XCompositeImage method is:
%
%      MagickBooleanType XCompositeImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _ComposeMenuInfo
{
  Display
    *display;

  XWindows
    *windows;

  int
    id;
} ComposeMenuInfo;

static CompositeOperator
  XSelectPasteOperator(const ComposeMenuInfo *,const char *const *,
    CompositeOperator);

typedef struct _CompositeTargets
{
  double
    *blend;

  CompositeOperator
    *compose;
} CompositeTargets;

static void XSelectDissolveBlend(Display *display,XWindows *windows,
  const CompositeTargets *targets)
{
  double
    *const blend = targets->blend;

  CompositeOperator
    *const compose = targets->compose;

  static char
    factor[MagickPathExtent] = "20.0";

  /*
    Dissolve the two images a given percent.
  */
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  (void) XDialogWidget(display,windows,"Dissolve",
    "Enter the blend factor (0.0 - 99.9%):",factor);
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
  if (*factor == '\0')
    return;
  (*blend)=StringToDouble(factor,(char **) NULL);
  (*compose)=DissolveCompositeOp;
}

static void XSelectDisplaceGeometry(Display *display,XWindows *windows,
  char *displacement_geometry,const CompositeTargets *targets)
{
  CompositeOperator
    *const compose = targets->compose;

  /*
    Get horizontal and vertical scale displacement geometry.
  */
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  (void) XDialogWidget(display,windows,"Displace",
    "Enter the horizontal and vertical scale:",displacement_geometry);
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
  if (*displacement_geometry == '\0')
    return;
  (*compose)=DisplaceCompositeOp;
}

static void XSetBlendAlpha(Image *image,CacheView *image_view,
  const Quantum opacity,ExceptionInfo *exception)
{
  int
    x,
    y;

  Quantum
    *q;

  for (y=0; y < (int) image->rows; y++)
  {
    q=GetCacheViewAuthenticPixels(image_view,0,(ssize_t) y,image->columns,1,
      exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) image->columns; x++)
    {
      SetPixelAlpha(image,opacity,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

static MagickBooleanType XCreateBlendMattes(Image *image,Image *composite_image,
  const double blend,ExceptionInfo *exception)
{
  if (blend != 0.0)
    {
      CacheView
        *image_view;

      Quantum
        opacity;

      /*
        Create mattes for blending.
      */
      (void) SetImageAlphaChannel(composite_image,OpaqueAlphaChannel,exception);
      opacity=(Quantum) (ScaleQuantumToChar(QuantumRange)-
        ((ssize_t) ScaleQuantumToChar(QuantumRange)*blend)/100);
      if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
      image->alpha_trait=BlendPixelTrait;
      image_view=AcquireAuthenticCacheView(image,exception);
      XSetBlendAlpha(image,image_view,opacity,exception);
      image_view=DestroyCacheView(image_view);
    }
  return(MagickTrue);
}

typedef struct _CompositeCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const ModeType
    *CompositeCommands;

  const char
    *const *CompositeMenu;

  char
    *displacement_geometry;

  CompositeTargets
    targets;

  ExceptionInfo
    *exception;
} CompositeCommandInput;

static void XCompositeControl(const CompositeCommandInput *input,const int id,
  size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const CompositeCommands = input->CompositeCommands;

  switch (CompositeCommands[id])
  {
    case CompositeHelpCommand:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Composite",ImageCompositeHelp);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    case CompositeDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static MagickBooleanType XCompositeCommandBlock(
  const CompositeCommandInput *input,XEvent *event,CompositeOperator *compose,
  size_t *state)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const char
    *const *const CompositeMenu = input->CompositeMenu;

  const ModeType
    *const CompositeCommands = input->CompositeCommands;

  const CompositeTargets
    targets = input->targets;

  int
    id;

  if ((*event).xany.window == windows->command.id)
    {
      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,CompositeMenu,event);
      if (id < 0)
        return(MagickTrue);
      switch (CompositeCommands[id])
      {
        case CompositeOperatorsCommand:
        {
          ComposeMenuInfo
            menu_info;

          menu_info.display=display;
          menu_info.windows=windows;
          menu_info.id=id;
          (*compose)=XSelectPasteOperator(&menu_info,CompositeMenu,(*compose));
          break;
        }
        case CompositeDissolveCommand:
        {
          XSelectDissolveBlend(display,windows,&targets);
          break;
        }
        case CompositeDisplaceCommand:
        {
          XSelectDisplaceGeometry(display,windows,input->displacement_geometry,
            &targets);
          break;
        }
        default:
        {
          XCompositeControl(input,id,state);
          break;
        }
      }
      return(MagickTrue);
    }
  return(MagickFalse);
}

typedef struct _CompositeEventTargets
{
  Image
    **composite_image;

  size_t
    *state;

  RectangleInfo
    *composite_info;

  CompositeOperator
    *compose;
} CompositeEventTargets;

static void XCompositeKeyPress(const CompositeCommandInput *input,
  XEvent *event,const CompositeEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const composite_image = targets->composite_image;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  int
    length;

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  length=XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key press: 0x%lx (%s)",(unsigned long) key_symbol,command);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*composite_image)=DestroyImage((*composite_image));
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Composite",ImageCompositeHelp);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static void XCompositePress(const CompositeCommandInput *input,
  const XEvent *event,const Cursor cursor,const CompositeEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const composite_image = targets->composite_image;

  RectangleInfo
    *const composite_info = targets->composite_info;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Press: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  /*
    Change cursor.
  */
  (*composite_info).width=(*composite_image)->columns;
  (*composite_info).height=(*composite_image)->rows;
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (*composite_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
  (*composite_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
}

static void XCompositeRelease(const CompositeCommandInput *input,
  const XEvent *event,const CompositeEventTargets *targets)
{
  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    *const composite_info = targets->composite_info;

  size_t
    *const state = targets->state;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Release: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  if (((*composite_info).width != 0) && ((*composite_info).height != 0))
    {
      /*
        User has selected the location of the composite image.
      */
      (*composite_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
      (*composite_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
      (*state)|=ExitState;
    }
}

static void XCompositeEvent(const CompositeCommandInput *input,XEvent *event,
  const Cursor cursor,const CompositeEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    *const composite_info = targets->composite_info;

  int
    x,
    y;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XCompositePress(input,event,cursor,targets);
      break;
    }
    case ButtonRelease:
    {
      XCompositeRelease(input,event,targets);
      break;
    }
    case Expose:
      break;
    case KeyPress:
    {
      XCompositeKeyPress(input,event,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      x=(*event).xmotion.x;
      y=(*event).xmotion.y;
      XTrackInfoWidget(display,windows,x,y);
      (*composite_info).x=(ssize_t) windows->image.x+x;
      (*composite_info).y=(ssize_t) windows->image.y+y;
      break;
    }
    default:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"Event type: %d",
          (*event).type);
      break;
    }
  }
}

static void XCompositeStep(const CompositeCommandInput *input,
  const Cursor cursor,const CompositeEventTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  CompositeOperator
    *const compose = targets->compose;

  RectangleInfo
    *const composite_info = targets->composite_info;

  size_t
    *const state = targets->state;

  char
    text[MagickPathExtent];

  RectangleInfo
    highlight_info;

  XEvent
    event;

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+ld%+ld ",
        (long) (*composite_info).x,(long) (*composite_info).y);
      XInfoWidget(display,windows,text);
    }
  highlight_info=(*composite_info);
  highlight_info.x=(*composite_info).x-windows->image.x;
  highlight_info.y=(*composite_info).y-windows->image.y;
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  if (XCompositeCommandBlock(input,&event,compose,state) != MagickFalse)
    return;
  XCompositeEvent(input,&event,cursor,targets);
}

static void XCompositeScaleLocation(const Image *image,
  const XWindows *windows,RectangleInfo *composite_info)
{
  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  (*composite_info).x+=x;
  (*composite_info).x=(ssize_t) (scale_factor*(*composite_info).x+0.5);
  (*composite_info).width=(unsigned int) (scale_factor*
    (*composite_info).width+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  (*composite_info).y+=y;
  (*composite_info).y=(ssize_t) (scale_factor*(*composite_info).y+0.5);
  (*composite_info).height=(unsigned int) (scale_factor*
    (*composite_info).height+0.5);
}

static MagickBooleanType XCompositeImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  const char
    *const CompositeMenu[] =
    {
      "Operators",
      "Dissolve",
      "Displace",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static char
    displacement_geometry[MagickPathExtent] = "30x30",
    filename[MagickPathExtent] = "\0";

  static CompositeOperator
    compose = CopyCompositeOp;

  static const ModeType
    CompositeCommands[] =
    {
      CompositeOperatorsCommand,
      CompositeDissolveCommand,
      CompositeDisplaceCommand,
      CompositeHelpCommand,
      CompositeDismissCommand
    };

  Cursor
    cursor;

  Image
    *composite_image;

  int
    x,
    y;

  double
    blend;

  RectangleInfo
    composite_info;

  size_t
    state;

  const CompositeTargets
    targets = { &blend, &compose };

  const CompositeEventTargets
    event_targets = { &composite_image, &state, &composite_info,
      &compose };

  const CompositeCommandInput
    command_input = { display, resource_info, windows, CompositeCommands,
      CompositeMenu, displacement_geometry, targets, exception };

  /*
    Request image file name from user.
  */
  XFileBrowserWidget(display,windows,"Composite",filename);
  if (*filename == '\0')
    return(MagickTrue);
  /*
    Read image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(resource_info->image_info->filename,filename,
    MagickPathExtent);
  composite_image=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (composite_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Composite");
  windows->command.data=1;
  (void) XCommandWidget(display,windows,CompositeMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  composite_info.x=(ssize_t) windows->image.x+x;
  composite_info.y=(ssize_t) windows->image.y+y;
  composite_info.width=0;
  composite_info.height=0;
  cursor=XCreateFontCursor(display,XC_ul_angle);
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  blend=0.0;
  state=DefaultState;
  do
  {
    XCompositeStep(&command_input,cursor,&event_targets);
  } while ((state & ExitState) == 0);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeCursor(display,cursor);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  /*
    Image compositing is relative to image configuration.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  XCompositeScaleLocation(image,windows,&composite_info);
  if ((composite_info.width != composite_image->columns) ||
      (composite_info.height != composite_image->rows))
    {
      Image
        *resize_image;

      /*
        Scale composite image.
      */
      resize_image=ResizeImage(composite_image,composite_info.width,
        composite_info.height,composite_image->filter,exception);
      composite_image=DestroyImage(composite_image);
      if (resize_image == (Image *) NULL)
        {
          XSetCursorState(display,windows,MagickFalse);
          return(MagickFalse);
        }
      composite_image=resize_image;
    }
  if (compose == DisplaceCompositeOp)
    (void) SetImageArtifact(composite_image,"compose:args",
      displacement_geometry);
  if (XCreateBlendMattes(image,composite_image,blend,exception) == MagickFalse)
    return(MagickFalse);
  /*
    Composite image with X Image window.
  */
  (void) CompositeImage(image,composite_image,compose,MagickTrue,
    composite_info.x,composite_info.y,exception);
  composite_image=DestroyImage(composite_image);
  XSetCursorState(display,windows,MagickFalse);
  /*
    Update image configuration.
  */
  XConfigureImageColormap(display,resource_info,windows,image,exception);
  (void) XConfigureImage(display,resource_info,windows,image,exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C o n f i g u r e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XConfigureImage() creates a new X image.  It also notifies the window
%  manager of the new image size and configures the transient widows.
%
%  The format of the XConfigureImage method is:
%
%      MagickBooleanType XConfigureImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XResizePanWindow(Display *display,XWindows *windows,Image *image)
{
  XSizeHints
    *size_hints;

  XWindowChanges
    window_changes;

  windows->pan.crop_geometry=windows->image.crop_geometry;
  XBestIconSize(display,&windows->pan,image);
  while (((windows->pan.width << 1) < MaxIconSize) &&
         ((windows->pan.height << 1) < MaxIconSize))
  {
    windows->pan.width<<=1;
    windows->pan.height<<=1;
  }
  if (windows->pan.geometry != (char *) NULL)
    (void) XParseGeometry(windows->pan.geometry,&windows->pan.x,&windows->pan.y,
      &windows->pan.width,&windows->pan.height);
  window_changes.width=(int) windows->pan.width;
  window_changes.height=(int) windows->pan.height;
  size_hints=XAllocSizeHints();
  if (size_hints != (XSizeHints *) NULL)
    {
      /*
        Set new size hints.
      */
      size_hints->flags=PSize | PMinSize | PMaxSize;
      size_hints->width=window_changes.width;
      size_hints->height=window_changes.height;
      size_hints->min_width=size_hints->width;
      size_hints->min_height=size_hints->height;
      size_hints->max_width=size_hints->width;
      size_hints->max_height=size_hints->height;
      (void) XSetNormalHints(display,windows->pan.id,size_hints);
      (void) XFree((void *) size_hints);
    }
  (void) XReconfigureWMWindow(display,windows->pan.id,windows->pan.screen,
    (unsigned int) (CWWidth | CWHeight),&window_changes);
}

typedef struct _ConfigureNotifyInput
{
  size_t
    width,
    height;

  ssize_t
    x,
    y;
} ConfigureNotifyInput;

static void XNotifyWindowManager(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const ConfigureNotifyInput *input)
{
  char
    geometry[MagickPathExtent];

  size_t
    height = input->height,
    mask,
    width = input->width;

  ssize_t
    x = input->x,
    y = input->y;

  XWindowChanges
    window_changes;

  if (resource_info->image_geometry != (char *) NULL)
    (void) FormatLocaleString(geometry,MagickPathExtent,"%s>!",
      resource_info->image_geometry);
  else
    (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>!",
      XDisplayWidth(display,windows->image.screen),
      XDisplayHeight(display,windows->image.screen));
  (void) ParseMetaGeometry(geometry,&x,&y,&width,&height);
  window_changes.width=(int) width;
  if (window_changes.width > XDisplayWidth(display,windows->image.screen))
    window_changes.width=XDisplayWidth(display,windows->image.screen);
  window_changes.height=(int) height;
  if (window_changes.height > XDisplayHeight(display,windows->image.screen))
    window_changes.height=XDisplayHeight(display,windows->image.screen);
  mask=(size_t) (CWWidth | CWHeight);
  if (resource_info->backdrop)
    {
      mask|=CWX | CWY;
      window_changes.x=((XDisplayWidth(display,windows->image.screen)/2)-
        ((int) width/2));
      window_changes.y=((XDisplayHeight(display,windows->image.screen)/2)-
        ((int) height/2));
    }
  (void) XReconfigureWMWindow(display,windows->image.id,windows->image.screen,
    (unsigned int) mask,&window_changes);
}

static MagickBooleanType XConfigureImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  ConfigureNotifyInput
    notify;

  MagickStatusType
    status;

  size_t
    height,
    width;

  ssize_t
    x,
    y;

  XWindowChanges
    window_changes;

  /*
    Dismiss if window dimensions are zero.
  */
  width=(unsigned int) windows->image.window_changes.width;
  height=(unsigned int) windows->image.window_changes.height;
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Configure Image: %dx%d=>%.17gx%.17g",windows->image.ximage->width,
      windows->image.ximage->height,(double) width,(double) height);
  if ((width*height) == 0)
    return(MagickTrue);
  x=0;
  y=0;
  /*
    Resize image to fit Image window dimensions.
  */
  XSetCursorState(display,windows,MagickTrue);
  (void) XFlush(display);
  if (((int) width != windows->image.ximage->width) ||
      ((int) height != windows->image.ximage->height))
    image->taint=MagickTrue;
  windows->magnify.x=(int)
    width*windows->magnify.x/windows->image.ximage->width;
  windows->magnify.y=(int)
    height*windows->magnify.y/windows->image.ximage->height;
  windows->image.x=((int) width*windows->image.x/windows->image.ximage->width);
  windows->image.y=((int) height*windows->image.y/
    windows->image.ximage->height);
  status=XMakeImage(display,resource_info,&windows->image,image,
    (unsigned int) width,(unsigned int) height,exception);
  if (status == MagickFalse)
    XNoticeWidget(display,windows,"Unable to configure X image:",
      windows->image.name);
  /*
    Notify window manager of the new configuration.
  */
  notify.width=width;
  notify.height=height;
  notify.x=x;
  notify.y=y;
  XNotifyWindowManager(display,resource_info,windows,&notify);
  (void) XClearWindow(display,windows->image.id);
  XRefreshWindow(display,&windows->image,(XEvent *) NULL);
  /*
    Update Magnify window configuration.
  */
  if (windows->magnify.mapped != MagickFalse)
    XMakeMagnifyImage(display,windows,exception);
  XResizePanWindow(display,windows,image);
  /*
    Update icon window configuration.
  */
  windows->icon.crop_geometry=windows->image.crop_geometry;
  XBestIconSize(display,&windows->icon,image);
  window_changes.width=(int) windows->icon.width;
  window_changes.height=(int) windows->icon.height;
  (void) XReconfigureWMWindow(display,windows->icon.id,windows->icon.screen,
    (unsigned int) (CWWidth | CWHeight),&window_changes);
  XSetCursorState(display,windows,MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X C r o p I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XCropImage() allows the user to select a region of the image and crop, copy,
%  or cut it.  For copy or cut, the image can subsequently be composited onto
%  the image with XPasteImage.
%
%  The format of the XCropImage method is:
%
%      MagickBooleanType XCropImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        const ClipboardMode mode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o mode: This unsigned value specified whether the image should be
%      cropped, copied, or cut.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsNearTopLeftCorner(const int x,const int y,
  const RectangleInfo rectangle)
{
  return(((x < (int) (rectangle.x+RoiDelta)) &&
    (x > (int) (rectangle.x-RoiDelta)) &&
    (y < (int) (rectangle.y+RoiDelta)) &&
    (y > (int) (rectangle.y-RoiDelta))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNearBottomLeftCorner(const int x,const int y,
  const RectangleInfo rectangle)
{
  return(((x < (int) (rectangle.x+RoiDelta)) &&
    (x > (int) (rectangle.x-RoiDelta)) &&
    (y < (int) (rectangle.y+(int) rectangle.height+RoiDelta)) &&
    (y > (int) (rectangle.y+(int) rectangle.height-RoiDelta))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsNearTopRightCorner(const int x,const int y,
  const RectangleInfo rectangle)
{
  return(((x < (int) (rectangle.x+(int) rectangle.width+RoiDelta)) &&
    (x > (int) (rectangle.x+(int) rectangle.width-RoiDelta)) &&
    (y < (int) (rectangle.y+RoiDelta)) &&
    (y > (int) (rectangle.y-RoiDelta))) ? MagickTrue : MagickFalse);
}

static inline MagickBooleanType IsNearBottomRightCorner(const int x,const int y,
  const RectangleInfo rectangle)
{
  return(((x < (int) (rectangle.x+(int) rectangle.width+RoiDelta)) &&
    (x > (int) (rectangle.x+(int) rectangle.width-RoiDelta)) &&
    (y < (int) (rectangle.y+(int) rectangle.height+RoiDelta)) &&
    (y > (int) (rectangle.y+(int) rectangle.height-RoiDelta))) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsSelectionMovedOrDone(
  const RectangleInfo rectangle,const int x,const int y,const size_t state)
{
  return(((((int) rectangle.x != x) && ((int) rectangle.y != y)) ||
    ((state & ExitState) != 0)) ? MagickTrue : MagickFalse);
}

static void XShowCropHelp(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const ClipboardMode mode)
{
  switch (mode)
  {
    case CopyMode:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Copy",ImageCopyHelp);
      break;
    }
    case CropMode:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Crop",ImageCropHelp);
      break;
    }
    case CutMode:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Cut",ImageCutHelp);
      break;
    }
  }
}

static void XMapInfoWidget(Display *display,XWindows *windows)
{
  if (windows->info.mapped == MagickFalse)
    (void) XMapWindow(display,windows->info.id);
}

static void XCheckSelectionBounds(XWindows *windows,const int x,const int y,
  RectangleInfo *crop_info)
{
  /*
    Check boundary conditions.
  */
  if ((*crop_info).x < 0)
    (*crop_info).x=0;
  else
    if ((*crop_info).x > (ssize_t) windows->image.ximage->width)
      (*crop_info).x=(ssize_t) windows->image.ximage->width;
  if ((int) (*crop_info).x < x)
    (*crop_info).width=(unsigned int) (x-(*crop_info).x);
  else
    {
      (*crop_info).width=(unsigned int) ((*crop_info).x-x);
      (*crop_info).x=(ssize_t) x;
    }
  if ((*crop_info).y < 0)
    (*crop_info).y=0;
  else
    if ((*crop_info).y > (ssize_t) windows->image.ximage->height)
      (*crop_info).y=(ssize_t) windows->image.ximage->height;
  if ((int) (*crop_info).y < y)
    (*crop_info).height=(unsigned int) (y-(*crop_info).y);
  else
    {
      (*crop_info).height=(unsigned int) ((*crop_info).y-y);
      (*crop_info).y=(ssize_t) y;
    }
}

typedef struct _CropCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ClipboardMode
    mode;

  RectangleInfo
    highlight_info;

  const char
    **RectifyModeMenu;

  const ModeType
    *RectifyCommands;
} CropCommandInput;

static void XCropRectifyCommand(const CropCommandInput *command_input,
  XEvent *event,size_t *state)
{
  Display
    *const display = command_input->display;

  XResourceInfo
    *const resource_info = command_input->resource_info;

  XWindows
    *const windows = command_input->windows;

  const ClipboardMode
    mode = command_input->mode;

  const RectangleInfo
    highlight_info = command_input->highlight_info;

  const char
    **const RectifyModeMenu = command_input->RectifyModeMenu;

  const ModeType
    *const RectifyCommands = command_input->RectifyCommands;

  int
    id;

  /*
    Select a command from the Command widget.
  */
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  id=XCommandWidget(display,windows,RectifyModeMenu,event);
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  if (id >= 0)
    switch (RectifyCommands[id])
    {
      case RectifyCopyCommand:
      {
        (*state)|=ExitState;
        break;
      }
      case RectifyHelpCommand:
      {
        (void) XSetFunction(display,windows->image.highlight_context,
          GXcopy);
        XShowCropHelp(display,resource_info,windows,mode);
        (void) XSetFunction(display,windows->image.highlight_context,
          GXinvert);
        break;
      }
      case RectifyDismissCommand:
      {
        /*
          Prematurely exit.
        */
        (*state)|=EscapeState;
        (*state)|=ExitState;
        break;
      }
      default:
        break;
    }
}

typedef struct _CropEventInput
{
  Display
    *display;

  XWindows
    *windows;

  const XEvent
    *event;

  RectangleInfo
    highlight_info;

  const char
    *text;

  XResourceInfo
    *resource_info;

  ClipboardMode
    mode;
} CropEventInput;

static void XCropButtonRelease(const CropEventInput *event_input,
  const RectangleInfo crop_info)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  const RectangleInfo
    highlight_info = event_input->highlight_info;

  if ((*event).xbutton.window == windows->pan.id)
    if ((highlight_info.x != crop_info.x-windows->image.x) ||
        (highlight_info.y != crop_info.y-windows->image.y))
      XHighlightRectangle(display,windows->image.id,
        windows->image.highlight_context,&highlight_info);
  (void) XSetSelectionOwner(display,XA_PRIMARY,windows->image.id,
    (*event).xbutton.time);
}

typedef struct _CropTargets
{
  int
    *x,
    *y;

  RectangleInfo
    *crop_info;

  size_t
    *state;

  XEvent
    *event;
} CropTargets;

static void XCropSelectionRequest(XEvent *event,const RectangleInfo crop_info,
  char *text)
{
  XSelectionEvent
    notify;

  XSelectionRequestEvent
    *request;

  /*
    Set primary selection.
  */
  (void) FormatLocaleString(text,MagickPathExtent,
    "%.17gx%.17g%+.20g%+.20g",(double) crop_info.width,(double)
    crop_info.height,(double) crop_info.x,(double) crop_info.y);
  request=(&((*event).xselectionrequest));
  (void) XChangeProperty(request->display,request->requestor,
    request->property,request->target,8,PropModeReplace,
    (unsigned char *) text,(int) strlen(text));
  notify.type=SelectionNotify;
  notify.display=request->display;
  notify.requestor=request->requestor;
  notify.selection=request->selection;
  notify.target=request->target;
  notify.time=request->time;
  if (request->property == None)
    notify.property=request->target;
  else
    notify.property=request->property;
  (void) XSendEvent(request->display,request->requestor,False,0,
    (XEvent *) &notify);
}

static void XCropMotion(const CropEventInput *event_input,
  const CropTargets *targets)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  if ((*event).xmotion.window != windows->image.id)
    return;
  /*
    Map and unmap Info widget as text cursor crosses its boundaries.
  */
  (*x)=(*event).xmotion.x;
  (*y)=(*event).xmotion.y;
  XTrackInfoWidget(display,windows,(*x),(*y));
  (*crop_info).x=(ssize_t) windows->image.x+(*event).xmotion.x;
  (*crop_info).y=(ssize_t) windows->image.y+(*event).xmotion.y;
}

static void XCropExpose(const CropEventInput *event_input,XEvent *event)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  const RectangleInfo
    highlight_info = event_input->highlight_info;

  const char
    *const text = event_input->text;

  if (((*event).xexpose.window == windows->image.id) &&
      ((*event).xexpose.count == 0))
    {
      (*event).xexpose.x=(int) highlight_info.x;
      (*event).xexpose.y=(int) highlight_info.y;
      (*event).xexpose.width=(int) highlight_info.width;
      (*event).xexpose.height=(int) highlight_info.height;
      XRefreshWindow(display,&windows->image,event);
    }
  if (((*event).xexpose.window == windows->info.id) &&
      ((*event).xexpose.count == 0))
    XInfoWidget(display,windows,text);
}

static void XCropButtonPress(const CropEventInput *event_input,
  const CropTargets *targets)
{
  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  size_t
    *const state = targets->state;

  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  (*x)=windows->image.x+(*event).xbutton.x;
  (*y)=windows->image.y+(*event).xbutton.y;
  if (IsNearTopLeftCorner((*x),(*y),(*crop_info)) != MagickFalse)
    {
      (*crop_info).x=(*crop_info).x+(ssize_t) (*crop_info).width;
      (*crop_info).y=(*crop_info).y+(ssize_t) (*crop_info).height;
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearBottomLeftCorner((*x),(*y),(*crop_info)) != MagickFalse)
    {
      (*crop_info).x=((*crop_info).x+(int) (*crop_info).width);
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearTopRightCorner((*x),(*y),(*crop_info)) != MagickFalse)
    {
      (*crop_info).y=((*crop_info).y+(ssize_t) (*crop_info).height);
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearBottomRightCorner((*x),(*y),(*crop_info)) != MagickFalse)
    {
      (*state)|=UpdateConfigurationState;
      return;
    }
  XCropButtonRelease(event_input,(*crop_info));
}

static void XCropKeyHelp(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const ClipboardMode mode)
{
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  switch (mode)
  {
    case CopyMode:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Copy",ImageCopyHelp);
      break;
    }
    case CropMode:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Cropg",ImageCropHelp);
      break;
    }
    case CutMode:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Cutg",ImageCutHelp);
      break;
    }
  }
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
}

static void XCropDownHelpKey(const CropEventInput *event_input,
  const CropTargets *targets,const KeySym key_symbol)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  XResourceInfo
    *const resource_info = event_input->resource_info;

  const ClipboardMode
    mode = event_input->mode;

  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_Down:
    case XK_KP_Down:
    {
      (*crop_info).y++;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XCropKeyHelp(display,resource_info,windows,mode);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static void XCropUpRightKey(const CropEventInput *event_input,
  const CropTargets *targets,const KeySym key_symbol)
{
  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((int) key_symbol)
  {
    case XK_Up:
    case XK_KP_Up:
    case XK_Next:
    {
      (*crop_info).y--;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      (*crop_info).x++;
      break;
    }
    default:
    {
      XCropDownHelpKey(event_input,targets,key_symbol);
      break;
    }
  }
}

static void XCropHomeLeftKey(const CropEventInput *event_input,
  const CropTargets *targets,const KeySym key_symbol)
{
  XWindows
    *const windows = event_input->windows;

  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      (*crop_info).x=(ssize_t) (windows->image.width/2L-(*crop_info).width/
        2L);
      (*crop_info).y=(ssize_t) (windows->image.height/2L-(*crop_info).height/
        2L);
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      (*crop_info).x--;
      break;
    }
    default:
    {
      XCropUpRightKey(event_input,targets,key_symbol);
      break;
    }
  }
}

static void XCropKeyPress(const CropEventInput *event_input,XEvent *event,
  const CropTargets *targets)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      (*state)|=EscapeState;
      magick_fallthrough;
    }
    case XK_Return:
    {
      (*state)|=ExitState;
      break;
    }
    default:
    {
      XCropHomeLeftKey(event_input,targets,key_symbol);
      break;
    }
  }
  (void) XSetSelectionOwner(display,XA_PRIMARY,windows->image.id,
    (*event).xkey.time);
}

static const ModeType
  CropCommands[] =
  {
    CropHelpCommand,
    CropDismissCommand
  },
  RectifyCommands[] =
  {
    RectifyCopyCommand,
    RectifyHelpCommand,
    RectifyDismissCommand
  };

typedef struct _CropContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ClipboardMode
    mode;

  const char
    *const *CropModeMenu;

  Cursor
    cursor;

  ExceptionInfo
    *exception;

  char
    *text;

  const char
    **RectifyModeMenu;
} CropContext;

static void XCropPointerCommand(const CropContext *context,
  const CropTargets *targets)
{
  int
    id;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(context->display,context->windows,context->CropModeMenu,
    event);
  if (id < 0)
    return;
  switch (CropCommands[id])
  {
    case CropHelpCommand:
    {
      XShowCropHelp(context->display,context->resource_info,context->windows,
        context->mode);
      break;
    }
    case CropDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static void XCropPointerPress(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const crop_info = targets->crop_info;

  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != context->windows->image.id)
    return;
  /*
    Note first corner of cropping rectangle-- exit loop.
  */
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
  (*crop_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
  (*crop_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
  (*state)|=ExitState;
}

static void XCropPointerKey(const CropContext *context,
  const CropTargets *targets)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  if ((*event).xkey.window != context->windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XShowCropHelp(context->display,context->resource_info,context->windows,
        context->mode);
      break;
    }
    default:
    {
      (void) XBell(context->display,0);
      break;
    }
  }
}

static void XCropPointerEvent(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XCropPointerPress(context,targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      XCropPointerKey(context,targets);
      break;
    }
    case MotionNotify:
    {
      if ((*event).xmotion.window != context->windows->image.id)
        break;
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(context->display,context->windows,(*x),(*y));
      (*crop_info).x=(ssize_t) context->windows->image.x+(*x);
      (*crop_info).y=(ssize_t) context->windows->image.y+(*y);
      break;
    }
    default:
      break;
  }
}

static void XCropTrackPointer(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const crop_info = targets->crop_info;

  do
  {
    if (context->windows->info.mapped != MagickFalse)
      {
        /*
          Display pointer position.
        */
        (void) FormatLocaleString(context->text,MagickPathExtent," %+ld%+ld ",
          (long) (*crop_info).x,(long) (*crop_info).y);
        XInfoWidget(context->display,context->windows,context->text);
      }
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XCropPointerCommand(context,targets);
        continue;
      }
    XCropPointerEvent(context,targets);
  } while (((*state) & ExitState) == 0);
}

static void XCropSizeEvent(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const crop_info = targets->crop_info;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      (*crop_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
      (*crop_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
      break;
    }
    case ButtonRelease:
    {
      /*
        User has committed to cropping rectangle.
      */
      (*crop_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
      (*crop_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
      XSetCursorState(context->display,context->windows,MagickFalse);
      (*state)|=ExitState;
      context->windows->command.data=0;
      (void) XCommandWidget(context->display,context->windows,
        context->RectifyModeMenu,(XEvent *) NULL);
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      (*crop_info).x=(ssize_t) context->windows->image.x+(*event).xmotion.x;
      (*crop_info).y=(ssize_t) context->windows->image.y+(*event).xmotion.y;
    }
    default:
      break;
  }
}

static void XCropDrawRegion(const CropContext *context,
  const CropTargets *targets,RectangleInfo *highlight_info)
{
  RectangleInfo
    *const crop_info = targets->crop_info;

  (*highlight_info)=(*crop_info);
  (*highlight_info).x=(*crop_info).x-context->windows->image.x;
  (*highlight_info).y=(*crop_info).y-context->windows->image.y;
  if (((*highlight_info).width > 3) && ((*highlight_info).height > 3))
    {
      /*
        Display info and draw cropping rectangle.
      */
      XMapInfoWidget(context->display,context->windows);
      (void) FormatLocaleString(context->text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) (*crop_info).width,(double)
        (*crop_info).height,(double) (*crop_info).x,(double) (*crop_info).y);
      XInfoWidget(context->display,context->windows,context->text);
      XHighlightRectangle(context->display,context->windows->image.id,
        context->windows->image.highlight_context,highlight_info);
    }
  else
    if (context->windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(context->display,context->windows->info.id,
        context->windows->info.screen);
}

static void XCropSizeRegion(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  RectangleInfo
    highlight_info;

  do
  {
    XCropDrawRegion(context,targets,&highlight_info);
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((highlight_info.width > 3) && (highlight_info.height > 3))
      XHighlightRectangle(context->display,context->windows->image.id,
        context->windows->image.highlight_context,&highlight_info);
    XCropSizeEvent(context,targets);
    if (IsSelectionMovedOrDone((*crop_info),(*x),(*y),(*state)) != MagickFalse)
      XCheckSelectionBounds(context->windows,(*x),(*y),crop_info);
  } while (((*state) & ExitState) == 0);
}

static void XCropWaitCommand(const CropContext *context,
  const CropTargets *targets,const RectangleInfo highlight_info)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  CropCommandInput
    command_input;

  command_input.display=context->display;
  command_input.resource_info=context->resource_info;
  command_input.windows=context->windows;
  command_input.mode=context->mode;
  command_input.highlight_info=highlight_info;
  command_input.RectifyModeMenu=context->RectifyModeMenu;
  command_input.RectifyCommands=RectifyCommands;
  XCropRectifyCommand(&command_input,event,state);
}

static void XCropWaitEvent(const CropContext *context,
  const CropTargets *targets,const RectangleInfo highlight_info)
{
  CropEventInput
    event_input;

  XEvent
    *const event = targets->event;

  RectangleInfo
    *const crop_info = targets->crop_info;

  event_input.display=context->display;
  event_input.windows=context->windows;
  event_input.event=(event);
  event_input.highlight_info=highlight_info;
  event_input.text=context->text;
  event_input.resource_info=context->resource_info;
  event_input.mode=context->mode;
  switch ((*event).type)
  {
    case ButtonPress:
    {
      XCropButtonPress(&event_input,targets);
      break;
    }
    case ButtonRelease:
    {
      XCropButtonRelease(&event_input,(*crop_info));
      break;
    }
    case Expose:
    {
      XCropExpose(&event_input,event);
      break;
    }
    case KeyPress:
    {
      XCropKeyPress(&event_input,event,targets);
      break;
    }
    case MotionNotify:
    {
      XCropMotion(&event_input,targets);
      break;
    }
    case SelectionRequest:
    {
      XCropSelectionRequest(event,(*crop_info),context->text);
    }
    default:
      break;
  }
}

static MagickBooleanType XCropWaitHead(const CropContext *context,
  const CropTargets *targets,RectangleInfo *highlight_info)
{
  size_t
    *const state = targets->state;

  RectangleInfo
    *const crop_info = targets->crop_info;

  if (context->windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(context->text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) (*crop_info).width,(double)
        (*crop_info).height,(double) (*crop_info).x,(double) (*crop_info).y);
      XInfoWidget(context->display,context->windows,context->text);
    }
  (*highlight_info)=(*crop_info);
  (*highlight_info).x=(*crop_info).x-context->windows->image.x;
  (*highlight_info).y=(*crop_info).y-context->windows->image.y;
  if (((*highlight_info).width <= 3) || ((*highlight_info).height <= 3))
    {
      (*state)|=EscapeState;
      (*state)|=ExitState;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XCropWaitCorner(const CropContext *context,
  const CropTargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    highlight_info;

  do
  {
    if (XCropWaitHead(context,targets,&highlight_info) == MagickFalse)
      break;
    XHighlightRectangle(context->display,context->windows->image.id,
      context->windows->image.highlight_context,&highlight_info);
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XCropWaitCommand(context,targets,highlight_info);
        continue;
      }
    XHighlightRectangle(context->display,context->windows->image.id,
      context->windows->image.highlight_context,&highlight_info);
    XCropWaitEvent(context,targets,highlight_info);
    if (((*state) & UpdateConfigurationState) != 0)
      {
        (void) XPutBackEvent(context->display,event);
        (void) XCheckDefineCursor(context->display,context->windows->image.id,
          context->cursor);
        break;
      }
  } while (((*state) & ExitState) == 0);
}

static void XCropSelectRegion(const CropContext *context,
  const CropTargets *targets)
{
  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const crop_info = targets->crop_info;

  do
  {
    /*
      Size rectangle as pointer moves until the mouse button is released.
    */
    (*x)=(int) (*crop_info).x;
    (*y)=(int) (*crop_info).y;
    (*crop_info).width=0;
    (*crop_info).height=0;
    (*state)=DefaultState;
    XCropSizeRegion(context,targets);
    /*
      Wait for user to grab a corner of the rectangle or press return.
    */
    (*state)=DefaultState;
    (void) XMapWindow(context->display,context->windows->info.id);
    XCropWaitCorner(context,targets);
  } while (((*state) & ExitState) == 0);
}

static void XCropCutPixels(Image *image,const RectangleInfo crop_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  int
    x,
    y;

  Quantum
    *q;

  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; y < (int) crop_info.height; y++)
  {
    q=GetCacheViewAuthenticPixels(image_view,crop_info.x,y+crop_info.y,
      crop_info.width,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) crop_info.width; x++)
    {
      SetPixelAlpha(image,TransparentAlpha,q);
      q+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
  image_view=DestroyCacheView(image_view);
}

static MagickBooleanType XCropCopyImage(const CropContext *context,Image *image,
  RectangleInfo crop_info)
{
  Image
    *crop_image;

  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  /*
    Copy image before applying image transforms.
  */
  XSetCursorState(context->display,context->windows,MagickTrue);
  XCheckRefreshWindows(context->display,context->windows);
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (context->windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(context->windows->image.crop_geometry,&x,&y,&width,
      &height);
  scale_factor=(double) width/context->windows->image.ximage->width;
  crop_info.x+=x;
  crop_info.x=(ssize_t) (scale_factor*crop_info.x+0.5);
  crop_info.width=(unsigned int) (scale_factor*crop_info.width+0.5);
  scale_factor=(double) height/context->windows->image.ximage->height;
  crop_info.y+=y;
  crop_info.y=(ssize_t) (scale_factor*crop_info.y+0.5);
  crop_info.height=(unsigned int) (scale_factor*crop_info.height+0.5);
  crop_info.x+=image->page.x;
  crop_info.y+=image->page.y;
  crop_image=CropImage(image,&crop_info,context->exception);
  XSetCursorState(context->display,context->windows,MagickFalse);
  if (crop_image == (Image *) NULL)
    return(MagickFalse);
  if (context->resource_info->copy_image != (Image *) NULL)
    context->resource_info->copy_image=DestroyImage(
      context->resource_info->copy_image);
  context->resource_info->copy_image=crop_image;
  if (context->mode == CopyMode)
    {
      (void) XConfigureImage(context->display,context->resource_info,
        context->windows,image,context->exception);
      return(MagickTrue);
    }
  /*
    Cut image.
  */
  if (SetImageStorageClass(image,DirectClass,context->exception) == MagickFalse)
    return(MagickFalse);
  image->alpha_trait=BlendPixelTrait;
  XCropCutPixels(image,crop_info,context->exception);
  /*
    Update image configuration.
  */
  XConfigureImageColormap(context->display,context->resource_info,
    context->windows,image,context->exception);
  (void) XConfigureImage(context->display,context->resource_info,
    context->windows,image,context->exception);
  return(MagickTrue);
}

static void XCropSelectArea(const CropContext *context,
  const CropTargets *targets)
{
  (void) XSetFunction(context->display,
    context->windows->image.highlight_context,GXinvert);
  XCropSelectRegion(context,targets);
  (void) XSetFunction(context->display,
    context->windows->image.highlight_context,GXcopy);
  XSetCursorState(context->display,context->windows,MagickFalse);
}

static MagickBooleanType XCropEscaped(const CropContext *context,
  const CropTargets *targets)
{
  size_t
    *const state = targets->state;

  if (((*state) & EscapeState) != 0)
    {
      /*
        User want to exit without cropping.
      */
      (void) XWithdrawWindow(context->display,context->windows->info.id,
        context->windows->info.screen);
      (void) XFreeCursor(context->display,context->cursor);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XMapCropCommand(Display *display,XWindows *windows,
  const char *const *CropModeMenu)
{
  windows->command.data=0;
  (void) XCommandWidget(display,windows,CropModeMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
}

static void XSetCropCommandName(XWindows *windows,const ClipboardMode mode)
{
  switch (mode)
  {
    case CopyMode:
    {
      (void) CloneString(&windows->command.name,"Copy");
      break;
    }
    case CropMode:
    {
      (void) CloneString(&windows->command.name,"Crop");
      break;
    }
    case CutMode:
    {
      (void) CloneString(&windows->command.name,"Cut");
      break;
    }
  }
}

static MagickBooleanType XCropReconfigure(const CropContext *context,
  Image *image,RectangleInfo crop_info)
{
  if (context->mode == CropMode)
    if (((int) crop_info.width != context->windows->image.ximage->width) ||
        ((int) crop_info.height != context->windows->image.ximage->height))
      {
        /*
          Reconfigure Image window as defined by cropping rectangle.
        */
        XSetCropGeometry(context->display,context->windows,&crop_info,image);
        context->windows->image.window_changes.width=(int) crop_info.width;
        context->windows->image.window_changes.height=(int) crop_info.height;
        (void) XConfigureImage(context->display,context->resource_info,
          context->windows,image,context->exception);
        return(MagickFalse);
      }
  return(MagickTrue);
}

static MagickBooleanType XCropImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  const ClipboardMode mode,ExceptionInfo *exception)
{
  const char
    *const CropModeMenu[] =
    {
      "Help",
      "Dismiss",
      (char *) NULL
    },
    *RectifyModeMenu[] =
    {
      "Crop",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  char
    text[MagickPathExtent];

  Cursor
    cursor;

  int
    x,
    y;

  RectangleInfo
    crop_info;

  size_t
    state;

  XEvent
    event;

  CropContext
    context;

  const CropTargets
    targets = { &x, &y, &crop_info, &state, &event };

  /*
    Map Command widget.
  */
  XSetCropCommandName(windows,mode);
  RectifyModeMenu[0]=windows->command.name;
  XMapCropCommand(display,windows,CropModeMenu);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  crop_info.x=(ssize_t) windows->image.x+x;
  crop_info.y=(ssize_t) windows->image.y+y;
  crop_info.width=0;
  crop_info.height=0;
  cursor=XCreateFontCursor(display,XC_fleur);
  state=DefaultState;
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.mode=mode;
  context.CropModeMenu=CropModeMenu;
  context.exception=exception;
  context.text=text;
  context.RectifyModeMenu=RectifyModeMenu;
  context.cursor=cursor;
  XCropTrackPointer(&context,&targets);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  if (XCropEscaped(&context,&targets) == MagickFalse)
    return(MagickTrue);
  XCropSelectArea(&context,&targets);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  if (XCropReconfigure(&context,image,crop_info) == MagickFalse)
    return(MagickTrue);
  return(XCropCopyImage(&context,image,crop_info));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawEditImage() draws a graphic element (point, line, rectangle, etc.) on
%  the image.
%
%  The format of the XDrawEditImage method is:
%
%      MagickBooleanType XDrawEditImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPolygonElement(const ElementType v)
{
  switch (v)
  {
    case PointElement:
    case PolygonElement:
    case FillPolygonElement:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static inline MagickBooleanType IsBoxedElement(const ElementType v)
{
  switch (v)
  {
    case RectangleElement:
    case CircleElement:
    case EllipseElement:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static inline MagickBooleanType IsElementComplete(const ElementType element,
  const unsigned int distance)
{
  return((((element != PolygonElement) &&
    (element != FillPolygonElement)) || (distance <= 9)) ? MagickTrue :
    MagickFalse);
}

typedef struct _DrawEditContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *DrawMenu;

  char
    *command;

  Window
    root_window;

  ExceptionInfo
    *exception;

  Image
    **image;

  Cursor
    cursor;

  const ModeType
    *DrawCommands;
} DrawEditContext;

typedef struct _DrawStyleTargets
{
  XDrawInfo
    *draw_info;

  unsigned int
    *pen_id;

  unsigned int
    *line_width;

  Pixmap
    *stipple;

  int
    *x;

  int
    *y;
} DrawStyleTargets;

static void XSelectDrawColor(const DrawEditContext *context,const int id,
  const DrawStyleTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const char
    *const *const DrawMenu = context->DrawMenu;

  char
    *const command = context->command;

  XDrawInfo
    *const draw_info = targets->draw_info;

  unsigned int
    *const pen_id = targets->pen_id;

  const char
    *ColorMenu[MaxNumberPens+1];

  int
    pen_number;

  MagickBooleanType
    transparent;

  XColor
    color;

  int
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="transparent";
  ColorMenu[MaxNumberPens-1]="Browser...";
  ColorMenu[MaxNumberPens]=(char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  pen_number=XMenuWidget(display,windows,DrawMenu[id],
    (const char **) ColorMenu,command);
  if (pen_number < 0)
    return;
  transparent=pen_number == (MaxNumberPens-2) ? MagickTrue :
    MagickFalse;
  if (transparent != MagickFalse)
    {
      (*draw_info).stencil=TransparentStencil;
      return;
    }
  if (pen_number == (MaxNumberPens-1))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set pen color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],&color);
  XBestPixel(display,windows->map_info->colormap,(XColor *) NULL,
    (unsigned int) MaxColors,&color);
  windows->pixel_info->pen_colors[pen_number]=color;
  (*pen_id)=(unsigned int) pen_number;
  (*draw_info).stencil=OpaqueStencil;
}

static void XSelectDrawWidth(const DrawEditContext *context,const int id,
  const DrawStyleTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const DrawMenu = context->DrawMenu;

  char
    *const command = context->command;

  unsigned int
    *const line_width = targets->line_width;

  const char
    *const WidthsMenu[] =
    {
      "1",
      "2",
      "4",
      "8",
      "16",
      "Dialog...",
      (char *) NULL,
    };

  static char
    width[MagickPathExtent] = "0";

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  entry=XMenuWidget(display,windows,DrawMenu[id],WidthsMenu,
    command);
  if (entry < 0)
    return;
  if (entry != 5)
    {
      (*line_width)=(unsigned int) StringToUnsignedLong(
        WidthsMenu[entry]);
      return;
    }
  (void) XDialogWidget(display,windows,"Ok","Enter line width:",
    width);
  if (*width == '\0')
    return;
  (*line_width)=(unsigned int) StringToUnsignedLong(width);
}

static void XCreateStippleBitmapTail(Display *display,const Window root_window,
  const int entry,Pixmap *stipple)
{
  switch (entry)
  {
    case 4:
    {
      (*stipple)=XCreateBitmapFromData(display,root_window,
        (char *) WavyBitmap,WavyWidth,WavyHeight);
      break;
    }
    case 5:
    {
      (*stipple)=XCreateBitmapFromData(display,root_window,
        (char *) HighlightBitmap,HighlightWidth,
        HighlightHeight);
      break;
    }
    case 6:
    default:
    {
      (*stipple)=XCreateBitmapFromData(display,root_window,
        (char *) OpaqueBitmap,OpaqueWidth,OpaqueHeight);
      break;
    }
  }
}

static Pixmap XCreateStippleBitmap(Display *display,const Window root_window,
  const int entry)
{
  Pixmap
    stipple;

  switch (entry)
  {
    case 0:
    {
      stipple=XCreateBitmapFromData(display,root_window,
        (char *) BricksBitmap,BricksWidth,BricksHeight);
      break;
    }
    case 1:
    {
      stipple=XCreateBitmapFromData(display,root_window,
        (char *) DiagonalBitmap,DiagonalWidth,DiagonalHeight);
      break;
    }
    case 2:
    {
      stipple=XCreateBitmapFromData(display,root_window,
        (char *) ScalesBitmap,ScalesWidth,ScalesHeight);
      break;
    }
    case 3:
    {
      stipple=XCreateBitmapFromData(display,root_window,
        (char *) VerticalBitmap,VerticalWidth,VerticalHeight);
      break;
    }
    default:
    {
      XCreateStippleBitmapTail(display,root_window,entry,&stipple);
      break;
    }
  }
  return(stipple);
}

static void XReadStippleFile(const DrawEditContext *context,
  const DrawStyleTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const Window
    root_window = context->root_window;

  ExceptionInfo
    *const exception = context->exception;

  Pixmap
    *const stipple = targets->stipple;

  int
    *const x = targets->x;

  int
    *const y = targets->y;

  Image
    *stipple_image;

  ImageInfo
    *image_info;

  int
    status;

  static char
    filename[MagickPathExtent] = "\0";

  unsigned int
    height,
    width;

  XFileBrowserWidget(display,windows,"Stipple",filename);
  if (*filename == '\0')
    return;
  /*
    Read image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  image_info=AcquireImageInfo();
  (void) CopyMagickString(image_info->filename,filename,
    MagickPathExtent);
  stipple_image=ReadImage(image_info,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (stipple_image == (Image *) NULL)
    return;
  (void) AcquireUniqueFileResource(filename);
  (void) FormatLocaleString(stipple_image->filename,
    MagickPathExtent,"xbm:%s",filename);
  (void) WriteImage(image_info,stipple_image,exception);
  stipple_image=DestroyImage(stipple_image);
  image_info=DestroyImageInfo(image_info);
  status=XReadBitmapFile(display,root_window,filename,&width,
    &height,stipple,x,y);
  (void) RelinquishUniqueFileResource(filename);
  if ((status != BitmapSuccess) != 0)
    XNoticeWidget(display,windows,"Unable to read X bitmap image:",
      filename);
}

static void XSelectDrawStipple(const DrawEditContext *context,const int id,
  const DrawStyleTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const DrawMenu = context->DrawMenu;

  char
    *const command = context->command;

  const Window
    root_window = context->root_window;

  Pixmap
    *const stipple = targets->stipple;

  const char
    *StipplesMenu[] =
    {
      "Brick",
      "Diagonal",
      "Scales",
      "Vertical",
      "Wavy",
      "Translucent",
      "Opaque",
      (char *) NULL,
      (char *) NULL,
    };

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  StipplesMenu[7]="Open...";
  entry=XMenuWidget(display,windows,DrawMenu[id],StipplesMenu,
    command);
  if (entry < 0)
    return;
  if ((*stipple) != (Pixmap) NULL)
    (void) XFreePixmap(display,(*stipple));
  (*stipple)=(Pixmap) NULL;
  if (entry != 7)
    {
      (*stipple)=XCreateStippleBitmap(display,root_window,entry);
      return;
    }
  XReadStippleFile(context,targets);
}

typedef struct _DrawPointerTargets
{
  int
    *x;

  int
    *y;

  size_t
    *state;

  XEvent
    *event;

  ElementType
    *element;
} DrawPointerTargets;

static void XDrawButtonPress(const DrawEditContext *context,
  const XEvent event,const DrawPointerTargets *targets)
{
  XWindows
    *const windows = context->windows;

  int
    *const x = targets->x;

  int
    *const y = targets->y;

  size_t
    *const state = targets->state;

  if (event.xbutton.button != Button1)
    return;
  if (event.xbutton.window != windows->image.id)
    return;
  /*
    exit loop.
  */
  (*x)=event.xbutton.x;
  (*y)=event.xbutton.y;
  (*state)|=ExitState;
}

static void XDrawKeyPress(const DrawEditContext *context,
  const DrawPointerTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = targets->state;

  XEvent
    *const event = targets->event;

  KeySym
    key_symbol;

  char
    command[MagickPathExtent];

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Rotation",ImageDrawHelp);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static RectangleInfo XFitSelectionRectangle(XWindows *windows,const int x,
  const int y,RectangleInfo rectangle_info)
{
  if (rectangle_info.x < 0)
    rectangle_info.x=0;
  else
    if (rectangle_info.x > (ssize_t) windows->image.width)
      rectangle_info.x=(ssize_t) windows->image.width;
  if ((int) rectangle_info.x < x)
    rectangle_info.width=(unsigned int) (x-rectangle_info.x);
  else
    {
      rectangle_info.width=(unsigned int) (rectangle_info.x-x);
      rectangle_info.x=(ssize_t) x;
    }
  if (rectangle_info.y < 0)
    rectangle_info.y=0;
  else
    if (rectangle_info.y > (ssize_t) windows->image.height)
      rectangle_info.y=(ssize_t) windows->image.height;
  if ((int) rectangle_info.y < y)
    rectangle_info.height=(unsigned int) (y-rectangle_info.y);
  else
    {
      rectangle_info.height=(unsigned int) (rectangle_info.y-y);
      rectangle_info.y=(ssize_t) y;
    }
  return(rectangle_info);
}

static RectangleInfo XDrawPolygonBounds(XPoint *coordinate_info,
  const int number_coordinates,RectangleInfo rectangle_info)
{
  int
    i,
    x,
    y;

  /*
    Determine polygon bounding box.
  */
  rectangle_info.x=(ssize_t) coordinate_info->x;
  rectangle_info.y=(ssize_t) coordinate_info->y;
  x=coordinate_info->x;
  y=coordinate_info->y;
  for (i=1; i < number_coordinates; i++)
  {
    if (coordinate_info[i].x > x)
      x=coordinate_info[i].x;
    if (coordinate_info[i].y > y)
      y=coordinate_info[i].y;
    if ((ssize_t) coordinate_info[i].x < rectangle_info.x)
      rectangle_info.x=MagickMax((ssize_t) coordinate_info[i].x,0);
    if ((ssize_t) coordinate_info[i].y < rectangle_info.y)
      rectangle_info.y=MagickMax((ssize_t) coordinate_info[i].y,0);
  }
  rectangle_info.width=(size_t) (x-rectangle_info.x);
  rectangle_info.height=(size_t) (y-rectangle_info.y);
  for (i=0; i < number_coordinates; i++)
  {
    coordinate_info[i].x-=rectangle_info.x;
    coordinate_info[i].y-=rectangle_info.y;
  }
  return(rectangle_info);
}

typedef struct _DrawShapeInput
{
  ElementType
    element;

  unsigned int
    distance;
} DrawShapeInput;

typedef struct _DrawShapeTargets
{
  XSegment
    *line_info;

  RectangleInfo
    *rectangle_info;

  XPoint
    **coordinate_info;

  size_t
    *state;

  int
    *number_coordinates;

  unsigned int
    *max_coordinates;
} DrawShapeTargets;

static void XDrawButtonRelease(const DrawEditContext *context,
  const DrawShapeInput *shape,const XEvent event,
  const DrawShapeTargets *targets)
{
  ExceptionInfo
    *const exception = context->exception;

  const ElementType
    element = shape->element;

  const unsigned int
    distance = shape->distance;

  XSegment
    *const line_info = targets->line_info;

  RectangleInfo
    *const rectangle_info = targets->rectangle_info;

  XPoint
    **const coordinate_info = targets->coordinate_info;

  size_t
    *const state = targets->state;

  int
    *const number_coordinates = targets->number_coordinates;

  unsigned int
    *const max_coordinates = targets->max_coordinates;

  /*
    User has committed to element.
  */
  (*line_info).x2=event.xbutton.x;
  (*line_info).y2=event.xbutton.y;
  (*rectangle_info).x=(ssize_t) event.xbutton.x;
  (*rectangle_info).y=(ssize_t) event.xbutton.y;
  (*coordinate_info)[(*number_coordinates)].x=event.xbutton.x;
  (*coordinate_info)[(*number_coordinates)].y=event.xbutton.y;
  if (IsElementComplete(element,distance) != MagickFalse)
    {
      (*state)|=ExitState;
      return;
    }
  (*number_coordinates)++;
  if ((*number_coordinates) < (int) (*max_coordinates))
    {
      (*line_info).x1=event.xbutton.x;
      (*line_info).y1=event.xbutton.y;
      return;
    }
  (*max_coordinates)<<=1;
  (*coordinate_info)=(XPoint *) ResizeQuantumMemory((*coordinate_info),
    (*max_coordinates),sizeof(*(*coordinate_info)));
  if ((*coordinate_info) == (XPoint *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'","...");
}

static void XDrawMotion(const DrawEditContext *context,
  const DrawShapeInput *shape,const XEvent event,
  const DrawShapeTargets *targets)
{
  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  const ElementType
    element = shape->element;

  XSegment
    *const line_info = targets->line_info;

  RectangleInfo
    *const rectangle_info = targets->rectangle_info;

  XPoint
    **const coordinate_info = targets->coordinate_info;

  int
    *const number_coordinates = targets->number_coordinates;

  unsigned int
    *const max_coordinates = targets->max_coordinates;

  if (event.xmotion.window != windows->image.id)
    return;
  if (element != PointElement)
    {
      (*line_info).x2=event.xmotion.x;
      (*line_info).y2=event.xmotion.y;
      (*rectangle_info).x=(ssize_t) event.xmotion.x;
      (*rectangle_info).y=(ssize_t) event.xmotion.y;
      return;
    }
  (*coordinate_info)[(*number_coordinates)].x=event.xbutton.x;
  (*coordinate_info)[(*number_coordinates)].y=event.xbutton.y;
  (*number_coordinates)++;
  if ((*number_coordinates) < (int) (*max_coordinates))
    return;
  (*max_coordinates)<<=1;
  (*coordinate_info)=(XPoint *) ResizeQuantumMemory((*coordinate_info),
    (*max_coordinates),sizeof(*(*coordinate_info)));
  if ((*coordinate_info) == (XPoint *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),
      ResourceLimitError,"MemoryAllocationFailed","`%s'","...");
}

typedef struct _DrawPreviewInput
{
  ElementType
    element;

  int
    number_coordinates;

  XPoint
    *coordinate_info;

  unsigned int
    distance;

  XSegment
    line_info;

  RectangleInfo
    rectangle_info;
} DrawPreviewInput;

static void XPreviewLine(const DrawEditContext *context,
  const DrawPreviewInput *preview,double *degrees)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    distance = preview->distance;

  const XSegment
    line_info = preview->line_info;

  char
    text[MagickPathExtent];

  if (distance > 9)
    {
      /*
        Display angle of the line.
      */
      (*degrees)=RadiansToDegrees(-atan2((double) (line_info.y2-
        line_info.y1),(double) (line_info.x2-line_info.x1)));
      (void) FormatLocaleString(text,MagickPathExtent," %g",
        (double) (*degrees));
      XInfoWidget(display,windows,text);
      XHighlightLine(display,windows->image.id,
        windows->image.highlight_context,&line_info);
    }
  else
    if (windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
}

static void XPreviewPolyline(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const int
    number_coordinates = preview->number_coordinates;

  XPoint
    *const coordinate_info = preview->coordinate_info;

  if (number_coordinates > 1)
    (void) XDrawLines(display,windows->image.id,
      windows->image.highlight_context,coordinate_info,
      number_coordinates,CoordModeOrigin);
}

static void XPreviewLongLine(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const unsigned int
    distance = preview->distance;

  const XSegment
    line_info = preview->line_info;

  if (distance > 9)
    XHighlightLine(display,windows->image.id,
      windows->image.highlight_context,&line_info);
}

static void XPreviewPoint(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const int
    number_coordinates = preview->number_coordinates;

  XPoint
    *const coordinate_info = preview->coordinate_info;

  char
    text[MagickPathExtent];

  if (number_coordinates > 1)
    {
      (void) XDrawLines(display,windows->image.id,
        windows->image.highlight_context,coordinate_info,
        number_coordinates,CoordModeOrigin);
      (void) FormatLocaleString(text,MagickPathExtent," %+d%+d",
        coordinate_info[number_coordinates-1].x,
        coordinate_info[number_coordinates-1].y);
      XInfoWidget(display,windows,text);
    }
}

static void XPreviewRectangle(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const RectangleInfo
    rectangle_info = preview->rectangle_info;

  char
    text[MagickPathExtent];

  if ((rectangle_info.width > 3) && (rectangle_info.height > 3))
    {
      /*
        Display info and draw drawing rectangle.
      */
      (void) FormatLocaleString(text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) rectangle_info.width,
        (double) rectangle_info.height,(double) rectangle_info.x,
        (double) rectangle_info.y);
      XInfoWidget(display,windows,text);
      XHighlightRectangle(display,windows->image.id,
        windows->image.highlight_context,&rectangle_info);
    }
  else
    if (windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
}

static void XPreviewEllipse(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const RectangleInfo
    rectangle_info = preview->rectangle_info;

  char
    text[MagickPathExtent];

  if ((rectangle_info.width > 3) && (rectangle_info.height > 3))
    {
      /*
        Display info and draw drawing rectangle.
      */
      (void) FormatLocaleString(text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) rectangle_info.width,
        (double) rectangle_info.height,(double) rectangle_info.x,
        (double) rectangle_info.y);
      XInfoWidget(display,windows,text);
      XHighlightEllipse(display,windows->image.id,
        windows->image.highlight_context,&rectangle_info);
    }
  else
    if (windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->info.id,
        windows->info.screen);
}

static void XPreviewRectangleErase(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const RectangleInfo
    rectangle_info = preview->rectangle_info;

  if ((rectangle_info.width > 3) && (rectangle_info.height > 3))
    XHighlightRectangle(display,windows->image.id,
      windows->image.highlight_context,&rectangle_info);
}

static void XPreviewEllipseErase(const DrawEditContext *context,
  const DrawPreviewInput *preview)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const RectangleInfo
    rectangle_info = preview->rectangle_info;

  if ((rectangle_info.width > 3) && (rectangle_info.height > 3))
    XHighlightEllipse(display,windows->image.id,
      windows->image.highlight_context,&rectangle_info);
}

static void XDrawCommandAction(const DrawEditContext *context,const int id,
  const DrawPointerTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  const Cursor
    cursor = context->cursor;

  const ModeType
    *const DrawCommands = context->DrawCommands;

  size_t
    *const state = targets->state;

  switch (DrawCommands[id])
  {
    case DrawUndoCommand:
    {
      (void) XMagickCommand(display,resource_info,windows,UndoCommand,
        image,exception);
      break;
    }
    case DrawHelpCommand:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Rotation",ImageDrawHelp);
      (void) XCheckDefineCursor(display,windows->image.id,cursor);
      break;
    }
    case DrawDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static void XDrawCommand(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawStyleTargets *style_targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const DrawMenu = context->DrawMenu;

  XEvent
    *const event = pointer_targets->event;

  ElementType
    *const element = pointer_targets->element;

  int
    id;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(display,windows,DrawMenu,event);
  if (id < 0)
    return;
  switch (context->DrawCommands[id])
  {
    case DrawElementCommand:
    {
      const char
        *const Elements[] =
        {
          "point",
          "line",
          "rectangle",
          "fill rectangle",
          "circle",
          "fill circle",
          "ellipse",
          "fill ellipse",
          "polygon",
          "fill polygon",
          (char *) NULL,
        };

      /*
        Select a command from the pop-up menu.
      */
      (*element)=(ElementType) (XMenuWidget(display,windows,
        DrawMenu[id],Elements,context->command)+1);
      break;
    }
    case DrawColorCommand:
    {
      XSelectDrawColor(context,id,style_targets);
      break;
    }
    case DrawStippleCommand:
    {
      XSelectDrawStipple(context,id,style_targets);
      break;
    }
    case DrawWidthCommand:
    {
      XSelectDrawWidth(context,id,style_targets);
      break;
    }
    default:
    {
      XDrawCommandAction(context,id,pointer_targets);
      break;
    }
  }
  (void) XCheckDefineCursor(display,windows->image.id,context->cursor);
}

static void XDrawShapeEvent(const DrawEditContext *context,
  const DrawShapeInput *shape,const XEvent event,
  const DrawShapeTargets *targets)
{
  switch (event.type)
  {
    case ButtonPress:
      break;
    case ButtonRelease:
    {
      XDrawButtonRelease(context,shape,event,targets);
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      XDrawMotion(context,shape,event,targets);
      break;
    }
    default:
      break;
  }
}

static void XClampLineEnd(const XWindows *windows,XSegment *line_info)
{
  if ((*line_info).x2 < 0)
    (*line_info).x2=0;
  else
    if ((*line_info).x2 > (int) windows->image.width)
      (*line_info).x2=(short) windows->image.width;
  if ((*line_info).y2 < 0)
    (*line_info).y2=0;
  else
    if ((*line_info).y2 > (int) windows->image.height)
      (*line_info).y2=(short) windows->image.height;
}

static void XSizeDrawRegion(const DrawEditContext *context,
  XDrawInfo *draw_info,const unsigned int line_width,
  const RectangleInfo *rectangle_info)
{
  Image
    **const image = context->image;

  XWindows
    *const windows = context->windows;

  int
    x,
    y;

  unsigned int
    height,
    width;

  width=(unsigned int) (*image)->columns;
  height=(unsigned int) (*image)->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  (*draw_info).x+=windows->image.x-((int) line_width/2);
  if ((*draw_info).x < 0)
    (*draw_info).x=0;
  (*draw_info).x=(int) width*(*draw_info).x/windows->image.ximage->width;
  (*draw_info).y+=windows->image.y-((int) line_width/2);
  if ((*draw_info).y < 0)
    (*draw_info).y=0;
  (*draw_info).y=(int) height*(*draw_info).y/windows->image.ximage->height;
  (*draw_info).width=(unsigned int) (*rectangle_info).width+(line_width << 1);
  if ((*draw_info).width > (unsigned int) (*image)->columns)
    (*draw_info).width=(unsigned int) (*image)->columns;
  (*draw_info).height=(unsigned int) (*rectangle_info).height+(line_width << 1);
  if ((*draw_info).height > (unsigned int) (*image)->rows)
    (*draw_info).height=(unsigned int) (*image)->rows;
  (void) FormatLocaleString((*draw_info).geometry,MagickPathExtent,"%ux%u%+d%+d",
    width*(*draw_info).width/(unsigned int) windows->image.ximage->width,
    height*(*draw_info).height/(unsigned int) windows->image.ximage->height,
    (*draw_info).x+x,(*draw_info).y+y);
}

static void XSetDrawLineInfo(XDrawInfo *draw_info,const XSegment *line_info,
  const unsigned int line_width)
{
  (*draw_info).line_info=(*line_info);
  if ((*line_info).x1 > (int) (line_width/2))
    (*draw_info).line_info.x1=(short) line_width/2;
  if ((*line_info).y1 > (int) (line_width/2))
    (*draw_info).line_info.y1=(short) line_width/2;
  (*draw_info).line_info.x2=(short) ((*line_info).x2-(*line_info).x1+
    ((int) line_width/2));
  (*draw_info).line_info.y2=(short) ((*line_info).y2-(*line_info).y1+
    ((int) line_width/2));
  if (((*draw_info).line_info.x2 < 0) && ((*draw_info).line_info.y2 < 0))
    {
      (*draw_info).line_info.x2=(-(*draw_info).line_info.x2);
      (*draw_info).line_info.y2=(-(*draw_info).line_info.y2);
    }
  if ((*draw_info).line_info.x2 < 0)
    {
      (*draw_info).line_info.x2=(-(*draw_info).line_info.x2);
      Swap((*draw_info).line_info.x1,(*draw_info).line_info.x2);
    }
  if ((*draw_info).line_info.y2 < 0)
    {
      (*draw_info).line_info.y2=(-(*draw_info).line_info.y2);
      Swap((*draw_info).line_info.y1,(*draw_info).line_info.y2);
    }
}

static void XDrawStartEvent(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  int
    *const x = pointer_targets->x;

  int
    *const y = pointer_targets->y;

  XEvent
    *const event = pointer_targets->event;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XDrawButtonPress(context,(*event),pointer_targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      XDrawKeyPress(context,pointer_targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(display,windows,(*x),(*y));
      break;
    }
  }
}

static void XWaitForDrawStart(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawStyleTargets *style_targets)
{
  const Cursor
    cursor = context->cursor;

  Display
    *const display = context->display;

  ExceptionInfo
    *const exception = context->exception;

  XWindows
    *const windows = context->windows;

  int
    *const x = pointer_targets->x;

  int
    *const y = pointer_targets->y;

  size_t
    *const state = pointer_targets->state;

  XEvent
    *const event = pointer_targets->event;

  char
    text[MagickPathExtent];

  XQueryPosition(display,windows->image.id,x,y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (*state)=DefaultState;
  do
  {
    if (windows->info.mapped != MagickFalse)
      {
        /*
          Display pointer position.
        */
        (void) FormatLocaleString(text,MagickPathExtent," %+d%+d ",
          (*x)+windows->image.x,(*y)+windows->image.y);
        XInfoWidget(display,windows,text);
      }
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    if ((*event).xany.window == windows->command.id)
      {
        XDrawCommand(context,pointer_targets,style_targets);
        continue;
      }
    XDrawStartEvent(context,pointer_targets);
  } while (((*state) & ExitState) == 0);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

static void XPreviewPointLineBox(const DrawEditContext *context,
  const DrawPreviewInput *preview,const ElementType element,double *degrees)
{
  switch (element)
  {
    case PointElement:
    default:
    {
      XPreviewPoint(context,preview);
      break;
    }
    case LineElement:
    {
      XPreviewLine(context,preview,degrees);
      break;
    }
    case RectangleElement:
    case FillRectangleElement:
    {
      XPreviewRectangle(context,preview);
      break;
    }
  }
}

static void XPreviewElement(const DrawEditContext *context,
  const DrawPreviewInput *preview,const ElementType element,double *degrees)
{
  switch (element)
  {
    default:
    {
      XPreviewPointLineBox(context,preview,element,degrees);
      break;
    }
    case CircleElement:
    case FillCircleElement:
    case EllipseElement:
    case FillEllipseElement:
    {
      XPreviewEllipse(context,preview);
      break;
    }
    case PolygonElement:
    case FillPolygonElement:
    {
      XPreviewPolyline(context,preview);
      XPreviewLine(context,preview,degrees);
      break;
    }
  }
}

static void XErasePointLineBox(const DrawEditContext *context,
  const DrawPreviewInput *preview,const ElementType element)
{
  switch (element)
  {
    case PointElement:
    default:
    {
      XPreviewPolyline(context,preview);
      break;
    }
    case LineElement:
    {
      XPreviewLongLine(context,preview);
      break;
    }
    case RectangleElement:
    case FillRectangleElement:
    {
      XPreviewRectangleErase(context,preview);
      break;
    }
  }
}

static void XErasePreviewElement(const DrawEditContext *context,
  const DrawPreviewInput *preview,const ElementType element)
{
  switch (element)
  {
    default:
    {
      XErasePointLineBox(context,preview,element);
      break;
    }
    case CircleElement:
    case FillCircleElement:
    case EllipseElement:
    case FillEllipseElement:
    {
      XPreviewEllipseErase(context,preview);
      break;
    }
    case PolygonElement:
    case FillPolygonElement:
    {
      XPreviewPolyline(context,preview);
      XPreviewLongLine(context,preview);
      break;
    }
  }
}

static void XStartDrawElement(const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets)
{
  int
    *const x = pointer_targets->x;

  int
    *const y = pointer_targets->y;

  XSegment
    *const line_info = shape_targets->line_info;

  RectangleInfo
    *const rectangle_info = shape_targets->rectangle_info;

  XPoint
    **const coordinate_info = shape_targets->coordinate_info;

  int
    *const number_coordinates = shape_targets->number_coordinates;

  (*line_info).x1=(*x);
  (*line_info).y1=(*y);
  (*line_info).x2=(*x);
  (*line_info).y2=(*y);
  (*rectangle_info).x=(ssize_t) (*x);
  (*rectangle_info).y=(ssize_t) (*y);
  (*rectangle_info).width=0;
  (*rectangle_info).height=0;
  (*number_coordinates)=1;
  (*coordinate_info)->x=(*x);
  (*coordinate_info)->y=(*y);
}

static void XTrackDrawElement(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets,unsigned int *distance)
{
  Display
    *const display = context->display;

  ExceptionInfo
    *const exception = context->exception;

  XWindows
    *const windows = context->windows;

  int
    *const x = pointer_targets->x;

  int
    *const y = pointer_targets->y;

  size_t
    *const state = pointer_targets->state;

  XEvent
    *const event = pointer_targets->event;

  ElementType
    *const element = pointer_targets->element;

  XSegment
    *const line_info = shape_targets->line_info;

  RectangleInfo
    *const rectangle_info = shape_targets->rectangle_info;

  XPoint
    **const coordinate_info = shape_targets->coordinate_info;

  int
    *const number_coordinates = shape_targets->number_coordinates;

  double
    degrees;

  DrawPreviewInput
    preview_input;

  DrawShapeInput
    shape_input;

  degrees=0.0;
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  (*state)=DefaultState;
  do
  {
    preview_input.element=(*element);
    preview_input.number_coordinates=(*number_coordinates);
    preview_input.coordinate_info=(*coordinate_info);
    preview_input.distance=(*distance);
    preview_input.line_info=(*line_info);
    preview_input.rectangle_info=(*rectangle_info);
    XPreviewElement(context,&preview_input,(*element),&degrees);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    XErasePreviewElement(context,&preview_input,(*element));
    shape_input.element=(*element);
    shape_input.distance=(*distance);
    XDrawShapeEvent(context,&shape_input,(*event),shape_targets);
    /*
      Check boundary conditions.
    */
    XClampLineEnd(windows,line_info);
    (*distance)=(unsigned int)
      ((((*line_info).x2-(*line_info).x1+1)*
        ((*line_info).x2-(*line_info).x1+1))+
       (((*line_info).y2-(*line_info).y1+1)*
        ((*line_info).y2-(*line_info).y1+1)));
    if (IsSelectionMovedOrDone((*rectangle_info),(*x),(*y),(*state)) !=
        MagickFalse)
      {
        (*rectangle_info)=XFitSelectionRectangle(windows,(*x),(*y),
          (*rectangle_info));
      }
  } while (((*state) & ExitState) == 0);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
}

static MagickStatusType XDrawElementImage(const DrawEditContext *context,
  XDrawInfo *draw_info)
{
  Display
    *const display = context->display;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  MagickStatusType
    status;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  status=XDrawImage(display,windows->pixel_info,draw_info,*image,exception);
  XSetCursorState(display,windows,MagickFalse);
  /*
    Update image colormap and return to image drawing.
  */
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
  return(status);
}

static MagickBooleanType XFitDrawBounds(
  const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets,const unsigned int distance)
{
  ElementType
    *const element = pointer_targets->element;

  RectangleInfo
    *const rectangle_info = shape_targets->rectangle_info;

  XPoint
    **const coordinate_info = shape_targets->coordinate_info;

  int
    *const number_coordinates = shape_targets->number_coordinates;

  if (IsPolygonElement((*element)) != MagickFalse)
    {
      (*rectangle_info)=XDrawPolygonBounds((*coordinate_info),
        (*number_coordinates),(*rectangle_info));
    }
  else
    if (distance <= 9)
      return(MagickTrue);
    else
      if (IsBoxedElement((*element)) != MagickFalse)
        {
          (*rectangle_info).width--;
          (*rectangle_info).height--;
        }
  return(MagickFalse);
}

static void XPrepareDrawInfo(const DrawEditContext *context,
  const DrawPointerTargets *pointer_targets,
  const DrawShapeTargets *shape_targets,const DrawStyleTargets *style_targets)
{
  Display
    *const display = context->display;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ElementType
    *const element = pointer_targets->element;

  XSegment
    *const line_info = shape_targets->line_info;

  RectangleInfo
    *const rectangle_info = shape_targets->rectangle_info;

  XPoint
    **const coordinate_info = shape_targets->coordinate_info;

  int
    *const number_coordinates = shape_targets->number_coordinates;

  XDrawInfo
    *const draw_info = style_targets->draw_info;

  unsigned int
    *const pen_id = style_targets->pen_id;

  unsigned int
    *const line_width = style_targets->line_width;

  Pixmap
    *const stipple = style_targets->stipple;

  (*draw_info).x=(int) (*rectangle_info).x;
  (*draw_info).y=(int) (*rectangle_info).y;
  (void) XMagickCommand(display,resource_info,windows,SaveToUndoBufferCommand,
    image,exception);
  XSizeDrawRegion(context,draw_info,(*line_width),rectangle_info);
  /*
    Initialize drawing attributes.
  */
  (*draw_info).degrees=0.0;
  (*draw_info).element=(*element);
  (*draw_info).stipple=(*stipple);
  (*draw_info).line_width=(*line_width);
  XSetDrawLineInfo(draw_info,line_info,(*line_width));
  (*draw_info).rectangle_info=(*rectangle_info);
  if ((*draw_info).rectangle_info.x > (ssize_t) ((*line_width)/2))
    (*draw_info).rectangle_info.x=(ssize_t) (*line_width)/2;
  if ((*draw_info).rectangle_info.y > (ssize_t) ((*line_width)/2))
    (*draw_info).rectangle_info.y=(ssize_t) (*line_width)/2;
  (*draw_info).number_coordinates=(unsigned int) (*number_coordinates);
  (*draw_info).coordinate_info=(*coordinate_info);
  windows->pixel_info->pen_color=windows->pixel_info->pen_colors[(*pen_id)];
}

static void XMapDrawCommandWidget(Display *display,XWindows *windows,
  const char *const *DrawMenu)
{
  (void) CloneString(&windows->command.name,"Draw");
  windows->command.data=4;
  (void) XCommandWidget(display,windows,DrawMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
}

static MagickBooleanType XAcquireDrawCoordinates(ExceptionInfo *exception,
  const DrawShapeTargets *targets)
{
  XPoint
    **const coordinate_info = targets->coordinate_info;

  unsigned int
    *const max_coordinates = targets->max_coordinates;

  (*max_coordinates)=2048;
  (*coordinate_info)=(XPoint *) AcquireQuantumMemory((size_t) (*max_coordinates),
    sizeof(*(*coordinate_info)));
  if ((*coordinate_info) == (XPoint *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'","...");
      return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType XDrawEditImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  const char
    *const DrawMenu[] =
    {
      "Element",
      "Color",
      "Stipple",
      "Width",
      "Undo",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static ElementType
    element = PointElement;

  static const ModeType
    DrawCommands[] =
    {
      DrawElementCommand,
      DrawColorCommand,
      DrawStippleCommand,
      DrawWidthCommand,
      DrawUndoCommand,
      DrawHelpCommand,
      DrawDismissCommand
    };

  static Pixmap
    stipple = (Pixmap) NULL;

  static unsigned int
    pen_id = 0,
    line_width = 1;

  char
    command[MagickPathExtent];

  Cursor
    cursor;

  int
    number_coordinates,
    x,
    y;

  MagickStatusType
    status;

  RectangleInfo
    rectangle_info;

  unsigned int
    distance,
    max_coordinates;

  size_t
    state;

  Window
    root_window;

  XDrawInfo
    draw_info;

  XEvent
    event;

  XPoint
    *coordinate_info;

  XSegment
    line_info;

  DrawEditContext
    draw_context;

  const DrawShapeTargets
    shape_targets = { &line_info, &rectangle_info, &coordinate_info, &state,
      &number_coordinates, &max_coordinates };

  const DrawPointerTargets
    pointer_targets = { &x, &y, &state, &event, &element };

  const DrawStyleTargets
    style_targets = { &draw_info, &pen_id, &line_width, &stipple, &x, &y };

  /*
    Allocate polygon info.
  */
  if (XAcquireDrawCoordinates(exception,&shape_targets) == MagickFalse)
    return(MagickFalse);
  /*
    Map Command widget.
  */
  XMapDrawCommandWidget(display,windows,DrawMenu);
  /*
    Wait for first button press.
  */
  root_window=XRootWindow(display,XDefaultScreen(display));
  draw_info.stencil=OpaqueStencil;
  status=MagickTrue;
  cursor=XCreateFontCursor(display,XC_tcross);
  draw_context.display=display;
  draw_context.resource_info=resource_info;
  draw_context.windows=windows;
  draw_context.DrawMenu=DrawMenu;
  draw_context.command=command;
  draw_context.root_window=root_window;
  draw_context.exception=exception;
  draw_context.image=image;
  draw_context.cursor=cursor;
  draw_context.DrawCommands=DrawCommands;
  for ( ; ; )
  {
    XWaitForDrawStart(&draw_context,&pointer_targets,&style_targets);
    if ((state & EscapeState) != 0)
      break;
    /*
      Draw element as pointer moves until the button is released.
    */
    distance=0;
    XStartDrawElement(&pointer_targets,&shape_targets);
    XTrackDrawElement(&draw_context,&pointer_targets,&shape_targets,&distance);
    if (XFitDrawBounds(&pointer_targets,&shape_targets,distance) != MagickFalse)
      continue;
    /*
      Drawing is relative to image configuration.
    */
    XPrepareDrawInfo(&draw_context,&pointer_targets,&shape_targets,
      &style_targets);
    /*
      Draw element on image.
    */
    status=XDrawElementImage(&draw_context,&draw_info);
  }
  XSetCursorState(display,windows,MagickFalse);
  coordinate_info=(XPoint *) RelinquishMagickMemory(coordinate_info);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D r a w P a n R e c t a n g l e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDrawPanRectangle() draws a rectangle in the pan window.  The pan window
%  displays a zoom image and the rectangle shows which portion of the image is
%  displayed in the Image window.
%
%  The format of the XDrawPanRectangle method is:
%
%      XDrawPanRectangle(Display *display,XWindows *windows)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
*/
static void XDrawPanRectangle(Display *display,XWindows *windows)
{
  double
    scale_factor;

  RectangleInfo
    highlight_info;

  /*
    Determine dimensions of the panning rectangle.
  */
  scale_factor=(double) windows->pan.width/windows->image.ximage->width;
  highlight_info.x=(ssize_t) (scale_factor*windows->image.x+0.5);
  highlight_info.width=(unsigned int) (scale_factor*windows->image.width+0.5);
  scale_factor=(double)
    windows->pan.height/windows->image.ximage->height;
  highlight_info.y=(ssize_t) (scale_factor*windows->image.y+0.5);
  highlight_info.height=(unsigned int) (scale_factor*windows->image.height+0.5);
  /*
    Display the panning rectangle.
  */
  (void) XClearWindow(display,windows->pan.id);
  XHighlightRectangle(display,windows->pan.id,windows->pan.annotate_context,
    &highlight_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X I m a g e C a c h e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XImageCache() handles the creation, manipulation, and destruction of the
%  image cache (undo and redo buffers).
%
%  The format of the XImageCache method is:
%
%      void XImageCache(Display *display,XResourceInfo *resource_info,
%        XWindows *windows,const DisplayCommand command,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o command: Specifies a command to perform.
%
%    o image: the image;  XImageCache may transform the image and return a new
%      image pointer.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XTrimUndoCache(const XResourceInfo *resource_info,size_t *bytes,
  Image **undo_image)
{
  Image
    *cache_image,
    *previous_image;

  if (*undo_image != (Image *) NULL)
    {
      /*
        Ensure the undo cache has enough memory available.
      */
      previous_image=*undo_image;
      while (previous_image != (Image *) NULL)
      {
        *bytes+=previous_image->list->columns*previous_image->list->rows*
          sizeof(PixelInfo);
        if (*bytes <= (resource_info->undo_cache << 20))
          {
            previous_image=GetPreviousImageInList(previous_image);
            continue;
          }
        *bytes-=previous_image->list->columns*previous_image->list->rows*
          sizeof(PixelInfo);
        if (previous_image == *undo_image)
          *undo_image=NewImageList();
        else
          previous_image->next->previous=NewImageList();
        break;
      }
      while (previous_image != (Image *) NULL)
      {
        /*
          Delete any excess memory from undo cache.
        */
        cache_image=previous_image;
        previous_image=GetPreviousImageInList(previous_image);
        cache_image->list=DestroyImage(cache_image->list);
        cache_image=DestroyImage(cache_image);
      }
    }
}

typedef struct _XImageCacheContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  ExceptionInfo
    *exception;
} XImageCacheContext;

static Image *XSaveUndoImage(const XImageCacheContext *cache_context,
  Image *undo_image)
{
  Display
    *const display = cache_context->display;

  XResourceInfo
    *const resource_info = cache_context->resource_info;

  XWindows
    *const windows = cache_context->windows;

  Image
    **const image = cache_context->image;

  ExceptionInfo
    *const exception = cache_context->exception;

  Image
    *cache_image;

  size_t
    bytes;

  bytes=(*image)->columns*(*image)->rows*sizeof(PixelInfo);
  XTrimUndoCache(resource_info,&bytes,&undo_image);
  if (bytes > (resource_info->undo_cache << 20))
    return(undo_image);
  /*
    Save image before transformations are applied.
  */
  cache_image=AcquireImage((ImageInfo *) NULL,exception);
  if (cache_image == (Image *) NULL)
    return(undo_image);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  cache_image->list=CloneImage(*image,0,0,MagickTrue,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (cache_image->list == (Image *) NULL)
    {
      cache_image=DestroyImage(cache_image);
      return(undo_image);
    }
  cache_image->columns=(size_t) windows->image.ximage->width;
  cache_image->rows=(size_t) windows->image.ximage->height;
  cache_image->geometry=windows->image.crop_geometry;
  if (windows->image.crop_geometry != (char *) NULL)
    {
      cache_image->geometry=AcquireString((char *) NULL);
      (void) CopyMagickString(cache_image->geometry,
        windows->image.crop_geometry,MagickPathExtent);
    }
  if (undo_image == (Image *) NULL)
    {
      undo_image=cache_image;
      return(undo_image);
    }
  undo_image->next=cache_image;
  undo_image->next->previous=undo_image;
  undo_image=undo_image->next;
  return(undo_image);
}

static const DisplayCommand
  UndoCacheCommands[] =
  {
    CutCommand,
    PasteCommand,
    ApplyCommand,
    HalfSizeCommand,
    OriginalSizeCommand,
    DoubleSizeCommand,
    ResizeCommand,
    TrimCommand,
    CropCommand,
    ChopCommand,
    FlipCommand,
    FlopCommand,
    RotateRightCommand,
    RotateLeftCommand,
    RotateCommand,
    ShearCommand,
    RollCommand,
    NegateCommand,
    ContrastStretchCommand,
    SigmoidalContrastCommand,
    NormalizeCommand,
    EqualizeCommand,
    HueCommand,
    SaturationCommand,
    BrightnessCommand,
    GammaCommand,
    SpiffCommand,
    DullCommand,
    GrayscaleCommand,
    MapCommand,
    QuantizeCommand,
    DespeckleCommand,
    EmbossCommand,
    ReduceNoiseCommand,
    AddNoiseCommand,
    SharpenCommand,
    BlurCommand,
    ThresholdCommand,
    EdgeDetectCommand,
    SpreadCommand,
    ShadeCommand,
    RaiseCommand,
    SegmentCommand,
    SolarizeCommand,
    SepiaToneCommand,
    SwirlCommand,
    ImplodeCommand,
    VignetteCommand,
    WaveCommand,
    OilPaintCommand,
    CharcoalDrawCommand,
    AnnotateCommand,
    AddBorderCommand,
    AddFrameCommand,
    CompositeCommand,
    CommentCommand,
    LaunchCommand,
    RegionOfInterestCommand,
    SaveToUndoBufferCommand,
    RedoCommand
  };

static MagickBooleanType IsUndoCacheCommand(const DisplayCommand command)
{
  size_t
    i;

  for (i=0; i < (sizeof(UndoCacheCommands)/sizeof(*UndoCacheCommands)); i++)
    if (UndoCacheCommands[i] == command)
      return(MagickTrue);
  return(MagickFalse);
}

static void XUndoImageCache(const XImageCacheContext *cache_context,
  Image **undo_image,Image **redo_image)
{
  Display
    *const display = cache_context->display;

  XResourceInfo
    *const resource_info = cache_context->resource_info;

  XWindows
    *const windows = cache_context->windows;

  Image
    **const image = cache_context->image;

  ExceptionInfo
    *const exception = cache_context->exception;

  char
    image_geometry[MagickPathExtent];

  Image
    *cache_image;

  /*
    Undo the last image transformation.
  */
  if ((*undo_image) == (Image *) NULL)
    {
      (void) XBell(display,0);
      ThrowXWindowException(ImageError,"NoImagesWereFound",
        (*image)->filename);
      return;
    }
  cache_image=(*undo_image);
  (*undo_image)=GetPreviousImageInList((*undo_image));
  windows->image.window_changes.width=(int) cache_image->columns;
  windows->image.window_changes.height=(int) cache_image->rows;
  (void) FormatLocaleString(image_geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(image,windows->image.crop_geometry,image_geometry,
    exception);
  if (windows->image.crop_geometry != (char *) NULL)
    windows->image.crop_geometry=(char *) RelinquishMagickMemory(
      windows->image.crop_geometry);
  windows->image.crop_geometry=cache_image->geometry;
  if ((*redo_image) != (Image *) NULL)
    (*redo_image)=DestroyImage((*redo_image));
  (*redo_image)=(*image);
  *image=cache_image->list;
  cache_image=DestroyImage(cache_image);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XRedoOrShowImageCache(const XImageCacheContext *cache_context,
  const DisplayCommand command,Image **redo_image,Image *undo_image)
{
  Display
    *const display = cache_context->display;

  XResourceInfo
    *const resource_info = cache_context->resource_info;

  XWindows
    *const windows = cache_context->windows;

  Image
    **const image = cache_context->image;

  ExceptionInfo
    *const exception = cache_context->exception;

  if (command == RedoCommand)
    {
      /*
        Redo the last image transformation.
      */
      if ((*redo_image) == (Image *) NULL)
        {
          (void) XBell(display,0);
          return;
        }
      windows->image.window_changes.width=(int) (*redo_image)->columns;
      windows->image.window_changes.height=(int) (*redo_image)->rows;
      if (windows->image.crop_geometry != (char *) NULL)
        windows->image.crop_geometry=(char *)
          RelinquishMagickMemory(windows->image.crop_geometry);
      windows->image.crop_geometry=(*redo_image)->geometry;
      *image=DestroyImage(*image);
      *image=(*redo_image);
      (*redo_image)=NewImageList();
      if (windows->image.orphan != MagickFalse)
        return;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      return;
    }
  if (command != InfoCommand)
    return;
  /*
    Display image info.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  XDisplayImageInfo(display,resource_info,windows,undo_image,*image,exception);
  XSetCursorState(display,windows,MagickFalse);
}

static void XImageCache(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const DisplayCommand command,Image **image,
  ExceptionInfo *exception)
{
  Image
    *cache_image;

  static Image
    *redo_image = (Image *) NULL,
    *undo_image = (Image *) NULL;

  XImageCacheContext
    cache_context;

  cache_context.display=display;
  cache_context.resource_info=resource_info;
  cache_context.windows=windows;
  cache_context.image=image;
  cache_context.exception=exception;
  switch (command)
  {
    case FreeBuffersCommand:
    {
      /*
        Free memory from the undo and redo cache.
      */
      while (undo_image != (Image *) NULL)
      {
        cache_image=undo_image;
        undo_image=GetPreviousImageInList(undo_image);
        cache_image->list=DestroyImage(cache_image->list);
        cache_image=DestroyImage(cache_image);
      }
      undo_image=NewImageList();
      if (redo_image != (Image *) NULL)
        redo_image=DestroyImage(redo_image);
      redo_image=NewImageList();
      return;
    }
    case UndoCommand:
    {
      XUndoImageCache(&cache_context,&undo_image,&redo_image);
      return;
    }
    default:
      break;
  }
  if (IsUndoCacheCommand(command) != MagickFalse)
    undo_image=XSaveUndoImage(&cache_context,undo_image);
  XRedoOrShowImageCache(&cache_context,command,&redo_image,undo_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X I m a g e W i n d o w C o m m a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XImageWindowCommand() makes a transform to the image or Image window as
%  specified by a user menu button or keyboard command.
%
%  The format of the XImageWindowCommand method is:
%
%      DisplayCommand XImageWindowCommand(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,
%        const MagickStatusType state,KeySym key_symbol,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o nexus:  Method XImageWindowCommand returns an image when the
%      user chooses 'Open Image' from the command menu.  Otherwise a null
%      image is returned.
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o state: key mask.
%
%    o key_symbol: Specifies a command to perform.
%
%    o image: the image;  XImageWIndowCommand may transform the image and
%      return a new image pointer.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XClampTrimTopQuantum(XResourceInfo *resource_info,
  const RectangleInfo crop_info)
{
  if (resource_info->quantum >= (int) crop_info.height)
    resource_info->quantum=(int) crop_info.height-1;
}

static void XClampTrimBottomQuantum(XResourceInfo *resource_info,
  const RectangleInfo crop_info)
{
  if (resource_info->quantum >= (int) (crop_info.height-(ssize_t) crop_info.y))
    resource_info->quantum=(int) (crop_info.height-(ssize_t) crop_info.y-1);
}

static void XClampTrimLeftQuantum(XResourceInfo *resource_info,
  const RectangleInfo crop_info)
{
  if (resource_info->quantum >= (int) crop_info.width)
    resource_info->quantum=(int) crop_info.width-1;
}

static void XClampTrimRightQuantum(XResourceInfo *resource_info,
  const RectangleInfo crop_info)
{
  if (resource_info->quantum >= (int) (crop_info.width-(ssize_t) crop_info.x))
    resource_info->quantum=(int) (crop_info.width-(ssize_t) crop_info.x-1);
}

typedef struct _XKeyCommandInfo
{
  int
    key;

  MagickStatusType
    first_mask;

  DisplayCommand
    first_command;

  MagickStatusType
    second_mask;

  DisplayCommand
    second_command,
    plain_command,
    control_command;
} XKeyCommandInfo;

static const XKeyCommandInfo
  ImageWindowKeyCommands[] =
  {
    { XK_o, ControlMask, OpenCommand, ControlMask, OpenCommand, NullCommand,
      OpenCommand },
    { XK_space, ControlMask, NextCommand, ControlMask, NextCommand,
      NextCommand, NextCommand },
    { XK_BackSpace, ControlMask, FormerCommand, ControlMask, FormerCommand,
      FormerCommand, FormerCommand },
    { XK_s, Mod1Mask, SwirlCommand, ControlMask, SaveCommand, ShearCommand,
      SaveCommand },
    { XK_p, Mod1Mask, OilPaintCommand, Mod4Mask, ColorCommand, NullCommand,
      PrintCommand },
    { XK_Print, Mod1Mask, OilPaintCommand, Mod4Mask, ColorCommand,
      NullCommand, PrintCommand },
    { XK_d, Mod4Mask, DrawCommand, ControlMask, DeleteCommand, NullCommand,
      DeleteCommand },
    { XK_Select, ControlMask, SelectCommand, ControlMask, SelectCommand,
      NullCommand, SelectCommand },
    { XK_n, ControlMask, NewCommand, ControlMask, NewCommand, NullCommand,
      NewCommand },
    { XK_q, ControlMask, QuitCommand, ControlMask, QuitCommand, QuitCommand,
      QuitCommand },
    { XK_Escape, ControlMask, QuitCommand, ControlMask, QuitCommand,
      QuitCommand, QuitCommand },
    { XK_z, ControlMask, UndoCommand, ControlMask, UndoCommand, NullCommand,
      UndoCommand },
    { XK_Undo, ControlMask, UndoCommand, ControlMask, UndoCommand,
      NullCommand, UndoCommand },
    { XK_r, ControlMask, RedoCommand, ControlMask, RedoCommand, RollCommand,
      RedoCommand },
    { XK_Redo, ControlMask, RedoCommand, ControlMask, RedoCommand,
      RollCommand, RedoCommand },
    { XK_x, ControlMask, CutCommand, ControlMask, CutCommand, NullCommand,
      CutCommand },
    { XK_c, Mod1Mask, CharcoalDrawCommand, ControlMask, CopyCommand,
      CropCommand, CopyCommand },
    { XK_v, Mod4Mask, CompositeCommand, ControlMask, PasteCommand,
      FlipCommand, PasteCommand },
    { XK_Insert, Mod4Mask, CompositeCommand, ControlMask, PasteCommand,
      FlipCommand, PasteCommand },
    { XK_less, ControlMask, HalfSizeCommand, ControlMask, HalfSizeCommand,
      HalfSizeCommand, HalfSizeCommand },
    { XK_minus, ControlMask, OriginalSizeCommand, ControlMask,
      OriginalSizeCommand, OriginalSizeCommand, OriginalSizeCommand },
    { XK_greater, ControlMask, DoubleSizeCommand, ControlMask,
      DoubleSizeCommand, DoubleSizeCommand, DoubleSizeCommand },
    { XK_percent, ControlMask, ResizeCommand, ControlMask, ResizeCommand,
      ResizeCommand, ResizeCommand },
    { XK_at, ControlMask, RefreshCommand, ControlMask, RefreshCommand,
      RefreshCommand, RefreshCommand },
    { XK_bracketleft, ControlMask, ChopCommand, ControlMask, ChopCommand,
      ChopCommand, ChopCommand },
    { XK_h, ControlMask, FlopCommand, ControlMask, FlopCommand, FlopCommand,
      FlopCommand },
    { XK_slash, ControlMask, RotateRightCommand, ControlMask,
      RotateRightCommand, RotateRightCommand, RotateRightCommand },
    { XK_backslash, ControlMask, RotateLeftCommand, ControlMask,
      RotateLeftCommand, RotateLeftCommand, RotateLeftCommand },
    { XK_asterisk, ControlMask, RotateCommand, ControlMask, RotateCommand,
      RotateCommand, RotateCommand },
    { XK_t, ControlMask, TrimCommand, ControlMask, TrimCommand, TrimCommand,
      TrimCommand },
    { XK_H, ControlMask, HueCommand, ControlMask, HueCommand, HueCommand,
      HueCommand },
    { XK_S, ControlMask, SaturationCommand, ControlMask, SaturationCommand,
      SaturationCommand, SaturationCommand },
    { XK_L, ControlMask, BrightnessCommand, ControlMask, BrightnessCommand,
      BrightnessCommand, BrightnessCommand },
    { XK_G, ControlMask, GammaCommand, ControlMask, GammaCommand,
      GammaCommand, GammaCommand },
    { XK_C, ControlMask, SpiffCommand, ControlMask, SpiffCommand,
      SpiffCommand, SpiffCommand },
    { XK_Z, ControlMask, DullCommand, ControlMask, DullCommand, DullCommand,
      DullCommand },
    { XK_N, ControlMask, NormalizeCommand, ControlMask, NormalizeCommand,
      NormalizeCommand, NormalizeCommand },
    { XK_equal, ControlMask, EqualizeCommand, ControlMask, EqualizeCommand,
      EqualizeCommand, EqualizeCommand },
    { XK_asciitilde, ControlMask, NegateCommand, ControlMask, NegateCommand,
      NegateCommand, NegateCommand },
    { XK_period, ControlMask, GrayscaleCommand, ControlMask, GrayscaleCommand,
      GrayscaleCommand, GrayscaleCommand },
    { XK_numbersign, ControlMask, QuantizeCommand, ControlMask,
      QuantizeCommand, QuantizeCommand, QuantizeCommand },
    { XK_F2, ControlMask, DespeckleCommand, ControlMask, DespeckleCommand,
      DespeckleCommand, DespeckleCommand },
    { XK_F3, ControlMask, EmbossCommand, ControlMask, EmbossCommand,
      EmbossCommand, EmbossCommand },
    { XK_F4, ControlMask, ReduceNoiseCommand, ControlMask, ReduceNoiseCommand,
      ReduceNoiseCommand, ReduceNoiseCommand },
    { XK_F5, ControlMask, AddNoiseCommand, ControlMask, AddNoiseCommand,
      AddNoiseCommand, AddNoiseCommand },
    { XK_F6, ControlMask, SharpenCommand, ControlMask, SharpenCommand,
      SharpenCommand, SharpenCommand },
    { XK_F7, ControlMask, BlurCommand, ControlMask, BlurCommand, BlurCommand,
      BlurCommand },
    { XK_F8, ControlMask, ThresholdCommand, ControlMask, ThresholdCommand,
      ThresholdCommand, ThresholdCommand },
    { XK_F9, ControlMask, EdgeDetectCommand, ControlMask, EdgeDetectCommand,
      EdgeDetectCommand, EdgeDetectCommand },
    { XK_F10, ControlMask, SpreadCommand, ControlMask, SpreadCommand,
      SpreadCommand, SpreadCommand },
    { XK_F11, ControlMask, ShadeCommand, ControlMask, ShadeCommand,
      ShadeCommand, ShadeCommand },
    { XK_F12, ControlMask, RaiseCommand, ControlMask, RaiseCommand,
      RaiseCommand, RaiseCommand },
    { XK_F13, ControlMask, SegmentCommand, ControlMask, SegmentCommand,
      SegmentCommand, SegmentCommand },
    { XK_i, Mod1Mask, ImplodeCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_w, Mod1Mask, WaveCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_m, Mod4Mask, MatteCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_b, Mod4Mask, AddBorderCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_f, Mod4Mask, AddFrameCommand, ControlMask, NullCommand, NullCommand,
      NullCommand },
    { XK_exclam, Mod4Mask, CommentCommand, ControlMask, NullCommand,
      NullCommand, NullCommand },
    { XK_a, Mod1Mask, ApplyCommand, Mod4Mask, AnnotateCommand, NullCommand,
      RegionOfInterestCommand },
    { XK_question, ControlMask, InfoCommand, ControlMask, InfoCommand,
      InfoCommand, InfoCommand },
    { XK_plus, ControlMask, ZoomCommand, ControlMask, ZoomCommand,
      ZoomCommand, ZoomCommand },
    { XK_Execute, ControlMask, LaunchCommand, ControlMask, LaunchCommand,
      LaunchCommand, LaunchCommand },
    { XK_F1, ControlMask, HelpCommand, ControlMask, HelpCommand, HelpCommand,
      HelpCommand },
    { XK_Find, ControlMask, BrowseDocumentationCommand, ControlMask,
      BrowseDocumentationCommand, BrowseDocumentationCommand,
      BrowseDocumentationCommand }
  };

static DisplayCommand XLookupKeyCommand(const int key,
  const MagickStatusType state)
{
  size_t
    i;

  for (i=0; i < (sizeof(ImageWindowKeyCommands)/
       sizeof(*ImageWindowKeyCommands)); i++)
  {
    if (ImageWindowKeyCommands[i].key != key)
      continue;
    if ((state & ImageWindowKeyCommands[i].first_mask) != 0)
      return(ImageWindowKeyCommands[i].first_command);
    if ((state & ImageWindowKeyCommands[i].second_mask) != 0)
      return(ImageWindowKeyCommands[i].second_command);
    if ((state & ControlMask) == 0)
      return(ImageWindowKeyCommands[i].plain_command);
    return(ImageWindowKeyCommands[i].control_command);
  }
  return(NullCommand);
}

static DisplayCommand XImageWindowCommand(Display *display,
  XResourceInfo *resource_info,XWindows *windows,const MagickStatusType state,
  KeySym key_symbol,Image **image,ExceptionInfo *exception)
{
  static char
    delta[MagickPathExtent+1] = "";

  static const char
    Digits[] = "01234567890";

  static KeySym
    last_symbol = XK_0;

  if ((key_symbol >= XK_0) && (key_symbol <= XK_9))
    {
      size_t
        length;

      if (((last_symbol < XK_0) || (last_symbol > XK_9)))
        {
          *delta='\0';
          resource_info->quantum=1;
        }
      last_symbol=key_symbol;
      length=strlen(delta);
      if (length < MagickPathExtent)
        {
          delta[length]=Digits[key_symbol-XK_0];
          delta[length+1]='\0';
        }
      resource_info->quantum=StringToLong(delta);
      return(NullCommand);
    }
  last_symbol=key_symbol;
  if (resource_info->immutable)
    {
      /*
        Virtual image window has a restricted command set.
      */
      switch (key_symbol)
      {
        case XK_question:
          return(InfoCommand);
        case XK_p:
        case XK_Print:
          return(PrintCommand);
        case XK_space:
          return(NextCommand);
        case XK_q:
        case XK_Escape:
          return(QuitCommand);
        default:
          break;
      }
      return(NullCommand);
    }
  switch ((int) key_symbol)
  {
    case XK_P:
    {
      if ((state & ShiftMask) == 0)
        return(NullCommand);
      return(ShowPreviewCommand);
    }
    case XK_Menu:
    {
      (void) XMapRaised(display,windows->command.id);
      return(NullCommand);
    }
    case XK_Next:
    case XK_Prior:
    case XK_Home:
    case XK_KP_Home:
    {
      XTranslateImage(display,windows,*image,key_symbol);
      return(NullCommand);
    }
    case XK_Up:
    case XK_KP_Up:
    case XK_Down:
    case XK_KP_Down:
    case XK_Left:
    case XK_KP_Left:
    case XK_Right:
    case XK_KP_Right:
    {
      if ((state & Mod1Mask) != 0)
        {
          RectangleInfo
            crop_info;

          /*
            Trim one pixel from edge of image.
          */
          crop_info.x=0;
          crop_info.y=0;
          crop_info.width=(size_t) windows->image.ximage->width;
          crop_info.height=(size_t) windows->image.ximage->height;
          if ((key_symbol == XK_Up) || (key_symbol == XK_KP_Up))
            {
              XClampTrimTopQuantum(resource_info,crop_info);
              crop_info.height-=(size_t) resource_info->quantum;
            }
          if ((key_symbol == XK_Down) || (key_symbol == XK_KP_Down))
            {
              XClampTrimBottomQuantum(resource_info,crop_info);
              crop_info.y+=resource_info->quantum;
              crop_info.height-=(size_t) resource_info->quantum;
            }
          if ((key_symbol == XK_Left) || (key_symbol == XK_KP_Left))
            {
              XClampTrimLeftQuantum(resource_info,crop_info);
              crop_info.width-=(size_t) resource_info->quantum;
            }
          if ((key_symbol == XK_Right) || (key_symbol == XK_KP_Right))
            {
              XClampTrimRightQuantum(resource_info,crop_info);
              crop_info.x+=resource_info->quantum;
              crop_info.width-=(size_t) resource_info->quantum;
            }
          if ((windows->image.x+(int) windows->image.width) > (int) crop_info.width)
            windows->image.x=(int) (crop_info.width-windows->image.width);
          if ((windows->image.y+(int) windows->image.height) > (int) crop_info.height)
            windows->image.y=(int) (crop_info.height-windows->image.height);
          XSetCropGeometry(display,windows,&crop_info,*image);
          windows->image.window_changes.width=(int) crop_info.width;
          windows->image.window_changes.height=(int) crop_info.height;
          (void) XSetWindowBackgroundPixmap(display,windows->image.id,None);
          (void) XConfigureImage(display,resource_info,windows,*image,
            exception);
          return(NullCommand);
        }
      XTranslateImage(display,windows,*image,key_symbol);
      return(NullCommand);
    }
    default:
      break;
  }
  return(XLookupKeyCommand((int) key_symbol,state));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g i c k C o m m a n d                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagickCommand() makes a transform to the image or Image window as
%  specified by a user menu button or keyboard command.
%
%  The format of the XMagickCommand method is:
%
%      Image *XMagickCommand(Display *display,XResourceInfo *resource_info,
%        XWindows *windows,const DisplayCommand command,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o command: Specifies a command to perform.
%
%    o image: the image;  XMagickCommand may transform the image and return a
%      new image pointer.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsImageWindowUncropped(
  const XWindows *windows,Image **image)
{
  return(((windows->image.crop_geometry == (char *) NULL) &&
    ((int) (*image)->columns == windows->image.ximage->width) &&
    ((int) (*image)->rows == windows->image.ximage->height)) ? MagickTrue :
    MagickFalse);
}

static inline MagickBooleanType IsImageWindowOriginalSize(
  const XWindows *windows,Image **image)
{
  return(((windows->image.width == (unsigned int) (*image)->columns) &&
    (windows->image.height == (unsigned int) (*image)->rows) &&
    (windows->image.crop_geometry == (char *) NULL)) ? MagickTrue :
    MagickFalse);
}

typedef struct _XMagickCommandContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  ImageInfo
    *image_info;

  ExceptionInfo
    *exception;
} XMagickCommandContext;

static void XApplyResizeCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  char
    geometry[MagickPathExtent];

  int
    status;

  size_t
    height,
    width;

  ssize_t
    x,
    y;

  /*
    Resize image.
  */
  width=(size_t) windows->image.ximage->width;
  height=(size_t) windows->image.ximage->height;
  x=0;
  y=0;
  (void) FormatLocaleString(geometry,MagickPathExtent,"%.17gx%.17g+0+0",
    (double) width,(double) height);
  status=XDialogWidget(display,windows,"Resize",
    "Enter resize geometry (e.g. 640x480, 200%):",geometry);
  if (*geometry == '\0')
    return;
  if (status == 0)
    (void) ConcatenateMagickString(geometry,"!",MagickPathExtent);
  (void) ParseMetaGeometry(geometry,&x,&y,&width,&height);
  windows->image.window_changes.width=(int) width;
  windows->image.window_changes.height=(int) height;
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyQuitCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  /*
    exit program.
  */
  if (resource_info->confirm_exit == MagickFalse)
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_exit,CurrentTime);
  else
    {
      int
        status;

      /*
        Confirm program exit.
      */
      status=XConfirmWidget(display,windows,"Do you really want to exit",
        resource_info->client_name);
      if (status > 0)
        XClientMessage(display,windows->image.id,windows->im_protocols,
          windows->im_exit,CurrentTime);
    }
}

static void XApplyDespeckleCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  Image
    *despeckle_image;

  /*
    Despeckle image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  despeckle_image=DespeckleImage(*image,exception);
  if (despeckle_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=despeckle_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyMapCommand(const XMagickCommandContext *context,
  QuantizeInfo quantize_info)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  Image
    *affinity_image;

  static char
    filename[MagickPathExtent] = "\0";

  /*
    Request image file name from user.
  */
  XFileBrowserWidget(display,windows,"Map",filename);
  if (*filename == '\0')
    return;
  /*
    Map image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  affinity_image=ReadImage(image_info,exception);
  if (affinity_image != (Image *) NULL)
    {
      (void) RemapImage(&quantize_info,*image,affinity_image,exception);
      affinity_image=DestroyImage(affinity_image);
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyQuantizeCommand(const XMagickCommandContext *context,
  QuantizeInfo quantize_info)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  int
    status;

  static char
    colors[MagickPathExtent] = "256";

  /*
    Query user for maximum number of colors.
  */
  status=XDialogWidget(display,windows,"Quantize",
    "Maximum number of colors:",colors);
  if (*colors == '\0')
    return;
  /*
    Color reduce the image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  quantize_info.number_colors=StringToUnsignedLong(colors);
  quantize_info.dither_method=status != 0 ? RiemersmaDitherMethod :
    NoDitherMethod;
  (void) QuantizeImage(&quantize_info,*image,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyReduceNoiseCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  Image
    *noise_image;

  static char
    radius[MagickPathExtent] = "0";

  /*
    Query user for noise radius.
  */
  (void) XDialogWidget(display,windows,"Reduce Noise",
    "Enter the noise radius:",radius);
  if (*radius == '\0')
    return;
  /*
    Reduce noise in the image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  noise_image=StatisticImage(*image,NonpeakStatistic,(size_t)
    geometry_info.rho,(size_t) geometry_info.rho,exception);
  if (noise_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=noise_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySharpenCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  Image
    *sharp_image;

  static char
    radius[MagickPathExtent] = "0.0x1.0";

  /*
    Query user for sharpen radius.
  */
  (void) XDialogWidget(display,windows,"Sharpen",
    "Enter the sharpen radius and standard deviation:",radius);
  if (*radius == '\0')
    return;
  /*
    Sharpen image scanlines.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  sharp_image=SharpenImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (sharp_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=sharp_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyBlurCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  Image
    *blur_image;

  static char
    radius[MagickPathExtent] = "0.0x1.0";

  /*
    Query user for blur radius.
  */
  (void) XDialogWidget(display,windows,"Blur",
    "Enter the blur radius and standard deviation:",radius);
  if (*radius == '\0')
    return;
  /*
    Blur an image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  blur_image=BlurImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (blur_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=blur_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyThresholdCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  double
    threshold;

  static char
    factor[MagickPathExtent] = "128";

  /*
    Query user for threshold value.
  */
  (void) XDialogWidget(display,windows,"Threshold",
    "Enter threshold value:",factor);
  if (*factor == '\0')
    return;
  /*
    Gamma correct image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  threshold=StringToDoubleInterval(factor,(double) QuantumRange+1.0);
  (void) BilevelImage(*image,threshold,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyEdgeDetectCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  Image
    *edge_image;

  static char
    radius[MagickPathExtent] = "0";

  /*
    Query user for edge factor.
  */
  (void) XDialogWidget(display,windows,"Detect Edges",
    "Enter the edge detect radius:",radius);
  if (*radius == '\0')
    return;
  /*
    Detect edge in image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(radius,&geometry_info);
  edge_image=EdgeImage(*image,geometry_info.rho,exception);
  if (edge_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=edge_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySpreadCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  Image
    *spread_image;

  static char
    amount[MagickPathExtent] = "2";

  /*
    Query user for spread amount.
  */
  (void) XDialogWidget(display,windows,"Spread",
    "Enter the displacement amount:",amount);
  if (*amount == '\0')
    return;
  /*
    Displace image pixels by a random amount.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(amount,&geometry_info);
  spread_image=EdgeImage(*image,geometry_info.rho,exception);
  if (spread_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=spread_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyShadeCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  Image
    *shade_image;

  int
    status;

  static char
    geometry[MagickPathExtent] = "30x30";

  /*
    Query user for the shade geometry.
  */
  status=XDialogWidget(display,windows,"Shade",
    "Enter the azimuth and elevation of the light source:",geometry);
  if (*geometry == '\0')
    return;
  /*
    Shade image pixels.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(geometry,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  shade_image=ShadeImage(*image,status != 0 ? MagickTrue : MagickFalse,
    geometry_info.rho,geometry_info.sigma,exception);
  if (shade_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=shade_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRaiseCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  RectangleInfo
    page_geometry;

  static char
    bevel_width[MagickPathExtent] = "10";

  /*
    Query user for bevel width.
  */
  (void) XDialogWidget(display,windows,"Raise","Bevel width:",bevel_width);
  if (*bevel_width == '\0')
    return;
  /*
    Raise an image.
  */
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) ParsePageGeometry(*image,bevel_width,&page_geometry,
    exception);
  (void) RaiseImage(*image,&page_geometry,MagickTrue,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySegmentCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  static char
    threshold[MagickPathExtent] = "1.0x1.5";

  /*
    Query user for smoothing threshold.
  */
  (void) XDialogWidget(display,windows,"Segment","Smooth threshold:",
    threshold);
  if (*threshold == '\0')
    return;
  /*
    Segment an image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(threshold,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  (void) SegmentImage(*image,sRGBColorspace,MagickFalse,geometry_info.rho,
    geometry_info.sigma,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySepiaToneCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  double
    threshold;

  Image
    *sepia_image;

  static char
    factor[MagickPathExtent] = "80%";

  /*
    Query user for sepia-tone factor.
  */
  (void) XDialogWidget(display,windows,"Sepia Tone",
    "Enter the sepia tone factor (0 - 99.9%):",factor);
  if (*factor == '\0')
    return;
  /*
    Sepia tone image pixels.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  threshold=StringToDoubleInterval(factor,(double) QuantumRange+1.0);
  sepia_image=SepiaToneImage(*image,threshold,exception);
  if (sepia_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=sepia_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySolarizeCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  double
    threshold;

  static char
    factor[MagickPathExtent] = "60%";

  /*
    Query user for solarize factor.
  */
  (void) XDialogWidget(display,windows,"Solarize",
    "Enter the solarize factor (0 - 99.9%):",factor);
  if (*factor == '\0')
    return;
  /*
    Solarize image pixels.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  threshold=StringToDoubleInterval(factor,(double) QuantumRange+1.0);
  (void) SolarizeImage(*image,threshold,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySwirlCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  Image
    *swirl_image;

  static char
    degrees[MagickPathExtent] = "60";

  /*
    Query user for swirl angle.
  */
  (void) XDialogWidget(display,windows,"Swirl","Enter the swirl angle:",
    degrees);
  if (*degrees == '\0')
    return;
  /*
    Swirl image pixels about the center.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(degrees,&geometry_info);
  swirl_image=SwirlImage(*image,geometry_info.rho,(*image)->interpolate,
    exception);
  if (swirl_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=swirl_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyImplodeCommand(const XMagickCommandContext *context,
  MagickStatusType *flags)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  Image
    *implode_image;

  static char
    factor[MagickPathExtent] = "0.3";

  /*
    Query user for implode factor.
  */
  (void) XDialogWidget(display,windows,"Implode",
    "Enter the implosion/explosion factor (-1.0 - 1.0):",factor);
  if (*factor == '\0')
    return;
  /*
    Implode image pixels about the center.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  *flags=ParseGeometry(factor,&geometry_info);
  implode_image=ImplodeImage(*image,geometry_info.rho,(*image)->interpolate,
    exception);
  if (implode_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=implode_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyVignetteCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  Image
    *vignette_image;

  static char
    geometry[MagickPathExtent] = "0x20";

  /*
    Query user for the vignette geometry.
  */
  (void) XDialogWidget(display,windows,"Vignette",
    "Enter the radius, sigma, and x and y offsets:",geometry);
  if (*geometry == '\0')
    return;
  /*
    Soften the edges of the image in vignette style
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(geometry,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  if ((flags & XiValue) == 0)
    geometry_info.xi=0.1*(*image)->columns;
  if ((flags & PsiValue) == 0)
    geometry_info.psi=0.1*(*image)->rows;
  vignette_image=VignetteImage(*image,geometry_info.rho,0.0,(ssize_t)
    ceil(geometry_info.xi-0.5),(ssize_t) ceil(geometry_info.psi-0.5),
    exception);
  if (vignette_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=vignette_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRollCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  RectangleInfo
    page_geometry;

  Image
    *roll_image;

  static char
    geometry[MagickPathExtent] = "+2+2";

  /*
    Query user for the roll geometry.
  */
  (void) XDialogWidget(display,windows,"Roll","Enter roll geometry:",
    geometry);
  if (*geometry == '\0')
    return;
  /*
    Roll image.
  */
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) ParsePageGeometry(*image,geometry,&page_geometry,
    exception);
  roll_image=RollImage(*image,page_geometry.x,page_geometry.y,
    exception);
  if (roll_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=roll_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyHueCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  char
    modulate_factors[MagickPathExtent];

  static char
    hue_percent[MagickPathExtent] = "110";

  /*
    Query user for percent hue change.
  */
  (void) XDialogWidget(display,windows,"Apply",
    "Enter percent change in image hue (0-200):",hue_percent);
  if (*hue_percent == '\0')
    return;
  /*
    Vary the image hue.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(modulate_factors,"100.0/100.0/",MagickPathExtent);
  (void) ConcatenateMagickString(modulate_factors,hue_percent,
    MagickPathExtent);
  (void) ModulateImage(*image,modulate_factors,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyEmbossCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  Image
    *emboss_image;

  static char
    radius[MagickPathExtent] = "0.0x1.0";

  /*
    Query user for emboss radius.
  */
  (void) XDialogWidget(display,windows,"Emboss",
    "Enter the emboss radius and standard deviation:",radius);
  if (*radius == '\0')
    return;
  /*
    Reduce noise in the image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  flags=ParseGeometry(radius,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=1.0;
  emboss_image=EmbossImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (emboss_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=emboss_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyAddNoiseCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  char
    **noises;

  Image
    *noise_image;

  static char
    noise_type[MagickPathExtent] = "Gaussian";

  /*
    Add noise to the image.
  */
  noises=GetCommandOptions(MagickNoiseOptions);
  if (noises == (char **) NULL)
    return;
  XListBrowserWidget(display,windows,&windows->widget,
    (const char **) noises,"Add Noise",
    "Select a type of noise to add to your image:",noise_type);
  noises=DestroyStringList(noises);
  if (*noise_type == '\0')
    return;
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  noise_image=AddNoiseImage(*image,(NoiseType) ParseCommandOption(
    MagickNoiseOptions,MagickFalse,noise_type),1.0,exception);
  if (noise_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=noise_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyRestoreCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  /*
    Restore Image window to its original size.
  */
  if (IsImageWindowOriginalSize(windows,image) != MagickFalse)
    {
      (void) XBell(display,0);
      return;
    }
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  if (windows->image.crop_geometry != (char *) NULL)
    {
      windows->image.crop_geometry=(char *)
        RelinquishMagickMemory(windows->image.crop_geometry);
      windows->image.crop_geometry=(char *) NULL;
      windows->image.x=0;
      windows->image.y=0;
    }
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplySaveCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  MagickStatusType
    status;

  /*
    Save image.
  */
  status=XSaveImage(display,resource_info,windows,*image,exception);
  if (status == MagickFalse)
    {
      char
        message[MagickPathExtent];

      (void) FormatLocaleString(message,MagickPathExtent,"%s:%s",
        exception->reason != (char *) NULL ? exception->reason : "",
        exception->description != (char *) NULL ? exception->description :
        "");
      XNoticeWidget(display,windows,"Unable to save file:",message);
      return;
    }
}

static Image *XApplyNewCommand(const XMagickCommandContext *context,
  Image *nexus)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ImageInfo
    *const image_info = context->image_info;

  ExceptionInfo
    *const exception = context->exception;

  int
    status;

  static char
    color[MagickPathExtent] = "gray",
    geometry[MagickPathExtent] = "640x480";

  static const char
    *format = "gradient";

  /*
    Query user for canvas geometry.
  */
  status=XDialogWidget(display,windows,"New","Enter image geometry:",
    geometry);
  if (*geometry == '\0')
    return(nexus);
  if (status == 0)
    format="xc";
  XColorBrowserWidget(display,windows,"Select",color);
  if (*color == '\0')
    return(nexus);
  /*
    Create canvas.
  */
  (void) FormatLocaleString(image_info->filename,MagickPathExtent,
    "%s:%s",format,color);
  (void) CloneString(&image_info->size,geometry);
  nexus=ReadImage(image_info,exception);
  CatchException(exception);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_next_image,CurrentTime);
  return(nexus);
}

static void XApplyShearCommand(const XMagickCommandContext *context,char *color)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  Image
    *shear_image;

  static char
    geometry[MagickPathExtent] = "45.0x45.0";

  /*
    Query user for shear color and geometry.
  */
  XColorBrowserWidget(display,windows,"Select",color);
  if (*color == '\0')
    return;
  (void) XDialogWidget(display,windows,"Shear","Enter shear geometry:",
    geometry);
  if (*geometry == '\0')
    return;
  /*
    Shear image.
  */
  (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
    exception);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) QueryColorCompliance(color,AllCompliance,
    &(*image)->background_color,exception);
  flags=ParseGeometry(geometry,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=geometry_info.rho;
  shear_image=ShearImage(*image,geometry_info.rho,geometry_info.sigma,
    exception);
  if (shear_image != (Image *) NULL)
    {
      *image=DestroyImage(*image);
      *image=shear_image;
    }
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (windows->image.orphan != MagickFalse)
    return;
  windows->image.window_changes.width=(int) (*image)->columns;
  windows->image.window_changes.height=(int) (*image)->rows;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static void XApplyApplyCommand(const XMagickCommandContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  ExceptionInfo
    *const exception = context->exception;

  char
    image_geometry[MagickPathExtent];

  if (IsImageWindowUncropped(windows,image) != MagickFalse)
    return;
  /*
    Apply size transforms to image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  /*
    Crop and/or scale displayed image.
  */
  (void) FormatLocaleString(image_geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(image,windows->image.crop_geometry,image_geometry,
    exception);
  if (windows->image.crop_geometry != (char *) NULL)
    windows->image.crop_geometry=(char *) RelinquishMagickMemory(
      windows->image.crop_geometry);
  windows->image.x=0;
  windows->image.y=0;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
}

static Image *XMagickCommand(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const DisplayCommand command,Image **image,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    modulate_factors[MagickPathExtent];

  GeometryInfo
    geometry_info;

  Image
    *nexus;

  ImageInfo
    *image_info;

  int
    x,
    y;

  MagickStatusType
    flags,
    status;

  QuantizeInfo
    quantize_info;

  RectangleInfo
    page_geometry;

  int
    i;

  static char
    color[MagickPathExtent] = "gray";

  unsigned int
    height,
    width;

  XMagickCommandContext
    context;

  /*
    Process user command.
  */
  XCheckRefreshWindows(display,windows);
  XImageCache(display,resource_info,windows,command,image,exception);
  nexus=NewImageList();
  windows->image.window_changes.width=windows->image.ximage->width;
  windows->image.window_changes.height=windows->image.ximage->height;
  image_info=CloneImageInfo(resource_info->image_info);
  SetGeometryInfo(&geometry_info);
  GetQuantizeInfo(&quantize_info);
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.image=image;
  context.image_info=image_info;
  context.exception=exception;
  switch (command)
  {
    case OpenCommand:
    {
      /*
        Load image.
      */
      nexus=XOpenImage(display,resource_info,windows,MagickFalse);
      break;
    }
    case NextCommand:
    {
      /*
        Display next image.
      */
      for (i=0; i < resource_info->quantum; i++)
        XClientMessage(display,windows->image.id,windows->im_protocols,
          windows->im_next_image,CurrentTime);
      break;
    }
    case FormerCommand:
    {
      /*
        Display former image.
      */
      for (i=0; i < resource_info->quantum; i++)
        XClientMessage(display,windows->image.id,windows->im_protocols,
          windows->im_former_image,CurrentTime);
      break;
    }
    case SelectCommand:
    {
      int
        status;

      /*
        Select image.
      */
      if (*resource_info->home_directory == '\0')
        (void) CopyMagickString(resource_info->home_directory,".",
          MagickPathExtent);
      status=chdir(resource_info->home_directory);
      if (status == -1)
        (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
          "UnableToOpenFile","%s",resource_info->home_directory);
      nexus=XOpenImage(display,resource_info,windows,MagickTrue);
      break;
    }
    case SaveCommand:
      XApplySaveCommand(&context);
      break;
    case PrintCommand:
    {
      /*
        Print image.
      */
      status=XPrintImage(display,resource_info,windows,*image,exception);
      if (status == MagickFalse)
        {
          char
            message[MagickPathExtent];

          (void) FormatLocaleString(message,MagickPathExtent,"%s:%s",
            exception->reason != (char *) NULL ? exception->reason : "",
            exception->description != (char *) NULL ? exception->description :
            "");
          XNoticeWidget(display,windows,"Unable to print file:",message);
          break;
        }
      break;
    }
    case DeleteCommand:
    {
      static char
        filename[MagickPathExtent] = "\0";

      /*
        Delete image file.
      */
      XFileBrowserWidget(display,windows,"Delete",filename);
      if (*filename == '\0')
        break;
      status=ShredFile(filename);
      if (remove_utf8(filename) < 0)
        status=MagickTrue;
      if (status != MagickFalse)
        XNoticeWidget(display,windows,"Unable to delete image file:",filename);
      break;
    }
    case NewCommand:
      nexus=XApplyNewCommand(&context,nexus);
      break;
    case VisualDirectoryCommand:
    {
      /*
        Visual Image directory.
      */
      nexus=XVisualDirectoryImage(display,resource_info,windows,exception);
      break;
    }
    case QuitCommand:
      XApplyQuitCommand(&context);
      break;
    case CutCommand:
    {
      /*
        Cut image.
      */
      (void) XCropImage(display,resource_info,windows,*image,CutMode,exception);
      break;
    }
    case CopyCommand:
    {
      /*
        Copy image.
      */
      (void) XCropImage(display,resource_info,windows,*image,CopyMode,
        exception);
      break;
    }
    case PasteCommand:
    {
      /*
        Paste image.
      */
      status=XPasteImage(display,resource_info,windows,*image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to paste X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case HalfSizeCommand:
    {
      /*
        Half image size.
      */
      windows->image.window_changes.width=windows->image.ximage->width/2;
      windows->image.window_changes.height=windows->image.ximage->height/2;
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case OriginalSizeCommand:
    {
      /*
        Original image size.
      */
      windows->image.window_changes.width=(int) (*image)->columns;
      windows->image.window_changes.height=(int) (*image)->rows;
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case DoubleSizeCommand:
    {
      /*
        Double the image size.
      */
      windows->image.window_changes.width=windows->image.ximage->width << 1;
      windows->image.window_changes.height=windows->image.ximage->height << 1;
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case ResizeCommand:
      XApplyResizeCommand(&context);
      break;
    case ApplyCommand:
      XApplyApplyCommand(&context);
      break;
    case RefreshCommand:
    {
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case RestoreCommand:
      XApplyRestoreCommand(&context);
      break;
    case CropCommand:
    {
      /*
        Crop image.
      */
      (void) XCropImage(display,resource_info,windows,*image,CropMode,
        exception);
      break;
    }
    case ChopCommand:
    {
      /*
        Chop image.
      */
      status=XChopImage(display,resource_info,windows,image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to cut X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case FlopCommand:
    {
      Image
        *flop_image;

      /*
        Flop image scanlines.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      flop_image=FlopImage(*image,exception);
      if (flop_image != (Image *) NULL)
        {
          *image=DestroyImage(*image);
          *image=flop_image;
        }
      CatchException(exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.crop_geometry != (char *) NULL)
        {
          /*
            Flop crop geometry.
          */
          width=(unsigned int) (*image)->columns;
          height=(unsigned int) (*image)->rows;
          (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
            &width,&height);
          (void) FormatLocaleString(windows->image.crop_geometry,
            MagickPathExtent,"%ux%u%+d%+d",width,height,(int) (*image)->columns-
            (int) width-x,y);
        }
      if (windows->image.orphan != MagickFalse)
        break;
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case FlipCommand:
    {
      Image
        *flip_image;

      /*
        Flip image scanlines.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      flip_image=FlipImage(*image,exception);
      if (flip_image != (Image *) NULL)
        {
          *image=DestroyImage(*image);
          *image=flip_image;
        }
      CatchException(exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.crop_geometry != (char *) NULL)
        {
          /*
            Flip crop geometry.
          */
          width=(unsigned int) (*image)->columns;
          height=(unsigned int) (*image)->rows;
          (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
            &width,&height);
          (void) FormatLocaleString(windows->image.crop_geometry,
            MagickPathExtent,"%ux%u%+d%+d",width,height,x,(int) (*image)->rows-
            (int) height-y);
        }
      if (windows->image.orphan != MagickFalse)
        break;
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case RotateRightCommand:
    {
      /*
        Rotate image 90 degrees clockwise.
      */
      status=XRotateImage(display,resource_info,windows,90.0,image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to rotate X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case RotateLeftCommand:
    {
      /*
        Rotate image 90 degrees counter-clockwise.
      */
      status=XRotateImage(display,resource_info,windows,-90.0,image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to rotate X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case RotateCommand:
    {
      /*
        Rotate image.
      */
      status=XRotateImage(display,resource_info,windows,0.0,image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to rotate X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case ShearCommand:
      XApplyShearCommand(&context,color);
      break;
    case RollCommand:
      XApplyRollCommand(&context);
      break;
    case TrimCommand:
    {
      static char
        fuzz[MagickPathExtent];

      /*
        Query user for the fuzz factor.
      */
      (void) FormatLocaleString(fuzz,MagickPathExtent,"%g%%",100.0*
        (*image)->fuzz/((double) QuantumRange+1.0));
      (void) XDialogWidget(display,windows,"Trim","Enter fuzz factor:",fuzz);
      if (*fuzz == '\0')
        break;
      (*image)->fuzz=StringToDoubleInterval(fuzz,(double) QuantumRange+1.0);
      /*
        Trim image.
      */
      status=XTrimImage(display,resource_info,windows,*image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to trim X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case HueCommand:
      XApplyHueCommand(&context);
      break;
    case SaturationCommand:
    {
      static char
        saturation_percent[MagickPathExtent] = "110";

      /*
        Query user for percent saturation change.
      */
      (void) XDialogWidget(display,windows,"Apply",
        "Enter percent change in color saturation (0-200):",saturation_percent);
      if (*saturation_percent == '\0')
        break;
      /*
        Vary color saturation.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) CopyMagickString(modulate_factors,"100.0/",MagickPathExtent);
      (void) ConcatenateMagickString(modulate_factors,saturation_percent,
        MagickPathExtent);
      (void) ModulateImage(*image,modulate_factors,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case BrightnessCommand:
    {
      static char
        brightness_percent[MagickPathExtent] = "110";

      /*
        Query user for percent brightness change.
      */
      (void) XDialogWidget(display,windows,"Apply",
        "Enter percent change in color brightness (0-200):",brightness_percent);
      if (*brightness_percent == '\0')
        break;
      /*
        Vary the color brightness.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) CopyMagickString(modulate_factors,brightness_percent,
        MagickPathExtent);
      (void) ModulateImage(*image,modulate_factors,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case GammaCommand:
    {
      static char
        factor[MagickPathExtent] = "1.6";

      /*
        Query user for gamma value.
      */
      (void) XDialogWidget(display,windows,"Gamma",
        "Enter gamma value (e.g. 1.2):",factor);
      if (*factor == '\0')
        break;
      /*
        Gamma correct image.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) GammaImage(*image,strtod(factor,(char **) NULL),exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case SpiffCommand:
    {
      /*
        Sharpen the image contrast.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) ContrastImage(*image,MagickTrue,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case DullCommand:
    {
      /*
        Dull the image contrast.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) ContrastImage(*image,MagickFalse,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case ContrastStretchCommand:
    {
      double
        black_point,
        white_point;

      static char
        levels[MagickPathExtent] = "1%";

      /*
        Query user for gamma value.
      */
      (void) XDialogWidget(display,windows,"Contrast Stretch",
        "Enter black and white points:",levels);
      if (*levels == '\0')
        break;
      /*
        Contrast stretch image.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      flags=ParseGeometry(levels,&geometry_info);
      black_point=geometry_info.rho;
      white_point=(flags & SigmaValue) != 0 ? geometry_info.sigma : black_point;
      if ((flags & PercentValue) != 0)
        {
          black_point*=(double) (*image)->columns*(*image)->rows/100.0;
          white_point*=(double) (*image)->columns*(*image)->rows/100.0;
        }
      white_point=(double) (*image)->columns*(*image)->rows-white_point;
      (void) ContrastStretchImage(*image,black_point,white_point,
        exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case SigmoidalContrastCommand:
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      static char
        levels[MagickPathExtent] = "3x50%";

      /*
        Query user for gamma value.
      */
      (void) XDialogWidget(display,windows,"Sigmoidal Contrast",
        "Enter contrast and midpoint:",levels);
      if (*levels == '\0')
        break;
      /*
        Contrast stretch image.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      flags=ParseGeometry(levels,&geometry_info);
      if ((flags & SigmaValue) == 0)
        geometry_info.sigma=1.0*(double) QuantumRange/2.0;
      if ((flags & PercentValue) != 0)
        geometry_info.sigma=1.0*(double) QuantumRange*geometry_info.sigma/100.0;
      (void) SigmoidalContrastImage(*image,MagickTrue,geometry_info.rho,
        geometry_info.sigma,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case NormalizeCommand:
    {
      /*
        Perform histogram normalization on the image.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) NormalizeImage(*image,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case EqualizeCommand:
    {
      /*
        Perform histogram equalization on the image.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) EqualizeImage(*image,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case NegateCommand:
    {
      /*
        Negate colors in image.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) NegateImage(*image,MagickFalse,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case GrayscaleCommand:
    {
      /*
        Convert image to grayscale.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) SetImageType(*image,(*image)->alpha_trait == UndefinedPixelTrait ?
        GrayscaleType : GrayscaleAlphaType,exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case MapCommand:
      XApplyMapCommand(&context,quantize_info);
      break;
    case QuantizeCommand:
      XApplyQuantizeCommand(&context,quantize_info);
      break;
    case DespeckleCommand:
      XApplyDespeckleCommand(&context);
      break;
    case EmbossCommand:
      XApplyEmbossCommand(&context);
      break;
    case ReduceNoiseCommand:
      XApplyReduceNoiseCommand(&context,&flags);
      break;
    case AddNoiseCommand:
      XApplyAddNoiseCommand(&context);
      break;
    case SharpenCommand:
      XApplySharpenCommand(&context,&flags);
      break;
    case BlurCommand:
      XApplyBlurCommand(&context,&flags);
      break;
    case ThresholdCommand:
      XApplyThresholdCommand(&context);
      break;
    case EdgeDetectCommand:
      XApplyEdgeDetectCommand(&context,&flags);
      break;
    case SpreadCommand:
      XApplySpreadCommand(&context,&flags);
      break;
    case ShadeCommand:
      XApplyShadeCommand(&context);
      break;
    case RaiseCommand:
      XApplyRaiseCommand(&context);
      break;
    case SegmentCommand:
      XApplySegmentCommand(&context);
      break;
    case SepiaToneCommand:
      XApplySepiaToneCommand(&context);
      break;
    case SolarizeCommand:
      XApplySolarizeCommand(&context);
      break;
    case SwirlCommand:
      XApplySwirlCommand(&context,&flags);
      break;
    case ImplodeCommand:
      XApplyImplodeCommand(&context,&flags);
      break;
    case VignetteCommand:
      XApplyVignetteCommand(&context);
      break;
    case WaveCommand:
    {
      Image
        *wave_image;

      static char
        geometry[MagickPathExtent] = "25x150";

      /*
        Query user for the wave geometry.
      */
      (void) XDialogWidget(display,windows,"Wave",
        "Enter the amplitude and length of the wave:",geometry);
      if (*geometry == '\0')
        break;
      /*
        Alter an image along a sine wave.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      flags=ParseGeometry(geometry,&geometry_info);
      if ((flags & SigmaValue) == 0)
        geometry_info.sigma=1.0;
      wave_image=WaveImage(*image,geometry_info.rho,geometry_info.sigma,
        (*image)->interpolate,exception);
      if (wave_image != (Image *) NULL)
        {
          *image=DestroyImage(*image);
          *image=wave_image;
        }
      CatchException(exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case OilPaintCommand:
    {
      Image
        *paint_image;

      static char
        radius[MagickPathExtent] = "0";

      /*
        Query user for circular neighborhood radius.
      */
      (void) XDialogWidget(display,windows,"Oil Paint",
        "Enter the mask radius:",radius);
      if (*radius == '\0')
        break;
      /*
        OilPaint image scanlines.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      flags=ParseGeometry(radius,&geometry_info);
      paint_image=OilPaintImage(*image,geometry_info.rho,geometry_info.sigma,
        exception);
      if (paint_image != (Image *) NULL)
        {
          *image=DestroyImage(*image);
          *image=paint_image;
        }
      CatchException(exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case CharcoalDrawCommand:
    {
      Image
        *charcoal_image;

      static char
        radius[MagickPathExtent] = "0x1";

      /*
        Query user for charcoal radius.
      */
      (void) XDialogWidget(display,windows,"Charcoal Draw",
        "Enter the charcoal radius and sigma:",radius);
      if (*radius == '\0')
        break;
      /*
        Charcoal the image.
      */
      (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
        exception);
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      flags=ParseGeometry(radius,&geometry_info);
      if ((flags & SigmaValue) == 0)
        geometry_info.sigma=geometry_info.rho;
      charcoal_image=CharcoalImage(*image,geometry_info.rho,geometry_info.sigma,
        exception);
      if (charcoal_image != (Image *) NULL)
        {
          *image=DestroyImage(*image);
          *image=charcoal_image;
        }
      CatchException(exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case AnnotateCommand:
    {
      /*
        Annotate the image with text.
      */
      status=XAnnotateEditImage(display,resource_info,windows,*image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to annotate X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case DrawCommand:
    {
      /*
        Draw image.
      */
      status=XDrawEditImage(display,resource_info,windows,image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to draw on the X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case ColorCommand:
    {
      /*
        Color edit.
      */
      status=XColorEditImage(display,resource_info,windows,image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to pixel edit X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case MatteCommand:
    {
      /*
        Matte edit.
      */
      status=XMatteEditImage(display,resource_info,windows,image,exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to matte edit X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case CompositeCommand:
    {
      /*
        Composite image.
      */
      status=XCompositeImage(display,resource_info,windows,*image,
        exception);
      if (status == MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to composite X image",
            (*image)->filename);
          break;
        }
      break;
    }
    case AddBorderCommand:
    {
      Image
        *border_image;

      static char
        geometry[MagickPathExtent] = "6x6";

      /*
        Query user for border color and geometry.
      */
      XColorBrowserWidget(display,windows,"Select",color);
      if (*color == '\0')
        break;
      (void) XDialogWidget(display,windows,"Add Border",
        "Enter border geometry:",geometry);
      if (*geometry == '\0')
        break;
      /*
        Add a border to the image.
      */
      (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
        exception);
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) QueryColorCompliance(color,AllCompliance,&(*image)->border_color,
        exception);
      (void) ParsePageGeometry(*image,geometry,&page_geometry,
        exception);
      border_image=BorderImage(*image,&page_geometry,(*image)->compose,
        exception);
      if (border_image != (Image *) NULL)
        {
          *image=DestroyImage(*image);
          *image=border_image;
        }
      CatchException(exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      windows->image.window_changes.width=(int) (*image)->columns;
      windows->image.window_changes.height=(int) (*image)->rows;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case AddFrameCommand:
    {
      FrameInfo
        frame_info;

      Image
        *frame_image;

      static char
        geometry[MagickPathExtent] = "6x6";

      /*
        Query user for frame color and geometry.
      */
      XColorBrowserWidget(display,windows,"Select",color);
      if (*color == '\0')
        break;
      (void) XDialogWidget(display,windows,"Add Frame","Enter frame geometry:",
        geometry);
      if (*geometry == '\0')
        break;
      /*
        Surround image with an ornamental border.
      */
      (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
        exception);
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) QueryColorCompliance(color,AllCompliance,&(*image)->matte_color,
        exception);
      (void) ParsePageGeometry(*image,geometry,&page_geometry,
        exception);
      frame_info.width=page_geometry.width;
      frame_info.height=page_geometry.height;
      frame_info.outer_bevel=page_geometry.x;
      frame_info.inner_bevel=page_geometry.y;
      frame_info.x=(ssize_t) frame_info.width;
      frame_info.y=(ssize_t) frame_info.height;
      frame_info.width=(*image)->columns+2*frame_info.width;
      frame_info.height=(*image)->rows+2*frame_info.height;
      frame_image=FrameImage(*image,&frame_info,(*image)->compose,exception);
      if (frame_image != (Image *) NULL)
        {
          *image=DestroyImage(*image);
          *image=frame_image;
        }
      CatchException(exception);
      XSetCursorState(display,windows,MagickFalse);
      if (windows->image.orphan != MagickFalse)
        break;
      windows->image.window_changes.width=(int) (*image)->columns;
      windows->image.window_changes.height=(int) (*image)->rows;
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
      (void) XConfigureImage(display,resource_info,windows,*image,exception);
      break;
    }
    case CommentCommand:
    {
      const char
        *value;

      FILE
        *file;

      int
        unique_file;

      /*
        Edit image comment.
      */
      unique_file=AcquireUniqueFileResource(image_info->filename);
      if (unique_file == -1)
        {
          XNoticeWidget(display,windows,"Unable to edit image comment",
            image_info->filename);
          break;
        }
      value=GetImageProperty(*image,"comment",exception);
      if (value == (char *) NULL)
        unique_file=close_utf8(unique_file)-1;
      else
        {
          const char
            *p;

          file=fdopen(unique_file,"w");
          if (file == (FILE *) NULL)
            {
              XNoticeWidget(display,windows,"Unable to edit image comment",
                image_info->filename);
              break;
            }
          for (p=value; *p != '\0'; p++)
            (void) fputc((int) *p,file);
          (void) fputc('\n',file);
          (void) fclose(file);
        }
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      status=InvokeDelegate(image_info,*image,"edit",(char *) NULL,
        exception);
      if (status == MagickFalse)
        XNoticeWidget(display,windows,"Unable to edit image comment",
          (char *) NULL);
      else
        {
          char
            *comment;

          comment=FileToString(image_info->filename,~0UL,exception);
          if (comment != (char *) NULL)
            {
              (void) SetImageProperty(*image,"comment",comment,exception);
              (*image)->taint=MagickTrue;
            }
        }
      (void) RelinquishUniqueFileResource(image_info->filename);
      XSetCursorState(display,windows,MagickFalse);
      break;
    }
    case LaunchCommand:
    {
      /*
        Launch program.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) AcquireUniqueFilename(filename);
      (void) FormatLocaleString((*image)->filename,MagickPathExtent,"launch:%s",
        filename);
      status=WriteImage(image_info,*image,exception);
      if (status == MagickFalse)
        XNoticeWidget(display,windows,"Unable to launch image editor",
          (char *) NULL);
      else
        {
          nexus=ReadImage(resource_info->image_info,exception);
          CatchException(exception);
          XClientMessage(display,windows->image.id,windows->im_protocols,
            windows->im_next_image,CurrentTime);
        }
      (void) RelinquishUniqueFileResource(filename);
      XSetCursorState(display,windows,MagickFalse);
      break;
    }
    case RegionOfInterestCommand:
    {
      /*
        Apply an image processing technique to a region of interest.
      */
      (void) XROIImage(display,resource_info,windows,image,exception);
      break;
    }
    case InfoCommand:
      break;
    case ZoomCommand:
    {
      /*
        Zoom image.
      */
      if (windows->magnify.mapped != MagickFalse)
        (void) XRaiseWindow(display,windows->magnify.id);
      else
        {
          /*
            Make magnify image.
          */
          XSetCursorState(display,windows,MagickTrue);
          (void) XMapRaised(display,windows->magnify.id);
          XSetCursorState(display,windows,MagickFalse);
        }
      break;
    }
    case ShowPreviewCommand:
    {
      char
        **previews;

      Image
        *preview_image;

      PreviewType
        preview;

      static char
        preview_type[MagickPathExtent] = "Gamma";

      /*
        Select preview type from menu.
      */
      previews=GetCommandOptions(MagickPreviewOptions);
      if (previews == (char **) NULL)
        break;
      XListBrowserWidget(display,windows,&windows->widget,
        (const char **) previews,"Preview",
        "Select an enhancement, effect, or F/X:",preview_type);
      previews=DestroyStringList(previews);
      if (*preview_type == '\0')
        break;
      /*
        Show image preview.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      preview=(PreviewType) ParseCommandOption(MagickPreviewOptions,
        MagickFalse,preview_type);
      (void) FormatImageProperty(*image,"group","%.17g",(double)
        windows->image.id);
      (void) DeleteImageProperty(*image,"label");
      (void) SetImageProperty(*image,"label","Preview",exception);
      preview_image=PreviewImage(*image,preview,exception);
      if (preview_image == (Image *) NULL)
        break;
      (void) AcquireUniqueFilename(filename);
      (void) FormatLocaleString(preview_image->filename,MagickPathExtent,
        "show:%s",filename);
      status=WriteImage(image_info,preview_image,exception);
      (void) RelinquishUniqueFileResource(filename);
      preview_image=DestroyImage(preview_image);
      if (status == MagickFalse)
        XNoticeWidget(display,windows,"Unable to show image preview",
          (*image)->filename);
      XDelay(display,1500);
      XSetCursorState(display,windows,MagickFalse);
      break;
    }
    case ShowHistogramCommand:
    {
      Image
        *histogram_image;

      /*
        Show image histogram.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) DeleteImageProperty(*image,"label");
      (void) FormatImageProperty(*image,"group","%.17g",(double)
        windows->image.id);
      (void) SetImageProperty(*image,"label","Histogram",exception);
      (void) AcquireUniqueFilename(filename);
      (void) FormatLocaleString((*image)->filename,MagickPathExtent,
        "histogram:%s",filename);
      status=WriteImage(image_info,*image,exception);
      (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
      histogram_image=ReadImage(image_info,exception);
      (void) RelinquishUniqueFileResource(filename);
      if (histogram_image == (Image *) NULL)
        break;
      (void) FormatLocaleString(histogram_image->filename,MagickPathExtent,
        "show:%s",filename);
      status=WriteImage(image_info,histogram_image,exception);
      histogram_image=DestroyImage(histogram_image);
      if (status == MagickFalse)
        XNoticeWidget(display,windows,"Unable to show histogram",
          (*image)->filename);
      XDelay(display,1500);
      XSetCursorState(display,windows,MagickFalse);
      break;
    }
    case ShowMatteCommand:
    {
      Image
        *matte_image;

      if ((*image)->alpha_trait == UndefinedPixelTrait)
        {
          XNoticeWidget(display,windows,
            "Image does not have any matte information",(*image)->filename);
          break;
        }
      /*
        Show image matte.
      */
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      (void) FormatImageProperty(*image,"group","%.17g",(double)
        windows->image.id);
      (void) DeleteImageProperty(*image,"label");
      (void) SetImageProperty(*image,"label","Matte",exception);
      (void) AcquireUniqueFilename(filename);
      (void) FormatLocaleString((*image)->filename,MagickPathExtent,"matte:%s",
        filename);
      status=WriteImage(image_info,*image,exception);
      (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
      matte_image=ReadImage(image_info,exception);
      (void) RelinquishUniqueFileResource(filename);
      if (matte_image == (Image *) NULL)
        break;
      (void) FormatLocaleString(matte_image->filename,MagickPathExtent,
        "show:%s",filename);
      status=WriteImage(image_info,matte_image,exception);
      matte_image=DestroyImage(matte_image);
      if (status == MagickFalse)
        XNoticeWidget(display,windows,"Unable to show matte",
          (*image)->filename);
      XDelay(display,1500);
      XSetCursorState(display,windows,MagickFalse);
      break;
    }
    case BackgroundCommand:
    {
      /*
        Background image.
      */
      status=XBackgroundImage(display,resource_info,windows,image,exception);
      if (status == MagickFalse)
        break;
      nexus=CloneImage(*image,0,0,MagickTrue,exception);
      if (nexus != (Image *) NULL)
        XClientMessage(display,windows->image.id,windows->im_protocols,
          windows->im_next_image,CurrentTime);
      break;
    }
    case SlideShowCommand:
    {
      static char
        delay[MagickPathExtent] = "5";

      /*
        Display next image after pausing.
      */
      (void) XDialogWidget(display,windows,"Slide Show",
        "Pause how many 1/100ths of a second between images:",delay);
      if (*delay == '\0')
        break;
      resource_info->delay=StringToUnsignedLong(delay);
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_next_image,CurrentTime);
      break;
    }
    case PreferencesCommand:
    {
      /*
        Set user preferences.
      */
      status=XPreferencesWidget(display,resource_info,windows);
      if (status == MagickFalse)
        break;
      nexus=CloneImage(*image,0,0,MagickTrue,exception);
      if (nexus != (Image *) NULL)
        XClientMessage(display,windows->image.id,windows->im_protocols,
          windows->im_next_image,CurrentTime);
      break;
    }
    case HelpCommand:
    {
      /*
        User requested help.
      */
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Display",DisplayHelp);
      break;
    }
    case BrowseDocumentationCommand:
    {
      Atom
        mozilla_atom;

      Window
        mozilla_window,
        root_window;

      /*
        Browse the ImageMagick documentation.
      */
      root_window=XRootWindow(display,XDefaultScreen(display));
      mozilla_atom=XInternAtom(display,"_MOZILLA_VERSION",MagickFalse);
      mozilla_window=XWindowByProperty(display,root_window,mozilla_atom);
      if (mozilla_window != (Window) NULL)
        {
          char
            command[MagickPathExtent];

          /*
            Display documentation using Netscape remote control.
          */
          (void) FormatLocaleString(command,MagickPathExtent,
            "openurl(%s,new-tab)",MagickAuthoritativeURL);
          mozilla_atom=XInternAtom(display,"_MOZILLA_COMMAND",MagickFalse);
          (void) XChangeProperty(display,mozilla_window,mozilla_atom,XA_STRING,
            8,PropModeReplace,(unsigned char *) command,(int) strlen(command));
          XSetCursorState(display,windows,MagickFalse);
          break;
        }
      XSetCursorState(display,windows,MagickTrue);
      XCheckRefreshWindows(display,windows);
      status=InvokeDelegate(image_info,*image,"browse",(char *) NULL,
        exception);
      if (status == MagickFalse)
        XNoticeWidget(display,windows,"Unable to browse documentation",
          (char *) NULL);
      XDelay(display,1500);
      XSetCursorState(display,windows,MagickFalse);
      break;
    }
    case VersionCommand:
    {
      XNoticeWidget(display,windows,GetMagickVersion((size_t *) NULL),
        GetMagickCopyright());
      break;
    }
    case SaveToUndoBufferCommand:
      break;
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
  image_info=DestroyImageInfo(image_info);
  return(nexus);
}

static void XUpdateMagnifyInfo(Display *display,XWindows *windows,const int x,
  const int y)
{
  char
    text[MagickPathExtent];

  if (windows->info.mapped != MagickFalse)
    {
      if ((x < (windows->info.x+(int) windows->info.width)) &&
          (y < (windows->info.y+(int) windows->info.height)))
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
    }
  else
    if ((x > (windows->info.x+(int) windows->info.width)) ||
        (y > (windows->info.y+(int) windows->info.height)))
      (void) XMapWindow(display,windows->info.id);
  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+d%+d ",
        windows->magnify.x,windows->magnify.y);
      XInfoWidget(display,windows,text);
    }
}

static void XClampMagnifyPosition(XWindows *windows,int *x,int *y)
{
  if ((*x) < 0)
    (*x)=0;
  else
    if ((*x) >= (int) windows->image.width)
      (*x)=(int) windows->image.width-1;
  if ((*y) < 0)
    (*y)=0;
  else
   if ((*y) >= (int) windows->image.height)
     (*y)=(int) windows->image.height-1;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g n i f y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagnifyImage() magnifies portions of the image as indicated by the pointer.
%  The magnified portion is displayed in a separate window.
%
%  The format of the XMagnifyImage method is:
%
%      void XMagnifyImage(Display *display,XWindows *windows,XEvent *event,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XMagnifyImage(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  int
    x,
    y;

  size_t
    state;

  /*
    Update magnified image until the mouse button is released.
  */
  (void) XCheckDefineCursor(display,windows->image.id,windows->magnify.cursor);
  state=DefaultState;
  x=event->xbutton.x;
  y=event->xbutton.y;
  windows->magnify.x=(int) windows->image.x+x;
  windows->magnify.y=(int) windows->image.y+y;
  do
  {
    /*
      Map and unmap Info widget as text cursor crosses its boundaries.
    */
    XUpdateMagnifyInfo(display,windows,x,y);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    switch (event->type)
    {
      case ButtonPress:
        break;
      case ButtonRelease:
      {
        /*
          User has finished magnifying image.
        */
        x=event->xbutton.x;
        y=event->xbutton.y;
        state|=ExitState;
        break;
      }
      case Expose:
        break;
      case MotionNotify:
      {
        x=event->xmotion.x;
        y=event->xmotion.y;
        break;
      }
      default:
        break;
    }
    /*
      Check boundary conditions.
    */
    XClampMagnifyPosition(windows,&x,&y);
  } while ((state & ExitState) == 0);
  /*
    Display magnified image.
  */
  XSetCursorState(display,windows,MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a g n i f y W i n d o w C o m m a n d                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMagnifyWindowCommand() moves the image within an Magnify window by one
%  pixel as specified by the key symbol.
%
%  The format of the XMagnifyWindowCommand method is:
%
%      void XMagnifyWindowCommand(Display *display,XWindows *windows,
%        const MagickStatusType state,const KeySym key_symbol,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o state: key mask.
%
%    o key_symbol: Specifies a KeySym which indicates which side of the image
%      to trim.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XSelectMagnifyDigit(XWindows *windows,const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_0:
    case XK_1:
    case XK_2:
    case XK_3:
    case XK_4:
    case XK_5:
    case XK_6:
    case XK_7:
    case XK_8:
    case XK_9:
    {
      windows->magnify.data=(key_symbol-XK_0);
      break;
    }
    case XK_KP_0:
    case XK_KP_1:
    case XK_KP_2:
    case XK_KP_3:
    case XK_KP_4:
    case XK_KP_5:
    case XK_KP_6:
    case XK_KP_7:
    case XK_KP_8:
    case XK_KP_9:
    {
      windows->magnify.data=(key_symbol-XK_KP_0);
      break;
    }
    default:
      break;
  }
}

static void XMoveMagnifyRightDown(XWindows *windows,const unsigned int quantum,
  const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Right:
    case XK_KP_Right:
    {
      if (windows->magnify.x < (int) (windows->image.ximage->width-1))
        windows->magnify.x+=(int) quantum;
      break;
    }
    case XK_Down:
    case XK_KP_Down:
    {
      if (windows->magnify.y < (int) (windows->image.ximage->height-1))
        windows->magnify.y+=(int) quantum;
      break;
    }
    default:
    {
      XSelectMagnifyDigit(windows,key_symbol);
      break;
    }
  }
}

static void XMoveMagnifyPosition(XWindows *windows,const unsigned int quantum,
  const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case XK_Left:
    case XK_KP_Left:
    {
      if (windows->magnify.x > 0)
        windows->magnify.x-=(int) quantum;
      break;
    }
    case XK_Up:
    case XK_KP_Up:
    {
      if (windows->magnify.y > 0)
        windows->magnify.y-=(int) quantum;
      break;
    }
    default:
    {
      XMoveMagnifyRightDown(windows,quantum,key_symbol);
      break;
    }
  }
}

static void XMagnifyWindowCommand(Display *display,XWindows *windows,
  const MagickStatusType state,const KeySym key_symbol,ExceptionInfo *exception)
{
  unsigned int
    quantum;

  /*
    User specified a magnify factor or position.
  */
  quantum=1;
  if ((state & Mod1Mask) != 0)
    quantum=10;
  switch ((int) key_symbol)
  {
    case QuitCommand:
    {
      (void) XWithdrawWindow(display,windows->magnify.id,
        windows->magnify.screen);
      break;
    }
    case XK_Home:
    case XK_KP_Home:
    {
      windows->magnify.x=(int) windows->image.width/2;
      windows->magnify.y=(int) windows->image.height/2;
      break;
    }
    default:
    {
      XMoveMagnifyPosition(windows,quantum,key_symbol);
      break;
    }
  }
  XMakeMagnifyImage(display,windows,exception);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a k e P a n I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMakePanImage() creates a thumbnail of the image and displays it in the Pan
%  icon window.
%
%  The format of the XMakePanImage method is:
%
%        void XMakePanImage(Display *display,XResourceInfo *resource_info,
%          XWindows *windows,Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XMakePanImage(Display *display,XResourceInfo *resource_info,
  XWindows *windows,Image *image,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  /*
    Create and display image for panning icon.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  windows->pan.x=(int) windows->image.x;
  windows->pan.y=(int) windows->image.y;
  status=XMakeImage(display,resource_info,&windows->pan,image,
    windows->pan.width,windows->pan.height,exception);
  if (status == MagickFalse)
    ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) XSetWindowBackgroundPixmap(display,windows->pan.id,
    windows->pan.pixmap);
  (void) XClearWindow(display,windows->pan.id);
  XDrawPanRectangle(display,windows);
  XSetCursorState(display,windows,MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X M a t t a E d i t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XMatteEditImage() allows the user to interactively change the Matte channel
%  of an image.  If the image is PseudoClass it is promoted to DirectClass
%  before the matte information is stored.
%
%  The format of the XMatteEditImage method is:
%
%      MagickBooleanType XMatteEditImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XReplaceRowAlpha(Image **image,Quantum *q,const PixelInfo target,
  const char *matte)
{
  int
    x;

  PixelInfo
    pixel;

  for (x=0; x < (int) (*image)->columns; x++)
  {
    GetPixelInfoPixel(*image,q,&pixel);
    if (IsFuzzyEquivalencePixelInfo(&pixel,&target))
      SetPixelAlpha(*image,(Quantum) StringToLong(matte),q);
    q+=(ptrdiff_t) GetPixelChannels(*image);
  }
}

static void XDiscardMagnifyKeys(Display *display,XWindows *windows,
  XEvent *event)
{
  if ((*event).xkey.window == windows->magnify.id)
    {
      Window
        window;

      window=windows->magnify.id;
      while (XCheckWindowEvent(display,window,KeyPressMask,event)) ;
    }
}

static void XSelectPaintMethod(const EditMenuInput *menu,
  const PaintMethodTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const MatteEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  PaintMethod
    *const method = targets->method;

  char
    **methods;

  int
    entry;

  /*
    Select a method from the pop-up menu.
  */
  methods=GetCommandOptions(MagickMethodOptions);
  if (methods == (char **) NULL)
    return;
  entry=XMenuWidget(display,windows,MatteEditMenu[id],
    (const char **) methods,command);
  if (entry >= 0)
    (*method)=(PaintMethod) ParseCommandOption(MagickMethodOptions,
      MagickFalse,methods[entry]);
  methods=DestroyStringList(methods);
}

static void XSelectFuzzFactor(const EditMenuInput *menu,
  const char *const *FuzzMenu,char *fuzz,Image **image)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const MatteEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  entry=XMenuWidget(display,windows,MatteEditMenu[id],FuzzMenu,
    command);
  if (entry < 0)
    return;
  if (entry != 5)
    {
      (*image)->fuzz=StringToDoubleInterval(FuzzMenu[entry],(double)
        QuantumRange+1.0);
      return;
    }
  (void) CopyMagickString(fuzz,"20%",MagickPathExtent);
  (void) XDialogWidget(display,windows,"Ok",
    "Enter fuzz factor (0.0 - 99.9%):",fuzz);
  if (*fuzz == '\0')
    return;
  (void) ConcatenateMagickString(fuzz,"%",MagickPathExtent);
  (*image)->fuzz=StringToDoubleInterval(fuzz,(double) QuantumRange+
    1.0);
}

typedef struct _MatteEditPixelInput
{
  Image
    **image;

  CacheView
    *image_view;

  int
    x_offset,
    y_offset;

  const char
    *matte;

  ExceptionInfo
    *exception;

  XResourceInfo
    *resource_info;

  PaintMethod
    method;

  XColor
    border_color;
} MatteEditPixelInput;

static void XMattePoint(const MatteEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  const char
    *const matte = pixel_input->matte;

  ExceptionInfo
    *const exception = pixel_input->exception;

  Quantum
    *q;

  /*
    Update matte information using point algorithm.
  */
  q=GetCacheViewAuthenticPixels(image_view,(ssize_t) x_offset,
    (ssize_t) y_offset,1,1,exception);
  if (q == (Quantum *) NULL)
    return;
  SetPixelAlpha(*image,(Quantum) StringToLong(matte),q);
  (void) SyncCacheViewAuthenticPixels(image_view,exception);
}

static void XMatteReplace(const MatteEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  const char
    *const matte = pixel_input->matte;

  ExceptionInfo
    *const exception = pixel_input->exception;

  int
    y;

  PixelInfo
    target;

  Quantum
    *q;

  /*
    Update matte information using replace algorithm.
  */
  (void) GetOneCacheViewVirtualPixelInfo(image_view,(ssize_t)
    x_offset,(ssize_t) y_offset,&target,exception);
  for (y=0; y < (int) (*image)->rows; y++)
  {
    q=GetCacheViewAuthenticPixels(image_view,0,(ssize_t) y,
      (*image)->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    XReplaceRowAlpha(image,q,target,matte);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

static void XMatteReset(const MatteEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  CacheView
    *const image_view = pixel_input->image_view;

  const char
    *const matte = pixel_input->matte;

  ExceptionInfo
    *const exception = pixel_input->exception;

  int
    x,
    y;

  Quantum
    *q;

  for (y=0; y < (int) (*image)->rows; y++)
  {
    q=QueueCacheViewAuthenticPixels(image_view,0,(ssize_t) y,
      (*image)->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    for (x=0; x < (int) (*image)->columns; x++)
    {
      SetPixelAlpha(*image,(Quantum) StringToLong(matte),q);
      q+=(ptrdiff_t) GetPixelChannels(*image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
  if (StringToLong(matte) == (long) OpaqueAlpha)
    (*image)->alpha_trait=UndefinedPixelTrait;
}

static void XMatteFloodfill(const MatteEditPixelInput *pixel_input)
{
  Image
    **const image = pixel_input->image;

  const int
    x_offset = pixel_input->x_offset,
    y_offset = pixel_input->y_offset;

  const char
    *const matte = pixel_input->matte;

  ExceptionInfo
    *const exception = pixel_input->exception;

  XResourceInfo
    *const resource_info = pixel_input->resource_info;

  const PaintMethod
    method = pixel_input->method;

  const XColor
    border_color = pixel_input->border_color;

  ChannelType
    channel_mask;

  DrawInfo
    *draw_info;

  PixelInfo
    target;

  /*
    Update matte information using floodfill algorithm.
  */
  (void) GetOneVirtualPixelInfo(*image,
    GetPixelCacheVirtualMethod(*image),(ssize_t) x_offset,(ssize_t)
    y_offset,&target,exception);
  if (method == FillToBorderMethod)
    {
      target.red=(double) ScaleShortToQuantum(
        border_color.red);
      target.green=(double) ScaleShortToQuantum(
        border_color.green);
      target.blue=(double) ScaleShortToQuantum(
        border_color.blue);
    }
  draw_info=CloneDrawInfo(resource_info->image_info,
    (DrawInfo *) NULL);
  draw_info->fill.alpha=(double) ClampToQuantum(
    StringToDouble(matte,(char **) NULL));
  channel_mask=SetImageChannelMask(*image,AlphaChannel);
  (void) FloodfillPaintImage(*image,draw_info,&target,(ssize_t)
    x_offset,(ssize_t) y_offset,
    method != FloodfillMethod ? MagickTrue : MagickFalse,exception);
  (void) SetPixelChannelMask(*image,channel_mask);
  draw_info=DestroyDrawInfo(draw_info);
}

static MagickBooleanType XMatteEditPixels(
  const MatteEditPixelInput pixel_input,const PaintMethod method,
  Image **image,ExceptionInfo *exception)
{
  switch (method)
  {
    case PointMethod:
    default:
    {
      XMattePoint(&pixel_input);
      break;
    }
    case ReplaceMethod:
    {
      XMatteReplace(&pixel_input);
      break;
    }
    case FloodfillMethod:
    case FillToBorderMethod:
    {
      XMatteFloodfill(&pixel_input);
      break;
    }
    case ResetMethod:
    {
      /*
        Update matte information using reset algorithm.
      */
      if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
      XMatteReset(&pixel_input);
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _BorderColorTargets
{
  XColor
    *border_color;
} BorderColorTargets;

static void XSelectBorderColor(const EditMenuInput *menu,
  XResourceInfo *resource_info,const BorderColorTargets *targets)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const MatteEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  XColor
    *const border_color = targets->border_color;

  const char
    *ColorMenu[MaxNumberPens];

  int
    pen_number;

  int
    i;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="Browser...";
  ColorMenu[MaxNumberPens-1]=(const char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  pen_number=XMenuWidget(display,windows,MatteEditMenu[id],
    (const char **) ColorMenu,command);
  if (pen_number < 0)
    return;
  if (pen_number == (MaxNumberPens-2))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set border color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],border_color);
}

static void XSelectMatteValue(const EditMenuInput *menu,
  const char *const *MatteMenu,char *message,char *matte)
{
  Display
    *const display = menu->display;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const MatteEditMenu = menu->MatteEditMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  entry=XMenuWidget(display,windows,MatteEditMenu[id],MatteMenu,
    command);
  if (entry < 0)
    return;
  if (entry != 2)
    {
      (void) FormatLocaleString(matte,MagickPathExtent,"%g",
        (double) OpaqueAlpha);
      if (LocaleCompare(MatteMenu[entry],"Transparent") == 0)
        (void) FormatLocaleString(matte,MagickPathExtent,"%g",
          (double) TransparentAlpha);
      return;
    }
  (void) FormatLocaleString(message,MagickPathExtent,
    "Enter matte value (0 - " "%g" "):",(double) QuantumRange);
  (void) XDialogWidget(display,windows,"Matte",message,matte);
  if (*matte == '\0')
    return;
}

typedef struct _MatteCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const ModeType
    *MatteEditCommands;

  Image
    **image;

  ExceptionInfo
    *exception;

  const char
    *const *MatteEditMenu;

  char
    *command,
    *matte;

  PaintMethodTargets
    method_targets;

  BorderColorTargets
    border_targets;

  char
    *text;
} MatteCommandInput;

static void XMatteEditControl(const MatteCommandInput *input,const int id,
  size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const MatteEditCommands = input->MatteEditCommands;

  Image
    **const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  switch (MatteEditCommands[id])
  {
    case MatteEditUndoCommand:
    {
      (void) XMagickCommand(display,resource_info,windows,UndoCommand,
        image,exception);
      break;
    }
    case MatteEditHelpCommand:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Matte Edit",ImageMatteEditHelp);
      break;
    }
    case MatteEditDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static void XMatteEditValues(const MatteCommandInput *input,const int id,
  size_t *state)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const ModeType
    *const MatteEditCommands = input->MatteEditCommands;

  Image
    **const image = input->image;

  const char
    *const *const MatteEditMenu = input->MatteEditMenu;

  EditMenuInput
    menu_input;

  switch (MatteEditCommands[id])
  {
    case MatteEditFuzzCommand:
    {
      const char
        *const FuzzMenu[] =
        {
          "0%",
          "2%",
          "5%",
          "10%",
          "15%",
          "Dialog...",
          (char *) NULL,
        };

      static char
        fuzz[MagickPathExtent];

      menu_input.display=display;
      menu_input.windows=windows;
      menu_input.MatteEditMenu=MatteEditMenu;
      menu_input.id=id;
      menu_input.command=input->command;
      XSelectFuzzFactor(&menu_input,FuzzMenu,fuzz,image);
      break;
    }
    case MatteEditValueCommand:
    {
      const char
        *const MatteMenu[] =
        {
          "Opaque",
          "Transparent",
          "Dialog...",
          (char *) NULL,
        };

      static char
        message[MagickPathExtent];

      menu_input.display=display;
      menu_input.windows=windows;
      menu_input.MatteEditMenu=MatteEditMenu;
      menu_input.id=id;
      menu_input.command=input->command;
      XSelectMatteValue(&menu_input,MatteMenu,message,input->matte);
      break;
    }
    default:
    {
      XMatteEditControl(input,id,state);
      break;
    }
  }
}

static MagickBooleanType XMatteCommandBlock(const MatteCommandInput *input,
  XEvent *event,const Cursor cursor,size_t *state)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  const char
    *const *const MatteEditMenu = input->MatteEditMenu;

  const ModeType
    *const MatteEditCommands = input->MatteEditCommands;

  const PaintMethodTargets
    method_targets = input->method_targets;

  const BorderColorTargets
    border_targets = input->border_targets;

  int
    id;

  if ((*event).xany.window == windows->command.id)
    {
      EditMenuInput
        menu_input;

      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,MatteEditMenu,event);
      if (id < 0)
        {
          (void) XCheckDefineCursor(display,windows->image.id,cursor);
          return(MagickTrue);
        }
      switch (MatteEditCommands[id])
      {
        case MatteEditMethod:
        {
          menu_input.display=display;
          menu_input.windows=windows;
          menu_input.MatteEditMenu=MatteEditMenu;
          menu_input.id=id;
          menu_input.command=input->command;
          XSelectPaintMethod(&menu_input,&method_targets);
          break;
        }
        case MatteEditBorderCommand:
        {
          menu_input.display=display;
          menu_input.windows=windows;
          menu_input.MatteEditMenu=MatteEditMenu;
          menu_input.id=id;
          menu_input.command=input->command;
          XSelectBorderColor(&menu_input,input->resource_info,&border_targets);
          break;
        }
        default:
        {
          XMatteEditValues(input,id,state);
          break;
        }
      }
      (void) XCheckDefineCursor(display,windows->image.id,cursor);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static MagickBooleanType XMatteWritePixels(const MatteCommandInput *input,
  const int x_offset,const int y_offset)
{
  Image
    **const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  const PaintMethod
    method = *input->method_targets.method;

  const XColor
    border_color = *input->border_targets.border_color;

  CacheView
    *image_view;

  MatteEditPixelInput
    pixel_input;

  if (SetImageStorageClass(*image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  if ((*image)->alpha_trait == UndefinedPixelTrait)
    (void) SetImageAlphaChannel(*image,OpaqueAlphaChannel,exception);
  image_view=AcquireAuthenticCacheView(*image,exception);
  pixel_input.image=image;
  pixel_input.image_view=image_view;
  pixel_input.x_offset=x_offset;
  pixel_input.y_offset=y_offset;
  pixel_input.matte=input->matte;
  pixel_input.exception=exception;
  pixel_input.resource_info=input->resource_info;
  pixel_input.method=method;
  pixel_input.border_color=border_color;
  if (XMatteEditPixels(pixel_input,method,image,exception) == MagickFalse)
    return(MagickFalse);
  image_view=DestroyCacheView(image_view);
  return(MagickTrue);
}

static MagickBooleanType XMatteUpdatePixels(const MatteCommandInput *input,
  int x_offset,int y_offset,size_t *state)
{
  XWindows
    *const windows = input->windows;

  Image
    **const image = input->image;

  unsigned int
    height,
    width;

  if (((*state) & UpdateConfigurationState) != 0)
    {
      int
        x,
        y;

      /*
        Matte edit is relative to image configuration.
      */
      (void) XClearArea(input->display,windows->image.id,x_offset,y_offset,1,1,
        MagickTrue);
      XPutPixel(windows->image.ximage,x_offset,y_offset,
        windows->pixel_info->background_color.pixel);
      width=(unsigned int) (*image)->columns;
      height=(unsigned int) (*image)->rows;
      x=0;
      y=0;
      if (windows->image.crop_geometry != (char *) NULL)
        (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,
          &height);
      x_offset=((int) width*(windows->image.x+x_offset)/
        windows->image.ximage->width+x);
      y_offset=((int) height*(windows->image.y+y_offset)/
        windows->image.ximage->height+y);
      if ((x_offset < 0) || (y_offset < 0))
        return(MagickTrue);
      if ((x_offset >= (int) (*image)->columns) ||
          (y_offset >= (int) (*image)->rows))
        return(MagickTrue);
      if (XMatteWritePixels(input,x_offset,y_offset) == MagickFalse)
        return(MagickFalse);
      (*state)&=(unsigned int) (~UpdateConfigurationState);
    }
  return(MagickTrue);
}

static size_t XMatteKeyPress(const MatteCommandInput *input,XEvent *event,
  size_t state)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  XDiscardMagnifyKeys(display,windows,event);
  if ((*event).xkey.window != windows->image.id)
    return(state);
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      state|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XTextViewHelp(display,input->resource_info,windows,MagickFalse,
        "Help Viewer - Matte Edit",ImageMatteEditHelp);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
  return(state);
}

typedef struct _MatteEventTargets
{
  int
    *x,
    *y;

  size_t
    *state;
} MatteEventTargets;

static void XMattePress(const MatteCommandInput *input,const XEvent *event,
  const MatteEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  if ((*event).xbutton.button != Button1)
    return;
  if (((*event).xbutton.window != windows->image.id) &&
      ((*event).xbutton.window != windows->magnify.id))
    return;
  /*
    Update matte data.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  (void) XMagickCommand(display,resource_info,windows,
    SaveToUndoBufferCommand,image,exception);
  (*state)|=UpdateConfigurationState;
}

static void XMatteRelease(const MatteCommandInput *input,const XEvent *event,
  const Cursor cursor,const MatteEventTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  if ((*event).xbutton.button != Button1)
    return;
  if (((*event).xbutton.window != windows->image.id) &&
      ((*event).xbutton.window != windows->magnify.id))
    return;
  /*
    Update colormap information.
  */
  (*x)=(*event).xbutton.x;
  (*y)=(*event).xbutton.y;
  XConfigureImageColormap(display,resource_info,windows,*image,exception);
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
  XInfoWidget(display,windows,input->text);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (*state)&=(unsigned int) (~UpdateConfigurationState);
}

static void XMatteEditEvent(const MatteCommandInput *input,XEvent *event,
  const Cursor cursor,const MatteEventTargets *targets)
{
  Display
    *const display = input->display;

  XWindows
    *const windows = input->windows;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XMattePress(input,event,targets);
      break;
    }
    case ButtonRelease:
    {
      XMatteRelease(input,event,cursor,targets);
      break;
    }
    case Expose:
      break;
    case KeyPress:
    {
      (*state)=XMatteKeyPress(input,event,(*state));
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(display,windows,(*x),(*y));
      break;
    }
    default:
      break;
  }
}

static MagickBooleanType XMatteEditStep(const MatteCommandInput *input,
  const Cursor cursor,const MatteEventTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  int
    x_offset,
    y_offset;

  XEvent
    event;

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(input->text,MagickPathExtent," %+d%+d ",
        (*x)+windows->image.x,(*y)+windows->image.y);
      XInfoWidget(display,windows,input->text);
    }
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  if (XMatteCommandBlock(input,&event,cursor,state) != MagickFalse)
    return(MagickTrue);
  XMatteEditEvent(input,&event,cursor,targets);
  if (event.xany.window == windows->magnify.id)
    {
      (*x)=windows->magnify.x-windows->image.x;
      (*y)=windows->magnify.y-windows->image.y;
    }
  x_offset=(*x);
  y_offset=(*y);
  if (XMatteUpdatePixels(input,x_offset,y_offset,state) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}

static const ModeType
  MatteEditCommandTable[] =
  {
    MatteEditMethod,
    MatteEditBorderCommand,
    MatteEditFuzzCommand,
    MatteEditValueCommand,
    MatteEditUndoCommand,
    MatteEditHelpCommand,
    MatteEditDismissCommand
  };

static MagickBooleanType XMatteEditImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  const char
    *const MatteEditMenu[] =
    {
      "Method",
      "Border Color",
      "Fuzz",
      "Matte Value",
      "Undo",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static char
    matte[MagickPathExtent] = "0";

  static PaintMethod
    method = PointMethod;

  static XColor
    border_color = { 0, 0, 0, 0, 0, 0 };

  char
    command[MagickPathExtent],
    text[MagickPathExtent];

  Cursor
    cursor;

  int
    x,
    y;

  size_t
    state;

  const PaintMethodTargets
    method_targets = { &method };

  const BorderColorTargets
    border_targets = { &border_color };

  const MatteEventTargets
    event_targets = { &x, &y, &state };

  const MatteCommandInput
    command_input = { display, resource_info, windows, MatteEditCommandTable,
      image, exception, MatteEditMenu, command, matte, method_targets,
      border_targets, text };

  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Matte Edit");
  windows->command.data=4;
  (void) XCommandWidget(display,windows,MatteEditMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Make cursor.
  */
  cursor=XMakeCursor(display,windows->image.id,windows->map_info->colormap,
    resource_info->background_color,resource_info->foreground_color);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  state=DefaultState;
  do
  {
    if (XMatteEditStep(&command_input,cursor,&event_targets) == MagickFalse)
      return(MagickFalse);
  } while ((state & ExitState) == 0);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeCursor(display,cursor);
  return(MagickTrue);
}

static MagickBooleanType XSelectCommandLineFile(Display *display,
  XWindows *windows,char *filename)
{
  char
    **filelist,
    **files;

  int
    count,
    status;

  int
    i,
    j;

  /*
    Select next image from the command line.
  */
  status=XGetCommand(display,windows->image.id,&files,&count);
  if (status == 0)
    {
      ThrowXWindowException(XServerError,"UnableToGetProperty","...");
      return(MagickFalse);
    }
  filelist=(char **) AcquireQuantumMemory((size_t) count,sizeof(*filelist));
  if (filelist == (char **) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        "...");
      (void) XFreeStringList(files);
      return(MagickFalse);
    }
  j=0;
  for (i=1; i < count; i++)
    if (*files[i] != '-')
      filelist[j++]=files[i];
  filelist[j]=(char *) NULL;
  XListBrowserWidget(display,windows,&windows->widget,
    (const char **) filelist,"Load","Select Image to Load:",filename);
  filelist=(char **) RelinquishMagickMemory(filelist);
  (void) XFreeStringList(files);
  return(MagickTrue);
}

static MagickBooleanType XSelectOpenFile(Display *display,XWindows *windows,
  const MagickBooleanType command,char *filename)
{
  if (command == MagickFalse)
    XFileBrowserWidget(display,windows,"Open",filename);
  else
    {
      if (XSelectCommandLineFile(display,windows,filename) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static MagickBooleanType XGrabDelay(Display *display,XWindows *windows,
  const ImageInfo *image_info)
{
  if (LocaleCompare(image_info->magick,"X") == 0)
    {
      char
        seconds[MagickPathExtent];

      /*
        User may want to delay the X server screen grab.
      */
      (void) CopyMagickString(seconds,"0",MagickPathExtent);
      (void) XDialogWidget(display,windows,"Grab","Enter any delay in seconds:",
        seconds);
      if (*seconds == '\0')
        return(MagickFalse);
      XDelay(display,(size_t) (1000*StringToLong(seconds)));
    }
  return(MagickTrue);
}

static void XRequestImageSize(Display *display,XWindows *windows,
  ImageInfo *image_info,const MagickInfo *magick_info)
{
  if ((magick_info != (const MagickInfo *) NULL) &&
      GetMagickRawSupport(magick_info) == MagickTrue)
    {
      char
        geometry[MagickPathExtent];

      /*
        Request image size from the user.
      */
      (void) CopyMagickString(geometry,"512x512",MagickPathExtent);
      if (image_info->size != (char *) NULL)
        (void) CopyMagickString(geometry,image_info->size,MagickPathExtent);
      (void) XDialogWidget(display,windows,"Load","Enter the image geometry:",
        geometry);
      (void) CloneString(&image_info->size,geometry);
    }
}

static void XDestroyTextList(char **textlist)
{
  int
    i;

  for (i=0; textlist[i] != (char *) NULL; i++)
    textlist[i]=DestroyString(textlist[i]);
  textlist=(char **) RelinquishMagickMemory(textlist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X O p e n I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XOpenImage() loads an image from a file.
%
%  The format of the XOpenImage method is:
%
%     Image *XOpenImage(Display *display,XResourceInfo *resource_info,
%       XWindows *windows,const unsigned int command)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o command: A value other than zero indicates that the file is selected
%      from the command line argument list.
%
*/
static Image *XOpenImage(Display *display,XResourceInfo *resource_info,
  XWindows *windows,const MagickBooleanType command)
{
  const MagickInfo
    *magick_info;

  ExceptionInfo
    *exception;

  Image
    *nexus;

  ImageInfo
    *image_info;

  static char
    filename[MagickPathExtent] = "\0";

  /*
    Request file name from user.
  */
  if (XSelectOpenFile(display,windows,command,filename) == MagickFalse)
    return((Image *) NULL);
  if (*filename == '\0')
    return((Image *) NULL);
  image_info=CloneImageInfo(resource_info->image_info);
  (void) SetImageInfoProgressMonitor(image_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  exception=AcquireExceptionInfo();
  (void) SetImageInfo(image_info,0,exception);
  if (XGrabDelay(display,windows,image_info) == MagickFalse)
    return((Image *) NULL);
  magick_info=GetMagickInfo(image_info->magick,exception);
  XRequestImageSize(display,windows,image_info,magick_info);
  /*
    Load the image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  nexus=ReadImage(image_info,exception);
  CatchException(exception);
  XSetCursorState(display,windows,MagickFalse);
  if (nexus != (Image *) NULL)
    XClientMessage(display,windows->image.id,windows->im_protocols,
      windows->im_next_image,CurrentTime);
  else
    {
      char
        *text,
        **textlist;

      /*
        Unknown image format.
      */
      text=FileToString(filename,~0UL,exception);
      if (text == (char *) NULL)
        return((Image *) NULL);
      textlist=StringToList(text);
      if (textlist != (char **) NULL)
        {
          char
            title[MagickPathExtent];

          (void) FormatLocaleString(title,MagickPathExtent,
            "Unknown format: %s",filename);
          XTextViewWidget(display,resource_info,windows,MagickTrue,title,
            (const char **) textlist);
          XDestroyTextList(textlist);
        }
      text=DestroyString(text);
    }
  exception=DestroyExceptionInfo(exception);
  image_info=DestroyImageInfo(image_info);
  return(nexus);
}

static Cursor XSelectPanCursor(Display *display,XWindows *windows)
{
  Cursor
    cursor;

  if ((windows->image.ximage->width > (int) windows->image.width) &&
      (windows->image.ximage->height > (int) windows->image.height))
    cursor=XCreateFontCursor(display,XC_fleur);
  else
    if (windows->image.ximage->width > (int) windows->image.width)
      cursor=XCreateFontCursor(display,XC_sb_h_double_arrow);
    else
      if (windows->image.ximage->height > (int) windows->image.height)
        cursor=XCreateFontCursor(display,XC_sb_v_double_arrow);
      else
        cursor=XCreateFontCursor(display,XC_arrow);
  return(cursor);
}

static void XClampPanOffset(XWindows *windows,RectangleInfo *pan_info,
  const double x_factor,const double y_factor)
{
  if ((*pan_info).x < (ssize_t) ((*pan_info).width/2))
    (*pan_info).x=0;
  else
    (*pan_info).x=(ssize_t) (x_factor*((*pan_info).x-((int)
      (*pan_info).width/2)));
  if ((*pan_info).x < 0)
    (*pan_info).x=0;
  else
    if ((int) ((*pan_info).x+windows->image.width) >
        windows->image.ximage->width)
      (*pan_info).x=(ssize_t) (windows->image.ximage->width-(int)
        windows->image.width);
  if ((*pan_info).y < (ssize_t) ((*pan_info).height/2))
    (*pan_info).y=0;
  else
    (*pan_info).y=(ssize_t) (y_factor*((*pan_info).y-((int)
      (*pan_info).height/2)));
  if ((*pan_info).y < 0)
    (*pan_info).y=0;
  else
    if ((int) ((*pan_info).y+windows->image.height) >
        windows->image.ximage->height)
      (*pan_info).y=(ssize_t) (windows->image.ximage->height-(int)
        windows->image.height);
}

static void XDisplayPanOffset(Display *display,XWindows *windows,
  const RectangleInfo pan_info)
{
  char
    text[MagickPathExtent];

  if ((windows->image.x != (int) pan_info.x) ||
      (windows->image.y != (int) pan_info.y))
    {
      /*
        Display image pan offset.
      */
      windows->image.x=(int) pan_info.x;
      windows->image.y=(int) pan_info.y;
      (void) FormatLocaleString(text,MagickPathExtent," %ux%u%+d%+d ",
        windows->image.width,windows->image.height,windows->image.x,
        windows->image.y);
      XInfoWidget(display,windows,text);
      /*
        Refresh Image window.
      */
      XDrawPanRectangle(display,windows);
      XRefreshWindow(display,&windows->image,(XEvent *) NULL);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X P a n I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPanImage() pans the image until the mouse button is released.
%
%  The format of the XPanImage method is:
%
%      void XPanImage(Display *display,XWindows *windows,XEvent *event,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XPanImage(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  Cursor
    cursor;

  double
    x_factor,
    y_factor;

  RectangleInfo
    pan_info;

  size_t
    state;

  /*
    Define cursor.
  */
  cursor=XSelectPanCursor(display,windows);
  (void) XCheckDefineCursor(display,windows->pan.id,cursor);
  /*
    Pan image as pointer moves until the mouse button is released.
  */
  x_factor=(double) windows->image.ximage->width/windows->pan.width;
  y_factor=(double) windows->image.ximage->height/windows->pan.height;
  pan_info.width=windows->pan.width*windows->image.width/
    (unsigned int) windows->image.ximage->width;
  pan_info.height=windows->pan.height*windows->image.height/
    (unsigned int) windows->image.ximage->height;
  pan_info.x=0;
  pan_info.y=0;
  state=UpdateConfigurationState;
  do
  {
    switch (event->type)
    {
      case ButtonPress:
      {
        /*
          User choose an initial pan location.
        */
        pan_info.x=(ssize_t) event->xbutton.x;
        pan_info.y=(ssize_t) event->xbutton.y;
        state|=UpdateConfigurationState;
        break;
      }
      case ButtonRelease:
      {
        /*
          User has finished panning the image.
        */
        pan_info.x=(ssize_t) event->xbutton.x;
        pan_info.y=(ssize_t) event->xbutton.y;
        state|=UpdateConfigurationState | ExitState;
        break;
      }
      case MotionNotify:
      {
        pan_info.x=(ssize_t) event->xmotion.x;
        pan_info.y=(ssize_t) event->xmotion.y;
        state|=UpdateConfigurationState;
      }
      default:
        break;
    }
    if ((state & UpdateConfigurationState) != 0)
      {
        /*
          Check boundary conditions.
        */
        XClampPanOffset(windows,&pan_info,x_factor,y_factor);
        XDisplayPanOffset(display,windows,pan_info);
        state&=(unsigned int) (~UpdateConfigurationState);
      }
    /*
      Wait for next event.
    */
    if ((state & ExitState) == 0)
      XScreenEvent(display,windows,event,exception);
  } while ((state & ExitState) == 0);
  /*
    Restore cursor.
  */
  (void) XCheckDefineCursor(display,windows->pan.id,windows->pan.cursor);
  (void) XFreeCursor(display,cursor);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X P a s t e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPasteImage() pastes an image previously saved with XCropImage in the X
%  window image at a location the user chooses with the pointer.
%
%  The format of the XPasteImage method is:
%
%      MagickBooleanType XPasteImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static CompositeOperator XSelectPasteOperator(const ComposeMenuInfo *menu_info,
  const char *const *PasteMenu,CompositeOperator compose)
{
  Display
    *const display = menu_info->display;

  XWindows
    *const windows = menu_info->windows;

  const int
    id = menu_info->id;

  char
    command[MagickPathExtent],
    **operators;

  int
    entry;

  /*
    Select a command from the pop-up menu.
  */
  operators=GetCommandOptions(MagickComposeOptions);
  if (operators == (char **) NULL)
    return(compose);
  entry=XMenuWidget(display,windows,PasteMenu[id],
    (const char **) operators,command);
  if (entry >= 0)
    compose=(CompositeOperator) ParseCommandOption(
      MagickComposeOptions,MagickFalse,operators[entry]);
  operators=DestroyStringList(operators);
  return(compose);
}

typedef struct _PasteCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *PasteMenu;

  const ModeType
    *PasteCommands;

  Image
    *image;

  ExceptionInfo
    *exception;
} PasteCommandInput;

static MagickBooleanType XPasteCommand(const PasteCommandInput *input,
  XEvent *event,CompositeOperator *compose,size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  const char
    *const *const PasteMenu = input->PasteMenu;

  const ModeType
    *const PasteCommands = input->PasteCommands;

  int
    id;

  if ((*event).xany.window == windows->command.id)
    {
      /*
        Select a command from the Command widget.
      */
      id=XCommandWidget(display,windows,PasteMenu,event);
      if (id < 0)
        return(MagickTrue);
      switch (PasteCommands[id])
      {
        case PasteOperatorsCommand:
        {
          ComposeMenuInfo
            menu_info;

          menu_info.display=display;
          menu_info.windows=windows;
          menu_info.id=id;
          (*compose)=XSelectPasteOperator(&menu_info,PasteMenu,(*compose));
          break;
        }
        case PasteHelpCommand:
        {
          XTextViewHelp(display,resource_info,windows,MagickFalse,
            "Help Viewer - Image Composite",ImagePasteHelp);
          break;
        }
        case PasteDismissCommand:
        {
          /*
            Prematurely exit.
          */
          (*state)|=EscapeState;
          (*state)|=ExitState;
          break;
        }
        default:
          break;
      }
      return(MagickTrue);
    }
  return(MagickFalse);
}

typedef struct _PasteFinishInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    *image;

  ExceptionInfo
    *exception;

  Cursor
    cursor;

  size_t
    state;

  Image
    *paste_image;

  RectangleInfo
    paste_info;

  CompositeOperator
    compose;
} PasteFinishInput;

static MagickBooleanType XPasteFinish(const PasteFinishInput *input)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    *const image = input->image;

  ExceptionInfo
    *const exception = input->exception;

  const Cursor
    cursor = input->cursor;

  const size_t
    state = input->state;

  const CompositeOperator
    compose = input->compose;

  Image
    *paste_image = input->paste_image;

  RectangleInfo
    paste_info = input->paste_info;

  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  XSetCursorState(display,windows,MagickFalse);
  (void) XFreeCursor(display,cursor);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  /*
    Image pasting is relative to image configuration.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  paste_info.x+=x;
  paste_info.x=(ssize_t) (scale_factor*paste_info.x+0.5);
  paste_info.width=(unsigned int) (scale_factor*paste_info.width+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  paste_info.y+=y;
  paste_info.y=(ssize_t) (scale_factor*paste_info.y*scale_factor+0.5);
  paste_info.height=(unsigned int) (scale_factor*paste_info.height+0.5);
  /*
    Paste image with X Image window.
  */
  (void) CompositeImage(image,paste_image,compose,MagickTrue,paste_info.x,
    paste_info.y,exception);
  paste_image=DestroyImage(paste_image);
  XSetCursorState(display,windows,MagickFalse);
  /*
    Update image colormap.
  */
  XConfigureImageColormap(display,resource_info,windows,image,exception);
  (void) XConfigureImage(display,resource_info,windows,image,exception);
  return(MagickTrue);
}

typedef struct _PasteTargets
{
  Image
    **paste_image;

  size_t
    *state;

  RectangleInfo
    *paste_info;

  CompositeOperator
    *compose;
} PasteTargets;

static void XPasteKeyPress(const PasteCommandInput *input,XEvent *event,
  const PasteTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    **const paste_image = targets->paste_image;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  int
    length;

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  length=XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  *(command+length)='\0';
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Key press: 0x%lx (%s)",(long) key_symbol,command);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*paste_image)=DestroyImage((*paste_image));
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Composite",ImagePasteHelp);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

static void XPastePress(const PasteCommandInput *input,const XEvent *event,
  const Cursor cursor,const PasteTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  Image
    *const image = input->image;

  Image
    **const paste_image = targets->paste_image;

  RectangleInfo
    *const paste_info = targets->paste_info;

  double
    scale_factor;

  int
    x,
    y;

  unsigned int
    height,
    width;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Press: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  /*
    Paste rectangle is relative to image configuration.
  */
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
      &width,&height);
  scale_factor=(double) windows->image.ximage->width/width;
  (*paste_info).width=(unsigned int) (scale_factor*(*paste_image)->columns+
    0.5);
  scale_factor=(double) windows->image.ximage->height/height;
  (*paste_info).height=(unsigned int) (scale_factor*(*paste_image)->rows+
    0.5);
  (void) XCheckDefineCursor(display,windows->image.id,cursor);
  (*paste_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
  (*paste_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
}

static void XPasteRelease(const PasteCommandInput *input,const XEvent *event,
  const PasteTargets *targets)
{
  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    *const paste_info = targets->paste_info;

  size_t
    *const state = targets->state;

  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Button Release: 0x%lx %u +%d+%d",(*event).xbutton.window,
      (*event).xbutton.button,(*event).xbutton.x,(*event).xbutton.y);
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  if (((*paste_info).width != 0) && ((*paste_info).height != 0))
    {
      /*
        User has selected the location of the paste image.
      */
      (*paste_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
      (*paste_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
      (*state)|=ExitState;
    }
}

static void XPasteEvent(const PasteCommandInput *input,XEvent *event,
  const Cursor cursor,const PasteTargets *targets)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  RectangleInfo
    *const paste_info = targets->paste_info;

  int
    x,
    y;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XPastePress(input,event,cursor,targets);
      break;
    }
    case ButtonRelease:
    {
      XPasteRelease(input,event,targets);
      break;
    }
    case Expose:
      break;
    case KeyPress:
    {
      XPasteKeyPress(input,event,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      x=(*event).xmotion.x;
      y=(*event).xmotion.y;
      XTrackInfoWidget(display,windows,x,y);
      (*paste_info).x=(ssize_t) windows->image.x+x;
      (*paste_info).y=(ssize_t) windows->image.y+y;
      break;
    }
    default:
    {
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"Event type: %d",
          (*event).type);
      break;
    }
  }
}

static void XPasteStep(const PasteCommandInput *input,const Cursor cursor,
  const PasteTargets *targets)
{
  Display
    *const display = input->display;

  ExceptionInfo
    *const exception = input->exception;

  XWindows
    *const windows = input->windows;

  CompositeOperator
    *const compose = targets->compose;

  RectangleInfo
    *const paste_info = targets->paste_info;

  size_t
    *const state = targets->state;

  char
    text[MagickPathExtent];

  RectangleInfo
    highlight_info;

  XEvent
    event;

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %+ld%+ld ",
        (long) (*paste_info).x,(long) (*paste_info).y);
      XInfoWidget(display,windows,text);
    }
  highlight_info=(*paste_info);
  highlight_info.x=(*paste_info).x-windows->image.x;
  highlight_info.y=(*paste_info).y-windows->image.y;
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  if (XPasteCommand(input,&event,compose,state) != MagickFalse)
    return;
  XPasteEvent(input,&event,cursor,targets);
}

static const ModeType
  PasteCommandTable[] =
  {
    PasteOperatorsCommand,
    PasteHelpCommand,
    PasteDismissCommand
  };

static MagickBooleanType XPasteImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  const char
    *const PasteMenu[] =
    {
      "Operator",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static CompositeOperator
    compose = CopyCompositeOp;

  Cursor
    cursor;

  Image
    *paste_image;

  int
    x,
    y;

  RectangleInfo
    paste_info;

  size_t
    state;

  const PasteTargets
    paste_targets = { &paste_image, &state, &paste_info, &compose };

  const PasteCommandInput
    command_input = { display, resource_info, windows, PasteMenu,
      PasteCommandTable, image, exception };

  /*
    Copy image.
  */
  if (resource_info->copy_image == (Image *) NULL)
    return(MagickFalse);
  paste_image=CloneImage(resource_info->copy_image,0,0,MagickTrue,exception);
  if (paste_image == (Image *) NULL)
    return(MagickFalse);
  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Paste");
  windows->command.data=1;
  (void) XCommandWidget(display,windows,PasteMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Track pointer until button 1 is pressed.
  */
  XSetCursorState(display,windows,MagickFalse);
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  paste_info.x=(ssize_t) windows->image.x+x;
  paste_info.y=(ssize_t) windows->image.y+y;
  paste_info.width=0;
  paste_info.height=0;
  cursor=XCreateFontCursor(display,XC_ul_angle);
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  state=DefaultState;
  do
  {
    XPasteStep(&command_input,cursor,&paste_targets);
  } while ((state & ExitState) == 0);
  {
    const PasteFinishInput
      finish = { display, resource_info, windows, image, exception, cursor,
        state, paste_image, paste_info, compose };

    return(XPasteFinish(&finish));
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X P r i n t I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XPrintImage() prints an image to a Postscript printer.
%
%  The format of the XPrintImage method is:
%
%      MagickBooleanType XPrintImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType XPrintImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    geometry[MagickPathExtent];

  const char
    *const PageSizes[] =
    {
      "Letter",
      "Tabloid",
      "Ledger",
      "Legal",
      "Statement",
      "Executive",
      "A3",
      "A4",
      "A5",
      "B4",
      "B5",
      "Folio",
      "Quarto",
      "10x14",
      (char *) NULL
    };

  Image
    *print_image;

  ImageInfo
    *image_info;

  MagickStatusType
    status;

  /*
    Request Postscript page geometry from user.
  */
  image_info=CloneImageInfo(resource_info->image_info);
  (void) FormatLocaleString(geometry,MagickPathExtent,"Letter");
  if (image_info->page != (char *) NULL)
    (void) CopyMagickString(geometry,image_info->page,MagickPathExtent);
  XListBrowserWidget(display,windows,&windows->widget,PageSizes,"Select",
    "Select Postscript Page Geometry:",geometry);
  if (*geometry == '\0')
    return(MagickTrue);
  image_info->page=GetPageGeometry(geometry);
  /*
    Apply image transforms.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  print_image=CloneImage(image,0,0,MagickTrue,exception);
  if (print_image == (Image *) NULL)
    return(MagickFalse);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(&print_image,windows->image.crop_geometry,geometry,
    exception);
  /*
    Print image.
  */
  (void) AcquireUniqueFilename(filename);
  (void) FormatLocaleString(print_image->filename,MagickPathExtent,"print:%s",
    filename);
  status=WriteImage(image_info,print_image,exception);
  (void) RelinquishUniqueFileResource(filename);
  print_image=DestroyImage(print_image);
  image_info=DestroyImageInfo(image_info);
  XSetCursorState(display,windows,MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X R O I I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XROIImage() applies an image processing technique to a region of interest.
%
%  The format of the XROIImage method is:
%
%      MagickBooleanType XROIImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image **image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _ROIMenuEntry
{
  const char
    *const **Menus;

  const DisplayCommand
    **Commands;

  int
    id,
    entry;
} ROIMenuEntry;

static DisplayCommand XSelectROIMenuEntry(const ROIMenuEntry *menu_entry,
  char *command,DisplayCommand display_command)
{
  const char
    *const **const Menus = menu_entry->Menus;

  const DisplayCommand
    **const Commands = menu_entry->Commands;

  const int
    id = menu_entry->id,
    entry = menu_entry->entry;

  if (entry >= 0)
    {
      (void) CopyMagickString(command,Menus[id][entry],
        MagickPathExtent);
      display_command=Commands[id][entry];
    }
  return(display_command);
}

#define ApplyMenus  7

typedef struct _ROITargets
{
  int
    *x,
    *y;

  RectangleInfo
    *roi_info;

  size_t
    *state;

  XEvent
    *event;

  DisplayCommand
    *display_command;

  RectangleInfo
    *crop_info;
} ROITargets;

static void XROIButtonPress(const CropEventInput *event_input,
  const RectangleInfo crop_info,const ROITargets *targets)
{
  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  size_t
    *const state = targets->state;

  (*x)=windows->image.x;
  (*y)=windows->image.y;
  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != windows->image.id)
    return;
  (*x)=windows->image.x+(*event).xbutton.x;
  (*y)=windows->image.y+(*event).xbutton.y;
  if (IsNearTopLeftCorner((*x),(*y),(*roi_info)) != MagickFalse)
    {
      (*roi_info).x=(*roi_info).x+(int) (*roi_info).width;
      (*roi_info).y=(*roi_info).y+(int) (*roi_info).height;
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearBottomLeftCorner((*x),(*y),(*roi_info)) != MagickFalse)
    {
      (*roi_info).x=(*roi_info).x+(int) (*roi_info).width;
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearTopRightCorner((*x),(*y),(*roi_info)) != MagickFalse)
    {
      (*roi_info).y=(*roi_info).y+(int) (*roi_info).height;
      (*state)|=UpdateConfigurationState;
      return;
    }
  if (IsNearBottomRightCorner((*x),(*y),(*roi_info)) != MagickFalse)
    {
      (*state)|=UpdateConfigurationState;
      return;
    }
  XCropButtonRelease(event_input,crop_info);
}

typedef struct _ROIApplyInput
{
  Display
    *display;

  XWindows
    *windows;

  const XEvent
    *event;

  const char
    *const *ApplyMenu;
} ROIApplyInput;

static void XROIButtonRelease(const ROIApplyInput *apply_input,
  const ROITargets *targets)
{
  Display
    *const display = apply_input->display;

  XWindows
    *const windows = apply_input->windows;

  const XEvent
    *const event = apply_input->event;

  const char
    *const *const ApplyMenu = apply_input->ApplyMenu;

  RectangleInfo
    *const roi_info = targets->roi_info;

  size_t
    *const state = targets->state;

  /*
    User has committed to region of interest rectangle.
  */
  (*roi_info).x=(ssize_t) windows->image.x+(*event).xbutton.x;
  (*roi_info).y=(ssize_t) windows->image.y+(*event).xbutton.y;
  XSetCursorState(display,windows,MagickFalse);
  (*state)|=ExitState;
  if (LocaleCompare(windows->command.name,"Apply") == 0)
    return;
  (void) CloneString(&windows->command.name,"Apply");
  windows->command.data=ApplyMenus;
  (void) XCommandWidget(display,windows,ApplyMenu,(XEvent *) NULL);
}

typedef struct _ROIKeyInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const XEvent
    *event;

  Image
    **image;

  ExceptionInfo
    *exception;
} ROIKeyInput;

typedef struct _ROIKeyTargets
{
  RectangleInfo
    *roi_info;

  DisplayCommand
    *display_command;

  size_t
    *state;
} ROIKeyTargets;

static void XROIDownHelpKey(const ROIKeyInput *key_input,
  const ROIKeyTargets *targets,const KeySym key_symbol)
{
  Display
    *const display = key_input->display;

  XWindows
    *const windows = key_input->windows;

  XResourceInfo
    *const resource_info = key_input->resource_info;

  const XEvent
    *const event = key_input->event;

  Image
    **const image = key_input->image;

  ExceptionInfo
    *const exception = key_input->exception;

  RectangleInfo
    *const roi_info = targets->roi_info;

  DisplayCommand
    *const display_command = targets->display_command;

  size_t
    *const state = targets->state;

  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_Down:
    case XK_KP_Down:
    {
      (*roi_info).y++;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Region of Interest",ImageROIHelp);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    default:
    {
      (*display_command)=XImageWindowCommand(display,resource_info,windows,
        (*event).xkey.state,key_symbol,image,exception);
      if ((*display_command) != NullCommand)
        (*state)|=UpdateRegionState;
      break;
    }
  }
}

static void XROIUpRightKey(const ROIKeyInput *key_input,
  const ROIKeyTargets *targets,const KeySym key_symbol)
{
  RectangleInfo
    *const roi_info = targets->roi_info;

  switch ((int) key_symbol)
  {
    case XK_Up:
    case XK_KP_Up:
    case XK_Next:
    {
      (*roi_info).y--;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      (*roi_info).x++;
      break;
    }
    default:
    {
      XROIDownHelpKey(key_input,targets,key_symbol);
      break;
    }
  }
}

static void XROIHomeLeftKey(const ROIKeyInput *key_input,
  const ROIKeyTargets *targets,const KeySym key_symbol)
{
  XWindows
    *const windows = key_input->windows;

  RectangleInfo
    *const roi_info = targets->roi_info;

  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      (*roi_info).x=(ssize_t) (windows->image.width/2L-(*roi_info).width/2L);
      (*roi_info).y=(ssize_t) (windows->image.height/2L-
        (*roi_info).height/2L);
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      (*roi_info).x--;
      break;
    }
    default:
    {
      XROIUpRightKey(key_input,targets,key_symbol);
      break;
    }
  }
}

typedef struct _ROIRegionInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  ExceptionInfo
    *exception;

  RectangleInfo
    roi_info;

  DisplayCommand
    display_command;

  const char
    *text;
} ROIRegionInput;

static MagickBooleanType XROIApplyRegion(const ROIRegionInput *region_input,
  RectangleInfo *crop_info)
{
  Display
    *const display = region_input->display;

  XResourceInfo
    *const resource_info = region_input->resource_info;

  XWindows
    *const windows = region_input->windows;

  Image
    **const image = region_input->image;

  ExceptionInfo
    *const exception = region_input->exception;

  const RectangleInfo
    roi_info = region_input->roi_info;

  const DisplayCommand
    display_command = region_input->display_command;

  double
    scale_factor;

  Image
    *roi_image;

  int
    x,
    y;

  MagickProgressMonitor
    progress_monitor;

  unsigned int
    height,
    width;

  /*
    Region of interest is relative to image configuration.
  */
  progress_monitor=SetImageProgressMonitor(*image,
    (MagickProgressMonitor) NULL,(*image)->client_data);
  (*crop_info)=roi_info;
  width=(unsigned int) (*image)->columns;
  height=(unsigned int) (*image)->rows;
  x=0;
  y=0;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
      &width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  (*crop_info).x+=x;
  (*crop_info).x=(ssize_t) (scale_factor*(*crop_info).x+0.5);
  (*crop_info).width=(unsigned int) (scale_factor*(*crop_info).width+0.5);
  scale_factor=(double)
    height/windows->image.ximage->height;
  (*crop_info).y+=y;
  (*crop_info).y=(ssize_t) (scale_factor*(*crop_info).y+0.5);
  (*crop_info).height=(unsigned int)
    (scale_factor*(*crop_info).height+0.5);
  roi_image=CropImage(*image,crop_info,exception);
  (void) SetImageProgressMonitor(*image,progress_monitor,
    (*image)->client_data);
  if (roi_image == (Image *) NULL)
    return(MagickTrue);
  /*
    Apply image processing technique to the region of interest.
  */
  windows->image.orphan=MagickTrue;
  (void) XMagickCommand(display,resource_info,windows,
    display_command,&roi_image,exception);
  progress_monitor=SetImageProgressMonitor(*image,
    (MagickProgressMonitor) NULL,(*image)->client_data);
  (void) XMagickCommand(display,resource_info,windows,
    SaveToUndoBufferCommand,image,exception);
  windows->image.orphan=MagickFalse;
  (void) CompositeImage(*image,roi_image,CopyCompositeOp,
    MagickTrue,(*crop_info).x,(*crop_info).y,exception);
  roi_image=DestroyImage(roi_image);
  (void) SetImageProgressMonitor(*image,progress_monitor,
    (*image)->client_data);
  return(MagickFalse);
}

static MagickBooleanType XROIUpdateRegion(const ROIRegionInput *region_input,
  size_t *state,RectangleInfo *crop_info)
{
  Display
    *const display = region_input->display;

  XResourceInfo
    *const resource_info = region_input->resource_info;

  XWindows
    *const windows = region_input->windows;

  Image
    **const image = region_input->image;

  ExceptionInfo
    *const exception = region_input->exception;

  const DisplayCommand
    display_command = region_input->display_command;

  const char
    *const text = region_input->text;

  if (((*state) & UpdateRegionState) != 0)
    {
      (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
      switch (display_command)
      {
        case UndoCommand:
        case RedoCommand:
        {
          (void) XMagickCommand(display,resource_info,windows,
            display_command,image,exception);
          break;
        }
        default:
        {
          if (XROIApplyRegion(region_input,crop_info) != MagickFalse)
            return(MagickTrue);
          break;
        }
      }
      if (display_command != InfoCommand)
        {
          XConfigureImageColormap(display,resource_info,windows,*image,
            exception);
          (void) XConfigureImage(display,resource_info,windows,*image,
            exception);
        }
      XCheckRefreshWindows(display,windows);
      XInfoWidget(display,windows,text);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      (*state)&=(unsigned int) (~UpdateRegionState);
    }
  return(MagickFalse);
}

static void XROIMotion(const CropEventInput *event_input,
  const ROITargets *targets)
{
  Display
    *const display = event_input->display;

  XWindows
    *const windows = event_input->windows;

  const XEvent
    *const event = event_input->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  if ((*event).xbutton.window != windows->image.id)
    return;
  /*
    Map and unmap Info widget as text cursor crosses its boundaries.
  */
  (*x)=(*event).xmotion.x;
  (*y)=(*event).xmotion.y;
  XTrackInfoWidget(display,windows,(*x),(*y));
  (*roi_info).x=(ssize_t) windows->image.x+(*event).xmotion.x;
  (*roi_info).y=(ssize_t) windows->image.y+(*event).xmotion.y;
}

static void XROIKeyPress(const ROIKeyInput *key_input,XEvent *event,
  const ROIKeyTargets *targets)
{
  Display
    *const display = key_input->display;

  XWindows
    *const windows = key_input->windows;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  if ((*event).xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Shift_L:
    case XK_Shift_R:
      break;
    case XK_Escape:
    case XK_F20:
    {
      (*state)|=EscapeState;
      magick_fallthrough;
    }
    case XK_Return:
    {
      (*state)|=ExitState;
      break;
    }
    default:
    {
      XROIHomeLeftKey(key_input,targets,key_symbol);
      break;
    }
  }
  (void) XSetSelectionOwner(display,XA_PRIMARY,windows->image.id,
    (*event).xkey.time);
}

typedef struct _ROICommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  RectangleInfo
    highlight_info;

  const char
    *const *ApplyMenu;

  const DisplayCommand
    *ApplyCommands;

  const char
    *const **Menus;

  const DisplayCommand
    **Commands;
} ROICommandInput;

static void XROICommand(const ROICommandInput *command_input,XEvent *event,
  DisplayCommand *display_command,size_t *state)
{
  Display
    *const display = command_input->display;

  XResourceInfo
    *const resource_info = command_input->resource_info;

  XWindows
    *const windows = command_input->windows;

  const RectangleInfo
    highlight_info = command_input->highlight_info;

  const char
    *const *const ApplyMenu = command_input->ApplyMenu;

  const DisplayCommand
    *const ApplyCommands = command_input->ApplyCommands;

  const char
    *const **const Menus = command_input->Menus;

  const DisplayCommand
    **const Commands = command_input->Commands;

  char
    command[MagickPathExtent];

  int
    entry,
    id;

  /*
    Select a command from the Command widget.
  */
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  (*display_command)=NullCommand;
  id=XCommandWidget(display,windows,ApplyMenu,event);
  if (id >= 0)
    {
      (void) CopyMagickString(command,ApplyMenu[id],MagickPathExtent);
      (*display_command)=ApplyCommands[id];
      if (id < ApplyMenus)
        {
          ROIMenuEntry
            menu_entry;

          /*
            Select a command from a pop-up menu.
          */
          entry=XMenuWidget(display,windows,ApplyMenu[id],
            (const char **) Menus[id],command);
          menu_entry.Menus=Menus;
          menu_entry.Commands=Commands;
          menu_entry.id=id;
          menu_entry.entry=entry;
          (*display_command)=XSelectROIMenuEntry(&menu_entry,command,
            (*display_command));
        }
    }
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
  XHighlightRectangle(display,windows->image.id,
    windows->image.highlight_context,&highlight_info);
  if ((*display_command) == HelpCommand)
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Region of Interest",ImageROIHelp);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      return;
    }
  if ((*display_command) == QuitCommand)
    {
      /*
        exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      return;
    }
  if ((*display_command) != NullCommand)
    (*state)|=UpdateRegionState;
}

static const char
  *const ROIMenu[] =
  {
    "Help",
    "Dismiss",
    (char *) NULL
  },
  *const ApplyMenu[] =
  {
    "File",
    "Edit",
    "Transform",
    "Enhance",
    "Effects",
    "F/X",
    "Miscellany",
    "Help",
    "Dismiss",
    (char *) NULL
  },
  *const ROIFileMenu[] =
  {
    "Save...",
    "Print...",
    (char *) NULL
  },
  *const ROIEditMenu[] =
  {
    "Undo",
    "Redo",
    (char *) NULL
  },
  *const ROITransformMenu[] =
  {
    "Flop",
    "Flip",
    "Rotate Right",
    "Rotate Left",
    (char *) NULL
  },
  *const ROIEnhanceMenu[] =
  {
    "Hue...",
    "Saturation...",
    "Brightness...",
    "Gamma...",
    "Spiff",
    "Dull",
    "Contrast Stretch...",
    "Sigmoidal Contrast...",
    "Normalize",
    "Equalize",
    "Negate",
    "Grayscale",
    "Map...",
    "Quantize...",
    (char *) NULL
  },
  *const ROIEffectsMenu[] =
  {
    "Despeckle",
    "Emboss",
    "Reduce Noise",
    "Add Noise",
    "Sharpen...",
    "Blur...",
    "Threshold...",
    "Edge Detect...",
    "Spread...",
    "Shade...",
    "Raise...",
    "Segment...",
    (char *) NULL
  },
  *const ROIFXMenu[] =
  {
    "Solarize...",
    "Sepia Tone...",
    "Swirl...",
    "Implode...",
    "Vignette...",
    "Wave...",
    "Oil Paint...",
    "Charcoal Draw...",
    (char *) NULL
  },
  *const ROIMiscellanyMenu[] =
  {
    "Image Info",
    "Zoom Image",
    "Show Preview...",
    "Show Histogram",
    "Show Matte",
    (char *) NULL
  };

static const char
  *const *ROIMenus[ApplyMenus] =
  {
    ROIFileMenu,
    ROIEditMenu,
    ROITransformMenu,
    ROIEnhanceMenu,
    ROIEffectsMenu,
    ROIFXMenu,
    ROIMiscellanyMenu
  };

static const DisplayCommand
  ApplyCommands[] =
  {
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    HelpCommand,
    QuitCommand
  },
  ROIFileCommands[] =
  {
    SaveCommand,
    PrintCommand
  },
  ROIEditCommands[] =
  {
    UndoCommand,
    RedoCommand
  },
  ROITransformCommands[] =
  {
    FlopCommand,
    FlipCommand,
    RotateRightCommand,
    RotateLeftCommand
  },
  ROIEnhanceCommands[] =
  {
    HueCommand,
    SaturationCommand,
    BrightnessCommand,
    GammaCommand,
    SpiffCommand,
    DullCommand,
    ContrastStretchCommand,
    SigmoidalContrastCommand,
    NormalizeCommand,
    EqualizeCommand,
    NegateCommand,
    GrayscaleCommand,
    MapCommand,
    QuantizeCommand
  },
  ROIEffectsCommands[] =
  {
    DespeckleCommand,
    EmbossCommand,
    ReduceNoiseCommand,
    AddNoiseCommand,
    SharpenCommand,
    BlurCommand,
    ThresholdCommand,
    EdgeDetectCommand,
    SpreadCommand,
    ShadeCommand,
    RaiseCommand,
    SegmentCommand
  },
  ROIFXCommands[] =
  {
    SolarizeCommand,
    SepiaToneCommand,
    SwirlCommand,
    ImplodeCommand,
    VignetteCommand,
    WaveCommand,
    OilPaintCommand,
    CharcoalDrawCommand
  },
  ROIMiscellanyCommands[] =
  {
    InfoCommand,
    ZoomCommand,
    ShowPreviewCommand,
    ShowHistogramCommand,
    ShowMatteCommand
  },
  ROICommands[] =
  {
    ROIHelpCommand,
    ROIDismissCommand
  };

static const DisplayCommand
  *ROIMenuCommands[ApplyMenus] =
  {
    ROIFileCommands,
    ROIEditCommands,
    ROITransformCommands,
    ROIEnhanceCommands,
    ROIEffectsCommands,
    ROIFXCommands,
    ROIMiscellanyCommands
  };

typedef struct _ROIContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Cursor
    cursor;

  ExceptionInfo
    *exception;

  char
    *text;

  Image
    **image;

  ROIKeyTargets
    key_targets;
} ROIContext;

static void XROIPointerCommand(const ROIContext *context,
  const ROITargets *targets)
{
  int
    id;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(context->display,context->windows,ROIMenu,event);
  if (id < 0)
    return;
  switch (ROICommands[id])
  {
    case ROIHelpCommand:
    {
      XTextViewHelp(context->display,context->resource_info,context->windows,
        MagickFalse,"Help Viewer - Region of Interest",ImageROIHelp);
      break;
    }
    case ROIDismissCommand:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    default:
      break;
  }
}

static void XROIPointerPress(const ROIContext *context,
  const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const roi_info = targets->roi_info;

  if ((*event).xbutton.button != Button1)
    return;
  if ((*event).xbutton.window != context->windows->image.id)
    return;
  /*
    Note first corner of region of interest rectangle-- exit loop.
  */
  (void) XCheckDefineCursor(context->display,context->windows->image.id,
    context->cursor);
  (*roi_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
  (*roi_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
  (*state)|=ExitState;
}

static void XROIPointerKey(const ROIContext *context,
  const ROITargets *targets)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  if ((*event).xkey.window != context->windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      XTextViewHelp(context->display,context->resource_info,context->windows,
        MagickFalse,"Help Viewer - Region of Interest",ImageROIHelp);
      break;
    }
    default:
    {
      (void) XBell(context->display,0);
      break;
    }
  }
}

static void XROIPointerEvent(const ROIContext *context,
  const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      XROIPointerPress(context,targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      XROIPointerKey(context,targets);
      break;
    }
    case MotionNotify:
    {
      /*
        Map and unmap Info widget as text cursor crosses its boundaries.
      */
      (*x)=(*event).xmotion.x;
      (*y)=(*event).xmotion.y;
      XTrackInfoWidget(context->display,context->windows,(*x),(*y));
      (*roi_info).x=(ssize_t) context->windows->image.x+(*x);
      (*roi_info).y=(ssize_t) context->windows->image.y+(*y);
      break;
    }
    default:
      break;
  }
}

static void XROITrackPointer(const ROIContext *context,
  const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const roi_info = targets->roi_info;

  do
  {
    if (context->windows->info.mapped != MagickFalse)
      {
        /*
          Display pointer position.
        */
        (void) FormatLocaleString(context->text,MagickPathExtent," %+ld%+ld ",
          (long) (*roi_info).x,(long) (*roi_info).y);
        XInfoWidget(context->display,context->windows,context->text);
      }
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XROIPointerCommand(context,targets);
        continue;
      }
    XROIPointerEvent(context,targets);
  } while (((*state) & ExitState) == 0);
}

static void XROISizeEvent(const ROIContext *context,
  const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  RectangleInfo
    *const roi_info = targets->roi_info;

  switch ((*event).type)
  {
    case ButtonPress:
    {
      (*roi_info).x=(ssize_t) context->windows->image.x+(*event).xbutton.x;
      (*roi_info).y=(ssize_t) context->windows->image.y+(*event).xbutton.y;
      break;
    }
    case ButtonRelease:
    {
      ROIApplyInput
        apply_input;

      apply_input.display=context->display;
      apply_input.windows=context->windows;
      apply_input.event=(event);
      apply_input.ApplyMenu=ApplyMenu;
      XROIButtonRelease(&apply_input,targets);
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      (*roi_info).x=(ssize_t) context->windows->image.x+(*event).xmotion.x;
      (*roi_info).y=(ssize_t) context->windows->image.y+(*event).xmotion.y;
    }
    default:
      break;
  }
}

static void XROIDrawRegion(const ROIContext *context,const ROITargets *targets,
  RectangleInfo *highlight_info)
{
  RectangleInfo
    *const roi_info = targets->roi_info;

  (*highlight_info)=(*roi_info);
  (*highlight_info).x=(*roi_info).x-context->windows->image.x;
  (*highlight_info).y=(*roi_info).y-context->windows->image.y;
  if (((*highlight_info).width > 3) && ((*highlight_info).height > 3))
    {
      /*
        Display info and draw region of interest rectangle.
      */
      XMapInfoWidget(context->display,context->windows);
      (void) FormatLocaleString(context->text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) (*roi_info).width,(double)
        (*roi_info).height,(double) (*roi_info).x,(double) (*roi_info).y);
      XInfoWidget(context->display,context->windows,context->text);
      XHighlightRectangle(context->display,context->windows->image.id,
        context->windows->image.highlight_context,highlight_info);
    }
  else
    if (context->windows->info.mapped != MagickFalse)
      (void) XWithdrawWindow(context->display,context->windows->info.id,
        context->windows->info.screen);
}

static void XROISizeRegion(const ROIContext *context,const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  RectangleInfo
    highlight_info;

  do
  {
    XROIDrawRegion(context,targets,&highlight_info);
    /*
      Wait for next event.
    */
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((highlight_info.width > 3) && (highlight_info.height > 3))
      XHighlightRectangle(context->display,context->windows->image.id,
        context->windows->image.highlight_context,&highlight_info);
    XROISizeEvent(context,targets);
    if (IsSelectionMovedOrDone((*roi_info),(*x),(*y),(*state)) != MagickFalse)
      XCheckSelectionBounds(context->windows,(*x),(*y),roi_info);
  } while (((*state) & ExitState) == 0);
}

static void XROIWaitCommand(const ROIContext *context,const ROITargets *targets,
  const RectangleInfo highlight_info)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  DisplayCommand
    *const display_command = targets->display_command;

  ROICommandInput
    command_input;

  command_input.display=context->display;
  command_input.resource_info=context->resource_info;
  command_input.windows=context->windows;
  command_input.highlight_info=highlight_info;
  command_input.ApplyMenu=ApplyMenu;
  command_input.ApplyCommands=ApplyCommands;
  command_input.Menus=ROIMenus;
  command_input.Commands=ROIMenuCommands;
  XROICommand(&command_input,event,display_command,state);
}

static void XROIWaitEvent(const ROIContext *context,const ROITargets *targets,
  const RectangleInfo highlight_info)
{
  CropEventInput
    event_input;

  ROIKeyInput
    key_input;

  XEvent
    *const event = targets->event;

  RectangleInfo
    *const roi_info = targets->roi_info;

  RectangleInfo
    *const crop_info = targets->crop_info;

  event_input.display=context->display;
  event_input.windows=context->windows;
  event_input.event=(event);
  event_input.highlight_info=highlight_info;
  event_input.text=context->text;
  key_input.display=context->display;
  key_input.resource_info=context->resource_info;
  key_input.windows=context->windows;
  key_input.event=(event);
  key_input.image=context->image;
  key_input.exception=context->exception;
  switch ((*event).type)
  {
    case ButtonPress:
    {
      XROIButtonPress(&event_input,(*crop_info),targets);
      break;
    }
    case ButtonRelease:
    {
      XCropButtonRelease(&event_input,(*crop_info));
      break;
    }
    case Expose:
    {
      XCropExpose(&event_input,event);
      break;
    }
    case KeyPress:
    {
      XROIKeyPress(&key_input,event,&context->key_targets);
      break;
    }
    case MotionNotify:
    {
      XROIMotion(&event_input,targets);
      break;
    }
    case SelectionRequest:
    {
      XCropSelectionRequest(event,(*roi_info),context->text);
    }
    default:
      break;
  }
}

static MagickBooleanType XROIWaitHead(const ROIContext *context,
  const ROITargets *targets,RectangleInfo *highlight_info)
{
  size_t
    *const state = targets->state;

  RectangleInfo
    *const roi_info = targets->roi_info;

  if (context->windows->info.mapped != MagickFalse)
    {
      /*
        Display pointer position.
      */
      (void) FormatLocaleString(context->text,MagickPathExtent,
        " %.17gx%.17g%+.20g%+.20g",(double) (*roi_info).width,(double)
        (*roi_info).height,(double) (*roi_info).x,(double) (*roi_info).y);
      XInfoWidget(context->display,context->windows,context->text);
    }
  (*highlight_info)=(*roi_info);
  (*highlight_info).x=(*roi_info).x-context->windows->image.x;
  (*highlight_info).y=(*roi_info).y-context->windows->image.y;
  if (((*highlight_info).width <= 3) || ((*highlight_info).height <= 3))
    {
      (*state)|=EscapeState;
      (*state)|=ExitState;
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XROIWaitCorner(const ROIContext *context,const ROITargets *targets)
{
  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  RectangleInfo
    *const roi_info = targets->roi_info;

  RectangleInfo
    *const crop_info = targets->crop_info;

  DisplayCommand
    *const display_command = targets->display_command;

  RectangleInfo
    highlight_info;

  do
  {
    ROIRegionInput
      region_input;

    if (XROIWaitHead(context,targets,&highlight_info) == MagickFalse)
      break;
    region_input.display=context->display;
    region_input.resource_info=context->resource_info;
    region_input.windows=context->windows;
    region_input.image=context->image;
    region_input.exception=context->exception;
    region_input.roi_info=(*roi_info);
    region_input.display_command=(*display_command);
    region_input.text=context->text;
    if (XROIUpdateRegion(&region_input,state,crop_info) != MagickFalse)
      continue;
    XHighlightRectangle(context->display,context->windows->image.id,
      context->windows->image.highlight_context,&highlight_info);
    XScreenEvent(context->display,context->windows,event,context->exception);
    if ((*event).xany.window == context->windows->command.id)
      {
        XROIWaitCommand(context,targets,highlight_info);
        continue;
      }
    XHighlightRectangle(context->display,context->windows->image.id,
      context->windows->image.highlight_context,&highlight_info);
    XROIWaitEvent(context,targets,highlight_info);
    if (((*state) & UpdateConfigurationState) != 0)
      {
        (void) XPutBackEvent(context->display,event);
        (void) XCheckDefineCursor(context->display,context->windows->image.id,
          context->cursor);
        break;
      }
  } while (((*state) & ExitState) == 0);
}

static void XROISelectRegion(const ROIContext *context,const ROITargets *targets)
{
  size_t
    *const state = targets->state;

  int
    *const x = targets->x,
    *const y = targets->y;

  RectangleInfo
    *const roi_info = targets->roi_info;

  RectangleInfo
    *const crop_info = targets->crop_info;

  DisplayCommand
    *const display_command = targets->display_command;

  do
  {
    /*
      Size rectangle as pointer moves until the mouse button is released.
    */
    (*x)=(int) (*roi_info).x;
    (*y)=(int) (*roi_info).y;
    (*roi_info).width=0;
    (*roi_info).height=0;
    (*state)=DefaultState;
    XROISizeRegion(context,targets);
    /*
      Wait for user to grab a corner of the rectangle or press return.
    */
    (*state)=DefaultState;
    (*display_command)=NullCommand;
    (*crop_info).x=0;
    (*crop_info).y=0;
    (void) XMapWindow(context->display,context->windows->info.id);
    XROIWaitCorner(context,targets);
  } while (((*state) & ExitState) == 0);
}

static MagickBooleanType XROIImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image **image,
  ExceptionInfo *exception)
{
  char
    text[MagickPathExtent];

  DisplayCommand
    display_command;

  Cursor
    cursor;

  int
    x,
    y;

  RectangleInfo
    crop_info,
    roi_info;

  size_t
    state;

  XEvent
    event;

  ROIContext
    context;

  const ROITargets
    targets = { &x, &y, &roi_info, &state, &event, &display_command,
      &crop_info };

  const ROIKeyTargets
    key_targets = { &roi_info, &display_command, &state };

  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"ROI");
  windows->command.data=0;
  (void) XCommandWidget(display,windows,ROIMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_update_widget,CurrentTime);
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,&x,&y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  roi_info.x=(ssize_t) windows->image.x+x;
  roi_info.y=(ssize_t) windows->image.y+y;
  roi_info.width=0;
  roi_info.height=0;
  cursor=XCreateFontCursor(display,XC_fleur);
  state=DefaultState;
  context.display=display;
  context.resource_info=resource_info;
  context.windows=windows;
  context.cursor=cursor;
  context.exception=exception;
  context.text=text;
  context.image=image;
  context.key_targets=key_targets;
  XROITrackPointer(&context,&targets);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  if ((state & EscapeState) != 0)
    {
      /*
        User want to exit without region of interest.
      */
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      (void) XFreeCursor(display,cursor);
      return(MagickTrue);
    }
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  XROISelectRegion(&context,&targets);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  XSetCursorState(display,windows,MagickFalse);
  if ((state & EscapeState) != 0)
    return(MagickTrue);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X R o t a t e I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XRotateImage() rotates the X image.  If the degrees parameter if zero, the
%  rotation angle is computed from the slope of a line drawn by the user.
%
%  The format of the XRotateImage method is:
%
%      MagickBooleanType XRotateImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,double degrees,
%        Image **image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o degrees: Specifies the number of degrees to rotate the image.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
typedef struct _RotatePenMenu
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *RotateMenu;

  int
    id;

  char
    *command;
} RotatePenMenu;

typedef struct _RotateTargets
{
  unsigned int
    *pen_id;
} RotateTargets;

static void XSelectRotatePenColor(const RotatePenMenu *menu,
  const RotateTargets *targets)
{
  Display
    *const display = menu->display;

  XResourceInfo
    *const resource_info = menu->resource_info;

  XWindows
    *const windows = menu->windows;

  const char
    *const *const RotateMenu = menu->RotateMenu;

  const int
    id = menu->id;

  char
    *const command = menu->command;

  unsigned int
    *const pen_id = targets->pen_id;

  const char
    *ColorMenu[MaxNumberPens];

  int
    i,
    pen_number;

  XColor
    color;

  /*
    Initialize menu selections.
  */
  for (i=0; i < (int) (MaxNumberPens-2); i++)
    ColorMenu[i]=resource_info->pen_colors[i];
  ColorMenu[MaxNumberPens-2]="Browser...";
  ColorMenu[MaxNumberPens-1]=(const char *) NULL;
  /*
    Select a pen color from the pop-up menu.
  */
  pen_number=XMenuWidget(display,windows,RotateMenu[id],
    (const char **) ColorMenu,command);
  if (pen_number < 0)
    return;
  if (pen_number == (MaxNumberPens-2))
    {
      static char
        color_name[MagickPathExtent] = "gray";

      /*
        Select a pen color from a dialog.
      */
      resource_info->pen_colors[pen_number]=color_name;
      XColorBrowserWidget(display,windows,"Select",color_name);
      if (*color_name == '\0')
        return;
    }
  /*
    Set pen color.
  */
  (void) XParseColor(display,windows->map_info->colormap,
    resource_info->pen_colors[pen_number],&color);
  XBestPixel(display,windows->map_info->colormap,(XColor *) NULL,
    (unsigned int) MaxColors,&color);
  windows->pixel_info->pen_colors[pen_number]=color;
  (*pen_id)=(unsigned int) pen_number;
}

typedef struct _RotateCommandInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XEvent
    event;

  const char
    *const *RotateMenu;

  const ModeType
    *RotateCommands,
    *DirectionCommands;

  char
    *command;

  RotateTargets
    targets;
} RotateCommandInput;

static void XRotateCommand(const RotateCommandInput *input,
  ModeType *direction,size_t *state)
{
  Display
    *const display = input->display;

  XResourceInfo
    *const resource_info = input->resource_info;

  XWindows
    *const windows = input->windows;

  XEvent
    event = input->event;

  const char
    *const *const RotateMenu = input->RotateMenu;

  int
    id;

  /*
    Select a command from the Command widget.
  */
  id=XCommandWidget(display,windows,RotateMenu,&event);
  if (id < 0)
    return;
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  switch (input->RotateCommands[id])
  {
    case RotateColorCommand:
    {
      RotatePenMenu
        menu;

      menu.display=display;
      menu.resource_info=resource_info;
      menu.windows=windows;
      menu.RotateMenu=RotateMenu;
      menu.id=id;
      menu.command=input->command;
      XSelectRotatePenColor(&menu,&input->targets);
      break;
    }
    case RotateDirectionCommand:
    {
      const char
        *Directions[] =
        {
          "horizontal",
          "vertical",
          (char *) NULL,
        };

      /*
        Select a command from the pop-up menu.
      */
      id=XMenuWidget(display,windows,RotateMenu[id],
        Directions,input->command);
      *direction=XSelectChopDirection(id,input->DirectionCommands,*direction);
      break;
    }
    case RotateHelpCommand:
    {
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Rotation",ImageRotateHelp);
      break;
    }
    case RotateDismissCommand:
    {
      /*
        Prematurely exit.
      */
      *state|=EscapeState;
      *state|=ExitState;
      break;
    }
    default:
      break;
  }
  (void) XSetFunction(display,windows->image.highlight_context,
    GXinvert);
}

typedef struct _RotateLineTargets
{
  XSegment
    *rotate_info;

  size_t
    *state;

  unsigned int
    *distance;

  double
    *degrees;
} RotateLineTargets;

static void XStartRotateLine(Display *display,XWindows *windows,
  const XEvent event,const RotateLineTargets *targets)
{
  XSegment
    *const rotate_info = targets->rotate_info;

  size_t
    *const state = targets->state;

  if (event.xbutton.button != Button1)
    return;
  if (event.xbutton.window != windows->image.id)
    return;
  /*
    exit loop.
  */
  (void) XSetFunction(display,windows->image.highlight_context,
    GXcopy);
  (*rotate_info).x1=event.xbutton.x;
  (*rotate_info).y1=event.xbutton.y;
  (*state)|=ExitState;
}

typedef struct _RotateKeyInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  XEvent
    event;
} RotateKeyInput;

static void XRotateKeyPress(const RotateKeyInput *key_input,
  const RotateLineTargets *targets)
{
  Display
    *const display = key_input->display;

  XResourceInfo
    *const resource_info = key_input->resource_info;

  XWindows
    *const windows = key_input->windows;

  XEvent
    event = key_input->event;

  size_t
    *const state = targets->state;

  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  if (event.xkey.window != windows->image.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &event.xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  switch ((int) key_symbol)
  {
    case XK_Escape:
    case XK_F20:
    {
      /*
        Prematurely exit.
      */
      (*state)|=EscapeState;
      (*state)|=ExitState;
      break;
    }
    case XK_F1:
    case XK_Help:
    {
      (void) XSetFunction(display,windows->image.highlight_context,
        GXcopy);
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Rotation",ImageRotateHelp);
      (void) XSetFunction(display,windows->image.highlight_context,
        GXinvert);
      break;
    }
    default:
    {
      (void) XBell(display,0);
      break;
    }
  }
}

typedef struct _RotateInfoInput
{
  Display
    *display;

  XWindows
    *windows;

  char
    *text;

  ModeType
    direction;

  double
    degrees;

  XSegment
    rotate_info;
} RotateInfoInput;

static void XShowRotateInfo(const RotateInfoInput *show)
{
  Display
    *const display = show->display;

  XWindows
    *const windows = show->windows;

  char
    *const text = show->text;

  const ModeType
    direction = show->direction;

  const double
    degrees = show->degrees;

  const XSegment
    rotate_info = show->rotate_info;

  /*
    Display info and draw rotation line.
  */
  if (windows->info.mapped == MagickFalse)
    (void) XMapWindow(display,windows->info.id);
  (void) FormatLocaleString(text,MagickPathExtent," %g",
    direction == VerticalRotateCommand ? degrees-90.0 : degrees);
  XInfoWidget(display,windows,text);
  XHighlightLine(display,windows->image.id,
    windows->image.highlight_context,&rotate_info);
}

typedef struct _RotateLoopInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  const char
    *const *RotateMenu;

  const ModeType
    *RotateCommands,
    *DirectionCommands;

  char
    *command;

  RotateTargets
    targets;

  ExceptionInfo
    *exception;
} RotateLoopInput;

static MagickBooleanType XRotateLoopCommand(
  const RotateLoopInput *loop_input,const XEvent event,ModeType *direction,
  size_t *state)
{
  Display
    *const display = loop_input->display;

  XResourceInfo
    *const resource_info = loop_input->resource_info;

  XWindows
    *const windows = loop_input->windows;

  const char
    *const *const RotateMenu = loop_input->RotateMenu;

  const ModeType
    *const RotateCommands = loop_input->RotateCommands,
    *const DirectionCommands = loop_input->DirectionCommands;

  char
    *const command = loop_input->command;

  const RotateTargets
    targets = loop_input->targets;

  if (event.xany.window == windows->command.id)
    {
      RotateCommandInput
        input;

      input.display=display;
      input.resource_info=resource_info;
      input.windows=windows;
      input.event=event;
      input.RotateMenu=RotateMenu;
      input.RotateCommands=RotateCommands;
      input.DirectionCommands=DirectionCommands;
      input.command=command;
      input.targets=targets;
      XRotateCommand(&input,direction,state);
      return(MagickTrue);
    }
  return(MagickFalse);
}

static void XRotateSelectStep(const RotateLoopInput *loop_input,
  const RotateLineTargets *line_targets,ModeType *direction)
{
  Display
    *const display = loop_input->display;

  XResourceInfo
    *const resource_info = loop_input->resource_info;

  XWindows
    *const windows = loop_input->windows;

  ExceptionInfo
    *const exception = loop_input->exception;

  XSegment
    *const rotate_info = line_targets->rotate_info;

  size_t
    *const state = line_targets->state;

  XEvent
    event;

  XHighlightLine(display,windows->image.id,
    windows->image.highlight_context,rotate_info);
  /*
    Wait for next event.
  */
  XScreenEvent(display,windows,&event,exception);
  XHighlightLine(display,windows->image.id,
    windows->image.highlight_context,rotate_info);
  if (XRotateLoopCommand(loop_input,event,direction,state) !=
      MagickFalse)
    return;
  switch (event.type)
  {
    case ButtonPress:
    {
      XStartRotateLine(display,windows,event,line_targets);
      break;
    }
    case ButtonRelease:
      break;
    case Expose:
      break;
    case KeyPress:
    {
      RotateKeyInput
        key_input;

      key_input.display=display;
      key_input.resource_info=resource_info;
      key_input.windows=windows;
      key_input.event=event;
      XRotateKeyPress(&key_input,line_targets);
      break;
    }
    case MotionNotify:
    {
      (*rotate_info).x1=event.xmotion.x;
      (*rotate_info).y1=event.xmotion.y;
    }
  }
  (*rotate_info).x2=(*rotate_info).x1;
  (*rotate_info).y2=(*rotate_info).y1;
  if ((*direction) == HorizontalRotateCommand)
    (*rotate_info).x2+=32;
  else
    (*rotate_info).y2-=32;
}

static void XRotateDragEvent(const XEvent event,
  const RotateLineTargets *targets)
{
  XSegment
    *const rotate_info = targets->rotate_info;

  size_t
    *const state = targets->state;

  switch (event.type)
  {
    case ButtonPress:
      break;
    case ButtonRelease:
    {
      /*
        User has committed to rotation line.
      */
      (*rotate_info).x2=event.xbutton.x;
      (*rotate_info).y2=event.xbutton.y;
      (*state)|=ExitState;
      break;
    }
    case Expose:
      break;
    case MotionNotify:
    {
      (*rotate_info).x2=event.xmotion.x;
      (*rotate_info).y2=event.xmotion.y;
    }
    default:
      break;
  }
}

static void XClampRotateSegment(const XWindows *windows,
  XSegment *rotate_info)
{
  /*
    Check boundary conditions.
  */
  if ((*rotate_info).x2 < 0)
    (*rotate_info).x2=0;
  else
    if ((*rotate_info).x2 > (int) windows->image.width)
      (*rotate_info).x2=(short) windows->image.width;
  if ((*rotate_info).y2 < 0)
    (*rotate_info).y2=0;
  else
    if ((*rotate_info).y2 > (int) windows->image.height)
      (*rotate_info).y2=(short) windows->image.height;
}

static void XRotateAngle(const XSegment *rotate_info,double *degrees,
  unsigned int *distance)
{
  /*
    Compute rotation angle from the slope of the line.
  */
  (*degrees)=0.0;
  (*distance)=(unsigned int)
    ((((*rotate_info).x2-(*rotate_info).x1+1)*
      ((*rotate_info).x2-(*rotate_info).x1+1))+
     (((*rotate_info).y2-(*rotate_info).y1+1)*
      ((*rotate_info).y2-(*rotate_info).y1+1)));
  if ((*distance) > 9)
    (*degrees)=RadiansToDegrees(-atan2((double) ((*rotate_info).y2-
      (*rotate_info).y1),(double) ((*rotate_info).x2-(*rotate_info).x1)));
}

static void XRotateDragLine(const RotateLoopInput *loop_input,
  const RotateLineTargets *line_targets,const ModeType direction)
{
  Display
    *const display = loop_input->display;

  XWindows
    *const windows = loop_input->windows;

  ExceptionInfo
    *const exception = loop_input->exception;

  XSegment
    *const rotate_info = line_targets->rotate_info;

  size_t
    *const state = line_targets->state;

  unsigned int
    *const distance = line_targets->distance;

  double
    *const degrees = line_targets->degrees;

  char
    text[MagickPathExtent];

  XEvent
    event;

  (*distance)=0;
  (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
  (*state)=DefaultState;
  do
  {
    if ((*distance) > 9)
      {
        RotateInfoInput
          show;

        show.display=display;
        show.windows=windows;
        show.text=text;
        show.direction=direction;
        show.degrees=(*degrees);
        show.rotate_info=(*rotate_info);
        XShowRotateInfo(&show);
      }
    else
      if (windows->info.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->info.id,
          windows->info.screen);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,&event,exception);
    if ((*distance) > 9)
      XHighlightLine(display,windows->image.id,
        windows->image.highlight_context,rotate_info);
    XRotateDragEvent(event,line_targets);
    XClampRotateSegment(windows,rotate_info);
    XRotateAngle(rotate_info,degrees,distance);
  } while (((*state) & ExitState) == 0);
  (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

typedef struct _RotateAngleTargets
{
  int
    *x,
    *y;

  ModeType
    *direction;

  double
    *degrees;
} RotateAngleTargets;

static MagickBooleanType XSelectRotateAngle(
  const RotateLoopInput *loop_input,const RotateAngleTargets *targets)
{
  Display
    *const display = loop_input->display;

  XWindows
    *const windows = loop_input->windows;

  const char
    *const *const RotateMenu = loop_input->RotateMenu;

  int
    *const x = targets->x,
    *const y = targets->y;

  ModeType
    *const direction = targets->direction;

  double
    *const degrees = targets->degrees;

  if ((*degrees) == 0.0)
    {
      unsigned int
        distance;

      size_t
        state;

      XSegment
        rotate_info;

      const RotateLineTargets
        line_targets = { &rotate_info, &state, &distance, degrees };

      /*
        Map Command widget.
      */
      (void) CloneString(&windows->command.name,"Rotate");
      windows->command.data=2;
      (void) XCommandWidget(display,windows,RotateMenu,(XEvent *) NULL);
      (void) XMapRaised(display,windows->command.id);
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_update_widget,CurrentTime);
      /*
        Wait for first button press.
      */
      (void) XSetFunction(display,windows->image.highlight_context,GXinvert);
      XQueryPosition(display,windows->image.id,x,y);
      rotate_info.x1=(*x);
      rotate_info.y1=(*y);
      rotate_info.x2=(*x);
      rotate_info.y2=(*y);
      state=DefaultState;
      do
      {
        XRotateSelectStep(loop_input,&line_targets,direction);
      } while ((state & ExitState) == 0);
      (void) XSetFunction(display,windows->image.highlight_context,GXcopy);
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      if ((state & EscapeState) != 0)
        return(MagickFalse);
      /*
        Draw line as pointer moves until the mouse button is released.
      */
      XRotateDragLine(loop_input,&line_targets,(*direction));
      if (distance <= 9)
        return(MagickFalse);
    }
  return(MagickTrue);
}

typedef struct _RotateCropInput
{
  XWindows
    *windows;

  Image
    **image;

  unsigned int
    rotations;

  int
    x,
    y;
} RotateCropInput;

static void XRotateCropGeometry(const RotateCropInput *crop,
  unsigned int *width,unsigned int *height)
{
  XWindows
    *const windows = crop->windows;

  Image
    **const image = crop->image;

  const unsigned int
    rotations = crop->rotations;

  int
    x = crop->x,
    y = crop->y;

  if (windows->image.crop_geometry != (char *) NULL)
    {
      /*
        Rotate crop geometry.
      */
      (*width)=(unsigned int) (*image)->columns;
      (*height)=(unsigned int) (*image)->rows;
      (void) XParseGeometry(windows->image.crop_geometry,&x,&y,width,height);
      switch (rotations % 4)
      {
        default:
        case 0:
          break;
        case 1:
        {
          /*
            Rotate 90 degrees.
          */
          (void) FormatLocaleString(windows->image.crop_geometry,
            MagickPathExtent,"%ux%u%+d%+d",(*height),(*width),
            (int) (*image)->columns-(int) (*height)-y,x);
          break;
        }
        case 2:
        {
          /*
            Rotate 180 degrees.
          */
          (void) FormatLocaleString(windows->image.crop_geometry,
            MagickPathExtent,"%ux%u%+d%+d",(*width),(*height),
            (int) (*width)-x,(int) (*height)-y);
          break;
        }
        case 3:
        {
          /*
            Rotate 270 degrees.
          */
          (void) FormatLocaleString(windows->image.crop_geometry,
            MagickPathExtent,"%ux%u%+d%+d",(*height),(*width),y,
            (int) (*image)->rows-(int) (*width)-x);
          break;
        }
      }
    }
}

typedef struct _RotateUpdateInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  ExceptionInfo
    *exception;

  double
    normalized_degrees;

  unsigned int
    rotations;
} RotateUpdateInput;

static MagickBooleanType XRotateUpdate(const RotateUpdateInput *update,
  unsigned int *width,unsigned int *height)
{
  Display
    *const display = update->display;

  XResourceInfo
    *const resource_info = update->resource_info;

  XWindows
    *const windows = update->windows;

  Image
    **const image = update->image;

  ExceptionInfo
    *const exception = update->exception;

  const double
    normalized_degrees = update->normalized_degrees;

  const unsigned int
    rotations = update->rotations;

  int
    x,
    y;

  if (windows->image.orphan != MagickFalse)
    return(MagickTrue);
  if (normalized_degrees != 0.0)
    {
      /*
        Update image colormap.
      */
      windows->image.window_changes.width=(int) (*image)->columns;
      windows->image.window_changes.height=(int) (*image)->rows;
      if (windows->image.crop_geometry != (char *) NULL)
        {
          /*
            Obtain dimensions of image from crop geometry.
          */
          (void) XParseGeometry(windows->image.crop_geometry,&x,&y,
            width,height);
          windows->image.window_changes.width=(int) (*width);
          windows->image.window_changes.height=(int) (*height);
        }
      XConfigureImageColormap(display,resource_info,windows,*image,exception);
    }
  else
    if (((rotations % 4) == 1) || ((rotations % 4) == 3))
      {
        windows->image.window_changes.width=windows->image.ximage->height;
        windows->image.window_changes.height=windows->image.ximage->width;
      }
  /*
    Update image configuration.
  */
  (void) XConfigureImage(display,resource_info,windows,*image,exception);
  return(MagickTrue);
}

typedef struct _RotateApplyInput
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  Image
    **image;

  ExceptionInfo
    *exception;

  double
    degrees,
    normalized_degrees;

  unsigned int
    pen_id;
} RotateApplyInput;

static MagickBooleanType XRotateApply(const RotateApplyInput *apply)
{
  Display
    *const display = apply->display;

  XResourceInfo
    *const resource_info = apply->resource_info;

  XWindows
    *const windows = apply->windows;

  Image
    **const image = apply->image;

  ExceptionInfo
    *const exception = apply->exception;

  const double
    degrees = apply->degrees,
    normalized_degrees = apply->normalized_degrees;

  const unsigned int
    pen_id = apply->pen_id;

  Image
    *rotate_image;

  if (normalized_degrees != 0.0)
    (void) XMagickCommand(display,resource_info,windows,ApplyCommand,image,
      exception);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  (*image)->background_color.red=(double) ScaleShortToQuantum(
    windows->pixel_info->pen_colors[pen_id].red);
  (*image)->background_color.green=(double) ScaleShortToQuantum(
    windows->pixel_info->pen_colors[pen_id].green);
  (*image)->background_color.blue=(double) ScaleShortToQuantum(
    windows->pixel_info->pen_colors[pen_id].blue);
  rotate_image=RotateImage(*image,degrees,exception);
  XSetCursorState(display,windows,MagickFalse);
  if (rotate_image == (Image *) NULL)
    return(MagickFalse);
  *image=DestroyImage(*image);
  *image=rotate_image;
  return(MagickTrue);
}

static const ModeType
  RotateDirectionTable[] =
  {
    HorizontalRotateCommand,
    VerticalRotateCommand
  },
  RotateCommandTable[] =
  {
    RotateColorCommand,
    RotateDirectionCommand,
    RotateHelpCommand,
    RotateDismissCommand
  };

static MagickBooleanType XRotateImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,double degrees,Image **image,
  ExceptionInfo *exception)
{
  const char
    *const RotateMenu[] =
    {
      "Pixel Color",
      "Direction",
      "Help",
      "Dismiss",
      (char *) NULL
    };

  static ModeType
    direction = HorizontalRotateCommand;

  static unsigned int
    pen_id = 0;

  char
    command[MagickPathExtent];

  int
    x,
    y;

  double
    normalized_degrees;

  unsigned int
    height,
    rotations,
    width;

  const RotateTargets
    targets = { &pen_id };

  const RotateLoopInput
    loop_input = { display, resource_info, windows, RotateMenu,
      RotateCommandTable, RotateDirectionTable, command, targets, exception };

  const RotateAngleTargets
    angle_targets = { &x, &y, &direction, &degrees };

  if (XSelectRotateAngle(&loop_input,&angle_targets) == MagickFalse)
    return(MagickTrue);
  if (direction == VerticalRotateCommand)
    degrees-=90.0;
  if (degrees == 0.0)
    return(MagickTrue);
  /*
    Rotate image.
  */
  normalized_degrees=degrees;
  while (normalized_degrees < -45.0)
    normalized_degrees+=360.0;
  for (rotations=0; normalized_degrees > 45.0; rotations++)
    normalized_degrees-=90.0;
  {
    const RotateApplyInput
      apply = { display, resource_info, windows, image, exception, degrees,
        normalized_degrees, pen_id };

    if (XRotateApply(&apply) == MagickFalse)
      return(MagickFalse);
  }
  {
    const RotateCropInput
      crop = { windows, image, rotations, x, y };

    XRotateCropGeometry(&crop,&width,&height);
  }
  {
    const RotateUpdateInput
      update = { display, resource_info, windows, image, exception,
        normalized_degrees, rotations };

    return(XRotateUpdate(&update,&width,&height));
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSaveImage() saves an image to a file.
%
%  The format of the XSaveImage method is:
%
%      MagickBooleanType XSaveImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsPostscriptFormat(
  const ImageInfo *image_info)
{
  return(((LocaleCompare(image_info->magick,"EPS") == 0) ||
    (LocaleCompare(image_info->magick,"PDF") == 0) ||
    (LocaleCompare(image_info->magick,"PS") == 0) ||
    (LocaleCompare(image_info->magick,"PS2") == 0)) ? MagickTrue :
    MagickFalse);
}

static void XSelectPageGeometry(Display *display,XWindows *windows,
  ImageInfo *image_info)
{
  if (IsPostscriptFormat(image_info) != MagickFalse)
    {
      char
        geometry[MagickPathExtent];

      const char
        *const PageSizes[] =
        {
          "Letter",
          "Tabloid",
          "Ledger",
          "Legal",
          "Statement",
          "Executive",
          "A3",
          "A4",
          "A5",
          "B4",
          "B5",
          "Folio",
          "Quarto",
          "10x14",
          (char *) NULL
        };

      /*
        Request page geometry from user.
      */
      (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
      if (LocaleCompare(image_info->magick,"PDF") == 0)
        (void) CopyMagickString(geometry,PSPageGeometry,MagickPathExtent);
      if (image_info->page != (char *) NULL)
        (void) CopyMagickString(geometry,image_info->page,MagickPathExtent);
      XListBrowserWidget(display,windows,&windows->widget,PageSizes,"Select",
        "Select page geometry:",geometry);
      if (*geometry != '\0')
        image_info->page=GetPageGeometry(geometry);
    }
}

static MagickBooleanType XRequestJPEGQuality(Display *display,
  XWindows *windows,Image *image,ImageInfo *image_info)
{
  if ((LocaleCompare(image_info->magick,"JPEG") == 0) ||
      (LocaleCompare(image_info->magick,"JPG") == 0))
    {
      char
        quality[MagickPathExtent];

      int
        status;

      /*
        Request JPEG quality from user.
      */
      (void) FormatLocaleString(quality,MagickPathExtent,"%.17g",(double)
        image->quality);
      status=XDialogWidget(display,windows,"Save","Enter JPEG quality:",
        quality);
      if (*quality == '\0')
        return(MagickFalse);
      image->quality=StringToUnsignedLong(quality);
      image_info->interlace=status != 0 ? NoInterlace : PlaneInterlace;
    }
  return(MagickTrue);
}

static MagickBooleanType XSeekOverwritePermission(Display *display,
  XWindows *windows,const char *filename)
{
  if (IsPathAccessible(filename) != MagickFalse)
    {
      int
        status;

      /*
        File exists-- seek user's permission before overwriting.
      */
      status=XConfirmWidget(display,windows,"Overwrite",filename);
      if (status <= 0)
        return(MagickFalse);
    }
  return(MagickTrue);
}

static void XGetSaveFilename(const XResourceInfo *resource_info,
  const Image *image,char *filename,ExceptionInfo *exception)
{
  if (resource_info->write_filename != (char *) NULL)
    (void) CopyMagickString(filename,resource_info->write_filename,
      MagickPathExtent);
  else
    {
      char
        path[MagickPathExtent];

      int
        status;

      GetPathComponent(image->filename,HeadPath,path);
      GetPathComponent(image->filename,TailPath,filename);
      if (*path != '\0')
        {
          status=chdir(path);
          if (status == -1)
            (void) ThrowMagickException(exception,GetMagickModule(),
              FileOpenError,"UnableToOpenFile","%s",path);
        }
    }
}

static MagickBooleanType XSaveImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent],
    geometry[MagickPathExtent];

  Image
    *save_image;

  ImageInfo
    *image_info;

  MagickStatusType
    status;

  /*
    Request file name from user.
  */
  XGetSaveFilename(resource_info,image,filename,exception);
  XFileBrowserWidget(display,windows,"Save",filename);
  if (*filename == '\0')
    return(MagickTrue);
  if (XSeekOverwritePermission(display,windows,filename) == MagickFalse)
    return(MagickTrue);
  image_info=CloneImageInfo(resource_info->image_info);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) SetImageInfo(image_info,1,exception);
  if (XRequestJPEGQuality(display,windows,image,image_info) == MagickFalse)
    return(MagickTrue);
  XSelectPageGeometry(display,windows,image_info);
  /*
    Apply image transforms.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  save_image=CloneImage(image,0,0,MagickTrue,exception);
  if (save_image == (Image *) NULL)
    return(MagickFalse);
  (void) FormatLocaleString(geometry,MagickPathExtent,"%dx%d!",
    windows->image.ximage->width,windows->image.ximage->height);
  (void) TransformImage(&save_image,windows->image.crop_geometry,geometry,
    exception);
  /*
    Write image.
  */
  (void) CopyMagickString(save_image->filename,filename,MagickPathExtent);
  status=WriteImage(image_info,save_image,exception);
  if (status != MagickFalse)
    image->taint=MagickFalse;
  save_image=DestroyImage(save_image);
  image_info=DestroyImageInfo(image_info);
  XSetCursorState(display,windows,MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S c r e e n E v e n t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XScreenEvent() handles global events associated with the Pan and Magnify
%  windows.
%
%  The format of the XScreenEvent function is:
%
%      void XScreenEvent(Display *display,XWindows *windows,XEvent *event,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a pointer to the Display structure;  returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o event: Specifies a pointer to a X11 XEvent structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int XPredicate(Display *magick_unused(display),XEvent *event,char *data)
{
  XWindows
    *windows;

  windows=(XWindows *) data;
  if ((event->type == ClientMessage) &&
      (event->xclient.window == windows->image.id))
    return(MagickFalse);
  return(MagickTrue);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static void XSetMagnifyPosition(XWindows *windows,int x,int y)
{
  if (x < 0)
    x=0;
  else
    if (x >= (int) windows->image.width)
      x=(int) (windows->image.width-1);
  windows->magnify.x=(int) windows->image.x+x;
  if (y < 0)
    y=0;
  else
   if (y >= (int) windows->image.height)
     y=(int) (windows->image.height-1);
  windows->magnify.y=windows->image.y+y;
}

static void XMagnifyAtButton(Display *display,XWindows *windows,
  XEvent *event,ExceptionInfo *exception)
{
  int
    x,
    y;

  /*
    Update magnified image.
  */
  x=event->xbutton.x;
  y=event->xbutton.y;
  XSetMagnifyPosition(windows,x,y);
  if (windows->magnify.mapped == MagickFalse)
    (void) XMapRaised(display,windows->magnify.id);
  XMakeMagnifyImage(display,windows,exception);
  if (event->type == ButtonRelease)
    (void) XWithdrawWindow(display,windows->info.id,
      windows->info.screen);
}

static void XScreenButton(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if ((event->xbutton.button == Button3) &&
      (event->xbutton.state & Mod1Mask))
    {
      /*
        Convert Alt-Button3 to Button2.
      */
      event->xbutton.button=Button2;
      event->xbutton.state&=(unsigned int) (~Mod1Mask);
    }
  if (event->xbutton.window == windows->backdrop.id)
    {
      (void) XSetInputFocus(display,event->xbutton.window,RevertToParent,
        event->xbutton.time);
      return;
    }
  if (event->xbutton.window == windows->pan.id)
    {
      XPanImage(display,windows,event,exception);
      return;
    }
  if ((event->xbutton.window == windows->image.id) &&
      (event->xbutton.button == Button2))
    {
      XMagnifyAtButton(display,windows,event,exception);
      return;
    }
}

static void XScreenClientMessage(Display *display,XWindows *windows,
  XEvent *event)
{
  /*
    If client window delete message, exit.
  */
  if (event->xclient.message_type != windows->wm_protocols)
    return;
  if (*event->xclient.data.l != (long) windows->wm_delete_window)
    return;
  if (event->xclient.window == windows->magnify.id)
    {
      (void) XWithdrawWindow(display,windows->magnify.id,
        windows->magnify.screen);
      return;
    }
}

static void XScreenConfigure(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if (event->xconfigure.window == windows->magnify.id)
    {
      unsigned int
        magnify;

      /*
        Magnify window has a new configuration.
      */
      windows->magnify.width=(unsigned int) event->xconfigure.width;
      windows->magnify.height=(unsigned int) event->xconfigure.height;
      if (windows->magnify.mapped == MagickFalse)
        return;
      magnify=1;
      while ((int) magnify <= event->xconfigure.width)
        magnify<<=1;
      while ((int) magnify <= event->xconfigure.height)
        magnify<<=1;
      magnify>>=1;
      if (((int) magnify != event->xconfigure.width) ||
          ((int) magnify != event->xconfigure.height))
        {
          XWindowChanges
            window_changes;

          window_changes.width=(int) magnify;
          window_changes.height=(int) magnify;
          (void) XReconfigureWMWindow(display,windows->magnify.id,
            windows->magnify.screen,(unsigned int) (CWWidth | CWHeight),
            &window_changes);
          return;
        }
      XMakeMagnifyImage(display,windows,exception);
      return;
    }
}

static void XScreenExpose(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  if (event->xexpose.window == windows->image.id)
    {
      XRefreshWindow(display,&windows->image,event);
      return;
    }
  if ((event->xexpose.window == windows->pan.id) &&
      (event->xexpose.count == 0))
    {
      XDrawPanRectangle(display,windows);
      return;
    }
  if ((event->xexpose.window == windows->magnify.id) &&
      (event->xexpose.count == 0))
    {
      XMakeMagnifyImage(display,windows,exception);
      return;
    }
}

static void XScreenKeyPress(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  if (event->xkey.window != windows->magnify.id)
    return;
  /*
    Respond to a user key press.
  */
  (void) XLookupString((XKeyEvent *) &event->xkey,command,(int)
    sizeof(command),&key_symbol,(XComposeStatus *) NULL);
  XMagnifyWindowCommand(display,windows,event->xkey.state,key_symbol,
    exception);
}

static void XScreenMap(Display *display,XWindows *windows,XEvent *event)
{
  if (event->xmap.window == windows->magnify.id)
    {
      windows->magnify.mapped=MagickTrue;
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      return;
    }
  if (event->xmap.window == windows->info.id)
    {
      windows->info.mapped=MagickTrue;
      return;
    }
}

static void XScreenMotion(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  int
    x,
    y;

  while (XCheckMaskEvent(display,ButtonMotionMask,event)) ;
  if ((event->xmotion.window == windows->image.id) &&
      (windows->magnify.mapped != MagickFalse))
    {
      /*
        Update magnified image.
      */
      x=event->xmotion.x;
      y=event->xmotion.y;
      XSetMagnifyPosition(windows,x,y);
      XMakeMagnifyImage(display,windows,exception);
    }
}

static void XScreenUnmap(Display *display,XWindows *windows,XEvent *event)
{
  if (event->xunmap.window == windows->magnify.id)
    {
      windows->magnify.mapped=MagickFalse;
      return;
    }
  if (event->xunmap.window == windows->info.id)
    {
      windows->info.mapped=MagickFalse;
      return;
    }
}

static void XScreenOtherEvent(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  switch (event->type)
  {
    case Expose:
    {
      XScreenExpose(display,windows,event,exception);
      break;
    }
    case KeyPress:
    {
      XScreenKeyPress(display,windows,event,exception);
      break;
    }
    case MapNotify:
    {
      XScreenMap(display,windows,event);
      break;
    }
    case MotionNotify:
    {
      XScreenMotion(display,windows,event,exception);
      break;
    }
    case UnmapNotify:
    {
      XScreenUnmap(display,windows,event);
      break;
    }
    default:
      break;
  }
}

static void XScreenEvent(Display *display,XWindows *windows,XEvent *event,
  ExceptionInfo *exception)
{
  (void) XIfEvent(display,event,XPredicate,(char *) windows);
  if (event->xany.window == windows->command.id)
    return;
  switch (event->type)
  {
    case ButtonPress:
    case ButtonRelease:
    {
      XScreenButton(display,windows,event,exception);
      break;
    }
    case ClientMessage:
    {
      XScreenClientMessage(display,windows,event);
      break;
    }
    case ConfigureNotify:
    {
      XScreenConfigure(display,windows,event,exception);
      break;
    }
    default:
    {
      XScreenOtherEvent(display,windows,event,exception);
      break;
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S e t C r o p G e o m e t r y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XSetCropGeometry() accepts a cropping geometry relative to the Image window
%  and translates it to a cropping geometry relative to the image.
%
%  The format of the XSetCropGeometry method is:
%
%      void XSetCropGeometry(Display *display,XWindows *windows,
%        RectangleInfo *crop_info,Image *image)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o crop_info:  A pointer to a RectangleInfo that defines a region of the
%      Image window to crop.
%
%    o image: the image.
%
*/
static void XSetCropGeometry(Display *display,XWindows *windows,
  RectangleInfo *crop_info,Image *image)
{
  char
    text[MagickPathExtent];

  int
    x,
    y;

  double
    scale_factor;

  unsigned int
    height,
    width;

  if (windows->info.mapped != MagickFalse)
    {
      /*
        Display info on cropping rectangle.
      */
      (void) FormatLocaleString(text,MagickPathExtent," %.17gx%.17g%+.20g%+.20g",
        (double) crop_info->width,(double) crop_info->height,(double)
        crop_info->x,(double) crop_info->y);
      XInfoWidget(display,windows,text);
    }
  /*
    Cropping geometry is relative to any previous crop geometry.
  */
  x=0;
  y=0;
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  else
    windows->image.crop_geometry=AcquireString((char *) NULL);
  /*
    Define the crop geometry string from the cropping rectangle.
  */
  scale_factor=(double) width/windows->image.ximage->width;
  if (crop_info->x > 0)
    x+=(int) (scale_factor*crop_info->x+0.5);
  width=(unsigned int) (scale_factor*crop_info->width+0.5);
  if (width == 0)
    width=1;
  scale_factor=(double) height/windows->image.ximage->height;
  if (crop_info->y > 0)
    y+=(int) (scale_factor*crop_info->y+0.5);
  height=(unsigned int) (scale_factor*crop_info->height+0.5);
  if (height == 0)
    height=1;
  (void) FormatLocaleString(windows->image.crop_geometry,MagickPathExtent,
    "%ux%u%+d%+d",width,height,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T i l e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTileImage() loads or deletes a selected tile from a visual image directory.
%  The load or delete command is chosen from a menu.
%
%  The format of the XTileImage method is:
%
%      Image *XTileImage(Display *display,XResourceInfo *resource_info,
%        XWindows *windows,Image *image,XEvent *event,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o tile_image:  XTileImage reads or deletes the tile image
%      and returns it.  A null image is returned if an error occurs.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image; returned from ReadImage.
%
%    o event: Specifies a pointer to a XEvent structure.  If it is NULL,
%      the entire image is refreshed.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsTileNameCharacter(const char *q)
{
  return(((*q != '\xff') && (*q != '\0')) ? MagickTrue : MagickFalse);
}

typedef struct _XTileFillInfo
{
  Image
    *image;

  CacheView
    *image_view;

  int
    x_offset,
    y_offset;

  unsigned int
    height,
    width;

  PixelInfo
    pixel;

  ExceptionInfo
    *exception;
} XTileFillInfo;

static void XFillMissingTile(const XTileFillInfo *fill)
{
  Image
    *const image = fill->image;

  CacheView
    *const image_view = fill->image_view;

  const int
    x_offset = fill->x_offset,
    y_offset = fill->y_offset;

  const unsigned int
    height = fill->height,
    width = fill->width;

  const PixelInfo
    pixel = fill->pixel;

  ExceptionInfo
    *const exception = fill->exception;

  int
    i,
    j;

  Quantum
    *s;

  for (i=0; i < (int) height; i++)
  {
    s=GetCacheViewAuthenticPixels(image_view,(ssize_t) x_offset,(ssize_t)
      y_offset+i,width,1,exception);
    if (s == (Quantum *) NULL)
      break;
    for (j=0; j < (int) width; j++)
    {
      SetPixelViaPixelInfo(image,&pixel,s);
      s+=(ptrdiff_t) GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
  }
}

static Image *XTileImage(Display *display,XResourceInfo *resource_info,
  XWindows *windows,Image *image,XEvent *event,ExceptionInfo *exception)
{
  const char
    *const VerbMenu[] =
    {
      "Load",
      "Next",
      "Former",
      "Delete",
      "Update",
      (char *) NULL,
    };

  static const ModeType
    TileCommands[] =
    {
      TileLoadCommand,
      TileNextCommand,
      TileFormerCommand,
      TileDeleteCommand,
      TileUpdateCommand
    };

  char
    command[MagickPathExtent],
    filename[MagickPathExtent];

  Image
    *tile_image;

  int
    id,
    status,
    tile,
    x,
    y;

  double
    scale_factor;

  char
    *p,
    *q;

  int
    i;

  unsigned int
    height,
    width;

  /*
    Tile image is relative to montage image configuration.
  */
  x=0;
  y=0;
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  if (windows->image.crop_geometry != (char *) NULL)
    (void) XParseGeometry(windows->image.crop_geometry,&x,&y,&width,&height);
  scale_factor=(double) width/windows->image.ximage->width;
  event->xbutton.x+=windows->image.x;
  event->xbutton.x=(int) (scale_factor*event->xbutton.x+x+0.5);
  scale_factor=(double) height/windows->image.ximage->height;
  event->xbutton.y+=windows->image.y;
  event->xbutton.y=(int) (scale_factor*event->xbutton.y+y+0.5);
  /*
    Determine size and location of each tile in the visual image directory.
  */
  width=(unsigned int) image->columns;
  height=(unsigned int) image->rows;
  x=0;
  y=0;
  (void) XParseGeometry(image->montage,&x,&y,&width,&height);
  tile=((event->xbutton.y-y)/(int) height)*(((int) image->columns-x)/(int)
    width)+(event->xbutton.x-x)/(int) width;
  if (tile < 0)
    {
      /*
        Button press is outside any tile.
      */
      (void) XBell(display,0);
      return((Image *) NULL);
    }
  /*
    Determine file name from the tile directory.
  */
  p=image->directory;
  for (i=tile; (i != 0) && (*p != '\0'); )
  {
    if (*p == '\xff')
      i--;
    p++;
  }
  if (*p == '\0')
    {
      /*
        Button press is outside any tile.
      */
      (void) XBell(display,0);
      return((Image *) NULL);
    }
  /*
    Select a command from the pop-up menu.
  */
  id=XMenuWidget(display,windows,"Tile Verb",VerbMenu,command);
  if (id < 0)
    return((Image *) NULL);
  q=p;
  while ((IsTileNameCharacter(q) != MagickFalse) &&
         ((size_t) (q-p+1) < sizeof(filename)))
    q++;
  (void) CopyMagickString(filename,p,(size_t) (q-p+1));
  /*
    Perform command for the selected tile.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  tile_image=NewImageList();
  switch (TileCommands[id])
  {
    case TileLoadCommand:
    {
      /*
        Load tile image.
      */
      XCheckRefreshWindows(display,windows);
      (void) CopyMagickString(resource_info->image_info->magick,"MIFF",
        MagickPathExtent);
      (void) CopyMagickString(resource_info->image_info->filename,filename,
        MagickPathExtent);
      tile_image=ReadImage(resource_info->image_info,exception);
      CatchException(exception);
      (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      break;
    }
    case TileNextCommand:
    {
      /*
        Display next image.
      */
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_next_image,CurrentTime);
      break;
    }
    case TileFormerCommand:
    {
      /*
        Display former image.
      */
      XClientMessage(display,windows->image.id,windows->im_protocols,
        windows->im_former_image,CurrentTime);
      break;
    }
    case TileDeleteCommand:
    {
      /*
        Delete tile image.
      */
      if (IsPathAccessible(filename) == MagickFalse)
        {
          XNoticeWidget(display,windows,"Image file does not exist:",filename);
          break;
        }
      status=XConfirmWidget(display,windows,"Really delete tile",filename);
      if (status <= 0)
        break;
      status=ShredFile(filename) == MagickFalse ? 0 : 1;
      status|=remove_utf8(filename);
      if (status != MagickFalse)
        {
          XNoticeWidget(display,windows,"Unable to delete image file:",
            filename);
          break;
        }
      magick_fallthrough;
    }
    case TileUpdateCommand:
    {
      int
        x_offset,
        y_offset;

      PixelInfo
        pixel;

      /*
        Ensure all the images exist.
      */
      tile=0;
      GetPixelInfo(image,&pixel);
      for (p=image->directory; *p != '\0'; p++)
      {
        CacheView
          *image_view;

        XTileFillInfo
          fill;

        q=p;
        while ((IsTileNameCharacter(q) != MagickFalse) &&
               ((size_t) (q-p+1) < sizeof(filename)))
          q++;
        (void) CopyMagickString(filename,p,(size_t) (q-p+1));
        p=q;
        if (IsPathAccessible(filename) != MagickFalse)
          {
            tile++;
            continue;
          }
        /*
          Overwrite tile with background color.
        */
        x_offset=((int) width*(tile % (((int) image->columns-x)/(int) width))+
          x);
        y_offset=((int) height*(tile/(((int) image->columns-x)/(int) width))+
          y);
        image_view=AcquireAuthenticCacheView(image,exception);
        (void) GetOneCacheViewVirtualPixelInfo(image_view,0,0,&pixel,exception);
        fill.image=image;
        fill.image_view=image_view;
        fill.x_offset=x_offset;
        fill.y_offset=y_offset;
        fill.height=height;
        fill.width=width;
        fill.pixel=pixel;
        fill.exception=exception;
        XFillMissingTile(&fill);
        image_view=DestroyCacheView(image_view);
        tile++;
      }
      windows->image.window_changes.width=(int) image->columns;
      windows->image.window_changes.height=(int) image->rows;
      XConfigureImageColormap(display,resource_info,windows,image,exception);
      (void) XConfigureImage(display,resource_info,windows,image,exception);
      break;
    }
    default:
      break;
  }
  XSetCursorState(display,windows,MagickFalse);
  return(tile_image);
}

static void XClampImageOffset(XWindows *windows)
{
  if (windows->image.x < 0)
    windows->image.x=0;
  else
    if ((windows->image.x+(int) windows->image.width) > windows->image.ximage->width)
      windows->image.x=windows->image.ximage->width-(int) windows->image.width;
  if (windows->image.y < 0)
    windows->image.y=0;
  else
    if ((windows->image.y+(int) windows->image.height) > windows->image.ximage->height)
      windows->image.y=windows->image.ximage->height-(int)
        windows->image.height;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T r a n s l a t e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTranslateImage() translates the image within an Image window by one pixel
%  as specified by the key symbol.  If the image has a montage string the
%  translation is respect to the width and height contained within the string.
%
%  The format of the XTranslateImage method is:
%
%      void XTranslateImage(Display *display,XWindows *windows,
%        Image *image,const KeySym key_symbol)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o key_symbol: Specifies a KeySym which indicates which side of the image
%      to trim.
%
*/
static MagickBooleanType XOffsetImageWindowDown(XWindows *windows,
  const KeySym key_symbol,const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Prior:
    case XK_Down:
    case XK_KP_Down:
    {
      windows->image.y+=(int) y_offset;
      break;
    }
    default:
      return(MagickFalse);
  }
  return(MagickTrue);
}

static MagickBooleanType XOffsetImageWindowUpRight(XWindows *windows,
  const KeySym key_symbol,const unsigned int x_offset,
  const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Next:
    case XK_Up:
    case XK_KP_Up:
    {
      windows->image.y-=(int) y_offset;
      break;
    }
    case XK_Right:
    case XK_KP_Right:
    {
      windows->image.x+=(int) x_offset;
      break;
    }
    default:
    {
      return(XOffsetImageWindowDown(windows,key_symbol,y_offset));
    }
  }
  return(MagickTrue);
}

static MagickBooleanType XOffsetImageWindow(XWindows *windows,
  const KeySym key_symbol,const unsigned int x_offset,
  const unsigned int y_offset)
{
  switch ((int) key_symbol)
  {
    case XK_Home:
    case XK_KP_Home:
    {
      windows->image.x=(int) windows->image.width/2;
      windows->image.y=(int) windows->image.height/2;
      break;
    }
    case XK_Left:
    case XK_KP_Left:
    {
      windows->image.x-=(int) x_offset;
      break;
    }
    default:
    {
      return(XOffsetImageWindowUpRight(windows,key_symbol,x_offset,y_offset));
    }
  }
  return(MagickTrue);
}

static void XTranslateImage(Display *display,XWindows *windows,
  Image *image,const KeySym key_symbol)
{
  char
    text[MagickPathExtent];

  int
    x,
    y;

  unsigned int
    x_offset,
    y_offset;

  /*
    User specified a pan position offset.
  */
  x_offset=windows->image.width;
  y_offset=windows->image.height;
  if (image->montage != (char *) NULL)
    (void) XParseGeometry(image->montage,&x,&y,&x_offset,&y_offset);
  if (XOffsetImageWindow(windows,key_symbol,x_offset,y_offset) == MagickFalse)
    return;
  /*
    Check boundary conditions.
  */
  XClampImageOffset(windows);
  /*
    Refresh Image window.
  */
  (void) FormatLocaleString(text,MagickPathExtent," %ux%u%+d%+d ",
    windows->image.width,windows->image.height,windows->image.x,
    windows->image.y);
  XInfoWidget(display,windows,text);
  XCheckRefreshWindows(display,windows);
  XDrawPanRectangle(display,windows);
  XRefreshWindow(display,&windows->image,(XEvent *) NULL);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X T r i m I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XTrimImage() trims the edges from the Image window.
%
%  The format of the XTrimImage method is:
%
%      MagickBooleanType XTrimImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static void XTrimLeftEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the left edge.
  */
  background=XGetPixel(windows->image.ximage,0,0);
  (*trim_info).width=(size_t) windows->image.ximage->width;
  for (x=0; x < windows->image.ximage->width; x++)
  {
    for (y=0; y < windows->image.ximage->height; y++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (y < windows->image.ximage->height)
      break;
  }
  (*trim_info).x=(ssize_t) x;
}

static void XTrimRightEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the right edge.
  */
  background=XGetPixel(windows->image.ximage,windows->image.ximage->width-1,0);
  for (x=windows->image.ximage->width-1; x != 0; x--)
  {
    for (y=0; y < windows->image.ximage->height; y++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (y < windows->image.ximage->height)
      break;
  }
  (*trim_info).width=(size_t) (x-(*trim_info).x+1);
}

static void XTrimTopEdge(const XWindows *windows,RectangleInfo *trim_info)
{
  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Crop the top edge.
  */
  background=XGetPixel(windows->image.ximage,0,0);
  (*trim_info).height=(size_t) windows->image.ximage->height;
  for (y=0; y < windows->image.ximage->height; y++)
  {
    for (x=0; x < windows->image.ximage->width; x++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (x < windows->image.ximage->width)
      break;
  }
  (*trim_info).y=(ssize_t) y;
}

static MagickBooleanType XTrimImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,Image *image,
  ExceptionInfo *exception)
{
  RectangleInfo
    trim_info;

  int
    x,
    y;

  size_t
    background,
    pixel;

  /*
    Trim edges from image.
  */
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  /*
    Crop the left edge.
  */
  XTrimLeftEdge(windows,&trim_info);
  if (trim_info.x == (ssize_t) windows->image.ximage->width)
    {
      XSetCursorState(display,windows,MagickFalse);
      return(MagickFalse);
    }
  /*
    Crop the right edge.
  */
  XTrimRightEdge(windows,&trim_info);
  /*
    Crop the top edge.
  */
  XTrimTopEdge(windows,&trim_info);
  /*
    Crop the bottom edge.
  */
  background=XGetPixel(windows->image.ximage,0,windows->image.ximage->height-1);
  for (y=windows->image.ximage->height-1; y != 0; y--)
  {
    for (x=0; x < windows->image.ximage->width; x++)
    {
      pixel=XGetPixel(windows->image.ximage,x,y);
      if (pixel != background)
        break;
    }
    if (x < windows->image.ximage->width)
      break;
  }
  trim_info.height=(size_t) (y-trim_info.y+1);
  if (((unsigned int) trim_info.width != windows->image.width) ||
      ((unsigned int) trim_info.height != windows->image.height))
    {
      /*
        Reconfigure Image window as defined by the trimming rectangle.
      */
      XSetCropGeometry(display,windows,&trim_info,image);
      windows->image.window_changes.width=(int) trim_info.width;
      windows->image.window_changes.height=(int) trim_info.height;
      (void) XConfigureImage(display,resource_info,windows,image,exception);
    }
  XSetCursorState(display,windows,MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X V i s u a l D i r e c t o r y I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XVisualDirectoryImage() creates a Visual Image Directory.
%
%  The format of the XVisualDirectoryImage method is:
%
%      Image *XVisualDirectoryImage(Display *display,
%        XResourceInfo *resource_info,XWindows *windows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server; returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o windows: Specifies a pointer to a XWindows structure.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *XVisualDirectoryImage(Display *display,
  XResourceInfo *resource_info,XWindows *windows,ExceptionInfo *exception)
{
#define TileImageTag  "Scale/Image"
#define XClientName  "montage"

  char
    **filelist;

  Image
    *images,
    *montage_image,
    *next_image,
    *thumbnail_image;

  ImageInfo
    *read_info;

  int
    number_files;

  MagickBooleanType
    backdrop;

  MagickStatusType
    status;

  MontageInfo
    *montage_info;

  RectangleInfo
    geometry;

  int
    i;

  static char
    filename[MagickPathExtent] = "\0",
    filenames[MagickPathExtent] = "*";

  XResourceInfo
    background_resources;

  /*
    Request file name from user.
  */
  XFileBrowserWidget(display,windows,"Directory",filenames);
  if (*filenames == '\0')
    return((Image *) NULL);
  /*
    Expand the filenames.
  */
  filelist=(char **) AcquireMagickMemory(sizeof(*filelist));
  if (filelist == (char **) NULL)
    {
      ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
        filenames);
      return((Image *) NULL);
    }
  number_files=1;
  filelist[0]=filenames;
  status=ExpandFilenames(&number_files,&filelist);
  if ((status == MagickFalse) || (number_files == 0))
    {
      if (number_files == 0)
        ThrowXWindowException(ImageError,"NoImagesWereFound",filenames)
      else
        ThrowXWindowException(ResourceLimitError,"MemoryAllocationFailed",
          filenames);
      return((Image *) NULL);
    }
  /*
    Set image background resources.
  */
  background_resources=(*resource_info);
  background_resources.window_id=AcquireString("");
  (void) FormatLocaleString(background_resources.window_id,MagickPathExtent,
    "0x%lx",windows->image.id);
  background_resources.backdrop=MagickTrue;
  /*
    Read each image and convert them to a tile.
  */
  backdrop=((windows->visual_info->klass == TrueColor) ||
    (windows->visual_info->klass == DirectColor)) ? MagickTrue : MagickFalse;
  read_info=CloneImageInfo(resource_info->image_info);
  (void) SetImageOption(read_info,"jpeg:size","120x120");
  (void) CloneString(&read_info->size,DefaultTileGeometry);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  images=NewImageList();
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  for (i=0; i < (int) number_files; i++)
  {
    (void) CopyMagickString(read_info->filename,filelist[i],MagickPathExtent);
    filelist[i]=DestroyString(filelist[i]);
    *read_info->magick='\0';
    next_image=ReadImage(read_info,exception);
    CatchException(exception);
    if (next_image == (Image *) NULL)
      continue;
    (void) DeleteImageProperty(next_image,"label");
    (void) SetImageProperty(next_image,"label",InterpretImageProperties(
      read_info,next_image,DefaultTileLabel,exception),exception);
    (void) ParseRegionGeometry(next_image,read_info->size,&geometry,
      exception);
    thumbnail_image=ThumbnailImage(next_image,geometry.width,
      geometry.height,exception);
    if (thumbnail_image != (Image *) NULL)
      {
        next_image=DestroyImage(next_image);
        next_image=thumbnail_image;
      }
    if (backdrop)
      {
        (void) XDisplayBackgroundImage(display,&background_resources,
          next_image,exception);
        XSetCursorState(display,windows,MagickTrue);
      }
    AppendImageToList(&images,next_image);
    if (images->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

        proceed=SetImageProgress(images,LoadImageTag,(MagickOffsetType) i,
          (MagickSizeType) number_files);
        if (proceed == MagickFalse)
          break;
      }
  }
  filelist=(char **) RelinquishMagickMemory(filelist);
  if (images == (Image *) NULL)
    {
      read_info=DestroyImageInfo(read_info);
      XSetCursorState(display,windows,MagickFalse);
      ThrowXWindowException(ImageError,"NoImagesWereLoaded",filenames);
      return((Image *) NULL);
    }
  /*
    Create the Visual Image Directory.
  */
  montage_info=CloneMontageInfo(read_info,(MontageInfo *) NULL);
  montage_info->pointsize=10;
  if (resource_info->font != (char *) NULL)
    (void) CloneString(&montage_info->font,resource_info->font);
  (void) CopyMagickString(montage_info->filename,filename,MagickPathExtent);
  montage_image=MontageImageList(read_info,montage_info,GetFirstImageInList(
    images),exception);
  images=DestroyImageList(images);
  montage_info=DestroyMontageInfo(montage_info);
  read_info=DestroyImageInfo(read_info);
  XSetCursorState(display,windows,MagickFalse);
  if (montage_image == (Image *) NULL)
    return(montage_image);
  XClientMessage(display,windows->image.id,windows->im_protocols,
    windows->im_next_image,CurrentTime);
  return(montage_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   X D i s p l a y B a c k g r o u n d I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDisplayBackgroundImage() displays an image in the background of a window.
%
%  The format of the XDisplayBackgroundImage method is:
%
%      MagickBooleanType XDisplayBackgroundImage(Display *display,
%        XResourceInfo *resource_info,Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o image: the image.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static MagickBooleanType XFindBackgroundWindow(Display *display,
  char *window_id,const Window root_window,XWindowInfo *window_info)
{
  if (LocaleCompare(window_id,"root") == 0)
    (*window_info).id=root_window;
  else
    {
      if (isdigit((int) ((unsigned char) *window_id)) != 0)
        (*window_info).id=XWindowByID(display,root_window,
          (Window) strtol((char *) window_id,(char **) NULL,0));
      if ((*window_info).id == (Window) NULL)
        (*window_info).id=XWindowByName(display,root_window,window_id);
    }
  if ((*window_info).id == (Window) NULL)
    {
      ThrowXWindowException(XServerError,"NoWindowWithSpecifiedIDExists",
        window_id);
      return(MagickFalse);
    }
  return(MagickTrue);
}

static void XGetBackgroundVisualType(Display *display,
  const XWindowInfo *window_info,XWindowAttributes *window_attributes,
  char *visual_type)
{
  int
    status;

  (*window_attributes).width=XDisplayWidth(display,XDefaultScreen(display));
  (*window_attributes).height=XDisplayHeight(display,XDefaultScreen(display));
  (void) CopyMagickString(visual_type,"default",MagickPathExtent);
  status=XGetWindowAttributes(display,(*window_info).id,window_attributes);
  if (status != 0)
    (void) FormatLocaleString(visual_type,MagickPathExtent,"0x%lx",
      XVisualIDFromVisual((*window_attributes).visual));
}

static void XFitBackgroundWindow(const XWindowAttributes *window_attributes,
  XWindowInfo *window_info)
{
  char
    geometry[MagickPathExtent];

  RectangleInfo
    geometry_info;

  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>",
    (*window_attributes).width,(*window_attributes).height);
  geometry_info.width=(*window_info).width;
  geometry_info.height=(*window_info).height;
  geometry_info.x=(ssize_t) (*window_info).x;
  geometry_info.y=(ssize_t) (*window_info).y;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  (*window_info).width=(unsigned int) geometry_info.width;
  (*window_info).height=(unsigned int) geometry_info.height;
  (*window_info).x=(int) geometry_info.x;
  (*window_info).y=(int) geometry_info.y;
}

typedef struct _BackgroundTargets
{
  XStandardColormap
    **map_info;

  XPixelInfo
    *pixel;

  XResourceInfo
    *resources;

  XVisualInfo
    **visual_info;

  XWindowInfo
    *window_info;
} BackgroundTargets;

static void XInitializeBackgroundVisual(Display *display,const Image *image,
  char *visual_type,const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  /*
    Allocate standard colormap.
  */
  (*map_info)=XAllocStandardColormap();
  if ((*map_info) == (XStandardColormap *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"MemoryAllocationFailed",
      image->filename);
  (*map_info)->colormap=(Colormap) NULL;
  (*pixel).pixels=(unsigned long *) NULL;
  /*
    Initialize visual info.
  */
  (*resources).map_type=(char *) NULL;
  (*resources).visual_type=visual_type;
  (*visual_info)=XBestVisualInfo(display,(*map_info),resources);
  if ((*visual_info) == (XVisualInfo *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToGetVisual",
      (*resources).visual_type);
  /*
    Initialize window info.
  */
  (*window_info).ximage=(XImage *) NULL;
  (*window_info).matte_image=(XImage *) NULL;
  (*window_info).pixmap=(Pixmap) NULL;
  (*window_info).matte_pixmap=(Pixmap) NULL;
}

static void XCreateBackgroundWindowInfo(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XStandardColormap
    **const map_info = targets->map_info;

  XPixelInfo
    *const pixel = targets->pixel;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  XGCValues
    context_values;

  /*
    Initialize Standard Colormap.
  */
  (*resources).colormap=SharedColormap;
  XMakeStandardColormap(display,(*visual_info),resources,image,(*map_info),
    pixel,exception);
  /*
    Graphic context superclass.
  */
  context_values.background=(*pixel).foreground_color.pixel;
  context_values.foreground=(*pixel).background_color.pixel;
  (*pixel).annotate_context=XCreateGC(display,(*window_info).id,
    (size_t) (GCBackground | GCForeground),&context_values);
  if ((*pixel).annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      image->filename);
  /*
    Initialize Image window attributes.
  */
  (*window_info).name=AcquireString("\0");
  (*window_info).icon_name=AcquireString("\0");
  XGetWindowInfo(display,(*visual_info),(*map_info),pixel,(XFontStruct *) NULL,
    resources,window_info);
}

static void XSizeBackgroundWindow(const Image *image,XWindowInfo *window_info)
{
  (*window_info).width=(unsigned int) image->columns;
  (*window_info).height=(unsigned int) image->rows;
  if ((image->columns != (*window_info).width) ||
      (image->rows != (*window_info).height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image->filename);
}

static void XMakeBackgroundXImage(Display *display,Image *image,
  ExceptionInfo *exception,const BackgroundTargets *targets)
{
  XResourceInfo
    *const resources = targets->resources;

  XWindowInfo
    *const window_info = targets->window_info;

  int
    status;

  status=XMakeImage(display,resources,window_info,image,(*window_info).width,
    (*window_info).height,exception) == MagickFalse ? 0 : 1;
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      image->filename);
  (*window_info).x=0;
  (*window_info).y=0;
}

typedef struct _BackgroundInput
{
  Display
    *display;

  Image
    *image;

  const XWindowAttributes
    *window_attributes;
} BackgroundInput;

static void XAdjustBackgroundSize(const BackgroundInput *input,
  const BackgroundTargets *targets,int *width,int *height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  const XWindowAttributes
    *const window_attributes = input->window_attributes;

  XResourceInfo
    *const resources = targets->resources;

  XVisualInfo
    **const visual_info = targets->visual_info;

  XWindowInfo
    *const window_info = targets->window_info;

  (*width)=(int) (*window_info).width;
  (*height)=(int) (*window_info).height;
  if ((*resources).backdrop != MagickFalse)
    {
      /*
        Center image on window.
      */
      (*window_info).x=((*window_attributes).width/2)-
        ((*window_info).ximage->width/2);
      (*window_info).y=((*window_attributes).height/2)-
        ((*window_info).ximage->height/2);
      (*width)=(*window_attributes).width;
      (*height)=(*window_attributes).height;
    }
  if (((*resources).image_geometry != (char *) NULL) &&
      (*(*resources).image_geometry != '\0'))
    {
      char
        default_geometry[MagickPathExtent];

      int
        flags,
        gravity;

      XSizeHints
        *size_hints;

      /*
        User specified geometry.
      */
      size_hints=XAllocSizeHints();
      if (size_hints == (XSizeHints *) NULL)
        ThrowXWindowFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",image->filename);
      size_hints->flags=0L;
      (void) FormatLocaleString(default_geometry,MagickPathExtent,"%dx%d",
        (*width),(*height));
      flags=XWMGeometry(display,(*visual_info)->screen,
        (*resources).image_geometry,default_geometry,
        (*window_info).border_width,size_hints,&(*window_info).x,
        &(*window_info).y,width,height,&gravity);
      if (flags & (XValue | YValue))
        {
          (*width)=(*window_attributes).width;
          (*height)=(*window_attributes).height;
        }
      (void) XFree((void *) size_hints);
    }
}

static void XDisplayBackgroundPixmap(const BackgroundInput *input,
  const BackgroundTargets *targets,const int width,const int height)
{
  Display
    *const display = input->display;

  Image
    *const image = input->image;

  XWindowInfo
    *const window_info = targets->window_info;

  size_t
    delay;

  /*
    Create the X pixmap.
  */
  (*window_info).pixmap=XCreatePixmap(display,(*window_info).id,
    (unsigned int) width,(unsigned int) height,(*window_info).depth);
  if ((*window_info).pixmap == (Pixmap) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXPixmap",
      image->filename);
  /*
    Display pixmap on the window.
  */
  if (((unsigned int) width > (*window_info).width) ||
      ((unsigned int) height > (*window_info).height))
    (void) XFillRectangle(display,(*window_info).pixmap,
      (*window_info).annotate_context,0,0,(unsigned int) width,
      (unsigned int) height);
  (void) XPutImage(display,(*window_info).pixmap,
    (*window_info).annotate_context,(*window_info).ximage,0,0,
    (*window_info).x,(*window_info).y,(unsigned int) (*window_info).width,
    (unsigned int) (*window_info).height);
  (void) XSetWindowBackgroundPixmap(display,(*window_info).id,
    (*window_info).pixmap);
  (void) XClearWindow(display,(*window_info).id);
  delay=1000*image->delay/(size_t) MagickMax(image->ticks_per_second,1L);
  XDelay(display,delay == 0UL ? 10UL : delay);
  (void) XSync(display,MagickFalse);
}

MagickExport MagickBooleanType XDisplayBackgroundImage(Display *display,
  XResourceInfo *resource_info,Image *image,ExceptionInfo *exception)
{
  char
    visual_type[MagickPathExtent];

  int
    height,
    width;

  static XPixelInfo
    pixel;

  static XStandardColormap
    *map_info;

  static XVisualInfo
    *visual_info = (XVisualInfo *) NULL;

  static XWindowInfo
    window_info;

  Window
    root_window;

  XResourceInfo
    resources;

  BackgroundInput
    input;

  const BackgroundTargets
    targets = { &map_info, &pixel, &resources, &visual_info, &window_info };

  XWindowAttributes
    window_attributes;

  /*
    Determine target window.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  resources=(*resource_info);
  window_info.id=(Window) NULL;
  root_window=XRootWindow(display,XDefaultScreen(display));
  if (XFindBackgroundWindow(display,resources.window_id,root_window,
      &window_info) == MagickFalse)
    return(MagickFalse);
  /*
    Determine window visual id.
  */
  XGetBackgroundVisualType(display,&window_info,&window_attributes,visual_type);
  if (visual_info == (XVisualInfo *) NULL)
    XInitializeBackgroundVisual(display,image,visual_type,&targets);
  /*
    Free previous root colors.
  */
  if (window_info.id == root_window)
    (void) XDestroyWindowColors(display,root_window);
  XCreateBackgroundWindowInfo(display,image,exception,&targets);
  /*
    Create the X image.
  */
  XSizeBackgroundWindow(image,&window_info);
  XFitBackgroundWindow(&window_attributes,&window_info);
  XMakeBackgroundXImage(display,image,exception,&targets);
  if (resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",image->filename,(double) image->scene,
        (double) image->columns,(double) image->rows);
      if (image->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          image->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",image->magick);
    }
  /*
    Adjust image dimensions as specified by backdrop or geometry options.
  */
  input.display=display;
  input.image=image;
  input.window_attributes=(&window_attributes);
  XAdjustBackgroundSize(&input,&targets,&width,&height);
  XDisplayBackgroundPixmap(&input,&targets,width,height);
  return(window_info.id == root_window ? MagickTrue : MagickFalse);
}

static void XPositionCommandAndWidget(Display *display,XWindows *windows,
  const XEvent event)
{
  XWindowChanges
    window_changes;

  if ((windows->command.geometry == (char *) NULL) &&
      (windows->command.mapped == MagickFalse))
    {
      windows->command.x=event.xconfigure.x-(int)
        windows->command.width-25;
      windows->command.y=event.xconfigure.y;
      XConstrainWindowPosition(display,&windows->command);
      window_changes.x=windows->command.x;
      window_changes.y=windows->command.y;
      (void) XReconfigureWMWindow(display,windows->command.id,
        windows->command.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
  if ((windows->widget.geometry == (char *) NULL) &&
      (windows->widget.mapped == MagickFalse))
    {
      windows->widget.x=event.xconfigure.x+
        event.xconfigure.width/10;
      windows->widget.y=event.xconfigure.y+
        event.xconfigure.height/10;
      XConstrainWindowPosition(display,&windows->widget);
      window_changes.x=windows->widget.x;
      window_changes.y=windows->widget.y;
      (void) XReconfigureWMWindow(display,windows->widget.id,
        windows->widget.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
}

static void XPositionMagnifyAndPan(Display *display,XWindows *windows,
  const XEvent event)
{
  XWindowChanges
    window_changes;

  if ((windows->magnify.geometry == (char *) NULL) &&
      (windows->magnify.mapped == MagickFalse))
    {
      windows->magnify.x=event.xconfigure.x+
        event.xconfigure.width+25;
      windows->magnify.y=event.xconfigure.y;
      XConstrainWindowPosition(display,&windows->magnify);
      window_changes.x=windows->magnify.x;
      window_changes.y=windows->magnify.y;
      (void) XReconfigureWMWindow(display,windows->magnify.id,
        windows->magnify.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
  if ((windows->pan.geometry == (char *) NULL) &&
      (windows->pan.mapped == MagickFalse))
    {
      windows->pan.x=event.xconfigure.x+(int)
        event.xconfigure.width+25;
      windows->pan.y=event.xconfigure.y+(int)
        windows->magnify.height+50;
      XConstrainWindowPosition(display,&windows->pan);
      window_changes.x=windows->pan.x;
      window_changes.y=windows->pan.y;
      (void) XReconfigureWMWindow(display,windows->pan.id,
        windows->pan.screen,(unsigned int) (CWX | CWY),
        &window_changes);
    }
}

static void XUpdatePanWindow(Display *display,XWindows *windows,
  const XEvent event)
{
  if ((event.xconfigure.width < windows->image.ximage->width) ||
      (event.xconfigure.height < windows->image.ximage->height))
    {
      (void) XMapRaised(display,windows->pan.id);
      XDrawPanRectangle(display,windows);
    }
  else
    if (windows->pan.mapped != MagickFalse)
      (void) XWithdrawWindow(display,windows->pan.id,
        windows->pan.screen);
}

typedef struct _DisplayEventContext
{
  Display
    *display;

  XResourceInfo
    *resource_info;

  XWindows
    *windows;

  ExceptionInfo
    *exception;

  XPixelInfo
    *pixel;

  XStandardColormap
    *map_info;

  XWindowInfo
    **magick_windows;

  unsigned int
    number_windows;

  MagickStatusType
    context_mask;

  size_t
    *state;

  Window
    root_window;

  const char
    *const *CommandMenu;

  char
    *command;

  XVisualInfo
    *visual_info;

  const char
    *const *VirtualMenu;

  DisplayCommand
    *VirtualCommands;

  const char
    *const *ShortCutsMenu;

  DisplayCommand
    *ShortCutsCommands;

  Image
    **image;

  time_t
    update_time;

  XVisualInfo
    *icon_visual;

  XStandardColormap
    *icon_map;

  XPixelInfo
    *icon_pixel;

  const char *const
    **Menus;

  DisplayCommand
    **Commands;

  XFontStruct
    *font_info;

  char
    **argv;

  int
    argc;

  XClassHint
    *class_hints;

  XWMHints
    *manager_hints;
} DisplayEventContext;

static void XConfigureImageWindow(const DisplayEventContext *context,
  const XEvent event,Image *display_image,const XPoint vid_info)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  /*
    Image window has a new configuration.
  */
  if (event.xconfigure.send_event != 0)
    {
      /*
        Position the transient windows relative of the Image window.
      */
      XPositionCommandAndWidget(display,windows,event);
      XPositionMagnifyAndPan(display,windows,event);
    }
  if ((event.xconfigure.width == (int) windows->image.width) &&
      (event.xconfigure.height == (int) windows->image.height))
    return;
  windows->image.width=(unsigned int) event.xconfigure.width;
  windows->image.height=(unsigned int) event.xconfigure.height;
  windows->image.x=0;
  windows->image.y=0;
  if (display_image->montage != (char *) NULL)
    {
      windows->image.x=vid_info.x;
      windows->image.y=vid_info.y;
    }
  if (windows->image.mapped != MagickFalse &&
      windows->image.stasis != MagickFalse)
    {
      /*
        Update image window configuration.
      */
      windows->image.window_changes.width=event.xconfigure.width;
      windows->image.window_changes.height=event.xconfigure.height;
      (void) XConfigureImage(display,resource_info,windows,
        display_image,exception);
    }
  /*
    Update pan window configuration.
  */
  XUpdatePanWindow(display,windows,event);
}

static MagickStatusType XConfigureMagnifyWindow(
  const DisplayEventContext *context,const XEvent event,Image *display_image,
  MagickStatusType status)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  unsigned int
    magnify;

  XWindowChanges
    window_changes;

  /*
    Magnify window has a new configuration.
  */
  windows->magnify.width=(unsigned int) event.xconfigure.width;
  windows->magnify.height=(unsigned int) event.xconfigure.height;
  if (windows->magnify.mapped == MagickFalse)
    return(status);
  magnify=1;
  while ((int) magnify <= event.xconfigure.width)
    magnify<<=1;
  while ((int) magnify <= event.xconfigure.height)
    magnify<<=1;
  magnify>>=1;
  if (((int) magnify != event.xconfigure.width) ||
      ((int) magnify != event.xconfigure.height))
    {
      window_changes.width=(int) magnify;
      window_changes.height=(int) magnify;
      (void) XReconfigureWMWindow(display,windows->magnify.id,
        windows->magnify.screen,(unsigned int) (CWWidth | CWHeight),
        &window_changes);
      return(status);
    }
  if (windows->magnify.mapped != MagickFalse &&
      windows->magnify.stasis != MagickFalse)
    {
      status=XMakeImage(display,resource_info,&windows->magnify,
        display_image,windows->magnify.width,windows->magnify.height,
        exception);
      XMakeMagnifyImage(display,windows,exception);
    }
  return(status);
}

static void XConfigurePanWindow(XWindows *windows,const XEvent event)
{
  /*
    Pan icon window has a new configuration.
  */
  if (event.xconfigure.send_event != 0)
    {
      windows->pan.x=event.xconfigure.x;
      windows->pan.y=event.xconfigure.y;
    }
  windows->pan.width=(unsigned int) event.xconfigure.width;
  windows->pan.height=(unsigned int) event.xconfigure.height;
}

static void XUpdateWindowColormaps(const DisplayEventContext *context,
  XGCValues *context_values)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  XWindowInfo
    **const magick_windows = context->magick_windows;

  const unsigned int
    number_windows = context->number_windows;

  const MagickStatusType
    context_mask = context->context_mask;

  int
    i;

  /*
    Update graphic context and window colormap.
  */
  for (i=0; i < (int) number_windows; i++)
  {
    if (magick_windows[i]->id == windows->icon.id)
      continue;
    (*context_values).background=pixel->background_color.pixel;
    (*context_values).foreground=pixel->foreground_color.pixel;
    (void) XChangeGC(display,magick_windows[i]->annotate_context,
      context_mask,context_values);
    (void) XChangeGC(display,magick_windows[i]->widget_context,
      context_mask,context_values);
    (*context_values).background=pixel->foreground_color.pixel;
    (*context_values).foreground=pixel->background_color.pixel;
    (*context_values).plane_mask=(*context_values).background ^
      (*context_values).foreground;
    (void) XChangeGC(display,magick_windows[i]->highlight_context,
      (size_t) (context_mask | GCPlaneMask),
      context_values);
    magick_windows[i]->attributes.background_pixel=
      pixel->background_color.pixel;
    magick_windows[i]->attributes.border_pixel=
      pixel->border_color.pixel;
    magick_windows[i]->attributes.colormap=map_info->colormap;
    (void) XChangeWindowAttributes(display,magick_windows[i]->id,
      (unsigned long) magick_windows[i]->mask,
      &magick_windows[i]->attributes);
  }
  if (windows->pan.mapped != MagickFalse)
    {
      (void) XSetWindowBackgroundPixmap(display,windows->pan.id,
        windows->pan.pixmap);
      (void) XClearWindow(display,windows->pan.id);
      XDrawPanRectangle(display,windows);
    }
  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
}

static Image *XDropImage(const DisplayEventContext *context,const XEvent event,
  Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  const Window
    root_window = context->root_window;

  Atom
    selection,
    type;

  int
    format,
    status;

  unsigned char
    *data;

  unsigned long
    after,
    length;

  /*
    Display image named by the Drag-and-Drop selection.
  */
  if ((*event.xclient.data.l != 2) && (*event.xclient.data.l != 128))
    return(nexus);
  selection=XInternAtom(display,"DndSelection",MagickFalse);
  status=XGetWindowProperty(display,root_window,selection,0L,(long)
    MagickPathExtent,MagickFalse,(Atom) AnyPropertyType,&type,&format,
    &length,&after,&data);
  if ((status != Success) || (length == 0))
    return(nexus);
  if (*event.xclient.data.l == 2)
    {
      /*
        Offix DND.
      */
      (void) CopyMagickString(resource_info->image_info->filename,
        (char *) data,MagickPathExtent);
    }
  else
    {
      /*
        XDND.
      */
      if (strncmp((char *) data, "file:", 5) != 0)
        {
          (void) XFree((void *) data);
          return(nexus);
        }
      (void) CopyMagickString(resource_info->image_info->filename,
        ((char *) data)+5,MagickPathExtent);
    }
  nexus=ReadImage(resource_info->image_info,exception);
  CatchException(exception);
  if (nexus != (Image *) NULL)
    *state|=NextImageState | ExitState;
  (void) XFree((void *) data);
  return(nexus);
}

typedef struct _DisplayEventTargets
{
  Image
    **nexus;

  Image
    **display_image;

  XEvent
    *event;

  XPoint
    *vid_info;
} DisplayEventTargets;

static void XSelectVirtualCommand(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const command = context->command;

  const char
    *const *const VirtualMenu = context->VirtualMenu;

  DisplayCommand
    *const VirtualCommands = context->VirtualCommands;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  int
    entry;

  /*
    Select a command from the Virtual menu.
  */
  entry=XMenuWidget(display,windows,"Commands",VirtualMenu,
    command);
  if (entry >= 0)
    (*nexus)=XMagickCommand(display,resource_info,windows,
      VirtualCommands[entry],display_image,exception);
}

static void XPressImageButton1(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const char
    *const *const CommandMenu = context->CommandMenu;

  if (resource_info->immutable)
    {
      XSelectVirtualCommand(context,targets);
      return;
    }
  /*
    Map/unmap Command widget.
  */
  if (windows->command.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->command.id,
      windows->command.screen);
  else
    {
      (void) XCommandWidget(display,windows,CommandMenu,
        (XEvent *) NULL);
      (void) XMapRaised(display,windows->command.id);
    }
}

static void XPressImageButton3(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  char
    *const command = context->command;

  const char
    *const *const ShortCutsMenu = context->ShortCutsMenu;

  DisplayCommand
    *const ShortCutsCommands = context->ShortCutsCommands;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  XPoint
    *const vid_info = targets->vid_info;

  int
    entry;

  if (resource_info->immutable)
    {
      XSelectVirtualCommand(context,targets);
      return;
    }
  if ((*display_image)->montage != (char *) NULL)
    {
      /*
        Open or delete a tile from a visual image directory.
      */
      (*nexus)=XTileImage(display,resource_info,windows,
        (*display_image),event,exception);
      if ((*nexus) != (Image *) NULL)
        *state|=MontageImageState | NextImageState | ExitState;
      (*vid_info).x=(short int) windows->image.x;
      (*vid_info).y=(short int) windows->image.y;
      return;
    }
  /*
    Select a command from the Short Cuts menu.
  */
  entry=XMenuWidget(display,windows,"Short Cuts",ShortCutsMenu,
    command);
  if (entry >= 0)
    (*nexus)=XMagickCommand(display,resource_info,windows,
      ShortCutsCommands[entry],display_image,exception);
}

static void XPressImageWindow(const DisplayEventContext *context,
  const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  Image
    **const display_image = targets->display_image;

  XEvent
    *const event = targets->event;

  switch ((*event).xbutton.button)
  {
    case Button1:
    {
      XPressImageButton1(context,targets);
      break;
    }
    case Button2:
    {
      /*
        User pressed the image magnify button.
      */
      (void) XMagickCommand(display,resource_info,windows,ZoomCommand,
        display_image,exception);
      XMagnifyImage(display,windows,event,exception);
      break;
    }
    case Button3:
    {
      XPressImageButton3(context,targets);
      break;
    }
    case Button4:
    {
      /*
        Wheel up.
      */
      XTranslateImage(display,windows,*image,XK_Up);
      break;
    }
    case Button5:
    {
      /*
        Wheel down.
      */
      XTranslateImage(display,windows,*image,XK_Down);
      break;
    }
    default:
      break;
  }
}

static void XPressMagnifyWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  char
    *const command = context->command;

  const char
    *const MagnifyMenu[] =
    {
      "2",
      "4",
      "5",
      "6",
      "7",
      "8",
      "9",
      "3",
      (char *) NULL,
    };

  int
    factor;

  static KeySym
    MagnifyCommands[] =
    {
      XK_2,
      XK_4,
      XK_5,
      XK_6,
      XK_7,
      XK_8,
      XK_9,
      XK_3
    };

  /*
    Select a magnify factor from the pop-up menu.
  */
  factor=XMenuWidget(display,windows,"Magnify",MagnifyMenu,command);
  if (factor >= 0)
    XMagnifyWindowCommand(display,windows,0,MagnifyCommands[factor],
      exception);
}

static void XPressPanWindow(const DisplayEventContext *context,XEvent *event)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const image = context->image;

  switch ((*event).xbutton.button)
  {
    case Button4:
    {
      /*
        Wheel up.
      */
      XTranslateImage(display,windows,*image,XK_Up);
      break;
    }
    case Button5:
    {
      /*
        Wheel down.
      */
      XTranslateImage(display,windows,*image,XK_Down);
      break;
    }
    default:
    {
      XPanImage(display,windows,event,exception);
      break;
    }
  }
}

static void XImageWindowKeyPress(const DisplayEventContext *context,
  const XEvent event,const KeySym key_symbol,const DisplayEventTargets *targets)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  Image
    **const nexus = targets->nexus;

  Image
    **const display_image = targets->display_image;

  DisplayCommand
    display_command;

  display_command=XImageWindowCommand(display,resource_info,windows,
    event.xkey.state,key_symbol,display_image,exception);
  if (display_command != NullCommand)
    (*nexus)=XMagickCommand(display,resource_info,windows,
      display_command,display_image,exception);
}

static void XPanWindowKeyPress(const DisplayEventContext *context,
  const KeySym key_symbol)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  Image
    **const image = context->image;

  if ((key_symbol == XK_q) || (key_symbol == XK_Escape))
    (void) XWithdrawWindow(display,windows->pan.id,
      windows->pan.screen);
  else
    if ((key_symbol == XK_F1) || (key_symbol == XK_Help))
      XTextViewHelp(display,resource_info,windows,MagickFalse,
        "Help Viewer - Image Pan",ImagePanHelp);
    else
      XTranslateImage(display,windows,*image,key_symbol);
}

static Image *XMapImageWindow(const DisplayEventContext *context,
  Image *display_image,Image *nexus)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XStandardColormap
    *const map_info = context->map_info;

  if (windows->backdrop.id != (Window) NULL)
    (void) XInstallColormap(display,map_info->colormap);
  if ((LocaleCompare(display_image->magick,"LOGO") == 0) &&
      (LocaleCompare(display_image->filename,"LOGO") == 0))
    nexus=XOpenImage(display,resource_info,windows,MagickFalse);
  if (((int) windows->image.width < windows->image.ximage->width) ||
      ((int) windows->image.height < windows->image.ximage->height))
    (void) XMapRaised(display,windows->pan.id);
  windows->image.mapped=MagickTrue;
  return(nexus);
}

static void XFocusImageWindow(Display *display,XWindows *windows)
{
  if (windows->backdrop.id != (Window) NULL)
    (void) XSetInputFocus(display,windows->image.id,RevertToParent,
      CurrentTime);
}

static void XUnmapIconWindow(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XStandardColormap
    *const map_info = context->map_info;

  XVisualInfo
    *const icon_visual = context->icon_visual;

  XStandardColormap
    *const icon_map = context->icon_map;

  XPixelInfo
    *const icon_pixel = context->icon_pixel;

  if (map_info->colormap == icon_map->colormap)
    XConfigureImageColormap(display,resource_info,windows,
      display_image,exception);
  (void) XFreeStandardColormap(display,icon_visual,icon_map,
    icon_pixel);
  windows->icon.mapped=MagickFalse;
}

static void XLogDisplayImage(XResourceInfo *resource_info,Image *display_image)
{
  if (resource_info->debug != MagickFalse)
    {
      (void) LogMagickEvent(X11Event,GetMagickModule(),
        "Image: %s[%.17g] %.17gx%.17g ",display_image->filename,
        (double) display_image->scene,(double) display_image->columns,
        (double) display_image->rows);
      if (display_image->colors != 0)
        (void) LogMagickEvent(X11Event,GetMagickModule(),"%.17gc ",(double)
          display_image->colors);
      (void) LogMagickEvent(X11Event,GetMagickModule(),"%s",
        display_image->magick);
    }
}

static time_t XGetUpdateTime(XResourceInfo *resource_info,Image *display_image)
{
  struct stat
    attributes;

  time_t
    update_time;

  update_time=0;
  if (resource_info->update != MagickFalse)
    {
      MagickBooleanType
        status;

      /*
        Determine when file data was last modified.
      */
      status=GetPathAttributes(display_image->filename,&attributes);
      if (status != MagickFalse)
        update_time=attributes.st_mtime;
    }
  return(update_time);
}

static void XNameImageWindow(const DisplayEventContext *context,
  Image *display_image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  size_t
    *const state = context->state;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,resource_info,
    &windows->image);
  windows->image.shape=MagickTrue;  /* non-rectangular shape hint */
  if (resource_info->use_shared_memory == MagickFalse)
    windows->image.shared_memory=MagickFalse;
  if ((resource_info->title != (char *) NULL) && !(*state & MontageImageState))
    {
      char
        *title;

      title=InterpretImageProperties(resource_info->image_info,display_image,
        resource_info->title,exception);
      (void) CloneString(&windows->image.name,title);
      (void) CloneString(&windows->image.icon_name,title);
      title=DestroyString(title);
    }
  else
    {
      char
        filename[MagickPathExtent],
        window_name[MagickPathExtent];

      /*
        Window name is the base of the filename.
      */
      GetPathComponent(display_image->magick_filename,TailPath,filename);
      if (display_image->scene == 0)
        (void) FormatLocaleString(window_name,MagickPathExtent,"%s: %s",
          MagickPackageName,filename);
      else
        (void) FormatLocaleString(window_name,MagickPathExtent,
          "%s: %s[scene: %.17g frames: %.17g]",MagickPackageName,filename,
          (double) display_image->scene,(double) GetImageListLength(
          display_image));
      (void) CloneString(&windows->image.name,window_name);
      (void) CloneString(&windows->image.icon_name,filename);
    }
}

static void XInitializeBackdropWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  XPixelInfo
    *const pixel = context->pixel;

  XStandardColormap
    *const map_info = context->map_info;

  const Window
    root_window = context->root_window;

  XVisualInfo
    *const visual_info = context->visual_info;

  XFontStruct
    *const font_info = context->font_info;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->backdrop);
  if ((resource_info->backdrop) || (windows->backdrop.id != (Window) NULL))
    {
      /*
        Initialize backdrop window.
      */
      windows->backdrop.x=0;
      windows->backdrop.y=0;
      (void) CloneString(&windows->backdrop.name,"Backdrop");
      windows->backdrop.flags=(size_t) (USSize | USPosition);
      windows->backdrop.width=(unsigned int)
        XDisplayWidth(display,visual_info->screen);
      windows->backdrop.height=(unsigned int)
        XDisplayHeight(display,visual_info->screen);
      windows->backdrop.border_width=0;
      windows->backdrop.immutable=MagickTrue;
      windows->backdrop.attributes.do_not_propagate_mask=ButtonPressMask |
        ButtonReleaseMask;
      windows->backdrop.attributes.event_mask=ButtonPressMask | KeyPressMask |
        StructureNotifyMask;
      manager_hints->flags=IconWindowHint | InputHint | StateHint;
      manager_hints->icon_window=windows->icon.id;
      manager_hints->input=MagickTrue;
      manager_hints->initial_state=resource_info->iconic ? IconicState :
        NormalState;
      XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
        &windows->backdrop);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (backdrop)",windows->backdrop.id);
      (void) XMapWindow(display,windows->backdrop.id);
      (void) XClearWindow(display,windows->backdrop.id);
      if (windows->image.id != (Window) NULL)
        {
          (void) XDestroyWindow(display,windows->image.id);
          windows->image.id=(Window) NULL;
        }
      /*
        Position image in the center the backdrop.
      */
      windows->image.flags|=USPosition;
      windows->image.x=(XDisplayWidth(display,visual_info->screen)/2)-
        ((int) windows->image.width/2);
      windows->image.y=(XDisplayHeight(display,visual_info->screen)/2)-
        ((int) windows->image.height/2);
    }
}

static void XMakeImageWindow(const DisplayEventContext *context)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const Window
    root_window = context->root_window;

  char
    **const argv = context->argv;

  const int
    argc = context->argc;

  XClassHint
    *const class_hints = context->class_hints;

  XWMHints
    *const manager_hints = context->manager_hints;

  manager_hints->flags=IconWindowHint | InputHint | StateHint;
  manager_hints->icon_window=windows->icon.id;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=resource_info->iconic ? IconicState :
    NormalState;
  if (windows->group_leader.id != (Window) NULL)
    {
      /*
        Follow the leader.
      */
      manager_hints->flags|=WindowGroupHint;
      manager_hints->window_group=windows->group_leader.id;
      (void) XSelectInput(display,windows->group_leader.id,StructureNotifyMask);
      if (resource_info->debug != MagickFalse)
        (void) LogMagickEvent(X11Event,GetMagickModule(),
          "Window id: 0x%lx (group leader)",windows->group_leader.id);
    }
  XMakeWindow(display,
    (Window) (resource_info->backdrop ? windows->backdrop.id : root_window),
    argv,argc,class_hints,manager_hints,&windows->image);
  (void) XChangeProperty(display,windows->image.id,windows->im_protocols,
    XA_STRING,8,PropModeReplace,(unsigned char *) NULL,0);
  if (windows->group_leader.id != (Window) NULL)
    (void) XSetTransientForHint(display,windows->image.id,
      windows->group_leader.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (image)",
      windows->image.id);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X D i s p l a y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XDisplayImage() displays an image via X11.  A new image is created and
%  returned if the user interactively transforms the displayed image.
%
%  The format of the XDisplayImage method is:
%
%      Image *XDisplayImage(Display *display,XResourceInfo *resource_info,
%        char **argv,int argc,Image **image,size_t *state,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o nexus:  Method XDisplayImage returns an image when the
%      user chooses 'Open Image' from the command menu or picks a tile
%      from the image directory.  Otherwise a null image is returned.
%
%    o display: Specifies a connection to an X server;  returned from
%      XOpenDisplay.
%
%    o resource_info: Specifies a pointer to a X11 XResourceInfo structure.
%
%    o argv: Specifies the application's argument list.
%
%    o argc: Specifies the number of arguments.
%
%    o image: Specifies an address to an address of an Image structure;
%
%    o exception: return any errors or warnings in this structure.
%
*/
static inline MagickBooleanType IsDynamicVisualClass(const int v)
{
  switch (v)
  {
    case GrayScale:
    case PseudoColor:
    case DirectColor:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

static Image *XReloadModifiedImage(XResourceInfo *resource_info,
  const Image *display_image,size_t *state,ExceptionInfo *exception)
{
  Image
    *nexus;

  (void) FormatLocaleString(
    resource_info->image_info->filename,MagickPathExtent,
    "%s:%s",display_image->magick,
    display_image->filename);
  nexus=ReadImage(resource_info->image_info,exception);
  if (nexus != (Image *) NULL)
    *state|=NextImageState | ExitState;
  return(nexus);
}

#define MagnifySize  256  /* must be a power of 2 */
#define MagickMenus  10
#define MagickTitle  "Commands"

static void XHandleProtocolMessage(const DisplayEventContext *context,
  const XEvent event,XGCValues *context_values)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  size_t
    *const state = context->state;

  const char
    *const *const CommandMenu = context->CommandMenu;

  if (*event.xclient.data.l == (long) windows->im_update_widget)
    {
      (void) CloneString(&windows->command.name,MagickTitle);
      windows->command.data=MagickMenus;
      (void) XCommandWidget(display,windows,CommandMenu,
        (XEvent *) NULL);
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_update_colormap)
    {
      XUpdateWindowColormaps(context,context_values);
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_former_image)
    {
      *state|=FormerImageState | ExitState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_next_image)
    {
      *state|=NextImageState | ExitState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_retain_colors)
    {
      *state|=RetainColorsState;
      return;
    }
  if (*event.xclient.data.l == (long) windows->im_exit)
    {
      *state|=ExitState;
      return;
    }
}

static void XReloadModifiedDisplay(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  ExceptionInfo
    *const exception = context->exception;

  size_t
    *const state = context->state;

  const time_t
    update_time = context->update_time;

  MagickBooleanType
    status;

  struct stat
    attributes;

  size_t
    delay;

  /*
    Determine if image file was modified.
  */
  status=GetPathAttributes(display_image->filename,&attributes);
  if ((status != MagickFalse) &&
      (update_time != attributes.st_mtime))
    {
      /*
        Redisplay image.
      */
      (*nexus)=XReloadModifiedImage(resource_info,display_image,
        state,exception);
    }
  delay=display_image->delay/(size_t) MagickMax(
    display_image->ticks_per_second,1L);
  (*timer)=GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1;
}

static void XUpdateImageTimer(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  XResourceInfo
    *const resource_info = context->resource_info;

  size_t
    *const state = context->state;

  if ((*timer) < GetMagickTime())
    {
      if (resource_info->update == MagickFalse)
        *state|=NextImageState | ExitState;
      else
        {
          XReloadModifiedDisplay(context,display_image,nexus,timer);
        }
    }
}

static MagickBooleanType XCheckImageTimer(const DisplayEventContext *context,
  Image *display_image,Image **nexus,time_t *timer)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  if ((display_image->delay != 0) || (resource_info->update != 0))
    {
      XUpdateImageTimer(context,display_image,nexus,timer);
      if (XEventsQueued(display,QueuedAfterFlush) == 0)
        {
          /*
            Do not block if delay > 0.
          */
          XDelay(display,SuspendTime << 2);
          return(MagickTrue);
        }
    }
  return(MagickFalse);
}

static MagickBooleanType XSelectCommandMenuEntry(
  const DisplayEventContext *context,const int id,
  DisplayCommand *display_command)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const char
    *const *const CommandMenu = context->CommandMenu;

  char
    *const command = context->command;

  const char *const
    **const Menus = context->Menus;

  DisplayCommand
    **const Commands = context->Commands;

  int
    entry;

  if (id < MagickMenus)
    {
      /*
        Select a command from a pop-up menu.
      */
      entry=XMenuWidget(display,windows,CommandMenu[id],Menus[id],
        command);
      if (entry < 0)
        return(MagickTrue);
      (void) CopyMagickString(command,Menus[id][entry],MagickPathExtent);
      (*display_command)=Commands[id][entry];
    }
  return(MagickFalse);
}

static const char
  *const CommandMenu[] =
  {
    "File",
    "Edit",
    "View",
    "Transform",
    "Enhance",
    "Effects",
    "F/X",
    "Image Edit",
    "Miscellany",
    "Help",
    (char *) NULL
  },
  *const FileMenu[] =
  {
    "Open...",
    "Next",
    "Former",
    "Select...",
    "Save...",
    "Print...",
    "Delete...",
    "New...",
    "Visual Directory...",
    "Quit",
    (char *) NULL
  },
  *const EditMenu[] =
  {
    "Undo",
    "Redo",
    "Cut",
    "Copy",
    "Paste",
    (char *) NULL
  },
  *const ViewMenu[] =
  {
    "Half Size",
    "Original Size",
    "Double Size",
    "Resize...",
    "Apply",
    "Refresh",
    "Restore",
    (char *) NULL
  },
  *const TransformMenu[] =
  {
    "Crop",
    "Chop",
    "Flop",
    "Flip",
    "Rotate Right",
    "Rotate Left",
    "Rotate...",
    "Shear...",
    "Roll...",
    "Trim Edges",
    (char *) NULL
  },
  *const EnhanceMenu[] =
  {
    "Hue...",
    "Saturation...",
    "Brightness...",
    "Gamma...",
    "Spiff",
    "Dull",
    "Contrast Stretch...",
    "Sigmoidal Contrast...",
    "Normalize",
    "Equalize",
    "Negate",
    "Grayscale",
    "Map...",
    "Quantize...",
    (char *) NULL
  },
  *const EffectsMenu[] =
  {
    "Despeckle",
    "Emboss",
    "Reduce Noise",
    "Add Noise...",
    "Sharpen...",
    "Blur...",
    "Threshold...",
    "Edge Detect...",
    "Spread...",
    "Shade...",
    "Raise...",
    "Segment...",
    (char *) NULL
  },
  *const FXMenu[] =
  {
    "Solarize...",
    "Sepia Tone...",
    "Swirl...",
    "Implode...",
    "Vignette...",
    "Wave...",
    "Oil Paint...",
    "Charcoal Draw...",
    (char *) NULL
  },
  *const ImageEditMenu[] =
  {
    "Annotate...",
    "Draw...",
    "Color...",
    "Matte...",
    "Composite...",
    "Add Border...",
    "Add Frame...",
    "Comment...",
    "Launch...",
    "Region of Interest...",
    (char *) NULL
  },
  *const MiscellanyMenu[] =
  {
    "Image Info",
    "Zoom Image",
    "Show Preview...",
    "Show Histogram",
    "Show Matte",
    "Background...",
    "Slide Show...",
    "Preferences...",
    (char *) NULL
  },
  *const HelpMenu[] =
  {
    "Overview",
    "Browse Documentation",
    "About Display",
    (char *) NULL
  },
  *const ShortCutsMenu[] =
  {
    "Next",
    "Former",
    "Open...",
    "Save...",
    "Print...",
    "Undo",
    "Restore",
    "Image Info",
    "Quit",
    (char *) NULL
  },
  *const VirtualMenu[] =
  {
    "Image Info",
    "Print",
    "Next",
    "Quit",
    (char *) NULL
  };

static const char
  *const *Menus[MagickMenus] =
  {
    FileMenu,
    EditMenu,
    ViewMenu,
    TransformMenu,
    EnhanceMenu,
    EffectsMenu,
    FXMenu,
    ImageEditMenu,
    MiscellanyMenu,
    HelpMenu
  };

static DisplayCommand
  CommandMenus[] =
  {
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
    NullCommand,
  },
  FileCommands[] =
  {
    OpenCommand,
    NextCommand,
    FormerCommand,
    SelectCommand,
    SaveCommand,
    PrintCommand,
    DeleteCommand,
    NewCommand,
    VisualDirectoryCommand,
    QuitCommand
  },
  EditCommands[] =
  {
    UndoCommand,
    RedoCommand,
    CutCommand,
    CopyCommand,
    PasteCommand
  },
  ViewCommands[] =
  {
    HalfSizeCommand,
    OriginalSizeCommand,
    DoubleSizeCommand,
    ResizeCommand,
    ApplyCommand,
    RefreshCommand,
    RestoreCommand
  },
  TransformCommands[] =
  {
    CropCommand,
    ChopCommand,
    FlopCommand,
    FlipCommand,
    RotateRightCommand,
    RotateLeftCommand,
    RotateCommand,
    ShearCommand,
    RollCommand,
    TrimCommand
  },
  EnhanceCommands[] =
  {
    HueCommand,
    SaturationCommand,
    BrightnessCommand,
    GammaCommand,
    SpiffCommand,
    DullCommand,
    ContrastStretchCommand,
    SigmoidalContrastCommand,
    NormalizeCommand,
    EqualizeCommand,
    NegateCommand,
    GrayscaleCommand,
    MapCommand,
    QuantizeCommand
  },
  EffectsCommands[] =
  {
    DespeckleCommand,
    EmbossCommand,
    ReduceNoiseCommand,
    AddNoiseCommand,
    SharpenCommand,
    BlurCommand,
    ThresholdCommand,
    EdgeDetectCommand,
    SpreadCommand,
    ShadeCommand,
    RaiseCommand,
    SegmentCommand
  },
  FXCommands[] =
  {
    SolarizeCommand,
    SepiaToneCommand,
    SwirlCommand,
    ImplodeCommand,
    VignetteCommand,
    WaveCommand,
    OilPaintCommand,
    CharcoalDrawCommand
  },
  ImageEditCommands[] =
  {
    AnnotateCommand,
    DrawCommand,
    ColorCommand,
    MatteCommand,
    CompositeCommand,
    AddBorderCommand,
    AddFrameCommand,
    CommentCommand,
    LaunchCommand,
    RegionOfInterestCommand
  },
  MiscellanyCommands[] =
  {
    InfoCommand,
    ZoomCommand,
    ShowPreviewCommand,
    ShowHistogramCommand,
    ShowMatteCommand,
    BackgroundCommand,
    SlideShowCommand,
    PreferencesCommand
  },
  HelpCommands[] =
  {
    HelpCommand,
    BrowseDocumentationCommand,
    VersionCommand
  },
  ShortCutsCommands[] =
  {
    NextCommand,
    FormerCommand,
    OpenCommand,
    SaveCommand,
    PrintCommand,
    UndoCommand,
    RestoreCommand,
    InfoCommand,
    QuitCommand
  },
  VirtualCommands[] =
  {
    InfoCommand,
    PrintCommand,
    NextCommand,
    QuitCommand
  };

static DisplayCommand
  *Commands[MagickMenus] =
  {
    FileCommands,
    EditCommands,
    ViewCommands,
    TransformCommands,
    EnhanceCommands,
    EffectsCommands,
    FXCommands,
    ImageEditCommands,
    MiscellanyCommands,
    HelpCommands
  };

MagickExport Image *XDisplayImage(Display *display,XResourceInfo *resource_info,
  char **argv,int argc,Image **image,size_t *state,ExceptionInfo *exception)
{
  char
    command[MagickPathExtent],
    *directory,
    geometry[MagickPathExtent],
    resource_name[MagickPathExtent];

  DisplayCommand
    display_command;

  Image
    *display_image,
    *nexus;

  int
    id;

  KeySym
    key_symbol;

  MagickStatusType
    context_mask,
    status;

  RectangleInfo
    geometry_info;

  int
    i;

  static char
    working_directory[MagickPathExtent];

  static XPoint
    vid_info;

  static XWindowInfo
    *magick_windows[MaxXWindows];

  static unsigned int
    number_windows;

  time_t
    timer,
    timestamp,
    update_time;

  unsigned int
    height,
    width;

  size_t
    delay;

  WarningHandler
    warning_handler;

  Window
    root_window;

  XClassHint
    *class_hints;

  XEvent
    event;

  XFontStruct
    *font_info;

  XGCValues
    context_values;

  XPixelInfo
    *icon_pixel,
    *pixel;

  XResourceInfo
    *icon_resources;

  XStandardColormap
    *icon_map,
    *map_info;

  XVisualInfo
    *icon_visual,
    *visual_info;

  XWindows
    *windows;

  XWMHints
    *manager_hints;

  DisplayEventContext
    event_context;

  const DisplayEventTargets
    event_targets = { &nexus, &display_image, &event, &vid_info };

  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickCoreSignature);
  if (IsEventLogging() != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
  display_image=(*image);
  warning_handler=(WarningHandler) NULL;
  windows=XSetWindows((XWindows *) ~0);
  if (windows != (XWindows *) NULL)
    {
      int
        status;

      if (*working_directory == '\0')
        (void) CopyMagickString(working_directory,".",MagickPathExtent);
      status=chdir(working_directory);
      if (status == -1)
        (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
          "UnableToOpenFile","%s",working_directory);
      warning_handler=resource_info->display_warnings ?
        SetErrorHandler(XWarning) : SetErrorHandler((ErrorHandler) NULL);
      warning_handler=resource_info->display_warnings ?
        SetWarningHandler(XWarning) : SetWarningHandler((WarningHandler) NULL);
    }
  else
    {
      /*
        Allocate windows structure.
      */
      resource_info->colors=display_image->colors;
      windows=XSetWindows(XInitializeWindows(display,resource_info));
      if (windows == (XWindows *) NULL)
        ThrowXWindowFatalException(XServerFatalError,"UnableToCreateWindow",
          (*image)->filename);
      /*
        Initialize window id's.
      */
      number_windows=0;
      magick_windows[number_windows++]=(&windows->icon);
      magick_windows[number_windows++]=(&windows->backdrop);
      magick_windows[number_windows++]=(&windows->image);
      magick_windows[number_windows++]=(&windows->info);
      magick_windows[number_windows++]=(&windows->command);
      magick_windows[number_windows++]=(&windows->widget);
      magick_windows[number_windows++]=(&windows->popup);
      magick_windows[number_windows++]=(&windows->magnify);
      magick_windows[number_windows++]=(&windows->pan);
      for (i=0; i < (int) number_windows; i++)
        magick_windows[i]->id=(Window) NULL;
      vid_info.x=0;
      vid_info.y=0;
    }
  /*
    Initialize font info.
  */
  if (windows->font_info != (XFontStruct *) NULL)
    (void) XFreeFont(display,windows->font_info);
  windows->font_info=XBestFont(display,resource_info,MagickFalse);
  if (windows->font_info == (XFontStruct *) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToLoadFont",
      resource_info->font);
  /*
    Initialize Standard Colormap.
  */
  map_info=windows->map_info;
  icon_map=windows->icon_map;
  visual_info=windows->visual_info;
  icon_visual=windows->icon_visual;
  pixel=windows->pixel_info;
  icon_pixel=windows->icon_pixel;
  font_info=windows->font_info;
  icon_resources=windows->icon_resources;
  class_hints=windows->class_hints;
  manager_hints=windows->manager_hints;
  root_window=XRootWindow(display,visual_info->screen);
  event_context.display=display;
  event_context.resource_info=resource_info;
  event_context.windows=windows;
  event_context.exception=exception;
  event_context.pixel=pixel;
  event_context.map_info=map_info;
  event_context.magick_windows=magick_windows;
  event_context.number_windows=number_windows;
  event_context.state=state;
  event_context.root_window=root_window;
  event_context.CommandMenu=CommandMenu;
  event_context.command=command;
  event_context.visual_info=visual_info;
  event_context.VirtualMenu=VirtualMenu;
  event_context.VirtualCommands=VirtualCommands;
  event_context.ShortCutsMenu=ShortCutsMenu;
  event_context.ShortCutsCommands=ShortCutsCommands;
  event_context.image=image;
  event_context.icon_visual=icon_visual;
  event_context.icon_map=icon_map;
  event_context.icon_pixel=icon_pixel;
  event_context.Menus=Menus;
  event_context.Commands=Commands;
  event_context.font_info=font_info;
  event_context.argv=argv;
  event_context.argc=argc;
  event_context.class_hints=class_hints;
  event_context.manager_hints=manager_hints;
  nexus=NewImageList();
  XLogDisplayImage(resource_info,display_image);
  XMakeStandardColormap(display,visual_info,resource_info,display_image,
    map_info,pixel,exception);
  display_image->taint=MagickFalse;
  /*
    Initialize graphic context.
  */
  windows->context.id=(Window) NULL;
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->context);
  (void) CloneString(&class_hints->res_name,resource_info->client_name);
  (void) CloneString(&class_hints->res_class,resource_info->client_name);
  class_hints->res_class[0]=(char) LocaleToUppercase((int)
    class_hints->res_class[0]);
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=WithdrawnState;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->context);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (context)",windows->context.id);
  context_values.background=pixel->background_color.pixel;
  context_values.font=font_info->fid;
  context_values.foreground=pixel->foreground_color.pixel;
  context_values.graphics_exposures=MagickFalse;
  context_mask=(MagickStatusType)
    (GCBackground | GCFont | GCForeground | GCGraphicsExposures);
  if (pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,pixel->annotate_context);
  pixel->annotate_context=XCreateGC(display,windows->context.id,
    context_mask,&context_values);
  if (pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  context_values.background=pixel->depth_color.pixel;
  if (pixel->widget_context != (GC) NULL)
    (void) XFreeGC(display,pixel->widget_context);
  pixel->widget_context=XCreateGC(display,windows->context.id,context_mask,
    &context_values);
  if (pixel->widget_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  context_values.background=pixel->foreground_color.pixel;
  context_values.foreground=pixel->background_color.pixel;
  context_values.plane_mask=context_values.background ^
    context_values.foreground;
  if (pixel->highlight_context != (GC) NULL)
    (void) XFreeGC(display,pixel->highlight_context);
  pixel->highlight_context=XCreateGC(display,windows->context.id,
    (size_t) (context_mask | GCPlaneMask),&context_values);
  if (pixel->highlight_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  (void) XDestroyWindow(display,windows->context.id);
  /*
    Initialize icon window.
  */
  XGetWindowInfo(display,icon_visual,icon_map,icon_pixel,(XFontStruct *) NULL,
    icon_resources,&windows->icon);
  windows->icon.geometry=resource_info->icon_geometry;
  XBestIconSize(display,&windows->icon,display_image);
  windows->icon.attributes.colormap=XDefaultColormap(display,
    icon_visual->screen);
  windows->icon.attributes.event_mask=ExposureMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=IconicState;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->icon);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (icon)",
      windows->icon.id);
  /*
    Initialize graphic context for icon window.
  */
  if (icon_pixel->annotate_context != (GC) NULL)
    (void) XFreeGC(display,icon_pixel->annotate_context);
  context_values.background=icon_pixel->background_color.pixel;
  context_values.foreground=icon_pixel->foreground_color.pixel;
  icon_pixel->annotate_context=XCreateGC(display,windows->icon.id,
    (size_t) (GCBackground | GCForeground),&context_values);
  if (icon_pixel->annotate_context == (GC) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateGraphicContext",
      display_image->filename);
  windows->icon.annotate_context=icon_pixel->annotate_context;
  /*
    Initialize Image window.
  */
  XNameImageWindow(&event_context,display_image);
  if (resource_info->immutable)
    windows->image.immutable=MagickTrue;
  windows->image.use_pixmap=resource_info->use_pixmap;
  windows->image.geometry=resource_info->image_geometry;
  (void) FormatLocaleString(geometry,MagickPathExtent,"%ux%u+0+0>!",
    XDisplayWidth(display,visual_info->screen),
    XDisplayHeight(display,visual_info->screen));
  geometry_info.width=display_image->columns;
  geometry_info.height=display_image->rows;
  geometry_info.x=0;
  geometry_info.y=0;
  (void) ParseMetaGeometry(geometry,&geometry_info.x,&geometry_info.y,
    &geometry_info.width,&geometry_info.height);
  windows->image.width=(unsigned int) geometry_info.width;
  windows->image.height=(unsigned int) geometry_info.height;
  windows->image.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | OwnerGrabButtonMask |
    PropertyChangeMask | StructureNotifyMask | SubstructureNotifyMask;
  XInitializeBackdropWindow(&event_context);
  XMakeImageWindow(&event_context);
  /*
    Initialize Info widget.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,resource_info,
    &windows->info);
  (void) CloneString(&windows->info.name,"Info");
  (void) CloneString(&windows->info.icon_name,"Info");
  windows->info.border_width=1;
  windows->info.x=2;
  windows->info.y=2;
  windows->info.flags|=PPosition;
  windows->info.attributes.win_gravity=UnmapGravity;
  windows->info.attributes.event_mask=ButtonPressMask | ExposureMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,windows->image.id,argv,argc,class_hints,manager_hints,
    &windows->info);
  windows->info.highlight_stipple=XCreateBitmapFromData(display,
    windows->info.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->info.shadow_stipple=XCreateBitmapFromData(display,
    windows->info.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->info.id,windows->image.id);
  if (windows->image.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (info)",
      windows->info.id);
  /*
    Initialize Command widget.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->command);
  windows->command.data=MagickMenus;
  (void) XCommandWidget(display,windows,CommandMenu,(XEvent *) NULL);
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.command",
    resource_info->client_name);
  windows->command.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) CloneString(&windows->command.name,MagickTitle);
  windows->command.border_width=0;
  windows->command.flags|=PPosition;
  windows->command.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | LeaveWindowMask |
    OwnerGrabButtonMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->command);
  windows->command.highlight_stipple=XCreateBitmapFromData(display,
    windows->command.id,(char *) HighlightBitmap,HighlightWidth,
    HighlightHeight);
  windows->command.shadow_stipple=XCreateBitmapFromData(display,
    windows->command.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->command.id,windows->image.id);
  if (windows->command.mapped != MagickFalse)
    (void) XMapRaised(display,windows->command.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (command)",windows->command.id);
  /*
    Initialize Widget window.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->widget);
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.widget",
    resource_info->client_name);
  windows->widget.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  windows->widget.border_width=0;
  windows->widget.flags|=PPosition;
  windows->widget.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | OwnerGrabButtonMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->widget);
  windows->widget.highlight_stipple=XCreateBitmapFromData(display,
    windows->widget.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->widget.shadow_stipple=XCreateBitmapFromData(display,
    windows->widget.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->widget.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (widget)",windows->widget.id);
  /*
    Initialize popup window.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->popup);
  windows->popup.border_width=0;
  windows->popup.flags|=PPosition;
  windows->popup.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | ExposureMask | KeyPressMask |
    KeyReleaseMask | LeaveWindowMask | StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->popup);
  windows->popup.highlight_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) HighlightBitmap,HighlightWidth,HighlightHeight);
  windows->popup.shadow_stipple=XCreateBitmapFromData(display,
    windows->popup.id,(char *) ShadowBitmap,ShadowWidth,ShadowHeight);
  (void) XSetTransientForHint(display,windows->popup.id,windows->image.id);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (pop up)",windows->popup.id);
  /*
    Initialize Magnify window and cursor.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->magnify);
  if (resource_info->use_shared_memory == MagickFalse)
    windows->magnify.shared_memory=MagickFalse;
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.magnify",
    resource_info->client_name);
  windows->magnify.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) FormatLocaleString(windows->magnify.name,MagickPathExtent,
    "Magnify %uX",resource_info->magnify);
  if (windows->magnify.cursor != (Cursor) NULL)
    (void) XFreeCursor(display,windows->magnify.cursor);
  windows->magnify.cursor=XMakeCursor(display,windows->image.id,
    map_info->colormap,resource_info->background_color,
    resource_info->foreground_color);
  if (windows->magnify.cursor == (Cursor) NULL)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateCursor",
      display_image->filename);
  windows->magnify.width=MagnifySize;
  windows->magnify.height=MagnifySize;
  windows->magnify.flags|=PPosition;
  windows->magnify.min_width=MagnifySize;
  windows->magnify.min_height=MagnifySize;
  windows->magnify.width_inc=MagnifySize;
  windows->magnify.height_inc=MagnifySize;
  windows->magnify.data=resource_info->magnify;
  windows->magnify.attributes.cursor=windows->magnify.cursor;
  windows->magnify.attributes.event_mask=ButtonPressMask | ButtonReleaseMask |
    ExposureMask | KeyPressMask | KeyReleaseMask | OwnerGrabButtonMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickTrue;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->magnify);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),
      "Window id: 0x%lx (magnify)",windows->magnify.id);
  (void) XSetTransientForHint(display,windows->magnify.id,windows->image.id);
  /*
    Initialize panning window.
  */
  XGetWindowInfo(display,visual_info,map_info,pixel,font_info,
    resource_info,&windows->pan);
  (void) CloneString(&windows->pan.name,"Pan Icon");
  windows->pan.width=windows->icon.width;
  windows->pan.height=windows->icon.height;
  (void) FormatLocaleString(resource_name,MagickPathExtent,"%s.pan",
    resource_info->client_name);
  windows->pan.geometry=XGetResourceClass(resource_info->resource_database,
    resource_name,"geometry",(char *) NULL);
  (void) XParseGeometry(windows->pan.geometry,&windows->pan.x,&windows->pan.y,
    &windows->pan.width,&windows->pan.height);
  windows->pan.flags|=PPosition;
  windows->pan.immutable=MagickTrue;
  windows->pan.attributes.event_mask=ButtonMotionMask | ButtonPressMask |
    ButtonReleaseMask | ExposureMask | KeyPressMask | KeyReleaseMask |
    StructureNotifyMask;
  manager_hints->flags=InputHint | StateHint | WindowGroupHint;
  manager_hints->input=MagickFalse;
  manager_hints->initial_state=NormalState;
  manager_hints->window_group=windows->image.id;
  XMakeWindow(display,root_window,argv,argc,class_hints,manager_hints,
    &windows->pan);
  if (resource_info->debug != MagickFalse)
    (void) LogMagickEvent(X11Event,GetMagickModule(),"Window id: 0x%lx (pan)",
      windows->pan.id);
  (void) XSetTransientForHint(display,windows->pan.id,windows->image.id);
  if (windows->info.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if ((windows->image.mapped == MagickFalse) ||
      (windows->backdrop.id != (Window) NULL))
    (void) XMapWindow(display,windows->image.id);
  /*
    Set our progress monitor and warning handlers.
  */
  if (warning_handler == (WarningHandler) NULL)
    {
      warning_handler=resource_info->display_warnings ?
        SetErrorHandler(XWarning) : SetErrorHandler((ErrorHandler) NULL);
      warning_handler=resource_info->display_warnings ?
        SetWarningHandler(XWarning) : SetWarningHandler((WarningHandler) NULL);
    }
  /*
    Initialize Image and Magnify X images.
  */
  windows->image.x=0;
  windows->image.y=0;
  windows->magnify.shape=MagickFalse;
  width=(unsigned int) display_image->columns;
  height=(unsigned int) display_image->rows;
  if ((display_image->columns != width) || (display_image->rows != height))
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  status=XMakeImage(display,resource_info,&windows->image,display_image,
    width,height,exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  status=XMakeImage(display,resource_info,&windows->magnify,(Image *) NULL,
    windows->magnify.width,windows->magnify.height,exception);
  if (status == MagickFalse)
    ThrowXWindowFatalException(XServerFatalError,"UnableToCreateXImage",
      display_image->filename);
  if (windows->magnify.mapped != MagickFalse)
    (void) XMapRaised(display,windows->magnify.id);
  if (windows->pan.mapped != MagickFalse)
    (void) XMapRaised(display,windows->pan.id);
  windows->image.window_changes.width=(int) display_image->columns;
  windows->image.window_changes.height=(int) display_image->rows;
  (void) XConfigureImage(display,resource_info,windows,display_image,exception);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  (void) XSync(display,MagickFalse);
  /*
    Respond to events.
  */
  delay=display_image->delay/(size_t)
    MagickMax(display_image->ticks_per_second,1L);
  timer=GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1;
  update_time=XGetUpdateTime(resource_info,display_image);
  *state&=(unsigned int) (~FormerImageState);
  *state&=(unsigned int) (~MontageImageState);
  *state&=(unsigned int) (~NextImageState);
  event_context.context_mask=context_mask;
  event_context.update_time=update_time;
  do
  {
    /*
      Handle a window event.
    */
    if (windows->image.mapped != MagickFalse)
      if (XCheckImageTimer(&event_context,display_image,&nexus,&timer) !=
          MagickFalse)
        continue;
    timestamp=GetMagickTime();
    (void) XNextEvent(display,&event);
    if ((windows->image.stasis == MagickFalse) ||
        (windows->magnify.stasis == MagickFalse))
      {
        if ((GetMagickTime()-timestamp) > 0)
          {
            windows->image.stasis=MagickTrue;
            windows->magnify.stasis=MagickTrue;
          }
      }
    if (event.xany.window == windows->command.id)
      {
        /*
          Select a command from the Command widget.
        */
        id=XCommandWidget(display,windows,CommandMenu,&event);
        if (id < 0)
          continue;
        (void) CopyMagickString(command,CommandMenu[id],MagickPathExtent);
        display_command=CommandMenus[id];
        if (XSelectCommandMenuEntry(&event_context,id,&display_command) !=
            MagickFalse)
          continue;
        if (display_command != NullCommand)
          nexus=XMagickCommand(display,resource_info,windows,display_command,
            &display_image,exception);
        continue;
      }
    switch (event.type)
    {
      case ButtonPress:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Button Press: 0x%lx %u +%d+%d",event.xbutton.window,
            event.xbutton.button,event.xbutton.x,event.xbutton.y);
        if ((event.xbutton.button == Button3) &&
            (event.xbutton.state & Mod1Mask))
          {
            /*
              Convert Alt-Button3 to Button2.
            */
            event.xbutton.button=Button2;
            event.xbutton.state&=(unsigned int) (~Mod1Mask);
          }
        if (event.xbutton.window == windows->backdrop.id)
          {
            (void) XSetInputFocus(display,event.xbutton.window,RevertToParent,
              event.xbutton.time);
            break;
          }
        if (event.xbutton.window == windows->image.id)
          {
            XPressImageWindow(&event_context,&event_targets);
            break;
          }
        if (event.xbutton.window == windows->magnify.id)
          {
            XPressMagnifyWindow(&event_context);
            break;
          }
        if (event.xbutton.window == windows->pan.id)
          {
            XPressPanWindow(&event_context,&event);
            break;
          }
        delay=display_image->delay/(size_t)
          MagickMax(display_image->ticks_per_second,1L);
        timer=GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1;
        break;
      }
      case ButtonRelease:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Button Release: 0x%lx %u +%d+%d",event.xbutton.window,
            event.xbutton.button,event.xbutton.x,event.xbutton.y);
        break;
      }
      case ClientMessage:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Client Message: 0x%lx 0x%lx %d 0x%lx",event.xclient.window,
            event.xclient.message_type,event.xclient.format,(unsigned long)
            event.xclient.data.l[0]);
        if (event.xclient.message_type == windows->im_protocols)
          {
            XHandleProtocolMessage(&event_context,event,&context_values);
            break;
          }
        if (event.xclient.message_type == windows->dnd_protocols)
          {
            nexus=XDropImage(&event_context,event,nexus);
            break;
          }
        /*
          If client window delete message, exit.
        */
        if (event.xclient.message_type != windows->wm_protocols)
          break;
        if (*event.xclient.data.l != (long) windows->wm_delete_window)
          break;
        (void) XWithdrawWindow(display,event.xclient.window,
          visual_info->screen);
        if (event.xclient.window == windows->image.id)
          {
            *state|=ExitState;
            break;
          }
        if (event.xclient.window == windows->pan.id)
          {
            /*
              Restore original image size when pan window is deleted.
            */
            windows->image.window_changes.width=windows->image.ximage->width;
            windows->image.window_changes.height=windows->image.ximage->height;
            (void) XConfigureImage(display,resource_info,windows,
              display_image,exception);
          }
        break;
      }
      case ConfigureNotify:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Configure Notify: 0x%lx %dx%d+%d+%d %d",event.xconfigure.window,
            event.xconfigure.width,event.xconfigure.height,event.xconfigure.x,
            event.xconfigure.y,event.xconfigure.send_event);
        if (event.xconfigure.window == windows->image.id)
          {
            XConfigureImageWindow(&event_context,event,display_image,vid_info);
            break;
          }
        if (event.xconfigure.window == windows->magnify.id)
          {
            status=XConfigureMagnifyWindow(&event_context,event,display_image,
              status);
            break;
          }
        if (windows->magnify.mapped != MagickFalse &&
            (event.xconfigure.window == windows->pan.id))
          {
            XConfigurePanWindow(windows,event);
            break;
          }
        if (event.xconfigure.window == windows->icon.id)
          {
            /*
              Icon window has a new configuration.
            */
            windows->icon.width=(unsigned int) event.xconfigure.width;
            windows->icon.height=(unsigned int) event.xconfigure.height;
            break;
          }
        break;
      }
      case DestroyNotify:
      {
        /*
          Group leader has exited.
        */
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Destroy Notify: 0x%lx",event.xdestroywindow.window);
        if (event.xdestroywindow.window == windows->group_leader.id)
          {
            *state|=ExitState;
            break;
          }
        break;
      }
      case EnterNotify:
      {
        /*
          Selectively install colormap.
        */
        if ((map_info->colormap != XDefaultColormap(display,visual_info->screen)) &&
            (event.xcrossing.mode != NotifyUngrab))
          XInstallColormap(display,map_info->colormap);
        break;
      }
      case Expose:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Expose: 0x%lx %dx%d+%d+%d",event.xexpose.window,
            event.xexpose.width,event.xexpose.height,event.xexpose.x,
            event.xexpose.y);
        /*
          Refresh windows that are now exposed.
        */
        if ((event.xexpose.window == windows->image.id) &&
            windows->image.mapped != MagickFalse)
          {
            XRefreshWindow(display,&windows->image,&event);
            delay=display_image->delay/(size_t) MagickMax(
              display_image->ticks_per_second,1L);
            timer=GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1;
            break;
          }
        if ((event.xexpose.window == windows->magnify.id) &&
            windows->magnify.mapped != MagickFalse)
          {
            XMakeMagnifyImage(display,windows,exception);
            break;
          }
        if (event.xexpose.window == windows->pan.id)
          {
            XDrawPanRectangle(display,windows);
            break;
          }
        if (event.xexpose.window == windows->icon.id)
          {
            XRefreshWindow(display,&windows->icon,&event);
            break;
          }
        break;
      }
      case KeyPress:
      {
        int
          length;

        /*
          Respond to a user key press.
        */
        length=XLookupString((XKeyEvent *) &event.xkey,command,(int)
          sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        *(command+length)='\0';
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Key press: %d 0x%lx (%s)",event.xkey.state,(unsigned long)
            key_symbol,command);
        if (event.xkey.window == windows->image.id)
          {
            XImageWindowKeyPress(&event_context,event,key_symbol,
              &event_targets);
          }
        if (event.xkey.window == windows->magnify.id)
          XMagnifyWindowCommand(display,windows,event.xkey.state,key_symbol,
            exception);
        if (event.xkey.window == windows->pan.id)
          {
            XPanWindowKeyPress(&event_context,key_symbol);
          }
        delay=display_image->delay/(size_t) MagickMax(
          display_image->ticks_per_second,1L);
        timer=GetMagickTime()+(time_t) (delay == 0 ? 1 : delay)+1;
        break;
      }
      case KeyRelease:
      {
        /*
          Respond to a user key release.
        */
        (void) XLookupString((XKeyEvent *) &event.xkey,command,(int)
          sizeof(command),&key_symbol,(XComposeStatus *) NULL);
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Key release: 0x%lx (%c)",(unsigned long) key_symbol,*command);
        break;
      }
      case LeaveNotify:
      {
        /*
          Selectively uninstall colormap.
        */
        if ((map_info->colormap != XDefaultColormap(display,visual_info->screen)) &&
            (event.xcrossing.mode != NotifyUngrab))
          XUninstallColormap(display,map_info->colormap);
        break;
      }
      case MapNotify:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),"Map Notify: 0x%lx",
            event.xmap.window);
        if (event.xmap.window == windows->backdrop.id)
          {
            (void) XSetInputFocus(display,event.xmap.window,RevertToParent,
              CurrentTime);
            windows->backdrop.mapped=MagickTrue;
            break;
          }
        if (event.xmap.window == windows->image.id)
          {
            nexus=XMapImageWindow(&event_context,display_image,nexus);
            break;
          }
        if (event.xmap.window == windows->magnify.id)
          {
            XMakeMagnifyImage(display,windows,exception);
            windows->magnify.mapped=MagickTrue;
            (void) XWithdrawWindow(display,windows->info.id,
              windows->info.screen);
            break;
          }
        if (event.xmap.window == windows->pan.id)
          {
            XMakePanImage(display,resource_info,windows,display_image,
              exception);
            windows->pan.mapped=MagickTrue;
            break;
          }
        if (event.xmap.window == windows->info.id)
          {
            windows->info.mapped=MagickTrue;
            break;
          }
        if (event.xmap.window == windows->icon.id)
          {
            MagickBooleanType
              taint;

            /*
              Create an icon image.
            */
            taint=display_image->taint;
            XMakeStandardColormap(display,icon_visual,icon_resources,
              display_image,icon_map,icon_pixel,exception);
            (void) XMakeImage(display,icon_resources,&windows->icon,
              display_image,windows->icon.width,windows->icon.height,
              exception);
            display_image->taint=taint;
            (void) XSetWindowBackgroundPixmap(display,windows->icon.id,
              windows->icon.pixmap);
            (void) XClearWindow(display,windows->icon.id);
            (void) XWithdrawWindow(display,windows->info.id,
              windows->info.screen);
            windows->icon.mapped=MagickTrue;
            break;
          }
        if (event.xmap.window == windows->command.id)
          {
            windows->command.mapped=MagickTrue;
            break;
          }
        if (event.xmap.window == windows->popup.id)
          {
            windows->popup.mapped=MagickTrue;
            break;
          }
        if (event.xmap.window == windows->widget.id)
          {
            windows->widget.mapped=MagickTrue;
            break;
          }
        break;
      }
      case MappingNotify:
      {
        (void) XRefreshKeyboardMapping(&event.xmapping);
        break;
      }
      case NoExpose:
        break;
      case PropertyNotify:
      {
        Atom
          type;

        int
          format,
          status;

        unsigned char
          *data;

        unsigned long
          after,
          length;

        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Property Notify: 0x%lx 0x%lx %d",event.xproperty.window,
            event.xproperty.atom,event.xproperty.state);
        if (event.xproperty.atom != windows->im_remote_command)
          break;
        /*
          Display image named by the remote command protocol.
        */
        status=XGetWindowProperty(display,event.xproperty.window,
          event.xproperty.atom,0L,(long) MagickPathExtent,MagickFalse,(Atom)
          AnyPropertyType,&type,&format,&length,&after,&data);
        if ((status != Success) || (length == 0))
          break;
        if (LocaleCompare((char *) data,"-quit") == 0)
          {
            XClientMessage(display,windows->image.id,windows->im_protocols,
              windows->im_exit,CurrentTime);
            (void) XFree((void *) data);
            break;
          }
        (void) CopyMagickString(resource_info->image_info->filename,
          (char *) data,MagickPathExtent);
        (void) XFree((void *) data);
        nexus=ReadImage(resource_info->image_info,exception);
        CatchException(exception);
        if (nexus != (Image *) NULL)
          *state|=NextImageState | ExitState;
        break;
      }
      case ReparentNotify:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Reparent Notify: 0x%lx=>0x%lx",event.xreparent.parent,
            event.xreparent.window);
        break;
      }
      case UnmapNotify:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),
            "Unmap Notify: 0x%lx",event.xunmap.window);
        if (event.xunmap.window == windows->backdrop.id)
          {
            windows->backdrop.mapped=MagickFalse;
            break;
          }
        if (event.xunmap.window == windows->image.id)
          {
            windows->image.mapped=MagickFalse;
            break;
          }
        if (event.xunmap.window == windows->magnify.id)
          {
            windows->magnify.mapped=MagickFalse;
            break;
          }
        if (event.xunmap.window == windows->pan.id)
          {
            windows->pan.mapped=MagickFalse;
            break;
          }
        if (event.xunmap.window == windows->info.id)
          {
            windows->info.mapped=MagickFalse;
            break;
          }
        if (event.xunmap.window == windows->icon.id)
          {
            XUnmapIconWindow(&event_context,display_image);
            break;
          }
        if (event.xunmap.window == windows->command.id)
          {
            windows->command.mapped=MagickFalse;
            break;
          }
        if (event.xunmap.window == windows->popup.id)
          {
            XFocusImageWindow(display,windows);
            windows->popup.mapped=MagickFalse;
            break;
          }
        if (event.xunmap.window == windows->widget.id)
          {
            XFocusImageWindow(display,windows);
            windows->widget.mapped=MagickFalse;
            break;
          }
        break;
      }
      default:
      {
        if (resource_info->debug != MagickFalse)
          (void) LogMagickEvent(X11Event,GetMagickModule(),"Event type: %d",
            event.type);
        break;
      }
    }
  } while (!(*state & ExitState));
  if ((*state & ExitState) == 0)
    (void) XMagickCommand(display,resource_info,windows,FreeBuffersCommand,
      &display_image,exception);
  else
    if (resource_info->confirm_edit != MagickFalse)
      {
        /*
          Query user if image has changed.
        */
        if ((resource_info->immutable == MagickFalse) &&
            display_image->taint != MagickFalse)
          {
            int
              status;

            status=XConfirmWidget(display,windows,"Your image changed.",
              "Do you want to save it");
            if (status == 0)
              *state&=(unsigned int) (~ExitState);
            else
              if (status > 0)
                (void) XMagickCommand(display,resource_info,windows,SaveCommand,
                  &display_image,exception);
          }
      }
  if (IsDynamicVisualClass(windows->visual_info->klass) != MagickFalse)
    {
      /*
        Withdraw pan and Magnify window.
      */
      if (windows->info.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
      if (windows->magnify.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->magnify.id,
          windows->magnify.screen);
      if (windows->command.mapped != MagickFalse)
        (void) XWithdrawWindow(display,windows->command.id,
          windows->command.screen);
    }
  if (windows->pan.mapped != MagickFalse)
    (void) XWithdrawWindow(display,windows->pan.id,windows->pan.screen);
  if ((resource_info->backdrop == MagickFalse) && (windows->backdrop.mapped))
    {
      (void) XWithdrawWindow(display,windows->backdrop.id,
        windows->backdrop.screen);
      (void) XDestroyWindow(display,windows->backdrop.id);
      windows->backdrop.id=(Window) NULL;
      (void) XWithdrawWindow(display,windows->image.id,
        windows->image.screen);
      (void) XDestroyWindow(display,windows->image.id);
      windows->image.id=(Window) NULL;
    }
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  if (((*state & FormerImageState) != 0) || ((*state & NextImageState) != 0))
    *state&=(unsigned int) (~ExitState);
  if (*state & ExitState)
    {
      /*
        Free Standard Colormap.
      */
      (void) XFreeStandardColormap(display,icon_visual,icon_map,icon_pixel);
      if (resource_info->map_type == (char *) NULL)
        (void) XFreeStandardColormap(display,visual_info,map_info,pixel);
      /*
        Free X resources.
      */
      if (resource_info->copy_image != (Image *) NULL)
        {
          resource_info->copy_image=DestroyImage(resource_info->copy_image);
          resource_info->copy_image=NewImageList();
        }
      DestroyXResources();
    }
  (void) XSync(display,MagickFalse);
  /*
    Restore our progress monitor and warning handlers.
  */
  (void) SetErrorHandler(warning_handler);
  (void) SetWarningHandler(warning_handler);
  /*
    Change to home directory.
  */
  directory=getcwd(working_directory,MagickPathExtent);
  (void) directory;
  {
    int
      status;

    if (*resource_info->home_directory == '\0')
      (void) CopyMagickString(resource_info->home_directory,".",MagickPathExtent);
    status=chdir(resource_info->home_directory);
    if (status == -1)
      (void) ThrowMagickException(exception,GetMagickModule(),FileOpenError,
        "UnableToOpenFile","%s",resource_info->home_directory);
  }
  *image=display_image;
  return(nexus);
}
#endif
