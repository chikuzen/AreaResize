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

typedef struct {
    int src_width;
    int src_height;
    int target_width;
    int target_height;
    int num_h;
    int den_h;
    int num_v;
    int den_v;
} params_t;

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

typedef struct {
    int blue;
    int green;
    int red;
    int alpha;
} i_rgb32_t;

typedef struct {
    BYTE blue;
    BYTE green;
    BYTE red;
    BYTE alpha;
} rgb32_t;

static bool ResizeHorizontalPlanar(BYTE* dstp, const BYTE* srcp, int src_pitch, params_t* params)
{
    int src_height = params->src_height;
    int target_width = params->target_width;
    int target_height = params->target_height;
    int num = params->num_h;
    int den = params->den_h;
    int* value = (int *)malloc(sizeof(int) * target_width);
    if (!value) {
        return false;
    }

    for (int y = 0, count_num = 0; y < src_height; y++) {
        int index_src = 0;
        for (int index_value = 0; index_value < target_width; index_value++) {
            value[index_value] = 0;
            for (int count_den = 0; count_den < den; count_den++) {
                value[index_value] += srcp[index_src];
                if (++count_num == num) {
                    count_num = 0;
                    index_src++;
                }
            }
        }

        for (int i = 0; i < target_width; i++) {
            dstp[i] = (BYTE)(value[i] / den);
        }
        srcp += src_pitch;
        dstp += target_width;
    }
    free(value);
    return true;
}

