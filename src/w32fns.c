/* Functions for the Win32 window system.
   Copyright (C) 1989, 1992, 1993, 1994, 1995 Free Software Foundation.

This file is part of GNU Emacs.

GNU Emacs is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Added by Kevin Gallo */

#include <signal.h>
#include <config.h>
#include <stdio.h>

#include "lisp.h"
#include "w32term.h"
#include "frame.h"
#include "window.h"
#include "buffer.h"
#include "dispextern.h"
#include "keyboard.h"
#include "blockinput.h"
#include "paths.h"
#include "ntheap.h"
#include "termhooks.h"

#include <commdlg.h>

extern void abort ();
extern void free_frame_menubar ();
extern struct scroll_bar *x_window_to_scroll_bar ();

/* The colormap for converting color names to RGB values */
Lisp_Object Vwin32_color_map;

/* The name we're using in resource queries.  */
Lisp_Object Vx_resource_name;

/* Non nil if no window manager is in use.  */
Lisp_Object Vx_no_window_manager;

/* The background and shape of the mouse pointer, and shape when not
   over text or in the modeline.  */
Lisp_Object Vx_pointer_shape, Vx_nontext_pointer_shape, Vx_mode_pointer_shape;
/* The shape when over mouse-sensitive text.  */
Lisp_Object Vx_sensitive_text_pointer_shape;

/* Color of chars displayed in cursor box.  */
Lisp_Object Vx_cursor_fore_pixel;

/* Search path for bitmap files.  */
Lisp_Object Vx_bitmap_file_path;

/* Evaluate this expression to rebuild the section of syms_of_w32fns
   that initializes and staticpros the symbols declared below.  Note
   that Emacs 18 has a bug that keeps C-x C-e from being able to
   evaluate this expression.

(progn
  ;; Accumulate a list of the symbols we want to initialize from the
  ;; declarations at the top of the file.
  (goto-char (point-min))
  (search-forward "/\*&&& symbols declared here &&&*\/\n")
  (let (symbol-list)
    (while (looking-at "Lisp_Object \\(Q[a-z_]+\\)")
      (setq symbol-list
	    (cons (buffer-substring (match-beginning 1) (match-end 1))
		  symbol-list))
      (forward-line 1))
    (setq symbol-list (nreverse symbol-list))
    ;; Delete the section of syms_of_... where we initialize the symbols.
    (search-forward "\n  /\*&&& init symbols here &&&*\/\n")
    (let ((start (point)))
      (while (looking-at "^  Q")
	(forward-line 2))
      (kill-region start (point)))
    ;; Write a new symbol initialization section.
    (while symbol-list
      (insert (format "  %s = intern (\"" (car symbol-list)))
      (let ((start (point)))
	(insert (substring (car symbol-list) 1))
	(subst-char-in-region start (point) ?_ ?-))
      (insert (format "\");\n  staticpro (&%s);\n" (car symbol-list)))
      (setq symbol-list (cdr symbol-list)))))

  */        

/*&&& symbols declared here &&&*/
Lisp_Object Qauto_raise;
Lisp_Object Qauto_lower;
Lisp_Object Qbackground_color;
Lisp_Object Qbar;
Lisp_Object Qborder_color;
Lisp_Object Qborder_width;
Lisp_Object Qbox;
Lisp_Object Qcursor_color;
Lisp_Object Qcursor_type;
Lisp_Object Qfont;
Lisp_Object Qforeground_color;
Lisp_Object Qgeometry;
Lisp_Object Qicon_left;
Lisp_Object Qicon_top;
Lisp_Object Qicon_type;
Lisp_Object Qicon_name;
Lisp_Object Qinternal_border_width;
Lisp_Object Qleft;
Lisp_Object Qmouse_color;
Lisp_Object Qnone;
Lisp_Object Qparent_id;
Lisp_Object Qscroll_bar_width;
Lisp_Object Qsuppress_icon;
Lisp_Object Qtop;
Lisp_Object Qundefined_color;
Lisp_Object Qvertical_scroll_bars;
Lisp_Object Qvisibility;
Lisp_Object Qwindow_id;
Lisp_Object Qx_frame_parameter;
Lisp_Object Qx_resource_name;
Lisp_Object Quser_position;
Lisp_Object Quser_size;
Lisp_Object Qdisplay;

/* The below are defined in frame.c.  */
extern Lisp_Object Qheight, Qminibuffer, Qname, Qonly, Qwidth;
extern Lisp_Object Qunsplittable, Qmenu_bar_lines;

extern Lisp_Object Vwindow_system_version;

extern Lisp_Object last_mouse_scroll_bar;
extern int last_mouse_scroll_bar_pos;
Time last_mouse_movement_time;


/* Extract a frame as a FRAME_PTR, defaulting to the selected frame
   and checking validity for Win32.  */

FRAME_PTR
check_x_frame (frame)
     Lisp_Object frame;
{
  FRAME_PTR f;

  if (NILP (frame))
    f = selected_frame;
  else
    {
      CHECK_LIVE_FRAME (frame, 0);
      f = XFRAME (frame);
    }
  if (! FRAME_WIN32_P (f))
    error ("non-win32 frame used");
  return f;
}

/* Let the user specify an display with a frame.
   nil stands for the selected frame--or, if that is not a win32 frame,
   the first display on the list.  */

static struct win32_display_info *
check_x_display_info (frame)
     Lisp_Object frame;
{
  if (NILP (frame))
    {
      if (FRAME_WIN32_P (selected_frame))
	return FRAME_WIN32_DISPLAY_INFO (selected_frame);
      else
	return &one_win32_display_info;
    }
  else if (STRINGP (frame))
    return x_display_info_for_name (frame);
  else
    {
      FRAME_PTR f;

      CHECK_LIVE_FRAME (frame, 0);
      f = XFRAME (frame);
      if (! FRAME_WIN32_P (f))
	error ("non-win32 frame used");
      return FRAME_WIN32_DISPLAY_INFO (f);
    }
}

/* Return the Emacs frame-object corresponding to an win32 window.
   It could be the frame's main window or an icon window.  */

/* This function can be called during GC, so use GC_xxx type test macros.  */

struct frame *
x_window_to_frame (dpyinfo, wdesc)
     struct win32_display_info *dpyinfo;
     HWND wdesc;
{
  Lisp_Object tail, frame;
  struct frame *f;

  for (tail = Vframe_list; GC_CONSP (tail); tail = XCONS (tail)->cdr)
    {
      frame = XCONS (tail)->car;
      if (!GC_FRAMEP (frame))
        continue;
      f = XFRAME (frame);
      if (f->output_data.nothing == 1 
	  || FRAME_WIN32_DISPLAY_INFO (f) != dpyinfo)
	continue;
      if (FRAME_WIN32_WINDOW (f) == wdesc)
        return f;
    }
  return 0;
}



/* Code to deal with bitmaps.  Bitmaps are referenced by their bitmap
   id, which is just an int that this section returns.  Bitmaps are
   reference counted so they can be shared among frames.

   Bitmap indices are guaranteed to be > 0, so a negative number can
   be used to indicate no bitmap.

   If you use x_create_bitmap_from_data, then you must keep track of
   the bitmaps yourself.  That is, creating a bitmap from the same
   data more than once will not be caught.  */


/* Functions to access the contents of a bitmap, given an id.  */

int
x_bitmap_height (f, id)
     FRAME_PTR f;
     int id;
{
  return FRAME_WIN32_DISPLAY_INFO (f)->bitmaps[id - 1].height;
}

int
x_bitmap_width (f, id)
     FRAME_PTR f;
     int id;
{
  return FRAME_WIN32_DISPLAY_INFO (f)->bitmaps[id - 1].width;
}

int
x_bitmap_pixmap (f, id)
     FRAME_PTR f;
     int id;
{
  return (int) FRAME_WIN32_DISPLAY_INFO (f)->bitmaps[id - 1].pixmap;
}


/* Allocate a new bitmap record.  Returns index of new record.  */

static int
x_allocate_bitmap_record (f)
     FRAME_PTR f;
{
  struct win32_display_info *dpyinfo = FRAME_WIN32_DISPLAY_INFO (f);
  int i;

  if (dpyinfo->bitmaps == NULL)
    {
      dpyinfo->bitmaps_size = 10;
      dpyinfo->bitmaps
	= (struct win32_bitmap_record *) xmalloc (dpyinfo->bitmaps_size * sizeof (struct win32_bitmap_record));
      dpyinfo->bitmaps_last = 1;
      return 1;
    }

  if (dpyinfo->bitmaps_last < dpyinfo->bitmaps_size)
    return ++dpyinfo->bitmaps_last;

  for (i = 0; i < dpyinfo->bitmaps_size; ++i)
    if (dpyinfo->bitmaps[i].refcount == 0)
      return i + 1;

  dpyinfo->bitmaps_size *= 2;
  dpyinfo->bitmaps
    = (struct win32_bitmap_record *) xrealloc (dpyinfo->bitmaps,
					   dpyinfo->bitmaps_size * sizeof (struct win32_bitmap_record));
  return ++dpyinfo->bitmaps_last;
}

/* Add one reference to the reference count of the bitmap with id ID.  */

void
x_reference_bitmap (f, id)
     FRAME_PTR f;
     int id;
{
  ++FRAME_WIN32_DISPLAY_INFO (f)->bitmaps[id - 1].refcount;
}

/* Create a bitmap for frame F from a HEIGHT x WIDTH array of bits at BITS.  */

int
x_create_bitmap_from_data (f, bits, width, height)
     struct frame *f;
     char *bits;
     unsigned int width, height;
{
  struct win32_display_info *dpyinfo = FRAME_WIN32_DISPLAY_INFO (f);
  Pixmap bitmap;
  int id;

  bitmap = CreateBitmap (width, height,
			 FRAME_WIN32_DISPLAY_INFO (XFRAME (frame))->n_planes,
			 FRAME_WIN32_DISPLAY_INFO (XFRAME (frame))->n_cbits,
			 bits);

  if (! bitmap)
    return -1;

  id = x_allocate_bitmap_record (f);
  dpyinfo->bitmaps[id - 1].pixmap = bitmap;
  dpyinfo->bitmaps[id - 1].file = NULL;
  dpyinfo->bitmaps[id - 1].hinst = NULL;
  dpyinfo->bitmaps[id - 1].refcount = 1;
  dpyinfo->bitmaps[id - 1].depth = 1;
  dpyinfo->bitmaps[id - 1].height = height;
  dpyinfo->bitmaps[id - 1].width = width;

  return id;
}

/* Create bitmap from file FILE for frame F.  */

int
x_create_bitmap_from_file (f, file)
     struct frame *f;
     Lisp_Object file;
{
  return -1;
#if 0
  struct win32_display_info *dpyinfo = FRAME_WIN32_DISPLAY_INFO (f);
  unsigned int width, height;
  Pixmap bitmap;
  int xhot, yhot, result, id;
  Lisp_Object found;
  int fd;
  char *filename;
  HINSTANCE hinst;

  /* Look for an existing bitmap with the same name.  */
  for (id = 0; id < dpyinfo->bitmaps_last; ++id)
    {
      if (dpyinfo->bitmaps[id].refcount
	  && dpyinfo->bitmaps[id].file
	  && !strcmp (dpyinfo->bitmaps[id].file, (char *) XSTRING (file)->data))
	{
	  ++dpyinfo->bitmaps[id].refcount;
	  return id + 1;
	}
    }

  /* Search bitmap-file-path for the file, if appropriate.  */
  fd = openp (Vx_bitmap_file_path, file, "", &found, 0);
  if (fd < 0)
    return -1;
  close (fd);

  filename = (char *) XSTRING (found)->data;

  hinst = LoadLibraryEx (filename, NULL, LOAD_LIBRARY_AS_DATAFILE);

  if (hinst == NULL)
      return -1;

  
  result = XReadBitmapFile (FRAME_WIN32_DISPLAY (f), FRAME_WIN32_WINDOW (f),
			    filename, &width, &height, &bitmap, &xhot, &yhot);
  if (result != BitmapSuccess)
    return -1;

  id = x_allocate_bitmap_record (f);
  dpyinfo->bitmaps[id - 1].pixmap = bitmap;
  dpyinfo->bitmaps[id - 1].refcount = 1;
  dpyinfo->bitmaps[id - 1].file = (char *) xmalloc (XSTRING (file)->size + 1);
  dpyinfo->bitmaps[id - 1].depth = 1;
  dpyinfo->bitmaps[id - 1].height = height;
  dpyinfo->bitmaps[id - 1].width = width;
  strcpy (dpyinfo->bitmaps[id - 1].file, XSTRING (file)->data);

  return id;
#endif
}

/* Remove reference to bitmap with id number ID.  */

int
x_destroy_bitmap (f, id)
     FRAME_PTR f;
     int id;
{
  struct win32_display_info *dpyinfo = FRAME_WIN32_DISPLAY_INFO (f);

  if (id > 0)
    {
      --dpyinfo->bitmaps[id - 1].refcount;
      if (dpyinfo->bitmaps[id - 1].refcount == 0)
	{
	  BLOCK_INPUT;
	  DeleteObject (dpyinfo->bitmaps[id - 1].pixmap);
	  if (dpyinfo->bitmaps[id - 1].file)
	    {
	      free (dpyinfo->bitmaps[id - 1].file);
	      dpyinfo->bitmaps[id - 1].file = NULL;
	    }
	  UNBLOCK_INPUT;
	}
    }
}

/* Free all the bitmaps for the display specified by DPYINFO.  */

static void
x_destroy_all_bitmaps (dpyinfo)
     struct win32_display_info *dpyinfo;
{
  int i;
  for (i = 0; i < dpyinfo->bitmaps_last; i++)
    if (dpyinfo->bitmaps[i].refcount > 0)
      {
	DeleteObject (dpyinfo->bitmaps[i].pixmap);
	if (dpyinfo->bitmaps[i].file)
	  free (dpyinfo->bitmaps[i].file);
      }
  dpyinfo->bitmaps_last = 0;
}

/* Connect the frame-parameter names for Win32 frames
   to the ways of passing the parameter values to the window system.

   The name of a parameter, as a Lisp symbol,
   has an `x-frame-parameter' property which is an integer in Lisp
   but can be interpreted as an `enum x_frame_parm' in C.  */

enum x_frame_parm
{
  X_PARM_FOREGROUND_COLOR,
  X_PARM_BACKGROUND_COLOR,
  X_PARM_MOUSE_COLOR,
  X_PARM_CURSOR_COLOR,
  X_PARM_BORDER_COLOR,
  X_PARM_ICON_TYPE,
  X_PARM_FONT,
  X_PARM_BORDER_WIDTH,
  X_PARM_INTERNAL_BORDER_WIDTH,
  X_PARM_NAME,
  X_PARM_AUTORAISE,
  X_PARM_AUTOLOWER,
  X_PARM_VERT_SCROLL_BAR,
  X_PARM_VISIBILITY,
  X_PARM_MENU_BAR_LINES
};


struct x_frame_parm_table
{
  char *name;
  void (*setter)( /* struct frame *frame, Lisp_Object val, oldval */ );
};

void x_set_foreground_color ();
void x_set_background_color ();
void x_set_mouse_color ();
void x_set_cursor_color ();
void x_set_border_color ();
void x_set_cursor_type ();
void x_set_icon_type ();
void x_set_icon_name ();
void x_set_font ();
void x_set_border_width ();
void x_set_internal_border_width ();
void x_explicitly_set_name ();
void x_set_autoraise ();
void x_set_autolower ();
void x_set_vertical_scroll_bars ();
void x_set_visibility ();
void x_set_menu_bar_lines ();
void x_set_scroll_bar_width ();
void x_set_unsplittable ();

static struct x_frame_parm_table x_frame_parms[] =
{
  "foreground-color", x_set_foreground_color,
  "background-color", x_set_background_color,
  "mouse-color", x_set_mouse_color,
  "cursor-color", x_set_cursor_color,
  "border-color", x_set_border_color,
  "cursor-type", x_set_cursor_type,
  "icon-type", x_set_icon_type,
  "icon-name", x_set_icon_name,
  "font", x_set_font,
  "border-width", x_set_border_width,
  "internal-border-width", x_set_internal_border_width,
  "name", x_explicitly_set_name,
  "auto-raise", x_set_autoraise,
  "auto-lower", x_set_autolower,
  "vertical-scroll-bars", x_set_vertical_scroll_bars,
  "visibility", x_set_visibility,
  "menu-bar-lines", x_set_menu_bar_lines,
  "scroll-bar-width", x_set_scroll_bar_width,
  "unsplittable", x_set_unsplittable,
};

/* Attach the `x-frame-parameter' properties to
   the Lisp symbol names of parameters relevant to Win32.  */

init_x_parm_symbols ()
{
  int i;

  for (i = 0; i < sizeof (x_frame_parms) / sizeof (x_frame_parms[0]); i++)
    Fput (intern (x_frame_parms[i].name), Qx_frame_parameter,
	  make_number (i));
}

/* Change the parameters of FRAME as specified by ALIST.
   If a parameter is not specially recognized, do nothing;
   otherwise call the `x_set_...' function for that parameter.  */

