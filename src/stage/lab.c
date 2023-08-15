/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "lab.h"

#include "../archive.h"
#include <stdlib.h> 
#include "../stage.h"
#include "../mutil.h"

//lab background structure
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
    int funnyangle;
} Back_lab;

//lab background functions
void Back_lab_DrawFG(StageBack *back)
{
    Back_lab *this = (Back_lab*)back;
    
    fixed_t fx, fy;
    
    fx = stage.camera.x;
    fy = stage.camera.y;

    Gfx_DrawRect(&this->coolfuckinguhhup_dst, 0, 0, 0);
    Gfx_DrawRect(&this->coolfuckinguhhdown_dst, 0, 0, 0);
    RECT dark_src = {0, 0, 256, 256};
    RECT_FIXED dark_dst = {
        FIXED_DEC(-41,1)- fx,
        FIXED_DEC(-181,1) - fy,
        FIXED_DEC(633,1),
        FIXED_DEC(370,1)
    };
    if (!stage.paused)
    this->funnyangle ++;
    if (this->funnyangle >= 256)
        this->funnyangle = 0;
    Debug_StageMoveDebug(&dark_dst, 8, fx, fy); 
    Stage_BlendTexRotate(&this->tex_back1, &dark_src, &dark_dst, MUtil_Sin(this->funnyangle*2)/35, 128, 0, stage.camera.bzoom, stage.camera.angle, 4);

}

void Back_lab_DrawBG(StageBack *back)
{
    Back_lab *this = (Back_lab*)back;
    
    fixed_t fx, fy;
    
    //Draw bg
    fx = stage.camera.x;
    fy = stage.camera.y;

    RECT back_src = {0, 0, 256, 256};
    RECT_FIXED back_dst = {
        FIXED_DEC(-305,1)- fx,
        FIXED_DEC(-135,1) - fy,
        FIXED_DEC(524,1),
        FIXED_DEC(280,1)
    };
    Debug_StageMoveDebug(&back_dst, 7, fx, fy); 
    Stage_DrawTex(&this->tex_back0, &back_src, &back_dst, stage.camera.bzoom, stage.camera.angle);
}

void Back_lab_Free(StageBack *back)
{
    Back_lab *this = (Back_lab*)back;
    
    //Free structure
    free(this);
}

void Back_lab_LoadCharacterSwap(void)
{
    stage.player->focus_x = FIXED_DEC(40,1);
    stage.player->focus_y = FIXED_DEC(6,1);
    stage.player->focus_zoom = stage.opponent->focus_zoom = FIXED_DEC(1,1);
    stage.opponent->focus_x = FIXED_DEC(-129,1);
    stage.opponent->focus_y = FIXED_DEC(-9,1);
//    stage.charswitchable[0] = Character_FromFile(stage.charswitchable[0], "\\CHAR\\DARWIN.CHR;1", stage.stage_def->pchar.x, stage.stage_def->pchar.y);
}

StageBack *Back_lab_New(void)
{
    //Allocate background structure
    Back_lab *this = (Back_lab*)malloc(sizeof(Back_lab));
    if (this == NULL)
        return NULL;
    
    //Set background functions
    this->back.draw_fg = Back_lab_DrawFG;
    this->back.draw_md = NULL;
    this->back.draw_bg = Back_lab_DrawBG;
    this->back.load = Back_lab_LoadCharacterSwap;
    this->back.free = Back_lab_Free;
    
    //Load background textures
    IO_Data arc_back = IO_Read("\\LAB\\BACK.ARC;1");
    Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
    Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
    Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
    free(arc_back);

    this->funnyangle = 0;

    return (StageBack*)this;
}
