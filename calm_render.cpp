internal void
ClearBitmap(bitmap *Bitmap, v4 Color01) {
    
    u32 Color255 = ((u32)(255.0f*Color01.A) << 24 |
                    (u32)(255.0f*Color01.R) << 16 |
                    (u32)(255.0f*Color01.G) << 8 |
                    (u32)(255.0f*Color01.B) << 0);
    
    
    u32 *Bits = (u32 *)Bitmap->Bits;
    
    for(u32 Y = 0;
        Y < Bitmap->Height;
        ++Y)
    {
        for(u32 X = 0;
            X < Bitmap->Width;
            ++X)
        {
            *Bits++ = Color255;
        }
    }
    
}

internal void
BltRectangle(bitmap *Bitmap, s32 XCorner, s32 YCorner, s32 Width, s32 Height, v4 Color01)
{
    
    u32 Color255 = ((u32)(255.0f*Color01.A) << 24 |
                    (u32)(255.0f*Color01.R) << 16 |
                    (u32)(255.0f*Color01.G) << 8 |
                    (u32)(255.0f*Color01.B) << 0);
    
    s32 X1 = XCorner;
    s32 Y1 = YCorner;
    s32 X2 = XCorner + Width;
    s32 Y2 = YCorner + Height;
    
    s32 XMin = Min(X1, X2);
    s32 XMax = Max(X1, X2);
    s32 YMin = Min(Y1, Y2);
    s32 YMax = Max(Y1, Y2);
    
    if(XMin < 0) { XMin = 0; }
    if(YMin < 0) { YMin = 0; }
    
    if(XMax > (s32)Bitmap->Width) { XMax = Bitmap->Width; }
    if(YMax > (s32)Bitmap->Height) { YMax = Bitmap->Height; }
    
    u8 *Bits = (u8 *)Bitmap->Bits + YMin*Bitmap->Pitch + XMin*sizeof(u32);
    
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)Bits;
        for(s32 X = XMin;
            X < XMax;
            ++X)
        {
            *Pixel++ = Color255;
        }
        Bits += Bitmap->Pitch;
    }
    
}

inline u32
Color01AsV4To255AsU32(v4 Color01)
{
    u32 Color255 = ((u32)(255.0f*Color01.A) << 24 |
                    (u32)(255.0f*Color01.R) << 16 |
                    (u32)(255.0f*Color01.G) << 8 |
                    (u32)(255.0f*Color01.B) << 0);
    
    return Color255;
    
}

internal void
FillRectangle(bitmap *Bitmap, s32 XCorner, s32 YCorner, s32 Width, s32 Height, u32 Color255)
{
    s32 X1 = XCorner;
    s32 Y1 = YCorner;
    s32 X2 = XCorner + Width;
    s32 Y2 = YCorner + Height;
    
    s32 XMin = Min(X1, X2);
    s32 XMax = Max(X1, X2);
    s32 YMin = Min(Y1, Y2);
    s32 YMax = Max(Y1, Y2);
    
    if(XMin < 0) { XMin = 0; }
    if(YMin < 0) { YMin = 0; }
    
    if(XMax > (s32)Bitmap->Width) { XMax = Bitmap->Width; }
    if(YMax > (s32)Bitmap->Height) { YMax = Bitmap->Height; }
    
    u8 *Bits = (u8 *)Bitmap->Bits + YMin*Bitmap->Pitch + XMin*sizeof(u32);
    
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)Bits;
        for(s32 X = XMin;
            X < XMax;
            ++X)
        {
            *Pixel++ = Color255;
        }
        Bits += Bitmap->Pitch;
    }
    
}
internal void
FillRectangle(bitmap *Bitmap, s32 XCorner, s32 YCorner, s32 Width, s32 Height, v4 Color01)
{
    u32 Color255 = Color01AsV4To255AsU32(Color01);
    
    FillRectangle(Bitmap, XCorner, YCorner, Width, Height, Color255);
}