void
x_set_frame_parameters (f, alist)
     FRAME_PTR f;
     Lisp_Object alist;
{
  Lisp_Object tail;

  /* If both of these parameters are present, it's more efficient to
     set them both at once.  So we wait until we've looked at the
     entire list before we set them.  */
  Lisp_Object width, height;

  /* Same here.  */
  Lisp_Object left, top;

  /* Same with these.  */
  Lisp_Object icon_left, icon_top;

  /* Record in these vectors all the parms specified.  */
  Lisp_Object *parms;
  Lisp_Object *values;
  int i;
  int left_no_change = 0, top_no_change = 0;
  int icon_left_no_change = 0, icon_top_no_change = 0;

  i = 0;
  for (tail = alist; CONSP (tail); tail = Fcdr (tail))
    i++;

  parms = (Lisp_Object *) alloca (i * sizeof (Lisp_Object));
  values = (Lisp_Object *) alloca (i * sizeof (Lisp_Object));

  /* Extract parm names and values into those vectors.  */

  i = 0;
  for (tail = alist; CONSP (tail); tail = Fcdr (tail))
    {
      Lisp_Object elt, prop, val;

      elt = Fcar (tail);
      parms[i] = Fcar (elt);
      values[i] = Fcdr (elt);
      i++;
    }

  width = height = top = left = Qunbound;
  icon_left = icon_top = Qunbound;

  /* Now process them in reverse of specified order.  */
  for (i--; i >= 0; i--)
    {
      Lisp_Object prop, val;

      prop = parms[i];
      val = values[i];

      if (EQ (prop, Qwidth))
	width = val;
      else if (EQ (prop, Qheight))
	height = val;
      else if (EQ (prop, Qtop))
	top = val;
      else if (EQ (prop, Qleft))
	left = val;
      else if (EQ (prop, Qicon_top))
	icon_top = val;
      else if (EQ (prop, Qicon_left))
	icon_left = val;
      else
	{
	  register Lisp_Object param_index, old_value;

	  param_index = Fget (prop, Qx_frame_parameter);
	  old_value = get_frame_param (f, prop);
	  store_frame_param (f, prop, val);
 	  if (NATNUMP (param_index)
	      && (XFASTINT (param_index)
		  < sizeof (x_frame_parms)/sizeof (x_frame_parms[0])))
	    (*x_frame_parms[XINT (param_index)].setter)(f, val, old_value);
	}
    }

  /* Don't die if just one of these was set.  */
  if (EQ (left, Qunbound))
    {
      left_no_change = 1;
      if (f->output_data.win32->left_pos < 0)
	left = Fcons (Qplus, Fcons (make_number (f->output_data.win32->left_pos), Qnil));
      else
	XSETINT (left, f->output_data.win32->left_pos);
    }
  if (EQ (top, Qunbound))
    {
      top_no_change = 1;
      if (f->output_data.win32->top_pos < 0)
	top = Fcons (Qplus, Fcons (make_number (f->output_data.win32->top_pos), Qnil));
      else
	XSETINT (top, f->output_data.win32->top_pos);
    }

  /* If one of the icon positions was not set, preserve or default it.  */
  if (EQ (icon_left, Qunbound) || ! INTEGERP (icon_left))
    {
      icon_left_no_change = 1;
      icon_left = Fcdr (Fassq (Qicon_left, f->param_alist));
      if (NILP (icon_left))
	XSETINT (icon_left, 0);
    }
  if (EQ (icon_top, Qunbound) || ! INTEGERP (icon_top))
    {
      icon_top_no_change = 1;
      icon_top = Fcdr (Fassq (Qicon_top, f->param_alist));
      if (NILP (icon_top))
	XSETINT (icon_top, 0);
    }

  /* Don't die if just one of these was set.  */
  if (EQ (width, Qunbound))
    XSETINT (width, FRAME_WIDTH (f));
  if (EQ (height, Qunbound))
    XSETINT (height, FRAME_HEIGHT (f));

  /* Don't set these parameters unless they've been explicitly
     specified.  The window might be mapped or resized while we're in
     this function, and we don't want to override that unless the lisp
     code has asked for it.

     Don't set these parameters unless they actually differ from the
     window's current parameters; the window may not actually exist
     yet.  */
  {
    Lisp_Object frame;

    check_frame_size (f, &height, &width);

    XSETFRAME (frame, f);

    if ((NUMBERP (width) && XINT (width) != FRAME_WIDTH (f))
	|| (NUMBERP (height) && XINT (height) != FRAME_HEIGHT (f)))
      Fset_frame_size (frame, width, height);

    if ((!NILP (left) || !NILP (top))
	&& ! (left_no_change && top_no_change)
	&& ! (NUMBERP (left) && XINT (left) == f->output_data.win32->left_pos
	      && NUMBERP (top) && XINT (top) == f->output_data.win32->top_pos))
      {
	int leftpos = 0;
	int toppos = 0;

	/* Record the signs.  */
	f->output_data.win32->size_hint_flags &= ~ (XNegative | YNegative);
	if (EQ (left, Qminus))
	  f->output_data.win32->size_hint_flags |= XNegative;
	else if (INTEGERP (left))
	  {
	    leftpos = XINT (left);
	    if (leftpos < 0)
	      f->output_data.win32->size_hint_flags |= XNegative;
	  }
	else if (CONSP (left) && EQ (XCONS (left)->car, Qminus)
		 && CONSP (XCONS (left)->cdr)
		 && INTEGERP (XCONS (XCONS (left)->cdr)->car))
	  {
	    leftpos = - XINT (XCONS (XCONS (left)->cdr)->car);
	    f->output_data.win32->size_hint_flags |= XNegative;
	  }
	else if (CONSP (left) && EQ (XCONS (left)->car, Qplus)
		 && CONSP (XCONS (left)->cdr)
		 && INTEGERP (XCONS (XCONS (left)->cdr)->car))
	  {
	    leftpos = XINT (XCONS (XCONS (left)->cdr)->car);
	  }

	if (EQ (top, Qminus))
	  f->output_data.win32->size_hint_flags |= YNegative;
	else if (INTEGERP (top))
	  {
	    toppos = XINT (top);
	    if (toppos < 0)
	      f->output_data.win32->size_hint_flags |= YNegative;
	  }
	else if (CONSP (top) && EQ (XCONS (top)->car, Qminus)
		 && CONSP (XCONS (top)->cdr)
		 && INTEGERP (XCONS (XCONS (top)->cdr)->car))
	  {
	    toppos = - XINT (XCONS (XCONS (top)->cdr)->car);
	    f->output_data.win32->size_hint_flags |= YNegative;
	  }
	else if (CONSP (top) && EQ (XCONS (top)->car, Qplus)
		 && CONSP (XCONS (top)->cdr)
		 && INTEGERP (XCONS (XCONS (top)->cdr)->car))
	  {
	    toppos = XINT (XCONS (XCONS (top)->cdr)->car);
	  }


	/* Store the numeric value of the position.  */
	f->output_data.win32->top_pos = toppos;
	f->output_data.win32->left_pos = leftpos;

	f->output_data.win32->win_gravity = NorthWestGravity;

	/* Actually set that position, and convert to absolute.  */
	x_set_offset (f, leftpos, toppos, -1);
      }

    if ((!NILP (icon_left) || !NILP (icon_top))
	&& ! (icon_left_no_change && icon_top_no_change))
      x_wm_set_icon_position (f, XINT (icon_left), XINT (icon_top));
  }
}

/* Store the screen positions of frame F into XPTR and YPTR.
   These are the positions of the containing window manager window,
   not Emacs's own window.  */

void
x_real_positions (f, xptr, yptr)
     FRAME_PTR f;
     int *xptr, *yptr;
{
  POINT pt;

  {
      RECT rect;
      
      GetClientRect(FRAME_WIN32_WINDOW(f), &rect);
      AdjustWindowRect(&rect, f->output_data.win32->dwStyle, FRAME_EXTERNAL_MENU_BAR(f));
      
      pt.x = rect.left;
      pt.y = rect.top;
  }

  ClientToScreen (FRAME_WIN32_WINDOW(f), &pt);

  *xptr = pt.x;
  *yptr = pt.y;
}

/* Insert a description of internally-recorded parameters of frame X
   into the parameter alist *ALISTPTR that is to be given to the user.
   Only parameters that are specific to Win32
   and whose values are not correctly recorded in the frame's
   param_alist need to be considered here.  */

x_report_frame_params (f, alistptr)
     struct frame *f;
     Lisp_Object *alistptr;
{
  char buf[16];
  Lisp_Object tem;

  /* Represent negative positions (off the top or left screen edge)
     in a way that Fmodify_frame_parameters will understand correctly.  */
  XSETINT (tem, f->output_data.win32->left_pos);
  if (f->output_data.win32->left_pos >= 0)
    store_in_alist (alistptr, Qleft, tem);
  else
    store_in_alist (alistptr, Qleft, Fcons (Qplus, Fcons (tem, Qnil)));

  XSETINT (tem, f->output_data.win32->top_pos);
  if (f->output_data.win32->top_pos >= 0)
    store_in_alist (alistptr, Qtop, tem);
  else
    store_in_alist (alistptr, Qtop, Fcons (Qplus, Fcons (tem, Qnil)));

  store_in_alist (alistptr, Qborder_width,
       	   make_number (f->output_data.win32->border_width));
  store_in_alist (alistptr, Qinternal_border_width,
       	   make_number (f->output_data.win32->internal_border_width));
  sprintf (buf, "%ld", (long) FRAME_WIN32_WINDOW (f));
  store_in_alist (alistptr, Qwindow_id,
       	   build_string (buf));
  store_in_alist (alistptr, Qicon_name, f->icon_name);
  FRAME_SAMPLE_VISIBILITY (f);
  store_in_alist (alistptr, Qvisibility,
		  (FRAME_VISIBLE_P (f) ? Qt
		   : FRAME_ICONIFIED_P (f) ? Qicon : Qnil));
  store_in_alist (alistptr, Qdisplay,
		  XCONS (FRAME_WIN32_DISPLAY_INFO (f)->name_list_element)->car);
}


#if 0
DEFUN ("win32-rgb", Fwin32_rgb, Swin32_rgb, 3, 3, 0,
       "Convert RGB numbers to a windows color reference.")
    (red, green, blue)
    Lisp_Object red, green, blue;
{
    Lisp_Object rgb;

    CHECK_NUMBER (red, 0);
    CHECK_NUMBER (green, 0);
    CHECK_NUMBER (blue, 0);

    XSET (rgb, Lisp_Int, RGB(XUINT(red), XUINT(green), XUINT(blue)));

    return (rgb);
}


#else
/* The default colors for the win32 color map */
typedef struct colormap_t 
{
  char *name;
  COLORREF colorref;
} colormap_t;

colormap_t win32_color_map[] = 
{
  {"snow"                      , RGB (255,250,250)},
  {"ghost white"               , RGB (248,248,255)},
  {"GhostWhite"                , RGB (248,248,255)},
  {"white smoke"               , RGB (245,245,245)},
  {"WhiteSmoke"                , RGB (245,245,245)},
  {"gainsboro"                 , RGB (220,220,220)},
  {"floral white"              , RGB (255,250,240)},
  {"FloralWhite"               , RGB (255,250,240)},
  {"old lace"                  , RGB (253,245,230)},
  {"OldLace"                   , RGB (253,245,230)},
  {"linen"                     , RGB (250,240,230)},
  {"antique white"             , RGB (250,235,215)},
  {"AntiqueWhite"              , RGB (250,235,215)},
  {"papaya whip"               , RGB (255,239,213)},
  {"PapayaWhip"                , RGB (255,239,213)},
  {"blanched almond"           , RGB (255,235,205)},
  {"BlanchedAlmond"            , RGB (255,235,205)},
  {"bisque"                    , RGB (255,228,196)},
  {"peach puff"                , RGB (255,218,185)},
  {"PeachPuff"                 , RGB (255,218,185)},
  {"navajo white"              , RGB (255,222,173)},
  {"NavajoWhite"               , RGB (255,222,173)},
  {"moccasin"                  , RGB (255,228,181)},
  {"cornsilk"                  , RGB (255,248,220)},
  {"ivory"                     , RGB (255,255,240)},
  {"lemon chiffon"             , RGB (255,250,205)},
  {"LemonChiffon"              , RGB (255,250,205)},
  {"seashell"                  , RGB (255,245,238)},
  {"honeydew"                  , RGB (240,255,240)},
  {"mint cream"                , RGB (245,255,250)},
  {"MintCream"                 , RGB (245,255,250)},
  {"azure"                     , RGB (240,255,255)},
  {"alice blue"                , RGB (240,248,255)},
  {"AliceBlue"                 , RGB (240,248,255)},
  {"lavender"                  , RGB (230,230,250)},
  {"lavender blush"            , RGB (255,240,245)},
  {"LavenderBlush"             , RGB (255,240,245)},
  {"misty rose"                , RGB (255,228,225)},
  {"MistyRose"                 , RGB (255,228,225)},
  {"white"                     , RGB (255,255,255)},
  {"black"                     , RGB (  0,  0,  0)},
  {"dark slate gray"           , RGB ( 47, 79, 79)},
  {"DarkSlateGray"             , RGB ( 47, 79, 79)},
  {"dark slate grey"           , RGB ( 47, 79, 79)},
  {"DarkSlateGrey"             , RGB ( 47, 79, 79)},
  {"dim gray"                  , RGB (105,105,105)},
  {"DimGray"                   , RGB (105,105,105)},
  {"dim grey"                  , RGB (105,105,105)},
  {"DimGrey"                   , RGB (105,105,105)},
  {"slate gray"                , RGB (112,128,144)},
  {"SlateGray"                 , RGB (112,128,144)},
  {"slate grey"                , RGB (112,128,144)},
  {"SlateGrey"                 , RGB (112,128,144)},
  {"light slate gray"          , RGB (119,136,153)},
  {"LightSlateGray"            , RGB (119,136,153)},
  {"light slate grey"          , RGB (119,136,153)},
  {"LightSlateGrey"            , RGB (119,136,153)},
  {"gray"                      , RGB (190,190,190)},
  {"grey"                      , RGB (190,190,190)},
  {"light grey"                , RGB (211,211,211)},
  {"LightGrey"                 , RGB (211,211,211)},
  {"light gray"                , RGB (211,211,211)},
  {"LightGray"                 , RGB (211,211,211)},
  {"midnight blue"             , RGB ( 25, 25,112)},
  {"MidnightBlue"              , RGB ( 25, 25,112)},
  {"navy"                      , RGB (  0,  0,128)},
  {"navy blue"                 , RGB (  0,  0,128)},
  {"NavyBlue"                  , RGB (  0,  0,128)},
  {"cornflower blue"           , RGB (100,149,237)},
  {"CornflowerBlue"            , RGB (100,149,237)},
  {"dark slate blue"           , RGB ( 72, 61,139)},
  {"DarkSlateBlue"             , RGB ( 72, 61,139)},
  {"slate blue"                , RGB (106, 90,205)},
  {"SlateBlue"                 , RGB (106, 90,205)},
  {"medium slate blue"         , RGB (123,104,238)},
  {"MediumSlateBlue"           , RGB (123,104,238)},
  {"light slate blue"          , RGB (132,112,255)},
  {"LightSlateBlue"            , RGB (132,112,255)},
  {"medium blue"               , RGB (  0,  0,205)},
  {"MediumBlue"                , RGB (  0,  0,205)},
  {"royal blue"                , RGB ( 65,105,225)},
  {"RoyalBlue"                 , RGB ( 65,105,225)},
  {"blue"                      , RGB (  0,  0,255)},
  {"dodger blue"               , RGB ( 30,144,255)},
  {"DodgerBlue"                , RGB ( 30,144,255)},
  {"deep sky blue"             , RGB (  0,191,255)},
  {"DeepSkyBlue"               , RGB (  0,191,255)},
  {"sky blue"                  , RGB (135,206,235)},
  {"SkyBlue"                   , RGB (135,206,235)},
  {"light sky blue"            , RGB (135,206,250)},
  {"LightSkyBlue"              , RGB (135,206,250)},
  {"steel blue"                , RGB ( 70,130,180)},
  {"SteelBlue"                 , RGB ( 70,130,180)},
  {"light steel blue"          , RGB (176,196,222)},
  {"LightSteelBlue"            , RGB (176,196,222)},
  {"light blue"                , RGB (173,216,230)},
  {"LightBlue"                 , RGB (173,216,230)},
  {"powder blue"               , RGB (176,224,230)},
  {"PowderBlue"                , RGB (176,224,230)},
  {"pale turquoise"            , RGB (175,238,238)},
  {"PaleTurquoise"             , RGB (175,238,238)},
  {"dark turquoise"            , RGB (  0,206,209)},
  {"DarkTurquoise"             , RGB (  0,206,209)},
  {"medium turquoise"          , RGB ( 72,209,204)},
  {"MediumTurquoise"           , RGB ( 72,209,204)},
  {"turquoise"                 , RGB ( 64,224,208)},
  {"cyan"                      , RGB (  0,255,255)},
  {"light cyan"                , RGB (224,255,255)},
  {"LightCyan"                 , RGB (224,255,255)},
  {"cadet blue"                , RGB ( 95,158,160)},
  {"CadetBlue"                 , RGB ( 95,158,160)},
  {"medium aquamarine"         , RGB (102,205,170)},
  {"MediumAquamarine"          , RGB (102,205,170)},
  {"aquamarine"                , RGB (127,255,212)},
  {"dark green"                , RGB (  0,100,  0)},
  {"DarkGreen"                 , RGB (  0,100,  0)},
  {"dark olive green"          , RGB ( 85,107, 47)},
  {"DarkOliveGreen"            , RGB ( 85,107, 47)},
  {"dark sea green"            , RGB (143,188,143)},
  {"DarkSeaGreen"              , RGB (143,188,143)},
  {"sea green"                 , RGB ( 46,139, 87)},
  {"SeaGreen"                  , RGB ( 46,139, 87)},
  {"medium sea green"          , RGB ( 60,179,113)},
  {"MediumSeaGreen"            , RGB ( 60,179,113)},
  {"light sea green"           , RGB ( 32,178,170)},
  {"LightSeaGreen"             , RGB ( 32,178,170)},
  {"pale green"                , RGB (152,251,152)},
  {"PaleGreen"                 , RGB (152,251,152)},
  {"spring green"              , RGB (  0,255,127)},
  {"SpringGreen"               , RGB (  0,255,127)},
  {"lawn green"                , RGB (124,252,  0)},
  {"LawnGreen"                 , RGB (124,252,  0)},
  {"green"                     , RGB (  0,255,  0)},
  {"chartreuse"                , RGB (127,255,  0)},
  {"medium spring green"       , RGB (  0,250,154)},
  {"MediumSpringGreen"         , RGB (  0,250,154)},
  {"green yellow"              , RGB (173,255, 47)},
  {"GreenYellow"               , RGB (173,255, 47)},
  {"lime green"                , RGB ( 50,205, 50)},
  {"LimeGreen"                 , RGB ( 50,205, 50)},
  {"yellow green"              , RGB (154,205, 50)},
  {"YellowGreen"               , RGB (154,205, 50)},
  {"forest green"              , RGB ( 34,139, 34)},
  {"ForestGreen"               , RGB ( 34,139, 34)},
  {"olive drab"                , RGB (107,142, 35)},
  {"OliveDrab"                 , RGB (107,142, 35)},
  {"dark khaki"                , RGB (189,183,107)},
  {"DarkKhaki"                 , RGB (189,183,107)},
  {"khaki"                     , RGB (240,230,140)},
  {"pale goldenrod"            , RGB (238,232,170)},
  {"PaleGoldenrod"             , RGB (238,232,170)},
  {"light goldenrod yellow"    , RGB (250,250,210)},
  {"LightGoldenrodYellow"      , RGB (250,250,210)},
  {"light yellow"              , RGB (255,255,224)},
  {"LightYellow"               , RGB (255,255,224)},
  {"yellow"                    , RGB (255,255,  0)},
  {"gold"                      , RGB (255,215,  0)},
  {"light goldenrod"           , RGB (238,221,130)},
  {"LightGoldenrod"            , RGB (238,221,130)},
  {"goldenrod"                 , RGB (218,165, 32)},
  {"dark goldenrod"            , RGB (184,134, 11)},
  {"DarkGoldenrod"             , RGB (184,134, 11)},
  {"rosy brown"                , RGB (188,143,143)},
  {"RosyBrown"                 , RGB (188,143,143)},
  {"indian red"                , RGB (205, 92, 92)},
  {"IndianRed"                 , RGB (205, 92, 92)},
  {"saddle brown"              , RGB (139, 69, 19)},
  {"SaddleBrown"               , RGB (139, 69, 19)},
  {"sienna"                    , RGB (160, 82, 45)},
  {"peru"                      , RGB (205,133, 63)},
  {"burlywood"                 , RGB (222,184,135)},
  {"beige"                     , RGB (245,245,220)},
  {"wheat"                     , RGB (245,222,179)},
  {"sandy brown"               , RGB (244,164, 96)},
  {"SandyBrown"                , RGB (244,164, 96)},
  {"tan"                       , RGB (210,180,140)},
  {"chocolate"                 , RGB (210,105, 30)},
  {"firebrick"                 , RGB (178,34, 34)},
  {"brown"                     , RGB (165,42, 42)},
  {"dark salmon"               , RGB (233,150,122)},
  {"DarkSalmon"                , RGB (233,150,122)},
  {"salmon"                    , RGB (250,128,114)},
  {"light salmon"              , RGB (255,160,122)},
  {"LightSalmon"               , RGB (255,160,122)},
  {"orange"                    , RGB (255,165,  0)},
  {"dark orange"               , RGB (255,140,  0)},
  {"DarkOrange"                , RGB (255,140,  0)},
  {"coral"                     , RGB (255,127, 80)},
  {"light coral"               , RGB (240,128,128)},
  {"LightCoral"                , RGB (240,128,128)},
  {"tomato"                    , RGB (255, 99, 71)},
  {"orange red"                , RGB (255, 69,  0)},
  {"OrangeRed"                 , RGB (255, 69,  0)},
  {"red"                       , RGB (255,  0,  0)},
  {"hot pink"                  , RGB (255,105,180)},
  {"HotPink"                   , RGB (255,105,180)},
  {"deep pink"                 , RGB (255, 20,147)},
  {"DeepPink"                  , RGB (255, 20,147)},
  {"pink"                      , RGB (255,192,203)},
  {"light pink"                , RGB (255,182,193)},
  {"LightPink"                 , RGB (255,182,193)},
  {"pale violet red"           , RGB (219,112,147)},
  {"PaleVioletRed"             , RGB (219,112,147)},
  {"maroon"                    , RGB (176, 48, 96)},
  {"medium violet red"         , RGB (199, 21,133)},
  {"MediumVioletRed"           , RGB (199, 21,133)},
  {"violet red"                , RGB (208, 32,144)},
  {"VioletRed"                 , RGB (208, 32,144)},
  {"magenta"                   , RGB (255,  0,255)},
  {"violet"                    , RGB (238,130,238)},
  {"plum"                      , RGB (221,160,221)},
  {"orchid"                    , RGB (218,112,214)},
  {"medium orchid"             , RGB (186, 85,211)},
  {"MediumOrchid"              , RGB (186, 85,211)},
  {"dark orchid"               , RGB (153, 50,204)},
  {"DarkOrchid"                , RGB (153, 50,204)},
  {"dark violet"               , RGB (148,  0,211)},
  {"DarkViolet"                , RGB (148,  0,211)},
  {"blue violet"               , RGB (138, 43,226)},
  {"BlueViolet"                , RGB (138, 43,226)},
  {"purple"                    , RGB (160, 32,240)},
  {"medium purple"             , RGB (147,112,219)},
  {"MediumPurple"              , RGB (147,112,219)},
  {"thistle"                   , RGB (216,191,216)},
  {"gray0"                     , RGB (  0,  0,  0)},
  {"grey0"                     , RGB (  0,  0,  0)},
  {"dark grey"                 , RGB (169,169,169)},
  {"DarkGrey"                  , RGB (169,169,169)},
  {"dark gray"                 , RGB (169,169,169)},
  {"DarkGray"                  , RGB (169,169,169)},
  {"dark blue"                 , RGB (  0,  0,139)},
  {"DarkBlue"                  , RGB (  0,  0,139)},
  {"dark cyan"                 , RGB (  0,139,139)},
  {"DarkCyan"                  , RGB (  0,139,139)},
  {"dark magenta"              , RGB (139,  0,139)},
  {"DarkMagenta"               , RGB (139,  0,139)},
  {"dark red"                  , RGB (139,  0,  0)},
  {"DarkRed"                   , RGB (139,  0,  0)},
  {"light green"               , RGB (144,238,144)},
  {"LightGreen"                , RGB (144,238,144)},
};

