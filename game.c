

/*
TODO:

redo player size: make the player size have a 'd?, fix player scale at low framerates
lock frame rate


Moar gameplay!
-Better levels (pong independent block movement, pong influence the ball speed, space invaders getting faster as time moves)
-More feel stuff (make the powerup and down animated (scale, color), screenshake, make menu/gameplay/start level transitionmake start level animation, review the size and the player wall collision (and add a sound to that))
-More Gameplay Exploration (more levels, add block life?)
Polish pass (engine, make fullscreen launch prettier, more optimizations? (floating point colors, simd, async), make an async ogg reader, save when quit, cooker, make return the player's mouse cursor to where it was before we set it?, make the audio safer (i think we got a rare case of the game changing the sound and updating the sound position uintentionally) and game)


arcade game ideas:
-tetris
-frogger
-centipade
-snake?
-tank
-donkey kong
-q*bert


Gameplay ideas:
-> life system (if the blocks goes from 5 to 4, it adds +1 for its neighbours)
            /               if the blocks goes from 4 to 3, it decreases size (to a hardcoded value))
            
Block layout

Block movement

reward player with power ups (triple shot was cool) when he gets x score
-Blocks randomly spawn more balls when killed

*/



// Assets
Bitmap bitmap_invincibility, bitmap_triple, bitmap_comet,
bitmap_strong, bitmap_tnt, bitmap_turtle, bitmap_inv,
bitmap_forcefield, bitmap_logo_light, bitmap_logo_dark, 
bitmap_half_logo_light, bitmap_half_logo_dark, test_font;

Loaded_Sound loaded_gameplay_music, loaded_menu_music, sound_sine, loaded_forcefield_sound, 
fireworks_sound, spring_sound, game_over_sound, lose_life_sound, start_game_sound, redirect_sound, ball_sound, 
powerup_sound, powerdown_sound, comet_begin, comet_loop, old_sound, interface_sound, player_wall_sound,
shatter_sound;
Loaded_Sound brick_sounds[5];
Loaded_Sound hit_sounds[16];
int next_hit_sound_to_play;
f32 last_hit_sound_played;

Playing_Sound *gameplay_music, *menu_music, *menu_level_sound;


enum {
    GM_GAMEPLAY,
    GM_MENU,
} typedef Game_Mode;

Game_Mode game_mode;
internal void change_game_mode(Game_Mode new_game_mode);

v2 player_target_p;
v2 player_target_dp;
v2 player_half_size;
int player_life;

v2 player_visual_p;
v2 player_visual_dp;
f32 player_squeeze_factor;
f32 player_squeeze_factor_d;

struct Playing_Sound *player_movement_sound, *forcefield_sound;



enum {
    L01_NORMAL,
    L02_WALL,
    
    L05_PONG,
	L03_TETRIS,
	L04_SUPERNOT,
    L06_INVADERS, //Make this the last level?

	
    L_COUNT,
} typedef Level;


struct {
    b32 locked;
    int highscore;
} typedef Level_Save;


#define SAVE_VERSION 1
struct {
    u32 version;
    Level_Save levels[L_COUNT];
} typedef Save_Data;

Save_Data save_data;

internal void
load_game() {
    String data = os_read_save_file();
    if (data.size) {
        u32 version = *(u32*)data.data;
        if (version == SAVE_VERSION) {
            save_data = *(Save_Data*)data.data;
        }
    }
}

internal void
save_game() {
    // Do that async
    String data;
    data.data = (char*)&save_data;
    data.size = sizeof(save_data);
    os_write_save_file(data);
}

#include "levels.c"
#include "menu.c"

#include "collision.c"

internal void
change_game_mode(Game_Mode new_game_mode) {
    game_mode = new_game_mode;
    switch(game_mode) {
        case GM_MENU: {
            menu_music->target_volume = 0.4f;
            gameplay_music->target_volume = 0.f;
            set_volume(forcefield_sound, 0.f);
            reset_power();
			reset_audio_pitch();
            score = 0;
            reset_balls();
			dt_multiplier = 1.0f;
            hot_level = L_COUNT;
        } break;
        
        case GM_GAMEPLAY: {
            start_game(current_level);
            menu_music->target_volume = 0.f;
            gameplay_music->target_volume = 0.2f;
        } break;
    }
}


#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

