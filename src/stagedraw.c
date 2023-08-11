#include "stage.h"
#include "stagedraw.h"

#include <stdlib.h>      
#include "timer.h"
#include "audio.h"
#include "pad.h"
#include "main.h"
#include "random.h"
#include "mutil.h"
#include "debug.h"
#include "save.h"

#include "menu.h"
#include "pause.h"
#include "trans.h"
#include "loadscr.h"
#include "str.h"

#include "object/combo.h"
#include "object/splash.h"

//Stage drawing functions
RECT getsdst(fixed_t rotation, const RECT_FIXED *dst, fixed_t zoom) {
    // Calculate the rotated coordinates
    uint8_t rotationAngle = rotation / FIXED_UNIT;  // Specify the desired rotation angle (in degrees)
    fixed_t rotatedX = FIXED_MUL((fixed_t)dst->x,FIXED_DEC(MUtil_Cos(rotationAngle),256)) - FIXED_MUL((fixed_t)dst->y,FIXED_DEC(MUtil_Sin(rotationAngle),256));
    fixed_t rotatedY = FIXED_MUL((fixed_t)dst->x,FIXED_DEC(MUtil_Sin(rotationAngle),256)) + FIXED_MUL((fixed_t)dst->y,FIXED_DEC(MUtil_Cos(rotationAngle),256));
    
    fixed_t l = (screen.SCREEN_WIDTH2  * FIXED_UNIT) + FIXED_MUL(rotatedX, zoom);// + FIXED_DEC(1,2);
    fixed_t t = (screen.SCREEN_HEIGHT2 * FIXED_UNIT) + FIXED_MUL(rotatedY, zoom);// + FIXED_DEC(1,2);
    fixed_t r = l + FIXED_MUL((fixed_t)dst->w, zoom);
    fixed_t b = t + FIXED_MUL((fixed_t)dst->h, zoom);
    
    l /= FIXED_UNIT;
    t /= FIXED_UNIT;
    r /= FIXED_UNIT;
    b /= FIXED_UNIT;
    
    RECT sdst = {
        l,
        t,
        r - l,
        b - t,
    };
    return sdst;
}

void Stage_Drawall(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, uint8_t cr, uint8_t cg, uint8_t cb, uint8_t mode, fixed_t zoom, fixed_t rotation)
{
    uint8_t rotationAngle = rotation / FIXED_UNIT; 
    RECT sdst = getsdst(rotation, dst, zoom);
	Gfx_Drawall(tex, src, &sdst, angle + rotationAngle, hx, hy, cr, cg, cb, mode);
}

void Stage_DrawTexRotateCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, uint8_t cr, uint8_t cg, uint8_t cb, fixed_t zoom, fixed_t rotation)
{
    Stage_Drawall(tex, src, dst, angle, hx, hy, cr, cg, cb, 0, zoom, rotation);
}

void Stage_BlendTexRotateCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, uint8_t cr, uint8_t cg, uint8_t cb, fixed_t zoom, fixed_t rotation, uint8_t mode)
{
    Stage_Drawall(tex, src, dst, angle, hx, hy, 0x80, 0x80, 0x80, 0, zoom, rotation);
}

void Stage_DrawTexRotate(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, fixed_t zoom, fixed_t rotation)
{
    Stage_Drawall(tex, src, dst, angle, hx, hy, 0x80, 0x80, 0x80, 0, zoom, rotation);
}

void Stage_BlendTexRotate(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, uint8_t angle, fixed_t hx, fixed_t hy, fixed_t zoom, fixed_t rotation, uint8_t mode)
{
    Stage_Drawall(tex, src, dst, angle, hx, hy, 0x80, 0x80, 0x80, mode, zoom, rotation);
}
//normal

void Stage_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, fixed_t rotation, uint8_t cr, uint8_t cg, uint8_t cb)
{
    Stage_Drawall(tex, src, dst, 0, 0, 0, cr, cg, cb, 0, zoom, rotation);
}

void Stage_BlendTexCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, fixed_t rotation, uint8_t cr, uint8_t cg, uint8_t cb, uint8_t mode)
{
    Stage_Drawall(tex, src, dst, 0, 0, 0, cr, cg, cb, mode, zoom, rotation);
}

void Stage_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, fixed_t rotation)
{
    Stage_Drawall(tex, src, dst, 0, 0, 0, 0x80, 0x80, 0x80, 0, zoom, rotation);
}

void Stage_BlendTex(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, uint8_t mode, fixed_t rotation)
{
    Stage_Drawall(tex, src, dst, 0, 0, 0, 0x80, 0x80, 0x80, mode, zoom, rotation);
}

void Stage_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom)
{
    Gfx_BlendTexArb(tex, src, p0, p1, p2, p3, 0);
}

void Stage_BlendTexArb(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom, uint8_t mode)
{
    //Don't draw if HUD and HUD is disabled
    #ifdef STAGE_NOHUD
        if (tex == &stage.tex_hud0 || tex == &stage.tex_hud1)
            return;
    #endif
    
    //Get screen-space points
    POINT s0 = {screen.SCREEN_WIDTH2 + (FIXED_MUL(p0->x, zoom) >> FIXED_SHIFT), screen.SCREEN_HEIGHT2 + (FIXED_MUL(p0->y, zoom) >> FIXED_SHIFT)};
    POINT s1 = {screen.SCREEN_WIDTH2 + (FIXED_MUL(p1->x, zoom) >> FIXED_SHIFT), screen.SCREEN_HEIGHT2 + (FIXED_MUL(p1->y, zoom) >> FIXED_SHIFT)};
    POINT s2 = {screen.SCREEN_WIDTH2 + (FIXED_MUL(p2->x, zoom) >> FIXED_SHIFT), screen.SCREEN_HEIGHT2 + (FIXED_MUL(p2->y, zoom) >> FIXED_SHIFT)};
    POINT s3 = {screen.SCREEN_WIDTH2 + (FIXED_MUL(p3->x, zoom) >> FIXED_SHIFT), screen.SCREEN_HEIGHT2 + (FIXED_MUL(p3->y, zoom) >> FIXED_SHIFT)};
    
    Gfx_BlendTexArb(tex, src, &s0, &s1, &s2, &s3, mode);
}