DEFUN ("win32-default-color-map", Fwin32_default_color_map, Swin32_default_color_map,
       0, 0, 0, "Return the default color map.")
     ()
{
  int i;
  colormap_t *pc = win32_color_map;
  Lisp_Object cmap;
  
  BLOCK_INPUT;
  
  cmap = Qnil;
  
  for (i = 0; i < sizeof (win32_color_map) / sizeof (win32_color_map[0]); 
       pc++, i++)
    cmap = Fcons (Fcons (build_string (pc->name),
			 make_number (pc->colorref)),
		  cmap);
  
  UNBLOCK_INPUT;
  
  return (cmap);
}
#endif

Lisp_Object 
win32_to_x_color (rgb)
     Lisp_Object rgb;
{
  Lisp_Object color;
  
  CHECK_NUMBER (rgb, 0);
  
  BLOCK_INPUT;
  
  color = Frassq (rgb, Vwin32_color_map);
  
  UNBLOCK_INPUT;
  
  if (!NILP (color))
    return (Fcar (color));
  else
    return Qnil;
}

COLORREF 
x_to_win32_color (colorname)
     char * colorname;
{
  register Lisp_Object tail, ret = Qnil;
  
  BLOCK_INPUT;
  
  for (tail = Vwin32_color_map; !NILP (tail); tail = Fcdr (tail))
    {
      register Lisp_Object elt, tem;

      elt = Fcar (tail);
      if (!CONSP (elt)) continue;

      tem = Fcar (elt);

      if (lstrcmpi (XSTRING (tem)->data, colorname) == 0)
	{
	  ret = XUINT(Fcdr (elt));
	  break;
	}

      QUIT;
    }
  
  UNBLOCK_INPUT;
  
  return ret;
}

/* Decide if color named COLOR is valid for the display associated with
   the selected frame; if so, return the rgb values in COLOR_DEF.
   If ALLOC is nonzero, allocate a new colormap cell.  */

int
defined_color (f, color, color_def, alloc)
     FRAME_PTR f;
     char *color;
     COLORREF *color_def;
     int alloc;
{
  register Lisp_Object tem;

  tem = x_to_win32_color (color);

  if (!NILP (tem)) 
    {
      *color_def = XUINT (tem);
      return 1;
    } 
  else 
    {
      return 0;
    }
}

/* Given a string ARG naming a color, compute a pixel value from it
   suitable for screen F.
   If F is not a color screen, return DEF (default) regardless of what
   ARG says.  */

int
x_decode_color (f, arg, def)
     FRAME_PTR f;
     Lisp_Object arg;
     int def;
{
  COLORREF cdef;

  CHECK_STRING (arg, 0);

  if (strcmp (XSTRING (arg)->data, "black") == 0)
    return BLACK_PIX_DEFAULT (f);
  else if (strcmp (XSTRING (arg)->data, "white") == 0)
    return WHITE_PIX_DEFAULT (f);

  if ((FRAME_WIN32_DISPLAY_INFO (f)->n_planes * FRAME_WIN32_DISPLAY_INFO (f)->n_cbits) == 1)
    return def;

  /* defined_color is responsible for coping with failures
     by looking for a near-miss.  */
  if (defined_color (f, XSTRING (arg)->data, &cdef, 1))
    return cdef;

  /* defined_color failed; return an ultimate default.  */
  return def;
}

/* Functions called only from `x_set_frame_param'
   to set individual parameters.

   If FRAME_WIN32_WINDOW (f) is 0,
   the frame is being created and its window does not exist yet.
   In that case, just record the parameter's new value
   in the standard place; do not attempt to change the window.  */

void
x_set_foreground_color (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  f->output_data.win32->foreground_pixel
    = x_decode_color (f, arg, BLACK_PIX_DEFAULT (f));
  if (FRAME_WIN32_WINDOW (f) != 0)
    {
      recompute_basic_faces (f);
      if (FRAME_VISIBLE_P (f))
        redraw_frame (f);
    }
}

void
x_set_background_color (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  Pixmap temp;
  int mask;

  f->output_data.win32->background_pixel
    = x_decode_color (f, arg, WHITE_PIX_DEFAULT (f));

  if (FRAME_WIN32_WINDOW (f) != 0)
    {
      SetWindowLong (FRAME_WIN32_WINDOW (f), WND_BACKGROUND_INDEX, f->output_data.win32->background_pixel);

      recompute_basic_faces (f);

      if (FRAME_VISIBLE_P (f))
        redraw_frame (f);
    }
}

void
x_set_mouse_color (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
#if 0
  Cursor cursor, nontext_cursor, mode_cursor, cross_cursor;
#endif
  int mask_color;

  if (!EQ (Qnil, arg))
    f->output_data.win32->mouse_pixel
      = x_decode_color (f, arg, BLACK_PIX_DEFAULT (f));
  mask_color = f->output_data.win32->background_pixel;
				/* No invisible pointers.  */
  if (mask_color == f->output_data.win32->mouse_pixel
	&& mask_color == f->output_data.win32->background_pixel)
    f->output_data.win32->mouse_pixel = f->output_data.win32->foreground_pixel;

#if 0
  BLOCK_INPUT;

  /* It's not okay to crash if the user selects a screwy cursor.  */
  x_catch_errors (FRAME_WIN32_DISPLAY (f));

  if (!EQ (Qnil, Vx_pointer_shape))
    {
      CHECK_NUMBER (Vx_pointer_shape, 0);
      cursor = XCreateFontCursor (FRAME_WIN32_DISPLAY (f), XINT (Vx_pointer_shape));
    }
  else
    cursor = XCreateFontCursor (FRAME_WIN32_DISPLAY (f), XC_xterm);
  x_check_errors (FRAME_WIN32_DISPLAY (f), "bad text pointer cursor: %s");

  if (!EQ (Qnil, Vx_nontext_pointer_shape))
    {
      CHECK_NUMBER (Vx_nontext_pointer_shape, 0);
      nontext_cursor = XCreateFontCursor (FRAME_WIN32_DISPLAY (f),
					  XINT (Vx_nontext_pointer_shape));
    }
  else
    nontext_cursor = XCreateFontCursor (FRAME_WIN32_DISPLAY (f), XC_left_ptr);
  x_check_errors (FRAME_WIN32_DISPLAY (f), "bad nontext pointer cursor: %s");

  if (!EQ (Qnil, Vx_mode_pointer_shape))
    {
      CHECK_NUMBER (Vx_mode_pointer_shape, 0);
      mode_cursor = XCreateFontCursor (FRAME_WIN32_DISPLAY (f),
				       XINT (Vx_mode_pointer_shape));
    }
  else
    mode_cursor = XCreateFontCursor (FRAME_WIN32_DISPLAY (f), XC_xterm);
  x_check_errors (FRAME_WIN32_DISPLAY (f), "bad modeline pointer cursor: %s");

  if (!EQ (Qnil, Vx_sensitive_text_pointer_shape))
    {
      CHECK_NUMBER (Vx_sensitive_text_pointer_shape, 0);
      cross_cursor
	= XCreateFontCursor (FRAME_WIN32_DISPLAY (f),
			     XINT (Vx_sensitive_text_pointer_shape));
    }
  else
    cross_cursor = XCreateFontCursor (FRAME_WIN32_DISPLAY (f), XC_crosshair);

  /* Check and report errors with the above calls.  */
  x_check_errors (FRAME_WIN32_DISPLAY (f), "can't set cursor shape: %s");
  x_uncatch_errors (FRAME_WIN32_DISPLAY (f));

  {
    XColor fore_color, back_color;

    fore_color.pixel = f->output_data.win32->mouse_pixel;
    back_color.pixel = mask_color;
    XQueryColor (FRAME_WIN32_DISPLAY (f),
		 DefaultColormap (FRAME_WIN32_DISPLAY (f),
				  DefaultScreen (FRAME_WIN32_DISPLAY (f))),
		 &fore_color);
    XQueryColor (FRAME_WIN32_DISPLAY (f),
		 DefaultColormap (FRAME_WIN32_DISPLAY (f),
				  DefaultScreen (FRAME_WIN32_DISPLAY (f))),
		 &back_color);
    XRecolorCursor (FRAME_WIN32_DISPLAY (f), cursor,
		    &fore_color, &back_color);
    XRecolorCursor (FRAME_WIN32_DISPLAY (f), nontext_cursor,
		    &fore_color, &back_color);
    XRecolorCursor (FRAME_WIN32_DISPLAY (f), mode_cursor,
		    &fore_color, &back_color);
    XRecolorCursor (FRAME_WIN32_DISPLAY (f), cross_cursor,
                    &fore_color, &back_color);
  }

  if (FRAME_WIN32_WINDOW (f) != 0)
    {
      XDefineCursor (FRAME_WIN32_DISPLAY (f), FRAME_WIN32_WINDOW (f), cursor);
    }

  if (cursor != f->output_data.win32->text_cursor && f->output_data.win32->text_cursor != 0)
    XFreeCursor (FRAME_WIN32_DISPLAY (f), f->output_data.win32->text_cursor);
  f->output_data.win32->text_cursor = cursor;

  if (nontext_cursor != f->output_data.win32->nontext_cursor
      && f->output_data.win32->nontext_cursor != 0)
    XFreeCursor (FRAME_WIN32_DISPLAY (f), f->output_data.win32->nontext_cursor);
  f->output_data.win32->nontext_cursor = nontext_cursor;

  if (mode_cursor != f->output_data.win32->modeline_cursor
      && f->output_data.win32->modeline_cursor != 0)
    XFreeCursor (FRAME_WIN32_DISPLAY (f), f->output_data.win32->modeline_cursor);
  f->output_data.win32->modeline_cursor = mode_cursor;
  if (cross_cursor != f->output_data.win32->cross_cursor
      && f->output_data.win32->cross_cursor != 0)
    XFreeCursor (FRAME_WIN32_DISPLAY (f), f->output_data.win32->cross_cursor);
  f->output_data.win32->cross_cursor = cross_cursor;

  XFlush (FRAME_WIN32_DISPLAY (f));
  UNBLOCK_INPUT;
#endif
}

void
x_set_cursor_color (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  unsigned long fore_pixel;

  if (!EQ (Vx_cursor_fore_pixel, Qnil))
    fore_pixel = x_decode_color (f, Vx_cursor_fore_pixel,
				 WHITE_PIX_DEFAULT (f));
  else
    fore_pixel = f->output_data.win32->background_pixel;
  f->output_data.win32->cursor_pixel = x_decode_color (f, arg, BLACK_PIX_DEFAULT (f));
  
  /* Make sure that the cursor color differs from the background color.  */
  if (f->output_data.win32->cursor_pixel == f->output_data.win32->background_pixel)
    {
      f->output_data.win32->cursor_pixel = f->output_data.win32->mouse_pixel;
      if (f->output_data.win32->cursor_pixel == fore_pixel)
	fore_pixel = f->output_data.win32->background_pixel;
    }
  f->output_data.win32->cursor_foreground_pixel = fore_pixel;

  if (FRAME_WIN32_WINDOW (f) != 0)
    {
      if (FRAME_VISIBLE_P (f))
	{
	  x_display_cursor (f, 0);
	  x_display_cursor (f, 1);
	}
    }
}

