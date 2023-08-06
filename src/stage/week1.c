/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week1.h"

#include "../archive.h"
#include <stdlib.h> 
#include "../stage.h"

//Week 1 background structure
typedef struct
{
    //Stage background base structure
    StageBack back;
    RECT coolfuckinguhhup_dst;
    RECT coolfuckinguhhdown_dst;
    
    //Textures
    Gfx_Tex tex_back0; //Stage and back
    Gfx_Tex tex_back1; //Curtains
} Back_Week1;

//Week 1 background functions
void Back_Week1_DrawFG(StageBack *back)
{
    Back_Week1 *this = (Back_Week1*)back;
    
    fixed_t fx, fy;
    
    //Draw curtains
    fx = stage.camera.x;
    fy = stage.camera.y;

    Gfx_DrawRect(&this->coolfuckinguhhup_dst, 0, 0, 0);
    Gfx_DrawRect(&this->coolfuckinguhhdown_dst, 0, 0, 0);

//    Gfx_BlendRect(&this->tex_back1, &dark_src, &dark_dst, stage.camera.bzoom, 1);

    RECT wall_src = {0, 0, 79, 256};
    RECT_FIXED wall_dst = {
        FIXED_DEC(-302,1)- fx,
        FIXED_DEC(-150,1) - fy,
        FIXED_DEC(103,1),
        FIXED_DEC(207,1)
    };
    if (stage.stage_id == StageId_MyAmazingWorld)
    {
        //init
        if (stage.song_step < 0)
        {
            this->coolfuckinguhhdown_dst = (RECT){
                0,
                screen.SCREEN_HEIGHT-32,
                screen.SCREEN_WIDTH,
                32
            };
            this->coolfuckinguhhup_dst = (RECT){
                0,
                0,
                screen.SCREEN_WIDTH,
                32
            };
            stage.opponent->focus_zoom = stage.camera.zoom = FIXED_DEC(23,10);

            stage.player->focus_x = FIXED_DEC(4,1);
            stage.player->focus_y = FIXED_DEC(-37,1);
            stage.player->focus_zoom = FIXED_DEC(13,10);
        }

        //pop up sillyi
        if (stage.song_step >= 258 && this->coolfuckinguhhup_dst.y+this->coolfuckinguhhup_dst.h >= 0)
        {
            this->coolfuckinguhhup_dst.y -= 2;
            this->coolfuckinguhhdown_dst.y += 2;
        }

        if (stage.song_step >= 160)
            stage.opponent->focus_x = FIXED_DEC(-120,1);
        else

        if (stage.opponent->focus_zoom >= FIXED_DEC(176, 100) && stage.song_step >= 0)
            stage.opponent->focus_zoom -= 1;
    }
    Debug_StageMoveDebug(&wall_dst, 5, fx, fy); 
    Stage_DrawTex(&this->tex_back1, &wall_src, &wall_dst, stage.camera.bzoom, stage.camera.angle);
}

void Back_Week1_DrawBG(StageBack *back)
{
    Back_Week1 *this = (Back_Week1*)back;
    
    fixed_t fx, fy;
    
    //Draw bg
    fx = stage.camera.x;
    fy = stage.camera.y;

    RECT back_src = {0, 0, 256, 256};
    RECT_FIXED back_dst = {
        FIXED_DEC(-250,1)- fx,
        FIXED_DEC(-150,1) - fy,
        FIXED_DEC(403,1),
        FIXED_DEC(216,1)
    };

    Debug_StageMoveDebug(&back_dst, 4, fx, fy);
    Stage_DrawTex(&this->tex_back0, &back_src, &back_dst, stage.camera.bzoom, stage.camera.angle);
}

void Back_Week1_Free(StageBack *back)
{
    Back_Week1 *this = (Back_Week1*)back;
    
    //Free structure
    free(this);
}

StageBack *Back_Week1_New(void)
{
    //Allocate background structure
    Back_Week1 *this = (Back_Week1*)malloc(sizeof(Back_Week1));
    if (this == NULL)
        return NULL;
    
    //Set background functions
    this->back.draw_fg = Back_Week1_DrawFG;
    this->back.draw_md = NULL;
    this->back.draw_bg = Back_Week1_DrawBG;
    this->back.free = Back_Week1_Free;
    
    //Load background textures
    IO_Data arc_back = IO_Read("\\WEEK1\\BACK.ARC;1");
    Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
    Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
    free(arc_back);
    
    return (StageBack*)this;
}
