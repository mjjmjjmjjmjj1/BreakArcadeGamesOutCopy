
// @Hack
f32 *player_visual_p_x_ptr;


struct Playing_Sound {
    b32 active;
    
    Loaded_Sound *sound;
    f32 position; //Sample position
    
    b32 looping;
    f32 volume;
    f32 target_volume;
    f32 fading_speed;
    f32 pan;
    f32 speed_multiplier;
    
    f32 *source_x;
    
    struct Playing_Sound *synced_sound;
    struct Playing_Sound *next_free;
} typedef Playing_Sound;

Playing_Sound playing_sounds[128];
int next_playing_sound;
Playing_Sound *first_free_sound;
int immutable_sound_count;

internal void
set_volume(Playing_Sound *sound, f32 volume) {
    volume = max(0.f, volume);
    sound->target_volume = volume;
    sound->volume = volume;
}

internal Playing_Sound*
play_sound(Loaded_Sound *sound, b32 looping) {
    Playing_Sound *result = first_free_sound;
    
    if (result) {
        first_free_sound = result->next_free;
    } else {
		assert(next_playing_sound >= 0 && next_playing_sound < array_count(playing_sounds));
        result = playing_sounds + next_playing_sound++;
		
		if (next_playing_sound >= array_count(playing_sounds)) {
            next_playing_sound = immutable_sound_count;
        }
		assert(next_playing_sound >= 0 && next_playing_sound < array_count(playing_sounds));
    }
    
    *result = (Playing_Sound){0};
    result->sound = sound;
    result->position = 0;
    result->pan = 0;
    result->volume = 1;
    result->target_volume = 1;
    result->fading_speed = 1.f;
    result->looping = looping;
    result->active = true;
    result->speed_multiplier = 1.f;
    
    return result;
}

internal Playing_Sound*
play_sound_with_variation(Loaded_Sound *sound, f32 variation, f32 *source) {
    Playing_Sound *result = play_sound(sound, false);
    set_volume(result, random_f32_in_range(1.f-variation, 1.f+variation));
    result->speed_multiplier = random_f32_in_range(1.f-variation, 1.f+variation);
    result->source_x = source;
    return result;
}

internal void
stop_sound(Playing_Sound *sound) {
    sound->active = false;
    sound->next_free = first_free_sound;
    first_free_sound = sound;
}

internal void
update_game_audio(Game_Sound_Buffer *sound_buffer, f32 dt) //audio_state *AudioState, game_assets *Assets, memory_arena *TempArena) no arena or assets currently
{    
    for (Playing_Sound *sound = playing_sounds; sound != playing_sounds + array_count(playing_sounds); sound++) 
	{
        if (!sound->active) continue;
        
        assert(sound->sound);
        
        if (sound->synced_sound) 
		{
            sound->position = sound->synced_sound->position;
            sound->speed_multiplier = sound->synced_sound->speed_multiplier;
        }
    }
    
    s16 *at = sound_buffer->samples;
    for (int i = 0; i < sound_buffer->samples_to_write; i++) 
	{ //@Speed: To be more cache friendly, we should iterate over the samples inside each sound
        
        f32 left_sample = 0;
        f32 right_sample = 0;
        
        for (Playing_Sound *sound = playing_sounds; sound != playing_sounds + array_count(playing_sounds); sound++) 
		{
            if (!sound->active) continue;
            assert(sound->sound);
            if (!sound->sound) continue;
            //sound->volume = move_towards(sound->volume, sound->target_volume, sound->fading_speed/sound_buffer->samples_per_second);
            
            if (sound->volume) 
			{
                //if (sound->source_x) sound->pan = move_towards(sound->pan, (*sound->source_x - *player_visual_p_x_ptr) * .005f, 10.f/sound_buffer->samples_per_second);
                
                int sample = clamp(0, (int)sound->position, sound->sound->sample_count - sound->sound->channel_count-1);
                
                f32 frac = sound->position - (f32)sample;
                sample *= sound->sound->channel_count;
                int next_sample = sample + sound->sound->channel_count-1 + 1;
                if (next_sample >= sound->sound->sample_count*sound->sound->channel_count) {
                    if (sound->looping) next_sample -= sound->sound->sample_count*sound->sound->channel_count;
                    else next_sample = sample;
                }
                
                f32 left_sound_sample_1 = (f32)sound->sound->samples[sample];
                f32 left_sound_sample_2 = (f32)sound->sound->samples[next_sample];
                
                f32 right_sound_sample_1 = (f32)sound->sound->samples[sample      + sound->sound->channel_count-1];
                f32 right_sound_sample_2 = (f32)sound->sound->samples[next_sample + sound->sound->channel_count-1];
                
                f32 left_sound_sample = lerp(left_sound_sample_1, frac, left_sound_sample_2)*sound->volume;
                f32 right_sound_sample = lerp(right_sound_sample_1, frac, right_sound_sample_2)*sound->volume;
                
                left_sample += (left_sound_sample*clampf(0, (1.f-sound->pan), 1.f) +
                                right_sound_sample*clampf(0, (-sound->pan), 1.f));
                right_sample += (right_sound_sample*clampf(0, (1.f+sound->pan), 1.f) +
                                 left_sound_sample*clampf(0, (sound->pan), 1.f));
            }
            
            sound->position += sound->speed_multiplier;
            if (sound->position >= sound->sound->sample_count)
			{
                if (sound->looping)
				{
					sound->position -= sound->sound->sample_count;
				}
                else 
				{
					sound->active = false;
					sound->next_free = first_free_sound;
					first_free_sound = sound;
				}
            }
        }
        
        f32 min = (f32)MIN_S16;
        f32 max = (f32)MAX_S16;
        //if (left_sample*.5f < min || left_sample*.5f > max || right_sample*.5f < min || right_sample*.5f > max) print_int(0);
        *at++ = (s16)clampf(min, left_sample*.5f, max);
        *at++ = (s16)clampf(min, right_sample*.5f, max);
    }
}