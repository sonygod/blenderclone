/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file DNA_brush_types.h
 *  \ingroup DNA
 */

#ifndef __DNA_BRUSH_TYPES_H__
#define __DNA_BRUSH_TYPES_H__


#include "DNA_ID.h"
#include "DNA_texture_types.h" /* for MTex */

//#ifndef MAX_MTEX // XXX Not used?
//#define MAX_MTEX	18
//#endif

struct CurveMapping;
struct MTex;
struct Image;

typedef struct BrushClone {
	struct Image *image;    /* image for clone tool */
	float offset[2];        /* offset of clone image from canvas */
	float alpha, pad;       /* transparency for drawing of clone image */
} BrushClone;

typedef struct Brush {
	ID id;

	struct BrushClone clone;
	struct CurveMapping *curve; /* falloff curve */
	struct MTex mtex;
	struct MTex mask_mtex;

	struct Brush *toggle_brush;

	struct ImBuf *icon_imbuf;
	PreviewImage *preview;
	struct ColorBand *gradient;	/* color gradient */
	char icon_filepath[1024]; /* 1024 = FILE_MAX */

	float normal_weight;

	short blend;        /* blend mode */
	short ob_mode;      /* & with ob->mode to see if the brush is compatible, use for display only. */
	float weight;       /* brush weight */
	int size;           /* brush diameter */
	int flag;           /* general purpose flag */
	int mask_pressure;	/* pressure influence for mask */
	float jitter;       /* jitter the position of the brush */
	int jitter_absolute;	/* absolute jitter in pixels */
	int overlay_flags;
	int spacing;        /* spacing of paint operations */
	int smooth_stroke_radius;   /* turning radius (in pixels) for smooth stroke */
	float smooth_stroke_factor; /* higher values limit fast changes in the stroke direction */
	float rate;         /* paint operations / second (airbrush) */

	float rgb[3];           /* color */
	float alpha;            /* opacity */

	float secondary_rgb[3];	/* background color */

	int sculpt_plane;       /* the direction of movement for sculpt vertices */

	float plane_offset;     /* offset for plane brushes (clay, flatten, fill, scrape) */

	float gravity_factor;   /* gravity factor for sculpting */
	int gradient_source;    /* source for color gradient application */

	char sculpt_tool;       /* active sculpt tool */
	char vertexpaint_tool;  /* active vertex/weight paint blend mode (poorly named) */
	char imagepaint_tool;   /* active image paint tool */
	char mask_tool;         /* enum BrushMaskTool, only used if sculpt_tool is SCULPT_TOOL_MASK */

	float autosmooth_factor;

	float crease_pinch_factor;

	float plane_trim;
	float height;           /* affectable height of brush (layer height for layer tool, i.e.) */

	float texture_sample_bias;

	int texture_overlay_alpha;
	int mask_overlay_alpha;
	int cursor_overlay_alpha;

	float unprojected_radius;

	float add_col[3];
	float sub_col[3];

	float stencil_pos[2];
	float stencil_dimension[2];

	float mask_stencil_pos[2];
	float mask_stencil_dimension[2];
} Brush;

typedef struct PaletteColor
{
	struct PaletteColor *next, *prev;
	/* two values, one to store rgb, other to store values for sculpt/weight */
	float rgb[3];
	float value;
} PaletteColor;

typedef struct Palette
{
	ID id;

	/* pointer to individual colours */
	ListBase colors;

	int num_of_colours;
	int pad;
} Palette;

/* Brush.gradient_source */
typedef enum BrushGradientSource {
	BRUSH_GRADIENT_PRESSURE = 0, /* gradient from pressure */
	BRUSH_GRADIENT_SPACING = 1 /* gradient from spacing */
} BrushGradientSource;

