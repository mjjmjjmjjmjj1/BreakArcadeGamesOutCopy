int hot_level = L_COUNT;
b32 use_light_logo = true;
int current_letter;
f32 timer = 0.05f;


f32 music_sixteenth_note_t;
int sixteenth_note_count;
#define MUSIC_TEMPO 130
#define SIXTEENTH_NOTE_TIME (60.f/(MUSIC_TEMPO*4.f))

u32 menu_text_colors[] = {0xff0080, 0xff2480, 0xff4880, 0xff6d80, 0xff9180, 0xffb680, 0xffda80, 0xffb680, 0xff9180, 0xff6d80, 0xff4880, 0xff2480};
u32 menu_dark_colors[] = {0x00257f, 0x00497f, 0x006e7f, 0x00927f, 0x00b77f, 0x00db7f, 0x00ff7f};
int menu_text_starting_color;

char *level_names[] = {"Breakout", "Powerup\\Breakout", "Pong", "Tetris", "SUPER\\NOT", "Space\\Invaders"};

f32 test;

internal void
update_menu(Input *input, f32 dt) {
    clear_screen(0x112a34);
    draw_rect((v2){0, 30}, (v2){120, 15.25f}, 0x2b454f);
    //draw_rotated_rect((v2){-arena_half_size.x-20, arena_half_size.y}, (v2){60, 40}, 45.f,  0x15a427);
    //draw_rotated_rect((v2){ arena_half_size.x+20, arena_half_size.y}, (v2){60, 40}, -45.f, 0x15a427);
    	
    {
        f32 first_p = -60.f;
        f32 range = 120.f;
        
        f32 delta_p = range / (L_COUNT-1);
        v2 p = (v2){first_p, -16};
        if (input->mouse_dp.x != 0 && input->mouse_dp.y != 0) {
            int old_hot = hot_level;
            int new_hot_level = clamp(0, (int)((player_target_p.x-first_p + delta_p*.5)/delta_p), L_COUNT-1);
            if (!save_data.levels[new_hot_level].locked) {
                hot_level = new_hot_level;
                if (old_hot != hot_level) 
				{	
					menu_level_sound = play_sound(&powerdown_sound, false);
					//set_volume(menu_level_sound, 1.0f);
				}
            } else hot_level = L_COUNT;
        }
        
        int align = TEXT_ALIGN_CENTER;
        
        for (int i = 0; i < L_COUNT; i++) {
            draw_rect(p, (v2){11, 6}, 0x2222AA);
            u32 color = 0xaaaaaa;
            f32 size = 5.0f;
            if (save_data.levels[i].locked) {
                color = 0x777777;
                //draw_text(level_names[i], p, size, &color, 1, 0, align); TODO: Text color
				draw_text(level_names[i], p, size, align);
            } else {
                color = 0xaaaaaa;
                if (i == hot_level) {
                    //draw_text(level_names[i], p, size, menu_text_colors, array_count(menu_text_colors), menu_text_starting_color, align);
					draw_text(level_names[i], p, size, align);
                } else {
                    color = 0xffffff;
                    //draw_text(level_names[i], p, size, &color, 1, 0, align);
					draw_text(level_names[i], p, size, align);
                }
                draw_number(save_data.levels[i].highscore, add_v2((v2){0, -14}, p), 3.f, color, 1);
            }
            
            p.x += delta_p;
        }
    }
    
    if (pressed(BUTTON_LMB)) {
        if (hot_level < L_COUNT && (!save_data.levels[hot_level].locked)) {
            current_level = hot_level;
            change_game_mode(GM_GAMEPLAY);
        }
    }
    
    u32 color = 0xffffff;
	//draw_bitmap(&bitmap_half_logo_light, (v2){0, 30}, (v2){15.25f, 3.f}, 1.f);
	draw_text("Break\\Arcade Games\\Out", (v2){0.0f, 38.5f}, 10.0f, TEXT_ALIGN_CENTER);
	//draw_rect((v2){0.f, 0.f}, (v2){0.01f, 100.1f}, 0x00ff00);
	/*
    if (use_light_logo) {
        
    } else {
        //draw_bitmap(&bitmap_logo_dark, (v2){0, 30}, (v2){32.25f, 15.f}, 1.f);
        //draw_bitmap(&bitmap_half_logo_dark, (v2){0, 28}, (v2){28.57f, 2.10f}, 1.f);
        //draw_text(&font_bitmaps,"BREAK", (v2){1, 42}, .64f, menu_dark_colors, array_count(menu_dark_colors), 0, TEXT_ALIGN_CENTER);
        draw_rect((v2){0, 20.75f}, (v2){11.25f, 3.1f}, 0x00ff00);
        u32 text_color = 0x00257f;
        //draw_text(&font_bitmaps,"OUT", (v2){.5f, 22.75}, .25f, &text_color, 1, 0, TEXT_ALIGN_CENTER);
    }
    
    
    {
        u32 color = 0xfffca2;
        //draw_text("CREATED BY DAN ZAIDAN", (v2){0, 13}, .2f, &color, 1, 0, TEXT_ALIGN_CENTER);
        color = 0xffda80;
        //draw_text("ON A LIVE STREAM\\WATCH THE DEVELOPMENT AT\\YOUTUBE.COM/DANZAIDAN", (v2){0, 8}, .175f, &color, 1, 0, TEXT_ALIGN_CENTER);
    }
	*/
    
}