/* Set the border-color of frame F to value described by ARG.
   ARG can be a string naming a color.
   The border-color is used for the border that is drawn by the server.
   Note that this does not fully take effect if done before
   F has a window; it must be redone when the window is created.  */

void
x_set_border_color (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  unsigned char *str;
  int pix;

  CHECK_STRING (arg, 0);
  str = XSTRING (arg)->data;

  pix = x_decode_color (f, arg, BLACK_PIX_DEFAULT (f));

  x_set_border_pixel (f, pix);
}

/* Set the border-color of frame F to pixel value PIX.
   Note that this does not fully take effect if done before
   F has an window.  */

x_set_border_pixel (f, pix)
     struct frame *f;
     int pix;
{
  f->output_data.win32->border_pixel = pix;

  if (FRAME_WIN32_WINDOW (f) != 0 && f->output_data.win32->border_width > 0)
    {
      if (FRAME_VISIBLE_P (f))
        redraw_frame (f);
    }
}

void
x_set_cursor_type (f, arg, oldval)
     FRAME_PTR f;
     Lisp_Object arg, oldval;
{
  if (EQ (arg, Qbar))
    {
      FRAME_DESIRED_CURSOR (f) = bar_cursor;
      f->output_data.win32->cursor_width = 2;
    }
  else if (CONSP (arg) && EQ (XCONS (arg)->car, Qbar)
	   && INTEGERP (XCONS (arg)->cdr))
    {
      FRAME_DESIRED_CURSOR (f) = bar_cursor;
      f->output_data.win32->cursor_width = XINT (XCONS (arg)->cdr);
    }
  else
    /* Treat anything unknown as "box cursor".
       It was bad to signal an error; people have trouble fixing
       .Xdefaults with Emacs, when it has something bad in it.  */
    FRAME_DESIRED_CURSOR (f) = filled_box_cursor;

  /* Make sure the cursor gets redrawn.  This is overkill, but how
     often do people change cursor types?  */
  update_mode_lines++;
}

void
x_set_icon_type (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
#if 0
  Lisp_Object tem;
  int result;

  if (STRINGP (arg))
    {
      if (STRINGP (oldval) && EQ (Fstring_equal (oldval, arg), Qt))
	return;
    }
  else if (!STRINGP (oldval) && EQ (oldval, Qnil) == EQ (arg, Qnil))
    return;

  BLOCK_INPUT;
  if (NILP (arg))
    result = x_text_icon (f,
			  (char *) XSTRING ((!NILP (f->icon_name)
					     ? f->icon_name
					     : f->name))->data);
  else
    result = x_bitmap_icon (f, arg);

  if (result)
    {
      UNBLOCK_INPUT;
      error ("No icon window available");
    }

  /* If the window was unmapped (and its icon was mapped),
     the new icon is not mapped, so map the window in its stead.  */
  if (FRAME_VISIBLE_P (f))
    {
#ifdef USE_X_TOOLKIT
      XtPopup (f->output_data.win32->widget, XtGrabNone);
#endif
      XMapWindow (FRAME_WIN32_DISPLAY (f), FRAME_WIN32_WINDOW (f));
    }

  XFlush (FRAME_WIN32_DISPLAY (f));
  UNBLOCK_INPUT;
#endif
}

/* Return non-nil if frame F wants a bitmap icon.  */

Lisp_Object
x_icon_type (f)
     FRAME_PTR f;
{
  Lisp_Object tem;

  tem = assq_no_quit (Qicon_type, f->param_alist);
  if (CONSP (tem))
    return XCONS (tem)->cdr;
  else
    return Qnil;
}

void
x_set_icon_name (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  Lisp_Object tem;
  int result;

  if (STRINGP (arg))
    {
      if (STRINGP (oldval) && EQ (Fstring_equal (oldval, arg), Qt))
	return;
    }
  else if (!STRINGP (oldval) && EQ (oldval, Qnil) == EQ (arg, Qnil))
    return;

  f->icon_name = arg;

#if 0
  if (f->output_data.win32->icon_bitmap != 0)
    return;

  BLOCK_INPUT;

  result = x_text_icon (f,
			 (char *) XSTRING ((!NILP (f->icon_name)
					   ? f->icon_name
					   : f->name))->data);

  if (result)
    {
      UNBLOCK_INPUT;
      error ("No icon window available");
    }

  /* If the window was unmapped (and its icon was mapped),
     the new icon is not mapped, so map the window in its stead.  */
  if (FRAME_VISIBLE_P (f))
    {
#ifdef USE_X_TOOLKIT
      XtPopup (f->output_data.win32->widget, XtGrabNone);
#endif
      XMapWindow (FRAME_WIN32_DISPLAY (f), FRAME_WIN32_WINDOW (f));
    }

  XFlush (FRAME_WIN32_DISPLAY (f));
  UNBLOCK_INPUT;
#endif
}

extern Lisp_Object x_new_font ();

void
x_set_font (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  Lisp_Object result;

  CHECK_STRING (arg, 1);

  BLOCK_INPUT;
  result = x_new_font (f, XSTRING (arg)->data);
  UNBLOCK_INPUT;
  
  if (EQ (result, Qnil))
    error ("Font \"%s\" is not defined", XSTRING (arg)->data);
  else if (EQ (result, Qt))
    error ("the characters of the given font have varying widths");
  else if (STRINGP (result))
    {
      recompute_basic_faces (f);
      store_frame_param (f, Qfont, result);
    }
  else
    abort ();
}

void
x_set_border_width (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  CHECK_NUMBER (arg, 0);

  if (XINT (arg) == f->output_data.win32->border_width)
    return;

  if (FRAME_WIN32_WINDOW (f) != 0)
    error ("Cannot change the border width of a window");

  f->output_data.win32->border_width = XINT (arg);
}

void
x_set_internal_border_width (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  int mask;
  int old = f->output_data.win32->internal_border_width;

  CHECK_NUMBER (arg, 0);
  f->output_data.win32->internal_border_width = XINT (arg);
  if (f->output_data.win32->internal_border_width < 0)
    f->output_data.win32->internal_border_width = 0;

  if (f->output_data.win32->internal_border_width == old)
    return;

  if (FRAME_WIN32_WINDOW (f) != 0)
    {
      BLOCK_INPUT;
      x_set_window_size (f, 0, f->width, f->height);
      UNBLOCK_INPUT;
      SET_FRAME_GARBAGED (f);
    }
}

void
x_set_visibility (f, value, oldval)
     struct frame *f;
     Lisp_Object value, oldval;
{
  Lisp_Object frame;
  XSETFRAME (frame, f);

  if (NILP (value))
    Fmake_frame_invisible (frame, Qt);
  else if (EQ (value, Qicon))
    Ficonify_frame (frame);
  else
    Fmake_frame_visible (frame);
}

void
x_set_menu_bar_lines (f, value, oldval)
     struct frame *f;
     Lisp_Object value, oldval;
{
  int nlines;
  int olines = FRAME_MENU_BAR_LINES (f);

  /* Right now, menu bars don't work properly in minibuf-only frames;
     most of the commands try to apply themselves to the minibuffer
     frame itslef, and get an error because you can't switch buffers
     in or split the minibuffer window.  */
  if (FRAME_MINIBUF_ONLY_P (f))
    return;

  if (INTEGERP (value))
    nlines = XINT (value);
  else
    nlines = 0;

  FRAME_MENU_BAR_LINES (f) = 0;
  if (nlines)
    FRAME_EXTERNAL_MENU_BAR (f) = 1;
  else
    {
      if (FRAME_EXTERNAL_MENU_BAR (f) == 1)
	free_frame_menubar (f);
      FRAME_EXTERNAL_MENU_BAR (f) = 0;
    }
}

/* Change the name of frame F to NAME.  If NAME is nil, set F's name to
       win32_id_name.

   If EXPLICIT is non-zero, that indicates that lisp code is setting the
       name; if NAME is a string, set F's name to NAME and set
       F->explicit_name; if NAME is Qnil, then clear F->explicit_name.

   If EXPLICIT is zero, that indicates that Emacs redisplay code is
       suggesting a new name, which lisp code should override; if
       F->explicit_name is set, ignore the new name; otherwise, set it.  */

void
x_set_name (f, name, explicit)
     struct frame *f;
     Lisp_Object name;
     int explicit;
{
  /* Make sure that requests from lisp code override requests from 
     Emacs redisplay code.  */
  if (explicit)
    {
      /* If we're switching from explicit to implicit, we had better
	 update the mode lines and thereby update the title.  */
      if (f->explicit_name && NILP (name))
	update_mode_lines = 1;

      f->explicit_name = ! NILP (name);
    }
  else if (f->explicit_name)
    return;

  /* If NAME is nil, set the name to the win32_id_name.  */
  if (NILP (name))
    {
      /* Check for no change needed in this very common case
	 before we do any consing.  */
      if (!strcmp (FRAME_WIN32_DISPLAY_INFO (f)->win32_id_name,
		   XSTRING (f->name)->data))
	return;
      name = build_string (FRAME_WIN32_DISPLAY_INFO (f)->win32_id_name);
    }
  else
    CHECK_STRING (name, 0);

  /* Don't change the name if it's already NAME.  */
  if (! NILP (Fstring_equal (name, f->name)))
    return;

  if (FRAME_WIN32_WINDOW (f))
    {
      BLOCK_INPUT;
      SetWindowText(FRAME_WIN32_WINDOW (f), XSTRING (name)->data);
      UNBLOCK_INPUT;
    }

  f->name = name;
}

/* This function should be called when the user's lisp code has
   specified a name for the frame; the name will override any set by the
   redisplay code.  */
void
x_explicitly_set_name (f, arg, oldval)
     FRAME_PTR f;
     Lisp_Object arg, oldval;
{
  x_set_name (f, arg, 1);
}

/* This function should be called by Emacs redisplay code to set the
   name; names set this way will never override names set by the user's
   lisp code.  */
void
x_implicitly_set_name (f, arg, oldval)
     FRAME_PTR f;
     Lisp_Object arg, oldval;
{
  x_set_name (f, arg, 0);
}

void
x_set_autoraise (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  f->auto_raise = !EQ (Qnil, arg);
}

void
x_set_autolower (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  f->auto_lower = !EQ (Qnil, arg);
}

void
x_set_unsplittable (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  f->no_split = !NILP (arg);
}

void
x_set_vertical_scroll_bars (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  if (NILP (arg) != ! FRAME_HAS_VERTICAL_SCROLL_BARS (f))
    {
      FRAME_HAS_VERTICAL_SCROLL_BARS (f) = ! NILP (arg);

      /* We set this parameter before creating the window for the
	 frame, so we can get the geometry right from the start.
	 However, if the window hasn't been created yet, we shouldn't
	 call x_set_window_size.  */
      if (FRAME_WIN32_WINDOW (f))
	x_set_window_size (f, 0, FRAME_WIDTH (f), FRAME_HEIGHT (f));
    }
}

void
x_set_scroll_bar_width (f, arg, oldval)
     struct frame *f;
     Lisp_Object arg, oldval;
{
  if (NILP (arg))
    {
      FRAME_SCROLL_BAR_PIXEL_WIDTH (f) = 0;
      FRAME_SCROLL_BAR_COLS (f) = 2;
    }
  else if (INTEGERP (arg) && XINT (arg) > 0
	   && XFASTINT (arg) != FRAME_SCROLL_BAR_PIXEL_WIDTH (f))
    {
      int wid = FONT_WIDTH (f->output_data.win32->font);
      FRAME_SCROLL_BAR_PIXEL_WIDTH (f) = XFASTINT (arg);
      FRAME_SCROLL_BAR_COLS (f) = (XFASTINT (arg) + wid-1) / wid;
      if (FRAME_WIN32_WINDOW (f))
	x_set_window_size (f, 0, FRAME_WIDTH (f), FRAME_HEIGHT (f));
    }
}

/* Subroutines of creating an frame.  */

/* Make sure that Vx_resource_name is set to a reasonable value.
   Fix it up, or set it to `emacs' if it is too hopeless.  */

static void
validate_x_resource_name ()
{
  int len;
  /* Number of valid characters in the resource name.  */
  int good_count = 0;
  /* Number of invalid characters in the resource name.  */
  int bad_count = 0;
  Lisp_Object new;
  int i;

  if (STRINGP (Vx_resource_name))
    {
      unsigned char *p = XSTRING (Vx_resource_name)->data;
      int i;

      len = XSTRING (Vx_resource_name)->size;

      /* Only letters, digits, - and _ are valid in resource names.
	 Count the valid characters and count the invalid ones.  */
      for (i = 0; i < len; i++)
	{
	  int c = p[i];
	  if (! ((c >= 'a' && c <= 'z')
		 || (c >= 'A' && c <= 'Z')
		 || (c >= '0' && c <= '9')
		 || c == '-' || c == '_'))
	    bad_count++;
	  else
	    good_count++;
	}
    }
  else
    /* Not a string => completely invalid.  */
    bad_count = 5, good_count = 0;

  /* If name is valid already, return.  */
  if (bad_count == 0)
    return;

  /* If name is entirely invalid, or nearly so, use `emacs'.  */
  if (good_count == 0
      || (good_count == 1 && bad_count > 0))
    {
      Vx_resource_name = build_string ("emacs");
      return;
    }

  /* Name is partly valid.  Copy it and replace the invalid characters
     with underscores.  */

  Vx_resource_name = new = Fcopy_sequence (Vx_resource_name);

  for (i = 0; i < len; i++)
    {
      int c = XSTRING (new)->data[i];
      if (! ((c >= 'a' && c <= 'z')
	     || (c >= 'A' && c <= 'Z')
	     || (c >= '0' && c <= '9')
	     || c == '-' || c == '_'))
	XSTRING (new)->data[i] = '_';
    }
}


extern char *x_get_string_resource ();

DEFUN ("x-get-resource", Fx_get_resource, Sx_get_resource, 2, 4, 0,
  "Return the value of ATTRIBUTE, of class CLASS, from the X defaults database.\n\
This uses `INSTANCE.ATTRIBUTE' as the key and `Emacs.CLASS' as the\n\
class, where INSTANCE is the name under which Emacs was invoked, or\n\
the name specified by the `-name' or `-rn' command-line arguments.\n\
\n\
The optional arguments COMPONENT and SUBCLASS add to the key and the\n\
class, respectively.  You must specify both of them or neither.\n\
If you specify them, the key is `INSTANCE.COMPONENT.ATTRIBUTE'\n\
and the class is `Emacs.CLASS.SUBCLASS'.")
  (attribute, class, component, subclass)
     Lisp_Object attribute, class, component, subclass;
{
  register char *value;
  char *name_key;
  char *class_key;

  CHECK_STRING (attribute, 0);
  CHECK_STRING (class, 0);

  if (!NILP (component))
    CHECK_STRING (component, 1);
  if (!NILP (subclass))
    CHECK_STRING (subclass, 2);
  if (NILP (component) != NILP (subclass))
    error ("x-get-resource: must specify both COMPONENT and SUBCLASS or neither");

  validate_x_resource_name ();

  /* Allocate space for the components, the dots which separate them,
     and the final '\0'.  Make them big enough for the worst case.  */
  name_key = (char *) alloca (XSTRING (Vx_resource_name)->size
			      + (STRINGP (component)
				 ? XSTRING (component)->size : 0)
			      + XSTRING (attribute)->size
			      + 3);

  class_key = (char *) alloca ((sizeof (EMACS_CLASS) - 1)
			       + XSTRING (class)->size
			       + (STRINGP (subclass)
				  ? XSTRING (subclass)->size : 0)
			       + 3);

  /* Start with emacs.FRAMENAME for the name (the specific one)
     and with `Emacs' for the class key (the general one).  */
  strcpy (name_key, XSTRING (Vx_resource_name)->data);
  strcpy (class_key, EMACS_CLASS);

  strcat (class_key, ".");
  strcat (class_key, XSTRING (class)->data);

  if (!NILP (component))
    {
      strcat (class_key, ".");
      strcat (class_key, XSTRING (subclass)->data);

      strcat (name_key, ".");
      strcat (name_key, XSTRING (component)->data);
    }

  strcat (name_key, ".");
  strcat (name_key, XSTRING (attribute)->data);

  value = x_get_string_resource (Qnil,
				 name_key, class_key);

  if (value != (char *) 0)
    return build_string (value);
  else
    return Qnil;
}

/* Used when C code wants a resource value.  */

char *
x_get_resource_string (attribute, class)
     char *attribute, *class;
{
  register char *value;
  char *name_key;
  char *class_key;

  /* Allocate space for the components, the dots which separate them,
     and the final '\0'.  */
  name_key = (char *) alloca (XSTRING (Vinvocation_name)->size
			      + strlen (attribute) + 2);
  class_key = (char *) alloca ((sizeof (EMACS_CLASS) - 1)
			       + strlen (class) + 2);

  sprintf (name_key, "%s.%s",
	   XSTRING (Vinvocation_name)->data,
	   attribute);
  sprintf (class_key, "%s.%s", EMACS_CLASS, class);

  return x_get_string_resource (selected_frame,
				name_key, class_key);
}

/* Types we might convert a resource string into.  */
enum resource_types
  {
    number, boolean, string, symbol
  };

/* Return the value of parameter PARAM.

   First search ALIST, then Vdefault_frame_alist, then the X defaults
   database, using ATTRIBUTE as the attribute name and CLASS as its class.

   Convert the resource to the type specified by desired_type.

   If no default is specified, return Qunbound.  If you call
   x_get_arg, make sure you deal with Qunbound in a reasonable way,
   and don't let it get stored in any Lisp-visible variables!  */

