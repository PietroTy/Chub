/**
 * @file solitairecpider.c
 * @brief Klondike Solitaire — Microsoft style, rectangle suits.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib/raylib.h"
#include "game.h"
#include "hub.h"

#define SW 640
#define SH 600
#define CARD_W 76
#define CARD_H 106
#define GAP_DOWN 18
#define GAP_UP   26
#define TOP_Y    12
#define TAB_Y    140

typedef struct { int suit; int rank; int face_up; } Card;
#define MAX_PILE 52
typedef struct { Card c[MAX_PILE]; int n; } Pile;

static Pile tab[7], found[4], stock, waste;
static Sound pop_sound;
static bool sound_loaded = false;
static int dragging, drag_src, drag_idx, drag_cnt;
static Card drag_c[MAX_PILE];
static float drag_x, drag_y, drag_off_x, drag_off_y;
static int sol_won, sol_lost;

/* pile_x: columns 0-6 for the 7 tableau (and stock/waste/foundation mapped) */
static int col_x(int i) { return 14 + i * (CARD_W + 13); }
static int is_red(int s) { return s==1||s==3; }

/* Draw pixel-art suit symbol (sz≈10) from rectangles */
static void draw_suit(int x,int y,int sz,int suit,Color col){
    int h=sz, w=sz;
    switch(suit){
    case 0: /* Spade: Pixel-art arrow shape with a stem */
        {
            /* 5-row triangle-ish top */
            int rows = 6;
            for (int i = 0; i < rows; i++) {
                int rw = (i + 1) * w / rows;
                if (rw < 1) rw = 1;
                DrawRectangle(x + (w - rw) / 2, y + i * (h * 3/4 / rows), rw, (h * 3/4 / rows) + 1, col);
            }
            /* base stem */
            DrawRectangle(x + w/2 - 1, y + h*3/4, 3, h/4, col);
            DrawRectangle(x + w/4, y + h - 2, w/2, 2, col);
        }
        break;
    case 1: /* Heart: two squares + narrowing bottom */
        DrawRectangle(x,y+h/4,w*5/12,h/2,col);
        DrawRectangle(x+w*7/12,y+h/4,w*5/12,h/2,col);
        DrawRectangle(x+w/6,y,w*2/3,h*3/5,col);
        for(int i=0;i<h*2/5;i++){int sw=w-i*2*w/(h*2/5);if(sw<1)sw=1;DrawRectangle(x+(w-sw)/2,y+h*3/5+i,sw,1,col);}
        break;
    case 2: /* Club: 3 distinct squares + stem */
        {
            int cs = w/2; /* circle size */
            DrawRectangle(x + w/2 - cs/2, y, cs, cs, col); /* top */
            DrawRectangle(x, y + h/2 - cs/2, cs, cs, col); /* left */
            DrawRectangle(x + w - cs, y + h/2 - cs/2, cs, cs, col); /* right */
            /* stem */
            DrawRectangle(x + w/2 - 1, y + h/2, 3, h/2, col);
            DrawRectangle(x + w/4, y + h - 2, w/2, 2, col);
        }
        break;
    case 3: /* Diamond: 5 rows */
        {int fw[]={1,3,5,3,1};for(int i=0;i<5;i++){int rw=fw[i]*w/5;DrawRectangle(x+(w-rw)/2,y+i*h/5,rw,h/5,col);}}
        break;
    }
}

