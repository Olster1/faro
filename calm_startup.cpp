                                                                                        
                                                                                            InitializeMemoryArena(&GameState->MemoryArena, (u8 *)Memory->GameStorage + sizeof(game_state), Memory->GameStorageSize - sizeof(game_state));
                                                                                        
                                                                                        GameState->PerFrameArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
                                                                                        
                                                                                        GameState->ScratchPad = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
                                                                                        
                                                                                        GameState->RenderArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
                                                                                        
                                                                                        GameState->UIState = PushStruct(&GameState->MemoryArena, ui_state);
                                                                                        
                                                                                        /////////////////////////////////
                                                                                        
                                                                                        GameState->RenderConsole = true;
                                                                                        b32 WasSuccessful = LoadAndReadLevelFile(GameState, Memory);
                                                                                        if(!WasSuccessful) {
                                                                                            AddWorldChunks(GameState, 0, 200, ChunkLight);
                                                                                        }
                                                                                        
                                                                                        GameState->Player = InitEntity(GameState, V2(10, 10), V2(WorldChunkInMeters, WorldChunkInMeters), Entity_Player);
                                                                                        AddChunkType(GameState->Player, ChunkDark);
                                                                                        AddChunkType(GameState->Player, ChunkLight);
                                                                                        
                                                                                        GameState->Camera = InitEntity(GameState, GameState->Player->Pos, V2(0, 0), Entity_Camera, false);
                                                                                        
                                                                                        InitEntity(GameState, V2(15, 10), V2(WorldChunkInMeters, WorldChunkInMeters), Entity_Home, false);
                                                                                        
                                                                                        
#include "calm_bitmaps.h"
#include "calm_wav_files.h"
                                                                                        
                                                                                        GameState->LanternAnimationCount = 0;
                                                                                        
                                                                                        char *FileNames[16] = {"lantern_man/lm_run_left_l.bmp",
                                                                                            "lantern_man/lm_run_left_r.bmp"};
                                                                                        
                                                                                        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 2, PI32, -0.3f);
                                                                                        
                                                                                        FileNames[0] = "lantern_man/lm_stand_left.bmp";
                                                                                        
                                                                                        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 1, PI32, -0.01f );
                                                                                        
                                                                                        FileNames[0] = "lantern_man/lm_run_right_l.bmp";
                                                                                        FileNames[1] = "lantern_man/lm_run_right_r.bmp";
                                                                                        
                                                                                        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 2, 0, 0.3f);
                                                                                        
                                                                                        FileNames[0] = "lantern_man/lm_stand_right.bmp";
                                                                                        
                                                                                        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 1, 0, 0.01f);
                                                                                        
                                                                                        FileNames[0] = "lantern_man/lm_front.bmp";
                                                                                        
                                                                                        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 1, PI32*1.5f, 0);
                                                                                        
                                                                                        GameState->GameMode = PLAY_MODE;
                                                                                        
                                                                                        GameState->Fonts = LoadFontFile("fonts.clm", Memory, &GameState->MemoryArena);
                                                                                        
                                                                                        font_quality_value Qualities[FontQuality_Count] = {};
                                                                                        Qualities[FontQuality_Debug] = InitFontQualityValue(1.0f);
                                                                                        Qualities[FontQuality_Size] = InitFontQualityValue(1.0f);
                                                                                        
                                                                                        //GameState->Font = FindFont(GameState->Fonts, Qualities);
                                                                                        GameState->DebugFont = FindFontByName(GameState->Fonts, "Liberation Mono", Qualities);
                                                                                        GameState->GameFont = FindFontByName(GameState->Fonts, "Karmina Regular", Qualities);
                                                                                        Memory->DebugFont = GameState->DebugFont;
                                                                                        InitConsole(&DebugConsole, GameState->DebugFont, 0, 2.0f, &GameState->MemoryArena);
                                                                                        
                                                                                        
                                                                                        GameState->PauseMenu.Timer.Period = 2.0f;
                                                                                        GameState->PauseMenu.Font = GameState->GameFont;
                                                                                        GameState->GameOverMenu.Timer.Period = 2.0f;
                                                                                        GameState->GameOverMenu.Font = GameState->GameFont;
                                                                                        
                                                                                        GameState->FramePeriod = 0.2f;
                                                                                        GameState->FrameIndex = 0;
                                                                                        GameState->FrameTime = 0;
                                                                                        GameState->CurrentAnimation = &GameState->LanternManAnimations[0];
                                                                                        
                                                                                        GameState->SunsetColors[0] = V3(0, 0, 0);
                                                                                        GameState->SunsetColors[1] = V3(0.371302f, 0.135257f, 0.504477f);
                                                                                        GameState->SunsetColors[2] = V3(0.787456f, 0.259568f, 0.271249f);
                                                                                        GameState->SunsetColors[3] = V3(0.979377f, 0.451467f, 0.0511329f);
                                                                                        GameState->SunsetColors[4] = V3(0.999918f, 0.681763f, 0.129456f);
                                                                                        GameState->SunsetColors[5] = V3(1.0f, 0.881837f, 0.490371f);
                                                                                        GameState->SunsetColors[6] = V3(1, 1, 1);
                                                                                        
#include "calm_ui_menus.h"
                                                                                        
                                                                                        
                                                                                        /// NOTE(OLIVER): Make sure player is on a valid tile
                                                                                        v2i PlayerPos = GetGridLocation(GameState->Player->Pos);
                                                                                        GetOrCreateWorldChunk(GameState->Chunks, PlayerPos.X, PlayerPos.Y, &GameState->MemoryArena, ChunkDark);
        /////