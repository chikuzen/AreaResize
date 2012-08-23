/*
    AreaResize.dll

    Copyright (C) 2012 Oka Motofumi(chikuzen.mo at gmail dot com)

    author : Oka Motofumi

    Permission to use, copy, modify, and/or distribute this software for any
    purpose with or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <windows.h>
#include "avisynth.h"

struct params {
    int src_width;
    int src_height;
    int target_width;
    int target_height;
    int num_h;
    int den_h;
    int num_v;
    int den_v;
};

typedef struct {
    int blue;
    int green;
    int red;
} i_rgb24_t;

typedef struct {
    BYTE blue;
    BYTE green;
    BYTE red;
} rgb24_t;

class AreaResize : public GenericVideoFilter {

    static const int num_plane = 3;
    struct params params[num_plane];

    BYTE* buff;

    int gcd(int x, int y)
    {
        int m = x % y;
        return m == 0 ? y : gcd(y, m);
    }

    PVideoFrame GetPlanarFrame(PVideoFrame src, int n, IScriptEnvironment* env);
    PVideoFrame GetPackedFrame(PVideoFrame src, int n, IScriptEnvironment* env);
     bool ResizeHorizontal(const BYTE* srcp, int src_pitch, int plane);
     bool ResizeVertical(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int plane);
    bool ResizeHorizontalRGB(const BYTE* srcp, int src_pitch);
    bool ResizeVerticalRGB(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch);

public:
    AreaResize(PClip _child, int target_width, int target_height, IScriptEnvironment* env);
    ~AreaResize();
    PVideoFrame _stdcall GetFrame(int n, IScriptEnvironment* env);
};

AreaResize::AreaResize(PClip _child, int target_width, int target_height, IScriptEnvironment* env) : GenericVideoFilter(_child)
{
    buff = NULL;
    if (target_width != vi.width) {
        buff = (BYTE *)malloc(target_width * vi.height * (vi.IsRGB24() ? 3 :1));
        if (!buff) {
            env->ThrowError("AreaResize: out of memory");
        }
    }

    for (int i = 0; i < num_plane; i++) {
        params[i].src_width     = i ? vi.width / vi.SubsampleH() : vi.width;
        params[i].src_height    = i ? vi.height / vi.SubsampleV() : vi.height;
        params[i].target_width  = i ? target_width / vi.SubsampleH() : target_width;
        params[i].target_height = i ? target_height / vi.SubsampleV() : target_height;
    }

    vi.width = target_width;
    vi.height = target_height;

    for (int i = 0; i < num_plane; i++) {
        int gcd_h = gcd(params[i].src_width, params[i].target_width);
        int gcd_v = gcd(params[i].src_height, params[i].target_height);
        params[i].num_h = params[i].target_width / gcd_h;
        params[i].den_h = params[i].src_width / gcd_h;
        params[i].num_v = params[i].target_height / gcd_v;
        params[i].den_v = params[i].src_height / gcd_v;
    }
}

AreaResize::~AreaResize()
{
    if (buff) {
        free(buff);
    }
}

PVideoFrame AreaResize::GetPlanarFrame(PVideoFrame src, int n, IScriptEnvironment* env)
{
    PVideoFrame dst = env->NewVideoFrame(vi);

    int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};
    for (int i = 0, time = vi.IsY8() ? 1 : num_plane; i < time; i++) {

        const BYTE* srcp = src->GetReadPtr(plane[i]);
        int src_pitch = src->GetPitch(plane[i]);

        const BYTE* resized_h;
        if (params[i].src_width == params[i].target_width) {
            resized_h = srcp;
        } else {
            if (!ResizeHorizontal(srcp, src_pitch, i)) {
                env->ThrowError("AreaResize: out of memory");
            }
            resized_h = buff;
            src_pitch = params[i].target_width;
        }

        BYTE* dstp = dst->GetWritePtr(plane[i]);
        int dst_pitch = dst->GetPitch(plane[i]);
        if (params[i].src_height == params[i].target_height) {
            env->BitBlt(dstp, dst_pitch, resized_h, src_pitch, params[i].target_width, params[i].target_height);
            continue;
        }

        if (!ResizeVertical(dstp, dst_pitch, resized_h, src_pitch, i)) {
            env->ThrowError("AreaResize: out of memory");
        }
    }
    
    return dst;
}

PVideoFrame AreaResize::GetPackedFrame(PVideoFrame src, int n, IScriptEnvironment* env)
{
    PVideoFrame dst = env->NewVideoFrame(vi);
    const BYTE* srcp = src->GetReadPtr();
    int src_pitch = src->GetPitch();

    const BYTE* resized_h;
    if (params[0].src_width == params[0].target_width) {
        resized_h = srcp;
    } else {
        if (!ResizeHorizontalRGB(srcp, src_pitch)) {
            env->ThrowError("AreaResize: out of memory");
        }
        resized_h = buff;
        src_pitch = params[0].target_width;
    }

    BYTE* dstp = dst->GetWritePtr();
    int dst_pitch = dst->GetPitch();
    if (params[0].src_height == params[0].target_height) {
        env->BitBlt(dstp, dst_pitch, resized_h, src_pitch, params[0].target_width, params[0].target_height);
    } else if (!ResizeVerticalRGB(dstp, dst_pitch, resized_h, src_pitch)) {
        env->ThrowError("AreaResize: out of memory");
    }

    return dst;
}

PVideoFrame __stdcall AreaResize::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src = child->GetFrame(n, env);
    if (params[0].src_width == params[0].target_width &&
        params[0].src_height == params[0].target_height) {
        return src;
    }

    if (vi.IsPlanar()) {
        return GetPlanarFrame(src, n, env);
    }
    return GetPackedFrame(src, n, env);
}

bool AreaResize::ResizeHorizontal(const BYTE* srcp, int src_pitch, int plane)
{
    BYTE* buff = this->buff;
    int src_height = params[plane].src_height;
    int target_width = params[plane].target_width;
    int target_height = params[plane].target_height;
    int num = params[plane].num_h;
    int den = params[plane].den_h;
    int* value = (int *)malloc(sizeof(int) * target_width);
    if (!value) {
        return false;
    }

    for (int y = 0; y < src_height; y++) {
        int index_src = 0;
        int index_dst = 0;
        int count_num = 0;
        int count_den = 0;
        while (index_dst < target_width) {
            value[index_dst] = 0;
            while (1) {
                value[index_dst] += srcp[index_src];
                if (++count_num == num) {
                    count_num = 0;
                    index_src++;
                }
                if (++count_den == den) {
                    count_den = 0;
                    index_dst++;
                    break;
                }
            }
        }

        for (int i = 0; i < target_width; i++) {
            buff[i] = (BYTE)(value[i] / den);
        }
        srcp += src_pitch;
        buff += target_width;
    }
    free(value);
    return true;
}

bool AreaResize::ResizeVertical(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int plane)
{
    int src_width = params[plane].target_width;
    int target_height = params[plane].target_height;
    int num = params[plane].num_v;
    int den = params[plane].den_v;
    int* value = (int *)malloc(sizeof(int) * target_height);
    if (!value) {
        return false;
    }

    for (int x = 0; x < src_width; x++) {
        int index_src = 0;
        int index_dst = 0;
        int count_num = 0;
        int count_den = 0;
        while (index_dst < target_height) {
            value[index_dst] = 0;
            while(1) {
                value[index_dst] += srcp[index_src];
                if (++count_num == num) {
                    count_num = 0;
                    index_src += src_pitch;
                }
                if (++count_den == den) {
                    count_den = 0;
                    index_dst++;
                    break;
                }
            }
        }
        for (int i = 0; i < target_height; i++) {
            dstp[i * dst_pitch] = (BYTE)(value[i] / den);
        }
        srcp++;
        dstp++;
    }
    free(value);
    return true;
}

bool AreaResize::ResizeHorizontalRGB(const BYTE* srcp, int src_pitch)
{
    rgb24_t* buff = reinterpret_cast<rgb24_t*>(this->buff);
    int src_height = params[0].src_height;
    int target_width = params[0].target_width;
    int num = params[0].num_h;
    int den = params[0].den_h;
    i_rgb24_t* value = (i_rgb24_t*)malloc(sizeof(i_rgb24_t) * target_width);
    if (!value) {
        return false;
    }

    for (int y = 0; y < src_height; y++) {
        int index_src = 0;
        int index_dst = 0;
        int count_num = 0;
        int count_den = 0;
        const rgb24_t* rgbp = reinterpret_cast<rgb24_t*>(const_cast<BYTE*>(srcp));
        while (index_dst < target_width) {
            value[index_dst].blue = 0;
            value[index_dst].green = 0;
            value[index_dst].red = 0;
            while (1) {
                value[index_dst].blue += rgbp[index_src].blue;
                value[index_dst].green += rgbp[index_src].green;
                value[index_dst].red += rgbp[index_src].red;
                if (++count_num == num) {
                    count_num = 0;
                    index_src++;
                }
                if (++count_den == den) {
                    count_den = 0;
                    index_dst++;
                    break;
                }
            }
        }
        for (int i = 0; i < target_width; i++) {
            buff[i].blue = (BYTE)(value[i].blue / den);
            buff[i].green = (BYTE)(value[i].green / den);
            buff[i].red = (BYTE)(value[i].red / den);
        }
        srcp += src_pitch;
        buff += target_width;
    }
    return true;
}

bool AreaResize::ResizeVerticalRGB(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch)
{
    int src_width = params[0].target_width;
    int target_height = params[0].target_height;
    int num = params[0].num_v;
    int den = params[0].den_v;
    const rgb24_t* src_rgbp = reinterpret_cast<rgb24_t*>(const_cast<BYTE*>(srcp));
    i_rgb24_t* value = (i_rgb24_t *)malloc(sizeof(i_rgb24_t) * target_height);
    if (!value) {
        return false;
    }

    for (int x = 0; x < src_width; x++) {
        int index_src = 0;
        int index_dst = 0;
        int count_num = 0;
        int count_den = 0;
        while (index_dst < target_height) {
            value[index_dst].blue = 0;
            value[index_dst].green = 0;
            value[index_dst].red = 0;
            while(1) {
                value[index_dst].blue += src_rgbp[index_src].blue;
                value[index_dst].green += src_rgbp[index_src].green;
                value[index_dst].red += src_rgbp[index_src].red;
                if (++count_num == num) {
                    count_num = 0;
                    index_src += src_pitch;
                }
                if (++count_den == den) {
                    count_den = 0;
                    index_dst++;
                    break;
                }
            }
        }
        for (int i = 0; i < target_height; i++) {
            dstp[i * dst_pitch] = (BYTE)(value[i].blue / den);
            dstp[i * dst_pitch + 1] = (BYTE)(value[i].green / den);
            dstp[i * dst_pitch + 2] = (BYTE)(value[i].red / den);
        }
        src_rgbp++;
        dstp += 3;
    }
    free(value);
    return true;
}

AVSValue __cdecl CreateAreaResize(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip clip = args[0].AsClip();
    int target_width = args[1].AsInt();
    int target_height = args[2].AsInt();

    if (target_width < 1 || target_height < 1) {
        env->ThrowError("target width/height must be 1 or higher.");
    }

    const VideoInfo& vi = clip->GetVideoInfo();
    if (vi.IsYV411() && target_width & 3) {
        env->ThrowError("AreaResize: Target width requires mod 4.");
    }
    if ((vi.IsYV16() || vi.IsYV12()) && target_width & 1) {
        env->ThrowError("AreaResize: Target width requires mod 2.");
    }
    if (vi.IsYV12() && target_height & 1) {
        env->ThrowError("AreaResize: Target height requires mod 2.");
    }
    if (vi.width < target_width || vi.height < target_height) {
        env->ThrowError("AreaResize: This filter is only for down scale.");
    }

    return new AreaResize(clip, target_width, target_height, env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("AreaResize", "cii", CreateAreaResize, 0);
    return "AreaResize for AviSynth 0.1.0";
}
