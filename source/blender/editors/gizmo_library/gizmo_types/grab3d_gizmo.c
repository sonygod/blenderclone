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
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file grab3d_gizmo.c
 *  \ingroup wm
 *
 * \name Grab Gizmo
 *
 * 3D Gizmo, also works in 2D views.
 *
 * \brief Simple gizmo to grab and translate.
 *
 * - `matrix[0]` is derived from Y and Z.
 * - `matrix[1]` currently not used.
 * - `matrix[2]` is the widget direction (for all gizmos).
 *
 */

#include "MEM_guardedalloc.h"

#include "BLI_math.h"

#include "BKE_context.h"

#include "BIF_gl.h"
#include "BIF_glutil.h"

#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_matrix.h"
#include "GPU_select.h"
#include "GPU_state.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "WM_api.h"
#include "WM_types.h"

#include "ED_screen.h"
#include "ED_view3d.h"
#include "ED_gizmo_library.h"

/* own includes */
#include "../gizmo_geometry.h"
#include "../gizmo_library_intern.h"

typedef struct GrabGizmo3D {
	wmGizmo gizmo;
	/* Added to 'matrix_basis' when calculating the matrix. */
	float prop_co[3];
} GrabGizmo3D;

static void gizmo_grab_matrix_basis_get(const wmGizmo *mpr, float r_matrix[4][4])
{
	GrabGizmo3D *grab = (GrabGizmo3D *)mpr;

	copy_m4_m4(r_matrix, grab->gizmo.matrix_basis);
	add_v3_v3(r_matrix[3], grab->prop_co);
}

static int gizmo_grab_modal(
        bContext *C, wmGizmo *mpr, const wmEvent *event,
        eWM_GizmoFlagTweak tweak_flag);

typedef struct GrabInteraction {
	float init_mval[2];

	/* only for when using properties */
	float init_prop_co[3];

	float init_matrix_final[4][4];
} GrabInteraction;

#define DIAL_RESOLUTION 32

/* -------------------------------------------------------------------- */

static void grab_geom_draw(
        const wmGizmo *mpr, const float color[4], const bool select, const int draw_options)
{
#ifdef USE_GIZMO_CUSTOM_DIAL
	UNUSED_VARS(grab3d, col, axis_modal_mat);
	wm_gizmo_geometryinfo_draw(&wm_gizmo_geom_data_grab3d, select);
#else
	const int draw_style = RNA_enum_get(mpr->ptr, "draw_style");
	const bool filled = (draw_options & ED_GIZMO_GRAB_DRAW_FLAG_FILL) != 0;

	GPU_line_width(mpr->line_width);

	Gwn_VertFormat *format = immVertexFormat();
	uint pos = GWN_vertformat_attr_add(format, "pos", GWN_COMP_F32, 2, GWN_FETCH_FLOAT);

	immBindBuiltinProgram(GPU_SHADER_3D_UNIFORM_COLOR);

	immUniformColor4fv(color);

	if (draw_style == ED_GIZMO_GRAB_STYLE_RING_2D) {
		if (filled) {
			imm_draw_circle_fill_2d(pos, 0, 0, 1.0f, DIAL_RESOLUTION);
		}
		else {
			imm_draw_circle_wire_2d(pos, 0, 0, 1.0f, DIAL_RESOLUTION);
		}
	}
	else if (draw_style == ED_GIZMO_GRAB_STYLE_CROSS_2D) {
		immBegin(GWN_PRIM_LINES, 4);
		immVertex2f(pos,  1.0f,  1.0f);
		immVertex2f(pos, -1.0f, -1.0f);

		immVertex2f(pos, -1.0f,  1.0f);
		immVertex2f(pos,  1.0f, -1.0f);
		immEnd();
	}
	else {
		BLI_assert(0);
	}

	immUnbindProgram();

	UNUSED_VARS(select);
#endif
}

