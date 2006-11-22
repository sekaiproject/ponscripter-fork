Herewith a brief explanation of the key points wherein Ponscripter differs from 
the ONScripter you know and love.

In no particular order, I'm afraid.



/-------------------------------------------------------------------------------
|  Directives

NScripter files can optionally begin with a resolution directive, which is usually
omitted for 640x480 games and included in 800x600 games: it takes the form of a
comment at the very start of the file, either

  ;mode640
  
or
 
  ;mode800

In Ponscripter this directive is mandatory, and must additionally be followed
by a second directive, which takes the form

  ;gameid Your name here

This second directive is used to confirm that the script file is intended for 
use with Ponscripter.  It is also used on some platforms to name a folder for 
the game's saved data:

  OS X: ~/Library/Preferences/[gameid] Files
  Linux: ~/.[gameid]
  Windows: C:\Documents and Settings\All Users\Application Data\[gameid]
           (or something similar)

(Note that on OS X and *nix saved games are per-user, while on Windows they are 
currently shared between all users.  This latter could easily be changed if so 
desired.)

(Consequently, you can install a game naturally on Linux by creating a launcher
script along the lines of

  #! /bin/sh
  /usr/local/bin/ponscr -r /usr/local/share/games/narcissu2

and expect it to work properly. The same naturally goes for FreeBSD and so on.)


/-------------------------------------------------------------------------------
|  h_mapfont <int>, <string>, [metrics file]

Ponscripter supports up to 8 fonts/styles at a time. The intended convention is 
that this should represent two typefaces (a text face and a display face), each 
with regular, italic, bold, and bold italic styles. Naturally this is not 
enforced, though the style tag system was devised with this convention in mind.
(See "Style tags" below for more details.)

Unlike ONScripter, there are no requirements as to the nature of the fonts used. 
They need not be TrueType, Japanese, monospaced, or called "default.ttf".

