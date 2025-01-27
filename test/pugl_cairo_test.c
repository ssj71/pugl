/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file pugl_cairo_test.c A simple Pugl test that creates a top-level window.
*/

#include "pugl/pugl.h"
#include "pugl/pugl_cairo_backend.h"

#include <cairo.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool     continuous  = false;
static int      quit        = 0;
static bool     entered     = false;
static bool     mouseDown   = false;
static unsigned framesDrawn = 0;

typedef struct {
	int         x;
	int         y;
	int         w;
	int         h;
	const char* label;
} Button;

static Button buttons[] = { { 128, 128, 64, 64, "1"   },
                            { 384, 128, 64, 64, "2"   },
                            { 128, 384, 64, 64, "3"   },
                            { 384, 384, 64, 64, "4"   },
                            { 0,   0,   0,   0,  NULL } };

static void
roundedBox(cairo_t* cr, double x, double y, double w, double h)
{
	static const double radius  = 10;
	static const double degrees = 3.14159265 / 180.0;

	cairo_new_sub_path(cr);
	cairo_arc(cr,
	          x + w - radius,
	          y + radius,
	          radius, -90 * degrees, 0 * degrees);
	cairo_arc(cr,
	          x + w - radius, y + h - radius,
	          radius, 0 * degrees, 90 * degrees);
	cairo_arc(cr,
	          x + radius, y + h - radius,
	          radius, 90 * degrees, 180 * degrees);
	cairo_arc(cr,
	          x + radius, y + radius,
	          radius, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);
}

static void
buttonDraw(cairo_t* cr, const Button* but, const double time)
{
	cairo_save(cr);
	cairo_translate(cr, but->x, but->y);
	cairo_rotate(cr, sin(time) * 3.141592);

	// Draw base
	if (mouseDown) {
		cairo_set_source_rgba(cr, 0.4, 0.9, 0.1, 1);
	} else {
		cairo_set_source_rgba(cr, 0.3, 0.5, 0.1, 1);
	}
	roundedBox(cr, 0, 0, but->w, but->h);
	cairo_fill_preserve(cr);

	// Draw border
	cairo_set_source_rgba(cr, 0.4, 0.9, 0.1, 1);
	cairo_set_line_width(cr, 4.0);
	cairo_stroke(cr);

	// Draw label
	cairo_text_extents_t extents;
	cairo_set_font_size(cr, 32.0);
	cairo_text_extents(cr, but->label, &extents);
	cairo_move_to(cr,
	              (but->w / 2.0) - extents.width / 2,
	              (but->h / 2.0) + extents.height / 2);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_show_text(cr, but->label);

	cairo_restore(cr);
}

static void
onDisplay(PuglView* view)
{
	cairo_t* cr = (cairo_t*)puglGetContext(view);

	// Draw background
	int width, height;
	puglGetSize(view, &width, &height);
	if (entered) {
		cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
	} else {
		cairo_set_source_rgb(cr, 0, 0, 0);
	}
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

	// Scale to view size
	const double scaleX = (width - (512 / (double)width)) / 512.0;
	const double scaleY = (height - (512 / (double)height)) / 512.0;
	cairo_scale(cr, scaleX, scaleY);

	// Draw button
	for (Button* b = buttons; b->label; ++b) {
		buttonDraw(cr, b, continuous ? puglGetTime(view) : 0.0);
	}

	++framesDrawn;
}

static void
onClose(PuglView* view)
{
	(void)view;
	quit = 1;
}

static void
onEvent(PuglView* view, const PuglEvent* event)
{
	switch (event->type) {
	case PUGL_KEY_PRESS:
		if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
			quit = 1;
		}
		break;
	case PUGL_BUTTON_PRESS:
		mouseDown = true;
		puglPostRedisplay(view);
		break;
	case PUGL_BUTTON_RELEASE:
		mouseDown = false;
		puglPostRedisplay(view);
		break;
	case PUGL_ENTER_NOTIFY:
		entered = true;
		puglPostRedisplay(view);
		break;
	case PUGL_LEAVE_NOTIFY:
		entered = false;
		puglPostRedisplay(view);
		break;
	case PUGL_EXPOSE:
		onDisplay(view);
		break;
	case PUGL_CLOSE:
		onClose(view);
		break;
	default: break;
	}
}

int
main(int argc, char** argv)
{
	bool ignoreKeyRepeat = false;
	bool resizable       = false;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-c")) {
			continuous = true;
		} else if (!strcmp(argv[i], "-h")) {
			printf("USAGE: %s [OPTIONS]...\n\n"
			       "  -c  Continuously animate and draw\n"
			       "  -h  Display this help\n"
			       "  -i  Ignore key repeat\n"
			       "  -r  Resizable window\n", argv[0]);
			return 0;
		} else if (!strcmp(argv[i], "-i")) {
			ignoreKeyRepeat = true;
		} else if (!strcmp(argv[i], "-r")) {
			resizable = true;
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
		}
	}

	PuglView* view = puglInit(NULL, NULL);
	puglInitWindowClass(view, "PuglCairoTest");
	puglInitWindowSize(view, 512, 512);
	puglInitWindowMinSize(view, 256, 256);
	puglInitWindowHint(view, PUGL_RESIZABLE, resizable);
	puglInitBackend(view, puglCairoBackend());

	puglIgnoreKeyRepeat(view, ignoreKeyRepeat);
	puglSetEventFunc(view, onEvent);

	if (puglCreateWindow(view, "Pugl Test")) {
		return 1;
	}

	puglShowWindow(view);

	float lastReportTime = (float)puglGetTime(view);
	while (!quit) {
		const float thisTime = (float)puglGetTime(view);

		if (continuous) {
			puglPostRedisplay(view);
		} else {
			puglWaitForEvent(view);
		}

		puglProcessEvents(view);

		if (continuous && thisTime > lastReportTime + 5) {
			const double fps = framesDrawn / (thisTime - lastReportTime);
			fprintf(stderr,
			        "%u frames in %.0f seconds = %.3f FPS\n",
			        framesDrawn, thisTime - lastReportTime, fps);

			lastReportTime = thisTime;
			framesDrawn    = 0;
		}
	}

	puglDestroy(view);
	return 0;
}