internal Bitmap
GetGlyphBitmap(char *FontFile, u32 Codepoint) //TODO: AddFontAsset version of this function would add the font bitmap to the memory arena
{
	String TTFFile = os_read_entire_file(FontFile);
	Bitmap result;
	if(TTFFile.size != 0)
	{
		stbtt_fontinfo Font;
		stbtt_InitFont(&Font, (u8 *)TTFFile.data, stbtt_GetFontOffsetForIndex((u8 *)TTFFile.data ,0));
		
		int width, height, XOffset, YOffset;
		u8 *MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 256.0f), 
										Codepoint, &width, &height, &XOffset, &YOffset);
		

		f32 scale;
		int descent;
		int baseline;
		scale = stbtt_ScaleForPixelHeight(&Font, 256);
		stbtt_GetFontVMetrics(&Font, 0, &descent, 0);
		baseline = (int) (descent*scale);
		
		result.width = width;
		result.height = height;
		result.pitch = result.width*4; //TODO: Replace 4 with defined variable called BITMAP_BYTES_PER_PIXEL
		result.memory = malloc(height*result.pitch);
		result.pixels = result.memory;
	
		u8 *Source = MonoBitmap;
		u8 *DestRow = (u8 *)result.memory + (height-1)*result.pitch;
		for(s32 Y = 0;
			Y < height;
			++Y)
		{
			u32 *Dest = (u32 *)DestRow;
			for(s32 X = 0;
				X < width;
				++X)
			{
				u8 Alpha = *Source++;
				*Dest++ = ((Alpha << 24) |
						(Alpha << 16) |
						(Alpha << 8) |
						(Alpha << 0));
			}
			
			DestRow -= result.pitch; //result.pitch; //Todo: replace with actual pitch. Should be SafeTruncateToUInt16(result.width*BITMAP_BYTES_PER_PIXEL);
		}
	
		stbtt_FreeBitmap(MonoBitmap, 0);
		
		os_free_file(TTFFile);
	
		result.AlignPercentageX = 1.0f / (r32)result.width;
		//result.AlignPercentageY = (f32)baseline / result.height;//
		
		if(Codepoint == 'g')
		{
			result.AlignPercentageY = 0.25f;
		}
		else if (Codepoint == 'j')
		{
			result.AlignPercentageY = 0.20f;
		} 
		else if (Codepoint == 'p')
		{
			result.AlignPercentageY = 0.25f;
		} 
		else if (Codepoint == 'q')
		{
			result.AlignPercentageY = 0.25f;
		} 
		else if (Codepoint == 'y')
		{
			result.AlignPercentageY = 0.30f;
		}
		else if (Codepoint == 'Q')
		{
			result.AlignPercentageY = 0.1f;
		}
		else
		{
			result.AlignPercentageY = 0.0f;
		}
		
		int test_test = 5;
	}
		
	return(result);
}