By default, each style N is associated with a file "fontN.ttf", which may either 
be in the game folder (a la ONScripter's "default.ttf") or in the game's main 
archive. To use fonts with different names, in different places, or to use non-
TrueType fonts, the mappings must be set manually with this command.

As you might expect, <int> is the font slot to be mapped, and <string> is the
filename (relative to the game's data path) of the font to use.  The font formats
supported are those supported by the Freetype library used by the game - this
will typically include at least TrueType and Type 1 fonts.

For Type 1 fonts, the outlines and metrics for each font are stored in separate
files: the outlines in either a .pfa or .pfb, and the metrics in a .afm.  Both
files are required for acceptable output.  The .pfa or .pfb should be given as
the font, and the .afm given in the extra <metrics file> parameter.

For example, given a "fonts" subdirectory of the game's data directory that
contains the standard GhostScript outline fonts, one could use the command

  h_mapfont 2, "fonts/n021004l.pfb", "fonts/n021004l.afm"

to assign URW Nimbus Roman Bold (a Times clone) to slot 2, which by convention
is the bold weight of the text font.

Note that fonts must still be stored as part of and distributed with the game.
It is not currently possible to use fonts that are installed on the user's
computer, though I intend to add support for this in future versions.

This command must be used in the *define section.


/-------------------------------------------------------------------------------
|  Syntax changes

The most major change is that [O]NScripter files are encoded in Shift_JIS, while
Ponscripter files must be encoded in UTF-8.

In ONscripter, ASCII text is marked with the ` character in one of three ways:
  some_command "`ASCII parameter"
  some_command `ASCII parameter`
  `ASCII display text\

In Ponscripter, all these examples use the ^ character instead:
  some_command "^UTF-8 text"
  some_command ^UTF-8 text^
  ^UTF-8 display text\

The reason for this change is that in order to make typing English text convenient,
Ponscripter introduces ASCII shortcuts for some common characters:

  `single curly quotes'
  ``double curly quotes''   (or ``double curly quotes")

Naturally you can also use the proper UTF-8 characters, but most keyboards can't 
type them conveniently.

To type two consecutive quotation marks, use a ZWNJ (zero-width non-joiner) to 
separate them.  Since there are probably no keyboards that can type a ZWNJ, the 
character | serves this purpose; to get a | in your text, use ||.  Hence, to 
write text with nested quotations, you might use a line like

  ^``He told me he'd `never leave me,'|'' she sobbed.\


/-------------------------------------------------------------------------------
|  h_ligate <string>, <int>
|  h_ligate <string>, remove
|  h_ligate <predefined set>

Additional shortcuts/ligatures can be defined (or existing shortcuts undefined) 
using the h_ligate command.

The first two forms of this command manage individual shortcuts/ligatures. The 
<string> is the text to replace, and the <int> is the Unicode character to use
in its place, or the token "remove" (without quotation marks) to unmap the 
replacement.  For example, to define the standard fi/fl ligatures, use

  h_ligate "fi", 0xfb01
  h_ligate "fl", 0xfb02

Where multiple replacements begin with the same substring, define the shortest 
first: internally a LIFO stack is used to store definitions, and the search 
stops with the first match, so if a shorter string is defined after a longer 
string, the search will stop when the shorter string is matched and the longer 
string will never be used.

Predefined sets define the shortcuts I personally use.  The following commands
exist:

* h_ligate none
  
Undefines all substitutions, including the curly-quotation-mark shortcuts that 
are defined by default.

* h_ligate basic
  
Redefines the curly-quotation-mark shortcuts.

* h_ligate punctuation
  
Defines the following shortcuts:
    ...   ellipsis
    --    en dash
    ---   em dash
    (c)   copyright symbol
    (r)   registered trademark symbol
    (tm)  trademark symbol
    ++    dagger
    +++   double dagger
    **    bullet
    %_    non-breaking space
    %-    non-breaking hyphen
    %.    thin space

* h_ligate f_ligatures

Defines the fi, fl, ff, ffi, and ffl ligatures. The latter three do not appear 
in most fonts, so you should normally define fi and fl ligatures manually as 
described above instead.

Note that Ponscripter does not check whether a character exists -- substitutions 
take place at the parsing stage, before it is known which font will be used. 
The user is responsible for ensuring that appropriate characters will exist.


/-------------------------------------------------------------------------------
|  Style tags

Text appearance is controlled with inline tag blocks.  A tag block is delimited
by ~tildes~, and contains any number of tags, which may optionally be separated
with spaces.

Where a tag block appears in display text, its effects last until the end of the
current screen; to apply a style to multiple screens, use h_fontstyle (see below).
Tag blocks can also be embedded to style text buttons, menu items, etc., in which
case they last to the end of the string in which they appear.

The following tags are recognised (N represents an arbitrary decimal integer):

FONT STYLE TAGS

  cN  Select font in slot N

  d   Default style (equivalent to c0)
  
  r   Selects regular style (default)
  i   Toggles italic style
  t   Selects book weight (default)
  b   Toggles bold weight
  f   Selects text face (default)
  s   Toggles display face
  
These latter six tags deserve explanation.  They assume the following font slot
assignment convention:

  0   text book regular
  1   text book italic
  2   text bold regular
  3   text bold italic
  4   display book regular
  5   display book italic 
  6   display bold regular
  7   display bold italic

The ~d~ tag selects slot 0.  The remaining tags combine to select other slots.
In practice, all most users will need to know is "do italics like ~i~this~i~".

For advanced users, it will be obvious that the implementation uses bitmasks. 
The following example illustrates how tags interact:

  ^This is ~i~emphasised~i~.  So is ~i~this~r~. (The two are functionally identical.)\
  ^Regular text ~s~display ~bi~bold italic display ~t~italic display ~si~regular text.\


FONT SIZE AND POSITION TAGS

  =N  Set font size to N pixels (0 sets it to the base font size)
  %N  Set font size to N% of the _base_ font size (as defined for the window)
  
  +N  Increase the _current_ font size by N pixels
  -N  Decrease the _current_ font size by N pixels
  
  xN  Set the horizontal text output position to N pixels right of the left margin
  yN  Set the vertical text output position to N pixels down from the top margin
  
  x+N  Shift the horizontal text output position N pixels to the right
  x-N  Shift the horizontal text output position N pixels to the left
  y+N  Shift the vertical text output position N pixels down
  y-N  Shift the vertical text output position N pixels up

As an example of the usage of these tags, Narcissu 2's omake mode displays page 
headings at the top of each screen with code like

  ^!s0~i %120 x-20 y-40~ Concept~i %100~!sd

Here the !s0 and !sd are the usual NScripter commands.  The first tag block 
selects italic text, 120% of the regular font size, and shifts the output 
position up and to the left.  The second tag block cancels the italic effect and 
resets the font size to normal.


/-------------------------------------------------------------------------------
|  h_fontstyle <string>

Sets default font styling.  Equivalent to inserting ~d<string>~ at the start of 
every subsequent text display command.  Note that this has no effect on text 
sprites.

Only font style tags are usable in h_fontstyle sections; font size and position
tags are not supported.

For example, to set an entire section of the game in your italic display font, use

  h_fontstyle ^si^
  ^Your text here\
  ^Several pages in italic display style\
  h_fontstyle ^d^


/-------------------------------------------------------------------------------
|  h_rendering <hinting>, <positioning>

Configures text rendering.

hinting:
  none = glyphs are rendered completely unhinted (default)
  full = glyphs are rendered with Freetype hinting
  light = glyphs are rendered with alternative Freetype hinting

positioning:
  integer = character positions are aligned to whole pixels (default)
  float = character positions use subpixel precision

Theoretically the float mode should give better spacing, but it's broken and 
normally looks awful. Feel free to try it if you feel adventurous, though.

Hinting should normally be enabled for Type 1 fonts; for TrueType fonts the
optimum setting very much depends on the font in question and on the settings
with which Freetype was compiled.


/-------------------------------------------------------------------------------
|  h_textextent <ivar>,<string>,[size_x],[size_y],[pitch_x]                                       
                                                                                                
Sets <ivar> to the width, in pixels, of <string> as rendered in the current 
sentence font. If given, size_x, size_y, and pitch_x override the current window 
settings (useful for calculating the size of buttons etc.)


/-------------------------------------------------------------------------------
|  h_centreline <string>

A basic hack to make it easier to centre text.

Sets the current x position to the expected location required to centre the 
given <string> on screen (NOT in the window, which must be large enough and 
appropriately positioned!) in the current sentence font.  The text is not 
actually displayed.

Sample usage:

  h_centreline ^...that dazzling sun... that summer's day...^
  ^...that dazzling sun... that summer's day...\


/-------------------------------------------------------------------------------
|  Command changes 1: size and position

All text and window sizing and positioning commands now operate in pixels, not 
zenkaku characters as before.  This breaks every single instance of locate, 
setwindow, and so forth.

For example, the Japanese version of Narcissu 2 defines its standard text output
window with the command

  setwindow 180,358,35,17,17,17,0,3,1,1,5,#ffffff,0,0,799,599

In the English version using Ponscripter, this becomes

  setwindow 150,358,540,240,19,19,0,0,1,1,5,#ffffff,0,0,799,599

The pertinent parts are the third and fourth numbers.  In the Japanese version, 
the third number is the width of the window in full-width characters, and the 
fourth is its height in lines.  In the English version, both width and height
are specified in pixels.


/-------------------------------------------------------------------------------
|  Command changes 2: sprite extension

The lsp command has been extended slightly.

In ONScripter, an English text sprite would be created using a definition string
like

  ":s/[xsize],[ysize],[pitch];#FORECOLOUR#BACKCOLOUR`text"

There are two changes in Ponscripter.  Firstly, obviously the ` is replaced with 
^ and text tags are permitted after it.  Secondly, if ":S/" is used in place of 
":s/", the x position of the sprite is measured relative to its centre, rather 
than to the left edge.  This makes it easier to create centered dialog buttons 
and the like.


/-------------------------------------------------------------------------------
|  Enhanced OS X support

OS X support in Ponscripter is enhanced in the same way as in "The world to 
reverse" and the Narcissu 2 demo, as follows:

1. Data files (script, fonts, archives) are stored in the Contents/Resources 
   directory within the application bundle.  This means your game will appear 
   as a single drag-to-install icon, not as an untidy bunch of files.

2. Saved games and configuration settings are stored in a subdirectory of the 
   user's ~/Library/Preferences, not in the directory holding the program. This 
   again makes it possible to treat the game as a regular OS X application.
   The name of the directory is based on the ;gameid directive (see "Directives"
   above).


/-------------------------------------------------------------------------------
|  Features removed

  Ruby, tategaki

Largely meaningless in Western text. Both can be simulated in small quantities 
if required by judicious use of h_textextent and font size/position tags.

  Clickstr
  
Never well supported in English, the clickstr function has been removed 
completely.  You will have to make all click pauses explicit with \.

  Glyph caching

You may theoretically notice slightly poorer performance rendering long runs of 
text.  In practice I haven't noticed any effect at all, and removing both 
independent glyph caches has simplified the code considerably.


/-------------------------------------------------------------------------------
|  Builds

Insani's ONScripter builds are all-in-one megalithic constructions designed to 
play any game on the market.  To give more polished packages, Ponscripter is 
designed to be built specifically for each game that will use it.  This way, 
unneeded functionality can be omitted from a build and the file size kept down.

The build included with this evaluation kit is the one designed for use with 
Narcissu 2.  It includes most features, but may possibly be missing 
functionality your project uses.  If you decide to use Ponscripter for your 
project, I would be happy to help produce suitable builds for you.

Since each game is intended to have a unique build anyway, the game's icon is 
now embedded within the executable file, not loaded from a PNG in the game 
directory.


/-------------------------------------------------------------------------------
|  Source code

ONScripter is GPL software, and so Ponscripter is as well.  Source code is 
therefore available if you want it.

I cannot guarantee support beyond that, but naturally I will do my best to
fix any bugs you find.

I would _prefer_ that you keep this package (and indeed the existence of 
Ponscripter) confidential for now, as it's not really quite ready to be revealed 
in public, but of course the GPL license means you have the freedom to choose 
for yourself what you do with it from here on.  Enjoy.