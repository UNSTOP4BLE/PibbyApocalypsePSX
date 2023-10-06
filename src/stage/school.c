/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "school.h"

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
    bool white;
    bool popup;
    bool popdown;
    bool chvisible[3];

    //Textures
    Gfx_Tex tex_back0; 
    Gfx_Tex tex_back1;
    IO_Data channels, channels_ptr[3];
    Gfx_Tex curchannel;

} Back_school;

//Week 1 background functions
void checkandload(StageBack *back)
{
    Back_school *this = (Back_school*)back;

    if (this->chvisible[0])
    {
        Gfx_LoadTex(&this->curchannel, this->channels_ptr[0], 0);
        return;
    }
    else if (this->chvisible[1])
    {
        Gfx_LoadTex(&this->curchannel, this->channels_ptr[1], 0);
        return;
    }
    else if (this->chvisible[2])
    {
        Gfx_LoadTex(&this->curchannel, this->channels_ptr[2], 0);
        return;
    }
}

void Back_school_DrawFG(StageBack *back)
{
    Back_school *this = (Back_school*)back;
    
    fixed_t fx, fy;
    
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
    if (stage.stage_id == StageId_MyAmazingWorld && !stage.paused)
    {
        if (this->popup)
        {       
            this->coolfuckinguhhup_dst.y += 2;
            this->coolfuckinguhhdown_dst.y -= 2;
            if (this->coolfuckinguhhup_dst.y >= 0)
                this->popup = false;
        }
        if (this->popdown)
        {
            this->coolfuckinguhhup_dst.y -= 2;
            this->coolfuckinguhhdown_dst.y += 2;
            if (this->coolfuckinguhhup_dst.y+this->coolfuckinguhhup_dst.h <= 0)
                this->popdown = false;
        }
        //init
        if (stage.song_step < 0)
        {
            stage.player2->visible = false;
            Gfx_SetClear(0,0,0);
            for (int i = 0; i < COUNT_OF(this->chvisible); i++)
                this->chvisible[i] = false;

            this->white = false;
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
        else
        {
            this->coolfuckinguhhup_dst.w = this->coolfuckinguhhdown_dst.w = screen.SCREEN_WIDTH;
        }

        
        if (stage.opponent->focus_zoom >= FIXED_DEC(176, 100) && stage.song_step >= 0)
            stage.opponent->focus_zoom -= 1;

        if (stage.flag & STAGE_FLAG_JUST_STEP)
        {
            switch(stage.song_step)
            {
                case 0:
                    stage.bumpspeed = 8;
                    break;
                case 160:
                    stage.opponent->focus_x = FIXED_DEC(-120,1);
                    break;
                case 256:
                 stage.player->mode = 0;
                    this->popdown = true;
                    stage.bumpspeed = 4;
                    break;
                case 497: 
                    stage.bumpspeed = 16;
                    break;
                case 512:
                    stage.bumpspeed = 8;
                    this->white = true;
                    this->popup = true;
                    stage.player->r = stage.player->g = stage.player->b = stage.opponent->r = stage.opponent->g = stage.opponent->b = 0;
                    Gfx_SetClear(255,255,255);
                    break;
                case 990:
                    this->popdown = true;
                    stage.bumpspeed = 4;
                    break;
                case 1024:
                    this->white = false;
                    stage.player->r = stage.player->g = stage.player->b = stage.opponent->r = stage.opponent->g = stage.opponent->b = 128;
                    Gfx_SetClear(0,0,0);
                    this->popup = true;
                    break;
                case 1072:
                    this->popdown = true;
                    stage.bumpspeed = 16;
                    break;
                case 1280:
                    this->popup = true; 
                    break;
                case 1552: //hello darwin
                    //black screen
                    break;
                case 1568:
                    this->white = true;
                    stage.player2->visible = true;
                    stage.player->r = stage.player->g = stage.player->b = stage.opponent->r = stage.opponent->g = stage.opponent->b = stage.player2->r = stage.player2->g = stage.player2->b = 0;
                    stage.player->mode = 4;
                    Gfx_SetClear(255,255,255);
                    break; 
                case 1823:
                    stage.bumpspeed = 8;
                    this->white = false;
                    stage.player->r = stage.player->g = stage.player->b = stage.opponent->r = stage.opponent->g = stage.opponent->b = stage.player2->r = stage.player2->g = stage.player2->b = 128;
                    this->popdown = true;
                    Gfx_SetClear(0,0,0);
                    break; 
                case 2079:
                    stage.bumpspeed = 16;
                    this->white = true;
                    stage.player2->r = stage.player2->g = stage.player2->b = stage.opponent->r = stage.opponent->g = stage.opponent->b = 0;
                    stage.player->visible = false;
                    this->popdown = true;
                    Gfx_SetClear(255,255,255);
                    break; 
                case 2144:
                    stage.bumpspeed = 4;
                    this->white = false;
                    stage.player2->r = stage.player2->g = stage.player2->b = stage.opponent->r = stage.opponent->g = stage.opponent->b = 128;
                    Gfx_SetClear(0,0,0);
                    
                  //  PlayState.triggerEventNote('Camera Follow Pos', '940', '720');
                    //wall.visible = false;
                  //  vignette2.visible = false; 
                   // vignette.visible = false;
                //    background.visible = false;
                 //   light.visible = false;
                    //PlayState.gf.y = 720;

                    this->chvisible[0] = true;
                    checkandload(back);
                    stage.opponent->focus_x = stage.player->focus_x = FIXED_DEC(-64, 1);
                    printf("focusx %d\n", stage.opponent->focus_x/1024);
                    stage.opponent->focus_y = stage.player->focus_y = FIXED_DEC(-47, 1);
                    stage.opponent->focus_zoom = stage.player->focus_zoom = FIXED_DEC(1, 1);
                    
                    //move darwin
                    stage.player2->x = FIXED_DEC(20,1);
                    stage.player2->y = FIXED_DEC(51,1);
                    break;
                case 2176:
                    this->chvisible[0] = false;
                    this->chvisible[1] = true;
                    checkandload(back);
                    break;            
                case 2208:
                    this->chvisible[1] = false;
                    this->chvisible[2] = true;
                    checkandload(back);
                    break;
                case 2272:
                    this->chvisible[2] = false;
                    this->chvisible[0] = true;
                    checkandload(back);
                    break;           
                case 2304:
                    this->chvisible[0] = false;
                    this->chvisible[1] = true;
                    checkandload(back);
                    break;  
                case 2336:
                    this->chvisible[1] = false;
                    this->chvisible[2] = true;
                    checkandload(back);
                    break;  
                case 2400:
                    this->chvisible[2] = false;
                    this->chvisible[0] = true;
                    checkandload(back);
                    break;  
                case 2432:
                    this->chvisible[0] = false;
                    this->chvisible[1] = true;
                    checkandload(back);
                    break;       
                case 2464:
                    this->chvisible[1] = false;
                    this->chvisible[2] = true;
                    checkandload(back);
                    break;  
                case 2528:
                    this->chvisible[2] = false;
                    this->chvisible[0] = true;
                    checkandload(back);
                    break;  
                case 2560:
                    this->chvisible[0] = false;
                    this->chvisible[1] = true;
                    checkandload(back);
                    break;  
                case 2592:
                    this->chvisible[1] = false;
                    this->chvisible[2] = true;
                    checkandload(back);
                    break;  
                case 2604:
                    this->chvisible[2] = false;
                    this->chvisible[0] = true;
                    checkandload(back);
                    break;  
                case 2624:
                    this->chvisible[0] = false;
                    this->chvisible[1] = true;
                    checkandload(back);
                    break;  
                case 2632:
                    this->chvisible[1] = false;
                    this->chvisible[2] = true;
                    checkandload(back);
                    break;  
                case 2640:
                    this->chvisible[2] = false;
                    this->chvisible[0] = true;
                    checkandload(back);
                    break;  
                case 2648:
                    this->chvisible[0] = false;
                    this->chvisible[1] = true;
                    checkandload(back);
                    break;  
                case 2656:
    //                PlayState.triggerEventNote('Camera Follow Pos', '', '');
                    this->chvisible[1] = false;
                    this->chvisible[2] = true;
                    checkandload(back);
                    break;
                case 2688:
                    for (int i = 0; i < COUNT_OF(this->chvisible); i++)
                        this->chvisible[i] = false;

                    stage.player->visible = true;
                    stage.player->mode = 0;
      //              add(void);
        //            add(rock4);
          //          add(rock3);
            //        add(rock2);
              //      add(house);
                //    add(rock);
                //    add(wtf);
                  //  add(glitch);
                    //PlayState.gf.x = 1670;
                    //PlayState.gf.y = 900;
                   // PlayState.dad.x = 900;
                    //PlayState.dad.y = 740;
                    //PlayState.boyfriend.x = 1570;
                    //PlayState.boyfriend.y = 800;
                break;
                default:break;
            }
        }
    }
    if (stage.stage_id == StageId_ChildsPlay)
    {
        if (stage.song_step <= 120)
            stage.opponent->set_anim(stage.opponent, CharAnim_LeftAlt);
        switch (stage.song_step)
        {
            case 360:
                if (stage.flag &STAGE_FLAG_JUST_STEP)
                    stage.opponent = Stage_SwitchCharacter(stage.opponent, 0); 
                break;
        }
    }
    Debug_StageMoveDebug(&wall_dst, 7, fx, fy); 
    if (!this->white && stage.stage_id == StageId_MyAmazingWorld && stage.song_step <= 2144)
        Stage_DrawTex(&this->tex_back1, &wall_src, &wall_dst, stage.camera.bzoom, stage.camera.angle);
}

void Back_school_DrawBG(StageBack *back)
{
    Back_school *this = (Back_school*)back;
    
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

    Debug_StageMoveDebug(&back_dst, 8, fx, fy);
    if (!this->white && (stage.stage_id == StageId_MyAmazingWorld && stage.song_step <= 2144))
        Stage_DrawTex(&this->tex_back0, &back_src, &back_dst, stage.camera.bzoom, stage.camera.angle);

    if (stage.song_step >= 2144 && stage.song_step <= 2688 && stage.stage_id == StageId_MyAmazingWorld)
        Stage_DrawTex(&this->curchannel, &back_src, &back_dst, stage.camera.bzoom, stage.camera.angle);
    if (stage.charswitchable[0] == NULL)
        FntPrint(-1, "BALLSCACK");
}

void Back_school_LoadCharacterSwap(void)
{
    stage.charswitchable[0] = Character_FromFile(stage.charswitchable[0], "\\CHAR\\GUMBALL.CHR;1", stage.stage_def->ochar.x, stage.stage_def->ochar.y);
}

void Back_school_Free(StageBack *back)
{
    Back_school *this = (Back_school*)back;
    
    //Free structure
    free(this->channels);
    free(this);
}

StageBack *Back_school_New(void)
{
    //Allocate background structure
    Back_school *this = (Back_school*)malloc(sizeof(Back_school));
    if (this == NULL)
        return NULL;
    
    //Set background functions
    this->back.draw_fg = Back_school_DrawFG;
    this->back.draw_md = NULL;
    this->back.draw_bg = Back_school_DrawBG;
    this->back.load = Back_school_LoadCharacterSwap;
    this->back.free = Back_school_Free;
    
    //Load background textures
    IO_Data arc_back = IO_Read("\\SCHOOL\\BACK.ARC;1");
    Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
    Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
    free(arc_back);
    
    this->channels = IO_Read("\\SCHOOl\\CHANNELS.ARC;1");
    this->channels_ptr[0] = Archive_Find(this->channels, "ch0.tim");
    this->channels_ptr[1] = Archive_Find(this->channels, "ch1.tim");
    this->channels_ptr[2] = Archive_Find(this->channels, "ch2.tim");
    return (StageBack*)this;
}