static void grab3d_get_translate(
        const wmGizmo *mpr, const wmEvent *event, const ARegion *ar,
        float co_delta[3])
{
	GrabInteraction *inter = mpr->interaction_data;
	const float mval_delta[2] = {
	    event->mval[0] - inter->init_mval[0],
	    event->mval[1] - inter->init_mval[1],
	};

	RegionView3D *rv3d = ar->regiondata;
	float co_ref[3];
	mul_v3_mat3_m4v3(co_ref, mpr->matrix_space, inter->init_prop_co);
	const float zfac = ED_view3d_calc_zfac(rv3d, co_ref, NULL);

	ED_view3d_win_to_delta(ar, mval_delta, co_delta, zfac);

	float matrix_space_inv[3][3];
	copy_m3_m4(matrix_space_inv, mpr->matrix_space);
	invert_m3(matrix_space_inv);
	mul_m3_v3(matrix_space_inv, co_delta);
}

static void grab3d_draw_intern(
        const bContext *C, wmGizmo *mpr,
        const bool select, const bool highlight)
{
	GrabInteraction *inter = mpr->interaction_data;
	const int draw_options = RNA_enum_get(mpr->ptr, "draw_options");
	const bool align_view = (draw_options & ED_GIZMO_GRAB_DRAW_FLAG_ALIGN_VIEW) != 0;
	float color[4];
	float matrix_final[4][4];
	float matrix_align[4][4];

	gizmo_color_get(mpr, highlight, color);
	WM_gizmo_calc_matrix_final(mpr, matrix_final);

	gpuPushMatrix();
	gpuMultMatrix(matrix_final);

	if (align_view) {
		float matrix_final_unit[4][4];
		RegionView3D *rv3d = CTX_wm_region_view3d(C);
		normalize_m4_m4(matrix_final_unit, matrix_final);
		mul_m4_m4m4(matrix_align, rv3d->viewmat, matrix_final_unit);
		zero_v3(matrix_align[3]);
		transpose_m4(matrix_align);
		gpuMultMatrix(matrix_align);
	}

	GPU_blend(true);
	grab_geom_draw(mpr, color, select, draw_options);
	GPU_blend(false);
	gpuPopMatrix();

	if (mpr->interaction_data) {
		gpuPushMatrix();
		gpuMultMatrix(inter->init_matrix_final);

		if (align_view) {
			gpuMultMatrix(matrix_align);
		}

		GPU_blend(true);
		grab_geom_draw(mpr, (const float[4]){0.5f, 0.5f, 0.5f, 0.5f}, select, draw_options);
		GPU_blend(false);
		gpuPopMatrix();
	}
}

static void gizmo_grab_draw_select(const bContext *C, wmGizmo *mpr, int select_id)
{
	GPU_select_load_id(select_id);
	grab3d_draw_intern(C, mpr, true, false);
}

static void gizmo_grab_draw(const bContext *C, wmGizmo *mpr)
{
	const bool is_modal = mpr->state & WM_GIZMO_STATE_MODAL;
	const bool is_highlight = (mpr->state & WM_GIZMO_STATE_HIGHLIGHT) != 0;

	(void)is_modal;

	GPU_blend(true);
	grab3d_draw_intern(C, mpr, false, is_highlight);
	GPU_blend(false);
}

static int gizmo_grab_modal(
        bContext *C, wmGizmo *mpr, const wmEvent *event,
        eWM_GizmoFlagTweak UNUSED(tweak_flag))
{
	GrabGizmo3D *grab = (GrabGizmo3D *)mpr;
	GrabInteraction *inter = mpr->interaction_data;
	ARegion *ar = CTX_wm_region(C);

	float prop_delta[3];
	if (CTX_wm_area(C)->spacetype == SPACE_VIEW3D) {
		grab3d_get_translate(mpr, event, ar, prop_delta);
	}
	else {
		float mval_proj_init[2], mval_proj_curr[2];
		if ((gizmo_window_project_2d(
		         C, mpr, inter->init_mval, 2, false, mval_proj_init) == false) ||
		    (gizmo_window_project_2d(
		         C, mpr, (const float[2]){UNPACK2(event->mval)}, 2, false, mval_proj_curr) == false))
		{
			return OPERATOR_RUNNING_MODAL;
		}
		sub_v2_v2v2(prop_delta, mval_proj_curr, mval_proj_init);
		prop_delta[2] = 0.0f;
	}
	add_v3_v3v3(grab->prop_co, inter->init_prop_co, prop_delta);

	/* set the property for the operator and call its modal function */
	wmGizmoProperty *mpr_prop = WM_gizmo_target_property_find(mpr, "offset");
	if (WM_gizmo_target_property_is_valid(mpr_prop)) {
		WM_gizmo_target_property_value_set_array(C, mpr, mpr_prop, grab->prop_co);
	}
	else {
		zero_v3(grab->prop_co);
	}

	ED_region_tag_redraw(ar);

	return OPERATOR_RUNNING_MODAL;
}