internal void
FillRectangle(bitmap *Bitmap, rect2 Rect, v4 Color01)
{
    FillRectangle(Bitmap, (s32)Rect.MinX, (s32)Rect.MinY, (s32)(Rect.MaxX - Rect.MinX), (s32)(Rect.MaxY - Rect.MinY), Color01);
}


#pragma pack(push, 1)
struct bitmap_header
{
    u16 MagicID;             
    u32 FileSize;            
    u16 Reserved1, Reserved2;
    u32 DataOffsetIntoFile;        
    u32 HeaderSize;        
    s32 Width, Height;     
    u16 ColorPlanes;       
    u16 BitsPerPixel;       
    u32 Compression;        
    u32 ImageSize;          
    
};
#pragma pack(pop)

internal bitmap
LoadBitmap(game_memory *Memory, memory_arena *MemoryArena, char *FileName, v2 AlignPercent = V2(0.5f, 0.5f)) {
    bitmap Result = {};
    
    file_read_result FileContents = Memory->PlatformReadEntireFile(FileName);
    
    if(FileContents.Data)
    {
        bitmap_header *BitmapHeader = (bitmap_header *)FileContents.Data;
        Assert(BitmapHeader->MagicID == 'MB');
        Assert(BitmapHeader->BitsPerPixel == 8 || BitmapHeader->BitsPerPixel == 32);
        Assert(BitmapHeader->Compression == 0);
        
        Result.Width = BitmapHeader->Width;
        Result.Height = BitmapHeader->Height;
        Result.Pitch = BitmapHeader->Width * sizeof(u32);
        Result.AlignPercent = AlignPercent;
        
        Result.Bits = (void *)((u8 *)BitmapHeader + BitmapHeader->DataOffsetIntoFile);
        
        
        if(BitmapHeader->BitsPerPixel == 8)
        {
            //This supports 8 bits per pixel images allowing them to be mapped to a 32bit/pixel space. 
            if(MemoryArena) {
                u32 *NewBits = PushArray(MemoryArena, u32, Result.Width*Result.Height);
                u32 Size  = Result.Width*Result.Height*sizeof(u8);
                
                u8 *NewBits_u8 = (u8 *)NewBits;
                u8 *OldBits_u8 = (u8 *)Result.Bits;
                while(Size--)
                {
                    u8 Value = *OldBits_u8++;
                    *NewBits_u8++ = 255 - Value;
                    *NewBits_u8++ = 255 - Value;
                    *NewBits_u8++ = 255 - Value;
                    *NewBits_u8++ = 255;
                    
                }
                
                Result.Bits = NewBits;
            }
        } else { //pre-multiplied alpha
            u8 *At = (u8 *)Result.Bits;
            for(s32 i = 0; i < (s32)(Result.Width*Result.Height); ++i) {
                r32 Alpha = (r32)(At[3])/255.0f;
                for(int colorIndex = 0; colorIndex < 3; ++colorIndex) {
                    At[colorIndex] = (u8)((r32)At[colorIndex]* Alpha); 
                }
                
                At += 4; 
            }
        }
        
    }
    
    return Result;
    
}

internal bitmap
CreateNewBitmap(memory_arena *Arena, u32 Width, u32 Height, b32 Clear)
{
    bitmap Result;
    
    Result.Width = Width;
    Result.Height = Height;
    Result.Pitch = Width *sizeof(u32);
    
    u32 MemorySize = Width*Height*sizeof(u32);
    Result.Bits = PushArray(Arena, u32, Width*Height);
    
    if(Clear)
    {
        ClearMemory(Result.Bits, MemorySize);
    }
    
    return Result;
}

void SortRenderGroup(render_group *Group) {
    u32 *Indexes = Group->SortedIndexes = PushArray(&Group->Arena, u32, Group->ElmCount);
    
    for(u32 i = 0; i < Group->ElmCount; ++i) {
        Indexes[i] = i;
    }
    
    render_element *Elms = (render_element *)Group->Arena.Base;
    
    
    u32 LastIndex = 0;
    b32 Sorted = false;
    while(!Sorted) {
        Sorted = true;
        r32 LastSortKey = Elms[Indexes[0]].SortKey;
        for(u32 i = 0; i < Group->ElmCount; ++i) {
            render_element *Elm = Elms + Indexes[i];
            if(Elm->SortKey < LastSortKey) {
                Assert(i);
                Indexes[i - 1] = Indexes[i];
                Indexes[i] = LastIndex;
                Sorted = false;
            }
            
            LastIndex = Indexes[i];
            LastSortKey = Elm->SortKey;
        }
    }
};