static Lisp_Object
x_get_arg (alist, param, attribute, class, type)
     Lisp_Object alist, param;
     char *attribute;
     char *class;
     enum resource_types type;
{
  register Lisp_Object tem;

  tem = Fassq (param, alist);
  if (EQ (tem, Qnil))
    tem = Fassq (param, Vdefault_frame_alist);
  if (EQ (tem, Qnil))
    {

      if (attribute)
	{
	  tem = Fx_get_resource (build_string (attribute),
				 build_string (class),
				 Qnil, Qnil);

	  if (NILP (tem))
	    return Qunbound;

	  switch (type)
	    {
	    case number:
	      return make_number (atoi (XSTRING (tem)->data));

	    case boolean:
	      tem = Fdowncase (tem);
	      if (!strcmp (XSTRING (tem)->data, "on")
		  || !strcmp (XSTRING (tem)->data, "true"))
		return Qt;
	      else 
		return Qnil;

	    case string:
	      return tem;

	    case symbol:
	      /* As a special case, we map the values `true' and `on'
		 to Qt, and `false' and `off' to Qnil.  */
	      {
		Lisp_Object lower;
		lower = Fdowncase (tem);
		if (!strcmp (XSTRING (lower)->data, "on")
		    || !strcmp (XSTRING (lower)->data, "true"))
		  return Qt;
		else if (!strcmp (XSTRING (lower)->data, "off")
		      || !strcmp (XSTRING (lower)->data, "false"))
		  return Qnil;
		else
		  return Fintern (tem, Qnil);
	      }

	    default:
	      abort ();
	    }
	}
      else
	return Qunbound;
    }
  return Fcdr (tem);
}

/* Record in frame F the specified or default value according to ALIST
   of the parameter named PARAM (a Lisp symbol).
   If no value is specified for PARAM, look for an X default for XPROP
   on the frame named NAME.
   If that is not found either, use the value DEFLT.  */

static Lisp_Object
x_default_parameter (f, alist, prop, deflt, xprop, xclass, type)
     struct frame *f;
     Lisp_Object alist;
     Lisp_Object prop;
     Lisp_Object deflt;
     char *xprop;
     char *xclass;
     enum resource_types type;
{
  Lisp_Object tem;

  tem = x_get_arg (alist, prop, xprop, xclass, type);
  if (EQ (tem, Qunbound))
    tem = deflt;
  x_set_frame_parameters (f, Fcons (Fcons (prop, tem), Qnil));
  return tem;
}

DEFUN ("x-parse-geometry", Fx_parse_geometry, Sx_parse_geometry, 1, 1, 0,
       "Parse an X-style geometry string STRING.\n\
Returns an alist of the form ((top . TOP), (left . LEFT) ... ).\n\
The properties returned may include `top', `left', `height', and `width'.\n\
The value of `left' or `top' may be an integer,\n\
or a list (+ N) meaning N pixels relative to top/left corner,\n\
or a list (- N) meaning -N pixels relative to bottom/right corner.")
     (string)
     Lisp_Object string;
{
  int geometry, x, y;
  unsigned int width, height;
  Lisp_Object result;

  CHECK_STRING (string, 0);

  geometry = XParseGeometry ((char *) XSTRING (string)->data,
			     &x, &y, &width, &height);

  result = Qnil;
  if (geometry & XValue)
    {
      Lisp_Object element;

      if (x >= 0 && (geometry & XNegative))
	element = Fcons (Qleft, Fcons (Qminus, Fcons (make_number (-x), Qnil)));
      else if (x < 0 && ! (geometry & XNegative))
	element = Fcons (Qleft, Fcons (Qplus, Fcons (make_number (x), Qnil)));
      else
	element = Fcons (Qleft, make_number (x));
      result = Fcons (element, result);
    }

  if (geometry & YValue)
    {
      Lisp_Object element;

      if (y >= 0 && (geometry & YNegative))
	element = Fcons (Qtop, Fcons (Qminus, Fcons (make_number (-y), Qnil)));
      else if (y < 0 && ! (geometry & YNegative))
	element = Fcons (Qtop, Fcons (Qplus, Fcons (make_number (y), Qnil)));
      else
	element = Fcons (Qtop, make_number (y));
      result = Fcons (element, result);
    }

  if (geometry & WidthValue)
    result = Fcons (Fcons (Qwidth, make_number (width)), result);
  if (geometry & HeightValue)
    result = Fcons (Fcons (Qheight, make_number (height)), result);

  return result;
}

/* Calculate the desired size and position of this window,
   and return the flags saying which aspects were specified.

   This function does not make the coordinates positive.  */

#define DEFAULT_ROWS 40
#define DEFAULT_COLS 80

static int
x_figure_window_size (f, parms)
     struct frame *f;
     Lisp_Object parms;
{
  register Lisp_Object tem0, tem1, tem2;
  int height, width, left, top;
  register int geometry;
  long window_prompting = 0;

  /* Default values if we fall through.
     Actually, if that happens we should get
     window manager prompting.  */
  f->width = DEFAULT_COLS;
  f->height = DEFAULT_ROWS;
  /* Window managers expect that if program-specified
     positions are not (0,0), they're intentional, not defaults.  */
  f->output_data.win32->top_pos = 0;
  f->output_data.win32->left_pos = 0;

  tem0 = x_get_arg (parms, Qheight, 0, 0, number);
  tem1 = x_get_arg (parms, Qwidth, 0, 0, number);
  tem2 = x_get_arg (parms, Quser_size, 0, 0, number);
  if (! EQ (tem0, Qunbound) || ! EQ (tem1, Qunbound))
    {
      if (!EQ (tem0, Qunbound))
	{
	  CHECK_NUMBER (tem0, 0);
	  f->height = XINT (tem0);
	}
      if (!EQ (tem1, Qunbound))
	{
	  CHECK_NUMBER (tem1, 0);
	  f->width = XINT (tem1);
	}
      if (!NILP (tem2) && !EQ (tem2, Qunbound))
	window_prompting |= USSize;
      else
	window_prompting |= PSize;
    }

  f->output_data.win32->vertical_scroll_bar_extra
    = (!FRAME_HAS_VERTICAL_SCROLL_BARS (f)
       ? 0
       : FRAME_SCROLL_BAR_PIXEL_WIDTH (f) > 0
       ? FRAME_SCROLL_BAR_PIXEL_WIDTH (f)
       : (FRAME_SCROLL_BAR_COLS (f) * FONT_WIDTH (f->output_data.win32->font)));
  f->output_data.win32->pixel_width = CHAR_TO_PIXEL_WIDTH (f, f->width);
  f->output_data.win32->pixel_height = CHAR_TO_PIXEL_HEIGHT (f, f->height);

  tem0 = x_get_arg (parms, Qtop, 0, 0, number);
  tem1 = x_get_arg (parms, Qleft, 0, 0, number);
  tem2 = x_get_arg (parms, Quser_position, 0, 0, number);
  if (! EQ (tem0, Qunbound) || ! EQ (tem1, Qunbound))
    {
      if (EQ (tem0, Qminus))
	{
	  f->output_data.win32->top_pos = 0;
	  window_prompting |= YNegative;
	}
      else if (CONSP (tem0) && EQ (XCONS (tem0)->car, Qminus)
	       && CONSP (XCONS (tem0)->cdr)
	       && INTEGERP (XCONS (XCONS (tem0)->cdr)->car))
	{
	  f->output_data.win32->top_pos = - XINT (XCONS (XCONS (tem0)->cdr)->car);
	  window_prompting |= YNegative;
	}
      else if (CONSP (tem0) && EQ (XCONS (tem0)->car, Qplus)
	       && CONSP (XCONS (tem0)->cdr)
	       && INTEGERP (XCONS (XCONS (tem0)->cdr)->car))
	{
	  f->output_data.win32->top_pos = XINT (XCONS (XCONS (tem0)->cdr)->car);
	}
      else if (EQ (tem0, Qunbound))
	f->output_data.win32->top_pos = 0;
      else
	{
	  CHECK_NUMBER (tem0, 0);
	  f->output_data.win32->top_pos = XINT (tem0);
	  if (f->output_data.win32->top_pos < 0)
	    window_prompting |= YNegative;
	}

      if (EQ (tem1, Qminus))
	{
	  f->output_data.win32->left_pos = 0;
	  window_prompting |= XNegative;
	}
      else if (CONSP (tem1) && EQ (XCONS (tem1)->car, Qminus)
	       && CONSP (XCONS (tem1)->cdr)
	       && INTEGERP (XCONS (XCONS (tem1)->cdr)->car))
	{
	  f->output_data.win32->left_pos = - XINT (XCONS (XCONS (tem1)->cdr)->car);
	  window_prompting |= XNegative;
	}
      else if (CONSP (tem1) && EQ (XCONS (tem1)->car, Qplus)
	       && CONSP (XCONS (tem1)->cdr)
	       && INTEGERP (XCONS (XCONS (tem1)->cdr)->car))
	{
	  f->output_data.win32->left_pos = XINT (XCONS (XCONS (tem1)->cdr)->car);
	}
      else if (EQ (tem1, Qunbound))
	f->output_data.win32->left_pos = 0;
      else
	{
	  CHECK_NUMBER (tem1, 0);
	  f->output_data.win32->left_pos = XINT (tem1);
	  if (f->output_data.win32->left_pos < 0)
	    window_prompting |= XNegative;
	}

      if (!NILP (tem2) && ! EQ (tem2, Qunbound))
	window_prompting |= USPosition;
      else
	window_prompting |= PPosition;
    }

  return window_prompting;
}



extern LRESULT CALLBACK win32_wnd_proc ();

BOOL 
win32_init_class (hinst)
     HINSTANCE hinst;
{
  WNDCLASS wc;

  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = (WNDPROC) win32_wnd_proc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = WND_EXTRA_BYTES;
  wc.hInstance = hinst;
  wc.hIcon = LoadIcon (hinst, EMACS_CLASS);
  wc.hCursor = LoadCursor (NULL, IDC_ARROW);
  wc.hbrBackground = NULL; // GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = EMACS_CLASS;

  return (RegisterClass (&wc));
}

HWND 
win32_createscrollbar (f, bar)
     struct frame *f;
     struct scroll_bar * bar;
{
  return (CreateWindow ("SCROLLBAR", "", SBS_VERT | WS_CHILD | WS_VISIBLE,
			/* Position and size of scroll bar.  */
			XINT(bar->left), XINT(bar->top), 
			XINT(bar->width), XINT(bar->height),
			FRAME_WIN32_WINDOW (f),
			NULL,
			hinst,
			NULL));
}

void 
win32_createwindow (f)
     struct frame *f;
{
  HWND hwnd;
  
  /* Do first time app init */
  
  if (!hprevinst)
    {
      win32_init_class (hinst);
    }
  
  FRAME_WIN32_WINDOW (f) = hwnd = CreateWindow (EMACS_CLASS,
						f->namebuf,
						f->output_data.win32->dwStyle | WS_CLIPCHILDREN,
						f->output_data.win32->left_pos,
						f->output_data.win32->top_pos,
						PIXEL_WIDTH (f),
						PIXEL_HEIGHT (f),
						NULL,
						NULL,
						hinst,
						NULL);
  
  if (hwnd)
    {
      SetWindowLong (hwnd, WND_X_UNITS_INDEX, FONT_WIDTH (f->output_data.win32->font));
      SetWindowLong (hwnd, WND_Y_UNITS_INDEX, f->output_data.win32->line_height);
      SetWindowLong (hwnd, WND_BACKGROUND_INDEX, f->output_data.win32->background_pixel);
    }
}

DWORD 
win_msg_worker (dw)
     DWORD dw;
{
  MSG msg;
  
  /* Ensure our message queue is created */
  
  PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE);
  
  PostThreadMessage (dwMainThreadId, WM_EMACS_DONE, 0, 0);
  
  while (GetMessage (&msg, NULL, 0, 0))
    {
      if (msg.hwnd == NULL)
	{
	  switch (msg.message)
	    {
	    case WM_EMACS_CREATEWINDOW:
	      win32_createwindow ((struct frame *) msg.wParam);
	      PostThreadMessage (dwMainThreadId, WM_EMACS_DONE, 0, 0);
	      break;
	    case WM_EMACS_CREATESCROLLBAR:
	      {
		HWND hwnd = win32_createscrollbar ((struct frame *) msg.wParam,
						   (struct scroll_bar *) msg.lParam);
		PostThreadMessage (dwMainThreadId, WM_EMACS_DONE, (WPARAM)hwnd, 0);
	      }
	      break;
	    case WM_EMACS_KILL:
	      return (0);
	    }
	}
      else
	{
	  DispatchMessage (&msg);
	}
    }
  
  return (0);
}

HDC 
map_mode (hdc)
     HDC hdc;
{
  if (hdc) 
    {
#if 0
      /* Make mapping mode be in 1/20 of point */
      
      SetMapMode (hdc, MM_ANISOTROPIC);
      SetWindowExtEx (hdc, 1440, 1440, NULL);
      SetViewportExtEx (hdc,
			GetDeviceCaps (hdc, LOGPIXELSX),
			GetDeviceCaps (hdc, LOGPIXELSY),
			NULL);
#endif
    }
  return (hdc);
}

/* Convert between the modifier bits Win32 uses and the modifier bits
   Emacs uses.  */
unsigned int
win32_get_modifiers ()
{
  return (((GetKeyState (VK_SHIFT)&0x8000)   ? shift_modifier  : 0) |
	  ((GetKeyState (VK_CONTROL)&0x8000) ? ctrl_modifier   : 0) |
	  ((GetKeyState (VK_MENU)&0x8000)    ? meta_modifier   : 0));
}

void 
my_post_msg (wmsg, hwnd, msg, wParam, lParam)
     Win32Msg * wmsg;
     HWND hwnd;
     UINT msg;
     WPARAM wParam;
     LPARAM lParam;
{
  wmsg->msg.hwnd = hwnd;
  wmsg->msg.message = msg;
  wmsg->msg.wParam = wParam;
  wmsg->msg.lParam = lParam;
  wmsg->msg.time = GetMessageTime ();

  post_msg (wmsg);
}

/* Main window procedure */

extern char *lispy_function_keys[];

LRESULT CALLBACK 
win32_wnd_proc (hwnd, msg, wParam, lParam)
     HWND hwnd;
     UINT msg;
     WPARAM wParam;
     LPARAM lParam;
{
  struct frame *f;
  LRESULT ret = 1;
  struct win32_display_info *dpyinfo = &one_win32_display_info;
  Win32Msg wmsg;
  
  switch (msg) 
    {
    case WM_ERASEBKGND:
      {
	HBRUSH hb;
	HANDLE oldobj;
	RECT rect;
	
	GetClientRect (hwnd, &rect);
	
	hb = CreateSolidBrush (GetWindowLong (hwnd, WND_BACKGROUND_INDEX));
	
	oldobj = SelectObject ((HDC)wParam, hb);
	
	FillRect((HDC)wParam, &rect, hb);
	
	SelectObject((HDC)wParam, oldobj);
	
	DeleteObject (hb);
	
	return (0);
      }
    case WM_PAINT:
      {
	PAINTSTRUCT paintStruct;
		    
	BeginPaint (hwnd, &paintStruct);
	wmsg.rect = paintStruct.rcPaint;
	EndPaint (hwnd, &paintStruct);
      
	my_post_msg (&wmsg, hwnd, msg, wParam, lParam);
      
	return (0);
      }
      
    case WM_CREATE:
      {
	HDC hdc = my_get_dc (hwnd);

	/* Make mapping mode be in 1/20 of point */

	map_mode (hdc);

	ReleaseDC (hwnd, hdc);
      }
      
      return (0);
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
#if 0
      if (! ((wParam >= VK_BACK && wParam <= VK_TAB)
	     || (wParam >= VK_CLEAR && wParam <= VK_RETURN)
	     || (wParam == VK_ESCAPE)
	     || (wParam >= VK_PRIOR && wParam <= VK_HELP)
	     || (wParam >= VK_LWIN && wParam <= VK_APPS)
	     || (wParam >= VK_NUMPAD0 && wParam <= VK_F24)
	     || (wParam >= VK_NUMLOCK && wParam <= VK_SCROLL)
	     || (wParam >= VK_ATTN && wParam <= VK_OEM_CLEAR)
	     || !TranslateMessage (&msg1)))
	{
	  goto dflt;
	}
#endif
      
      /* Check for special characters since translate message 
	 seems to always indicate true.  */
      
      if (wParam == VK_MENU
	  || wParam == VK_SHIFT
	  || wParam == VK_CONTROL
	  || wParam == VK_CAPITAL)
	break;
      
      /* Anything we do not have a name for needs to be translated or 
	 returned as ascii keystroke.  */
      
      if (lispy_function_keys[wParam] == 0)
	{
	  MSG msg1;
	    
	  msg1.hwnd = hwnd;
	  msg1.message = msg;
	  msg1.wParam = wParam;
	  msg1.lParam = lParam;
	    
	  if (TranslateMessage (&msg1))
	    break;
	  else
	    msg = WM_CHAR;
	}
      
      /* Fall through */
      
    case WM_SYSCHAR:
    case WM_CHAR:
      wmsg.dwModifiers = win32_get_modifiers ();
      
      my_post_msg (&wmsg, hwnd, msg, wParam, lParam);
      break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      {
	BOOL up;

	if (parse_button (msg, NULL, &up))
	  {
	    if (up) ReleaseCapture ();
	    else SetCapture (hwnd);
	  }
      }
      
      wmsg.dwModifiers = win32_get_modifiers ();
      
      my_post_msg (&wmsg, hwnd, msg, wParam, lParam);
      goto dflt;
    case WM_MOUSEMOVE:
    case WM_MOVE:
    case WM_SIZE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_CLOSE:
    case WM_VSCROLL:
    case WM_SYSCOMMAND:
    case WM_COMMAND:
      my_post_msg (&wmsg, hwnd, msg, wParam, lParam);
      goto dflt;
    case WM_WINDOWPOSCHANGING:
      {
	WINDOWPLACEMENT wp;
	LPWINDOWPOS lppos = (WINDOWPOS *) lParam;
	
	GetWindowPlacement (hwnd, &wp);
	
	if (wp.showCmd != SW_SHOWMINIMIZED && ! (lppos->flags & SWP_NOSIZE))
	  {
	    RECT rect;
	    int wdiff;
	    int hdiff;
	    DWORD dwXUnits;
	    DWORD dwYUnits;
	    RECT wr;
	    
	    GetWindowRect (hwnd, &wr);
	    
	    enter_crit ();
	    
	    dwXUnits = GetWindowLong (hwnd, WND_X_UNITS_INDEX);
	    dwYUnits = GetWindowLong (hwnd, WND_Y_UNITS_INDEX);
	    
	    leave_crit ();
	    
	    memset (&rect, 0, sizeof (rect));
	    AdjustWindowRect (&rect, GetWindowLong (hwnd, GWL_STYLE), 
			      GetMenu (hwnd) != NULL);

	    /* All windows have an extra pixel so subtract 1 */
	    
	    wdiff = (lppos->cx - (rect.right - rect.left) - 0) % dwXUnits;
	    hdiff = (lppos->cy - (rect.bottom - rect.top) - 0) % dwYUnits;
	    
	    if (wdiff || hdiff)
	      {
		/* For right/bottom sizing we can just fix the sizes.  
		   However for top/left sizing we will need to fix the X 
		   and Y positions as well.  */
		
		lppos->cx -= wdiff;
		lppos->cy -= hdiff;
		
		if (wp.showCmd != SW_SHOWMAXIMIZED 
		    && ! (lppos->flags & SWP_NOMOVE))
		  {
		    if (lppos->x != wr.left || lppos->y != wr.top)
		      {
			lppos->x += wdiff;
			lppos->y += hdiff;
		      }
		    else
		      {
			lppos->flags |= SWP_NOMOVE;
		      }
		  }
		
		ret = 0;
	      }
	  }
      }
    
      if (ret == 0) return (0);
      
      goto dflt;
    case WM_EMACS_DESTROYWINDOW:
      DestroyWindow ((HWND) wParam);
      break;
    default:
    dflt:
      return DefWindowProc (hwnd, msg, wParam, lParam);
    }
  
  return (1);
}

