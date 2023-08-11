/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

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

//Stage constants
//#define STAGE_NOHUD //Disable the HUD

int note_x[8];
int note_y[8];

static const uint16_t note_key[] = {INPUT_LEFT, INPUT_DOWN, INPUT_UP, INPUT_RIGHT};
static const uint8_t note_anims[4][3] = {
    {CharAnim_Left,  CharAnim_LeftAlt,  PlayerAnim_LeftMiss},
    {CharAnim_Down,  CharAnim_DownAlt,  PlayerAnim_DownMiss},
    {CharAnim_Up,    CharAnim_UpAlt,    PlayerAnim_UpMiss},
    {CharAnim_Right, CharAnim_RightAlt, PlayerAnim_RightMiss},
};


//Stage definitions
bool noteshake;
static uint32_t Sounds[7];

#include "stage/week1.h"
#include "stage/void.h"

static const StageDef stage_defs[StageId_Max] = {
    #include "stagedef_disc1.h"
};

//Stage states
Stage stage;
Debug debug;

//Stage music functions
static void Stage_StartVocal(void)
{
    if (!(stage.flag & STAGE_FLAG_VOCAL_ACTIVE))
    {
        Audio_SetVolume(2, 0x3FFF, 0x3FFF);
//        Audio_ChannelXA(stage.stage_def->music_channel);
        stage.flag |= STAGE_FLAG_VOCAL_ACTIVE;
    }
}

static void Stage_CutVocal(void)
{
    if (stage.flag & STAGE_FLAG_VOCAL_ACTIVE)
    {
        Audio_SetVolume(2, 0x0000, 0x0000);
//        Audio_ChannelXA(stage.stage_def->music_channel + 1);
        stage.flag &= ~STAGE_FLAG_VOCAL_ACTIVE;
    }
}

//Stage camera functions
static void Stage_FocusCharacter(Character *ch, fixed_t div)
{
    //if (ch == stage.player)
    //    printf("focusing on player with x %d and y %d and z %d\n", ch->focus_x, ch->focus_y, ch->focus_zoom);
  //  else
//        printf("focusing on opponent with x %d and y %d and z %d\n", ch->focus_x, ch->focus_y, ch->focus_zoom);
    //Use character focus settings to update target position and zoom
    stage.camera.tx = ch->focus_x;
    stage.camera.ty = ch->focus_y;
    stage.camera.tz = ch->focus_zoom;
    stage.camera.td = div;
}

static void Stage_ScrollCamera(void)
{
    if (stage.prefs.debug)
        Debug_ScrollCamera();
    else 
    {
        if (stage.freecam)
        {
            if (pad_state.held & PAD_LEFT)
                stage.camera.x -= FIXED_DEC(2,1);
            if (pad_state.held & PAD_UP)
                stage.camera.y -= FIXED_DEC(2,1);
            if (pad_state.held & PAD_RIGHT)
                stage.camera.x += FIXED_DEC(2,1);
            if (pad_state.held & PAD_DOWN)
                stage.camera.y += FIXED_DEC(2,1);
            if (pad_state.held & PAD_TRIANGLE)
                stage.camera.zoom -= FIXED_DEC(1,100);
            if (pad_state.held & PAD_CROSS)
                stage.camera.zoom += FIXED_DEC(1,100);
        }
        else
        {
            //stage.camera.x = FIXED_LERP(stage.camera.x, stage.camera.tx, stage.camera.speed);
          //  stage.camera.y = FIXED_LERP(stage.camera.y, stage.camera.ty, stage.camera.speed);
        //    stage.camera.zoom = FIXED_LERP(stage.camera.zoom, stage.camera.tz, stage.camera.speed);
            
            //Scroll based off current divisor
            stage.camera.x = FIXED_LERP(stage.camera.x, stage.camera.tx, stage.camera.speed);
            stage.camera.y = FIXED_LERP(stage.camera.y, stage.camera.ty, stage.camera.speed);
            stage.camera.zoom = FIXED_LERP(stage.camera.zoom, stage.camera.tz, stage.camera.speed);
            stage.camera.angle = FIXED_LERP(stage.camera.angle, stage.camera.ta << FIXED_SHIFT, stage.camera.speed+FIXED_DEC(4,10));
            stage.camera.hudangle = FIXED_LERP(stage.camera.hudangle, stage.camera.hudta << FIXED_SHIFT, stage.camera.speed);
            
        }
    }
        
    //Update other camera stuff
    stage.camera.bzoom = FIXED_MUL(stage.camera.zoom, stage.bump);

}

//Stage section functions
static void Stage_ChangeBPM(uint16_t bpm, uint16_t step)
{
    //Update last BPM
    stage.last_bpm = bpm;
    
    //Update timing base
    if (stage.step_crochet)
        stage.time_base += FIXED_DIV(((fixed_t)step - stage.step_base) << FIXED_SHIFT, stage.step_crochet);
    stage.step_base = step;
    
    //Get new crochet and times
    stage.step_crochet = ((fixed_t)bpm << FIXED_SHIFT) * 8 / 240; //15/12/24
    stage.step_time = FIXED_DIV(FIXED_DEC(12,1), stage.step_crochet);
    
    //Get new crochet based values
    stage.early_safe = stage.late_safe = stage.step_crochet / 6; //10 frames
    stage.late_sus_safe = stage.late_safe;
    stage.early_sus_safe = stage.early_safe * 2 / 5;
}

static Section *Stage_GetPrevSection(Section *section)
{
    if (section > stage.sections)
        return section - 1;
    return NULL;
}

static uint16_t Stage_GetSectionStart(Section *section)
{
    Section *prev = Stage_GetPrevSection(section);
    if (prev == NULL)
        return 0;
    return prev->end;
}

//Section scroll structure
typedef struct
{
    fixed_t start;   //Seconds
    fixed_t length;  //Seconds
    uint16_t start_step;  //Sub-steps
    uint16_t length_step; //Sub-steps
    
    fixed_t size; //Note height
} SectionScroll;

static void Stage_GetSectionScroll(SectionScroll *scroll, Section *section)
{
    //Get BPM
    uint16_t bpm = section->flag & SECTION_FLAG_BPM_MASK;
    
    //Get section step info
    scroll->start_step = Stage_GetSectionStart(section);
    scroll->length_step = section->end - scroll->start_step;
    
    //Get section time length
    scroll->length = (scroll->length_step * FIXED_DEC(15,1) / 12) * 24 / bpm;
    
    //Get note height
    scroll->size = FIXED_MUL(stage.speed, scroll->length * (12 * 150) / scroll->length_step) + FIXED_UNIT;
}

//Note hit detection
static uint8_t Stage_HitNote(PlayerState *this, uint8_t type, fixed_t offset)
{
    //Get hit type
    if (offset < 0)
        offset = -offset;
    
    uint8_t hit_type;
    if (offset > stage.late_safe * 81 / 100)
        hit_type = 3; //SHIT
    else if (offset > stage.late_safe * 54 / 100)
        hit_type = 2; //BAD
    else if (offset > stage.late_safe * 3 / 11)
        hit_type = 1; //GOOD
    else
        hit_type = 0; //SICK
    
    //Increment combo and score
    this->combo++;
    
    static const int32_t score_inc[] = {
        35, //SICK
        20, //GOOD
        10, //BAD
         5, //SHIT
    };
    this->score += score_inc[hit_type];

    this->min_accuracy += 1;

    if (hit_type == 3)
        this->max_accuracy += 4;
    else if (hit_type == 2)
        this->max_accuracy += 3;
    else if (hit_type == 1)
        this->max_accuracy += 2;
    else
        this->max_accuracy += 1;
    
    this->refresh_accuracy = true;
    this->refresh_score = true;
    
    //Restore vocals and health
    Stage_StartVocal();

    if (hit_type != 3 && hit_type != 2) //dont increase if shit or bad
        this->health += 300;
    
    //Create combo object telling of our combo
    Obj_Combo *combo = Obj_Combo_New(
        this->character->focus_x,
        this->character->focus_y,
        hit_type,
        this->combo >= 10 ? this->combo : 0xFFFF
    );
    if (combo != NULL)
        ObjectList_Add(&stage.objlist_fg, (Object*)combo);
    
    //Create note splashes if SICK
    if (hit_type == 0)
    {
        for (int i = 0; i < 3; i++)
        {
            //Create splash object
            Obj_Splash *splash = Obj_Splash_New(
                note_x[type],
                note_y[type] * (stage.prefs.downscroll ? -1 : 1),
                type & 0x3
            );
            if (splash != NULL)
                ObjectList_Add(&stage.objlist_splash, (Object*)splash);
        }
    }
    
    return hit_type;
}