static int gizmo_grab_invoke(
        bContext *UNUSED(C), wmGizmo *mpr, const wmEvent *event)
{
	GrabInteraction *inter = MEM_callocN(sizeof(GrabInteraction), __func__);

	inter->init_mval[0] = event->mval[0];
	inter->init_mval[1] = event->mval[1];

#if 0
	copy_v3_v3(inter->init_prop_co, grab->prop_co);
#else
	wmGizmoProperty *mpr_prop = WM_gizmo_target_property_find(mpr, "offset");
	if (WM_gizmo_target_property_is_valid(mpr_prop)) {
		WM_gizmo_target_property_value_get_array(mpr, mpr_prop, inter->init_prop_co);
	}
#endif

	WM_gizmo_calc_matrix_final(mpr, inter->init_matrix_final);

	mpr->interaction_data = inter;

	return OPERATOR_RUNNING_MODAL;
}


static int gizmo_grab_test_select(
        bContext *C, wmGizmo *mpr, const wmEvent *event)
{
	float point_local[2];

	if (gizmo_window_project_2d(
	        C, mpr, (const float[2]){UNPACK2(event->mval)}, 2, true, point_local) == false)
	{
		return -1;
	}

	/* The 'mpr->scale_final' is already applied when projecting. */
	if (len_squared_v2(point_local) < 1.0f) {
		return 0;
	}

	return -1;
}

static void gizmo_grab_property_update(wmGizmo *mpr, wmGizmoProperty *mpr_prop)
{
	GrabGizmo3D *grab = (GrabGizmo3D *)mpr;
	if (WM_gizmo_target_property_is_valid(mpr_prop)) {
		WM_gizmo_target_property_value_get_array(mpr, mpr_prop, grab->prop_co);
	}
	else {
		zero_v3(grab->prop_co);
	}
}

static int gizmo_grab_cursor_get(wmGizmo *UNUSED(mpr))
{
	return BC_NSEW_SCROLLCURSOR;
}

/* -------------------------------------------------------------------- */
/** \name Grab Gizmo API
 *
 * \{ */

static void GIZMO_WT_grab_3d(wmGizmoType *wt)
{
	/* identifiers */
	wt->idname = "GIZMO_WT_grab_3d";

	/* api callbacks */
	wt->draw = gizmo_grab_draw;
	wt->draw_select = gizmo_grab_draw_select;
	wt->test_select = gizmo_grab_test_select;
	wt->matrix_basis_get = gizmo_grab_matrix_basis_get;
	wt->invoke = gizmo_grab_invoke;
	wt->property_update = gizmo_grab_property_update;
	wt->modal = gizmo_grab_modal;
	wt->cursor_get = gizmo_grab_cursor_get;

	wt->struct_size = sizeof(GrabGizmo3D);

	/* rna */
	static EnumPropertyItem rna_enum_draw_style[] = {
		{ED_GIZMO_GRAB_STYLE_RING_2D, "RING_2D", 0, "Ring", ""},
		{ED_GIZMO_GRAB_STYLE_CROSS_2D, "CROSS_2D", 0, "Ring", ""},
		{0, NULL, 0, NULL, NULL}
	};
	static EnumPropertyItem rna_enum_draw_options[] = {
		{ED_GIZMO_GRAB_DRAW_FLAG_FILL, "FILL", 0, "Filled", ""},
		{ED_GIZMO_GRAB_DRAW_FLAG_ALIGN_VIEW, "ALIGN_VIEW", 0, "Align View", ""},
		{0, NULL, 0, NULL, NULL}
	};

	RNA_def_enum(wt->srna, "draw_style", rna_enum_draw_style, ED_GIZMO_GRAB_STYLE_RING_2D, "Draw Style", "");
	RNA_def_enum_flag(wt->srna, "draw_options", rna_enum_draw_options, 0, "Draw Options", "");

	WM_gizmotype_target_property_def(wt, "offset", PROP_FLOAT, 3);
}

void ED_gizmotypes_grab_3d(void)
{
	WM_gizmotype_append(GIZMO_WT_grab_3d);
}

/** \} */ // Grab Gizmo API