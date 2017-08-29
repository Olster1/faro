
inline void PushOnToList(search_cell **SentinelPtr, s32 X, s32 Y, s32 CameFromX, s32 CameFromY,  memory_arena *Arena, search_cell **FreeListPtr) {
    search_cell *Sentinel = *SentinelPtr;
    
    search_cell *NewCell = *FreeListPtr;
    
    if(NewCell) {
        *FreeListPtr = (*FreeListPtr)->Next;
    } else {
        NewCell = PushStruct(Arena, search_cell);
    }
    
    NewCell->Pos.X = X;
    NewCell->Pos.Y = Y;
    
    NewCell->CameFrom.X = CameFromX;
    NewCell->CameFrom.Y = CameFromY;
    
    NewCell->Next = Sentinel->Next;
    NewCell->Prev = Sentinel;
    
    Sentinel->Next->Prev = NewCell;
    Sentinel->Next = NewCell;
}

inline void RemoveOffList(search_cell **SentinelPtr, search_cell *CellToRemove, search_cell **FreelistPtr) {
    search_cell *Sentinel = *SentinelPtr;
    
    Assert(CellToRemove != Sentinel);
    
    CellToRemove->Next->Prev = CellToRemove->Prev;
    CellToRemove->Prev->Next = CellToRemove->Next;
    
    CellToRemove->Next = *FreelistPtr;
    *FreelistPtr = CellToRemove;
    CellToRemove->Prev = 0;
    
}

inline b32 PushToList(world_chunk **WorldChunks, s32 X, s32 Y, s32 CameFromX, s32 CameFromY,  world_chunk **VisitedHash, search_cell **SentinelPtr, search_cell **FreeListPtr, v2i TargetP, 
                      memory_arena *TempArena, search_type SearchType, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    b32 Found = false;
    
    switch(SearchType) {
        case SEARCH_VALID_SQUARES: {world_chunk *Chunk = GetOrCreateWorldChunk(WorldChunks, X, Y, 0, ChunkTypes[0]);
            if(Chunk && IsValidType(Chunk, ChunkTypes, ChunkTypeCount) && !GetOrCreateWorldChunk(VisitedHash, X, Y, 0, ChunkTypes[0])) {   
                
                world_chunk *VisitedChunk = GetOrCreateWorldChunk(VisitedHash, X, Y, TempArena, ChunkTypes[0]);
                VisitedChunk->MainType = Chunk->Type;
                
                if(X == TargetP.X && Y == TargetP.Y) { 
                    Found = true;
                } 
                PushOnToList(SentinelPtr, X, Y, CameFromX, CameFromY, TempArena, FreeListPtr); 
            }
        } break;
        case SEARCH_FOR_TYPE: { 
            if(!GetOrCreateWorldChunk(VisitedHash, X, Y, 0, ChunkTypes[0])) {   
                GetOrCreateWorldChunk(VisitedHash, X, Y, TempArena, ChunkTypes[0]); 
                world_chunk *Chunk = GetOrCreateWorldChunk(WorldChunks, X, Y, 0, ChunkTypes[0]);
                if(Chunk && Chunk->MainType == ChunkTypes[0]) { 
                    Found = true;
                }  
                PushOnToList(SentinelPtr, X, Y, CameFromX, CameFromY, TempArena, FreeListPtr); 
            }
        } break;
        case SEARCH_INVALID_SQUARES: { 
            if(!GetOrCreateWorldChunk(VisitedHash, X, Y, 0, ChunkTypes[0])) {   
                GetOrCreateWorldChunk(VisitedHash, X, Y, TempArena, ChunkTypes[0]); 
                world_chunk *Chunk = GetOrCreateWorldChunk(WorldChunks, X, Y, 0, ChunkTypes[0]);
                if(Chunk && IsValidType(Chunk, ChunkTypes, ChunkTypeCount)) { 
                    Found = true;
                }  
                PushOnToList(SentinelPtr, X, Y, CameFromX, CameFromY, TempArena, FreeListPtr); 
            }
        } break;
        default: {
            InvalidCodePath;
        }
    }
    
    
    return Found;
}

struct search_info {
    search_cell *SearchCellList;
    world_chunk **VisitedHash;
    
    search_cell *Sentinel;
    
    b32 Found;
    
    b32 Initialized;
};