static void draw_card(int x,int y,Card *c,int hi){
    if(!c->face_up){
        DrawRectangle(x,y,CARD_W,CARD_H,(Color){40,80,160,255});
        DrawRectangleLinesEx((Rectangle){(float)x,(float)y,(float)CARD_W,(float)CARD_H},1,(Color){60,120,220,255});
        /* cleaner card back */
        DrawRectangle(x+4,y+4,CARD_W-8,CARD_H-8,(Color){30,60,130,255});
        return;
    }
    DrawRectangle(x,y,CARD_W,CARD_H,WHITE);
    Color bc = hi?(Color){255,200,50,255}:(Color){180,180,180,255};
    DrawRectangleLinesEx((Rectangle){(float)x,(float)y,(float)CARD_W,(float)CARD_H},hi?2:1,bc);

    Color sc = is_red(c->suit)?(Color){200,30,30,255}:(Color){20,20,40,255};
    static const char *rk[]={"?","A","2","3","4","5","6","7","8","9","10","J","Q","K"};
    int sz = c->rank==10 ? 18 : 22;
    DrawText(rk[c->rank],x+4,y+2,sz,sc);
    draw_suit(x+4,y+2+sz+2,10,c->suit,sc);
    /* bottom-right */
    int rk_w = MeasureText(rk[c->rank], sz);
    DrawText(rk[c->rank],x+CARD_W-rk_w-4,y+CARD_H-sz-12,sz,sc);
    draw_suit(x+CARD_W-14,y+CARD_H-14,10,c->suit,sc);
    /* center suit */
    draw_suit(x+CARD_W/2-11,y+CARD_H/2-7,22,c->suit,sc);
}

static void shuffle(Card *d,int n){for(int i=n-1;i>0;i--){int j=GetRandomValue(0,i);Card t=d[i];d[i]=d[j];d[j]=t;}}

static void sol_reset(void){
    Card deck[52]; int idx=0;
    for(int s=0;s<4;s++) for(int r=1;r<=13;r++) deck[idx++]=(Card){s,r,0};
    shuffle(deck,52);
    for(int i=0;i<7;i++) tab[i].n=0;
    for(int i=0;i<4;i++) found[i].n=0;
    stock.n=0; waste.n=0;
    idx=0;
    for(int i=0;i<7;i++) for(int j=0;j<=i;j++){
        tab[i].c[tab[i].n]=deck[idx++];
        tab[i].c[tab[i].n].face_up=(j==i);
        tab[i].n++;
    }
    while(idx<52) stock.c[stock.n++]=deck[idx++];
    dragging=0; sol_won=0; sol_lost=0;
}

void solitairecpider_init(void){
    sol_reset();
    pop_sound = LoadSound("assets/shared/pop.wav");
    sound_loaded = true;
    printf("[SOLITAIRECPIDER] Game initialized\n");
}

/* Compute y-position of card j in tableau pile i */
static int tab_card_y(int i,int j){
    int y=TAB_Y;
    for(int k=0;k<j;k++) y+=(tab[i].c[k].face_up?GAP_UP:GAP_DOWN);
    return y;
}
static int tab_top_y(int i){return tab_card_y(i,tab[i].n>0?tab[i].n-1:0);}

/* Can card 'a' be placed on top of tableau card 'b'? */
static int can_tab(Card a,Card b){return b.face_up && is_red(a.suit)!=is_red(b.suit) && a.rank==b.rank-1;}
/* Can card 'a' go on foundation f? */
static int can_found(Card a,int f){
    if(found[f].n==0) return a.rank==1;
    Card top=found[f].c[found[f].n-1];
    return a.suit==top.suit && a.rank==top.rank+1;
}

/* Auto-move top card to best foundation */
static int auto_to_foundation(Card c,int src_pile,int src_idx){
    for(int f=0;f<4;f++){
        if(can_found(c,f)){
            found[f].c[found[f].n++]=c;
            if(src_pile==11) waste.n--;
            else if(src_pile>=0&&src_pile<7){
                tab[src_pile].n=src_idx;
                if(tab[src_pile].n>0) tab[src_pile].c[tab[src_pile].n-1].face_up=1;
            }
            return 1;
        }
    }
    return 0;
}