internal inline r32 GetSortKey(render_group *Group, v3 Pos) {
    r32 InvLengthSqr = 1.0f / LengthSqr(Group->ScreenDim);
    r32 Result = Pos.Z + InvLengthSqr*(LengthSqr(Group->ScreenDim - Pos.XY));
    return Result;
}

internal void
DrawBitmap(bitmap *Buffer, bitmap *Bitmap, s32 XOrigin_, s32 YOrigin_, rect2 Bounds, v4 Color = V4(1, 1, 1, 1))
{
    if(Bitmap) {
        
        // TODO(OLIVER): This needs fixing for clip space
        Bounds = Rect2(Max(Bounds.Min.X, 0), Max(Bounds.Min.Y, 0), Min(Bounds.Max.X, Buffer->Width), Min(Bounds.Max.Y, Buffer->Height));
        ///////
        
        r32 Inv_255 = 1.0f / 255.0f;
        
        s32 XOrigin = XOrigin_ - (s32)(Bitmap->AlignPercent.X*Bitmap->Width);
        s32 YOrigin = YOrigin_ - (s32)(Bitmap->AlignPercent.Y*Bitmap->Height);
        
        s32 YMin = 0;
        s32 XMin = 0;
        s32 XMax = (s32)Bitmap->Width;
        s32 YMax = (s32)Bitmap->Height;
        
        s32 XMaxOffset = (XOrigin + XMax) - (s32)Bounds.MaxX;
        s32 YMaxOffset = (YOrigin + YMax) - (s32)Bounds.MaxY;
        if(XMaxOffset >= 0) XMax = XMax - XMaxOffset;
        if(YMaxOffset >= 0) YMax = YMax - YMaxOffset;
        
        if(XOrigin < (s32)Bounds.MinX)
        {
            XMin = (s32)Abs((r32)XOrigin);
            XOrigin = (s32)Bounds.MinX;
        }
        if(YOrigin < (s32)Bounds.MinY)
        {
            YMin = (s32)Abs((r32)YOrigin);
            YOrigin = (s32)Bounds.MinY;
        }
        
        Assert(XMax <= (s32)Bitmap->Width);
        Assert(YMax <= (s32)Bitmap->Height);
        
        u8 *Dest = (u8 *)Buffer->Bits + YOrigin*Buffer->Pitch + XOrigin*sizeof(u32);
        u8 *Source = (u8 *)Bitmap->Bits;
        
        
        for(s32 Y = YMin;
            Y < YMax;
            ++Y)
        {
            
            u32 *DestAt = (u32 *)Dest;
            u32 *SourceAt = (u32 *)Source;
            
            for(s32 X = XMin;
                X < XMax;
                ++X)
            {
                u32 SC = *SourceAt;
                v4 S;
                S.R = (r32)((SC >> 16) & 0xFF);
                S.G = (r32)((SC >> 8) & 0xFF);
                S.B = (r32)((SC >> 0) & 0xFF);
                S.A = (r32)((SC >> 24) & 0xFF);
                
                S = Hadamard(Color, S);
                
                u32 DC = *DestAt;
                v4 D;
                D.R = (r32)((DC >> 16) & 0xFF);
                D.G = (r32)((DC >> 8) & 0xFF);
                D.B = (r32)((DC >> 0) & 0xFF);
                D.A = (r32)((DC >> 24) & 0xFF);
                
                r32 NormalAlpha = Inv_255 * S.A;
                
                r32 BR = D.R + (S.R - D.R)*NormalAlpha;
                r32 BG = D.G + (S.G - D.G)*NormalAlpha;
                r32 BB = D.B + (S.B - D.B)*NormalAlpha;
                r32 BA = D.A + (S.A - D.A)*NormalAlpha;
                
                *DestAt = ((u32)BA << 24) | ((u32)BR << 16) | ((u32)BG << 8) | ((u32)BB << 0);
                //*DestAt = SC; //Blt the source values
                DestAt++;
                SourceAt++;
            }
            
            Dest += Buffer->Pitch;
            Source += Bitmap->Pitch;
        }
    }
}

