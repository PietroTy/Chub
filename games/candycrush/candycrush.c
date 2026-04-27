/**
 * @file candycrush.c — Match-3 with Candy Crush special gems
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib/raylib.h"
#include "game.h"
#include "hub.h"

#define BW 480
#define BH 510
#define GRID_W 8
#define GRID_H 8
#define CELL_SZ (BW/GRID_W)
#define GEM_TYPES 6
#define SPECIAL_NONE  0
#define SPECIAL_LINE  1
#define SPECIAL_BOMB  2
#define SPECIAL_COLOR 3

typedef struct { int type; int special; float pop_t; float swap_x; float swap_y; } Cell;

static Cell grid[GRID_H][GRID_W];
static int sel_r, sel_c, has_selection;
static int candycrush_score, candycrush_highscore;
static float anim_timer;
static int candycrush_state; /* 0=idle 1=swapping 2=popping 3=dropping 4=swapback */
static int to_remove[GRID_H][GRID_W];
static int swap_r1,swap_c1,swap_r2,swap_c2;
static int pivot_r,pivot_c; /* where the moved piece landed */
static Sound pop_sound, pop_special_sound;
static bool sound_loaded;

static Color gem_colors[GEM_TYPES] = {
    {255,80,80,255},{80,180,255,255},{80,255,100,255},
    {255,200,50,255},{200,80,255,255},{255,140,50,255},
};

static void draw_tetris_block(int x, int y, int sz, Color col) {
    Color hi = {(unsigned char)fmin(255, col.r + 90), (unsigned char)fmin(255, col.g + 90), (unsigned char)fmin(255, col.b + 90), 255};
    Color dk = {(unsigned char)(col.r * 0.4f), (unsigned char)(col.g * 0.4f), (unsigned char)(col.b * 0.4f), 255};
    DrawRectangle(x, y, sz, sz, dk);
    DrawRectangle(x, y, sz-2, sz-2, hi);
    DrawRectangle(x+2, y+2, sz-4, sz-4, col);
    DrawRectangle(x+4, y+4, 4, 4, hi); // Small interior glint square
}

static void draw_gem(int x, int y, int sz, int type, int special, Color col) {
    int b = sz / 3; // Sub-block size for 3x3 pattern
    int t = type % GEM_TYPES;
    int cx = x + sz/2, cy = y + sz/2;

    if (special == SPECIAL_COLOR) {
        // Master Gem: 3x3 with each block a different color
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                Color c = gem_colors[(i * 3 + j) % GEM_TYPES];
                draw_tetris_block(x + j * b, y + i * b, b, c);
            }
        }
        return;
    }

    // Normal drawing
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            bool draw = false;
            switch(t) {
                case 0: draw = true; break; // Full 3x3
                case 1: if (i == 0 || i == 2 || j == 0 || j == 2) draw = true; break; // Hollow 3x3
                case 2: if ((i == 0 && j == 1) || (i == 1 && (j == 0 || j == 2)) || (i == 2 && j == 1)) draw = true; break; // Diamond
                case 3: if (i == 1 || j == 1) draw = true; break; // Cross
                case 4: if (i == 1) draw = true; break; // Horizontal
                case 5: if (j == 1) draw = true; break; // Vertical
            }
            if (draw) {
                draw_tetris_block(x + j * b, y + i * b, b, col);
            }
        }
    }
    
    // Translucent Overlays for specials
    float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 10.0f);
    unsigned char alpha = (unsigned char)(140 * pulse);
    int pad = b / 4; // Margin to make them smaller
    int small_sz = b - pad * 2;

    if (special == SPECIAL_LINE) {
        // Lightning: Cross pattern overlay
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (i == 1 || j == 1) DrawRectangle(x + j * b + pad, y + i * b + pad, small_sz, small_sz, (Color){255, 255, 255, alpha});
            }
        }
    }
    else if (special == SPECIAL_BOMB) {
        // Bomb: Hollow pattern overlay
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (i == 0 || i == 2 || j == 0 || j == 2) DrawRectangle(x + j * b + pad, y + i * b + pad, small_sz, small_sz, (Color){255, 255, 255, alpha});
            }
        }
    }
}

static void fill_cell(int r,int c){grid[r][c].type=GetRandomValue(0,GEM_TYPES-1);grid[r][c].special=SPECIAL_NONE;grid[r][c].pop_t=0;grid[r][c].swap_x=0;grid[r][c].swap_y=0;}

static void fix_grid(void){
    for(int i=0;i<50;i++){int f=0;
        for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W-2;c++)if(grid[r][c].type==grid[r][c+1].type&&grid[r][c].type==grid[r][c+2].type){grid[r][c+2].type=(grid[r][c+2].type+1)%GEM_TYPES;f=1;}
        for(int c=0;c<GRID_W;c++)for(int r=0;r<GRID_H-2;r++)if(grid[r][c].type==grid[r+1][c].type&&grid[r][c].type==grid[r+2][c].type){grid[r+2][c].type=(grid[r+2][c].type+1)%GEM_TYPES;f=1;}
        if(!f)break;}
}

