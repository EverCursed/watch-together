#include "ui.h"
#include "watchtogether_simd.h"
#include "common/custom_malloc.h"
#include "defines.h"
#include <SDL2/SDL.h>
#include <assert.h>

/*
bg_color =               { 0, 0, 0, 128 }; 
button_color =           { 220, 220, 220, 128};
button_mouseover_color = { 247, 247, 247, 128 };
button_pressed_color =   { 200, 200, 200, 128 };
text_color =             { 255, 255, 255, 0 };
*/
u32 bg_color                = 0x80000000;
//u32 button_color            = 0x80DCDCDC;
u32 button_mouseover_color  = 0x80F7F7F7;
u32 button_pressed_color    = 0x80C8C8C8;
//u32 text_color              = 0x00FFFFFF;

v4 button_color = { .x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f };
SDL_Color text_color = { 255, 255, 255, 255 };

//////////////////////// TEXT BOXES //////////////////////////////

internal void
CreateMenuTextBox(MenuScreen *s, r32 x, r32 y, r32 w, r32 h, r32 scale, char *text)
{
    assert(s->text_blocks->ntexts < MAX_SCREEN_TEXT_BLOCK_COUNT);
    
    int ntext = s->text_blocks->ntexts++;
    
    int vi = ntext/4;
    int ve = ntext%4;
    
    s->text_blocks->x[vi].E[ve] = x;
    s->text_blocks->y[vi].E[ve] = y;
    s->text_blocks->w[vi].E[ve] = w;
    s->text_blocks->h[vi].E[ve] = h;
    
    s->text_blocks->s[vi].E[ve] = scale;
    
    assert(text != NULL);
    
    SDL_Surface *temp_surface = TTF_RenderText_Blended(s->parent->font, text, text_color);
    s->text_blocks->rendered_text[ntext] = PlatformConvertSurfaceToTexture(temp_surface);
    SDL_FreeSurface(temp_surface);
    
    dbg_success("New Texture: %p\n", s->text_blocks->rendered_text[ntext]);
    
    int w_texture, h_texture;
    SDL_QueryTexture(s->text_blocks->rendered_text[ntext],
                     NULL, NULL, &w_texture, &h_texture);
    s->text_blocks->text_width[vi].E[ve] = (float)w_texture;
    s->text_blocks->text_height[vi].E[ve] = (float)h_texture;
}

///////////////////////// BUTTONS ////////////////////////////////

internal i32
CreateMenuButton(MenuScreen *s,
                 r32 x, r32 y,
                 r32 width, r32 height,
                 char *text,
                 void (*function)(void *))
{
    int buttons = s->buttons->nbuttons++;
    int vi = buttons/4;
    int ve = buttons%4;
    
    assert(buttons < MAX_SCREEN_BUTTON_COUNT);
    
    s->buttons->x[vi].E[ve] = x;
    s->buttons->y[vi].E[ve] = y;
    s->buttons->w[vi].E[ve] = width;
    s->buttons->h[vi].E[ve] = height;
    
    s->buttons->color[buttons] = button_color;
    s->buttons->action[buttons] = function;
    
    if(text) CreateMenuTextBox(s, x, y, width, height, BUTTON_TEXT_HEIGHT_FRACTION, text);
    
    return buttons;
}

internal void
DestroyMenuButton(MenuScreen *s, int button_index)
{
    int button_count = s->buttons->nbuttons--;
    for(int i = button_index; i < button_count-1; i++)
    {
        // TODO(Val): maybe replace this with some kind of wide shift?
        
        int vi = i/4;
        int ve = i%4;
        int vi2 = (i+1)/4;
        int ve2 = (i+1)%4;
        
        s->buttons->x[vi].E[ve] = s->buttons->x[vi2].E[ve2];
        s->buttons->y[vi].E[ve] = s->buttons->y[vi2].E[ve2];
        s->buttons->w[vi].E[ve] = s->buttons->w[vi2].E[ve2];
        s->buttons->h[vi].E[ve] = s->buttons->h[vi2].E[ve2];
        
        s->buttons->color[i] = s->buttons->color[i+1];
        s->buttons->action[i] = s->buttons->action[i+1];
        s->buttons->rendered_text[i] = s->buttons->rendered_text[i+1];
        
        s->buttons->mouse_hovered[i] = s->buttons->mouse_hovered[i+1];
        s->buttons->mouse_clicked[i] = s->buttons->mouse_clicked[i+1];
    }
}