inline void
LerpToEdge(r32 *Start, r32 *End, r32 Edge)
{
    r32 StartX_ = *Start;
    r32 EndX_ = *End;
    
    if(StartX_ < Edge && EndX_ >= Edge)
    {
        r32 t = 0.0f;
        t = (Edge - StartX_) / (EndX_ - StartX_);
        *Start += t*(EndX_ - StartX_);
    }
}

internal b32
DrawLine(bitmap *Buffer, r32 StartX_, r32 StartY_, r32  EndX_, r32 EndY_, u32 Color)
{
    
    b32 Result = false;
    
    //TODO(ollie): Line Clipping for line -> something to think about
    // SetBitmapValueSafe() handles out of bounds at the moment
#if 0
    LerpToEdge(&StartX_, &EndX_, 0.0f);
    LerpToEdge(&EndX_, &StartX_, 0.0f);
    LerpToEdge(&StartX_, &EndX_, (r32)Buffer->Width);
    LerpToEdge(&EndX_, &StartX_, (r32)Buffer->Width);
    
    LerpToEdge(&StartY_, &EndY_, 0.0f);
    LerpToEdge(&EndY_, &StartY_, 0.0f);
    LerpToEdge(&StartY_, &EndY_, (r32)Buffer->Height);
    LerpToEdge(&EndY_, &StartY_, (r32)Buffer->Height);
    
    StartX_ = Max(StartX_, 0);
    StartX_ = Min(StartX_, Buffer->Width);
    
    EndX_ = Max(EndX_, 0);
    EndX_ = Min(EndX_, Buffer->Width);
    
    StartY_ = Max(StartY_, 0);
    StartY_ = Min(StartY_, Buffer->Height);
    
    EndY_ = Max(EndY_, 0);
    EndY_ = Min(EndY_, Buffer->Height);    
    b32 DrawLine = true;
    
    if(StartY_ < 0.0f)
    {
        Assert(EndY_ < 0.0f);
        DrawLine = false;
    }
    if(StartX_ < 0.0f)
    {
        Assert(EndX_ < 0.0f);
        DrawLine = false;
    }
    if(StartY_ >= Buffer->Height)
    {
        Assert(EndY_ >= Buffer->Height);
        DrawLine = false;
    }
    if(StartX_ >= Buffer->Width)
    {
        Assert(EndX_ >= Buffer->Width);
        DrawLine = false;
    }
#endif
    
    v2 Start = V2(StartX_, StartY_);
    v2 End = V2(EndX_, EndY_);
    if(End.X < Start.X)
    {
        v2 Temp = Start;
        Start = End;
        End = Temp;
    }
    
    v2 UpIncrement = V2(0, 1.0f);
    if(End.Y < Start.Y)
    {
        UpIncrement = V2(0, -1.0f);
    }
    
    v2 NormalLine = Normal(End - Start);
    
    r32 LineLength = Length(End - Start);
    
    for(v2 Pos = Start;
        Inner(NormalLine, Pos - Start) < LineLength;
        )
    {
        Result = false;
        SetBitmapValueSafe(Buffer, (u32)Pos.X, (u32)Pos.Y, Color);
        
        v2 OptionRight = Pos + V2(1.0f, 0);
        v2 OptionUp = Pos + UpIncrement;
        
        r32 RightScore = Abs(Inner(Perp(NormalLine), OptionRight - Start));
        r32 UpScore = Abs(Inner(Perp(NormalLine), OptionUp - Start));
        
        if(RightScore < UpScore)
        {
            Pos = OptionRight;
        }
        else
        {
            Pos = OptionUp;
        }
    }
    
    SetBitmapValueSafe(Buffer, (u32)End.X, (u32)End.Y, Color);
    
    return Result;
}

