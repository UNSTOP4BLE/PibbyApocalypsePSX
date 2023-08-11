#ifndef PSXF_GUARD_STAGEDRAW_H
#define PSXF_GUARD_STAGEDRAW_H

#include "stage.h"

#include "io.h"
#include "gfx.h"
#include "pad.h"

#include "fixed.h"
#include "character.h"
#include "player.h"
#include "object.h"
#include "font.h"
#include "debug.h"

void Stage_Drawall(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, uint8_t cr, uint8_t cg, uint8_t cb, uint8_t mode, fixed_t zoom, fixed_t rotation);

//Stage drawing functions
void Stage_DrawTexRotateCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, uint8_t cr, uint8_t cg, uint8_t cb, fixed_t zoom, fixed_t rotation);
void Stage_BlendTexRotateCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, uint8_t cr, uint8_t cg, uint8_t cb, fixed_t zoom, fixed_t rotation, uint8_t mode);
void Stage_DrawTexRotate(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, fixed_t zoom, fixed_t rotation);
void Stage_BlendTexRotate(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, fixed_t zoom, fixed_t rotation, uint8_t mode);
void Stage_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, fixed_t rotation, uint8_t cr, uint8_t cg, uint8_t cb);
void Stage_BlendTexCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, fixed_t rotation, uint8_t cr, uint8_t cg, uint8_t cb, uint8_t mode);
void Stage_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, fixed_t rotation);
void Stage_BlendTex(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, uint8_t mode, fixed_t rotation);
void Stage_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom);
void Stage_BlendTexArb(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom, uint8_t mode);

#endif
