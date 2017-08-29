
internal b32 SaveLevelToDisk(game_memory *Memory, game_state *GameState, char *FileName) {
    b32 WasSuccessful = true;
    u32 OffsetAt = 0;
    //Actually write data
    game_file_handle Handle = Memory->PlatformBeginFileWrite(FileName);
    
    char MemoryToWrite1[sizeof(world_chunk)] = {};
    
    s32 SizeOfMemoryToWrite1 = Print(MemoryToWrite1, "//CHUNKS\n");
    
    Memory->PlatformWriteFile(&Handle, MemoryToWrite1, SizeOfMemoryToWrite1, OffsetAt);
    
    OffsetAt += SizeOfMemoryToWrite1;
    
    
    if(!Handle.HasErrors) {
        //Compile the data into format
        for(u32 i = 0; i < ArrayCount(GameState->Chunks); ++i) {
            world_chunk *Chunk = GameState->Chunks[i];
            while(Chunk) {
                
                char MemoryToWrite[sizeof(world_chunk)] = {};
                
                s32 SizeOfMemoryToWrite = Print(MemoryToWrite, "%d %d %d %d\n", Chunk->X, Chunk->Y, Chunk->Type, Chunk->MainType);
                
                Memory->PlatformWriteFile(&Handle, MemoryToWrite, SizeOfMemoryToWrite, OffsetAt);
                
                OffsetAt += SizeOfMemoryToWrite;
                
                WasSuccessful &= (!Handle.HasErrors);
                
                
                Chunk = Chunk->Next;
            }
        }
        
        if(!Handle.HasErrors) {
            WasSuccessful = true;
        }
    }
    
    Memory->PlatformEndFile(Handle);
    
    return WasSuccessful;
}


internal b32 LoadAndReadLevelFile(game_state *GameState, game_memory *Memory) {
    b32 WasSuccessful = true;
    char *FileName = "level1.omm";
    game_file_handle Handle =  Memory->PlatformBeginFile(FileName);
    if(!Handle.HasErrors) {
        AddToOutBuffer("Loaded Level File \n");
        size_t FileSize = Memory->PlatformFileSize(FileName);
        temp_memory TempMem = MarkMemory(&GameState->ScratchPad);
        void *FileMemory = PushSize(&GameState->ScratchPad, FileSize);
        if(FileSize) {
            Memory->PlatformReadFile(Handle, FileMemory, FileSize, 0);
            if(!Handle.HasErrors) {
                char*At = (char *)FileMemory;
                while(true) {
                    if(*At == '\0') {
                        break;
                    }
                    s32 Negative = 1;
                    s32 Data[4] = {};
                    u32 DataAt = 0;
                    if(*At == '\n') {
                        At++;
                    }
                    while(*At != '\n' && *At != '\0') {
                        switch(*At) {
                            case ' ': {
                                At++;
                            } break;
                            case '/': {
                                At++;
                                if(*At == '/') {
                                    while(*At != '\n' && *At != '\0') {
                                        At++;
                                    }
                                    if(*At == '\n') {
                                        At++;
                                    }
                                    
                                }
                            } break;
                            case '-': {
                                Negative = -1;
                                At++;
                            } break;
                            default: {
                                if(IsNumeric(*At)) {
                                    u32 ValuesAt = 0;
                                    s32 Values[512] = {};
                                    while(IsNumeric(*At)) {
                                        Values[ValuesAt++] = (s32)(*At - '0');
                                        At++;
                                    }
                                    u32 SizeOfArray = ValuesAt - 1;
                                    for(u32 i = 0; i < ValuesAt; ++i) {
                                        Data[DataAt++] += Negative*Values[i]*Power(10, (SizeOfArray - i));
                                    }
                                    Negative = 1;
                                    
                                } else {
                                    InvalidCodePath;
                                }
                            }
                            
                        }
                        
                    }
                    if(DataAt == 4) { 
                        world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, Data[0], Data[1], &GameState->MemoryArena, (chunk_type)Data[3]);
                        Chunk->Type = (chunk_type)Data[2];
                    }
                }
            }
        }
        ReleaseMemory(&TempMem);
        Memory->PlatformEndFile(Handle);
    } else {
        WasSuccessful = false;
        
        
    }
    return WasSuccessful;
    
}