void 
my_create_window (f)
     struct frame * f;
{
  MSG msg;

  PostThreadMessage (dwWinThreadId, WM_EMACS_CREATEWINDOW, (WPARAM)f, 0);
  GetMessage (&msg, NULL, WM_EMACS_DONE, WM_EMACS_DONE);
}

/* Create and set up the win32 window for frame F.  */

static void
win32_window (f, window_prompting, minibuffer_only)
     struct frame *f;
     long window_prompting;
     int minibuffer_only;
{
  BLOCK_INPUT;

  /* Use the resource name as the top-level window name
     for looking up resources.  Make a non-Lisp copy
     for the window manager, so GC relocation won't bother it.

     Elsewhere we specify the window name for the window manager.  */
     
  {
    char *str = (char *) XSTRING (Vx_resource_name)->data;
    f->namebuf = (char *) xmalloc (strlen (str) + 1);
    strcpy (f->namebuf, str);
  }

  my_create_window (f);

  validate_x_resource_name ();

  /* x_set_name normally ignores requests to set the name if the
     requested name is the same as the current name.  This is the one
     place where that assumption isn't correct; f->name is set, but
     the server hasn't been told.  */
  {
    Lisp_Object name;
    int explicit = f->explicit_name;

    f->explicit_name = 0;
    name = f->name;
    f->name = Qnil;
    x_set_name (f, name, explicit);
  }

  UNBLOCK_INPUT;

  if (!minibuffer_only && FRAME_EXTERNAL_MENU_BAR (f))
    initialize_frame_menubar (f);

  if (FRAME_WIN32_WINDOW (f) == 0)
    error ("Unable to create window");
}

/* Handle the icon stuff for this window.  Perhaps later we might
   want an x_set_icon_position which can be called interactively as
   well.  */

static void
x_icon (f, parms)
     struct frame *f;
     Lisp_Object parms;
{
  Lisp_Object icon_x, icon_y;

  /* Set the position of the icon.  Note that win95 groups all
     icons in the tray.  */
  icon_x = x_get_arg (parms, Qicon_left, 0, 0, number);
  icon_y = x_get_arg (parms, Qicon_top, 0, 0, number);
  if (!EQ (icon_x, Qunbound) && !EQ (icon_y, Qunbound))
    {
      CHECK_NUMBER (icon_x, 0);
      CHECK_NUMBER (icon_y, 0);
    }
  else if (!EQ (icon_x, Qunbound) || !EQ (icon_y, Qunbound))
    error ("Both left and top icon corners of icon must be specified");

  BLOCK_INPUT;

  if (! EQ (icon_x, Qunbound))
    x_wm_set_icon_position (f, XINT (icon_x), XINT (icon_y));

  UNBLOCK_INPUT;
}

DEFUN ("x-create-frame", Fx_create_frame, Sx_create_frame,
       1, 1, 0,
  "Make a new window, which is called a \"frame\" in Emacs terms.\n\
Returns an Emacs frame object.\n\
ALIST is an alist of frame parameters.\n\
If the parameters specify that the frame should not have a minibuffer,\n\
and do not specify a specific minibuffer window to use,\n\
then `default-minibuffer-frame' must be a frame whose minibuffer can\n\
be shared by the new frame.\n\
\n\
This function is an internal primitive--use `make-frame' instead.")
  (parms)
     Lisp_Object parms;
{
  struct frame *f;
  Lisp_Object frame, tem;
  Lisp_Object name;
  int minibuffer_only = 0;
  long window_prompting = 0;
  int width, height;
  int count = specpdl_ptr - specpdl;
  struct gcpro gcpro1;
  Lisp_Object display;
  struct win32_display_info *dpyinfo;
  Lisp_Object parent;
  struct kboard *kb;

  /* Use this general default value to start with
     until we know if this frame has a specified name.  */
  Vx_resource_name = Vinvocation_name;

  display = x_get_arg (parms, Qdisplay, 0, 0, string);
  if (EQ (display, Qunbound))
    display = Qnil;
  dpyinfo = check_x_display_info (display);
#ifdef MULTI_KBOARD
  kb = dpyinfo->kboard;
#else
  kb = &the_only_kboard;
#endif

  name = x_get_arg (parms, Qname, "title", "Title", string);
  if (!STRINGP (name)
      && ! EQ (name, Qunbound)
      && ! NILP (name))
    error ("Invalid frame name--not a string or nil");

  if (STRINGP (name))
    Vx_resource_name = name;

  /* See if parent window is specified.  */
  parent = x_get_arg (parms, Qparent_id, NULL, NULL, number);
  if (EQ (parent, Qunbound))
    parent = Qnil;
  if (! NILP (parent))
    CHECK_NUMBER (parent, 0);

  tem = x_get_arg (parms, Qminibuffer, 0, 0, symbol);
  if (EQ (tem, Qnone) || NILP (tem))
    f = make_frame_without_minibuffer (Qnil, kb, display);
  else if (EQ (tem, Qonly))
    {
      f = make_minibuffer_frame ();
      minibuffer_only = 1;
    }
  else if (WINDOWP (tem))
    f = make_frame_without_minibuffer (tem, kb, display);
  else
    f = make_frame (1);

  /* Note that Windows does support scroll bars.  */
  FRAME_CAN_HAVE_SCROLL_BARS (f) = 1;

  XSETFRAME (frame, f);
  GCPRO1 (frame);

  f->output_method = output_win32;
  f->output_data.win32 = (struct win32_output *) xmalloc (sizeof (struct win32_output));
  bzero (f->output_data.win32, sizeof (struct win32_output));

/*  FRAME_WIN32_DISPLAY_INFO (f) = dpyinfo; */
#ifdef MULTI_KBOARD
  FRAME_KBOARD (f) = kb;
#endif

  /* Specify the parent under which to make this window.  */

  if (!NILP (parent))
    {
      f->output_data.win32->parent_desc = (Window) parent;
      f->output_data.win32->explicit_parent = 1;
    }
  else
    {
      f->output_data.win32->parent_desc = FRAME_WIN32_DISPLAY_INFO (f)->root_window;
      f->output_data.win32->explicit_parent = 0;
    }

  /* Note that the frame has no physical cursor right now.  */
  f->phys_cursor_x = -1;

  /* Set the name; the functions to which we pass f expect the name to
     be set.  */
  if (EQ (name, Qunbound) || NILP (name))
    {
      f->name = build_string (dpyinfo->win32_id_name);
      f->explicit_name = 0;
    }
  else
    {
      f->name = name;
      f->explicit_name = 1;
      /* use the frame's title when getting resources for this frame.  */
      specbind (Qx_resource_name, name);
    }

  /* Extract the window parameters from the supplied values
     that are needed to determine window geometry.  */
  {
    Lisp_Object font;

    font = x_get_arg (parms, Qfont, "font", "Font", string);
    BLOCK_INPUT;
    /* First, try whatever font the caller has specified.  */
    if (STRINGP (font))
      font = x_new_font (f, XSTRING (font)->data);
#if 0
    /* Try out a font which we hope has bold and italic variations.  */
    if (!STRINGP (font))
      font = x_new_font (f, "-misc-fixed-medium-r-normal-*-*-140-*-*-c-*-iso8859-1");
    if (! STRINGP (font))
      font = x_new_font (f, "-*-*-medium-r-normal-*-*-140-*-*-c-*-iso8859-1");
    if (! STRINGP (font))
      /* This was formerly the first thing tried, but it finds too many fonts
	 and takes too long.  */
      font = x_new_font (f, "-*-*-medium-r-*-*-*-*-*-*-c-*-iso8859-1");
    /* If those didn't work, look for something which will at least work.  */
    if (! STRINGP (font))
      font = x_new_font (f, "-*-fixed-*-*-*-*-*-140-*-*-c-*-iso8859-1");
    if (! STRINGP (font))
      font = x_new_font (f, "-*-system-medium-r-normal-*-*-200-*-*-c-120-*-*");
#endif
    if (! STRINGP (font))
      font = x_new_font (f, "-*-terminal-medium-r-normal-*-*-180-*-*-c-120-*-*");
    UNBLOCK_INPUT;
    if (! STRINGP (font))
      font = build_string ("-*-system");

    x_default_parameter (f, parms, Qfont, font, 
			 "font", "Font", string);
  }

  x_default_parameter (f, parms, Qborder_width, make_number (2),
		       "borderwidth", "BorderWidth", number);
  /* This defaults to 2 in order to match xterm.  We recognize either
     internalBorderWidth or internalBorder (which is what xterm calls
     it).  */
  if (NILP (Fassq (Qinternal_border_width, parms)))
    {
      Lisp_Object value;

      value = x_get_arg (parms, Qinternal_border_width,
			 "internalBorder", "BorderWidth", number);
      if (! EQ (value, Qunbound))
	parms = Fcons (Fcons (Qinternal_border_width, value),
		       parms);
    }
  x_default_parameter (f, parms, Qinternal_border_width, make_number (0),
		       "internalBorderWidth", "BorderWidth", number);
  x_default_parameter (f, parms, Qvertical_scroll_bars, Qt,
		       "verticalScrollBars", "ScrollBars", boolean);

  /* Also do the stuff which must be set before the window exists.  */
  x_default_parameter (f, parms, Qforeground_color, build_string ("black"),
		       "foreground", "Foreground", string);
  x_default_parameter (f, parms, Qbackground_color, build_string ("white"),
		       "background", "Background", string);
  x_default_parameter (f, parms, Qmouse_color, build_string ("black"),
		       "pointerColor", "Foreground", string);
  x_default_parameter (f, parms, Qcursor_color, build_string ("black"),
		       "cursorColor", "Foreground", string);
  x_default_parameter (f, parms, Qborder_color, build_string ("black"),
		       "borderColor", "BorderColor", string);

  x_default_parameter (f, parms, Qmenu_bar_lines, make_number (1),
		       "menuBar", "MenuBar", number);
  x_default_parameter (f, parms, Qscroll_bar_width, Qnil,
		       "scrollBarWidth", "ScrollBarWidth", number);

  f->output_data.win32->dwStyle = WS_OVERLAPPEDWINDOW;
  f->output_data.win32->parent_desc = FRAME_WIN32_DISPLAY_INFO (f)->root_window;
  window_prompting = x_figure_window_size (f, parms);

  if (window_prompting & XNegative)
    {
      if (window_prompting & YNegative)
	f->output_data.win32->win_gravity = SouthEastGravity;
      else
	f->output_data.win32->win_gravity = NorthEastGravity;
    }
  else
    {
      if (window_prompting & YNegative)
	f->output_data.win32->win_gravity = SouthWestGravity;
      else
	f->output_data.win32->win_gravity = NorthWestGravity;
    }

  f->output_data.win32->size_hint_flags = window_prompting;

  win32_window (f, window_prompting, minibuffer_only);
  x_icon (f, parms);
  init_frame_faces (f);

  /* We need to do this after creating the window, so that the
     icon-creation functions can say whose icon they're describing.  */
  x_default_parameter (f, parms, Qicon_type, Qnil,
		       "bitmapIcon", "BitmapIcon", symbol);

  x_default_parameter (f, parms, Qauto_raise, Qnil,
		       "autoRaise", "AutoRaiseLower", boolean);
  x_default_parameter (f, parms, Qauto_lower, Qnil,
		       "autoLower", "AutoRaiseLower", boolean);
  x_default_parameter (f, parms, Qcursor_type, Qbox,
		       "cursorType", "CursorType", symbol);

  /* Dimensions, especially f->height, must be done via change_frame_size.
     Change will not be effected unless different from the current
     f->height.  */
  width = f->width;
  height = f->height;
  f->height = f->width = 0;
  change_frame_size (f, height, width, 1, 0);

  /* Tell the server what size and position, etc, we want,
     and how badly we want them.  */
  BLOCK_INPUT;
  x_wm_set_size_hint (f, window_prompting, 0);
  UNBLOCK_INPUT;

  tem = x_get_arg (parms, Qunsplittable, 0, 0, boolean);
  f->no_split = minibuffer_only || EQ (tem, Qt);

  UNGCPRO;

  /* It is now ok to make the frame official
     even if we get an error below.
     And the frame needs to be on Vframe_list
     or making it visible won't work.  */
  Vframe_list = Fcons (frame, Vframe_list);

  /* Now that the frame is official, it counts as a reference to
     its display.  */
  FRAME_WIN32_DISPLAY_INFO (f)->reference_count++;

  /* Make the window appear on the frame and enable display,
     unless the caller says not to.  However, with explicit parent,
     Emacs cannot control visibility, so don't try.  */
  if (! f->output_data.win32->explicit_parent)
    {
      Lisp_Object visibility;

      visibility = x_get_arg (parms, Qvisibility, 0, 0, symbol);
      if (EQ (visibility, Qunbound))
	visibility = Qt;

      if (EQ (visibility, Qicon))
	x_iconify_frame (f);
      else if (! NILP (visibility))
	x_make_frame_visible (f);
      else
	/* Must have been Qnil.  */
	;
    }

  return unbind_to (count, frame);
}

/* FRAME is used only to get a handle on the X display.  We don't pass the
   display info directly because we're called from frame.c, which doesn't
   know about that structure.  */
Lisp_Object
x_get_focus_frame (frame)
     struct frame *frame;
{
  struct win32_display_info *dpyinfo = FRAME_WIN32_DISPLAY_INFO (frame);
  Lisp_Object xfocus;
  if (! dpyinfo->win32_focus_frame)
    return Qnil;

  XSETFRAME (xfocus, dpyinfo->win32_focus_frame);
  return xfocus;
}

DEFUN ("focus-frame", Ffocus_frame, Sfocus_frame, 1, 1, 0,
  "Set the focus on FRAME.")
  (frame)
     Lisp_Object frame;
{
  CHECK_LIVE_FRAME (frame, 0);

  if (FRAME_WIN32_P (XFRAME (frame)))
    {
      BLOCK_INPUT;
      x_focus_on_frame (XFRAME (frame));
      UNBLOCK_INPUT;
      return frame;
    }

  return Qnil;
}

DEFUN ("unfocus-frame", Funfocus_frame, Sunfocus_frame, 0, 0, 0,
       "If a frame has been focused, release it.")
     ()
{
  if (FRAME_WIN32_P (selected_frame))
    {
      struct win32_display_info *dpyinfo = FRAME_WIN32_DISPLAY_INFO (selected_frame);

      if (dpyinfo->win32_focus_frame)
	{
	  BLOCK_INPUT;
	  x_unfocus_frame (dpyinfo->win32_focus_frame);
	  UNBLOCK_INPUT;
	}
    }
  
  return Qnil;
}

XFontStruct 
*win32_load_font (dpyinfo,name)
struct win32_display_info *dpyinfo;
char * name;
{
  XFontStruct * font = NULL;
  BOOL ok;
  
  {
    LOGFONT lf;
	
    if (!name || !x_to_win32_font(name, &lf)) 
      return (NULL);
	
    font = (XFontStruct *) xmalloc (sizeof (XFontStruct));
	
    if (!font) return (NULL);
	
    BLOCK_INPUT;
	
    font->hfont = CreateFontIndirect(&lf);
  }
  
    if (font->hfont == NULL) 
      {
	ok = FALSE;
      } 
    else 
      {
	HDC hdc;
	HANDLE oldobj;

	hdc = my_get_dc (dpyinfo->root_window);
	
	oldobj = SelectObject (hdc, font->hfont);
	
	ok = GetTextMetrics (hdc, &font->tm);
	
	SelectObject (hdc, oldobj);
	
	ReleaseDC (dpyinfo->root_window, hdc);
      }
  
  UNBLOCK_INPUT;
  
  if (ok) return (font);
  
  win32_unload_font(dpyinfo, font);
  return (NULL);
}

