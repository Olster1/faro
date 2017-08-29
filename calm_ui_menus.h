            
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