static void Stage_MissNote(PlayerState *this)
{
    this->max_accuracy += 1;
    this->refresh_accuracy = true;
    this->miss += 1;
    this->refresh_miss = true;

    if (this->combo)
    {
        //Kill combo
        if (stage.gf != NULL && this->combo > 5)
            stage.gf->set_anim(stage.gf, CharAnim_DownAlt); //Cry if we lost a large combo
        this->combo = 0;
        
        //Create combo object telling of our lost combo
        Obj_Combo *combo = Obj_Combo_New(
            this->character->focus_x,
            this->character->focus_y,
            0xFF,
            0
        );
        if (combo != NULL)
            ObjectList_Add(&stage.objlist_fg, (Object*)combo);
    }
}

static void Stage_NoteCheck(PlayerState *this, uint8_t type)
{
    //Perform note check
    for (Note *note = stage.cur_note;; note++)
    {
        if ((note->type & NOTE_FLAG_P2SING))
        {
            stage.player2sing = "single";
        }
        else            
            stage.player2sing = "none";

//        if (!(note->type & NOTE_FLAG_MINE))
  //      {
            //Check if note can be hit
            fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
            if (note_fp - stage.early_safe > stage.note_scroll)
                break;
            if (note_fp + stage.late_safe < stage.note_scroll)
                continue;
            if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || (note->type & NOTE_FLAG_SUSTAIN))
                continue;
            
            //Hit the note
            note->type |= NOTE_FLAG_HIT;

           this->character->set_anim(this->character, note_anims[type & 0x3][(note->type & NOTE_FLAG_ALT_ANIM) != 0]);

            uint8_t hit_type = Stage_HitNote(this, type, stage.note_scroll - note_fp);
            this->arrow_hitan[type & 0x3] = stage.step_time;
            (void)hit_type;
            return;
       /* }
        else
        {
            //Check if mine can be hit
            fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
            if (note_fp - (stage.late_safe * 3 / 5) > stage.note_scroll)
                break;
            if (note_fp + (stage.late_safe * 2 / 5) < stage.note_scroll)
                continue;
            if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || (note->type & NOTE_FLAG_SUSTAIN))
                continue;
            
            //Hit the mine
            note->type |= NOTE_FLAG_HIT;
    
            this->health -= 2000;

            if (this->character->spec & CHAR_SPEC_MISSANIM)
                this->character->set_anim(this->character, note_anims[type & 0x3][2]);
            else
                this->character->set_anim(this->character, note_anims[type & 0x3][0]);
            this->arrow_hitan[type & 0x3] = -1;
            
            return;
        }*/
    }
    
    //Missed a note
    this->arrow_hitan[type & 0x3] = -1;
    
    if (!stage.prefs.ghost)
    {
        if (stage.prefs.sfxmiss) 
            Audio_PlaySound(Sounds[RandomRange(4,6)], 0xBB8); //Randomly plays a miss sound
        
        if (this->character->spec & CHAR_SPEC_MISSANIM)
            this->character->set_anim(this->character, note_anims[type & 0x3][2]);

        else
            this->character->set_anim(this->character, note_anims[type & 0x3][0]);
        Stage_MissNote(this);
        
        this->health -= 430;
        this->score -= 1;
        this->refresh_score = true;
    }
}

static void Stage_SustainCheck(PlayerState *this, uint8_t type)
{
    //Perform note check
    for (Note *note = stage.cur_note;; note++)
    {
        if ( (note->type & NOTE_FLAG_P2SING))
        {
            stage.player2sing = "single";
        }
        else            
            stage.player2sing = "none";
        //Check if note can be hit
        fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
        if (note_fp - stage.early_sus_safe > stage.note_scroll)
            break;
        if (note_fp + stage.late_sus_safe < stage.note_scroll)
            continue;
        if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || !(note->type & NOTE_FLAG_SUSTAIN))
            continue;
        
        //Hit the note
        note->type |= NOTE_FLAG_HIT;
        
        this->character->set_anim(this->character, note_anims[type & 0x3][(note->type & NOTE_FLAG_ALT_ANIM) != 0]);
        
        Stage_StartVocal();
        this->health += 230;
        this->arrow_hitan[type & 0x3] = stage.step_time;
            
    }
}

static void CheckNewScore()
{
    if (stage.mode == StageMode_Normal && !stage.prefs.botplay && !stage.prefs.practice && timer.timermin == 0 && timer.timer <= 5)
    {
        if (stage.player_state[0].score >= stage.prefs.savescore[stage.stage_id][stage.stage_diff])
            stage.prefs.savescore[stage.stage_id][stage.stage_diff] = stage.player_state[0].score;          
    }
}

static void Stage_ProcessPlayer(PlayerState *this, Pad *pad, bool playing)
{
    //Handle player note presses
    if (stage.prefs.botplay == 0) {
        if (playing)
        {
            uint8_t i = ((this->character == stage.opponent) || (this->character == stage.opponent2)) ? NOTE_FLAG_OPPONENT : 0;
            
            this->pad_held = this->character->pad_held = pad->held;
            this->pad_press = pad->press;
            
            if (this->pad_held & INPUT_LEFT)
                Stage_SustainCheck(this, 0 | i);
            if (this->pad_held & INPUT_DOWN)
                Stage_SustainCheck(this, 1 | i);
            if (this->pad_held & INPUT_UP)
                Stage_SustainCheck(this, 2 | i);
            if (this->pad_held & INPUT_RIGHT)
                Stage_SustainCheck(this, 3 | i);
            
            if (this->pad_press & INPUT_LEFT)
                Stage_NoteCheck(this, 0 | i);
            if (this->pad_press & INPUT_DOWN)
                Stage_NoteCheck(this, 1 | i);
            if (this->pad_press & INPUT_UP)
                Stage_NoteCheck(this, 2 | i);
            if (this->pad_press & INPUT_RIGHT)
                Stage_NoteCheck(this, 3 | i);
        }
        else
        {
            this->pad_held = this->character->pad_held = 0;
            this->pad_press = 0;
        }
    }
    
    if (stage.prefs.botplay == 1) {
        //Do perfect note checks
        if (playing)
        {
            uint8_t i = ((this->character == stage.opponent) || (this->character == stage.opponent2)) ? NOTE_FLAG_OPPONENT : 0;
            
            uint8_t hit[4] = {0, 0, 0, 0};
            for (Note *note = stage.cur_note;; note++)
            {
                //Check if note can be hit
                fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
                if (note_fp - stage.early_safe - FIXED_DEC(12,1) > stage.note_scroll)
                    break;
                if (note_fp + stage.late_safe < stage.note_scroll)
                    continue;
                if ((note->type & NOTE_FLAG_OPPONENT) != i)
                    continue;
                
                //Handle note hit
                if (!(note->type & NOTE_FLAG_SUSTAIN))
                {
                    if (note->type & NOTE_FLAG_HIT)
                        continue;
                    if (stage.note_scroll >= note_fp)
                        hit[note->type & 0x3] |= 1;
                    else if (!(hit[note->type & 0x3] & 8))
                        hit[note->type & 0x3] |= 2;
                }
                else if (!(hit[note->type & 0x3] & 2))
                {
                    if (stage.note_scroll <= note_fp)
                        hit[note->type & 0x3] |= 4;
                    hit[note->type & 0x3] |= 8;
                }
            }
            
            //Handle input
            this->pad_held = 0;
            this->pad_press = 0;
            
            for (uint8_t j = 0; j < 4; j++)
            {
                if (hit[j] & 5)
                {
                    this->pad_held |= note_key[j];
                    Stage_SustainCheck(this, j | i);
                }
                if (hit[j] & 1)
                {
                    this->pad_press |= note_key[j];
                    Stage_NoteCheck(this, j | i);
                }
            }
            
            this->character->pad_held = this->pad_held;
        }
        else
        {
            this->pad_held = this->character->pad_held = 0;
            this->pad_press = 0;
        }
    }
}

//Stage HUD functions
static void Stage_DrawHealth(int16_t health, uint8_t i, int8_t ox)
{
	//Check if we should use 'dying' frame
    int8_t dying;
    int8_t winning;
    if (ox < 0)
    {
        dying = (health >= 18000) * 46;
        winning = (health <= 2000) * 46*2;
    }
    else
    {
    dying = (health <= 2000) * 46;
    winning = (health >= 18000) * 46*2;
    }

    //Get src and dst
    fixed_t hx = (128 << FIXED_SHIFT) * (10000 - health) / 10000;
    RECT src = {
        (i % 1) * 114 + dying + winning,
        16 + (i / 1) * 46,
        46,
        46,
    };
    RECT_FIXED dst = {
        hx + ox * FIXED_DEC(23,1) - FIXED_DEC(23,1),
        FIXED_DEC(screen.SCREEN_HEIGHT2 - 32 + 4 - 23, 1),
        src.w << FIXED_SHIFT,
        src.h << FIXED_SHIFT
    };
    if (stage.prefs.downscroll)
        dst.y = -dst.y - dst.h;

    dst.y += stage.noteshakey;
    dst.x += stage.noteshakex;

    //Draw health icon
    if (stage.mode == StageMode_Swap)
    {
        dst.w = -dst.w;
        dst.x += FIXED_DEC(46,1);
    }
    else
    {
        dst.w = dst.w;
        dst.x = dst.x;
    }

    Stage_DrawTex(&stage.tex_hud1, &src, &dst, FIXED_MUL(stage.bump, stage.sbump), stage.camera.hudangle);
}