void 
win32_unload_font (dpyinfo, font)
     struct win32_display_info *dpyinfo;
     XFontStruct * font;
{
  if (font) 
    {
      if (font->hfont) DeleteObject(font->hfont);
      xfree (font);
    }
}

/* The font conversion stuff between x and win32 */

/* X font string is as follows (from faces.el)
 * (let ((- 		"[-?]")
 *      (foundry	"[^-]+")
 *      (family 	"[^-]+")
 *      (weight		"\\(bold\\|demibold\\|medium\\)")		; 1
 *      (weight\?	"\\([^-]*\\)")					; 1
 *      (slant		"\\([ior]\\)")					; 2
 *      (slant\?	"\\([^-]?\\)")					; 2
 *      (swidth		"\\([^-]*\\)")					; 3
 *      (adstyle	"[^-]*")					; 4
 *      (pixelsize	"[0-9]+")
 *      (pointsize	"[0-9][0-9]+")
 *      (resx		"[0-9][0-9]+")
 *      (resy		"[0-9][0-9]+")
 *      (spacing	"[cmp?*]")
 *      (avgwidth	"[0-9]+")
 *      (registry	"[^-]+")
 *      (encoding	"[^-]+")
 *      )
 *  (setq x-font-regexp
 *	(concat "\\`\\*?[-?*]"
 *		foundry - family - weight\? - slant\? - swidth - adstyle -
 *		pixelsize - pointsize - resx - resy - spacing - registry -
 *		encoding "[-?*]\\*?\\'"
 *		))
 *  (setq x-font-regexp-head
 *	(concat "\\`[-?*]" foundry - family - weight\? - slant\?
 *		"\\([-*?]\\|\\'\\)"))
 *  (setq x-font-regexp-slant (concat - slant -))
 *  (setq x-font-regexp-weight (concat - weight -))
 * nil)	    
 */
    
#define FONT_START       "[-?]"
#define FONT_FOUNDRY     "[^-]+"
#define FONT_FAMILY      "\\([^-]+\\)"                      /* 1 */
#define FONT_WEIGHT      "\\(bold\\|demibold\\|medium\\)"   /* 2 */
#define FONT_WEIGHT_Q    "\\([^-]*\\)"                      /* 2 */
#define FONT_SLANT       "\\([ior]\\)"                      /* 3 */
#define FONT_SLANT_Q     "\\([^-]?\\)"                      /* 3 */
#define FONT_SWIDTH      "\\([^-]*\\)"                      /* 4 */
#define FONT_ADSTYLE     "[^-]*"
#define FONT_PIXELSIZE   "[^-]*"
#define FONT_POINTSIZE   "\\([0-9][0-9]+\\|\\*\\)"          /* 5 */
#define FONT_RESX        "[0-9][0-9]+"
#define FONT_RESY        "[0-9][0-9]+"
#define FONT_SPACING     "[cmp?*]"
#define FONT_AVGWIDTH    "[0-9]+"
#define FONT_REGISTRY    "[^-]+"
#define FONT_ENCODING    "[^-]+"

#define FONT_REGEXP      ("\\`\\*?[-?*]"     \
			  FONT_FOUNDRY   "-" \
			  FONT_FAMILY    "-" \
			  FONT_WEIGHT_Q  "-" \
			  FONT_SLANT_Q   "-" \
			  FONT_SWIDTH    "-" \
			  FONT_ADSTYLE   "-" \
			  FONT_PIXELSIZE "-" \
			  FONT_POINTSIZE "-" \
			  "[-?*]\\|\\'")

#define FONT_REGEXP_HEAD ("\\`[-?*]"        \
			  FONT_FOUNDRY  "-" \
			  FONT_FAMILY   "-" \
			  FONT_WEIGHT_Q "-" \
			  FONT_SLANT_Q      \
			  "\\([-*?]\\|\\'\\)")

#define FONT_REGEXP_SLANT  "-" FONT_SLANT  "-"
#define FONT_REGEXP_WEIGHT "-" FONT_WEIGHT "-"

LONG 
x_to_win32_weight (lpw)
     char * lpw;
{
  if (!lpw) return (FW_DONTCARE);
  
  if (stricmp (lpw, "bold") == 0)
    return (FW_BOLD);
  else if (stricmp (lpw, "demibold") == 0)
    return (FW_SEMIBOLD);
  else if (stricmp (lpw, "medium") == 0)
    return (FW_MEDIUM);
  else if (stricmp (lpw, "normal") == 0)
    return (FW_NORMAL);
  else
    return (FW_DONTCARE);
}

char * 
win32_to_x_weight (fnweight)
     int fnweight;
{
  if (fnweight >= FW_BOLD) 
    return ("bold");
  else if (fnweight >= FW_SEMIBOLD) 
    return ("demibold");
  else if (fnweight >= FW_MEDIUM) 
    return ("medium");
  else  
    return ("normal");
}

BOOL 
win32_to_x_font (lplogfont, lpxstr, len)
     LOGFONT * lplogfont;
     char * lpxstr;
     int len;
{
  if (!lpxstr) return (FALSE);

  if (lplogfont) 
    {
      int height = (lplogfont->lfHeight * 1440) 
	/ one_win32_display_info.height_in;
      int width = (lplogfont->lfWidth * 1440) 
	/ one_win32_display_info.width_in;

      height = abs (height);
      _snprintf (lpxstr, len - 1,
		 "-*-%s-%s-%c-%s-%s-*-%d-*-*-%c-%d-*-*-",
		 lplogfont->lfFaceName,
		 win32_to_x_weight (lplogfont->lfWeight),
		 lplogfont->lfItalic ? 'i' : 'r',
		 "*", "*", 
		 height,
		 ((lplogfont->lfPitchAndFamily & 0x3) == VARIABLE_PITCH) ? 'p' : 'c',
		 width);
    } 
  else 
    {
      strncpy (lpxstr, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-", len - 1);
    }

  lpxstr[len - 1] = 0;		/* just to be sure */
  return (TRUE);
}

BOOL 
x_to_win32_font (lpxstr, lplogfont)
     char * lpxstr;
     LOGFONT * lplogfont;
{
  if (!lplogfont) return (FALSE);
  
  memset (lplogfont, 0, sizeof (*lplogfont));
  
  lplogfont->lfCharSet = OEM_CHARSET;
  lplogfont->lfOutPrecision = OUT_DEFAULT_PRECIS;
  lplogfont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lplogfont->lfQuality = DEFAULT_QUALITY;
  
  if (lpxstr && *lpxstr == '-') lpxstr++;
  
  {
    int fields;
    char name[50], weight[20], slant, pitch, height[10], width[10];
    
    fields = (lpxstr
	      ? sscanf (lpxstr, 
			"%*[^-]-%[^-]-%[^-]-%c-%*[^-]-%*[^-]-%*[^-]-%[^-]-%*[^-]-%*[^-]-%c-%[^-]",
			name, weight, &slant, height, &pitch, width)
	      : 0);
    
    if (fields == EOF) return (FALSE);
    
    if (fields > 0 && name[0] != '*') 
      {
	strncpy (lplogfont->lfFaceName, name, LF_FACESIZE);
      } 
    else 
      {
	lplogfont->lfFaceName[0] = 0;
      }
    
    fields--;
    
    lplogfont->lfWeight = x_to_win32_weight((fields > 0 ? weight : ""));
    
    fields--;
    
    lplogfont->lfItalic = (fields > 0 && slant == 'i');
    
    fields--;
    
    if (fields > 0 && height[0] != '*')
      lplogfont->lfHeight = (atoi (height) * one_win32_display_info.height_in) / 1440;
    
    fields--;
    
    lplogfont->lfPitchAndFamily = (fields > 0 && pitch == 'p') ? VARIABLE_PITCH : FIXED_PITCH;
    
    fields--;
    
    if (fields > 0 && width[0] != '*')
      lplogfont->lfWidth = (atoi (width) * one_win32_display_info.width_in) / 1440;
  }
  
  return (TRUE);
}

BOOL 
win32_font_match (lpszfont1, lpszfont2)
    char * lpszfont1;
    char * lpszfont2;
{
  char * s1 = lpszfont1, *e1;
  char * s2 = lpszfont2, *e2;
  
  if (s1 == NULL || s2 == NULL) return (FALSE);
  
  if (*s1 == '-') s1++;
  if (*s2 == '-') s2++;
  
  while (1) 
    {
      int len1, len2;

      e1 = strchr (s1, '-');
      e2 = strchr (s2, '-');

      if (e1 == NULL || e2 == NULL) return (TRUE);

      len1 = e1 - s1;
      len2 = e2 - s2;

      if (*s1 != '*' && *s2 != '*'
	  && (len1 != len2 || strnicmp (s1, s2, len1) != 0))
	return (FALSE);

      s1 = e1 + 1;
      s2 = e2 + 1;
    }
}

typedef struct enumfont_t 
{
  HDC hdc;
  int numFonts;
  XFontStruct *size_ref;
  Lisp_Object *pattern;
  Lisp_Object *head;
  Lisp_Object *tail;
} enumfont_t;

int CALLBACK 
enum_font_cb2 (lplf, lptm, FontType, lpef)
    ENUMLOGFONT * lplf;
    NEWTEXTMETRIC * lptm;
    int FontType;
    enumfont_t * lpef;
{
  if (lplf->elfLogFont.lfStrikeOut || lplf->elfLogFont.lfUnderline
      || (lplf->elfLogFont.lfCharSet != ANSI_CHARSET && lplf->elfLogFont.lfCharSet != OEM_CHARSET))
    return (1);
  
  /*    if (!lpef->size_ref || lptm->tmMaxCharWidth == FONT_WIDTH (lpef->size_ref)) */
  {
    char buf[100];

    if (!win32_to_x_font (lplf, buf, 100)) return (0);

    if (NILP (*(lpef->pattern)) || win32_font_match (buf, XSTRING (*(lpef->pattern))->data))
      {
	*lpef->tail = Fcons (build_string (buf), Qnil);
	lpef->tail = &XCONS (*lpef->tail)->cdr;
	lpef->numFonts++;
      }
  }
  
  return (1);
}

int CALLBACK 
enum_font_cb1 (lplf, lptm, FontType, lpef)
     ENUMLOGFONT * lplf;
     NEWTEXTMETRIC * lptm;
     int FontType;
     enumfont_t * lpef;
{
  return EnumFontFamilies (lpef->hdc,
			   lplf->elfLogFont.lfFaceName,
			   (FONTENUMPROC) enum_font_cb2,
			   (LPARAM) lpef);
}


DEFUN ("x-list-fonts", Fx_list_fonts, Sx_list_fonts, 1, 3, 0,
  "Return a list of the names of available fonts matching PATTERN.\n\
If optional arguments FACE and FRAME are specified, return only fonts\n\
the same size as FACE on FRAME.\n\
\n\
PATTERN is a string, perhaps with wildcard characters;\n\
  the * character matches any substring, and\n\
  the ? character matches any single character.\n\
  PATTERN is case-insensitive.\n\
FACE is a face name--a symbol.\n\
\n\
The return value is a list of strings, suitable as arguments to\n\
set-face-font.\n\
\n\
Fonts Emacs can't use (i.e. proportional fonts) may or may not be excluded\n\
even if they match PATTERN and FACE.")
  (pattern, face, frame)
    Lisp_Object pattern, face, frame;
{
  int num_fonts;
  char **names;
  XFontStruct *info;
  XFontStruct *size_ref;
  Lisp_Object namelist;
  Lisp_Object list;
  FRAME_PTR f;
  enumfont_t ef;

  CHECK_STRING (pattern, 0);
  if (!NILP (face))
    CHECK_SYMBOL (face, 1);

  f = check_x_frame (frame);

  /* Determine the width standard for comparison with the fonts we find.  */

  if (NILP (face))
    size_ref = 0;
  else
    {
      int face_id;

      /* Don't die if we get called with a terminal frame.  */
      if (! FRAME_WIN32_P (f))
	error ("non-win32 frame used in `x-list-fonts'");

      face_id = face_name_id_number (f, face);

      if (face_id < 0 || face_id >= FRAME_N_PARAM_FACES (f)
	  || FRAME_PARAM_FACES (f) [face_id] == 0)
	size_ref = f->output_data.win32->font;
      else
	{
	  size_ref = FRAME_PARAM_FACES (f) [face_id]->font;
	  if (size_ref == (XFontStruct *) (~0))
	    size_ref = f->output_data.win32->font;
	}
    }

  /* See if we cached the result for this particular query.  */
  list = Fassoc (pattern,
		 XCONS (FRAME_WIN32_DISPLAY_INFO (f)->name_list_element)->cdr);

  /* We have info in the cache for this PATTERN.  */
  if (!NILP (list))
    {
      Lisp_Object tem, newlist;

      /* We have info about this pattern.  */
      list = XCONS (list)->cdr;

      if (size_ref == 0)
	return list;

      BLOCK_INPUT;

      /* Filter the cached info and return just the fonts that match FACE.  */
      newlist = Qnil;
      for (tem = list; CONSP (tem); tem = XCONS (tem)->cdr)
	{
	  XFontStruct *thisinfo;

          thisinfo = win32_load_font (FRAME_WIN32_DISPLAY_INFO (f), XSTRING (XCONS (tem)->car)->data);

          if (thisinfo && same_size_fonts (thisinfo, size_ref))
	    newlist = Fcons (XCONS (tem)->car, newlist);

	  win32_unload_font (FRAME_WIN32_DISPLAY_INFO (f), thisinfo);
        }

      UNBLOCK_INPUT;

      return newlist;
    }

  BLOCK_INPUT;

  namelist = Qnil;
  ef.pattern = &pattern;
  ef.tail = ef.head = &namelist;
  ef.numFonts = 0;

  {
    ef.hdc = my_get_dc (FRAME_WIN32_WINDOW (f));

    EnumFontFamilies (ef.hdc, NULL, (FONTENUMPROC) enum_font_cb1, (LPARAM)&ef);
    
    ReleaseDC  (FRAME_WIN32_WINDOW (f), ef.hdc);
  }

  UNBLOCK_INPUT;

  if (ef.numFonts)
    {
      int i;
      Lisp_Object cur;

      /* Make a list of all the fonts we got back.
	 Store that in the font cache for the display.  */
      XCONS (FRAME_WIN32_DISPLAY_INFO (f)->name_list_element)->cdr
	= Fcons (Fcons (pattern, namelist),
		 XCONS (FRAME_WIN32_DISPLAY_INFO (f)->name_list_element)->cdr);

      /* Make a list of the fonts that have the right width.  */
      list = Qnil;
      cur=namelist;
      for (i = 0; i < ef.numFonts; i++)
        {
	  int keeper;

	  if (!size_ref)
	    keeper = 1;
	  else
	    {
	      XFontStruct *thisinfo;

	      BLOCK_INPUT;
	      thisinfo = win32_load_font (FRAME_WIN32_DISPLAY_INFO (f), XSTRING (Fcar (cur))->data);

	      keeper = thisinfo && same_size_fonts (thisinfo, size_ref);

	      win32_unload_font (FRAME_WIN32_DISPLAY_INFO (f), thisinfo);

	      UNBLOCK_INPUT;
	    }
          if (keeper)
	    list = Fcons (build_string (XSTRING (Fcar (cur))->data), list);

	  cur = Fcdr (cur);
        }
      list = Fnreverse (list);
    }

  return list;
}

DEFUN ("x-color-defined-p", Fx_color_defined_p, Sx_color_defined_p, 1, 2, 0,
       "Return non-nil if color COLOR is supported on frame FRAME.\n\
If FRAME is omitted or nil, use the selected frame.")
  (color, frame)
     Lisp_Object color, frame;
{
  COLORREF foo;
  FRAME_PTR f = check_x_frame (frame);

  CHECK_STRING (color, 1);

  if (defined_color (f, XSTRING (color)->data, &foo, 0))
    return Qt;
  else
    return Qnil;
}

DEFUN ("x-color-values", Fx_color_values, Sx_color_values, 1, 2, 0,
  "Return a description of the color named COLOR on frame FRAME.\n\
The value is a list of integer RGB values--(RED GREEN BLUE).\n\
These values appear to range from 0 to 65280 or 65535, depending\n\
on the system; white is (65280 65280 65280) or (65535 65535 65535).\n\
If FRAME is omitted or nil, use the selected frame.")
  (color, frame)
     Lisp_Object color, frame;
{
  COLORREF foo;
  FRAME_PTR f = check_x_frame (frame);

  CHECK_STRING (color, 1);

  if (defined_color (f, XSTRING (color)->data, &foo, 0))
    {
      Lisp_Object rgb[3];

      rgb[0] = make_number (GetRValue (foo));
      rgb[1] = make_number (GetGValue (foo));
      rgb[2] = make_number (GetBValue (foo));
      return Flist (3, rgb);
    }
  else
    return Qnil;
}

DEFUN ("x-display-color-p", Fx_display_color_p, Sx_display_color_p, 0, 1, 0,
  "Return t if the X display supports color.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  if ((dpyinfo->n_planes * dpyinfo->n_cbits) <= 2)
    return Qnil;

  return Qt;
}

DEFUN ("x-display-grayscale-p", Fx_display_grayscale_p, Sx_display_grayscale_p,
  0, 1, 0,
  "Return t if the X display supports shades of gray.\n\
Note that color displays do support shades of gray.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  if ((dpyinfo->n_planes * dpyinfo->n_cbits) <= 1)
    return Qnil;

  return Qt;
}

DEFUN ("x-display-pixel-width", Fx_display_pixel_width, Sx_display_pixel_width,
  0, 1, 0,
  "Returns the width in pixels of the X display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  return make_number (dpyinfo->width);
}

DEFUN ("x-display-pixel-height", Fx_display_pixel_height,
  Sx_display_pixel_height, 0, 1, 0,
  "Returns the height in pixels of the X display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  return make_number (dpyinfo->height);
}

DEFUN ("x-display-planes", Fx_display_planes, Sx_display_planes,
  0, 1, 0,
  "Returns the number of bitplanes of the display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  return make_number (dpyinfo->n_planes * dpyinfo->n_cbits);
}

DEFUN ("x-display-color-cells", Fx_display_color_cells, Sx_display_color_cells,
  0, 1, 0,
  "Returns the number of color cells of the display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);
  HDC hdc;
  int cap;

  hdc = my_get_dc (dpyinfo->root_window);
  
  cap = GetDeviceCaps (hdc,NUMCOLORS);
  
  ReleaseDC (dpyinfo->root_window, hdc);
  
  return make_number (cap);
}

DEFUN ("x-server-max-request-size", Fx_server_max_request_size,
       Sx_server_max_request_size,
  0, 1, 0,
  "Returns the maximum request size of the server of display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  return make_number (1);
}

DEFUN ("x-server-vendor", Fx_server_vendor, Sx_server_vendor, 0, 1, 0,
  "Returns the vendor ID string of the Win32 system (Microsoft).\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);
  char *vendor = "Microsoft Corp.";

  if (! vendor) vendor = "";
  return build_string (vendor);
}

DEFUN ("x-server-version", Fx_server_version, Sx_server_version, 0, 1, 0,
  "Returns the version numbers of the server of display DISPLAY.\n\
The value is a list of three integers: the major and minor\n\
version numbers, and the vendor-specific release\n\
number.  See also the function `x-server-vendor'.\n\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  return Fcons (make_number (nt_major_version),
		Fcons (make_number (nt_minor_version), Qnil));
}