///////////////////////// SCREENS ////////////////////////////////

internal void
AddMenuScreenToMenu(Menu *m, MenuScreen *s)
{
    // TODO(Val): Maybe make this return an error code if full?
    
    if(m->nscreens < MAX_SCREEN_CREATION_COUNT)
    {
        m->screen_container[m->nscreens] = s;
        s->parent = m;
    }
}

internal MenuScreen *
CreateMenuScreen(Menu *m)
{
    MenuScreen *s = custom_malloc(sizeof(MenuScreen));
    
    s->parent = m;
    s->buttons = custom_malloc(sizeof(MenuButtons));
    s->text_blocks = custom_malloc(sizeof(MenuTexts));
    
    s->buttons->nbuttons = 0;
    s->text_blocks->ntexts = 0;
    
    if(m) AddMenuScreenToMenu(m, s);
    
    return s;
}

internal void
DestroyMenuScreen(MenuScreen *s)
{
    // NOTE(Val): Do not use this function for now. This will only be used when destroying full menu.
    
    // TODO(Val): This should be done somewhat safer
    
    custom_free(s->buttons);
    custom_free(s->text_blocks);
    custom_free(s);
}

internal MenuScreen *
GetTopmostMenuScreen(Menu *m)
{
    if(m->nstack > 0)
        return m->screen_stack[m->nstack - 1];
    else
        return NULL;
}

internal void PushMenuScreen(Menu *m, MenuScreen *s)
{
    // TODO(Val): check if screen belongs to this menu
    
    if(m->nstack < MAX_SCREEN_STACK_HEIGHT)
    {
        m->screen_stack[m->nstack] = s;
        m->nstack++;
    }
}

internal MenuScreen* PopMenuScreen(Menu *m)
{
    if(m->nstack > 0)
        m->nstack--;
}

internal inline i32
MenuScreenItems(MenuScreen *s)
{
    return (s->buttons->nbuttons + s->text_blocks->ntexts);
}

/////////////////////////// MENU //////////////////////////////////

internal Menu *
CreateMenuMenu()
{
    Menu *m = custom_malloc(sizeof(Menu));
    
    m->nstack = 0;
    m->nscreens = 0;
    
    m->font = TTF_OpenFont(TTF_FONT_TYPE, TTF_FONT_SIZE);
    if(m->font)
    {
        wlog(LOG_INFO, "Loaded fonts successfully");
        return m;
    }
    else
    {
        wlog(LOG_FATAL, "Failed to load font");
        return NULL;
    }
    
    return m;
}

internal void
DestroyMenuMenu(Menu *m)
{
    for(int i = 0; i < m->nscreens; i++)
        DestroyMenuScreen(m->screen_container[i]);
    
    custom_free(m);
}

///////////////////////// RENDERING ///////////////////////////////

