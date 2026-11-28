/*
  Copyright @ 1999 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/license/

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private methods of display-image.c: the typedefs, menus and
  bitmaps at the top of the X11 branch, the annotate, background and chop
  commands and the colour replacement helpers, from ClipboardMode to
  XReplaceImageColor; included by that file only.
*/
#ifndef MAGICKCORE_DISPLAY_IMAGE_ANNOTATE_CHOP_PRIVATE_H
#define MAGICKCORE_DISPLAY_IMAGE_ANNOTATE_CHOP_PRIVATE_H

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

typedef struct _ImageWindowKeyInput
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
} ImageWindowKeyInput;

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

typedef struct _MagnifyWindowInput
{
  Display
    *display;

  XWindows
    *windows;

  ExceptionInfo
    *exception;
} MagnifyWindowInput;

/*
  Forward declarations.
*/
static DisplayCommand
  XImageWindowCommand(const ImageWindowKeyInput *,const MagickStatusType,
    KeySym);
static Image
  *XMagickCommand(const XImageCacheContext *,const DisplayCommand),
  *XOpenImage(Display *,XResourceInfo *,XWindows *,const MagickBooleanType),
  *XTileImage(Display *,XResourceInfo *,XWindows *,Image *,XEvent *,
    ExceptionInfo *),
  *XVisualDirectoryImage(Display *,XResourceInfo *,XWindows *,
    ExceptionInfo *);
static MagickBooleanType
  XAnnotateEditImage(Display *,XResourceInfo *,XWindows *,Image *,
    ExceptionInfo *),
  XBackgroundImage(const ImageWindowKeyInput *),
  XChopImage(Display *,XResourceInfo *,XWindows *,Image **,
    ExceptionInfo *),
  XCropImage(Display *,XResourceInfo *,XWindows *,Image *,const ClipboardMode,
    ExceptionInfo *),
  XColorEditImage(const XImageCacheContext *),
  XCompositeImage(Display *,XResourceInfo *,XWindows *,Image *,
    ExceptionInfo *),
  XConfigureImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *),
  XDrawEditImage(const XImageCacheContext *),
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
  XImageCache(const XImageCacheContext *,const DisplayCommand),
  XMagnifyImage(Display *,XWindows *,XEvent *,ExceptionInfo *),
  XMakePanImage(Display *,XResourceInfo *,XWindows *,Image *,ExceptionInfo *),
  XPanImage(Display *,XWindows *,XEvent *,ExceptionInfo *),
  XMagnifyWindowCommand(const MagnifyWindowInput *,const MagickStatusType,
    const KeySym),
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
  return((MagickBooleanType) ((status != Success) || (type != XA_STRING) ||
    (format == 32) || (length == 0)));
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

  Cursor
    *cursor;
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
  {
    const XMenuRequest
      menu_request = { AnnotateMenu[id], (const char **) FontMenu, command };

    font_number=XMenuWidget(display,windows,&menu_request);
  }
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
  {
    const XMenuRequest
      menu_request = { AnnotateMenu[id], (const char **) ColorMenu, command };

    pen_number=XMenuWidget(display,windows,&menu_request);
  }
  if (pen_number < 0)
    return;
  (*transparent_pen)=(MagickBooleanType) (pen_number == (MaxNumberPens-2));
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
  {
    const XBestPixelTarget
      pixel_target = { display, windows->map_info->colormap };

    XBestPixel(&pixel_target,(XColor *) NULL,(unsigned int) MaxColors,color);
  }
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
  {
    const XMenuRequest
      menu_request = { AnnotateMenu[id], (const char **) ColorMenu, command };

    pen_number=XMenuWidget(display,windows,&menu_request);
  }
  if (pen_number < 0)
    return;
  (*transparent_box)=(MagickBooleanType) (pen_number == (MaxNumberPens-2));
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
  {
    const XBestPixelTarget
      pixel_target = { display, windows->map_info->colormap };

    XBestPixel(&pixel_target,(XColor *) NULL,(unsigned int) MaxColors,color);
  }
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
  {
    const XMenuRequest
      menu_request = { AnnotateMenu[id], RotateMenu, command };

    entry=XMenuWidget(display,windows,&menu_request);
  }
  if (entry < 0)
    return;
  if (entry != 8)
    {
      (*degrees)=StringToDouble(RotateMenu[entry],(char **) NULL);
      return;
    }
  {
    const XDialogRequest
      dialog_request = { "OK", "Enter rotation angle:", angle };

    (void) XDialogWidget(display,windows,&dialog_request);
  }
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