internal void
DrawRectangle(bitmap *Buffer, rect2 Rect, v4 Color01)
{
    u32 Color255 = Color01AsV4To255AsU32(Color01);
    
    DrawLine(Buffer, GetMinCorner(Rect).X, GetMinCorner(Rect).Y,
             GetMinCorner(Rect).X, GetMaxCorner(Rect).Y, Color255);
    
    DrawLine(Buffer, GetMinCorner(Rect).X, GetMinCorner(Rect).Y,
             GetMaxCorner(Rect).X, GetMinCorner(Rect).Y, Color255);
    
    DrawLine(Buffer, GetMinCorner(Rect).X, GetMaxCorner(Rect).Y,
             GetMaxCorner(Rect).X, GetMaxCorner(Rect).Y, Color255);
    
    DrawLine(Buffer, GetMaxCorner(Rect).X, GetMinCorner(Rect).Y,
             GetMaxCorner(Rect).X, GetMaxCorner(Rect).Y, Color255);
}

inline v2 Scale(transform *Transform, v2 Pos) {
    v2 Result = Hadamard(Transform->Scale, Pos);
    return Result;
}

inline v2 Transform(transform *Transform, v2 Pos) {
    v2 Result = Hadamard(Transform->Scale, Pos);
    Result += Transform->Offset;
    return Result;
}

inline rect2 Transform(transform *TransformPtr, rect2 Dim) {
    rect2 Result = Rect2MinMax(Transform(TransformPtr, Dim.Min), Transform(TransformPtr, Dim.Max)); 
    return Result;
}

inline v2 InverseTransform(transform *Transform, v2 Pos) {
    
    v2 Result = Pos - Transform->Offset;
    Result = Hadamard(Result, V2(1.0f / Transform->Scale.X, 1.0f / Transform->Scale.Y));
    
    return Result;
} 

inline render_element *PushRenderElm(render_group *Group, render_element_type Type) {
    render_element *Elm = (render_element *)PushSize(&Group->Arena, sizeof(render_element));
    Elm->Type = Type;
    Group->ElmCount++;
    return Elm;
}

void PushClear(render_group *Group, v4 Color) {
    render_element *Elm = PushRenderElm(Group, render_clear);
    Elm->Color = Color;
}

void PushRectOutline(render_group *Group, rect2 Dim, r32 ZDepth, v4 Color = V4(0, 0, 0, 1)) {
    render_element *Elm = PushRenderElm(Group, render_rect_outline);
    
    Elm->ZDepth = ZDepth;
    Elm->Dim = Transform(&Group->Transform, Dim);
    Elm->Color = Color;
    Elm->SortKey = ZDepth;
}
inline void PushRectCenterOutline(render_group *Group, rect2 Dim, r32 ZDepth, v4 Color = V4(0, 0, 0, 1)) {
    
    v2 WHDim = V2(GetWidth(Dim), GetHeight(Dim));
    rect2 NewDim = Rect2CenterDim(Dim.Min, WHDim);
    PushRectOutline(Group, NewDim, ZDepth, Color);
    
}

void PushBitmap(render_group *Group, v3 Pos, bitmap *Bitmap, r32 WidthInWorldSpace,  rect2 ClipRect, r32 SortKey, v4 Color = V4(1, 1, 1, 1), r32 Angle = 0.0f, r32 SkewFactor = 0.0f, v2 ScaleFactor = V2(1, 1)) {
    
    render_element *Info = PushRenderElm(Group, render_bitmap);
    
    
    Info->Bitmap = Bitmap;
    Info->ClipRect = ClipRect;
    Info->Angle = Angle;
    Info->SkewFactor = SkewFactor; // This is between 0 & 1
    Info->Scale = ScaleFactor;
    Info->Color = Color;
    r32 WidthToHeightRatio = SafeRatio0((r32)Bitmap->Height, (r32)Bitmap->Width); 
    r32 HeightInWorldSpace = WidthToHeightRatio*WidthInWorldSpace;
    v2 Dim = Scale(&Group->Transform, V2(WidthInWorldSpace, HeightInWorldSpace));
    v2 Offset = Hadamard(Bitmap->AlignPercent, Dim);
    Info->Dim = Rect2MinMax(-Offset, Dim - Offset);
    Info->Pos = Transform(&Group->Transform, Pos.XY);
    Info->ZDepth = Pos.Z;
    Info->SortKey = GetSortKey(Group, ToV3(Info->Pos, Pos.Z));
}