static void Stage_DrawHealthBar(int16_t x, int32_t color)
{   
    //colors for health bar
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t blue = (color >> 8) & 0xFF;
    uint8_t green = (color) & 0xFF;
    //Get src and dst
    RECT src = {
        0,
        0,
        x,
        8
    };
        RECT_FIXED dst = {
            FIXED_DEC(-128,1), 
            (screen.SCREEN_HEIGHT2 - 32) << FIXED_SHIFT, 
            FIXED_DEC(src.w,1), 
            FIXED_DEC(8,1)
        };

    if (stage.prefs.downscroll)
        dst.y = -dst.y - dst.h;
    
    Stage_DrawTexCol(&stage.tex_hud1, &src, &dst, stage.bump, stage.camera.hudangle, red >> 1, blue >> 1, green >> 1);
}

static void Stage_Player2(void)
{
    //check which mode you choose
    static char* checkoption;

    //change mode to single(only opponent2 sing)
    if (strcmp(stage.player2sing, "single") == 0 && checkoption != stage.player2sing)
    {
        if (stage.mode == StageMode_Swap)
        {
            stage.player_state[1].character->pad_held = 0;
            stage.player_state[1].character = stage.player2;
        }
        else
        {
            stage.player_state[0].character->pad_held = 0;
            stage.player_state[0].character = stage.player2;
        }
    }

    //change mode to none (opponent2 don't sing)
    else if (strcmp(stage.player2sing, "none") == 0 && checkoption != stage.player2sing)
    {
        if (stage.mode == StageMode_Swap)
        {
            stage.player_state[1].character->pad_held = 0;
            stage.player_state[1].character = stage.player;
        }
        else
        {
            stage.player_state[0].character->pad_held = 0;
            stage.player_state[0].character = stage.player;
        }
    }

    if (checkoption != stage.player2sing)
        checkoption = stage.player2sing;
}

static void Stage_Opponent2(void)
{
    //check which mode you choose
    static char* checkoption;

    //change mode to single(only opponent2 sing)
    if (strcmp(stage.oppo2sing, "single") == 0 && checkoption != stage.oppo2sing)
    {
        if (stage.mode == StageMode_Swap)
        {
            stage.player_state[0].character->pad_held = 0;
            stage.player_state[0].character = stage.opponent2;
        }
        else
        {
            stage.player_state[1].character->pad_held = 0;
            stage.player_state[1].character = stage.opponent2;
        }
    }

    //change mode to none (opponent2 don't sing)
    else if (strcmp(stage.oppo2sing, "none") == 0 && checkoption != stage.oppo2sing)
    {
        if (stage.mode == StageMode_Swap)
        {
            stage.player_state[0].character->pad_held = 0;
            stage.player_state[0].character = stage.opponent;
        }
        else
        {
            stage.player_state[1].character->pad_held = 0;
            stage.player_state[1].character = stage.opponent;
        }
    }

    if (checkoption != stage.oppo2sing)
        checkoption = stage.oppo2sing;
}

static void Stage_DrawStrum(uint8_t i, RECT *note_src, RECT_FIXED *note_dst)
{
    (void)note_dst;
    
    PlayerState *this = &stage.player_state[((i ^ stage.note_swap) & NOTE_FLAG_OPPONENT) != 0];
    i &= 0x3;
    
    if (this->arrow_hitan[i] > 0)
    {
        //Play hit animation
        uint8_t frame = ((this->arrow_hitan[i] << 1) / stage.step_time) & 1;
        note_src->x = (i + 1) << 5;
        note_src->y = 64 - (frame << 5);
        
        this->arrow_hitan[i] -= Timer_GetDT();
        if (this->arrow_hitan[i] <= 0)
        {
            if (this->pad_held & note_key[i])
                this->arrow_hitan[i] = 1;
            else
                this->arrow_hitan[i] = 0;
        }
    }
    else if (this->arrow_hitan[i] < 0)
    {
        //Play depress animation
        note_src->x = (i + 1) << 5;
        note_src->y = 96;
        if (!(this->pad_held & note_key[i]))
            this->arrow_hitan[i] = 0;
    }
    else
    {
        note_src->x = 0;
        note_src->y = i << 5;
    }
}

static void Stage_DrawNotes(void)
{
    //Check if opponent should draw as bot
    uint8_t bot = (stage.mode >= StageMode_2P) ? 0 : NOTE_FLAG_OPPONENT;
    
    //Initialize scroll state
    SectionScroll scroll;
    scroll.start = stage.time_base;
    
    Section *scroll_section = stage.section_base;
    Stage_GetSectionScroll(&scroll, scroll_section);
    
    //Push scroll back until cur_note is properly contained
    while (scroll.start_step > stage.cur_note->pos)
    {
        //Look for previous section
        Section *prev_section = Stage_GetPrevSection(scroll_section);
        if (prev_section == NULL)
            break;
        
        //Push scroll back
        scroll_section = prev_section;
        Stage_GetSectionScroll(&scroll, scroll_section);
        scroll.start -= scroll.length;
    }
    
    //Draw notes
    for (Note *note = stage.cur_note; note->pos != 0xFFFF; note++)
    {
        //Update scroll
        while (note->pos >= scroll_section->end)
        {
            //Push scroll forward
            scroll.start += scroll.length;
            Stage_GetSectionScroll(&scroll, ++scroll_section);
        }
        
        //Get note information
        uint8_t i = ((note->type ^ stage.note_swap) & NOTE_FLAG_OPPONENT) != 0;
        PlayerState *this = &stage.player_state[i];
        
        fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
        fixed_t time = (scroll.start - stage.song_time) + (scroll.length * (note->pos - scroll.start_step) / scroll.length_step);
        fixed_t y = note_y[(note->type & 0x7)] + FIXED_MUL(stage.speed, time * 150);
        
        //Check if went above screen
        if (y < FIXED_DEC(-16 - screen.SCREEN_HEIGHT2, 1))
        {
            //Wait for note to exit late time
            if (note_fp + stage.late_safe >= stage.note_scroll)
                continue;
            
            //Miss note if player's note
            if (!((note->type ^ stage.note_swap) & (bot | NOTE_FLAG_HIT)))
            {
                if (stage.mode < StageMode_Net1 || i == ((stage.mode == StageMode_Net1) ? 0 : 1))
                {
                    //Missed note
                    Stage_CutVocal();
                    Stage_MissNote(this);
                    this->health -= 475;
                    
                }
            }
            
            //Update current note
            stage.cur_note++;
        }
        else
        {
            //Don't draw if below screen
            RECT note_src;
            RECT_FIXED note_dst;
            if (y > (FIXED_DEC(screen.SCREEN_HEIGHT,2) + scroll.size) || note->pos == 0xFFFF)
                break;
            
            //Draw note
            if (note->type & NOTE_FLAG_SUSTAIN)
            {
                //Check for sustain clipping
                fixed_t clip;
                y -= scroll.size;
                if (((note->type ^ stage.note_swap) & (bot | NOTE_FLAG_HIT)) || ((this->pad_held & note_key[note->type & 0x3]) && (note_fp + stage.late_sus_safe >= stage.note_scroll)))
                {
                    clip = note_y[(note->type & 0x7)] - y;
                    if (clip < 0)
                        clip = 0;
                }
                else
                {
                    clip = 0;
                }
                
                //Draw sustain
                if (note->type & NOTE_FLAG_SUSTAIN_END)
                {
                    if (clip < (24 << FIXED_SHIFT))
                    {
                        note_src.x = 160;
                        note_src.y = ((note->type & 0x3) << 5) + 4 + (clip >> FIXED_SHIFT);
                        note_src.w = 32;
                        note_src.h = 28 - (clip >> FIXED_SHIFT);
                        
                        note_dst.x = stage.noteshakex + note_x[(note->type & 0x7)] - FIXED_DEC(16,1);
                        note_dst.y = stage.noteshakey + y + clip;
                        note_dst.w = note_src.w << FIXED_SHIFT;
                        note_dst.h = (note_src.h << FIXED_SHIFT);
                        
                        if (stage.prefs.downscroll)
                        {
                            note_dst.y = -note_dst.y;
                            note_dst.h = -note_dst.h;
                        }
                        //draw for opponent
                        if (stage.prefs.middlescroll && note->type & NOTE_FLAG_OPPONENT)
                            Stage_BlendTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, 4, stage.camera.hudangle);
                        else
                            Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, stage.camera.hudangle);
                    }
                }
                else
                {
                    //Get note height
                    fixed_t next_time = (scroll.start - stage.song_time) + (scroll.length * (note->pos + 12 - scroll.start_step) / scroll.length_step);
                    fixed_t next_y = note_y[(note->type & 0x7)] + FIXED_MUL(stage.speed, next_time * 150) - scroll.size;
                    fixed_t next_size = next_y - y;
                    
                    if (clip < next_size)
                    {
                        note_src.x = 160;
                        note_src.y = ((note->type & 0x3) << 5);
                        note_src.w = 32;
                        note_src.h = 16;
                        
                        note_dst.x = stage.noteshakex + note_x[(note->type & 0x7)] - FIXED_DEC(16,1);
                        note_dst.y = stage.noteshakey + y + clip;
                        note_dst.w = note_src.w << FIXED_SHIFT;
                        note_dst.h = (next_y - y) - clip;
                        
                        if (stage.prefs.downscroll)
                            note_dst.y = -note_dst.y - note_dst.h;
                        //draw for opponent
                        if (stage.prefs.middlescroll && note->type & NOTE_FLAG_OPPONENT)
                            Stage_BlendTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, 4, stage.camera.hudangle);
                        else
                            Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, stage.camera.hudangle);
                    }
                }
            }
            else
            {
                //Don't draw if already hit
                if (note->type & NOTE_FLAG_HIT)
                    continue;
                
                //Draw note
                note_src.x = 32 + ((note->type & 0x3) << 5);
                note_src.y = 0;
                note_src.w = 32;
                note_src.h = 32;
                
                note_dst.x = stage.noteshakex + note_x[(note->type & 0x7)] - FIXED_DEC(16,1);
                note_dst.y = stage.noteshakey + y - FIXED_DEC(16,1);
                note_dst.w = note_src.w << FIXED_SHIFT;
                note_dst.h = note_src.h << FIXED_SHIFT;
                
                if (stage.prefs.downscroll)
                    note_dst.y = -note_dst.y - note_dst.h;
                //draw for opponent
                if (stage.prefs.middlescroll && note->type & NOTE_FLAG_OPPONENT)
                    Stage_BlendTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, 4, stage.camera.hudangle);
                else
                    Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, stage.camera.hudangle);
            }
        }
    }
}