/* Returns 1 if matches found; creates specials at pivot but does NOT remove them */
static int check_matches(void) {
    int h_len[GRID_H][GRID_W]={0}, h_s[GRID_H][GRID_W];
    int v_len[GRID_H][GRID_W]={0}, v_s[GRID_H][GRID_W];
    int found=0;
    for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++){to_remove[r][c]=0;h_s[r][c]=c;v_s[r][c]=r;}

    /* H runs */
    for(int r=0;r<GRID_H;r++){int c=0;while(c<GRID_W){if(grid[r][c].type<0){c++;continue;}int t=grid[r][c].type,len=1;while(c+len<GRID_W&&grid[r][c+len].type==t)len++;if(len>=3){for(int k=0;k<len;k++){h_len[r][c+k]=len;h_s[r][c+k]=c;}found=1;}c+=len;}}
    /* V runs */
    for(int c=0;c<GRID_W;c++){int r=0;while(r<GRID_H){if(grid[r][c].type<0){r++;continue;}int t=grid[r][c].type,len=1;while(r+len<GRID_H&&grid[r+len][c].type==t)len++;if(len>=3){for(int k=0;k<len;k++){v_len[r+k][c]=len;v_s[r+k][c]=r;}found=1;}r+=len;}}
    if(!found)return 0;

    /* Mark to_remove */
    for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++)if(h_len[r][c]>=3||v_len[r][c]>=3)to_remove[r][c]=1;

    /* L/T: intersection of H>=3 and V>=3 -> BOMB (placed, not removed) */
    for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++)
        if(h_len[r][c]>=3&&v_len[r][c]>=3){grid[r][c].special=SPECIAL_BOMB;to_remove[r][c]=0;}

    /* H runs >=4 -> LINE or COLOR at pivot (not at L/T) */
    for(int r=0;r<GRID_H;r++){for(int c=0;c<GRID_W;){if(!h_len[r][c]||h_s[r][c]!=c){c++;continue;}int len=h_len[r][c];if(len>=4){int sc=c,best=sc;float bd=1e9f;for(int k=sc;k<sc+len;k++){float d=(r-pivot_r)*(float)(r-pivot_r)+(k-pivot_c)*(float)(k-pivot_c);if(d<bd){bd=d;best=k;}}if(grid[r][best].special==SPECIAL_NONE){grid[r][best].special=(len>=5)?SPECIAL_COLOR:SPECIAL_LINE;}to_remove[r][best]=0;}c+=len;}}

    /* V runs >=4 -> LINE or COLOR at pivot */
    for(int c=0;c<GRID_W;c++){for(int r=0;r<GRID_H;){if(!v_len[r][c]||v_s[r][c]!=r){r++;continue;}int len=v_len[r][c];if(len>=4){int sr=r,best=sr;float bd=1e9f;for(int k=sr;k<sr+len;k++){float d=(k-pivot_r)*(float)(k-pivot_r)+(c-pivot_c)*(float)(c-pivot_c);if(d<bd){bd=d;best=k;}}if(grid[best][c].special==SPECIAL_NONE){grid[best][c].special=(len>=5)?SPECIAL_COLOR:SPECIAL_LINE;}to_remove[best][c]=0;}r+=len;}}

    return 1;
}

/* Expand EXISTING specials that are currently in to_remove (already-placed ones being destroyed) */
static void expand_specials(void) {
    for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++){
        if(!to_remove[r][c])continue;
        if(grid[r][c].special==SPECIAL_LINE){for(int k=0;k<GRID_W;k++)to_remove[r][k]=1;for(int k=0;k<GRID_H;k++)to_remove[k][c]=1;}
        else if(grid[r][c].special==SPECIAL_BOMB){for(int dr=-2;dr<=2;dr++)for(int dc=-2;dc<=2;dc++){int nr=r+dr,nc=c+dc;if(nr>=0&&nr<GRID_H&&nc>=0&&nc<GRID_W)to_remove[nr][nc]=1;}}
        else if(grid[r][c].special==SPECIAL_COLOR){int t=grid[r][c].type;for(int rr=0;rr<GRID_H;rr++)for(int cc=0;cc<GRID_W;cc++)if(grid[rr][cc].type==t)to_remove[rr][cc]=1;}
    }
}

static void remove_matches(void){
    for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++)if(to_remove[r][c]){
        grid[r][c].type=-1;grid[r][c].special=SPECIAL_NONE;
        candycrush_score+=10;if(candycrush_score>candycrush_highscore){candycrush_highscore=candycrush_score;hub_save_score(5,candycrush_highscore);}
    }
}