internal void
update_game(Input *input, f32 dt) {
    
	dt *= dt_multiplier;
   
    if (!initialized) {
        initialized = true;
        
        player_visual_p_x_ptr = &player_visual_p.x;
        
        player_target_p.y = -40;
        player_half_size = (v2){10, 2};
        
        arena_half_size = (v2){85, 45};
        arena_left_wall_visual_p = -arena_half_size.x;
        arena_left_wall_visual_dp = 0.f;
        arena_right_wall_visual_p = arena_half_size.x;
        arena_right_wall_visual_dp = 0.f;
        arena_top_wall_visual_p = arena_half_size.y;
        arena_top_wall_visual_dp = 0.f;
       
	   game_mode = GM_MENU;
            
        bitmap_invincibility = load_png("data\\powerup_invincibility.png");
        bitmap_triple = load_png("data\\powerup_triple.png");
        bitmap_comet = load_png("data\\powerup_comet.png");
        bitmap_inv = load_png("data\\powerdown_inverted.png");
        bitmap_tnt = load_png("data\\powerdown_instakill.png");
        bitmap_turtle = load_png("data\\powerdown_slow.png");
        bitmap_strong = load_png("data\\powerdown_strong.png");
        bitmap_forcefield = load_png("data\\force_field.png");
        
        bitmap_half_logo_light = load_png("data\\half_logo_light.png");
        bitmap_half_logo_dark  = load_png("data\\half_logo_dark.png");
		
		int bitmap_index = 0;
		int asci_index = 65;
		for(u32 Character = 'A';
			Character <= 'Z';
			++Character)
		{
			font_bitmaps[bitmap_index].bitmap = GetGlyphBitmap("data/arial.ttf", Character);
			
			asci_index++;
			bitmap_index++;
		}
		asci_index = 97;
		for(u32 Character = 'a';
			Character <= 'z';
			++Character)
			{
				font_bitmaps[bitmap_index].bitmap = GetGlyphBitmap("data/arial.ttf", Character);

				asci_index++;
				bitmap_index++;
			}

		
        loaded_forcefield_sound= load_wav("data\\sfx\\force_field.wav");
        fireworks_sound = load_wav("data\\sfx\\fireworks_1.wav");
        spring_sound = load_wav("data\\sfx\\spring.wav");
        start_game_sound = load_wav("data\\sfx\\start game.wav");
        game_over_sound = load_wav("data\\sfx\\game_over.wav");
        lose_life_sound = load_wav("data\\sfx\\lose_life.wav");
        redirect_sound = load_wav("data\\sfx\\redirect_sound.wav");
        ball_sound = load_wav("data\\sfx\\whistle.wav");
        comet_begin = load_wav("data\\sfx\\comet_begin.wav");
        comet_loop = load_wav("data\\sfx\\comet_loop.wav");
        old_sound = load_wav("data\\sfx\\old_sound.wav");
        powerup_sound = load_wav("data\\sfx\\powerup_sound.wav");
        powerdown_sound = load_wav("data\\sfx\\powerdown_sound.wav");
        interface_sound = load_wav("data\\sfx\\interface.wav");
        player_wall_sound = load_wav("data\\sfx\\player_wall.wav");
        
        brick_sounds[0] = load_wav("data\\sfx\\brick_1.wav");
        brick_sounds[1] = load_wav("data\\sfx\\brick_2.wav");
        brick_sounds[2] = load_wav("data\\sfx\\brick_3.wav");
        brick_sounds[3] = load_wav("data\\sfx\\brick_4.wav");
        brick_sounds[4] = load_wav("data\\sfx\\brick_5.wav");
        
        hit_sounds[0] = load_wav("data\\sfx\\hit_1.wav");
        hit_sounds[1] = load_wav("data\\sfx\\hit_2.wav");
        hit_sounds[2] = load_wav("data\\sfx\\hit_3.wav");
        hit_sounds[3] = load_wav("data\\sfx\\hit_4.wav");
        hit_sounds[4] = load_wav("data\\sfx\\hit_5.wav");
        hit_sounds[5] = load_wav("data\\sfx\\hit_6.wav");
        hit_sounds[6] = load_wav("data\\sfx\\hit_7.wav");
        hit_sounds[7] = load_wav("data\\sfx\\hit_8.wav");
        hit_sounds[8] = load_wav("data\\sfx\\hit_9.wav");
        hit_sounds[9] = load_wav("data\\sfx\\hit_10.wav");
        hit_sounds[10] = load_wav("data\\sfx\\hit_11.wav");
        hit_sounds[11] = load_wav("data\\sfx\\hit_12.wav");
        hit_sounds[12] = load_wav("data\\sfx\\hit_13.wav");
        hit_sounds[13] = load_wav("data\\sfx\\hit_14.wav");
        hit_sounds[14] = load_wav("data\\sfx\\hit_15.wav");
        hit_sounds[15] = load_wav("data\\sfx\\hit_16.wav");
		
		shatter_sound = load_wav("data\\sfx\\glass_smash.wav");
        
        loaded_gameplay_music = load_wav("data\\breakout_main.wav");
        loaded_menu_music = load_wav("data\\breakout_menu.wav");
        sound_sine = load_wav("data\\sfx\\sine.wav");
        gameplay_music = play_sound(&loaded_gameplay_music, true);
        set_volume(gameplay_music, 0.f);
        menu_music = play_sound(&loaded_menu_music, true);
        menu_music->synced_sound = gameplay_music;
		set_volume(menu_music, 0.0f);
        player_movement_sound = play_sound(&sound_sine, true);
        player_movement_sound->fading_speed = 1.0f;
        set_volume(player_movement_sound, 0.f);
        
        forcefield_sound = play_sound(&loaded_forcefield_sound, false);
        set_volume(forcefield_sound, 0.f);
        		
        // Ball sounds
        {
            for (int i = 0; i < 3; i++) {
                balls[i].sound = play_sound(&ball_sound, true);
                set_volume(balls[i].sound, 0.f);
                balls[i].sound->fading_speed = 10.f;
                balls[i].sound->source_x = &balls[i].p.x;
                
                
                balls[i].comet_sound = play_sound(&comet_loop, true);
                set_volume(balls[i].comet_sound, 0.f);
                balls[i].comet_sound->fading_speed = 3.f;
                balls[i].comet_sound->source_x = &balls[i].p.x;
            }
        }
        
        forcefield_sound = play_sound(&loaded_forcefield_sound, false);
        set_volume(forcefield_sound, 0.f);
        
        
        immutable_sound_count = next_playing_sound;
        
        
        
        // Load Game Save
        save_data.version = SAVE_VERSION;
        for (int i = 1; i < array_count(save_data.levels); i++) {
            save_data.levels[i].locked = true;
        }
        load_game();
    }
    
    // Player movement
    v2 player_desired_p;
    {
        f32 speed_multiplier = 1.f;
        if (slow_player_t > 0) speed_multiplier = .1f;
        
        f32 mouse_world_dp = speed_multiplier * pixels_dp_to_world(input->mouse_dp).x;
        
        if (inverted_controls_t <= 0)
            player_desired_p.x = player_target_p.x + mouse_world_dp;
        else
            player_desired_p.x = player_target_p.x - mouse_world_dp;
        
        // Player wall collision
        f32 base_player_half_size_x = 10.f; // We hardcode this to avoid the size changes to make the collision feel not solid
        f32 left_most_p = arena_left_wall_visual_p + base_player_half_size_x;
        if (player_desired_p.x < left_most_p) {
            player_squeeze_factor_d = (player_desired_p.x - left_most_p) * -1.f;
            player_desired_p.x = left_most_p;
            player_target_dp.x = 0.f;
        }
        f32 right_most_p = arena_right_wall_visual_p - base_player_half_size_x;
        if (player_desired_p.x > right_most_p) {
            player_squeeze_factor_d = (player_desired_p.x - right_most_p) * 1.f;
            player_desired_p.x = right_most_p;
            player_target_dp.x = 0.f;
        }
        
        
        player_desired_p.y = player_target_p.y;
        player_visual_p.y = player_target_p.y;
        
		if(current_level != L04_SUPERNOT)
		{
			f32 player_squeeze_factor_dd = 100.f*(-player_squeeze_factor) + 10.f*(-player_squeeze_factor_d);
			player_squeeze_factor_d += player_squeeze_factor_dd*dt;
			player_squeeze_factor += player_squeeze_factor_dd*square(dt)*.5f + player_squeeze_factor_d * dt;
		}
	
		// Spring
		v2 player_visual_ddp = {0};
		player_visual_ddp.x = 500.f*(player_desired_p.x - player_visual_p.x) + 20.f*(0 - player_visual_dp.x);
		player_visual_dp = add_v2(player_visual_dp, mul_v2(player_visual_ddp, dt));
		player_visual_p = add_v2(player_visual_p, add_v2(
									mul_v2(player_visual_dp, dt),
									mul_v2(player_visual_ddp, square(dt)*.5f)));
							
		if(current_level != L04_SUPERNOT)
		{						
			player_half_size.x = base_player_half_size_x + (dt*1.f*absf(player_target_dp.x)) - player_squeeze_factor;
			player_half_size.y = max(.5f, 2.f + (dt*-0.05f*absf(player_target_dp.x)) + player_squeeze_factor);
		}
    }
    
    if (pressed(BUTTON_ESC)) {
        if (game_mode == GM_GAMEPLAY) {
            change_game_mode(GM_MENU);
        } else {
            running = false;
            return;
        }
    }
    
    if (game_mode == GM_GAMEPLAY) {
        
		simulate_level(input, current_level, dt);
		   
        for (Block *block = blocks; block != blocks+array_count(blocks); block++) {
            if (!block->life) continue;          
            simulate_block_for_level(block, current_level, dt);
        }
		
        // Update balls	
        for (Ball *ball = balls; ball != balls + array_count(balls); ball++) {
            if (!(ball->flags & BALL_ACTIVE)) continue;
            
            f32 ball_movement_left = 1.f;
			while(ball_movement_left > 0.f){
				ball->desired_p = add_v2(ball->p, mul_v2(ball->dp, dt * ball_movement_left));
                   
            if (ball->dp.y < 0 && aabb_vs_aabb(player_visual_p, player_half_size, ball->desired_p, (v2){ball->half_size, ball->half_size})) {
                
                // Player collision with ball
				add_screenshake(10.0f);
                increase_ball_size(ball);
                touchless_bonus = 0;
                reset_and_reverse_ball_dp_y(ball);
                ball->dp.x = (ball->p.x - player_visual_p.x)*7.5f;
                ball->dp.x += clampf(-25, player_target_dp.x*.5f, 25);
                ball->desired_p.y = player_visual_p.y + player_half_size.y;
                first_ball_movement = false;
                spawn_particle_explosion(10, ball->desired_p, 8.f, 1.f, .3f, 0xffffff);
                
                if (number_of_triple_shots) {
                    number_of_triple_shots--;
                    spawn_triple_shot_balls();
                }
                play_sound_with_variation(&fireworks_sound, .5f, 0);
                if (absf(player_target_dp.x) > 1000.f) play_sound_with_variation(&player_wall_sound, .25f, 0);
				ball_movement_left = 0.f; //@Hack									 

				
            } else if (ball->desired_p.x + ball->half_size > arena_right_wall_visual_p) {
                add_screenshake(10.0f);
				increase_ball_size(ball);
                ball->desired_p.x = arena_right_wall_visual_p - ball->half_size;
                ball->dp.x = max(30, ball->dp.x);
                ball->dp.x *= -1;
                arena_right_wall_visual_dp = -30.f;
                do_wall_hit_effects(ball->desired_p, &arena_right_wall_visual_dp);
				ball_movement_left = 0.f; //@Hack
            } else if (ball->desired_p.x - ball->half_size < arena_left_wall_visual_p) {
                add_screenshake(10.0f);
				increase_ball_size(ball);
                ball->desired_p.x = arena_left_wall_visual_p + ball->half_size;
                ball->dp.x *= -1;
                ball->dp.x = max(30, ball->dp.x);
                arena_left_wall_visual_dp = 30.f;
                do_wall_hit_effects(ball->desired_p, &arena_left_wall_visual_dp);
				ball_movement_left = 0.f; //@Hack
            }
            if (ball->desired_p.y + ball->half_size > arena_top_wall_visual_p) {
                add_screenshake(10.0f);
				increase_ball_size(ball);
                ball->desired_p.y = arena_top_wall_visual_p - ball->half_size;
                ball->dp.y = max(30, ball->dp.y);
                reset_and_reverse_ball_dp_y(ball);
                process_ball_when_dp_y_down(ball);
                arena_top_wall_visual_dp = -30.f;
                do_wall_hit_effects(ball->desired_p, 0);
				ball_movement_left = 0.f; //@Hack
            }
            
            // Ball reached end of arena (Game over)
            if (ball->desired_p.y - ball->half_size < -50) {
                if (invincibility_t <= 0)
				{
					lose_life(false);
				}else {
                    ball->desired_p.y = -50 + ball->half_size;
                    reset_and_reverse_ball_dp_y(ball);
                    touchless_bonus = 0;
					add_screenshake(25.0f);
					play_sound_with_variation(&redirect_sound, 0.3f, &ball->p.x);
                }
				ball_movement_left = 0.f; //@Hack
            }
            

			
			//Ball block collision
			if (!first_ball_movement || ball_movement_left < 0.00001f) { 
				f32 min_t = ball_movement_left;
				Block *hit_block = 0;
				b32 ball_update_axis = -1; //0 for x | 1 for y | -1 for no update
				for (Block *block = blocks; block != blocks+array_count(blocks); block++) {
					if (!block->life) continue;
					
						if(!do_ball_block_collision(ball, block, &min_t, &hit_block, &ball_update_axis))
						{
							if (aabb_vs_aabb(ball->desired_p, (v2){ball->half_size, ball->half_size}, block->p, block->half_size))
							{
								// The ball is already inside the block
								min_t = 0.f;
								v2 sweep_path = sub_v2(ball->p, ball->desired_p);
								ball_update_axis = absf(sweep_path.y) > absf(sweep_path.x) ? 1 : 0;
								hit_block = block;
								break;
							}
						}

					}
					
					if(hit_block) {
						//Hit a block
						add_screenshake(25.0f);
						if (strong_blocks_t <= 0) {
							hit_block->life--;
							if (!hit_block->life) {
								block_destroyed(hit_block, ball, true);
							}
						}
						
						f32 old_size = ball->half_size;
						increase_ball_size(ball);
						if (hit_block->ball_speed_multiplier > ball->speed_multiplier) 
								ball->speed_multiplier = hit_block->ball_speed_multiplier;						
						
						v2 size_offset = {0};				
						if(ball_update_axis == 1)
						{
							if (ball->dp.y > 0) 
							{
								size_offset.y = -(ball->half_size - old_size) * 2.0f;
								if (comet_t <= 0) 
								{
									reset_and_reverse_ball_dp_y(ball);
									process_ball_when_dp_y_down(ball);
								}
							} 
							else 
							{
								if (comet_t <= 0) 
								{
									size_offset.y = (ball->half_size - old_size) * 2.0f;
									reset_and_reverse_ball_dp_y(ball);
								}
							}								
						} else if (ball_update_axis == 0) 
						{
							if(ball->dp.x > 0)
							{
								size_offset.x = -(ball->half_size - old_size) * 2.0f;
							}
							else
							{
								size_offset.x = (ball->half_size - old_size) * 2.0f;
							}
							if (comet_t <= 0) ball->dp.x *= -1;		
						} 
						else 
						{
							invalid_code_path;
						}
						
						ball->p = lerp_v2(ball->p, min_t, ball->desired_p);
						ball_movement_left = 1.f - min_t;
						ball->p = add_v2(ball->p, size_offset);

#if 0					
						//declarations
						global_variable v2 old_ball_p[16];
						global_variable f32 old_ball_half_size[16];
						global_variable int next_old_ball;
						
						old_ball_p[next_old_ball] = ball->p;
						old_ball_half_size[next_old_ball] = ball->half_size;
						next_old_ball++;
						if(next_old_ball > array_count(old_ball_p)) next_old_ball = 0;
						//render			        
						for (int i = 0; i < array_count(old_ball_p); i++)
						{
							u32 color = 0x00ff00;
							draw_transparent_rotated_rect(old_ball_p[i], (v2){old_ball_half_size[i], old_ball_half_size[i]}, 0.0f, color, 0.3f);
						}
#endif					
					} else {
						ball_movement_left = 0.f;
						ball->p = ball->desired_p;
					}
			} else {
				ball_movement_left = 0.f;
				ball->p = ball->desired_p;
			}				
			
		}
     }
        
        clear_arena_screen((v2){0, 0}, arena_left_wall_visual_p, arena_right_wall_visual_p, arena_top_wall_visual_p, ARENA_COLOR);
		
        // Force field rendering
        if (invincibility_t > 0) {
            if (invincibility_alpha < 1.f) {
                invincibility_alpha += dt*5.f;
            }
            draw_bitmap(&bitmap_forcefield, (v2){0, player_visual_p.y-7.5f}, (v2){-arena_left_wall_visual_p+arena_right_wall_visual_p, 2.5f}, invincibility_alpha);
        } else {
            if (invincibility_alpha > 0.f) {
                invincibility_alpha -= dt*5.f;
                draw_bitmap(&bitmap_forcefield, (v2){0, player_visual_p.y-7.5f}, (v2){-arena_left_wall_visual_p+arena_right_wall_visual_p, 2.5f}, invincibility_alpha);
            }
        }


        
		//draw blocks
		if (blocks->half_size.x >= 3.9f){
			for (Block *block = blocks; block != blocks+array_count(blocks); block++) {
				if (!block->life) continue;
				draw_rect_subpixel(block->p, block->half_size, block->color);
			}
		} else {
			for (Block *block = blocks; block != blocks+array_count(blocks); block++) {
				if (!block->life) continue;
				draw_rect(block->p, block->half_size, block->color);
			}
		}
        
		
		//Update power blocks
        for (Power_Block *power_block = power_blocks;
             power_block!= power_blocks+array_count(power_blocks);
             power_block++) {
            if (power_block->kind == POWER_INACTIVE) continue;
            
            power_block->p.y -= 15*dt;
			f32 anim_speed_modifier = 1.5f - absf(2.0f*(power_block->anim_t - 0.5f));
			if(power_block->anim_add_direction)
			{
				
				power_block->anim_t += dt * anim_speed_modifier;
				if(power_block->anim_t >= 0.9f){
					power_block->anim_add_direction = !power_block->anim_add_direction;
				}
				power_block->half_size.x += power_block->anim_t*dt;
				power_block->half_size.y += power_block->anim_t*dt;
				
			}else 
			{
				power_block->anim_t -= dt * anim_speed_modifier;
				if(power_block->anim_t <= 0.1f){
					power_block->anim_add_direction = !power_block->anim_add_direction;
				}
				power_block->half_size.x -= power_block->anim_t*dt;
				power_block->half_size.y -= power_block->anim_t*dt;
				
			}
            
            if (aabb_vs_aabb(player_visual_p, player_half_size, power_block->p, power_block->half_size)) {
				switch(power_block->kind) {
                    case POWER_INVINCILITY: {
                        invincibility_t += 5.f;
                    } break;
                    
                    case POWER_COMET: {
                        if (comet_t <= 0) {
                            Playing_Sound *sound = play_sound(&comet_begin, false);
                            set_volume(sound, .4f);
                        }
                        comet_t += 5.f;
                    } break;
                    
                    case POWER_TRIPLE_SHOT: {
                        number_of_triple_shots++;
                    } break;
                    
                    
                    case POWER_INSTAKILL: {
                        lose_life(false);
                    } break;
                    
                    case POWER_STRONG_BLOCKS: {
                        strong_blocks_t += 5.f;
                    } break;
                    
                    case POWER_INVERTED_CONTROLS: {
                        inverted_controls_t += 5.f;
                    } break;
                    
                    case POWER_SLOW_PLAYER: {
                        slow_player_t += 5.f;
                    } break;
                    
                    invalid_default_case;
                }
                
                if (power_block->kind <= POWERUP_LAST) play_sound(&powerup_sound, false);
                else play_sound(&powerdown_sound, false);
                
                power_block->kind = POWER_INACTIVE;
                continue;
            }
            
            switch (power_block->kind) {
                case POWER_INVINCILITY: {
                    draw_bitmap(&bitmap_invincibility, power_block->p, power_block->half_size, 1);
                } break;
                
                case POWER_TRIPLE_SHOT: {
                    draw_bitmap(&bitmap_triple, power_block->p, power_block->half_size, 1);
                } break;
                
                case POWER_COMET: {
                    draw_bitmap(&bitmap_comet, power_block->p, power_block->half_size, 1);
                } break;
                
                case POWER_INSTAKILL: {
                    draw_bitmap(&bitmap_tnt, power_block->p, power_block->half_size, 1);
                } break;
                
                case POWER_SLOW_PLAYER: {
                    draw_bitmap(&bitmap_turtle, power_block->p, power_block->half_size, 1);
                } break;
                
                case POWER_INVERTED_CONTROLS: {
                    draw_bitmap(&bitmap_inv, power_block->p, power_block->half_size, 1);
                } break;
                
                case POWER_STRONG_BLOCKS: {
                    draw_bitmap(&bitmap_strong, power_block->p, power_block->half_size, 1);
                } break;
                
                invalid_default_case;
            }
			
			// power at bottom of screen
			if(power_block->p.y < -60.f){
				power_block->kind = POWER_INACTIVE;
			}
        }
		

        
        
        // Render Particles
        for (int i = 0; i < array_count(particles); i++) {			
			Particle *p = particles + i;
            if (p->life <= 0.f) continue;
            
            p->life -= p->life_d*dt;
            p->p = add_v2(p->p, mul_v2(p->dp, dt));
            
            draw_transparent_rotated_rect(p->p, p->half_size, p->angle, p->color, p->life);
        }
        
        
        // Render balls
        for (Ball *ball = balls; ball != balls + array_count(balls); ball++) {
            if (!(ball->flags & BALL_ACTIVE)) continue;
            
			//ball whistle sound
            if (ball->sound) {
                f32 distance_factor = clampf(0.f, 1.f/(ball->p.y - player_visual_p.y)*2.f, 1.f);
                ball->sound->speed_multiplier = clampf(.6f, (70.f - (ball->p.y - player_visual_p.y))/70.f, 1.f);
                if(current_level == L04_SUPERNOT)
				{
					ball->sound->target_volume = 0.0f;
				} 
				else
				{
					if (ball->dp.y > 0)
						ball->sound->target_volume = .02f;
					else
						ball->sound->target_volume =  (absf(ball->dp.x+ball->dp.y)*.006f + distance_factor) * .2f;
				}
			}
			
            ball->trail_spawner_t -= dt;
            if (ball->trail_spawner_t <= 0.f) {
                f32 speed_t = map_into_range_normalized(2500, len_sq(ball->dp), 100000);
                ball->trail_spawner_t += (1.0f+speed_t) * 0.001f; //@Cleanup: Use speed_t
                
                u32 color = lerp_color(0x00bbee, speed_t, 0x33ffff);
                f32 life = .32f;
                if (comet_t > 0.f) {color = 0xff0000; life = 0.5f;}
                else if (ball->flags & BALL_DESTROYED_ON_DP_Y_DOWN) color = 0xffff00;
                
                f32 angle = find_look_at_rotation(ball->dp, (v2){0, 1});
                Particle *p = spawn_particle(ball->p, 2.f, (v2){ball->half_size+speed_t*2.0f, ball->half_size}, angle, life, 1.f, color);
            }
            
            if (ball->comet_sound) {
                if (comet_t > 0.f) 
                    ball->comet_sound->target_volume = .3f;
                else
                    ball->comet_sound->target_volume = 0.f;
            }
            
            draw_rect(ball->p, (v2){ball->half_size, ball->half_size}, 0xffffff);
            
            ball->half_size -= dt*max(1.f, ball->half_size);
            if (ball->half_size < .75) ball->half_size = .75;
        }
        
        // Wall movements
        {
            f32 arena_left_wall_visual_ddp = 150.f*(-arena_half_size.x - arena_left_wall_visual_p) + 7.f*(-arena_left_wall_visual_dp);
            arena_left_wall_visual_dp += arena_left_wall_visual_ddp*dt;
            arena_left_wall_visual_p += arena_left_wall_visual_ddp*square(dt)*.5f +
                arena_left_wall_visual_dp*dt;
            
            f32 arena_right_wall_visual_ddp = 150.f*(arena_half_size.x - arena_right_wall_visual_p) + 7.f*(-arena_right_wall_visual_dp);
            arena_right_wall_visual_dp += arena_right_wall_visual_ddp*dt;
            arena_right_wall_visual_p += arena_right_wall_visual_ddp*square(dt)*.5f +
                arena_right_wall_visual_dp*dt;
            
            f32 arena_top_wall_visual_ddp = 150.f*(arena_half_size.y - arena_top_wall_visual_p) + 7.f*(-arena_top_wall_visual_dp);
            arena_top_wall_visual_dp += arena_top_wall_visual_ddp*dt;
            arena_top_wall_visual_p += arena_top_wall_visual_ddp*square(dt)*.5f +
                arena_top_wall_visual_dp*dt;
            
            draw_arena_rects((v2){0, 0}, arena_left_wall_visual_p, arena_right_wall_visual_p, arena_top_wall_visual_p, WALL_COLOR);
        }
        
        if (comet_t > 0) comet_t -= dt;
        if (strong_blocks_t > 0) strong_blocks_t -= dt;
        if (inverted_controls_t > 0) inverted_controls_t -= dt;
        
        if (advance_level) start_game(advance_level_target);
        if (lose_life_at_end_of_frame)
		{
			lose_life_at_end_of_frame = false;
			
			Playing_Sound *sound = play_sound(&lose_life_sound, false);
			set_volume(sound, .35f);
			
			reset_balls();
			zero_array(power_blocks);
			
			first_ball_movement = true;
			init_ball(balls);
			
			reset_power();
		}
		
        for (int i = 0; i < player_life; i++) {
            draw_rect((v2){-arena_half_size.x+1.25f+i*3.f, arena_half_size.y+2.5f},
                      (v2){1.25f, 1.25f}, 0xcccccc);
            draw_rect((v2){-arena_half_size.x+1.25f+i*3.f, arena_half_size.y+2.5f},
                      (v2){1, 1}, 0x00cccc);
        }
        
        draw_number(score, (v2){-arena_half_size.x+25.f, arena_half_size.y+2.5f}, 2.5f, 0xffffff, 6);
        
        // Draw HUD
        {
            v2 p = {-arena_half_size.x+4.f, -arena_half_size.y};
            if (invincibility_t > 0) {
                draw_bitmap(&bitmap_invincibility, p, (v2){2, 2}, 1.f);
                draw_f32(invincibility_t, add_v2(p, (v2){10, 0}), 2.f, 0xffffff);
                p.y += 5.f;
            }
            
            if (number_of_triple_shots > 0) {
                draw_bitmap(&bitmap_triple, p, (v2){2, 2}, 1.f);
                draw_number(number_of_triple_shots, add_v2(p, (v2){5, 0}), 2.f, 0xffffff, 1);
                p.y += 5.f;
            }
            
            if (comet_t > 0) {
                draw_bitmap(&bitmap_comet, p, (v2){2, 2}, 1.f);
                draw_f32(comet_t, add_v2(p, (v2){10, 0}), 2.f, 0xffffff);
                p.y += 5.f;
            }
            
            if (strong_blocks_t > 0) {
                draw_bitmap(&bitmap_strong, p, (v2){2, 2}, 1.f);
                draw_f32(strong_blocks_t, add_v2(p, (v2){10, 0}), 2.f, 0xffffff);
                p.y += 5.f;
            }
            
            if (inverted_controls_t > 0) {
                draw_bitmap(&bitmap_inv, p, (v2){2, 2}, 1.f);
                draw_f32(inverted_controls_t, add_v2(p, (v2){10, 0}), 2.f, 0xffffff);
                p.y += 5.f;
            }
            
            if (slow_player_t > 0) {
                draw_bitmap(&bitmap_turtle, p, (v2){2, 2}, 1.f);
                draw_f32(slow_player_t, add_v2(p, (v2){10, 0}), 2.f, 0xffffff);
                p.y += 5.f;
            }
            
        }
        
		post_simulate_level(current_level, dt);
		
    } else {
        update_menu(input, dt);
    }
    
    // This is here to make sure the timer is incremented even if not on the menu, but this does menu stuff @Cleanup
    {
        music_sixteenth_note_t += dt;
        if (music_sixteenth_note_t >= SIXTEENTH_NOTE_TIME) {
            music_sixteenth_note_t -= SIXTEENTH_NOTE_TIME;
            menu_text_starting_color++;
            if (menu_text_starting_color >= array_count(menu_text_colors)) menu_text_starting_color = 0;
            sixteenth_note_count++;
            if (sixteenth_note_count == 3)
                use_light_logo = !use_light_logo;
            else if (sixteenth_note_count == 4) {
                use_light_logo = !use_light_logo;
                sixteenth_note_count = 0;
            }
        }
        
    }
    
    // Player renderer
    {
		f32 diff = player_desired_p.x - player_visual_p.x;
        player_target_dp.x = (diff) / dt;
        player_target_p = player_desired_p;
        if (invincibility_t > 0) {
            invincibility_t -= dt;
            draw_rect_subpixel(player_visual_p, player_half_size, 0xffffff);
        } else {
            draw_rect_subpixel(player_visual_p, player_half_size, 0x00ff00);
        }
        //draw_rect_subpixel(player_target_p, (v2){2, 2}, 0x0000ff);
        
		//get average volume. tried to get rid of popping. was unsuccessful
		f32 average_volume_ = 0; 
        
		//Update Camera
		{
			v2 ddp = add_v2(mul_v2(sub_v2((v2){0, 0}, cam_p), 1000.0f),
						   mul_v2(sub_v2((v2){0, 0}, cam_dp), 15.0f));
			cam_dp = add_v2(mul_v2(ddp, dt), cam_dp);
			cam_p = add_v2(add_v2(mul_v2(ddp, square(dt) * 0.5f), 
								mul_v2(cam_dp, dt)), cam_p);

		}
		if(current_level != L04_SUPERNOT)
		{
			//player movement audio
			player_movement_sound->target_volume = clampf(0.f, ((absf(player_target_dp.x)*0.00025f)), 2.0f)*.3f;
			f32 target = clampf(.1f, .1f+((absf(player_target_dp.x * dt)*0.03f)), 3.0f);
			player_movement_sound->speed_multiplier = move_towards(player_movement_sound->speed_multiplier, target, dt*(5.f + target));
		}
        set_volume(forcefield_sound, 0); //TODO: replace with sound when ball hits forcefield
    }
    
    
#if DEVELOPMENT
    if (pressed(BUTTON_UP)) comet_t += 100;//invincibility_t += dt*1000.f;
    //if (is_down(BUTTON_DOWN)) number_of_triple_shots += 10;
    if (pressed(BUTTON_RIGHT)) {blocks_destroyed = num_blocks-1; test_for_win_condition();}
	draw_f32(dt * 1000.0f, (v2){85, 48}, 2.5f, 0xff0000);
#endif
    
    draw_messages(dt);
    
}