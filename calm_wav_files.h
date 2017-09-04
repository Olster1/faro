        InitializeAudioState(&GameState->AudioState, &GameState->MemoryArena);
        
        //"Moonlight_Hall.wav","Faro.wav"
        GameState->BackgroundMusic = LoadWavFileDEBUG(Memory, "Faro.wav", 0, 0, &GameState->MemoryArena);
        PlaySound(&GameState->AudioState, &GameState->BackgroundMusic);
        GameState->FootstepsSound[0] = LoadWavFileDEBUG(Memory, "foot_steps_sand1.wav", 0, 0, &GameState->MemoryArena);
        
        GameState->FootstepsSound[1] = LoadWavFileDEBUG(Memory, "foot_steps_sand2.wav", 0, 0, &GameState->MemoryArena);
        