static bool ResizeVerticalPlanar(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, params_t* params)
{
    int src_width = params->target_width;
    int target_height = params->target_height;
    int num = params->num_v;
    int den = params->den_v;
    int* value = (int *)malloc(sizeof(int) * target_height);
    if (!value) {
        return false;
    }

    for (int x = 0, count_num = 0; x < src_width; x++) {
        int index_src = 0;
        for (int index_value = 0; index_value < target_height; index_value++) {
            value[index_value] = 0;
            for (int count_den = 0; count_den < den; count_den++) {
                value[index_value] += srcp[index_src];
                if (++count_num == num) {
                    count_num = 0;
                    index_src += src_pitch;
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

static bool ResizeHorizontalRGB32(BYTE* dstp, const BYTE* srcp, int src_pitch, params_t* params)
{
    rgb32_t* buff = reinterpret_cast<rgb32_t*>(dstp);
    int src_height = params->src_height;
    int target_width = params->target_width;
    int num = params->num_h;
    int den = params->den_h;
    i_rgb32_t* value = (i_rgb32_t*)malloc(sizeof(i_rgb32_t) * target_width);
    if (!value) {
        return false;
    }

    for (int y = 0, count_num = 0; y < src_height; y++) {
        int index_src = 0;
        const rgb32_t* rgbp = reinterpret_cast<rgb32_t*>(const_cast<BYTE*>(srcp));
        for (int index_value = 0; index_value < target_width; index_value++) {
            value[index_value].blue = 0;
            value[index_value].green = 0;
            value[index_value].red = 0;
            value[index_value].alpha = 0;
            for (int count_den = 0; count_den < den; count_den++) {
                value[index_value].blue += rgbp[index_src].blue;
                value[index_value].green += rgbp[index_src].green;
                value[index_value].red += rgbp[index_src].red;
                value[index_value].alpha += rgbp[index_src].alpha;
                if (++count_num == num) {
                    count_num = 0;
                    index_src++;
                }
            }
        }
        for (int i = 0; i < target_width; i++) {
            buff[i].blue = (BYTE)(value[i].blue / den);
            buff[i].green = (BYTE)(value[i].green / den);
            buff[i].red = (BYTE)(value[i].red / den);
            buff[i].alpha = (BYTE)(value[i].alpha / den);
        }
        srcp += src_pitch;
        buff += target_width;
    }
    free(value);
    return true;
}

static bool ResizeVerticalRGB32(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, params_t* params)
{
    int src_width = params->target_width;
    int const target_height = params->target_height;
    int num = params->num_v;
    int den = params->den_v;
    i_rgb32_t* value = (i_rgb32_t *)malloc(sizeof(i_rgb32_t) * target_height);
    if (!value) {
        return false;
    }

    for (int x = 0, count_num = 0; x < src_width; x++) {
        int index_src_b = 0;
        int index_src_g = 1;
        int index_src_r = 2;
        int index_src_a = 3;
        for (int index_value = 0; index_value < target_height; index_value++) {
            value[index_value].blue = 0;
            value[index_value].green = 0;
            value[index_value].red = 0;
            value[index_value].alpha = 0;
            for (int count_den = 0; count_den < den; count_den++) {
                value[index_value].blue += srcp[index_src_b];
                value[index_value].green += srcp[index_src_g];
                value[index_value].red += srcp[index_src_r];
                value[index_value].alpha += srcp[index_src_a];
                if (++count_num == num) {
                    count_num = 0;
                    index_src_b += src_pitch;
                    index_src_g += src_pitch;
                    index_src_r += src_pitch;
                    index_src_a += src_pitch;
                }
            }
        }
        for (int i = 0; i < target_height; i++) {
            int index = i * dst_pitch;
            dstp[index++] = (BYTE)(value[i].blue / den);
            dstp[index++] = (BYTE)(value[i].green / den);
            dstp[index++] = (BYTE)(value[i].red / den);
            dstp[index] = (BYTE)(value[i].alpha / den);
        }
        srcp += 4;
        dstp += 4;
    }
    free(value);
    return true;
}

static bool ResizeHorizontalRGB24(BYTE* dstp, const BYTE* srcp, int src_pitch, params_t* params)
{
    rgb24_t* buff = reinterpret_cast<rgb24_t*>(dstp);
    int src_height = params->src_height;
    int target_width = params->target_width;
    int num = params->num_h;
    int den = params->den_h;
    i_rgb24_t* value = (i_rgb24_t*)malloc(sizeof(i_rgb24_t) * target_width);
    if (!value) {
        return false;
    }

    for (int y = 0, count_num = 0; y < src_height; y++) {
        int index_src = 0;
        const rgb24_t* rgbp = reinterpret_cast<rgb24_t*>(const_cast<BYTE*>(srcp));
        for (int index_value = 0; index_value < target_width; index_value++) {
            value[index_value].blue = 0;
            value[index_value].green = 0;
            value[index_value].red = 0;
            for (int count_den = 0; count_den < den; count_den++) {
                value[index_value].blue += rgbp[index_src].blue;
                value[index_value].green += rgbp[index_src].green;
                value[index_value].red += rgbp[index_src].red;
                if (++count_num == num) {
                    count_num = 0;
                    index_src++;
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
    free(value);
    return true;
}

static bool ResizeVerticalRGB24(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, params_t* params)
{
    int src_width = params->target_width;
    int target_height = params->target_height;
    int num = params->num_v;
    int den = params->den_v;
    i_rgb24_t* value = (i_rgb24_t *)malloc(sizeof(i_rgb24_t) * target_height);
    if (!value) {
        return false;
    }

    for (int x = 0, count_num = 0; x < src_width; x++) {
        int index_src_b = 0;
        int index_src_g = 1;
        int index_src_r = 2;
        for (int index_value = 0; index_value < target_height; index_value++) {
            value[index_value].blue = 0;
            value[index_value].green = 0;
            value[index_value].red = 0;
            for (int count_den = 0; count_den < den; count_den++) {
                value[index_value].blue += srcp[index_src_b];
                value[index_value].green += srcp[index_src_g];
                value[index_value].red += srcp[index_src_r];
                if (++count_num == num) {
                    count_num = 0;
                    index_src_b += src_pitch;
                    index_src_g += src_pitch;
                    index_src_r += src_pitch;
                }
            }
        }
        for (int i = 0; i < target_height; i++) {
            int index = i * dst_pitch;
            dstp[index++] = (BYTE)(value[i].blue / den);
            dstp[index++] = (BYTE)(value[i].green / den);
            dstp[index] = (BYTE)(value[i].red / den);
        }
        srcp += 3;
        dstp += 3;
    }
    free(value);
    return true;
}

static int gcd(int x, int y)
{
    int m = x % y;
    return m == 0 ? y : gcd(y, m);
}

class AreaResize : public GenericVideoFilter {

    static const int num_plane = 3;
    params_t params[num_plane];

    BYTE* buff;

    bool (*ResizeHorizontal)(BYTE*, const BYTE*, int, params_t*);
    bool (*ResizeVertical)(BYTE*, int, const BYTE*, int, params_t*);

public:
    AreaResize(PClip _child, int target_width, int target_height, IScriptEnvironment* env);
    ~AreaResize();
    PVideoFrame _stdcall GetFrame(int n, IScriptEnvironment* env);
};

AreaResize::AreaResize(PClip _child, int target_width, int target_height, IScriptEnvironment* env) : GenericVideoFilter(_child)
{
    buff = NULL;
    if (target_width != vi.width) {
        buff = (BYTE *)malloc(target_width * vi.height * (vi.IsRGB32() ? 4: vi.IsRGB24() ? 3 : 1));
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

    if (vi.IsRGB32()) {
        ResizeHorizontal = ResizeHorizontalRGB32;
        ResizeVertical = ResizeVerticalRGB32;
    } else if (vi.IsRGB24()) {
        ResizeHorizontal = ResizeHorizontalRGB24;
        ResizeVertical = ResizeVerticalRGB24;
    } else {
        ResizeHorizontal = ResizeHorizontalPlanar;
        ResizeVertical = ResizeVerticalPlanar;
    }
}

AreaResize::~AreaResize()
{
    if (buff) {
        free(buff);
    }
}

PVideoFrame AreaResize::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src = child->GetFrame(n, env);
    if (params[0].src_width == params[0].target_width &&
        params[0].src_height == params[0].target_height) {
        return src;
    }

    PVideoFrame dst = env->NewVideoFrame(vi);

    int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};
    for (int i = 0, time = vi.IsInterleaved() ? 1 : 3; i < time; i++) {
        const BYTE* srcp = src->GetReadPtr(plane[i]);
        int src_pitch = src->GetPitch(plane[i]);

        const BYTE* resized_h;
        if (params[i].src_width == params[i].target_width) {
            resized_h = srcp;
        } else {
            if (!ResizeHorizontal(buff, srcp, src_pitch, &params[i])) {
                return dst;
            }
            resized_h = buff;
            src_pitch = dst->GetRowSize(plane[i]);
        }

        BYTE* dstp = dst->GetWritePtr(plane[i]);
        int dst_pitch = dst->GetPitch(plane[i]);
        if (params[i].src_height == params[i].target_height) {
            env->BitBlt(dstp, dst_pitch, resized_h, src_pitch, dst->GetRowSize(plane[i]), dst->GetHeight(plane[i]));
            continue;
        }

        if (!ResizeVertical(dstp, dst_pitch, resized_h, src_pitch, &params[i])) {
            return dst;
        }
    }

    return dst;
}

AVSValue __cdecl CreateAreaResize(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip clip = args[0].AsClip();
    int target_width = args[1].AsInt();
    int target_height = args[2].AsInt();

    if (target_width < 1 || target_height < 1) {
        env->ThrowError("AreaResize: target width/height must be 1 or higher.");
    }

    const VideoInfo& vi = clip->GetVideoInfo();
    if (vi.IsYUY2()) {
        env->ThrowError("AreaResize: Unsupported colorspace(YUY2).");
    }
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
