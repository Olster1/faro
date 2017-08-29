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

internal void
GameUpdateAndRender(bitmap *Buffer, game_memory *Memory, render_group *OrthoRenderGroup, render_group *RenderGroup,  r32 dt)
{
    game_state *GameState = (game_state *)Memory->GameStorage;
    r32 MetersToPixels = 60.0f;
    if(!GameState->IsInitialized)
    {
        
#include "calm_startup.cpp"
        
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
                
                v2i PlayerMove = {};
                
                
                if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Left]))
                {
                    PlayerMove = V2int(-1, 0);
                }
                if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Right]))
                {
                    PlayerMove = V2int(1, 0);
                }
                if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Down]))
                {
                    PlayerMove = V2int(0, -1);
                }
                if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Up]))
                {
                    PlayerMove = V2int(0, 1);
                }
                
                if(GameState->UIState->ControlCamera || GameState->UIState->GamePaused) {
                    GlobalPlayerMove = PlayerMove;
                    PlayerMove = {};
                }
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
                    
                    r32 MinX = (X*WorldChunkInMeters) - Camera->Pos.X;
                    r32 MinY = (Y*WorldChunkInMeters) - Camera->Pos.Y;
                    rect2 Rect = Rect2(MinX, MinY, MinX + WorldChunkInMeters,
                                       MinY + WorldChunkInMeters);
                    
                    if(Chunk) {
                        
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