inline void InitSearchInfo(search_info *Info,  game_state *GameState, memory_arena *TempArena) {
    Info->SearchCellList = 0;
    Info->VisitedHash = PushArray(TempArena, world_chunk *, ArrayCount(GameState->Chunks), true);
    
    Info->Sentinel = &GameState->SearchCellSentinel;
    
    Info->Sentinel->Next = Info->Sentinel->Prev = Info->Sentinel;
    
    Info->Found = false;
    
    Info->Initialized = true;
}

internal search_cell *CalculatePath(game_state *GameState, world_chunk **WorldChunks, v2i StartP, v2i EndP,
                                    memory_arena *TempArena, search_type SearchType, search_info *Info, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    
    /* 
    We are doing this because for the other search types we flip the direction around for the AI search. Maybe look into this to get a better idea so we don't have to have this.
    */
    if(SearchType == SEARCH_FOR_TYPE) {
        v2i TempP = StartP;
        StartP = EndP;
        EndP = TempP;
    }
    
    s32 AtX = StartP.X;
    s32 AtY = StartP.Y;
    
#define PUSH_TO_LIST(X, Y) if(!Info->Found) {Info->Found = PushToList(WorldChunks, X, Y, AtX, AtY, Info->VisitedHash, &Info->Sentinel, &GameState->SearchCellFreeList, EndP, TempArena, SearchType, ChunkTypes,ChunkTypeCount); }
    
    if(!Info->Initialized) {
        InitSearchInfo(Info, GameState, TempArena);
        PUSH_TO_LIST(AtX, AtY);
    }
    
    
    search_cell *Cell = 0;
    while(Info->Sentinel->Prev != Info->Sentinel && !Info->Found) {
        Cell = Info->Sentinel->Prev;
        AtX = Cell->Pos.X;
        AtY = Cell->Pos.Y;
        
        PUSH_TO_LIST((AtX + 1), AtY);
        PUSH_TO_LIST((AtX - 1), AtY);
        PUSH_TO_LIST(AtX, (AtY + 1));
        PUSH_TO_LIST(AtX, (AtY - 1));
#if MOVE_DIAGONAL
        PUSH_TO_LIST((AtX + 1), (AtY + 1));
        PUSH_TO_LIST((AtX + 1), (AtY - 1));
        PUSH_TO_LIST((AtX - 1), (AtY + 1));
        PUSH_TO_LIST((AtX - 1), (AtY - 1));
#endif
        
        RemoveOffList(&Info->Sentinel, Cell, &Info->SearchCellList);
    }
    
    if(Info->Found) {
        RemoveOffList(&Info->Sentinel, Info->Sentinel->Next, &Info->SearchCellList);
    }
    
    
    return Info->SearchCellList;
}

//This returns a boolean if the location we were looking for was found. 
internal b32 RetrievePath(game_state *GameState, v2i StartP, v2i EndP, path_nodes *Path, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    b32 Result = false;
    
    memory_arena *TempArena = &GameState->ScratchPad;
    temp_memory TempMem = MarkMemory(TempArena);
    search_info SearchInfo = {};
    
    search_cell *SearchCellList = CalculatePath(GameState, GameState->Chunks, StartP, EndP, TempArena,  SEARCH_VALID_SQUARES, &SearchInfo, ChunkTypes, ChunkTypeCount);
    
    // NOTE(Oliver): This will fire if the player is standing on a light world piece but is not connected to the philosopher's area. 
    // TODO(Oliver): Do we want to change this to an 'if' to account for when the player is out of the zone. Like abort the search and go back to random walking?
    if(SearchInfo.Found) {
        Result = true;
        search_cell *Cell = SearchCellList;
        
        //Compile path taken to get to the destination
        v2i LookingFor = Cell->Pos;
        while(SearchCellList) {
            b32 Found = false;
            for(search_cell **TempCellPtr = &SearchCellList; *TempCellPtr; TempCellPtr = &(*TempCellPtr)->Next) {
                search_cell *TempCell = *TempCellPtr;
                if(TempCell->Pos.X == LookingFor.X && TempCell->Pos.Y == LookingFor.Y) {
                    Assert(Path->Count < ArrayCount(Path->Points));
                    Path->Points[Path->Count++] = ToV2(LookingFor);
                    LookingFor = TempCell->CameFrom;
                    *TempCellPtr = TempCell->Next;
                    Found = true;
                    break;
                }
            }
            
            if(!Found) {
                break;
            }
        }
        
        
    }
    
    ReleaseMemory(&TempMem);
    
    return Result;
}