internal void
RenderMenu(Menu *m, i32 output_width, i32 output_height)
{
    MenuScreen *s = GetTopmostMenuScreen(m);
    if(s)
    {
        int nbuttons = s->buttons->nbuttons;
        int ntexts = s->text_blocks->ntexts;
        int rounded_ntexts = (ntexts+3)/4*4;
        int rounded_nbuttons = (nbuttons+3)/4*4;
        
        // NOTE(Val): Preparing for render
        m->rects = custom_malloc(sizeof(SDL_Rect)*rounded_nbuttons);
        m->textures = custom_malloc(sizeof(SDL_Texture *) * rounded_ntexts);
        m->rects_count = nbuttons;
        m->text_count = ntexts;
        m->text_rects = custom_malloc(sizeof(SDL_Rect)*rounded_ntexts);
        
        v4 screen_x = v4_set1_ps((float)output_width);
        v4 screen_y = v4_set1_ps((float)output_height);
        
        for(int i = 0; i < (nbuttons+3)/4; i+=1)
        {
            v4 x_fin, y_fin, w_fin, h_fin;
            
            x_fin = v4_mul_ps(s->buttons->x[i], screen_x);
            y_fin = v4_mul_ps(s->buttons->y[i], screen_y);
            w_fin = v4_mul_ps(s->buttons->w[i], screen_x);
            h_fin = v4_mul_ps(s->buttons->h[i], screen_y);
            
            x_fin = v4_cvtps_epi32(x_fin);
            y_fin = v4_cvtps_epi32(y_fin);
            w_fin = v4_cvtps_epi32(w_fin);
            h_fin = v4_cvtps_epi32(h_fin);
            
            for(int j = 0; j < 4; j++)
            {
                m->rects[i*4 + j].x = x_fin.s.E[j];
                m->rects[i*4 + j].y = y_fin.s.E[j];
                m->rects[i*4 + j].w = w_fin.s.E[j];
                m->rects[i*4 + j].h = h_fin.s.E[j];
            };
        }
        
        v4 div2 = v4_set1_ps(0.5f);
        
        for(int i = 0; i < (ntexts+3)/4; i++)
        {
            v4 x_bounding, y_bounding, w_bounding, h_bounding;
            v4 x_fin, y_fin, w_fin, h_fin;
            
            x_bounding = v4_mul_ps(s->text_blocks->x[i], screen_x);
            y_bounding = v4_mul_ps(s->text_blocks->y[i], screen_y);
            w_bounding = v4_mul_ps(s->text_blocks->w[i], screen_x);
            h_bounding = v4_mul_ps(s->text_blocks->h[i], screen_y);
            
            h_fin = v4_mul_ps(h_bounding, s->text_blocks->s[i]);
            w_fin = v4_mul_ps(v4_div_ps(h_fin, s->text_blocks->text_height[i]), s->text_blocks->text_width[i]);
            
            x_fin = v4_add_ps(x_bounding, v4_mul_ps(v4_sub_ps(w_bounding, w_fin), div2));
            y_fin = v4_add_ps(y_bounding, v4_mul_ps(v4_sub_ps(h_bounding, h_fin), div2));
            
            x_fin = v4_cvtps_epi32(x_fin);
            y_fin = v4_cvtps_epi32(y_fin);
            w_fin = v4_cvtps_epi32(w_fin);
            h_fin = v4_cvtps_epi32(h_fin);
            
            for(int j = 0; j < 4; j++)
            {
                m->text_rects[i*4 + j].x = x_fin.s.E[j];
                m->text_rects[i*4 + j].y = y_fin.s.E[j];
                m->text_rects[i*4 + j].w = w_fin.s.E[j];
                m->text_rects[i*4 + j].h = h_fin.s.E[j];
            };
        }
        
        for(int j = 0; j < ntexts; j++)
        {
            m->textures[j] = s->text_blocks->rendered_text[j];
        }
    }
}

///////////////////////// MOUSE INPUT /////////////////////////////

static fptr
MenuGetClickedButton(Menu *m, i32 x, i32 y, i32 screen_x, i32 screen_y)
{
    v4 mouse_x = v4_set1_ps((float)x/(float)screen_x);
    v4 mouse_y = v4_set1_ps((float)y/(float)screen_y);
    
    MenuScreen *s = GetTopmostMenuScreen(m);
    if(!s) return NULL;
    
    MenuButtons *b = s->buttons;
    
    int nbuttons = (b->nbuttons+3)/4;
    for(int i = 0; i < nbuttons; i++)
    {
        v4 x_right = v4_add_ps(b->x[i], b->w[i]);
        v4 y_bottom = v4_add_ps(b->y[i], b->h[i]);
        
        v4 x_res = v4_cmpge_ps(mouse_x, b->x[i]);
        v4 y_res = v4_cmpge_ps(mouse_y, b->y[i]);
        
        v4 x_res2 = v4_cmple_ps(mouse_x, x_right);
        v4 y_res2 = v4_cmple_ps(mouse_y, y_bottom);
        
        v4 res_x = v4_and_ps(x_res, x_res2);
        v4 res_y = v4_and_ps(y_res, y_res2);
        
        v4 res = v4_and_ps(res_x, res_y);
        
        for(int j = 0; j < 4; j++)
        {
            if(res.E[j])
            {
                return b->action[i*4 + j];
            }
        }
    }
    
    return NULL;
}
