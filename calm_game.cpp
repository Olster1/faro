/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "calm_game.h"
#include "meta_enum_arrays.h"
#include "calm_sound.cpp"
#include "calm_render.cpp"
#include "calm_menu.cpp"
#include "calm_console.cpp"
#include "calm_chunks.cpp"
#include "calm_io.cpp"
#include "calm_entity.cpp"
#include "calm_ai.cpp"
#include "calm_ui.cpp"
#include "calm_meta.h"


internal void InitAnimation(animation *Animation, game_memory *Memory,  char **FileNames, u32 FileNameCount, r32 DirectionValue, r32 PoseValue) {
    Animation->Qualities[POSE] = PoseValue;
    Animation->Qualities[DIRECTION] = DirectionValue;
    
    for(u32 i = 0; i < FileNameCount; ++i) {
        Animation->Frames[Animation->FrameCount++] = LoadBitmap(Memory, 0, FileNames[i]);
        
    }
}

static quality_info QualityInfo[ANIMATE_QUALITY_COUNT] = {{1.0, false}, {2*PI32, true}};

internal animation *GetAnimation(animation *Animations, u32 AnimationsCount,  r32 *Qualities,
                                 r32 *Weights) {
    animation *BestResult = 0;
    r32 BestValue = 100000.0f;
    forN(AnimationsCount) {
        animation *Anim = Animations + AnimationsCountIndex;
        r32 ThisValue = 0;
        for(u32 i = 0; i < ANIMATE_QUALITY_COUNT; ++i) {
            r32 Value = Qualities[i];
            r32 TestValue = Anim->Qualities[i];
            quality_info Info= QualityInfo[i];
            
            r32 DifferenceSqr = Sqr(TestValue - Value);
            if(Info.IsPeriodic) {
                if(DifferenceSqr > Sqr(Info.MaxValue/2)) {
                    r32 Min = TestValue;
                    r32 Max = Value;
                    if(Max < Min) {
                        Min = Value;
                        Max = TestValue;
                    }
                    Min += Info.MaxValue/2;
                    DifferenceSqr = Sqr(Max - Min);
                }
            }
            ThisValue += Weights[i]*(DifferenceSqr / Info.MaxValue);
        }
        if(ThisValue < BestValue) {
            BestResult = Anim;
            BestValue = ThisValue;
        }
    }
    
    Assert(BestResult);
    return BestResult;
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
#if 0
    // TODO(OLIVER): Work on this
    if(TargetP == StartP) {
        // TODO(OLIVER): This might not be a valid position
        StartP = GetClosestGridLocation(Entity->Pos);
    }
#endif
    
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

internal void
GameUpdateAndRender(bitmap *Buffer, game_memory *Memory, render_group *OrthoRenderGroup, render_group *RenderGroup,  r32 dt)
{
    game_state *GameState = (game_state *)Memory->GameStorage;
    r32 MetersToPixels = 60.0f;
    if(!GameState->IsInitialized)
    {
        
        InitializeMemoryArena(&GameState->MemoryArena, (u8 *)Memory->GameStorage + sizeof(game_state), Memory->GameStorageSize - sizeof(game_state));
        
        GameState->PerFrameArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->ScratchPad = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->RenderArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->UIState = PushStruct(&GameState->MemoryArena, ui_state);
        
        /////////////////////////////////
        
        GameState->RenderConsole = true;
        
        GameState->Player = InitEntity(GameState, V2(-1, -1), V2(WorldChunkInMeters, WorldChunkInMeters), Entity_Player);
        AddChunkType(GameState->Player, ChunkDark);
        AddChunkType(GameState->Player, ChunkLight);
        
        GameState->Camera = InitEntity(GameState, V2(0, 0), V2(0, 0), Entity_Camera);
        GameState->Camera->Collides = false;
        
        //InitEntity(GameState, V2(2, 2), V2(1, 1), Entity_Guru, true);
        
#if CREATE_PHILOSOPHER
        entity *Philosopher = InitEntity(GameState, V2(2, 2), V2(1, 1), Entity_Philosopher, true);
        Philosopher->MovePeriod = 0.4f;
        AddChunkType(Philosopher, ChunkLight);
#endif
        //"Moonlight_Hall.wav","Faro.wav"
        GameState->BackgroundMusic = LoadWavFileDEBUG(Memory, "podcast1.wav", &GameState->MemoryArena);
        //PushSound(GameState, &GameState->BackgroundMusic, 1.0, false);
        GameState->FootstepsSound[0] = LoadWavFileDEBUG(Memory, "foot_steps_sand1.wav", &GameState->MemoryArena);
        
        GameState->FootstepsSound[1] = LoadWavFileDEBUG(Memory, "foot_steps_sand2.wav", &GameState->MemoryArena);
        
        
        GameState->MossBlockBitmap = LoadBitmap(Memory, 0, "moss_block1.bmp");
        GameState->MagicianHandBitmap = LoadBitmap(Memory, 0, "magician_hand.bmp");
        
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
        
        v2 Pos = {};
        fori_count(10) {
            do {
                Pos.X = (r32)RandomBetween(&GameState->GeneralEntropy, -10, 10);
                Pos.Y = (r32)RandomBetween(&GameState->GeneralEntropy, -10, 10);
            } while(Pos == GameState->Player->Pos 
                    #if CREATE_PHILOSOPHER
                    || Pos == Philosopher->Pos
                    #endif
                    );
            AddEntity(GameState, Pos, Entity_Block);
        }
        
        // NOTE(OLIVER): Create Board
        
        // NOTE(OLIVER): Make sure player is on a valid tile
        v2i PlayerPos = GetGridLocation(GameState->Player->Pos);
        GetOrCreateWorldChunk(GameState->Chunks, PlayerPos.X, PlayerPos.Y, &GameState->MemoryArena, ChunkDark);
#if CREATE_PHILOSOPHER
        v2i PhilosopherPos = GetGridLocation(Philosopher->Pos);
        GetOrCreateWorldChunk(GameState->Chunks, PhilosopherPos.X, PhilosopherPos.Y, &GameState->MemoryArena, ChunkLight);
#endif
        ///////////////
#if 1
        ui_element_settings UISet = {};
        UISet.Pos = V2(0.7f*(r32)Buffer->Width, (r32)Buffer->Height);
        UISet.Name = 0;
        
        PushUIElement(GameState->UIState, UI_Moveable,  UISet);
        {
            UISet.Type = UISet.Name = "Save Level";
            AddUIElement(GameState->UIState, UI_Button, UISet);
        }
        PopUIElement(GameState->UIState);
        
        UISet= {};
        UISet.Pos = V2(0, (r32)Buffer->Height);
        
        PushUIElement(GameState->UIState, UI_Moveable,  UISet);
        {
            PushUIElement(GameState->UIState, UI_DropDownBoxParent, UISet);
            {
                for(u32 TypeI = 0; TypeI < ArrayCount(entity_type_Values); ++TypeI) {
                    UISet.Name = entity_type_Names[TypeI];
                    UISet.EnumArray.Array = entity_type_Values;
                    UISet.EnumArray.Index = TypeI;
                    UISet.EnumArray.Type = entity_type_Names[ArrayCount(entity_type_Names) - 1];
                    UISet.ValueLinkedTo = &GameState->UIState->InitInfo;
                    
                    AddUIElement(GameState->UIState, UI_DropDownBox, UISet);
                }
                
                for(u32 TypeI = 0; TypeI < ArrayCount(chunk_type_Values); ++TypeI) {
                    UISet.Name = chunk_type_Names[TypeI];
                    UISet.EnumArray.Array = chunk_type_Values;
                    UISet.EnumArray.Index = TypeI;
                    UISet.EnumArray.Type = chunk_type_Names[ArrayCount(chunk_type_Names) - 1];
                    UISet.ValueLinkedTo = &GameState->UIState->InitInfo;
                    
                    AddUIElement(GameState->UIState, UI_DropDownBox, UISet);
                }
            }
            PopUIElement(GameState->UIState);
            
            UISet.ValueLinkedTo = &GameState->UIState->ControlCamera;
            UISet.Name = "Control Camera";
            AddUIElement(GameState->UIState, UI_CheckBox, UISet);
            
            UISet.ValueLinkedTo = &GameState->UIState->GamePaused;
            UISet.Name = "Pause Game";
            AddUIElement(GameState->UIState, UI_CheckBox, UISet);
        }
        PopUIElement(GameState->UIState);
#endif
        GameState->IsInitialized = true;
    }
    EmptyMemoryArena(&GameState->RenderArena);
    InitRenderGroup(OrthoRenderGroup, Buffer, &GameState->RenderArena, MegaBytes(1), V2(1, 1), V2(0, 0), V2(1, 1));
    
    
    
    InitRenderGroup(RenderGroup, Buffer, &GameState->RenderArena, MegaBytes(1), V2(60, 60), 0.5f*V2i(Buffer->Width, Buffer->Height), V2(1, 1));
    
    temp_memory PerFrameMemory = MarkMemory(&GameState->PerFrameArena);
    
    DebugConsole.RenderGroup = OrthoRenderGroup;
    
    PushClear(RenderGroup, V4(0.5f, 0.5f, 0.5f, 1)); // TODO(Oliver): I guess it doesn't matter but maybe push this to the ortho group once z-index scheme is in place. 
    
    entity *Player = GameState->Player;
    
    switch(GameState->GameMode) {
        case MENU_MODE: {
            UpdateMenu(&GameState->PauseMenu,GameState, Memory, OrthoRenderGroup, dt);
            GameState->RenderConsole = false;
            
        } break;
        case GAMEOVER_MODE: {
            
            char *MenuOptions[] = {"Restart", "Quit"};
            
            if(WasPressed(Memory->GameButtons[Button_Enter])) {
                switch(GameState->GameOverMenu.IndexAt) {
                    case 0: {
                        Player->Pos = V2(-1, -1);
                        Player->VectorIndexAt = Player->Path.Count;
                        GameState->GameMode = PLAY_MODE;
                    } break;
                    case 1: {
                        EndProgram(Memory); 
                    } break;
                }
                GameState->GameOverMenu.Timer.Value = 0.0f;
            }
            
            UpdateAndRenderMenu(&GameState->GameOverMenu, OrthoRenderGroup, MenuOptions, ArrayCount(MenuOptions), dt, Memory);
        } break;
        case WIN_MODE: {
            char *MenuOptions[] = {"Restart", "Quit"};
            //Go to next level here?
            if(WasPressed(Memory->GameButtons[Button_Enter])) {
                switch(GameState->GameOverMenu.IndexAt) {
                    case 0: {
                        Player->Pos = V2(-1, -1);
                        Player->VectorIndexAt = Player->Path.Count;
                        GameState->GameMode = PLAY_MODE;
                    } break;
                    case 1: {
                        EndProgram(Memory); 
                    } break;
                }
                GameState->GameOverMenu.Timer.Value = 0.0f;
            }
            
            UpdateAndRenderMenu(&GameState->GameOverMenu, OrthoRenderGroup, MenuOptions, ArrayCount(MenuOptions), dt, Memory);
        }
        case PLAY_MODE: {
            GameState->RenderConsole = true;
            
            v2 MouseP_PerspectiveSpace = InverseTransform(&RenderGroup->Transform, V2(Memory->MouseX, Memory->MouseY));
            v2 MouseP_OrthoSpace = InverseTransform(&OrthoRenderGroup->Transform, V2(Memory->MouseX, Memory->MouseY));
            v2i GlobalPlayerMove = {};
            
            if(WasPressed(Memory->GameButtons[Button_Escape]))
            {
                GameState->GameMode = MENU_MODE;
                break;
            }
            if(WasPressed(Memory->GameButtons[Button_F1]))
            {
                if(DebugConsole.ViewMode == VIEW_MID) {
                    DebugConsole.ViewMode = VIEW_CLOSE;
                    DebugConsole.InputIsActive = false;
                } else {
                    DebugConsole.ViewMode = VIEW_MID;
                    DebugConsole.InputIsActive = true;
                    DebugConsole.InputClickTimer.Value = 0;
                }
                
            }
            if(WasPressed(Memory->GameButtons[Button_F2]))
            {
                if(DebugConsole.ViewMode == VIEW_FULL) {
                    DebugConsole.ViewMode = VIEW_CLOSE;
                    DebugConsole.InputIsActive = false;
                } else {
                    DebugConsole.ViewMode = VIEW_FULL;
                    DebugConsole.InputIsActive = true;
                    DebugConsole.InputClickTimer.Value = 0;
                }
            }
            
            if(WasPressed(Memory->GameButtons[Button_LeftMouse]))
            {
                if(InBounds(DebugConsole.InputConsoleRect, (s32)Memory->MouseX, (s32)Memory->MouseY)) {
                    DebugConsole.InputIsHot = true;
                }
            }
            if(WasReleased(Memory->GameButtons[Button_LeftMouse])) {
                
                if(InBounds(DebugConsole.InputConsoleRect, (s32)Memory->MouseX, (s32)Memory->MouseY) && DebugConsole.InputIsHot) {
                    DebugConsole.InputIsHot = false;
                    DebugConsole.InputIsActive = true;
                    DebugConsole.InputClickTimer.Value = 0;
                } else if(!DebugConsole.InputIsHot){
                    DebugConsole.InputIsActive = false;
                }
            }
            
            if(DebugConsole.InputIsActive) {
                if(WasPressed(Memory->GameButtons[Button_Arrow_Left])) {
                    DebugConsole.Input.WriteIndexAt = Max(0, (s32)DebugConsole.Input.WriteIndexAt - 1);
                    DebugConsole.InputTimer.Value = 0;
                }
                if(WasPressed(Memory->GameButtons[Button_Arrow_Right])) {
                    DebugConsole.Input.WriteIndexAt = Min(DebugConsole.Input.IndexAt, DebugConsole.Input.WriteIndexAt + 1);
                    DebugConsole.InputTimer.Value = 0;
                }
                for(u32 KeyIndex = 0; KeyIndex <= Memory->SizeOfGameKeys; ++KeyIndex) {
                    
                    game_button *Button = Memory->GameKeys + KeyIndex;
                    
                    if(Button->IsDown) {
                        DebugConsole.InputTimer.Value = 0;
                        switch(KeyIndex) {
                            case 8: { //Backspace
                                Splice_("", &DebugConsole.Input, -1);
                            } break;
                            default: {
                                AddToInBuffer("%s", &KeyIndex);
                            }
                        }
                    }
                }
                if(WasPressed(Memory->GameButtons[Button_Enter])) {
                    IssueCommand(&DebugConsole.Input);
                    ClearBuffer(&DebugConsole.Input);
                }
                
            } else {
                
                v2i PlayerGridP = {};
                v2i PlayerMove = {};
                
                if(CanMove(Player)) {  // NOTE(Oliver): This is to stop the player moving diagonal.  
                    if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Left]))
                    {
                        PlayerMove = V2int(-1, 0);
                        PlayerGridP = GetGridLocation(Player->Pos);
                    }
                    else if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Right]))
                    {
                        PlayerMove = V2int(1, 0);
                        PlayerGridP = GetClosestGridLocation(Player->Pos);
                    }
                    else if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Down]))
                    {
                        PlayerMove = V2int(0, -1);
                        PlayerGridP = GetGridLocation(Player->Pos);
                    }
                    else if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Up]))
                    {
                        PlayerMove = V2int(0, 1);
                        PlayerGridP = GetClosestGridLocation(Player->Pos);
                    }
                }
                
                if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Space]))
                {
                    if(GameState->PlayerIsSettingPath) {
                        PlayerGridP = GetGridLocation(Player->Pos);
                        entity *Entity = InitEntity(GameState, V2i(PlayerGridP), V2(1, 1), Entity_Dropper);
                        AddChunkType(Entity, ChunkLight);
                        AddChunkType(Entity, ChunkDark);
                        Entity->VectorIndexAt = 1;
                        Entity->Path = GameState->PathToSet;
                        GameState->PlayerIsSettingPath = false;
                    } else {
                        GameState->PlayerIsSettingPath = true;
                        GameState->PathToSet.Count = 0;
                        v2 PosToAdd = V2i(GetGridLocation(Player->Pos));
                        Assert(IsValidGridPosition(GameState, PosToAdd));
                        AddToPath(&GameState->PathToSet, PosToAdd);
                    }
                    
                }
                
                if(GameState->PlayerIsSettingPath) {
                    v2i DefineMove = {};
                    if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Left]))
                    {
                        DefineMove = V2int(-1, 0);
                    }
                    else if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Right]))
                    {
                        DefineMove = V2int(1, 0);
                        
                    }
                    else if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Down]))
                    {
                        DefineMove = V2int(0, -1);
                        
                    }
                    else if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Up]))
                    {
                        DefineMove = V2int(0, 1);
                        
                    }
                    
                    PlayerMove = {};
                    v2 LastMove = V2i(GetGridLocation(Player->Pos));
                    
                    if(DefineMove != V2int(0, 0)) {
                        if(GameState->PathToSet.Count > 0) {
                            LastMove =  GameState->PathToSet.Points[GameState->PathToSet.Count - 1];
                            v2 NewMove = LastMove + V2i(DefineMove);
                            if(IsValidGridPosition(GameState, NewMove)) {
                                AddToPath(&GameState->PathToSet, NewMove);
                            } else {
                                //Abort? I think this is a gameplay decision,and later code will handle whether it is valid or not.
                            }
                            
                        } 
                    }
                }
                
                if(GameState->UIState->ControlCamera || GameState->UIState->GamePaused) {
                    GlobalPlayerMove = PlayerMove;
                    PlayerMove = {};
                }
                