static void drop_gems(void){
    for(int c=0;c<GRID_W;c++){for(int r=GRID_H-1;r>=0;r--){if(grid[r][c].type==-1){int f=0;for(int a=r-1;a>=0;a--){if(grid[a][c].type>=0){grid[r][c]=grid[a][c];grid[a][c].type=-1;grid[a][c].special=SPECIAL_NONE;grid[r][c].swap_y=-(float)((r-a)*CELL_SZ);f=1;break;}}if(!f){fill_cell(r,c);grid[r][c].swap_y=-(float)(GRID_H*CELL_SZ);}}}}
}

static void candycrush_reset(void){
    for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++)fill_cell(r,c);
    fix_grid();candycrush_score=0;candycrush_highscore=hub_load_score(5);has_selection=0;candycrush_state=0;anim_timer=0;
    swap_r1=swap_c1=swap_r2=swap_c2=pivot_r=pivot_c=-1;
}

void candycrush_init(void){pop_sound=LoadSound("assets/shared/pop.wav");pop_special_sound=LoadSound("assets/candycrush/pop_special.wav");sound_loaded=true;candycrush_reset();printf("[CANDYCRUSH] Game initialized\n");}

/* Activate color gem swap: mark all gems of target_type for removal */
static void activate_color_gem(int color_r,int color_c,int target_type){
    for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++)if(grid[r][c].type==target_type)to_remove[r][c]=1;
    to_remove[color_r][color_c]=1;
    expand_specials();
    candycrush_state=2;anim_timer=0;has_selection=0; // wait, should be candycrush_state
}

void candycrush_update(void){
    float dt=GetFrameTime();
    if(IsKeyPressed(KEY_R))candycrush_reset();

    /* Swap animation */
    if(candycrush_state==1||candycrush_state==4){
        anim_timer+=dt;float dur=0.15f,prog=anim_timer/dur;if(prog>1)prog=1;
        float fr=swap_r2-swap_r1,fc=swap_c2-swap_c1,p=(candycrush_state==1)?prog:(1-prog);
        grid[swap_r1][swap_c1].swap_x=fc*CELL_SZ*p;grid[swap_r1][swap_c1].swap_y=fr*CELL_SZ*p;
        grid[swap_r2][swap_c2].swap_x=-fc*CELL_SZ*p;grid[swap_r2][swap_c2].swap_y=-fr*CELL_SZ*p;
        if(anim_timer>=dur){
            grid[swap_r1][swap_c1].swap_x=0;grid[swap_r1][swap_c1].swap_y=0;
            grid[swap_r2][swap_c2].swap_x=0;grid[swap_r2][swap_c2].swap_y=0;
            if(candycrush_state==1){
                if(check_matches()){expand_specials();candycrush_state=2;anim_timer=0;}
                else{int tt=grid[swap_r1][swap_c1].type,ts=grid[swap_r1][swap_c1].special;grid[swap_r1][swap_c1].type=grid[swap_r2][swap_c2].type;grid[swap_r1][swap_c1].special=grid[swap_r2][swap_c2].special;grid[swap_r2][swap_c2].type=tt;grid[swap_r2][swap_c2].special=ts;candycrush_state=4;anim_timer=0;}
            }else{candycrush_state=0;}
        }
        return;
    }
    /* Pop animation */
    if(candycrush_state==2){
        anim_timer+=dt;
        for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++)if(to_remove[r][c])grid[r][c].pop_t=anim_timer;
        if(anim_timer>=0.3f){
            int has_sp=0;for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++)if(to_remove[r][c]&&grid[r][c].special)has_sp=1;
            if(sound_loaded){if(has_sp)PlaySound(pop_special_sound);else PlaySound(pop_sound);}
            remove_matches();
            drop_gems(); // Set up offsets for falling
            candycrush_state=3;anim_timer=0;
        }
        return;
    }
    /* Drop animation */
    if(candycrush_state==3){
        anim_timer+=dt;float dur=0.15f,prog=anim_timer/dur;if(prog>1)prog=1;
        bool any_moving=false;
        for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++){
            if(grid[r][c].swap_y<0){
                grid[r][c].swap_y += 800.0f * dt; // Fall speed increased
                if(grid[r][c].swap_y>0)grid[r][c].swap_y=0;
                else any_moving=true;
            }
        }
        if(!any_moving && anim_timer>=dur){
            for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++){grid[r][c].swap_x=0;grid[r][c].swap_y=0;grid[r][c].pop_t=0;}
            pivot_r=GRID_H/2;pivot_c=GRID_W/2;
            if(check_matches()){expand_specials();candycrush_state=2;anim_timer=0;}else candycrush_state=0;
        }
        return;
    }

    if(candycrush_state!=0)return;

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        Vector2 mp=GetMousePosition();
        int c=(int)(mp.x/CELL_SZ),r=(int)((mp.y-30)/CELL_SZ);
        if(r>=0&&r<GRID_H&&c>=0&&c<GRID_W&&grid[r][c].type>=0){
            if(!has_selection){sel_r=r;sel_c=c;has_selection=1;}
            else{
                int dr=abs(r-sel_r),dc=abs(c-sel_c);
                if((dr==1&&dc==0)||(dr==0&&dc==1)){
                    /* Color gem swap: activate immediately without normal match check */
                    if(grid[sel_r][sel_c].special==SPECIAL_COLOR&&grid[r][c].type>=0){
                        activate_color_gem(sel_r,sel_c,grid[r][c].type);
                    } else if(grid[r][c].special==SPECIAL_COLOR&&grid[sel_r][sel_c].type>=0){
                        activate_color_gem(r,c,grid[sel_r][sel_c].type);
                    } else {
                        /* Normal swap */
                        swap_r1=sel_r;swap_c1=sel_c;swap_r2=r;swap_c2=c;
                        pivot_r=r;pivot_c=c;
                        int tt=grid[sel_r][sel_c].type,ts=grid[sel_r][sel_c].special;
                        grid[sel_r][sel_c].type=grid[r][c].type;grid[sel_r][sel_c].special=grid[r][c].special;
                        grid[r][c].type=tt;grid[r][c].special=ts;
                        candycrush_state=1;anim_timer=0;
                    }
                }
                has_selection=0;
            }
        }else has_selection=0;
    }
}