int drawshit = 0;
static void Stage_CountDown(void)
{
    if (stage.flag &= STAGE_FLAG_JUST_STEP)
    {
        switch(stage.song_step)
        {
            case -20:
                Audio_PlaySound(Sounds[0], 0x3fff); //3
                break;
            case -15:
                drawshit = 3; 
                Audio_PlaySound(Sounds[1], 0x3fff); //2 
                break;
            case -10:
                drawshit = 2;
                Audio_PlaySound(Sounds[2], 0x3fff); //1
                break;
            case -5:
                drawshit = 1;
                Audio_PlaySound(Sounds[3], 0x3fff); //go
                break;
        }
    }
    RECT ready_src = {197, 112, 58, 118};   
    RECT_FIXED ready_dst = {FIXED_DEC(10,1), FIXED_DEC(30,1), FIXED_DEC(58 * 2,1), FIXED_DEC(118 * 2,1)};   

    RECT set_src = {211, 65, 44, 94};   
    RECT_FIXED set_dst = {FIXED_DEC(10,1), FIXED_DEC(40,1), FIXED_DEC(54 * 2,1), FIXED_DEC(96 * 2,1)};  

    RECT go_src = {207, 17, 48, 95};    
    RECT_FIXED go_dst = {FIXED_DEC(10,1), FIXED_DEC(30,1), FIXED_DEC(48 * 2,1), FIXED_DEC(95 * 2,1)};   

    if (drawshit == 3 && stage.song_step >= -15 && stage.song_step <= -12)
        Stage_DrawTex(&stage.tex_hud1, &ready_src, &ready_dst, stage.bump, FIXED_DEC(90,1));
    else if (drawshit == 3 && stage.song_step >= -12 && stage.song_step <= -11)
        Stage_BlendTex(&stage.tex_hud1, &ready_src, &ready_dst, stage.bump, FIXED_DEC(90,1), 4);

    if (drawshit == 2 && stage.song_step >= -10 && stage.song_step <= -7)
        Stage_DrawTex(&stage.tex_hud0, &set_src, &set_dst, stage.bump, FIXED_DEC(90,1));
    else if (drawshit == 2 && stage.song_step >= -7 && stage.song_step <= -6)
        Stage_BlendTex(&stage.tex_hud0, &set_src, &set_dst, stage.bump, FIXED_DEC(90,1), 4);

    if (drawshit == 1 && stage.song_step >= -5 && stage.song_step <= -2)
        Stage_DrawTex(&stage.tex_hud1, &go_src, &go_dst, stage.bump, FIXED_DEC(90,1));
    else if (drawshit == 1 && stage.song_step >= -2 && stage.song_step <= -1)
        Stage_BlendTex(&stage.tex_hud1, &go_src, &go_dst, stage.bump, FIXED_DEC(90,1), 4);
}

//Stage loads
static void Stage_LoadPlayer(void)
{
    //Load player character
    Character_Free(stage.player);
    if (stage.stage_def->pchar.path != NULL)
        stage.player = Character_FromFile(stage.player, stage.stage_def->pchar.path, stage.stage_def->pchar.x, stage.stage_def->pchar.y);
    else
        stage.player = NULL;
}

static void Stage_LoadPlayer2(void)
{
    //Load player character
    Character_Free(stage.player2);
    
    if (stage.stage_def->pchar2.path != NULL) {
        stage.player2 = Character_FromFile(stage.player2, stage.stage_def->pchar2.path, stage.stage_def->pchar2.x, stage.stage_def->pchar2.y);
    }
    else
        stage.player2 = NULL;
    
}

static void Stage_LoadOpponent(void)
{
    //Load opponent character
    Character_Free(stage.opponent);
    if (stage.stage_def->ochar.path != NULL)
        stage.opponent = Character_FromFile(stage.opponent, stage.stage_def->ochar.path, stage.stage_def->ochar.x, stage.stage_def->ochar.y);
    else
        stage.opponent = NULL;
}

static void Stage_LoadOpponent2(void)
{
    //Load opponent character
    Character_Free(stage.opponent2);
    if (stage.stage_def->ochar2.path != NULL) {
        stage.opponent2 = Character_FromFile(stage.opponent2, stage.stage_def->ochar2.path, stage.stage_def->ochar2.x, stage.stage_def->ochar2.y);;
    }
    else
        stage.opponent2 = NULL;
}

static void Stage_LoadGirlfriend(void)
{
    //Load girlfriend character
    Character_Free(stage.gf);
    if (stage.stage_def->gchar.path != NULL)
        stage.gf =  Character_FromFile(stage.gf, stage.stage_def->gchar.path, stage.stage_def->gchar.x, stage.stage_def->gchar.y);
    else
        stage.gf = NULL;
}

static void Stage_LoadStage(void)
{
    //Load back
    if (stage.back != NULL)
        stage.back->free(stage.back);
    stage.back = stage.stage_def->back();
}

static void Stage_LoadChart(void)
{
    //Load stage data
    char chart_path[64];
    
    //Use standard path convention
    sprintf(chart_path, "\\WEEK%d\\%d.%d%c.CHT;1", stage.stage_def->week, stage.stage_def->week, stage.stage_def->week_song, "ENH"[stage.stage_diff]);
    
    
    if (stage.chart_data != NULL)
        free(stage.chart_data);
    stage.chart_data = IO_Read(chart_path);
    uint8_t *chart_byte = (uint8_t*)stage.chart_data;

        //Directly use section and notes pointers
        stage.sections = (Section*)(chart_byte + 6);
        stage.notes = (Note*)(chart_byte + ((uint16_t*)stage.chart_data)[2]);
        
        for (Note *note = stage.notes; note->pos != 0xFFFF; note++)
            stage.num_notes++;
    
    //Count max scores
    stage.player_state[0].max_score = 0;
    stage.player_state[1].max_score = 0;
    for (Note *note = stage.notes; note->pos != 0xFFFF; note++)
    {
        if (note->type & (NOTE_FLAG_SUSTAIN))
            continue;
        if (note->type & NOTE_FLAG_OPPONENT)
            stage.player_state[1].max_score += 35;
        else
            stage.player_state[0].max_score += 35;
    }
    if (stage.mode >= StageMode_2P && stage.player_state[1].max_score > stage.player_state[0].max_score)
        stage.max_score = stage.player_state[1].max_score;
    else
        stage.max_score = stage.player_state[0].max_score;
    
    stage.cur_section = stage.sections;
    stage.cur_note = stage.notes;
    
    stage.speed = *((fixed_t*)stage.chart_data);
    
    stage.step_crochet = 0;
    stage.time_base = 0;
    stage.step_base = 0;
    stage.section_base = stage.cur_section;
    Stage_ChangeBPM(stage.cur_section->flag & SECTION_FLAG_BPM_MASK, 0);
}

