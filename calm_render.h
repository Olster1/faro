#if !defined(CALM_RENDER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

enum render_element_type
{
    render_clear,
    render_bitmap,
    render_triangle,
    render_rect,
    render_rect_outline,
};

struct render_element {
    render_element_type Type;
    
    v2 Pos;
    bitmap *Bitmap;
    rect2 ClipRect;
    v4 Color;
    r32 ZDepth;
    rect2 Dim;
    r32 Angle;
    r32 SkewFactor;
    v2 Scale;
    r32 SortKey;
};

struct render_element_header
{
    render_element_type Type;
    
};

struct render_element_clear
{
    v4 Color;
};

struct render_element_bitmap
{
    v2 Pos;
    bitmap *Bitmap;
    rect2 ClipRect;
    v4 Color;
    r32 ZDepth;
    rect2 Dim;
    r32 Angle;
    r32 SkewFactor;
    v2 Scale;
    r32 SortKey;
};

struct render_element_rect
{
    rect2 Dim;
    r32 ZDepth;
    v4 Color;
    r32 SortKey;
};

struct render_element_rect_outline
{
    rect2 Dim;
    v4 Color;
    r32 ZDepth;
    r32 SortKey;
};


struct transform {
    v2 Scale; 
    v2 Rotation;
    v2 Offset;
};

struct render_group
{
    v2 ScreenDim;
    
    transform Transform;
    
    bitmap *Buffer;
    
    temp_memory TempMem;
    memory_arena Arena;
    u32 ElmCount;
    u32 *SortedIndexes;
    
    b32 IsInitialized;
};

void PushBitmapScale(render_group *Group, v3 Pos, bitmap *Bitmap, r32 Scale, rect2 ClipRect, r32 SortKey, v4 Color);

void SortRenderGroup(render_group *Group);

inline v2 Transform(transform *Transform, v2 Pos);
inline rect2 Transform(transform *TransformPtr, rect2 Dim);
v2 InverseTransform(transform *Transform, v2 Pos);

#define M(Value, Index) (((real32 *)&Value)[Index])
#define Mi(Value, Index) (((int32 *)&Value)[Index])

#define CALM_RENDER_H
#endif