DEFUN ("x-display-screens", Fx_display_screens, Sx_display_screens, 0, 1, 0,
  "Returns the number of screens on the server of display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  return make_number (1);
}

DEFUN ("x-display-mm-height", Fx_display_mm_height, Sx_display_mm_height, 0, 1, 0,
  "Returns the height in millimeters of the X display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);
  HDC hdc;
  int cap;

  hdc = my_get_dc (dpyinfo->root_window);
  
  cap = GetDeviceCaps (hdc, VERTSIZE);
  
  ReleaseDC (dpyinfo->root_window, hdc);
  
  return make_number (cap);
}

DEFUN ("x-display-mm-width", Fx_display_mm_width, Sx_display_mm_width, 0, 1, 0,
  "Returns the width in millimeters of the X display DISPLAY.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  HDC hdc;
  int cap;

  hdc = my_get_dc (dpyinfo->root_window);
  
  cap = GetDeviceCaps (hdc, HORZSIZE);
  
  ReleaseDC (dpyinfo->root_window, hdc);
  
  return make_number (cap);
}

DEFUN ("x-display-backing-store", Fx_display_backing_store,
  Sx_display_backing_store, 0, 1, 0,
  "Returns an indication of whether display DISPLAY does backing store.\n\
The value may be `always', `when-mapped', or `not-useful'.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  return intern ("not-useful");
}

DEFUN ("x-display-visual-class", Fx_display_visual_class,
  Sx_display_visual_class, 0, 1, 0,
  "Returns the visual class of the display DISPLAY.\n\
The value is one of the symbols `static-gray', `gray-scale',\n\
`static-color', `pseudo-color', `true-color', or `direct-color'.\n\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
	(display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

#if 0
  switch (dpyinfo->visual->class)
    {
    case StaticGray:  return (intern ("static-gray"));
    case GrayScale:   return (intern ("gray-scale"));
    case StaticColor: return (intern ("static-color"));
    case PseudoColor: return (intern ("pseudo-color"));
    case TrueColor:   return (intern ("true-color"));
    case DirectColor: return (intern ("direct-color"));
    default:
      error ("Display has an unknown visual class");
    }
#endif

  error ("Display has an unknown visual class");
}

DEFUN ("x-display-save-under", Fx_display_save_under,
  Sx_display_save_under, 0, 1, 0,
  "Returns t if the display DISPLAY supports the save-under feature.\n\
The optional argument DISPLAY specifies which display to ask about.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If omitted or nil, that stands for the selected frame's display.")
  (display)
     Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  return Qnil;
}

int
x_pixel_width (f)
     register struct frame *f;
{
  return PIXEL_WIDTH (f);
}

int
x_pixel_height (f)
     register struct frame *f;
{
  return PIXEL_HEIGHT (f);
}

int
x_char_width (f)
     register struct frame *f;
{
  return FONT_WIDTH (f->output_data.win32->font);
}

int
x_char_height (f)
     register struct frame *f;
{
  return f->output_data.win32->line_height;
}

int
x_screen_planes (frame)
     Lisp_Object frame;
{
  return (FRAME_WIN32_DISPLAY_INFO (XFRAME (frame))->n_planes * 
	  FRAME_WIN32_DISPLAY_INFO (XFRAME (frame))->n_cbits);
}

/* Return the display structure for the display named NAME.
   Open a new connection if necessary.  */

struct win32_display_info *
x_display_info_for_name (name)
     Lisp_Object name;
{
  Lisp_Object names;
  struct win32_display_info *dpyinfo;

  CHECK_STRING (name, 0);

  for (dpyinfo = &one_win32_display_info, names = win32_display_name_list;
       dpyinfo;
       dpyinfo = dpyinfo->next, names = XCONS (names)->cdr)
    {
      Lisp_Object tem;
      tem = Fstring_equal (XCONS (XCONS (names)->car)->car, name);
      if (!NILP (tem))
	return dpyinfo;
    }

  /* Use this general default value to start with.  */
  Vx_resource_name = Vinvocation_name;

  validate_x_resource_name ();

  dpyinfo = win32_term_init (name, (unsigned char *)0,
			     (char *) XSTRING (Vx_resource_name)->data);

  if (dpyinfo == 0)
    error ("Cannot connect to server %s", XSTRING (name)->data);

  XSETFASTINT (Vwindow_system_version, 3);

  return dpyinfo;
}

DEFUN ("x-open-connection", Fx_open_connection, Sx_open_connection,
       1, 3, 0, "Open a connection to a server.\n\
DISPLAY is the name of the display to connect to.\n\
Optional second arg XRM-STRING is a string of resources in xrdb format.\n\
If the optional third arg MUST-SUCCEED is non-nil,\n\
terminate Emacs if we can't open the connection.")
  (display, xrm_string, must_succeed)
     Lisp_Object display, xrm_string, must_succeed;
{
  unsigned int n_planes;
  unsigned char *xrm_option;
  struct win32_display_info *dpyinfo;

  CHECK_STRING (display, 0);
  if (! NILP (xrm_string))
    CHECK_STRING (xrm_string, 1);

  Vwin32_color_map = Fwin32_default_color_map ();

  if (! NILP (xrm_string))
    xrm_option = (unsigned char *) XSTRING (xrm_string)->data;
  else
    xrm_option = (unsigned char *) 0;

  /* Use this general default value to start with.  */
  Vx_resource_name = Vinvocation_name;

  validate_x_resource_name ();

  /* This is what opens the connection and sets x_current_display.
     This also initializes many symbols, such as those used for input.  */
  dpyinfo = win32_term_init (display, xrm_option,
			     (char *) XSTRING (Vx_resource_name)->data);

  if (dpyinfo == 0)
    {
      if (!NILP (must_succeed))
	fatal ("Cannot connect to server %s.\n",
	       XSTRING (display)->data);
      else
	error ("Cannot connect to server %s", XSTRING (display)->data);
    }

  XSETFASTINT (Vwindow_system_version, 3);
  return Qnil;
}

DEFUN ("x-close-connection", Fx_close_connection,
       Sx_close_connection, 1, 1, 0,
   "Close the connection to DISPLAY's server.\n\
For DISPLAY, specify either a frame or a display name (a string).\n\
If DISPLAY is nil, that stands for the selected frame's display.")
  (display)
  Lisp_Object display;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);
  struct win32_display_info *tail;
  int i;

  if (dpyinfo->reference_count > 0)
    error ("Display still has frames on it");

  BLOCK_INPUT;
  /* Free the fonts in the font table.  */
  for (i = 0; i < dpyinfo->n_fonts; i++)
    {
      if (dpyinfo->font_table[i].name)
	free (dpyinfo->font_table[i].name);
      /* Don't free the full_name string;
	 it is always shared with something else.  */
      win32_unload_font (dpyinfo, dpyinfo->font_table[i].font);
    }
  x_destroy_all_bitmaps (dpyinfo);

  x_delete_display (dpyinfo);
  UNBLOCK_INPUT;

  return Qnil;
}

DEFUN ("x-display-list", Fx_display_list, Sx_display_list, 0, 0, 0,
  "Return the list of display names that Emacs has connections to.")
  ()
{
  Lisp_Object tail, result;

  result = Qnil;
  for (tail = win32_display_name_list; ! NILP (tail); tail = XCONS (tail)->cdr)
    result = Fcons (XCONS (XCONS (tail)->car)->car, result);

  return result;
}

DEFUN ("x-synchronize", Fx_synchronize, Sx_synchronize, 1, 2, 0,
   "If ON is non-nil, report errors as soon as the erring request is made.\n\
If ON is nil, allow buffering of requests.\n\
This is a noop on Win32 systems.\n\
The optional second argument DISPLAY specifies which display to act on.\n\
DISPLAY should be either a frame or a display name (a string).\n\
If DISPLAY is omitted or nil, that stands for the selected frame's display.")
  (on, display)
    Lisp_Object display, on;
{
  struct win32_display_info *dpyinfo = check_x_display_info (display);

  return Qnil;
}


/* These are the win32 specialized functions */

DEFUN ("win32-select-font", Fwin32_select_font, Swin32_select_font, 0, 1, 0,
   "This will display the Win32 font dialog and return an X font string corresponding to the selection.")
  (frame)
     Lisp_Object frame;
{
  FRAME_PTR f = check_x_frame (frame);
  CHOOSEFONT cf;
  LOGFONT lf;
  char buf[100];

  bzero (&cf, sizeof (cf));

  cf.lStructSize = sizeof (cf);
  cf.hwndOwner = FRAME_WIN32_WINDOW (f);
  cf.Flags = CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_SCREENFONTS;
  cf.lpLogFont = &lf;

  if (!ChooseFont (&cf) || !win32_to_x_font (&lf, buf, 100))
      return Qnil;

  return build_string (buf);
}


syms_of_win32fns ()
{
  /* The section below is built by the lisp expression at the top of the file,
     just above where these variables are declared.  */
  /*&&& init symbols here &&&*/
  Qauto_raise = intern ("auto-raise");
  staticpro (&Qauto_raise);
  Qauto_lower = intern ("auto-lower");
  staticpro (&Qauto_lower);
  Qbackground_color = intern ("background-color");
  staticpro (&Qbackground_color);
  Qbar = intern ("bar");
  staticpro (&Qbar);
  Qborder_color = intern ("border-color");
  staticpro (&Qborder_color);
  Qborder_width = intern ("border-width");
  staticpro (&Qborder_width);
  Qbox = intern ("box");
  staticpro (&Qbox);
  Qcursor_color = intern ("cursor-color");
  staticpro (&Qcursor_color);
  Qcursor_type = intern ("cursor-type");
  staticpro (&Qcursor_type);
  Qfont = intern ("font");
  staticpro (&Qfont);
  Qforeground_color = intern ("foreground-color");
  staticpro (&Qforeground_color);
  Qgeometry = intern ("geometry");
  staticpro (&Qgeometry);
  Qicon_left = intern ("icon-left");
  staticpro (&Qicon_left);
  Qicon_top = intern ("icon-top");
  staticpro (&Qicon_top);
  Qicon_type = intern ("icon-type");
  staticpro (&Qicon_type);
  Qicon_name = intern ("icon-name");
  staticpro (&Qicon_name);
  Qinternal_border_width = intern ("internal-border-width");
  staticpro (&Qinternal_border_width);
  Qleft = intern ("left");
  staticpro (&Qleft);
  Qmouse_color = intern ("mouse-color");
  staticpro (&Qmouse_color);
  Qnone = intern ("none");
  staticpro (&Qnone);
  Qparent_id = intern ("parent-id");
  staticpro (&Qparent_id);
  Qscroll_bar_width = intern ("scroll-bar-width");
  staticpro (&Qscroll_bar_width);
  Qsuppress_icon = intern ("suppress-icon");
  staticpro (&Qsuppress_icon);
  Qtop = intern ("top");
  staticpro (&Qtop);
  Qundefined_color = intern ("undefined-color");
  staticpro (&Qundefined_color);
  Qvertical_scroll_bars = intern ("vertical-scroll-bars");
  staticpro (&Qvertical_scroll_bars);
  Qvisibility = intern ("visibility");
  staticpro (&Qvisibility);
  Qwindow_id = intern ("window-id");
  staticpro (&Qwindow_id);
  Qx_frame_parameter = intern ("x-frame-parameter");
  staticpro (&Qx_frame_parameter);
  Qx_resource_name = intern ("x-resource-name");
  staticpro (&Qx_resource_name);
  Quser_position = intern ("user-position");
  staticpro (&Quser_position);
  Quser_size = intern ("user-size");
  staticpro (&Quser_size);
  Qdisplay = intern ("display");
  staticpro (&Qdisplay);
  /* This is the end of symbol initialization.  */

  Fput (Qundefined_color, Qerror_conditions,
	Fcons (Qundefined_color, Fcons (Qerror, Qnil)));
  Fput (Qundefined_color, Qerror_message,
	build_string ("Undefined color"));

  DEFVAR_LISP ("win32-color-map", &Vwin32_color_map,
	       "A array of color name mappings for windows.");
  Vwin32_color_map = Qnil;

  init_x_parm_symbols ();

  DEFVAR_LISP ("x-bitmap-file-path", &Vx_bitmap_file_path,
    "List of directories to search for bitmap files for win32.");
  Vx_bitmap_file_path = decode_env_path ((char *) 0, "PATH");

  DEFVAR_LISP ("x-pointer-shape", &Vx_pointer_shape,
    "The shape of the pointer when over text.\n\
Changing the value does not affect existing frames\n\
unless you set the mouse color.");
  Vx_pointer_shape = Qnil;

  DEFVAR_LISP ("x-resource-name", &Vx_resource_name,
    "The name Emacs uses to look up resources; for internal use only.\n\
`x-get-resource' uses this as the first component of the instance name\n\
when requesting resource values.\n\
Emacs initially sets `x-resource-name' to the name under which Emacs\n\
was invoked, or to the value specified with the `-name' or `-rn'\n\
switches, if present.");
  Vx_resource_name = Qnil;

  Vx_nontext_pointer_shape = Qnil;

  Vx_mode_pointer_shape = Qnil;

  DEFVAR_INT ("x-sensitive-text-pointer-shape",
	      &Vx_sensitive_text_pointer_shape,
	      "The shape of the pointer when over mouse-sensitive text.\n\
This variable takes effect when you create a new frame\n\
or when you set the mouse color.");
  Vx_sensitive_text_pointer_shape = Qnil;

  DEFVAR_LISP ("x-cursor-fore-pixel", &Vx_cursor_fore_pixel,
	       "A string indicating the foreground color of the cursor box.");
  Vx_cursor_fore_pixel = Qnil;

  DEFVAR_LISP ("x-no-window-manager", &Vx_no_window_manager,
	       "Non-nil if no window manager is in use.\n\
Emacs doesn't try to figure this out; this is always nil\n\
unless you set it to something else.");
  /* We don't have any way to find this out, so set it to nil
     and maybe the user would like to set it to t.  */
  Vx_no_window_manager = Qnil;

  defsubr (&Sx_get_resource);
  defsubr (&Sx_list_fonts);
  defsubr (&Sx_display_color_p);
  defsubr (&Sx_display_grayscale_p);
  defsubr (&Sx_color_defined_p);
  defsubr (&Sx_color_values);
  defsubr (&Sx_server_max_request_size);
  defsubr (&Sx_server_vendor);
  defsubr (&Sx_server_version);
  defsubr (&Sx_display_pixel_width);
  defsubr (&Sx_display_pixel_height);
  defsubr (&Sx_display_mm_width);
  defsubr (&Sx_display_mm_height);
  defsubr (&Sx_display_screens);
  defsubr (&Sx_display_planes);
  defsubr (&Sx_display_color_cells);
  defsubr (&Sx_display_visual_class);
  defsubr (&Sx_display_backing_store);
  defsubr (&Sx_display_save_under);
  defsubr (&Sx_parse_geometry);
  defsubr (&Sx_create_frame);
  defsubr (&Sfocus_frame);
  defsubr (&Sunfocus_frame);
  defsubr (&Sx_open_connection);
  defsubr (&Sx_close_connection);
  defsubr (&Sx_display_list);
  defsubr (&Sx_synchronize);

  /* Win32 specific functions */

  defsubr (&Swin32_select_font);
}

#undef abort

void 
win32_abort()
{
    MessageBox (NULL,
		"A fatal error has occurred - aborting!",
		"Emacs Abort Dialog",
		MB_OK|MB_ICONEXCLAMATION);
    abort();
}
