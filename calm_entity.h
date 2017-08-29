
enum entity_type
{
    Entity_Null,
    Entity_Block, 
    Entity_Guru, 
    Entity_Philosopher,
    Entity_Player,
    Entity_Camera,
    Entity_Home,
    
    ///////
    Entity_Count,
};

enum search_type {
    SEARCH_VALID_SQUARES,
    SEARCH_INVALID_SQUARES,
    SEARCH_FOR_TYPE,
};

struct search_cell {
    v2i Pos;
    v2i CameFrom;
    
    search_cell *Prev;
    search_cell *Next;
};

struct path_nodes {
    v2 Points[512];
    u32 Count;
};

struct entity
{
    entity_type Type;
    
    v2 Pos;
    v2 Velocity;
    v2 Dim;
    
    u32 WalkSoundAt; //For Sound effects
    
    r32 LifeSpan;//seconds
    
    //NOTE(oliver): This is used by the camera only at the moment 1/5/17
    v2 AccelFromLastFrame; 
    //
    
    //For UI editor. Maybe move it to the UI element struct. Oliver 13/8/17 
    v2 RollBackPos;
    //
    r32 Time;
    //particle_system ParticleSystem;
    
    // NOTE(OLIVER): This is for the philosopher AI random walk
    v2i LastMoves[1];
    u32 LastMoveAt;
    v2i LastSearchPos;
    //
    
    u32 VectorIndexAt;
    u32 ChunkTypeCount;
    chunk_type ValidChunkTypes[16];
    
    path_nodes Path;
    
    b32 IsAtEndOfMove;
    
    v2 EndOffsetTargetP;
    v2 BeginOffsetTargetP;
    
    r32 MoveT;
    r32 MovePeriod;
    
    s32 LifePoints;
    mat2 Rotation;
    r32 FacingDirection;    
    
    b32 TriggerAction;
    b32 Moves;
    
    r32 InverseWeight;
    b32 Collides; 
    u32 Index;
    
    particle_system ParticleSystem;
    
};

struct line
{
    v2 Begin;
    v2 End;
};