static int sol_has_moves(void) {
    // 1. Foundation moves (from Tableau or Waste)
    for (int f = 0; f < 4; f++) {
        if (waste.n > 0 && can_found(waste.c[waste.n-1], f)) return 1;
        for (int i = 0; i < 7; i++) {
            if (tab[i].n > 0 && can_found(tab[i].c[tab[i].n-1], f)) return 1;
        }
    }

    // 2. Tableau moves
    for (int i = 0; i < 7; i++) {
        Card *dest_top = (tab[i].n > 0) ? &tab[i].c[tab[i].n-1] : NULL;
        
        // From Waste
        if (waste.n > 0) {
            if (dest_top) { if (can_tab(waste.c[waste.n-1], *dest_top)) return 1; }
            else if (waste.c[waste.n-1].rank == 13) return 1;
        }
        
        // From other Tableau piles
        for (int src = 0; src < 7; src++) {
            if (src == i || tab[src].n == 0) continue;
            for (int j = 0; j < tab[src].n; j++) {
                if (!tab[src].c[j].face_up) continue;
                if (dest_top) { if (can_tab(tab[src].c[j], *dest_top)) return 1; }
                else if (tab[src].c[j].rank == 13 && j > 0) return 1; // King to empty only if it reveals something
                break; // Only check the bottom-most face-up card of a stack
            }
        }
    }

    // 3. Stock/Waste cycle potential
    // If there's ANY card in stock or waste that can be placed, it's not a stalemate
    for (int k = 0; k < waste.n; k++) {
        Card c = waste.c[k];
        for (int f = 0; f < 4; f++) if (can_found(c, f)) return 1;
        for (int i = 0; i < 7; i++) {
            if (tab[i].n > 0) { if (can_tab(c, tab[i].c[tab[i].n-1])) return 1; }
            else if (c.rank == 13) return 1;
        }
    }
    for (int k = 0; k < stock.n; k++) {
        Card c = stock.c[k];
        for (int f = 0; f < 4; f++) if (can_found(c, f)) return 1;
        for (int i = 0; i < 7; i++) {
            if (tab[i].n > 0) { if (can_tab(c, tab[i].c[tab[i].n-1])) return 1; }
            else if (c.rank == 13) return 1;
        }
    }

    return 0;
}

