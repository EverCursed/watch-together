#ifndef UI_H
#define UI_H

#include "watchtogether_simd.h"
#include <SDL2/SDL_ttf.h>

#define MAX_TEXT_BLOCK_TEXT_LENGTH  64
#define MAX_BUTTON_TEXT_LENGTH      64
#define MAX_SCREEN_BUTTON_COUNT     16
#define MAX_SCREEN_TEXT_BLOCK_COUNT 32
#define MAX_SCREEN_STACK_HEIGHT     8
#define MAX_SCREEN_CREATION_COUNT   64

#define TTF_FONT_TYPE "fonts/OpenSans-Regular.ttf"
#define TTF_FONT_SIZE 64

#define BUTTON_TEXT_HEIGHT_FRACTION 0.75f

typedef i32 button_id;
typedef void (*fptr)(void *);

typedef struct MenuButtons {
    i32 nbuttons;
    
    v4x4 dimensions[MAX_SCREEN_BUTTON_COUNT/4];
    
    v4 color[MAX_SCREEN_BUTTON_COUNT];
    //char text[MAX_BUTTON_TEXT_LENGTH];
    SDL_Texture *rendered_text[MAX_SCREEN_BUTTON_COUNT];
    void (*action[MAX_SCREEN_BUTTON_COUNT])(void *);
    
    b32 mouse_hovered[MAX_SCREEN_BUTTON_COUNT];
    b32 mouse_clicked[MAX_SCREEN_BUTTON_COUNT];
    b32 button_disabled[MAX_SCREEN_BUTTON_COUNT];
} MenuButtons;

typedef struct MenuTexts {
    i32 ntexts;
    
    v4x4 dimensions[MAX_SCREEN_TEXT_BLOCK_COUNT/4];
    
    v4 s[MAX_SCREEN_TEXT_BLOCK_COUNT/4];
    
    v4 text_height[MAX_SCREEN_TEXT_BLOCK_COUNT/4];
    v4 text_width[MAX_SCREEN_TEXT_BLOCK_COUNT/4];
    
    SDL_Texture *rendered_text[MAX_SCREEN_TEXT_BLOCK_COUNT];
    //char text[MAX_TEXT_BLOCK_TEXT_LENGTH];
} MenuTexts;

typedef struct Menu Menu;

typedef struct MenuScreen {
    Menu       *parent;
    
    MenuButtons *buttons;
    MenuTexts   *text_blocks;
} MenuScreen;

typedef struct Menu {
    i32 nstack;
    i32 nscreens;
    
    MenuScreen *screen_stack[MAX_SCREEN_STACK_HEIGHT];
    MenuScreen *screen_container[MAX_SCREEN_CREATION_COUNT];
    
    MenuScreen *main_menu_screen;
    MenuScreen *options_screen;
    MenuScreen *debug_screen;
    
    TTF_Font *font;
    
    i32 rects_count;
    i32 text_count;
    SDL_Rect *rects;
    SDL_Texture **textures;
    SDL_Rect *text_rects;
} Menu;

internal void RenderMenu(Menu *m, i32 output_width, i32 output_height);
internal void DestroyMenuMenu(Menu *m);
internal Menu* CreateMenuMenu();
internal inline i32 MenuScreenItems(MenuScreen *s);
internal MenuScreen* GetTopmostMenuScreen(Menu *m);
internal void DestroyMenuScreen(MenuScreen *s);
internal MenuScreen* CreateMenuScreen(Menu *m);
internal void AddMenuScreenToMenu(Menu *m, MenuScreen *s);
internal i32 CreateMenuButton(MenuScreen *s,
                              f32 x, f32 y,
                              f32 width, f32 height,
                              char *text,
                              void (*function)(void *));
internal void CreateMenuTextBox(MenuScreen *s, f32 x, f32 y, f32 w, f32 h, f32 scale, char *text);
internal void PushMenuScreen(Menu *m, MenuScreen *s);
internal MenuScreen* PopMenuScreen(Menu *m);

internal fptr
MenuGetClickedButtonAction(Menu *m, i32 x, i32 y, i32 screen_x, i32 screen_y);


#endif //UI_H