static void XFindBoxPixelIndex(XWindows *windows,const unsigned int box_id)
{
  ssize_t
    i;

  if (windows->pixel_info->colors != 0)
    for (i=0; i < (ssize_t) windows->pixel_info->colors; i++)
      if (windows->pixel_info->pixels[i] ==
          windows->pixel_info->pen_colors[box_id].pixel)
        {
          windows->pixel_info->box_index=(unsigned short) i;
          break;
        }
}

static void XSetBoxPixelIndex(XWindows *windows,const unsigned int box_id)
{
  windows->pixel_info->box_color=windows->pixel_info->pen_colors[box_id];
  XFindBoxPixelIndex(windows,box_id);
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
      {
        const XHelpView view = { context->display, context->resource_info,
          context->windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Annotation",ImageAnnotateHelp);
      }
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
      {
        const XHelpView view = { context->display, context->resource_info,
          context->windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Annotation",ImageAnnotateHelp);
      }
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
      {
        const XHelpView view = { context->display, context->resource_info,
          context->windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Annotation",ImageAnnotateHelp);
      }
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

static MagickBooleanType XAnnotateTextKeyEvent(
  const AnnotateContext *context,const AnnotateTargets *targets)
{
  char
    command[MagickPathExtent];

  KeySym
    key_symbol;

  XEvent
    *const event = targets->event;

  size_t
    *const state = targets->state;

  switch ((*event).type)
  {
    case KeyPress:
    {
      if ((*event).xkey.window != context->windows->image.id)
        break;
      if (XAnnotateTextKey(context,targets) == MagickFalse)
        return(MagickFalse);
      break;
    }
    case KeyRelease:
    {
      /*
        Respond to a user key release.
      */
      (void) XLookupString((XKeyEvent *) &(*event).xkey,command,(int)
        sizeof(command),&key_symbol,(XComposeStatus *) NULL);
      (*state)&=(size_t) (~ModifierState);
      break;
    }
    case SelectionNotify:
    {
      if (XAnnotateTextPaste(context,targets) == MagickFalse)
        return(MagickFalse);
      break;
    }
    default:
      break;
  }
  return(MagickTrue);
}

static MagickBooleanType XAnnotateTextEvent(const AnnotateContext *context,
  const AnnotateTargets *targets)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  const GC
    annotate_context = context->annotate_context;

  XFontStruct
    *const font_info = context->font_info;

  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  int
    *const x = targets->x,
    *const y = targets->y;

  XEvent
    *const text_event = targets->text_event;

  XEvent
    *const event = targets->event;

  /*
    Erase text cursor.
  */
  (*text_event).xexpose.x=(*x);
  (*text_event).xexpose.y=(*y)-font_info->max_bounds.ascent;
  (void) XClearArea(display,windows->image.id,(*x),(*text_event).xexpose.y,
    (unsigned int) (*text_event).xexpose.width,(unsigned int)
    (*text_event).xexpose.height,MagickFalse);
  XRefreshWindow(display,&windows->image,text_event);
  switch ((*event).type)
  {
    case ButtonPress:
    {
      XAnnotateTextPress(context,targets);
      break;
    }
    case Expose:
    {
      if ((*event).xexpose.count == 0)
        {
          /*
            Refresh Image window.
          */
          XRefreshWindow(display,&windows->image,(XEvent *) NULL);
          XRedrawTextLines(display,windows,(*annotate_info),annotate_context);
          (void) XDrawString(display,windows->image.id,annotate_context,
            (*x),(*y),"_",1);
        }
      break;
    }
    default:
    {
      if (XAnnotateTextKeyEvent(context,targets) == MagickFalse)
        return(MagickFalse);
      break;
    }
  }
  return(MagickTrue);
}

typedef struct _AnnotateRenderInput
{
  Image
    *image;

  int
    x,
    y;

  unsigned int
    height,
    width;
} AnnotateRenderInput;

static MagickBooleanType XAnnotateRenderLine(const AnnotateContext *context,
  const AnnotateRenderInput *input,XAnnotateInfo *annotate_info)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  XFontStruct
    *const font_info = context->font_info;

  const unsigned int
    box_id = context->box_id,
    pen_id = context->pen_id;

  Image
    *const image = input->image;

  const int
    x = input->x,
    y = input->y;

  const unsigned int
    height = input->height,
    width = input->width;

  int
    status;

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
  {
    const XImageRenderTarget
      target = { display, windows->pixel_info };

    status=XAnnotateImage(&target,annotate_info,image,exception) ==
      MagickFalse ? 0 : 1;
  }
  if (status == 0)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType XAnnotateRenderLines(const AnnotateContext *context,
  XAnnotateInfo *annotate_info,Image *image)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  const GC
    annotate_context = context->annotate_context;

  XFontStruct
    *const font_info = context->font_info;

  int
    x,
    y;

  unsigned int
    height,
    width;

  XAnnotateInfo
    *previous_info;

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

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
    {
      const AnnotateRenderInput
        input = { image, x, y, height, width };

      if (XAnnotateRenderLine(context,&input,annotate_info) == MagickFalse)
        return(MagickFalse);
    }
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
  XConfigureImageColormap(&cmap_target,image,exception);
  (void) XConfigureImage(display,resource_info,windows,image,exception);
  return(MagickTrue);
}

static void XFallBackToDefaultFont(XFontStruct **font_info,Display *display,
  XWindows *windows,XResourceInfo *resource_info,const unsigned int font_id)
{
  if ((*font_info) == (XFontStruct *) NULL)
    {
      XNoticeWidget(display,windows,"Unable to load font:",
        resource_info->font_name[font_id]);
      (*font_info)=windows->font_info;
    }
}

static MagickBooleanType XAnnotateLoadFont(const AnnotateContext *context,
  const AnnotateTargets *targets,XFontStruct **font_info)
{
  Display
    *const display = context->display;

  XResourceInfo
    *const resource_info = context->resource_info;

  XWindows
    *const windows = context->windows;

  const unsigned int
    font_id = *context->font_targets->font_id;

  int
    *const x = targets->x,
    *const y = targets->y;

  /*
    Set font info and check boundary conditions.
  */
  (*font_info)=XLoadQueryFont(display,resource_info->font_name[font_id]);
  XFallBackToDefaultFont(font_info,display,windows,resource_info,font_id);
  if (((*x)+(*font_info)->max_bounds.width) >= (int) windows->image.width)
    (*x)=(int) windows->image.width-(*font_info)->max_bounds.width;
  if ((*y) < (int) ((*font_info)->ascent+(*font_info)->descent))
    (*y)=(int) (*font_info)->ascent+(*font_info)->descent;
  if (((int) (*font_info)->max_bounds.width > (int) windows->image.width) ||
      (((*font_info)->ascent+(*font_info)->descent) >=
       (int) windows->image.height))
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType XAcquireAnnotateInfo(
  const AnnotateContext *context,const AnnotateTargets *targets,
  XFontStruct *font_info)
{
  XWindows
    *const windows = context->windows;

  const MagickBooleanType
    transparent_box = *context->box_color_targets->transparent_box,
    transparent_pen = *context->font_color_targets->transparent_pen;

  const double
    degrees = *context->rotate_targets->degrees;

  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  int
    *const x = targets->x,
    *const y = targets->y;

  /*
    Initialize annotate structure.
  */
  (*annotate_info)=(XAnnotateInfo *) AcquireMagickMemory(
    sizeof(*(*annotate_info)));
  if ((*annotate_info) == (XAnnotateInfo *) NULL)
    return(MagickFalse);
  XGetAnnotateInfo((*annotate_info));
  (*annotate_info)->x=(*x);
  (*annotate_info)->y=(*y);
  if ((transparent_box == MagickFalse) && (transparent_pen == MagickFalse))
    (*annotate_info)->stencil=OpaqueStencil;
  else
    if (transparent_box == MagickFalse)
      (*annotate_info)->stencil=BackgroundStencil;
    else
      (*annotate_info)->stencil=ForegroundStencil;
  (*annotate_info)->height=(unsigned int) (font_info->ascent+
    font_info->descent);
  (*annotate_info)->degrees=degrees;
  (*annotate_info)->font_info=font_info;
  (*annotate_info)->text=(char *) AcquireQuantumMemory((size_t) 
    windows->image.width/(size_t) MagickMax(font_info->min_bounds.width,1)+2UL,
    sizeof(*(*annotate_info)->text));
  if ((*annotate_info)->text == (char *) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType XAnnotateText(AnnotateContext *context,
  const AnnotateTargets *targets,XFontStruct *font_info,Image *image)
{
  Display
    *const display = context->display;

  XWindows
    *const windows = context->windows;

  ExceptionInfo
    *const exception = context->exception;

  const unsigned int
    box_id = *context->box_color_targets->box_id,
    pen_id = *context->font_color_targets->pen_id;

  XAnnotateInfo
    **const annotate_info = targets->annotate_info;

  char
    **const p = targets->p;

  int
    *const x = targets->x,
    *const y = targets->y;

  XEvent
    *const text_event = targets->text_event,
    *const event = targets->event;

  size_t
    *const state = targets->state;

  Cursor
    cursor;

  GC
    annotate_context;

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
  (*state)=DefaultState;
  (void) XDrawString(display,windows->image.id,annotate_context,(*x),(*y),"_",
    1);
  (*text_event).xexpose.width=(int) font_info->max_bounds.width;
  (*text_event).xexpose.height=font_info->max_bounds.ascent+
    font_info->max_bounds.descent;
  (*p)=(*annotate_info)->text;
  context->cursor=cursor;
  context->annotate_context=annotate_context;
  context->font_info=font_info;
  context->box_id=box_id;
  context->pen_id=pen_id;
  do
  {
    /*
      Display text cursor.
    */
    *(*p)='\0';
    (void) XDrawString(display,windows->image.id,annotate_context,(*x),(*y),
      "_",1);
    /*
      Wait for next event.
    */
    XScreenEvent(display,windows,event,exception);
    if ((*event).xany.window == windows->command.id)
      {
        XAnnotateTextCommand(context,targets);
        continue;
      }
    if (XAnnotateTextEvent(context,targets) == MagickFalse)
      return(MagickFalse);
  } while (((*state) & ExitState) == 0);
  (void) XFreeCursor(display,cursor);
  return(XAnnotateRenderLines(context,(*annotate_info),image));
}

static void XStartAnnotateTracking(Display *display,XWindows *windows,
  const AnnotateTargets *targets)
{
  Cursor
    *const cursor = targets->cursor;

  int
    *const x = targets->x,
    *const y = targets->y;

  size_t
    *const state = targets->state;

  /*
    Map Command widget.
  */
  (void) CloneString(&windows->command.name,"Annotate");
  windows->command.data=4;
  (void) XCommandWidget(display,windows,AnnotateMenu,(XEvent *) NULL);
  (void) XMapRaised(display,windows->command.id);
  {
    const XClientReason reason = { windows->im_update_widget, CurrentTime };

    XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
  }
  /*
    Track pointer until button 1 is pressed.
  */
  XQueryPosition(display,windows->image.id,x,y);
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask | PointerMotionMask);
  (*cursor)=XCreateFontCursor(display,XC_left_side);
  (void) XCheckDefineCursor(display,windows->image.id,(*cursor));
  (*state)=DefaultState;
}

static MagickBooleanType XFinishAnnotateTracking(Display *display,
  XWindows *windows,const size_t state)
{
  (void) XSelectInput(display,windows->image.id,
    windows->image.attributes.event_mask);
  (void) XWithdrawWindow(display,windows->info.id,windows->info.screen);
  if ((state & EscapeState) != 0)
    return(MagickFalse);
  return(MagickTrue);
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

  int
    x,
    y;

  size_t
    state;

  XAnnotateInfo
    *annotate_info;

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
    targets = { &annotate_info, &p, &x, &y, &text_event, &state, &event,
      &cursor };

  const AnnotateRotateTargets
    rotate_targets = { &degrees };

  const AnnotateBoxColorTargets
    box_color_targets = { &transparent_box, &color, &box_id };

  const AnnotateFontColorTargets
    font_color_targets = { &transparent_pen, &color, &pen_id };

  const AnnotateFontTargets
    font_targets = { &font_id };

  XStartAnnotateTracking(display,windows,&targets);
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
  if (XFinishAnnotateTracking(display,windows,state) == MagickFalse)
    return(MagickTrue);
  /*
    Set font info and check boundary conditions.
  */
  if (XAnnotateLoadFont(&context,&targets,&font_info) == MagickFalse)
    return(MagickFalse);
  /*
    Initialize annotate structure.
  */
  if (XAcquireAnnotateInfo(&context,&targets,font_info) == MagickFalse)
    return(MagickFalse);
  return(XAnnotateText(&context,&targets,font_info,image));
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
static MagickBooleanType XBackgroundImage(
  const ImageWindowKeyInput *window_input)
{
  Display
    *const display = window_input->display;

  ExceptionInfo
    *const exception = window_input->exception;

  Image
    **const image = window_input->image;

  XResourceInfo
    *const resource_info = window_input->resource_info;

  XWindows
    *const windows = window_input->windows;

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

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
  {
    const XDialogRequest
      dialog_request = { "Background",
        "Enter window id (id 0x00 selects window with pointer):", window_id };

    status=XDialogWidget(display,windows,&dialog_request);
  }
  if (*window_id == '\0')
    return(MagickFalse);
  (void) XMagickCommand(&command_context,ApplyCommand);
  XInfoWidget(display,windows,BackgroundImageTag);
  XSetCursorState(display,windows,MagickTrue);
  XCheckRefreshWindows(display,windows);
  background_resources=(*resource_info);
  background_resources.window_id=window_id;
  background_resources.backdrop=(MagickBooleanType) (status != 0);
  status=XDisplayBackgroundImage(display,&background_resources,*image,
    exception) == MagickFalse ? 0 : 1;
  if (status != MagickFalse)
    {
      const XClientReason reason = { windows->im_retain_colors, CurrentTime };

      XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
    }
  XSetCursorState(display,windows,MagickFalse);
  (void) XMagickCommand(&command_context,UndoCommand);
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

static void XClampChopSegmentToImage(XSegment *segment_info,
  const XWindows *windows)
{
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

static void XClampChopSegment(const XWindows *windows,XSegment *segment_info)
{
  /*
    Check boundary conditions.
  */
  XClampChopSegmentToImage(segment_info,windows);
}

static void XSetChopWidthFromRight(const XSegment *segment_info,
  RectangleInfo *chop_info,const XWindows *windows)
{
  if ((*segment_info).x1 > (*segment_info).x2)
    {
      (*chop_info).width=(size_t) ((*segment_info).x1-(*segment_info).x2+1);
      (*chop_info).x=(ssize_t) windows->image.x+(*segment_info).x2;
    }
}

static void XChopHorizontalGeometry(const XWindows *windows,
  const XSegment *segment_info,RectangleInfo *chop_info)
{
  (*chop_info).width=(size_t) ((*segment_info).x2-(*segment_info).x1+1);
  (*chop_info).height=0;
  (*chop_info).x=(ssize_t) windows->image.x+(*segment_info).x1;
  (*chop_info).y=0;
  XSetChopWidthFromRight(segment_info,chop_info,windows);
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
      {
        const XHelpView view = { display, resource_info, windows, MagickFalse };

        XTextViewHelp(&view,"Help Viewer - Image Chop",ImageChopHelp);
      }
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
          {
            const XMenuRequest
              menu_request = { ChopMenu[id], Directions, command };

            id=XMenuWidget(display,windows,&menu_request);
          }
          (*direction)=XSelectChopDirection(id,DirectionCommands,(*direction));
          break;
        }
        case ChopHelpCommand:
        {
          {
            const XHelpView view = { display, resource_info, windows,
              MagickFalse };

            XTextViewHelp(&view,"Help Viewer - Image Chop",ImageChopHelp);
          }
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

  const XImageCacheContext
    command_context = { display, resource_info, windows, image, exception };

  const XImageColormapTarget
    cmap_target = { display, resource_info, windows };

  /*
    Image chopping is relative to image configuration.
  */
  (void) XMagickCommand(&command_context,ApplyCommand);
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
  XConfigureImageColormap(&cmap_target,*image,exception);
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
  {
    const XClientReason reason = { windows->im_update_widget, CurrentTime };

    XClientMessage(display,windows->image.id,windows->im_protocols,&reason);
  }
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

#endif