//NOTE(Ollie): Pulled this out from the below code to use elsewhere. 
internal void ComputeValidArea(game_state *GameState, search_info *SearchInfo, v2i StartP, chunk_type *ChunkTypes, u32 ChunkTypeCount, memory_arena *TempArena) {
    
    CalculatePath(GameState, GameState->Chunks, StartP, V2int(MAX_S32, MAX_S32), TempArena,  SEARCH_VALID_SQUARES, SearchInfo, ChunkTypes, ChunkTypeCount);
    
    Assert(!SearchInfo->Found);
}



/*NOTE(Ollie): This works by first flood filling the island
the player is situated on using the visited hash and setting the targetPos to somwhere impossible (a very big number). This gives us all 
valid positions the player could move to. We then find the closest 
valid cell to where the player clicked, only searching the hash of
valid cells returned to us by the last funciton. 
*/
internal v2i SearchForClosestValidPosition_(game_state *GameState, v2i TargetP, v2i StartP, chunk_type *ChunkTypes, u32 ChunkTypeCount, search_type SearchType) {
    
    v2i Result = {};
    memory_arena *TempArena = &GameState->ScratchPad;
    temp_memory TempMem = MarkMemory(TempArena);
    //Make valid square search area by choosing a really far away position
    
    search_info FirstStageSearchInfo = {};
    ComputeValidArea(GameState, &FirstStageSearchInfo, StartP, ChunkTypes, ChunkTypeCount, TempArena);
    //Find the target within the valid search area. 
    search_info SecondStageSearchInfo = {};
    
    search_cell *SearchCellList = CalculatePath(GameState, FirstStageSearchInfo.VisitedHash, TargetP, StartP, TempArena, SearchType, &SecondStageSearchInfo, ChunkTypes, ChunkTypeCount);
    search_cell *Cell = SearchCellList;
    Assert(SecondStageSearchInfo.Found); //Not sure about this Assert: I think it can be found or not found 
    
    Assert(GetOrCreateWorldChunk(GameState->Chunks, Cell->Pos.X, Cell->Pos.Y, 0, ChunkTypes[0]));
    
    Result = Cell->Pos;
    
    ReleaseMemory(&TempMem);
    return Result;
}

internal v2i SearchForClosestValidPosition(game_state *GameState, v2i TargetP, v2i StartP, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    v2i Result = SearchForClosestValidPosition_(GameState, TargetP, StartP, ChunkTypes, ChunkTypeCount, SEARCH_INVALID_SQUARES);
    return Result;
}

internal v2i SearchForClosestValidChunkType(game_state *GameState, v2i StartP, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    v2i Result = SearchForClosestValidPosition_(GameState, V2int(MAX_S32, MAX_S32), StartP, ChunkTypes, ChunkTypeCount, SEARCH_FOR_TYPE);
    return Result;
}

inline b32 HasMovesLeft(entity *Entity) {
    b32 Result = Entity->VectorIndexAt < Entity->Path.Count;
    return Result;
}

internal inline r32 GetFractionOfMove(entity *Entity) {
    r32 tValue = Clamp(0, Entity->MoveT / Entity->MovePeriod, 1);
    return tValue;
}

internal inline b32 CanMove(entity *Entity, r32 PercentOfMoveFinished = 1.0f) {
    b32 Result = false;
    if(!HasMovesLeft(Entity) || GetFractionOfMove(Entity) > PercentOfMoveFinished) {
        Result = true;
    }
    
    return Result;
}

inline b32 ContainsChunkType(entity *Entity, chunk_type ChunkType) {
    
    b32 Result = false;
    fori_count(Entity->ChunkTypeCount) {
        if(Entity->ValidChunkTypes[Index] == ChunkType) {
            Result = true;
            break;
        }
    }
    return Result;
}

internal inline b32 IsValidChunkType(game_state *GameState, entity *Entity, v2i Pos, b32 Break = false) {
    b32 Result = false;
    world_chunk *ChunkHeadingFor = GetOrCreateWorldChunk(GameState->Chunks, Pos.X, Pos.Y, 0, ChunkNull);
    if(ChunkHeadingFor) {
        Result = ContainsChunkType(Entity, ChunkHeadingFor->Type);
    }
    
    if(Break && !Result) {
        BreakPoint;
    }
    return Result;
}

