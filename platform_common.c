typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef size_t memory_index;
    
typedef float real32;
typedef double real64;
    
typedef int8 s8;
typedef int8 s08;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;
typedef bool32 b32;

typedef uint8 u8;
typedef uint8 u08;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef real32 r32;
typedef real64 r64;

/////////
// Input

struct {
    b32 is_down;
    b32 changed;
} typedef Button;

enum {
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_UP,
    BUTTON_DOWN,
    
    BUTTON_LMB,
    BUTTON_ESC,
    
    BUTTON_COUNT,
};

struct {
    v2i mouse_dp;
    
    Button buttons[BUTTON_COUNT];
    
} typedef Input;

#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)
#define is_down(b) (input->buttons[b].is_down)



////////////
// Audio

struct {
    int size; //buffer size in bytes
    int channel_count;
    int samples_per_second;
    int bytes_per_sample;
    int running_sample_index;
    
    s16 *samples;
    int samples_to_write;
} typedef Game_Sound_Buffer;


//////////
// Rendering

inline uint16
SafeTruncateToUInt16(int32 Value)
{
    // TODO(casey): Defines for maximum values
    assert(Value <= 65535);
    assert(Value >= 0);
    uint16 Result = (uint16)Value;
    return(Result);
}


struct {
    int width, height;
    u32 *pixels;
} typedef Render_Buffer;


/////////
// Platform services to the game

internal String os_read_entire_file(char *file_path);
internal void os_free_file(String s);
internal String os_read_save_file();
internal b32 os_write_save_file(String data);

#define OS_JOB_CALLBACK(name) void name(struct Os_Job_Queue *queue, void *data)
typedef OS_JOB_CALLBACK(Os_Job_Callback);
internal void os_add_job_to_queue(struct Os_Job_Queue *queue, Os_Job_Callback *callback, void *data);

#if PROFILER
inline u64 os_get_perf_counter();
inline f32 os_seconds_elapsed(u64 last_counter);
#endif