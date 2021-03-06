#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_SRGB8_ALPHA8 0x8C43

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define ERROR_INVALID_VERSION_ARB               0x2095
#define ERROR_INVALID_PROFILE_ARB               0x2096

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

struct opengl_info {
    b32 ModernContext;
    char *Vendor;
    char *Renderer;
    char *Version;
    char *ShandingLanguageVersion; 
    char *Extensions;
    
    b32 GL_ARB_framebuffer_sRGB;
    b32 GL_EXT_texture_sRGB;
};

inline opengl_info OpenGlGetExtensions(b32 ModernContext) {
    opengl_info Result = {};
    
    Result.ModernContext = ModernContext;
    Result.Vendor = (char *)glGetString(GL_VENDOR);
    Result.Renderer = (char *)glGetString(GL_RENDERER);
    Result.Version = (char *)glGetString(GL_VERSION);
    if(Result.ModernContext) {
        Result.ShandingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    } else {
        Result.ShandingLanguageVersion = "(none)";
    }
    
    Result.Extensions = (char *)glGetString(GL_EXTENSIONS);
    
    char *At = Result.Extensions;
    while(*At) {
        char *Begin = At;
        Assert(!IsWhiteSpace(*At));
        while(!IsWhiteSpace(*At)) {
            At++;
        }
        s32 Length = (s32)((intptr)At - (intptr)Begin);
        if(0) {}
        else if(DoStringsMatch("GL_ARB_framebuffer_sRGB", Begin, Length)) { Result.GL_ARB_framebuffer_sRGB = true; }
        else if(DoStringsMatch("GL_EXT_texture_sRGB", Begin, Length)) { Result.GL_EXT_texture_sRGB = true; }
        
        while(IsWhiteSpace(*At)) {
            At++;
        }
        
    }
    
    return Result;
}

internal void OpenGlInit(b32 ModernContext) {
    opengl_info OpenGlInfo = OpenGlGetExtensions(ModernContext);
    
    GlobalOpenGlDefaultInternalTextureFormat = GL_RGBA8;
    if(OpenGlInfo.GL_EXT_texture_sRGB) {
        //GlobalOpenGlDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
    }
    
    if(OpenGlInfo.GL_ARB_framebuffer_sRGB) 
    {
        glEnable(GL_FRAMEBUFFER_SRGB);
        
    }
    
}

internal void
OpenGlDrawRectangle(v2 MinP, v2 MaxP, v4 Color) {
    
    glColor4f(Color.R, Color.G, Color.B, Color.A);
    
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0, 0);
    glVertex2f(MinP.X, MinP.Y);
    glTexCoord2f(0, 1);
    glVertex2f(MinP.X, MaxP.Y);
    glTexCoord2f(1, 1);
    glVertex2f(MaxP.X, MaxP.Y);
    
    glTexCoord2f(0, 0);
    glVertex2f(MinP.X, MinP.Y);
    glTexCoord2f(1, 0);
    glVertex2f(MaxP.X, MinP.Y);
    glTexCoord2f(1, 1);
    glVertex2f(MaxP.X, MaxP.Y);
    glEnd();
    
}

internal void
OpenGlDrawRectangleOutline(v2 MinP, v2 MaxP, v4 Color, r32 Thickness = 5.0f) {
    OpenGlDrawRectangle(MinP, V2(MinP.X + Thickness, MaxP.Y), Color);
    OpenGlDrawRectangle(MinP, V2(MaxP.X, MinP.Y + Thickness), Color);
    OpenGlDrawRectangle(V2(MinP.X, MaxP.Y - Thickness), MaxP, Color);
    OpenGlDrawRectangle(V2(MaxP.X - Thickness, MinP.Y), MaxP, Color);
    
}

inline void SetModelMatrix(r32 Angle, v2 Translation, r32 SkewFactor, v2 ScaleFactor) {
    glMatrixMode(GL_MODELVIEW); 
    r32 SkewAngle = SkewFactor*(PI32 / 4);
    r32 aX = ScaleFactor.X*Cos(Angle + SkewAngle); 
    r32 bX = ScaleFactor.X*Sin(Angle + SkewAngle);
    r32 aY = ScaleFactor.Y*-Sin(Angle - SkewAngle);
    r32 bY = ScaleFactor.Y*Cos(Angle - SkewAngle); 
    r32 ModelMat[] = {
        aX,  bX,  0,  0,
        aY,  bY,  0,  0,
        0,  0,  1,  0,
        Translation.X, Translation.Y, 0,  1
    };
    glLoadMatrixf(ModelMat);
}


