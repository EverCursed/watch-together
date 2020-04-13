#include "ui.h"
#include "watchtogether_simd.h"
#include "common/custom_malloc.h"
#include "defines.h"
#include <SDL2/SDL.h>

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

v4 button_color = { .F32 = { 1.0f, 1.0f, 1.0f, 1.0f } };
SDL_Color text_color = { 255, 255, 255, 255 };

//////////////////////// TEXT BOXES //////////////////////////////

internal void
CreateMenuTextBox(MenuScreen *s, f32 x, f32 y, f32 w, f32 h, f32 scale, char *text)
{
    assert(s->text_blocks->ntexts < MAX_SCREEN_TEXT_BLOCK_COUNT);
    
    int ntext = s->text_blocks->ntexts++;
    
    int vi = ntext/4;
    int ve = ntext%4;
    
    s->text_blocks->dimensions[vi].x.F32[ve] = x; 
    s->text_blocks->dimensions[vi].y.F32[ve] = y; 
    s->text_blocks->dimensions[vi].z.F32[ve] = w; 
    s->text_blocks->dimensions[vi].w.F32[ve] = h; 
    
    s->text_blocks->s[vi].F32[ve] = scale;
    
    assert(text != NULL);
    
    SDL_Surface *temp_surface = TTF_RenderText_Blended(s->parent->font, text, text_color);
    s->text_blocks->rendered_text[ntext] = PlatformConvertSurfaceToTexture(temp_surface);
    SDL_FreeSurface(temp_surface);
    
    dbg_success("New Texture: %p\n", s->text_blocks->rendered_text[ntext]);
    
    int w_texture, h_texture;
    SDL_QueryTexture(s->text_blocks->rendered_text[ntext],
                     NULL, NULL, &w_texture, &h_texture);
    s->text_blocks->text_width[vi].F32[ve] = (float)w_texture;
    s->text_blocks->text_height[vi].F32[ve] = (float)h_texture;
}

///////////////////////// BUTTONS ////////////////////////////////

internal i32
CreateMenuButton(MenuScreen *s,
                 f32 x, f32 y,
                 f32 w, f32 h,
                 char *text,
                 void (*function)(void *))
{
    int buttons = s->buttons->nbuttons++;
    int vi = buttons/4;
    int ve = buttons%4;
    
    assert(buttons < MAX_SCREEN_BUTTON_COUNT);
    
    s->buttons->dimensions[vi].x.F32[ve] = x; 
    s->buttons->dimensions[vi].y.F32[ve] = y; 
    s->buttons->dimensions[vi].z.F32[ve] = w; 
    s->buttons->dimensions[vi].w.F32[ve] = h; 
    
    s->buttons->color[buttons] = button_color;
    s->buttons->action[buttons] = function;
    
    if(text) CreateMenuTextBox(s, x, y, w, h, BUTTON_TEXT_HEIGHT_FRACTION, text);
    
    return buttons;
}

// TODO(Val): Allow menu modules to be dynamically removed?

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
        
        v4 screen_x = v4_set1_ps((f32)output_width);
        v4 screen_y = v4_set1_ps((f32)output_height);
        
        
        
        for(int i = 0; i < (nbuttons+3)/4; i+=1)
        {
            v4 x_fin, y_fin, w_fin, h_fin;
            
            x_fin = v4_cvtps_epi32(v4_mul_ps(s->buttons->dimensions[i].x, screen_x));
            y_fin = v4_cvtps_epi32(v4_mul_ps(s->buttons->dimensions[i].y, screen_y));
            w_fin = v4_cvtps_epi32(v4_mul_ps(s->buttons->dimensions[i].z, screen_x));
            h_fin = v4_cvtps_epi32(v4_mul_ps(s->buttons->dimensions[i].w, screen_y));
            
            
            for(int j = 0; j < 4; j++)
            {
                m->rects[i*4 + j].x = x_fin.I32[j];
                m->rects[i*4 + j].y = y_fin.I32[j];
                m->rects[i*4 + j].w = w_fin.I32[j];
                m->rects[i*4 + j].h = h_fin.I32[j];
                
                //dbg_error("rect = { %d, %d, %d, %d }\n",
                //m->text_rects[i*4 + j].x,
                //m->text_rects[i*4 + j].y,
                //m->text_rects[i*4 + j].w,
                //m->text_rects[i*4 + j].h);
            };
        }
        
        v4 div2 = v4_set1_ps(0.5f);
        
        for(int i = 0; i < (ntexts+3)/4; i++)
        {
            v4 x_bounding, y_bounding, w_bounding, h_bounding;
            v4 x_fin, y_fin, w_fin, h_fin;
            
            x_bounding = v4_mul_ps(s->text_blocks->dimensions[i].x, screen_x);
            y_bounding = v4_mul_ps(s->text_blocks->dimensions[i].y, screen_y);
            w_bounding = v4_mul_ps(s->text_blocks->dimensions[i].z, screen_x);
            h_bounding = v4_mul_ps(s->text_blocks->dimensions[i].w, screen_y);
            
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
                m->text_rects[i*4 + j].x = x_fin.I32[j];
                m->text_rects[i*4 + j].y = y_fin.I32[j];
                m->text_rects[i*4 + j].w = w_fin.I32[j];
                m->text_rects[i*4 + j].h = h_fin.I32[j];
            };
        }
        
        for(int j = 0; j < ntexts; j++)
        {
            m->textures[j] = s->text_blocks->rendered_text[j];
        }
    }
}

///////////////////////// MOUSE INPUT /////////////////////////////

static func
MenuGetClickedButtonAction(Menu *m, i32 x, i32 y, i32 screen_x, i32 screen_y)
{
    v4 mouse_x = v4_set1_ps((float)x/(float)screen_x);
    v4 mouse_y = v4_set1_ps((float)y/(float)screen_y);
    
    MenuScreen *s = GetTopmostMenuScreen(m);
    if(!s) return NULL;
    
    MenuButtons *b = s->buttons;
    
    int nbuttons = (b->nbuttons+3)/4;
    for(int i = 0; i < nbuttons; i++)
    {
        v4 x_right = v4_add_ps(b->dimensions[i].x, b->dimensions[i].z);
        v4 y_bottom = v4_add_ps(b->dimensions[i].y, b->dimensions[i].w);
        
        v4 x_res = v4_cmpge_ps(mouse_x, b->dimensions[i].x);
        v4 y_res = v4_cmpge_ps(mouse_y, b->dimensions[i].y);
        
        v4 x_res2 = v4_cmple_ps(mouse_x, x_right);
        v4 y_res2 = v4_cmple_ps(mouse_y, y_bottom);
        
        v4 res_x = v4_and_ps(x_res, x_res2);
        v4 res_y = v4_and_ps(y_res, y_res2);
        
        v4 res = v4_and_ps(res_x, res_y);
        
        for(int j = 0; j < 4; j++)
        {
            if(res.I32[j])
            {
                return b->action[i*4 + j];
            }
        }
    }
    
    return NULL;
}