static void Stage_LoadSFX(void)
{
    //Load SFX
    CdlFILE file;

    //intro sound
    for (uint8_t i = 0; i < 4;i++)
    {
        char text[0x80];
        sprintf(text, "\\SOUNDS\\INTRO%d%s.VAG;1", i, "");
        Sounds[i] = Audio_LoadSound(text);
    }

    //miss sound
    if (stage.prefs.sfxmiss)
    {
        for (uint8_t i = 0; i < 3;i++)
        {
            char text[0x80];
            sprintf(text, "\\SOUNDS\\MISS%d.VAG;1", i + 1);
            Sounds[i + 4] = Audio_LoadSound(text);
        }
    }
}

static void Stage_LoadMusic(void)
{
    //Offset sing ends
    if (stage.player != NULL)
        stage.player->sing_end -= stage.note_scroll;
    if (stage.player2 != NULL)
        stage.player2->sing_end -= stage.note_scroll;
    if (stage.opponent != NULL)
        stage.opponent->sing_end -= stage.note_scroll;
    if (stage.opponent2 != NULL)
        stage.opponent2->sing_end -= stage.note_scroll;
    if (stage.gf != NULL)
        stage.gf->sing_end -= stage.note_scroll;
    
    //Find music file and begin seeking to it
    char audio_path[64];
    sprintf(audio_path, "\\MUSIC\\%d_%d.VAG;1", stage.stage_def->week, stage.stage_def->week_song);
    Audio_LoadStream(audio_path, false);

    //Initialize music state
    stage.note_scroll = FIXED_DEC(-5 * 6 * 12,1);
    stage.song_time = FIXED_DIV(stage.note_scroll, stage.step_crochet);
    stage.interp_time = 0;
    stage.interp_ms = 0;
    stage.interp_speed = 0;
    
    //Offset sing ends again

    if (stage.player != NULL)
        stage.player->sing_end += stage.note_scroll;
    if (stage.player2 != NULL)
        stage.player2->sing_end += stage.note_scroll;
    if (stage.opponent != NULL)
        stage.opponent->sing_end += stage.note_scroll;
    if (stage.opponent2 != NULL)
        stage.opponent2->sing_end += stage.note_scroll;
    if (stage.gf != NULL)
        stage.gf->sing_end += stage.note_scroll;
}