/* Brush.flag */
typedef enum BrushFlags {
	BRUSH_AIRBRUSH = (1 << 0),
	BRUSH_TORUS = (1 << 1),
	BRUSH_ALPHA_PRESSURE = (1 << 2),
	BRUSH_SIZE_PRESSURE = (1 << 3),
	BRUSH_JITTER_PRESSURE = (1 << 4),
	BRUSH_SPACING_PRESSURE = (1 << 5),
	BRUSH_POLYLINE = (1 << 6),
	BRUSH_RAKE = (1 << 7),
	BRUSH_ANCHORED = (1 << 8),
	BRUSH_DIR_IN = (1 << 9),
	BRUSH_SPACE = (1 << 10),
	BRUSH_SMOOTH_STROKE = (1 << 11),
	BRUSH_PERSISTENT = (1 << 12),
	BRUSH_ACCUMULATE = (1 << 13),
	BRUSH_LOCK_ALPHA = (1 << 14),
	BRUSH_ORIGINAL_NORMAL = (1 << 15),
	BRUSH_OFFSET_PRESSURE = (1 << 16),
	BRUSH_SPACE_ATTEN = (1 << 18),
	BRUSH_ADAPTIVE_SPACE = (1 << 19),
	BRUSH_LOCK_SIZE = (1 << 20),
	BRUSH_USE_GRADIENT = (1 << 21),
	BRUSH_EDGE_TO_EDGE = (1 << 22),
	BRUSH_DRAG_DOT = (1 << 23),
	BRUSH_INVERSE_SMOOTH_PRESSURE = (1 << 24),
	BRUSH_RANDOM_ROTATION = (1 << 25),
	BRUSH_PLANE_TRIM = (1 << 26),
	BRUSH_FRONTFACE = (1 << 27),
	BRUSH_CUSTOM_ICON = (1 << 28),
	BRUSH_LINE = (1 << 29),
	BRUSH_ABSOLUTE_JITTER = (1 << 30)
} BrushFlags;

typedef enum {
	BRUSH_MASK_PRESSURE_RAMP = (1 << 1),
	BRUSH_MASK_PRESSURE_CUTOFF = (1 << 2)
} BrushMaskPressureFlags;

/* Brush.overlay_flags */
typedef enum OverlayFlags {
	BRUSH_OVERLAY_CURSOR = (1),
	BRUSH_OVERLAY_PRIMARY = (1 << 1),
	BRUSH_OVERLAY_SECONDARY = (1 << 2),
	BRUSH_OVERLAY_CURSOR_OVERRIDE_ON_STROKE = (1 << 3),
	BRUSH_OVERLAY_PRIMARY_OVERRIDE_ON_STROKE = (1 << 4),
	BRUSH_OVERLAY_SECONDARY_OVERRIDE_ON_STROKE = (1 << 5)
} OverlayFlags;

#define BRUSH_OVERLAY_OVERRIDE_MASK (BRUSH_OVERLAY_CURSOR_OVERRIDE_ON_STROKE | \
									 BRUSH_OVERLAY_PRIMARY_OVERRIDE_ON_STROKE | \
									 BRUSH_OVERLAY_SECONDARY_OVERRIDE_ON_STROKE)

/* Brush.sculpt_tool */
typedef enum BrushSculptTool {
	SCULPT_TOOL_DRAW = 1,
	SCULPT_TOOL_SMOOTH = 2,
	SCULPT_TOOL_PINCH = 3,
	SCULPT_TOOL_INFLATE = 4,
	SCULPT_TOOL_GRAB = 5,
	SCULPT_TOOL_LAYER = 6,
	SCULPT_TOOL_FLATTEN = 7,
	SCULPT_TOOL_CLAY = 8,
	SCULPT_TOOL_FILL = 9,
	SCULPT_TOOL_SCRAPE = 10,
	SCULPT_TOOL_NUDGE = 11,
	SCULPT_TOOL_THUMB = 12,
	SCULPT_TOOL_SNAKE_HOOK = 13,
	SCULPT_TOOL_ROTATE = 14,
	SCULPT_TOOL_SIMPLIFY = 15,
	SCULPT_TOOL_CREASE = 16,
	SCULPT_TOOL_BLOB = 17,
	SCULPT_TOOL_CLAY_STRIPS = 18,
	SCULPT_TOOL_MASK = 19,
	SCULPT_TOOL_GRAVITY = 20
} BrushSculptTool;

/* ImagePaintSettings.tool */
typedef enum BrushImagePaintTool {
	PAINT_TOOL_DRAW = 0,
	PAINT_TOOL_SOFTEN = 1,
	PAINT_TOOL_SMEAR = 2,
	PAINT_TOOL_CLONE = 3,
	PAINT_TOOL_FILL = 4
} BrushImagePaintTool;

/* direction that the brush displaces along */
enum {
	SCULPT_DISP_DIR_AREA = 0,
	SCULPT_DISP_DIR_VIEW = 1,
	SCULPT_DISP_DIR_X = 2,
	SCULPT_DISP_DIR_Y = 3,
	SCULPT_DISP_DIR_Z = 4
};

enum {
	PAINT_BLEND_MIX = 0,
	PAINT_BLEND_ADD = 1,
	PAINT_BLEND_SUB = 2,
	PAINT_BLEND_MUL = 3,
	PAINT_BLEND_BLUR = 4,
	PAINT_BLEND_LIGHTEN = 5,
	PAINT_BLEND_DARKEN = 6
};

typedef enum {
	BRUSH_MASK_DRAW = 0,
	BRUSH_MASK_SMOOTH = 1
} BrushMaskTool;

#define MAX_BRUSH_PIXEL_RADIUS 200

#endif