#if !MOVE_VIA_MOUSE 
                if(PlayerMove != V2int(0, 0)) {
                    v2i GridTargetPos = (PlayerGridP + PlayerMove);
                    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, GridTargetPos.X, GridTargetPos.Y, 0, ChunkNull);
                    
                    // NOTE(OLIVER): This is where the player moves the block...
                    if(Chunk && Chunk->Type == ChunkBlock) {
                        v2i BlockTargetPos = GridTargetPos + PlayerMove;
                        world_chunk *NextChunk = GetOrCreateWorldChunk(GameState->Chunks, BlockTargetPos.X, BlockTargetPos.Y, 0, ChunkNull);
                        entity *Block = GetEntity(GameState, WorldChunkInMeters*V2i(GridTargetPos), Entity_Block);
                        Assert(Block && Block->Type == Entity_Block);
                        if(NextChunk && IsValidType(NextChunk, Block->ValidChunkTypes, Block->ChunkTypeCount) && !GetEntity(GameState, V2i(BlockTargetPos), Entity_Philosopher, false)) {
                            Chunk->Type = Chunk->MainType;
                            NextChunk->Type = ChunkBlock;
                            InitializeMove(GameState, Block, WorldChunkInMeters*V2i(BlockTargetPos));
                            
                        }
                    }
                    
                    InitializeMove(GameState, Player, WorldChunkInMeters*V2i(GridTargetPos), true);
                }