inline void OpenGlSetScreenSpace(s32 Width, s32 Height) {
    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION); 
    
    r32 a = SafeRatio1(2.0f, (r32)Width); 
    r32 b = SafeRatio1(2.0f, (r32)Height);
    r32 ProjMat[] = {
        a,  0,  0,  0,
        0,  b,  0,  0,
        0,  0,  1,  0,
        -1, -1, 0,  1
    };
    
    glLoadMatrixf(ProjMat);
}

global_variable u32 TextureHandleIndex = 0;


void OpenGlRenderToOutput(render_group *Group, b32 draw) {
    if(draw) { // NOTE(Oliver): For Debug purposes
        
        SortRenderGroup(Group);
        
        glViewport(0, 0, (s32)Group->ScreenDim.X, (s32)Group->ScreenDim.Y);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); //This is our alpha blend function
        
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        
        s32 Width = (s32)Group->ScreenDim.X;
        s32 Height = (s32)Group->ScreenDim.Y;
        
        OpenGlSetScreenSpace(Width, Height);
        
        u8 *Base = (u8 *)Group->Arena.Base;
        u8 *At = Base;
        render_element *Elms = (render_element *)Group->Arena.Base;
        rect2 BufferRect = Rect2(0, 0, (r32)Width, (r32)Height);
        //while((s32)(At - Base) < Group->Arena.CurrentSize) {
        for(u32 i = 0; i < Group->ElmCount; ++i) {
            
            render_element *Info = Elms + Group->SortedIndexes[i];
            
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW); 
            glLoadIdentity();
            
            glMatrixMode(GL_PROJECTION); 
            r32 ProjMat[] = {
                1,  0,  Info->ZDepth,
                0,  1,  Info->ZDepth, 
                0,  0,  1,  0,
                0, 0, 0,  1
            };
            
            glLoadMatrixf(ProjMat);
            
            switch(Info->Type) {
                case render_clear: {
                    
                    glDisable(GL_TEXTURE_2D);
                    glClearColor(Info->Color.R, Info->Color.G, Info->Color.B, 0);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glEnable(GL_TEXTURE_2D);
                    
                } break;
                case render_bitmap: {
                    
                    bitmap *Bitmap = Info->Bitmap;
                    
                    SetModelMatrix(Info->Angle, Info->Pos, Info->SkewFactor, Info->Scale);
                    
                    if(Bitmap->Handle) {
                        glBindTexture(GL_TEXTURE_2D, Bitmap->Handle); 
                    } else {
                        Bitmap->Handle = ++TextureHandleIndex;
                        
                        glBindTexture(GL_TEXTURE_2D, Bitmap->Handle); 
                        
                        glTexImage2D(GL_TEXTURE_2D, 0, GlobalOpenGlDefaultInternalTextureFormat, Bitmap->Width, Bitmap->Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, Bitmap->Bits);
                        
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); 
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 
                        
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 
                        
                    }
                    
                    rect2 Dim = Info->Dim;
                    OpenGlDrawRectangle(Dim.Min, Dim.Max, Info->Color);
                    
                } break;
                case render_rect: {
                    
                    v2 MinP = Info->Dim.Min;
                    v2 MaxP = MinP + V2(Info->Dim.MaxX - Info->Dim.MinX, Info->Dim.MaxY - Info->Dim.MinY);
                    
                    glDisable(GL_TEXTURE_2D);
                    OpenGlDrawRectangle(MinP, MaxP, Info->Color);
                    glEnable(GL_TEXTURE_2D);
                    
                } break;
                case render_rect_outline: {
                    
                    v2 MinP = Info->Dim.Min;
                    v2 MaxP = MinP + V2(Info->Dim.MaxX - Info->Dim.MinX, Info->Dim.MaxY - Info->Dim.MinY);
                    
                    glDisable(GL_TEXTURE_2D);
                    OpenGlDrawRectangleOutline(MinP, MaxP, Info->Color);
                    glEnable(GL_TEXTURE_2D);
                    
                } break;
                default: {
                    InvalidCodePath;
                }
            }
            Assert((At - Base) >= 0);
        }
    }
    ReleaseMemory(&Group->TempMem);
}