void candycrush_draw(void){
    ClearBackground((Color){20,15,30,255});
    DrawRectangle(0,0,BW,30,(Color){0,0,0,200});
    DrawText(TextFormat(L("Pontos: %d", "Score: %d"),candycrush_score),8,6,18,WHITE);
    const char *hs=TextFormat(L("Recorde: %d", "Highscore: %d"),candycrush_highscore);
    DrawText(hs,BW-MeasureText(hs,18)-8,6,18,GRAY);

    int oy=30;
    for(int r=0;r<GRID_H;r++)for(int c=0;c<GRID_W;c++)
        if((r+c)%2)DrawRectangle(c*CELL_SZ,oy+r*CELL_SZ,CELL_SZ,CELL_SZ,(Color){30,18,42,255});

    for(int r=0;r<GRID_H;r++){for(int c=0;c<GRID_W;c++){
        Cell *ce=&grid[r][c];if(ce->type<0)continue;
        int bx=c*CELL_SZ+(int)ce->swap_x,by=oy+r*CELL_SZ+(int)ce->swap_y;
        if(candycrush_state==2&&to_remove[r][c]&&ce->pop_t>0){
            float p=ce->pop_t/0.3f;if(p>1)p=1;
            if(sinf(p*3.14159f*4)>0){int sm=(int)(CELL_SZ*(1-p*0.7f)),off=(CELL_SZ-sm)/2;DrawRectangle(bx+off,by+off,sm,sm,WHITE);}
            else draw_gem(bx+2,by+2,CELL_SZ-4,ce->type,ce->special,gem_colors[ce->type%GEM_TYPES]);
            continue;
        }
        draw_gem(bx+2,by+2,CELL_SZ-4,ce->type,ce->special,gem_colors[ce->type%GEM_TYPES]);
    }}

    if(has_selection){float pulse=0.5f+0.5f*sinf((float)GetTime()*6);Color sc={255,255,255,(unsigned char)(180+70*pulse)};DrawRectangleLinesEx((Rectangle){(float)(sel_c*CELL_SZ),(float)(oy+sel_r*CELL_SZ),(float)CELL_SZ,(float)CELL_SZ},3,sc);}
    for(int r=0;r<=GRID_H;r++)DrawLine(0,oy+r*CELL_SZ,BW,oy+r*CELL_SZ,(Color){255,255,255,20});
    for(int c=0;c<=GRID_W;c++)DrawLine(c*CELL_SZ,oy,c*CELL_SZ,oy+GRID_H*CELL_SZ,(Color){255,255,255,20});


}

void candycrush_unload(void){if(sound_loaded){UnloadSound(pop_sound);UnloadSound(pop_special_sound);sound_loaded=false;}printf("[CANDYCRUSH] Game unloaded\n");}

Game CANDYCRUSH_GAME={.name="Candy Crush",.description="Combine gemas e faca combos explosivos",.description_en="Combine gems and make explosive combos",.game_width=BW,.game_height=BH,.init=candycrush_init,.update=candycrush_update,.draw=candycrush_draw,.unload=candycrush_unload};
