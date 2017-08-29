
#define GetChunkHash(X, Y) (Abs(X*19 + Y*23) % WORLD_CHUNK_HASH_SIZE)
internal world_chunk *GetOrCreateWorldChunk(world_chunk **Chunks, s32 X, s32 Y, memory_arena *Arena, chunk_type Type) {
    u32 HashIndex = GetChunkHash(X, Y);
    world_chunk *Chunk = Chunks[HashIndex];
    world_chunk *Result = 0;
    
    while(Chunk) {
        
        if(Chunk->X == X && Chunk->Y == Y) {
            Result = Chunk;
            break;
        }
        
        Chunk = Chunk->Next;
    } 
    
    if(!Result && Arena) {
        Assert(Type != ChunkNull);
        Result  = PushStruct(Arena, world_chunk);
        Result->X = X;
        Result->Y = Y;
        Result->MainType = Result->Type = Type;
        Result->Next = Chunks[HashIndex];
        Chunks[HashIndex] = Result;
    }
    
    return Result;
}

inline void AddWorldChunks(game_state *GameState, s32 Min, s32 Max, chunk_type ChunkType) {
    for(s32 x = Min; x < Max; x++) {
        for(s32 y = Min; y < Max; y++) {
            world_chunk *Result = GetOrCreateWorldChunk(GameState->Chunks, x, y, &GameState->MemoryArena, ChunkType);
            Assert(Result);
        }
    }
    
}


inline void AddWorldChunks(game_state *GameState, u32 Count, s32 Min, s32 Max, chunk_type ChunkType) {
    
    fori_count(Count) {
        s32 Y = RandomBetween(&GameState->GeneralEntropy, Min, Max);
        s32 X = RandomBetween(&GameState->GeneralEntropy, Min, Max);
        
        world_chunk *Result = GetOrCreateWorldChunk(GameState->Chunks, X, Y, &GameState->MemoryArena, ChunkType);
        Assert(Result);
        
    }
    
}

inline v2i GetGridLocation(v2 Pos) {
    r32 MetersToWorldChunks = 1.0f / WorldChunkInMeters;
    
    v2i Result = ToV2i_floor(MetersToWorldChunks*Pos); 
    
    return Result;
}

inline v2i GetClosestGridLocation(v2 Pos) {
    r32 MetersToWorldChunks = 1.0f / WorldChunkInMeters;
    
    v2i Result = ToV2i_ceil(MetersToWorldChunks*Pos); 
    
    return Result;
}

inline v2 GetClosestGridLocationR32(v2 Pos) {
    v2i TempLocation = GetClosestGridLocation(Pos);
    v2 Result = V2i(TempLocation.X, TempLocation.Y);
    return Result;
}

inline v2 GetGridLocationR32(v2 Pos) {
    v2i TempLocation = GetGridLocation(Pos);
    v2 Result = V2i(TempLocation.X, TempLocation.Y);
    return Result;
}

inline b32 IsValidGridPosition(game_state *GameState, v2 WorldPos) {
    v2i GridPos = GetClosestGridLocation(WorldPos);
    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, GridPos.X, GridPos.Y, 0, ChunkNull);
    b32 Result = false;
    if(Chunk) {
        Result = true;
    }
    return Result;
}


inline b32 IsValidType(world_chunk *Chunk, chunk_type *Types, u32 TypeCount) {
    b32 Result = false;
    forN(TypeCount) {
        chunk_type Type = Types[TypeCountIndex];
        //Assert(Type != ChunkBlock);
        if(Type == Chunk->Type) {
            Result = true;
            break;
        }
    }
    return Result;
}

inline void SetChunkType(game_state *GameState, v2 Pos, chunk_type ChunkType) {
    v2i GridLocation = GetGridLocation(Pos);
    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, (s32)GridLocation.X, (s32)GridLocation.Y, 0, ChunkNull);
    
    Assert(ChunkType != ChunkNull);
    if(Chunk){
        if(ChunkType == ChunkMain) {
            Chunk->Type = Chunk->MainType;
        } else {
            Chunk->Type = ChunkType;
        }
    }
}

