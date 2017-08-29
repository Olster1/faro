        enum element_ui_type {
        UI_Parent,
        UI_Moveable,
        UI_DropDownBoxParent,
        UI_DropDownBox,
        UI_CheckBox,
        UI_Button,
        UI_Entity
    };
    
    struct enum_array_data  {
        u32 Index;
        void* Array;
        char *Type;
    };
    
    
    struct ui_element;
    struct ui_element_settings {
        v2 Pos;
        v2 Dim;
        b32 Active;
        
        //DropDown data
        u32 ActiveChildIndex; //make this null to show if Active?
        //
        
        char *Name;
        void *ValueLinkedTo;
        
        union {
            struct {
                enum_array_data EnumArray;
            };
            char *Type;
        };
    };
    
#define AlterValue(ValueToAlter, Array, Index, type) {*(type *)ValueToAlter = (type)((type *)Array)[Index];}
    
#define GetEntityFromElement(UIState) entity *Entity = (entity *)UIState->InteractingWith->Set.ValueLinkedTo;
    
#define GetEntityFromElement_(Element) entity *Entity = (entity *)Element->Set.ValueLinkedTo;
    
    struct ui_element {
        element_ui_type Type;
        
        ui_element_settings Set;
        
        // TODO(NAME): This is for looping through the children, Instead of making another type?
        u32 TempChildrenCount; 
        //
        
        ui_element *Parent;
        
        u32 Index;
        
        u32 ChildrenCount;
        u32 ChildrenIndexes[32];
        
        b32 IsValid;
    };
    
    struct ui_stack_item {
        element_ui_type Type;
        u32 Index;
    };
    
#define CastValue(Info, type) type Value = ((type *)Info->Array)[Info->Index]; 
    
    struct ui_state {
        u32 ElmCount;
        ui_element Elements[128];
        
        u32 FreeElmCount;
        u32 ElementsFree[64];
        
        u32 StackAt;
        ui_stack_item StackItems [64]; //Maybe put these on local stack?
        
        ui_element *InteractingWith;
        ui_element *HotEntity;
        
        //DEBUG VARIABLES
        b32 ControlCamera;
        enum_array_data InitInfo;
        b32 GamePaused;
    };
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            