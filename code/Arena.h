#ifndef ARENA_H
#define ARENA_H

struct app_memory
{
    void *Memory;
    size_t Size;
    size_t Use;
};

struct arena
{
    char *Base;
    size_t Size;
    size_t Use;
};

#define PushStruct(Arena, Struct) PushSize(Arena, sizeof(Struct))
#define PushArray(Arena, Count, Struct) PushSize(Arena, sizeof(Struct)*(Count))

#endif