void solitairecpider_update(void){
    if(sol_won||sol_lost){if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_SPACE))sol_reset();return;}
    if(IsKeyPressed(KEY_R)) sol_reset();

    Vector2 mp=GetMousePosition();

    /* Stock click */
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&!dragging){
        Rectangle sr={(float)col_x(0),(float)TOP_Y,(float)CARD_W,(float)CARD_H};
        if(CheckCollisionPointRec(mp,sr)){
            if(stock.n>0){Card c=stock.c[--stock.n];c.face_up=1;waste.c[waste.n++]=c;}
            else{while(waste.n>0){Card c=waste.c[--waste.n];c.face_up=0;stock.c[stock.n++]=c;}}
            return;
        }
    }

    /* Double-click auto-foundation */
    static float last_click_time=0; static int last_click_pile=-99;
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&!dragging){
        float now=(float)GetTime();
        bool dbl=(now-last_click_time<0.35f);

        /* waste double-click */
        Rectangle wr={(float)col_x(1),(float)TOP_Y,(float)CARD_W,(float)CARD_H};
        if(waste.n>0&&CheckCollisionPointRec(mp,wr)){
            if(dbl&&last_click_pile==11){auto_to_foundation(waste.c[waste.n-1],11,waste.n-1);}
            last_click_time=now; last_click_pile=11;
        }
        /* tableau double-click */
        for(int i=0;i<7;i++){
            if(tab[i].n>0){
                Card top=tab[i].c[tab[i].n-1];
                if(!top.face_up) continue;
                int ty=tab_top_y(i);
                Rectangle cr={(float)col_x(i),(float)ty,(float)CARD_W,(float)CARD_H};
                if(CheckCollisionPointRec(mp,cr)){
                    if(dbl&&last_click_pile==i) auto_to_foundation(top,i,tab[i].n-1);
                    last_click_time=now; last_click_pile=i;
                }
            }
        }
    }

    /* Start drag from waste */
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&!dragging&&waste.n>0){
        Rectangle wr={(float)col_x(1),(float)TOP_Y,(float)CARD_W,(float)CARD_H};
        if(CheckCollisionPointRec(mp,wr)){
            dragging=1; drag_src=11; drag_idx=waste.n-1; drag_cnt=1;
            drag_c[0]=waste.c[waste.n-1];
            drag_off_x=mp.x-col_x(1); drag_off_y=mp.y-TOP_Y;
            drag_x=mp.x-drag_off_x; drag_y=mp.y-drag_off_y;
        }
    }

    /* Start drag from foundation */
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&!dragging){
        for(int f=0;f<4;f++){
            if(found[f].n==0) continue;
            Rectangle fr={(float)col_x(3+f),(float)TOP_Y,(float)CARD_W,(float)CARD_H};
            if(CheckCollisionPointRec(mp,fr)){
                dragging=1; drag_src=20+f; drag_idx=found[f].n-1; drag_cnt=1;
                drag_c[0]=found[f].c[found[f].n-1];
                drag_off_x=mp.x-fx; drag_off_y=mp.y-TOP_Y;
                drag_x=mp.x-drag_off_x; drag_y=mp.y-drag_off_y;
                break;
            }
        }
    }

    /* Start drag from tableau */
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&!dragging){
        for(int i=0;i<7&&!dragging;i++){
            /* find topmost face-up card that contains the click */
            for(int j=0;j<tab[i].n;j++){
                if(!tab[i].c[j].face_up) continue;
                int cy=tab_card_y(i,j);
                int bot=(j==tab[i].n-1)?cy+CARD_H:tab_card_y(i,j+1);
                Rectangle cr={(float)col_x(i),(float)cy,(float)CARD_W,(float)(bot-cy)};
                if(CheckCollisionPointRec(mp,cr)){
                    /* Grab from j upward (j is the one clicked, higher indices are on top visually) */
                    dragging=1; drag_src=i; drag_idx=j; drag_cnt=tab[i].n-j;
                    for(int k=0;k<drag_cnt;k++) drag_c[k]=tab[i].c[j+k];
                    drag_off_x=mp.x-col_x(i); drag_off_y=mp.y-cy;
                    drag_x=mp.x-drag_off_x; drag_y=mp.y-drag_off_y;
                    break;
                }
            }
        }
    }

    if(dragging){
        drag_x=mp.x-drag_off_x; drag_y=mp.y-drag_off_y;
        /* Clamp to screen with a small margin so it never fully disappears */
        if (drag_x < -CARD_W + 20) drag_x = -CARD_W + 20;
        if (drag_x > SW - 20) drag_x = SW - 20;
        if (drag_y < -CARD_H + 20) drag_y = -CARD_H + 20;
        if (drag_y > SH - 20) drag_y = SH - 20;
    }

    if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)&&dragging){
        int placed=0;
        Card first=drag_c[0];

        /* Try foundation (single card only) */
        if(drag_cnt==1){
            for(int f=0;f<4&&!placed;f++){
                Rectangle fr={(float)col_x(3+f),(float)TOP_Y,(float)CARD_W,(float)CARD_H};
                if(CheckCollisionPointRec(mp,fr)&&can_found(first,f)){
                    found[f].c[found[f].n++]=first; placed=1;
                }
            }
        }

        /* Try tableau — find nearest column by dragged card center */
        if(!placed){
            /* Use the center of the dragged card, not raw mouse cursor */
            float card_cx = drag_x + CARD_W / 2.0f;
            float card_cy = drag_y + CARD_H / 2.0f;

            /* Find the column whose center is closest horizontally */
            int best_col = -1;
            float best_dist = CARD_W * 0.9f; /* must be within ~1 card-width */
            for(int i=0;i<7;i++){
                float col_cx = col_x(i) + CARD_W / 2.0f;
                float dx = fabsf(card_cx - col_cx);
                if(dx < best_dist){ best_dist=dx; best_col=i; }
            }

            if(best_col >= 0 && card_cy >= TAB_Y - CARD_H){
                int i = best_col;
                /* Skip if dropping back on same source pile with same cards */
                if(i == drag_src && tab[i].n > 0 && tab[i].c[tab[i].n-1].rank == first.rank
                   && tab[i].c[tab[i].n-1].suit == first.suit) { /* same card = no-op */ }
                else if(tab[i].n==0){
                    if(first.rank==13){
                        for(int k=0;k<drag_cnt;k++) tab[i].c[tab[i].n++]=drag_c[k];
                        placed=1;
                    }
                } else {
                    Card top=tab[i].c[tab[i].n-1];
                    /* For the source pile, the top card visible is the one BELOW drag_idx */
                    if(i==drag_src && drag_idx>0) top=tab[i].c[drag_idx-1];
                    if(can_tab(first,top)){
                        for(int k=0;k<drag_cnt;k++) tab[i].c[tab[i].n++]=drag_c[k];
                        placed=1;
                    }
                }
            }
        }

        if(placed){
            if(drag_src==11) waste.n--;
            else if(drag_src>=20&&drag_src<24) found[drag_src-20].n--;
            else if(drag_src>=0&&drag_src<7){
                tab[drag_src].n=drag_idx;
                if(tab[drag_src].n>0) tab[drag_src].c[tab[drag_src].n-1].face_up=1;
            }
        }

        dragging=0;

        /* win check: all cards face up and stock/waste empty */
        int all_face_up = 1;
        for (int i = 0; i < 7; i++) {
            for (int j = 0; j < tab[i].n; j++) {
                if (!tab[i].c[j].face_up) { all_face_up = 0; break; }
            }
            if (!all_face_up) break;
        }
        
        if (all_face_up && stock.n == 0 && waste.n == 0) {
            sol_won = 1;
        } else {
            /* backup win check: all in foundation */
            int total = 0;
            for (int f = 0; f < 4; f++) total += found[f].n;
            if (total == 52) sol_won = 1;
        }

        if (!sol_won && !sol_has_moves()) sol_lost = 1;
    }
}