bool str_done, str_canplay;
static void Stage_LoadState(void)
{
    //Initialize stage state
    stage.flag = STAGE_FLAG_VOCAL_ACTIVE;
    
    stage.gf_speed = 1 << 2;
    
    stage.state = StageState_Play;
    
    if (stage.mode == StageMode_Swap)
    {
        if (stage.opponent != NULL)
            stage.player_state[0].character = stage.opponent;
        stage.player_state[1].character = stage.player;
    }
    else
    {   
        stage.player_state[0].character = stage.player;
        
        if (stage.opponent != NULL)
            stage.player_state[1].character = stage.opponent;
    }

    for (int i = 0; i < 2; i++)
    {
        memset(stage.player_state[i].arrow_hitan, 0, sizeof(stage.player_state[i].arrow_hitan));
        
        stage.player_state[i].health = 10000;
        stage.player_state[i].combo = 0;
        stage.oppo2sing = "none";
        stage.player2sing = "none";
        drawshit = 1;
        if (!stage.prefs.debug)
            stage.freecam = 0;
        stage.player_state[i].miss = 0;
        stage.player_state[i].accuracy = 0;
        stage.player_state[i].max_accuracy = 0;
        stage.player_state[i].min_accuracy = 0;
        stage.player_state[i].refresh_score = false;
        stage.player_state[i].score = 0;
        stage.song_beat = 0;
        stage.bumpspeed = 16;
        timer.secondtimer = 0;
        timer.timer = 0;
        timer.timermin = 0;     
        timer.timersec = 0;
        str_done = false;
        str_canplay = true;
        stage.paused = false;
        strcpy(stage.player_state[i].accuracy_text, "Accuracy: ?");
        strcpy(stage.player_state[i].miss_text, "Misses: 0");
        strcpy(stage.player_state[i].score_text, "Score: 0");
        
        stage.player_state[i].pad_held = stage.player_state[i].pad_press = 0;
    }
    
    //BF
    note_y[0] = FIXED_DEC(32 - screen.SCREEN_HEIGHT2 + 5, 1);
    note_y[1] = FIXED_DEC(32 - screen.SCREEN_HEIGHT2 + 5, 1);//+34
    note_y[2] = FIXED_DEC(32 - screen.SCREEN_HEIGHT2 + 5, 1);
    note_y[3] = FIXED_DEC(32 - screen.SCREEN_HEIGHT2 + 5, 1);
    //Opponent
    note_y[4] = FIXED_DEC(32 - screen.SCREEN_HEIGHT2 + 5, 1);
    note_y[5] = FIXED_DEC(32 - screen.SCREEN_HEIGHT2 + 5, 1);//+34
    note_y[6] = FIXED_DEC(32 - screen.SCREEN_HEIGHT2 + 5, 1);
    note_y[7] = FIXED_DEC(32 - screen.SCREEN_HEIGHT2 + 5, 1);

    //middle note x
    if(stage.prefs.middlescroll)
    {
        //bf
        note_x[0] = FIXED_DEC(26 - 78,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
        note_x[1] = FIXED_DEC(60 - 78,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4); //+34
        note_x[2] = FIXED_DEC(94 - 78,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
        note_x[3] = FIXED_DEC(128 - 78,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
        //opponent
        note_x[4] = FIXED_DEC(-50 - 78,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
        note_x[5] = FIXED_DEC(-16 - 78,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4); //+34
        note_x[6] = FIXED_DEC(170 - 78,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
        note_x[7] = FIXED_DEC(204 - 78,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
    }
    else
    {
        //bf
        note_x[0] = FIXED_DEC(26,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
        note_x[1] = FIXED_DEC(60,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4); //+34
        note_x[2] = FIXED_DEC(94,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
        note_x[3] = FIXED_DEC(128,1) + FIXED_DEC(screen.SCREEN_WIDEADD,4);
        //opponent
        note_x[4] = FIXED_DEC(-128,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
        note_x[5] = FIXED_DEC(-94,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4); //+34
        note_x[6] = FIXED_DEC(-60,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
        note_x[7] = FIXED_DEC(-26,1) - FIXED_DEC(screen.SCREEN_WIDEADD,4);
    }

    ObjectList_Free(&stage.objlist_splash);
    ObjectList_Free(&stage.objlist_fg);
    ObjectList_Free(&stage.objlist_bg);
}

//Stage functions

void Stage_Load(StageId id, StageDiff difficulty, bool story)
{
    //Get stage definition
    stage.stage_def = &stage_defs[stage.stage_id = id];
    stage.stage_diff = difficulty;
    stage.story = story;
    
    //Load HUD textures
        Gfx_LoadTex(&stage.tex_hud0, IO_Read("\\STAGE\\HUD0.TIM;1"), GFX_LOADTEX_FREE);
    char iconpath[30];
    sprintf(iconpath, "\\STAGE\\HUD1-%d.TIM;1", stage.stage_def->week);
    Gfx_LoadTex(&stage.tex_hud1, IO_Read(iconpath), GFX_LOADTEX_FREE);

    //Load stage background
    Stage_LoadStage();

    //load fonts
    FontData_Load(&stage.font_cdr, Font_CDR);
    FontData_Load(&stage.font_bold, Font_Bold);
    
    //Load characters
    Stage_LoadPlayer();
    Stage_LoadPlayer2();
    Stage_LoadOpponent();
    Stage_LoadOpponent2();  
    Stage_LoadGirlfriend();

    //Load stage chart
    Stage_LoadChart();

    //Initialize stage state
    stage.story = story;
    
    Stage_LoadState();
    
    //Initialize camera
    if (stage.cur_section->flag & SECTION_FLAG_OPPFOCUS)
        Stage_FocusCharacter(stage.opponent, FIXED_UNIT);
    else 
        Stage_FocusCharacter(stage.player, FIXED_UNIT);
    
    stage.camera.hudangle = stage.camera.angle = stage.camera.ta = stage.camera.hudta = 0;

    stage.camera.speed = FIXED_DEC(5,100);
    stage.camera.x = stage.camera.tx;
    stage.camera.y = stage.camera.ty;
    stage.camera.zoom = stage.camera.tz;
    stage.camera.angle = stage.camera.ta;
    stage.camera.hudangle = stage.camera.hudta;
    
    stage.bump = FIXED_UNIT;
    stage.sbump = FIXED_UNIT;
    
    //Initialize stage according to mode
    stage.note_swap = (stage.mode == StageMode_Swap && (!(stage.prefs.middlescroll))) ? 4 : 0;
    
    //Load music
    stage.note_scroll = 0;
    Stage_LoadMusic();
    //Load SFX
    Stage_LoadSFX();
    
    //Test offset
    stage.offset = stage.prefs.audio_offset;
    printf("[Stage_Load] Done (id=%d)\n", id);
}

void Stage_Unload(void)
{
    //Disable net mode to not break the game
    if (stage.mode >= StageMode_Net1)
        stage.mode = StageMode_Normal;
    
    //Unload stage background
    if (stage.back != NULL)
        stage.back->free(stage.back);
    stage.back = NULL;
    
    //Unload stage data
    free(stage.chart_data);
    stage.chart_data = NULL;
    
    //Free objects
    ObjectList_Free(&stage.objlist_splash);
    ObjectList_Free(&stage.objlist_fg);
    ObjectList_Free(&stage.objlist_bg);
    
    //Free characters
    Character_Free(stage.player);
    stage.player = NULL;
    Character_Free(stage.player2);
    stage.player2 = NULL;
    Character_Free(stage.opponent);
    stage.opponent = NULL;
    Character_Free(stage.opponent2);
    stage.opponent2 = NULL;
    Character_Free(stage.gf);
    stage.gf = NULL;
    
    Audio_DestroyStream();
}

static bool Stage_NextLoad(void)
{
    CheckNewScore();
    writeSaveFile();

    uint8_t load = stage.stage_def->next_load;
    if (load == 0)
    {
        //Do stage transition if full reload
        stage.trans = StageTrans_NextSong;
        Trans_Start();
        return false;
    }
    else
    {
        //Get stage definition
        stage.stage_def = &stage_defs[stage.stage_id = stage.stage_def->next_stage];
        
        //Load stage background
        if (load & STAGE_LOAD_STAGE)
            Stage_LoadStage();
        
        //Load characters
        if (load & STAGE_LOAD_PLAYER)
        {
            Stage_LoadPlayer();
        }
        else if (stage.player != NULL)
        {
            stage.player->x = stage.stage_def->pchar.x;
            stage.player->y = stage.stage_def->pchar.y;
        }
        if (load & STAGE_LOAD_PLAYER2)
        {
            Stage_LoadPlayer2();
        }
        else if (stage.player2 != NULL)
        {
            stage.player2->x = stage.stage_def->pchar2.x;
            stage.player2->y = stage.stage_def->pchar2.y;
        }

        if (load & STAGE_LOAD_OPPONENT)
        {
            Stage_LoadOpponent();
        }
        else if (stage.opponent != NULL)
        {
            stage.opponent->x = stage.stage_def->ochar.x;
            stage.opponent->y = stage.stage_def->ochar.y;
        }
        if (load & STAGE_LOAD_OPPONENT2)
        {
            Stage_LoadOpponent2();
        }
        else if (stage.opponent2 != NULL)
        {
            stage.opponent2->x = stage.stage_def->ochar2.x;
            stage.opponent2->y = stage.stage_def->ochar2.y;
        }
        if (load & STAGE_LOAD_GIRLFRIEND)
        {
            Stage_LoadGirlfriend();
        }
        else if (stage.gf != NULL)
        {
            stage.gf->x = stage.stage_def->gchar.x;
            stage.gf->y = stage.stage_def->gchar.y;
        }
        
        //Load stage chart
        Stage_LoadChart();
        
        //Initialize stage state
        Stage_LoadState();
        
        //Load music
        Stage_LoadMusic();
        
        //Reset timer
        Timer_Reset();
        return true;
    }
}

static int deadtimer;
static bool inctimer;

void Stage_Tick(void)
{
    SeamLoad:;
    
    if (stage.state != StageState_STR)
    {
        //Tick transition
        if (stage.paused == false && pad_state.press & PAD_START && stage.state == StageState_Play && stage.song_time > 0)
        {
            stage.pause_scroll = -1;
            pad_state.press = false;
            Audio_StopStream();
            stage.paused = true;
        }

        if (stage.paused)
        {
            switch (stage.pause_state)
            {
                case 0:
                    PausedState();
                    break;
                case 1:
                    OptionsState((int *)&note_x);
                    break;
            }

        }

        if (pad_state.press & (PAD_START | PAD_CROSS) && stage.state != StageState_Play)
        {
            if (deadtimer == 0)
            {
                inctimer = true;
                Audio_StopStream();
                Audio_PlaySound(Sounds[1], 0x3fff);
            }
        }
        else if (pad_state.press & PAD_CIRCLE && stage.state != StageState_Play)
        {
            stage.trans = StageTrans_Menu;
            Trans_Start();
        }

        if (inctimer)
            deadtimer ++;

        if (deadtimer == 200 && stage.state != StageState_Play)
        {
            stage.trans = StageTrans_Reload;
            Trans_Start();
        }

        if (Trans_Tick())
        {
            switch (stage.trans)
            {
                case StageTrans_Menu:
                    CheckNewScore();
                    writeSaveFile();
                    //Load appropriate menu
                    Stage_Unload();
                    
                    LoadScr_Start();
            
                        if (stage.stage_id <= StageId_LastVanilla)
                        {
                            if (stage.story)
                                Menu_Load(MenuPage_Story);
                            else
                                Menu_Load(MenuPage_Freeplay);
                        }
                        else
                        {
                            Menu_Load(MenuPage_Credits);
                        }
                    
                    LoadScr_End();
                    
                    gameloop = GameLoop_Menu;
                    return;
                case StageTrans_NextSong:
                    //Load next song
                    Stage_Unload();
                    
                    LoadScr_Start();
                    Stage_Load(stage.stage_def->next_stage, stage.stage_diff, stage.story);
                    LoadScr_End();
                    break;
                case StageTrans_Reload:
                    //Reload song
                    Stage_Unload();
                    
                    LoadScr_Start();
                    Stage_Load(stage.stage_id, stage.stage_diff, stage.story);
                    LoadScr_End();
                    break;
            }
        }
         
        if (stage.story && !str_done && str_canplay)
        {
            Trans_Clear();
            for (int i = 0; i < COUNT_OF(movies); i++){ 
                if (movies[i].stage == stage.stage_id)
                { 
                    str_done = false;
                    str_canplay = false;
                    stage.state = StageState_STR;
                    STR_StartStream(movies[i].path);
                    break; 
                } 
            } 
        }
    }
    switch (stage.state)
    {
        case StageState_STR:
        { 
            if (!stage.str_playing)
            {                           
                str_done = true;
                stage.state = StageState_Play;
                Gfx_Flip();
                Trans_Set();
            }
        }
        case StageState_Play:
        {
            Character *movecam[2];
            //move camera
            if (!stage.prefs.debug)
            {
                if (stage.cur_section->flag & SECTION_FLAG_OPPFOCUS)
                {
                    if (strcmp(stage.oppo2sing, "single") == 0) 
                        movecam[1] = stage.opponent2;
                    else
                        movecam[0] = stage.opponent;
                }
                else
                {
                    if (strcmp(stage.player2sing, "single") == 0) 
                        movecam[1] = stage.player2;
                    else
                        movecam[0] = stage.player;
                }
                for (int i = 0; i < 2; i++)
                {
                    if (movecam[i] == NULL)
                        continue;
                    switch (movecam[i]->animatable.anim)
                    {
                        case CharAnim_Up: stage.camera.y -= FIXED_DEC(2,10); break;
                        case CharAnim_Down: stage.camera.y += FIXED_DEC(2,10); break;
                        case CharAnim_Left:  if (movecam[i] == stage.player || movecam[i] == stage.player2) stage.camera.angle -= FIXED_DEC(15,10); else stage.camera.angle += FIXED_DEC(15,10); stage.camera.x -= FIXED_DEC(2,10); break;
                        case CharAnim_Right: if (movecam[i] == stage.player || movecam[i] == stage.player2) stage.camera.angle += FIXED_DEC(15,10); else stage.camera.angle -= FIXED_DEC(15,10); stage.camera.x += FIXED_DEC(2,10); break;
                        default:break;
                    }
                }
            }

            if (stage.prefs.songtimer)
                StageTimer_Draw();
            if (stage.prefs.debug)
                Debug_StageDebug();

 //           if (stage.opponent == NULL)
   //             FntPrint(-1, "oh nooo ur shit is null noo");
            if (stage.song_time >= 0)
            FntPrint(-1, "step %d, beat %d", stage.song_step, stage.song_beat);

            Stage_CountDown();

            Stage_Player2();
            Stage_Opponent2();

            if (stage.prefs.botplay)
            {
                //Draw botplay
                RECT bot_src = {174, 225, 67, 16};
                RECT_FIXED bot_dst = {FIXED_DEC(-33,1), FIXED_DEC(-60,1), FIXED_DEC(67,1), FIXED_DEC(16,1)};

                bot_dst.y += stage.noteshakey;
                bot_dst.x += stage.noteshakex;
                
                if (!stage.prefs.debug)
                    Stage_DrawTex(&stage.tex_hud0, &bot_src, &bot_dst, stage.bump, stage.camera.hudangle);
            }

            if (noteshake) 
            {
                stage.noteshakex = RandomRange(FIXED_DEC(-5,1),FIXED_DEC(5,1));
                stage.noteshakey = RandomRange(FIXED_DEC(-5,1),FIXED_DEC(5,1));
            }
            else
            {
                stage.noteshakex = 0;
                stage.noteshakey = 0;
            }

            //Clear per-frame flags
            stage.flag &= ~(STAGE_FLAG_JUST_STEP | STAGE_FLAG_SCORE_REFRESH);
            
            //Get song position
            bool playing;
            fixed_t next_scroll;
        
            if (!stage.paused)
            {
                if (stage.note_scroll < 0)
                {
                    //Play countdown sequence
                    stage.song_time += Timer_GetDT();
                        
                    //Update song
                    if (stage.song_time >= 0)
                    {
                        //Song has started
                        playing = true;

                        Audio_StartStream(false);//(stage.stage_def->music_track, 0x40, stage.stage_def->music_channel, 0);
                        
                        if (stage.flag & STAGE_FLAG_VOCAL_ACTIVE)      
                            stage.flag &= ~STAGE_FLAG_VOCAL_ACTIVE;
                        Stage_StartVocal();
                        
                        //Update song time
                        fixed_t audio_time = (fixed_t)Audio_GetTime(1000) - stage.offset;
                        if (audio_time < 0)
                            audio_time = 0;
                        stage.interp_ms = (audio_time << FIXED_SHIFT) / 1000;
                        stage.interp_time = 0;
                        stage.song_time = stage.interp_ms;
                    }
                    else
                    {
                        //Still scrolling
                        playing = false;
                    }
                    
                    //Update scroll
                    next_scroll = FIXED_MUL(stage.song_time, stage.step_crochet);
                }
                else if (Audio_IsPlaying())
                {
                    fixed_t audio_time_pof = (fixed_t)Audio_GetTime(1000);
                    fixed_t audio_time = (audio_time_pof > 0) ? (audio_time_pof - stage.offset) : 0;
                    
                    //Old sync
                    stage.interp_ms = (audio_time << FIXED_SHIFT) / 1000;
                    stage.interp_time = 0;
                    stage.song_time = stage.interp_ms;
                    
                    playing = true;
                    
                    //Update scroll
                    next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
                }
                else
                {
                    //Song has ended
                    playing = false;
                    stage.song_time += Timer_GetDT();
                        
                    //Update scroll
                    next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
                    
                    //Transition to menu or next song
                    if (stage.story && stage.stage_def->next_stage != stage.stage_id)
                    {
                        if (Stage_NextLoad())
                            goto SeamLoad;
                    }
                    else
                    {
                        stage.trans = StageTrans_Menu;
                        Trans_Start();
                    }
                }   
                RecalcScroll:;
                //Update song scroll and step
                if (next_scroll > stage.note_scroll)
                {
                    if (((stage.note_scroll / 12) & FIXED_UAND) != ((next_scroll / 12) & FIXED_UAND))
                        stage.flag |= STAGE_FLAG_JUST_STEP;
                    stage.note_scroll = next_scroll;
                    stage.song_step = (stage.note_scroll >> FIXED_SHIFT);
                    if (stage.note_scroll < 0)
                        stage.song_step -= 11;
                    stage.song_step /= 12;
                }
                
                //Update section
                if (stage.note_scroll >= 0)
                {
                    //Check if current section has ended
                    uint16_t end = stage.cur_section->end;
                    if ((stage.note_scroll >> FIXED_SHIFT) >= end)
                    {
                        //Increment section pointer
                        stage.cur_section++;
                        
                        //Update BPM
                        uint16_t next_bpm = stage.cur_section->flag & SECTION_FLAG_BPM_MASK;
                        Stage_ChangeBPM(next_bpm, end);
                        stage.section_base = stage.cur_section;
                        
                        //Recalculate scroll based off new BPM
                        next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
                        goto RecalcScroll;
                    }
                }
            }

            //Handle bump
            if ((stage.bump = FIXED_UNIT + FIXED_MUL(stage.bump - FIXED_UNIT, FIXED_DEC(95,100))) <= FIXED_DEC(1003,1000))
                stage.bump = FIXED_UNIT;
            stage.sbump = FIXED_UNIT + FIXED_MUL(stage.sbump - FIXED_UNIT, FIXED_DEC(60,100));
            
            if (playing && (stage.flag & STAGE_FLAG_JUST_STEP))
            {
                //Check if screen should bump
                bool is_bump_step = (stage.song_step & stage.bumpspeed-1) == 0;
     
                //Bump screen
                if (is_bump_step)
                    stage.bump = FIXED_DEC(103,100);

                //Bump health every 4 steps
                if ((stage.song_step & 0x3) == 0)
                    stage.sbump = FIXED_DEC(103,100);
            }
            
            //Scroll camera
            if (stage.cur_section->flag & SECTION_FLAG_OPPFOCUS)
                Stage_FocusCharacter(stage.opponent, FIXED_UNIT / 24);
            else 
                Stage_FocusCharacter(stage.player, FIXED_UNIT / 24);
            Stage_ScrollCamera();
            
            //Draw Score
            for (int i = 0; i < ((stage.mode >= StageMode_2P) ? 2 : 1); i++)
            {
                PlayerState *this = &stage.player_state[i];
                    
                if (this->refresh_score)
                {
                    if (this->score != 0)
                        sprintf(this->score_text, "Score: %d0", this->score * stage.max_score / this->max_score);
                    else
                        strcpy(this->score_text, "Score: 0");
                    this->refresh_score = false;
                }
                
                stage.font_cdr.draw(&stage.font_cdr,
                    this->score_text,
                    (stage.mode == StageMode_2P && i == 0) ? FIXED_DEC(10,1) : FIXED_DEC(-150,1), 
                    (screen.SCREEN_HEIGHT2 - 22) << FIXED_SHIFT,
                    FontAlign_Left
                );
            }
                
            //Draw Combo Break
            for (int i = 0; i < ((stage.mode >= StageMode_2P) ? 2 : 1); i++)
            {
                PlayerState *this = &stage.player_state[i];

                if (this->refresh_miss)
                {
                    if (this->miss != 0)
                        sprintf(this->miss_text, "Misses: %d", this->miss);
                    else
                        strcpy(this->miss_text, "Misses: 0");
                    this->refresh_miss = false;
                }

                stage.font_cdr.draw(&stage.font_cdr,
                    this->miss_text,
                    (stage.mode == StageMode_2P && i == 0) ? FIXED_DEC(100,1) : FIXED_DEC(-60,1), 
                    (screen.SCREEN_HEIGHT2 - 22) << FIXED_SHIFT,
                    FontAlign_Left
                );
            }
                
            //Draw Accuracy
            for (int i = 0; i < ((stage.mode >= StageMode_2P) ? 2 : 1); i++)
            {
                PlayerState *this = &stage.player_state[i];
                if (this->max_accuracy) // prevent division by zero
                    this->accuracy = (this->min_accuracy * 100) / (this->max_accuracy);

                //Rank
                if (this->accuracy == 100 && this->miss == 0)
                    strcpy(this->rank, "[SFC]");
                else if (this->accuracy >= 80 && this->miss == 0)
                    strcpy(this->rank, "[GFC]");
                else if (this->miss == 0)
                    strcpy(this->rank, "[FC]");
                else
                    strcpy(this->rank, "");
                
                if (this->refresh_accuracy)
                {
                    if (this->accuracy != 0)
                        sprintf(this->accuracy_text, "Accuracy: %d%% %s", this->accuracy, this->rank);
                    else
                        strcpy(this->accuracy_text, "Accuracy: ?"); 
                    this->refresh_accuracy = false;
                }
                //sorry for this shit lmao
                stage.font_cdr.draw(&stage.font_cdr,
                    this->accuracy_text,
                    (stage.mode == StageMode_2P && i == 0) ? FIXED_DEC(50,1) : (stage.mode == StageMode_2P && i == 1) ? FIXED_DEC(-110,1) : FIXED_DEC(39,1), 
                    (stage.mode == StageMode_2P) ? FIXED_DEC(85,1) : (screen.SCREEN_HEIGHT2 - 22) << FIXED_SHIFT,
                    FontAlign_Left
                );
            }
            
            switch (stage.mode)
            {
                case StageMode_Normal:
                case StageMode_Swap:
                {
                    //Handle player 1 inputs
                    Stage_ProcessPlayer(&stage.player_state[0], &pad_state, playing);
                    
                    //Handle opponent notes
                    uint8_t opponent_anote = CharAnim_Idle;
                    uint8_t opponent_snote = CharAnim_Idle;
                        
                        for (Note *note = stage.cur_note;; note++)
                    {
                        if (note->pos > (stage.note_scroll >> FIXED_SHIFT))
                            break;
                        
                        //Opponent note hits
                        if (playing && ((note->type ^ stage.note_swap) & NOTE_FLAG_OPPONENT) && !(note->type & NOTE_FLAG_HIT))
                        {
                            //Opponent hits note
                            stage.player_state[1].arrow_hitan[note->type & 0x3] = stage.step_time;
                            Stage_StartVocal();
                            if (note->type & NOTE_FLAG_SUSTAIN)
                                opponent_snote = note_anims[note->type & 0x3][(note->type & NOTE_FLAG_ALT_ANIM) != 0];
                            else
                                opponent_anote = note_anims[note->type & 0x3][(note->type & NOTE_FLAG_ALT_ANIM) != 0];
                            note->type |= NOTE_FLAG_HIT;
                        }
                    }
                    
                    if (opponent_anote != CharAnim_Idle)
                        stage.player_state[1].character->set_anim(stage.player_state[1].character, opponent_anote);
                    else if (opponent_snote != CharAnim_Idle)
                        stage.player_state[1].character->set_anim(stage.player_state[1].character, opponent_snote);
                    break;
                    break;
                }
                case StageMode_2P:
                {
                    //Handle player 1 and 2 inputs
                    Stage_ProcessPlayer(&stage.player_state[0], &pad_state, playing);
                    Stage_ProcessPlayer(&stage.player_state[1], &pad_state_2, playing);
                    break;
                }
            }

            if (!stage.prefs.debug)
            {
                if (stage.mode < StageMode_2P)
                {
                    //Perform health checks
                    if (stage.player_state[0].health <= 0 && stage.prefs.practice == 0)
                    {
                        //Player has died
                        stage.player_state[0].health = 0;
                            
                        stage.state = StageState_Dead;
                    }
                    if (stage.player_state[0].health > 20000)
                        stage.player_state[0].health = 20000;

                    if (stage.player_state[0].health <= 0 && stage.prefs.practice)
                        stage.player_state[0].health = 0;

                    //Draw health heads
                    Stage_DrawHealth(stage.player_state[0].health, stage.player_state[0].character->health_i,    1);
                    Stage_DrawHealth(stage.player_state[0].health, stage.player_state[1].character->health_i, -1);
                    
                    //Draw health bar
                    if (stage.mode == StageMode_Swap)
                    {
                        if (stage.player != NULL)
                            Stage_DrawHealthBar(255 - (255 * stage.player_state[0].health / 20000), stage.player->health_bar);
                        if (stage.opponent != NULL)
                            Stage_DrawHealthBar(255, stage.opponent->health_bar);
                    }
                    else
                    {   
                        if (stage.opponent != NULL)
                            Stage_DrawHealthBar(255 - (255 * stage.player_state[0].health / 20000), stage.opponent->health_bar);
                        if (stage.player != NULL)
                            Stage_DrawHealthBar(255, stage.player->health_bar);
                    }
                }
            
                //Tick note splashes
                ObjectList_Tick(&stage.objlist_splash);
                
                //Draw stage notes
                Stage_DrawNotes();
                
                //Draw note HUD
                RECT note_src = {0, 0, 32, 32};
                RECT_FIXED note_dst = {0, 0 + stage.noteshakey, FIXED_DEC(32,1), FIXED_DEC(32,1)};
                
                for (uint8_t i = 0; i < 4; i++)
                {
                    //BF
                    note_dst.x = stage.noteshakex + note_x[i] - FIXED_DEC(16,1);
                    note_dst.y = stage.noteshakey + note_y[i] - FIXED_DEC(16,1);
                    if (stage.prefs.downscroll)
                        note_dst.y = -note_dst.y - note_dst.h;
                    
                    Stage_DrawStrum(i, &note_src, &note_dst);

                    Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, stage.camera.hudangle);
                    
                    //Opponent
                    note_dst.x = stage.noteshakex + note_x[(i | 0x4)] - FIXED_DEC(16,1);
                    note_dst.y = stage.noteshakey + note_y[(i | 0x4)] - FIXED_DEC(16,1);
                    
                    if (stage.prefs.downscroll)
                        note_dst.y = -note_dst.y - note_dst.h;
                    Stage_DrawStrum(i | 4, &note_src, &note_dst);

                    if (stage.prefs.middlescroll)
                        Stage_BlendTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, 4, stage.camera.hudangle);
                    else
                        Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump, stage.camera.hudangle);
                }
            }

            //Tick foreground objects
            ObjectList_Tick(&stage.objlist_fg);

            //Draw stage foreground
            if (stage.back->draw_fg != NULL)
                stage.back->draw_fg(stage.back);
            
            
            //Tick characters
            if (stage.mode == StageMode_Swap)
            {
                if (stage.opponent != NULL)
                    stage.opponent->tick(stage.opponent);
                if (stage.player != NULL)
                    stage.player->tick(stage.player);
            }
            else
            {

                if (stage.player != NULL)
                    stage.player->tick(stage.player);
                if (stage.opponent != NULL)
                    stage.opponent->tick(stage.opponent);
            }
            if (stage.player2 != NULL)
                stage.player2->tick(stage.player2);

            if (stage.opponent2 != NULL)
                stage.opponent2->tick(stage.opponent2);
            
            //Draw stage middle
            if (stage.back->draw_md != NULL)
                stage.back->draw_md(stage.back);
            
            //Tick girlfriend
            if (stage.gf != NULL)
                stage.gf->tick(stage.gf);
            
            //Tick background objects
            ObjectList_Tick(&stage.objlist_bg);
            
            //Draw stage background
            if (stage.back->draw_bg != NULL)
                stage.back->draw_bg(stage.back);
            
            if (stage.song_step > 0)
                stage.song_beat = stage.song_step / 4;
                
            if (!stage.paused)  
                StageTimer_Tick();

            break;
        }
        case StageState_Dead: //Start BREAK animation and reading extra data from CD
        {
            //Stop music immediately
            Audio_StopStream();
            deadtimer = 0;
            inctimer = false;

            //Unload stage data
            Audio_ClearAlloc();
            free(stage.chart_data);
            stage.chart_data = NULL;
            
            //Free background
            stage.back->free(stage.back);
            stage.back = NULL;
            
            //Free objects
            ObjectList_Free(&stage.objlist_fg);
            ObjectList_Free(&stage.objlist_bg);
            
            //Free opponent and girlfriend
            Character_Free(stage.opponent);
            stage.opponent = NULL;
            Character_Free(stage.opponent2);
            stage.opponent2 = NULL;
            Character_Free(stage.gf);
            stage.gf = NULL;
            
            //Reset stage state
            stage.flag = 0;
            stage.bump = stage.sbump = FIXED_UNIT;
            
            //Change background colour to black
            Gfx_SetClear(0, 0, 0);
            
            //Run death animation, focus on player, and change state
            if (stage.player != NULL)
            {
                stage.player->set_anim(stage.player, PlayerAnim_Dead0);
                Stage_FocusCharacter(stage.player, 0);
            }
            stage.song_time = 0;
            
            stage.state = StageState_DeadLoad;
                Sounds[0] = Audio_LoadSound("\\SOUNDS\\LOSS.VAG;1");
            Audio_PlaySound(Sounds[0], 0x3fff);
        }
    //Fallthrough
        case StageState_DeadLoad:
        {
            //Scroll camera and tick player
            if (stage.song_time < FIXED_UNIT)
                stage.song_time += FIXED_UNIT / 60;
            stage.camera.td = FIXED_DEC(-2, 100) + FIXED_MUL(stage.song_time, FIXED_DEC(45, 1000));
            if (stage.camera.td > 0)
                Stage_ScrollCamera();
            if (stage.player != NULL)
                stage.player->tick(stage.player);
            
            //Drop mic and change state if CD has finished reading and animation has ended
            if (IO_IsReading() || stage.player->animatable.anim != PlayerAnim_Dead1)
                break;
            
            //load sounds
                Sounds[1] = Audio_LoadSound("\\SOUNDS\\END.VAG;1");
            
            if (stage.player != NULL)
                stage.player->set_anim(stage.player, PlayerAnim_Dead2);
            stage.camera.td = FIXED_DEC(25, 1000);
            stage.state = StageState_DeadDrop;
            break;
        }
        case StageState_DeadDrop:
        {
            //Scroll camera and tick player
            Stage_ScrollCamera();

            if (stage.player != NULL)
                stage.player->tick(stage.player);
            
            //Enter next state once mic has been dropped
            if (stage.player != NULL &&stage.player->animatable.anim == PlayerAnim_Dead3)
            {
                stage.state = StageState_DeadRetry;
                //Audio_PlayXA_Track(XA_GameOver, 0x40, 1, true);
                Audio_LoadStream("fdf", true);
                Audio_StartStream(false);
            }
            break;
        }
        case StageState_DeadRetry:
        {
            //Randomly twitch
            if (stage.player != NULL && stage.player->animatable.anim == PlayerAnim_Dead3)
            {
                if (RandomRange(0, 29) == 0)
                    stage.player->set_anim(stage.player, PlayerAnim_Dead4);
                if (RandomRange(0, 29) == 0)
                    stage.player->set_anim(stage.player, PlayerAnim_Dead5);
            }
            
            //Scroll camera and tick player
            Stage_ScrollCamera();
            if (stage.player != NULL)
                stage.player->tick(stage.player);
            break;
        }
        default:
            break;
    }
}