internal b32 UpdateEntityPositionViaFunction(game_state *GameState, entity *Entity, r32 dt) {
    b32 WentToNewMove = false;
    Entity->IsAtEndOfMove = true;
    if(HasMovesLeft(Entity)) {
        Entity->IsAtEndOfMove = false;
        Assert(Entity->VectorIndexAt > 0);
        v2i GridA = ToV2i(Entity->Path.Points[Entity->VectorIndexAt - 1]);
        v2 A = WorldChunkInMeters*V2i(GridA);
        
        v2i GridB = ToV2i(Entity->Path.Points[Entity->VectorIndexAt]);
        v2 B = WorldChunkInMeters*V2i(GridB);
        
        if(!IsValidChunkType(GameState, Entity, GridB)) {
            chunk_type Types[] = {ChunkLight, ChunkDark};
            v2i NewPos = SearchForClosestValidChunkType(GameState, GridA, Types,ArrayCount(Types));
            Entity->Pos = WorldChunkInMeters*V2i(NewPos);
            
        }
        
        Entity->MoveT += dt;
        
        r32 tValue = GetFractionOfMove(Entity);
        
        Entity->Pos = SineousLerp0To1(A, tValue, B);
        
        //For Animation
        v2 Direction = Normal(B - A);
        Entity->Velocity = 20.0f*Direction;
        //
        
        if(Entity->MoveT / Entity->MovePeriod >= 1.0f) {
            Entity->Pos = B;
            Entity->MoveT -= Entity->MovePeriod;
            Entity->VectorIndexAt++;
            WentToNewMove = true;
            Entity->IsAtEndOfMove = false;
#if 1
            if(HasMovesLeft(Entity)) {
                
                v2i GoingTo = ToV2i(Entity->Path.Points[Entity->VectorIndexAt]);
                if(!IsValidChunkType(GameState, Entity, GoingTo)) {
                    Entity->Path.Count = 0;
                    Assert(!HasMovesLeft(Entity));
                }
            }
#endif
        }
    }
    
    return WentToNewMove;
}

internal b32 IsPartOfPath(s32 X, s32 Y, path_nodes *Path) {
    b32 Result = false;
    for(u32 PathIndex = 0; PathIndex < Path->Count; ++PathIndex){
        if(Path->Points[PathIndex].X == X && Path->Points[PathIndex].Y == Y) {
            Result = true;
            break;
        }
    }
    
    return Result;
}


internal b32 
InitializeMove(game_state *GameState, entity *Entity, v2 TargetP_r32, b32 StrictMove = false) {
    
    r32 MetersToWorldChunks = 1.0f / WorldChunkInMeters;
    
    v2i TargetP = ToV2i_floor(MetersToWorldChunks*TargetP_r32); 
    v2i StartP = GetGridLocation(Entity->Pos);
    
    if(StrictMove) {
        memory_arena *TempArena = &GameState->ScratchPad;
        temp_memory TempMem = MarkMemory(TempArena);
        search_info SearchInfo = {};
        ComputeValidArea(GameState, &SearchInfo, StartP, Entity->ValidChunkTypes, Entity->ChunkTypeCount, TempArena);
        
        world_chunk *Chunk = GetOrCreateWorldChunk(SearchInfo.VisitedHash, TargetP.X, TargetP.Y, 0, ChunkNull);
        if(Chunk) {
            //Keep TargetP the same
        } else {
            TargetP = StartP;
        }
        
        ReleaseMemory(&TempMem);
    } else{
        if(!IsValidChunkType(GameState, Entity, StartP)) {
            chunk_type Types[] = {ChunkLight, ChunkDark};
            v2i NewPos = SearchForClosestValidChunkType(GameState, StartP, Types,ArrayCount(Types));
            Entity->Pos = WorldChunkInMeters*V2i(NewPos);
            StartP = GetGridLocation(Entity->Pos);
            Assert(IsValidChunkType(GameState, Entity, StartP));
        } 
        
        TargetP = SearchForClosestValidPosition(GameState, TargetP, StartP, Entity->ValidChunkTypes, Entity->ChunkTypeCount);
    }
    
    Entity->BeginOffsetTargetP = Entity->Pos- WorldChunkInMeters*ToV2(StartP); 
    Entity->EndOffsetTargetP = TargetP_r32 - WorldChunkInMeters*ToV2(TargetP);
    
    Entity->Path.Count = 0;
    b32 FoundTargetP =  RetrievePath(GameState, TargetP, StartP, &Entity->Path, Entity->ValidChunkTypes, Entity->ChunkTypeCount);
    if(FoundTargetP) {
        v2 * EndPoint = Entity->Path.Points + (Entity->Path.Count - 1);
        v2 *StartPoint = Entity->Path.Points;
        *StartPoint = *StartPoint + Entity->BeginOffsetTargetP;
#if END_WITH_OFFSET
        *EndPoint = *EndPoint + Entity->EndOffsetTargetP;
#endif
        Entity->VectorIndexAt = 1;
        Entity->MoveT = 0;
    }
    
    return FoundTargetP;
}