/* Draw empty slot with a subtle dark-green shadow fill */
static void draw_slot(int x,int y,Color c){
    /* Filled rectangle for the "shadow" effect */
    DrawRectangle(x, y, CARD_W, CARD_H, (Color){0, 40, 0, 100});
    DrawRectangleLinesEx((Rectangle){(float)x,(float)y,(float)CARD_W,(float)CARD_H},1,c);
}

void solitairecpider_draw(void) {
    ClearBackground((Color){0, 100, 0, 255}); /* green felt */

    /* larger checkerboard pattern for the felt background */
    int patch_size = 40;
    for (int r = 0; r < SH / patch_size; r++) {
        for (int c = 0; c < SW / patch_size; c++) {
            if ((r + c) % 2 == 0) {
                DrawRectangle(c * patch_size, r * patch_size, patch_size, patch_size, (Color){0, 92, 0, 255});
            }
        }
    }

    /* Stock */
    int sx = col_x(0);
    if (stock.n > 0) {
        DrawRectangle(sx, TOP_Y, CARD_W, CARD_H, (Color){40, 80, 160, 255});
        DrawRectangleLinesEx((Rectangle){(float)sx, (float)TOP_Y, (float)CARD_W, (float)CARD_H}, 2, (Color){80, 130, 220, 255});
        DrawText(TextFormat("%d", stock.n), sx + CARD_W / 2 - 10, TOP_Y + CARD_H / 2 - 8, 16, WHITE);
    } else {
        draw_slot(sx, TOP_Y, (Color){0, 130, 0, 200});
        DrawText("<<", sx + CARD_W / 2 - 12, TOP_Y + CARD_H / 2 - 8, 16, (Color){0, 150, 0, 200});
    }

    /* Waste */
    if (waste.n > 0) {
        /* Don't draw the top card if it's currently being dragged */
        if (!(dragging && drag_src == 11)) {
            draw_card(col_x(1), TOP_Y, &waste.c[waste.n - 1], 0);
        } else if (waste.n > 1) {
            /* Show the card underneath if there is one */
            draw_card(col_x(1), TOP_Y, &waste.c[waste.n - 2], 0);
        } else {
            draw_slot(col_x(1), TOP_Y, (Color){0, 130, 0, 150});
        }
    } else {
        draw_slot(col_x(1), TOP_Y, (Color){0, 130, 0, 150});
    }

    /* Foundation */
    static const char *suit_lbl[] = {"A", "A", "A", "A"};
    for (int f = 0; f < 4; f++) {
        int fx = col_x(3 + f);
        if (found[f].n > 0) {
            /* Don't draw the top card if it's currently being dragged */
            if (!(dragging && drag_src == 20 + f)) {
                draw_card(fx, TOP_Y, &found[f].c[found[f].n - 1], 0);
            } else if (found[f].n > 1) {
                draw_card(fx, TOP_Y, &found[f].c[found[f].n - 2], 0);
            } else {
                draw_slot(fx, TOP_Y, (Color){0, 150, 0, 180});
                Color hc = (f == 1 || f == 3) ? (Color){180, 60, 60, 200} : (Color){60, 60, 180, 200};
                draw_suit(fx + CARD_W / 2 - 10, TOP_Y + CARD_H / 2 - 10, 20, f, hc);
            }
        } else {
            draw_slot(fx, TOP_Y, (Color){0, 150, 0, 180});
            /* suit hint */
            Color hc = (f == 1 || f == 3) ? (Color){180, 60, 60, 200} : (Color){60, 60, 180, 200};
            draw_suit(fx + CARD_W / 2 - 10, TOP_Y + CARD_H / 2 - 10, 20, f, hc);
            DrawText(suit_lbl[f], fx + 4, TOP_Y + 2, 12, (Color){255, 255, 255, 80});
        }
    }

    /* Tableau */
    for (int i = 0; i < 7; i++) {
        int tx = col_x(i);
        /* Show shadow if pile is empty OR if we are dragging the bottom-most card(s) */
        int effective_n = tab[i].n;
        if (dragging && drag_src == i) effective_n = drag_idx;

        if (effective_n == 0) {
            draw_slot(tx, TAB_Y, (Color){0, 130, 0, 150});
        }
        for (int j = 0; j < effective_n; j++) {
            int cy = tab_card_y(i, j);
            draw_card(tx, cy, &tab[i].c[j], 0);
        }
    }

    /* Dragging cards */
    if (dragging) {
        for (int k = 0; k < drag_cnt; k++) {
            draw_card((int)drag_x, (int)drag_y + k * GAP_UP, &drag_c[k], 1);
        }
    }



    /* Win screen */
    if (sol_won) {
        DrawRectangle(0, 0, SW, SH, (Color){0, 0, 0, 200});
        
        int center_y = SH / 2 - 80;
        const char *wt = L("VITORIA!", "VICTORY!");
        int wt_w = MeasureText(wt, 60);
        DrawText(wt, (SW - wt_w) / 2, center_y, 60, GOLD);
        
        const char *msg = L("Todas as cartas foram reveladas!", "All cards have been revealed!");
        int msg_w = MeasureText(msg, 24);
        DrawText(msg, (SW - msg_w) / 2, center_y + 80, 24, WHITE);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);
        const char *hint = L("Pressione ENTER para Reiniciar", "Press ENTER to Restart");
        int hint_w = MeasureText(hint, 20);
        DrawText(hint, (SW - hint_w) / 2, center_y + 140, 20, (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)});
    }

    if (sol_lost) {
        DrawRectangle(0, 0, SW, SH, (Color){0, 0, 0, 180});
        int center_y = SH / 2 - 40;
        const char *wt = L("SEM MOVIMENTOS!", "NO MOVES!");
        int wt_w = MeasureText(wt, 50);
        DrawText(wt, (SW - wt_w) / 2, center_y, 50, LIGHTGRAY);
        
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);
        const char *hint = L("Pressione ENTER para Tentar de Novo", "Press ENTER to Try Again");
        int hint_w = MeasureText(hint, 20);
        DrawText(hint, (SW - hint_w) / 2, center_y + 80, 20, (Color){200, 200, 200, (unsigned char)(150 + 100 * pulse)});
    }


}

void solitairecpider_unload(void){
    if (sound_loaded) UnloadSound(pop_sound);
    printf("[SOLITAIRECPIDER] Game unloaded\n");
}

Game SOLITAIRECPIDER_GAME={
    .name="solitaire Cpider",
    .description="O classico jogo de cartas Paciencia",
    .description_en="The classic Solitaire card game",
    .game_width=SW,.game_height=SH,
    .init=solitairecpider_init,.update=solitairecpider_update,
    .draw=solitairecpider_draw,.unload=solitairecpider_unload,
};
