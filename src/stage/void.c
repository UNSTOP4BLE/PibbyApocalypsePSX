/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "void.h"

#include "../archive.h"
#include <stdlib.h> 
#include "../stage.h"

//void background structure
typedef struct
{
    //Stage background base structure
    StageBack back;
    RECT coolfuckinguhhup_dst;
    RECT coolfuckinguhhdown_dst;
    bool white;
    bool popup;
    bool popdown;

    //Textures
    Gfx_Tex tex_back0; 
    Gfx_Tex tex_back1;
    Gfx_Tex tex_back2;
    Gfx_Tex tex_back3;
} Back_void;

//void background functions
void Back_void_DrawFG(StageBack *back)
{
    Back_void *this = (Back_void*)back;
    
    fixed_t fx, fy;
    
    fx = stage.camera.x;
    fy = stage.camera.y;

    Gfx_DrawRect(&this->coolfuckinguhhup_dst, 0, 0, 0);
    Gfx_DrawRect(&this->coolfuckinguhhdown_dst, 0, 0, 0);
}

void Back_void_DrawBG(StageBack *back)
{
    Back_void *this = (Back_void*)back;
    
    fixed_t fx, fy;
    
    //Draw bg
    fx = stage.camera.x;
    fy = stage.camera.y;

    RECT back_src = {0, 0, 256, 256};
    RECT_FIXED back_dst = {
        FIXED_DEC(0,1)- fx,
        FIXED_DEC(0,1) - fy,
        FIXED_DEC(707,1),
        FIXED_DEC(442,1)
    };
    RECT island_src = {0, 101, 256, 155};
    RECT_FIXED island_dst = {
        FIXED_DEC(61,1)- fx,
        FIXED_DEC(276,1) - fy,
        FIXED_DEC(592,1),
        FIXED_DEC(166,1)
    };
    
    RECT house_src = {0, 101, 256, 155};
    RECT_FIXED island_dst = {
        FIXED_DEC(61,1)- fx,
        FIXED_DEC(276,1) - fy,
        FIXED_DEC(592,1),
        FIXED_DEC(166,1)
    };
    
    
    Debug_StageMoveDebug(&flyingshit_dst, 8, fx, fy); 
    //if (!this->white)
    Stage_DrawTex(&this->tex_back2, &flyingshit_src, &flyingshit_dst, stage.camera.bzoom, stage.camera.angle);

    Debug_StageMoveDebug(&house_dst, 8, fx, fy); 
    //if (!this->white)
    Stage_DrawTex(&this->tex_back2, &house_src, &house_dst, stage.camera.bzoom, stage.camera.angle);

    Debug_StageMoveDebug(&island_dst, 8, fx, fy); 
    //if (!this->white)
    Stage_DrawTex(&this->tex_back2, &island_src, &island_dst, stage.camera.bzoom, stage.camera.angle);

    Debug_StageMoveDebug(&back_dst, 7, fx, fy); 
    //if (!this->white)
    Stage_DrawTex(&this->tex_back0, &back_src, &back_dst, stage.camera.bzoom, stage.camera.angle);

}

void Back_void_Free(StageBack *back)
{
    Back_void *this = (Back_void*)back;
    
    //Free structure
    free(this);
}

StageBack *Back_void_New(void)
{
    //Allocate background structure
    Back_void *this = (Back_void*)malloc(sizeof(Back_void));
    if (this == NULL)
        return NULL;
    
    //Set background functions
    this->back.draw_fg = Back_void_DrawFG;
    this->back.draw_md = NULL;
    this->back.draw_bg = Back_void_DrawBG;
    this->back.free = Back_void_Free;
    
    //Load background textures
    IO_Data arc_back = IO_Read("\\VOID\\BACK.ARC;1");
    Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
    Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
    Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
    Gfx_LoadTex(&this->tex_back3, Archive_Find(arc_back, "back3.tim"), 0);
    free(arc_back);

    return (StageBack*)this;
}