#endif
            }
            
            r32 Qualities[ANIMATE_QUALITY_COUNT] = {};
            r32 Weights[ANIMATE_QUALITY_COUNT] = {1.0f, 1.0f};
            Qualities[POSE] = Length(Player->Velocity);
            Qualities[DIRECTION] = (r32)atan2(Player->Velocity.Y, Player->Velocity.X); 
#if 0
            animation *NewAnimation = GetAnimation(GameState->LanternManAnimations, GameState->LanternAnimationCount, Qualities, Weights);
#else 
            animation *NewAnimation = &GameState->LanternManAnimations[4];
#endif
            r32 FastFactor = 1000;
            GameState->FramePeriod = 0.2f; //FastFactor / LengthSqr(Player->Velocity);
            // TODO(OLIVER): Make this into a sineous function
            GameState->FramePeriod = Clamp(0.2f, GameState->FramePeriod, 0.4f);
            
            Assert(NewAnimation);
            if(NewAnimation != GameState->CurrentAnimation) {
                GameState->FrameIndex = 0;
                GameState->CurrentAnimation = NewAnimation;
            }
            
            rect2 BufferRect = Rect2(0, 0, (r32)Buffer->Width, (r32)Buffer->Height);
            
            entity *Camera = GameState->Camera;
            
            
            
