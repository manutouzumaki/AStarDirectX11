void *
PushSize(arena *Arena, size_t Size)
{
    Assert(Arena->Use + Size <= Arena->Size);
    void *Result = (void *)(Arena->Base + Arena->Use);
    Arena->Use += Size;
    return Result; 
}

void 
InitArena(app_memory *AppMemory, arena *Arena, size_t Size)
{
    Arena->Base = (char *)AppMemory->Memory + AppMemory->Use;
    Arena->Size = Size;
    AppMemory->Use += Size;
}

void 
ClearArena(arena *Arena)
{
    Arena->Use = 0;
}