/*
This is the code we had for updating the AI. It looks for the players postion and if it finds it, it sets that as its new path. If not it just does a random walk. 

//////////////////////////////
    if(!GameState->UIState->GamePaused) {
        //First we try to find the player
        v2i PlayerPos = GetGridLocation(Player->Pos);
        world_chunk *PlayerChunk = GetOrCreateWorldChunk(GameState->Chunks, PlayerPos.X, PlayerPos.Y, 0, ChunkNull);
        Assert(PlayerChunk);
        b32 WasSuccessful = false;
        
        // NOTE(Oliver): This should be a more complete function that compares paths and decides if we need to construct a new one.
        
        //Case where the chunk changes to a new type, so search isn't valid anymore -> Maybe go to last valid position...
        // TODO(Oliver): Maybe we could fix this during the move. If the chunk suddenly changes to an invalid chunk, we will try backtrace or go to the nearest valid chunk type. 
        
        if(LookingForPosition(Entity, PlayerPos) && !ContainsChunkType(Entity, PlayerChunk->Type)) {
            EndPath(Entity);
        }
        
        if(Entity->LastSearchPos != PlayerPos &&  ContainsChunkType(Entity, PlayerChunk->Type) && Entity->IsAtEndOfMove) {
            v2 TargetPos_r32 = WorldChunkInMeters*V2i(PlayerPos);
            
            WasSuccessful = InitializeMove(GameState, Entity, TargetPos_r32);
            if(WasSuccessful) {
                Entity->LastSearchPos = PlayerPos;
            }
        } 
        //Then if we couldn't find the player we just move a random direction
        if(!WasSuccessful && !HasMovesLeft(Entity)) {
        
            random_series *RandGenerator = &GameState->GeneralEntropy;
            
            s32 PossibleStates[] = {-1, 0, 1};
            v2i Dir = {};
            for(;;) {
                b32 InArray = false;
                fori(Entity->LastMoves) {
                    if(Dir == Entity->LastMoves[Index]) {
                        InArray = true;
                        break;
                    }
                }
                if(InArray) {
                    s32 Axis = RandomBetween(RandGenerator, 0, 1);
                    if(Axis) {
                        Dir.X = RandomBetween(RandGenerator, -1, 1);
                        Dir.Y = 0;
                        
                    } else {
                        Dir.X = 0;
                        Dir.Y = RandomBetween(RandGenerator, -1, 1);
                    }
                    
                } else {
                    break;
                }
            }
            Entity->LastMoves[Entity->LastMoveAt++] = V2int(-Dir.X,-Dir.Y);
            if(Entity->LastMoveAt >= ArrayCount(Entity->LastMoves)) { Entity->LastMoveAt = 0;}
            
            v2 TargetPos_r32 = WorldChunkInMeters*V2i(GetGridLocation(Entity->Pos) + Dir);
            b32 SuccessfulMove = InitializeMove(GameState, Entity, TargetPos_r32);
            Assert(SuccessfulMove);
        }
        
        UpdateEntityPositionViaFunction(GameState, Entity, dt);
        if(GetGridLocation(Entity->Pos) == GetGridLocation(Player->Pos)) {
            GameState->GameMode = GAMEOVER_MODE;
        }
    }
    PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 0, 1));
}
*/