#if UPDATE_CAMERA_POS
            r32 DragFactor = 0.2f;
            v2 Acceleration = Camera->AccelFromLastFrame;
            if(GameState->UIState->ControlCamera) { //maybe turn these into defines like on Handmade Hero?
                Acceleration = 400*V2i(GlobalPlayerMove);
            } 
            Camera->Pos = dt*dt*Acceleration + dt*Camera->Velocity + Camera->Pos;
            Camera->Velocity = Camera->Velocity + dt*Acceleration - DragFactor*Camera->Velocity;
            
#endif
            
            v2 CamPos = GameState->Camera->Pos;
            
#define DRAW_GRID 1
            
#if DRAW_GRID
            r32 ScreenFactor = 0.7f;
            v2 HalfScreenDim = ScreenFactor*Hadamard(Inverse(RenderGroup->Transform.Scale), RenderGroup->ScreenDim);
            v2i Min = ToV2i(CamPos - HalfScreenDim);
            v2i Max = ToV2i(CamPos + HalfScreenDim);
            
            for(s32 X = Min.X; X < Max.X; X++) {
                for(s32 Y = Min.Y; Y < Max.Y; ++Y) {
                    
                    v2i GridP = GetGridLocation(V2i(X, Y));
                    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, GridP.X, GridP.Y, 0, ChunkNull);
                    if(Chunk) {
                        
                        r32 MinX = (Chunk->X*WorldChunkInMeters) - Camera->Pos.X;
                        r32 MinY = (Chunk->Y*WorldChunkInMeters) - Camera->Pos.Y;
                        rect2 Rect = Rect2(MinX, MinY, MinX + WorldChunkInMeters,
                                           MinY + WorldChunkInMeters);
                        
                        v4 Color01 = {0, 0, 0, 1};
                        switch (Chunk->Type) {
                            case ChunkDark: {
                                Color01 = {0.2f, 0.2f, 0.2f, 1};
                            } break;
                            case ChunkLight: {
                                Color01 = {0.8f, 0.8f, 0.6f, 1};
                            } break;
                            case ChunkBlock: {
                                Color01 = {0.7f, 0.3f, 0, 1};
                            } break;
                            case ChunkNull: {
                                Color01 = {0, 0, 0, 1};
                            } break;
                            default: {
                                InvalidCodePath;
                            }
                        }
                        
#if DRAW_PLAYER_PATH
                        if(IsPartOfPath(Chunk->X, Chunk->Y, &Player->Path)) {
                            Color01 = {1, 0, 1, 1};
                            PushRect(RenderGroup, Rect, 1, Color01);
                        } else 
#endif
                        {
                            PushRect(RenderGroup, Rect, 1, Color01);
                        }
                    }
                }
            }