void PushBitmapScale(render_group *Group, v3 Pos, bitmap *Bitmap, r32 Scale,  rect2 ClipRect,r32 SortKey,  v4 Color = V4(1, 1, 1, 1)) {
    
    PushBitmap(Group, Pos, Bitmap, Scale*Bitmap->Width, ClipRect, SortKey, Color);
}

void PushRect(render_group *Group, rect2 Dim, r32 ZDepth, v4 Color) {
    render_element *Info = PushRenderElm(Group, render_rect);
    
    Info->SortKey = Info->ZDepth = ZDepth;
    Info->Dim = Transform(&Group->Transform, Dim);
    Info->Color = Color;
}

void PushRectCenter(render_group *Group, rect2 Dim, r32 ZDepth, v4 Color) {
    v2 WHDim = V2(GetWidth(Dim), GetHeight(Dim));
    rect2 NewDim = Rect2CenterDim(Dim.Min, WHDim);
    PushRect(Group, NewDim, ZDepth, Color);
}


internal void InitRenderGroup(render_group *Group, bitmap *Buffer,memory_arena *MemoryArena, memory_size MemorySize, v2 Scale, v2 Offset, v2 Rotation) {
    Group->ScreenDim = V2i(Buffer->Width, Buffer->Height);
    Group->Buffer = Buffer;
    Group->TempMem = MarkMemory(MemoryArena);
    Group->Arena = SubMemoryArena(MemoryArena, MemorySize);
    Group->Transform.Scale = Scale; 
    Group->Transform.Offset = Offset;
    Group->Transform.Rotation = Rotation;
    Group->ElmCount = 0;
    Group->IsInitialized = true;
}

internal void EndRenderGroup(render_group *Group) {
    ReleaseMemory(&Group->TempMem);
    Group->IsInitialized = false;
    
}

void RenderGroupToOutput(render_group *Group) {
    u8 *Base = (u8 *)Group->Arena.Base;
    u8 *At = Base;
    
    rect2 BufferRect = Rect2(0, 0, Group->ScreenDim.X, Group->ScreenDim.Y);
    while((s32)(At - Base) < Group->Arena.CurrentSize) {
        render_element_header *Header = (render_element_header *)At;
        switch(Header->Type) {
            case render_clear: {
                render_element_clear *Info = (render_element_clear *)(Header + 1);
                
                ClearBitmap(Group->Buffer, Info->Color);
                
                At += sizeof(render_element_header) + sizeof(render_element_clear);
            } break;
            case render_bitmap: {
                render_element_bitmap *Info = (render_element_bitmap *)(Header + 1);
                
                DrawBitmap(Group->Buffer, Info->Bitmap, (s32)Info->Pos.X, (s32)Info->Pos.Y, Info->ClipRect, Info->Color);
                
                At += sizeof(render_element_header) + sizeof(render_element_bitmap);
            } break;
            case render_rect: {
                render_element_rect *Info = (render_element_rect *)(Header + 1);
                
                FillRectangle(Group->Buffer, Info->Dim, Info->Color);
                
                At += sizeof(render_element_header) + sizeof(render_element_rect);
            } break;
            case render_rect_outline: {
                render_element_rect_outline *Info = (render_element_rect_outline *)(Header + 1);
                
                DrawRectangle(Group->Buffer, Info->Dim, Info->Color);
                
                At += sizeof(render_element_header) + sizeof(render_element_rect_outline);
            } break;
            default: {
                InvalidCodePath;
            }
        }
        Assert((At - Base) >= 0);
    }
    
    EndRenderGroup(Group);
}