#endif
            
            r32 PercentOfScreen = 0.1f;
            v2 CameraWindow = (1.0f/MetersToPixels)*PercentOfScreen*V2i(Buffer->Width, Buffer->Height);
            //NOTE(): Draw Camera bounds that the player stays within. 
            PushRectOutline(RenderGroup, Rect2(-CameraWindow.X, -CameraWindow.Y, CameraWindow.X, CameraWindow.Y), 1, V4(1, 0, 1, 1));
            ////
            for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
            {
                entity *Entity = GameState->Entities + EntityIndex;
                {
                    v2 EntityRelP = Entity->Pos - CamPos;
                    
                    rect2 EntityAsRect = Rect2MinDim(EntityRelP, Entity->Dim);
                    
                    switch(Entity->Type)
                    {
                        case((u32)Entity_Player):
                        {
                            if(!GameState->UIState->GamePaused) {
                                /////////NOTE(Oliver): Update Player's postion/////////////// 
                                
                                b32 DidMove = UpdateEntityPositionViaFunction(GameState, Entity, dt);
                                if(DidMove) {
                                    PushSound(GameState, &GameState->FootstepsSound[Entity->WalkSoundAt++], 1.0, false);
                                    if(Entity->WalkSoundAt >= 2) {
                                        Entity->WalkSoundAt = 0;
                                    }
                                }
                                
                                ///////// NOTE(OLIVER): Update camera acceleration
                                if((Abs(EntityRelP.X) > CameraWindow.X || Abs(EntityRelP.Y) > CameraWindow.Y)) {
                                    Camera->AccelFromLastFrame = 10.0f*EntityRelP;
                                } else {
                                    Camera->AccelFromLastFrame = V2(0, 0);
                                }
                                /////// NOTE(OLIVER): Player Animation
                                if(GameState->CurrentAnimation) {
                                    bitmap CurrentBitmap = GameState->CurrentAnimation->Frames[GameState->FrameIndex];
                                    
                                    GameState->FrameTime += dt;
                                    if(GameState->FrameTime > GameState->FramePeriod) {
                                        GameState->FrameTime = 0;
                                        GameState->FrameIndex++;
                                        if(GameState->FrameIndex >= GameState->CurrentAnimation->FrameCount) {
                                            GameState->FrameIndex = 0;
                                        }
                                    }
                                    
                                    //PushBitmap(RenderGroup, V3(EntityRelP, 1), &CurrentBitmap,  BufferRect);
                                }
                            }
                            
                            PushRect(RenderGroup, EntityAsRect, 1, V4(0, 1, 1, 1));
                            ///////
                        } break;
                        case Entity_Philosopher: {
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
                            //PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->MossBlockBitmap,  BufferRect);
                        } break;
                        case Entity_Block: {
                            if(!GameState->UIState->GamePaused) {
                                UpdateEntityPositionViaFunction(GameState, Entity, dt);
                            }
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0.9f, 0.6f, 1)); 
                            if(!IsValidGridPosition(GameState, Entity->Pos)) {
                                PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 0, 0.4f)); 
                            }
                            
                            PushRectOutline(RenderGroup, EntityAsRect, 1, V4(0, 0, 0, 1)); 
                            //PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->MossBlockBitmap,  BufferRect);
                        } break;
                        case Entity_Dropper: {
                            b32 Moved = UpdateEntityPositionViaFunction(GameState, Entity, dt);
                            if(Moved) {
                                entity* Changer = InitEntity(GameState, Entity->Pos, WorldChunkInMeters*V2(0.5f, 0.5f), Entity_Chunk_Changer);
                                AddChunkTypeToChunkChanger(Changer, ChunkLight);
                                AddChunkTypeToChunkChanger(Changer, ChunkDark);
                                
                            }
                            
                            if(!HasMovesLeft(Entity)) {
                                //Add to a list to remove afterwards
                                RemoveEntity(GameState, EntityIndex);
                            } else {
                                PushRect(RenderGroup, EntityAsRect, 1, V4(0.8f, 0.8f, 0, 1));
                            }
                        } break;
                        case Entity_Chunk_Changer: {
                            
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 0, 0.1f));
                            
                            v2i GridP = GetGridLocation(Entity->Pos);
                            
                            world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, GridP.X, GridP.Y, 0, ChunkNull);
                            Assert(Chunk);
                            b32 MoveToNextChunk = UpdateTimer(&Entity->ChunkTimer, dt);
                            if(MoveToNextChunk) {
                                Chunk->Type = Entity->ChunkList[Entity->ChunkAt];
                                if(Entity->LoopChunks) {
                                    Entity->ChunkAt = WrapU32(0, Entity->ChunkAt + 1, Entity->ChunkListCount - 1);
                                } else {
                                    Entity->ChunkAt = ClampU32(0, Entity->ChunkAt + 1, Entity->ChunkListCount - 1);
                                }
                            }
                            //Destory Entity if past its life span
                            Entity->LifeSpan -= dt;
                            if(Entity->LifeSpan <= 0.0f) {
                                b32 WasRemoved = RemoveEntity(GameState, EntityIndex);
                                Assert(WasRemoved);
                            }
                            
                            
                        } break;
                        case Entity_Home: {
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 1, 1));
                            entity *Philospher = GetEntity(GameState, Entity->Pos, Entity_Philosopher);
                            if(Philospher) {
                                GameState->GameMode = WIN_MODE;
                            }
                        } break;
                        case((u32)Entity_Guru): {
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 1, 1));
                            if(Entity->TriggerAction) {
                                //AddToOutBuffer("TriggerAction");
                                draw_text_options TextOptions = InitDrawOptions();
                                TextOptions.AdvanceYAtStart = true;
                                char *DialogString = "Hello there weary traveller, how have you been?";
                                rect2 Bounds = Rect2MinMaxWithCheck(V2(EntityRelP.X, EntityRelP.Y), V2(EntityRelP.X + 4, EntityRelP.Y - 4));
                                PushRectOutline(RenderGroup, Bounds, 1, V4(0, 1, 1, 1));
                                
                                TextToOutput(RenderGroup, GameState->GameFont, DialogString, EntityRelP.X, EntityRelP.Y, Bounds, V4(0, 0, 0, 1), &TextOptions);
                                
                                
                            }
                            
                            //PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->MossBlockBitmap,  BufferRect);
                        } break;
                    }
                }
            }
            
            UpdateAndDrawUI(GameState->UIState, GameState, CamPos, MouseP_PerspectiveSpace, MouseP_OrthoSpace, GameState->DebugFont, OrthoRenderGroup, Memory, RenderGroup);
            
        } //END OF PLAY_MODE 
    } //switch on GAME_MODE
    
    
    if(GameState->RenderConsole) {
        RenderConsole(&DebugConsole, dt); 
    }
    
    //RenderGroupToOutput(RenderGroup);
    //RenderGroupToOutput(&OrthoRenderGroup);
    
    ReleaseMemory(&PerFrameMemory);
    
}
