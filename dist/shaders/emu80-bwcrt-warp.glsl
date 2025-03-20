// (c) Copyright Viktor Pykhonin, 2025
// This Emu80 shader based on Libretro's shaders scanline.glsl and hauss_horiz.glsl

#version 130

#define BLUR_WIDTH (1.0 / 1000.0)
#define SCANLINE_SINE 0.5
#define LUM_K 0.8
#define warpX 0.021
#define warpY 0.027


#define pi 3.141592654

#if defined(VERTEX)

in vec4 VertexCoord;
in vec4 TexCoord;

out vec2 TEX0;

uniform mat4 MVPMatrix;
uniform vec2 OutputSize;
uniform vec2 TextureSize;
uniform vec2 InputSize;

void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    TEX0 = TexCoord.xy;
}

#elif defined(FRAGMENT)

out vec4 FragColor;

uniform vec2 OutputSize;
uniform vec2 TextureSize;
uniform vec2 InputSize;
uniform sampler2D Texture;
in vec2 TEX0;


float grayscale(highp vec3 rgb)
{
    float r, g, b;
    if (rgb.r <= 0.04045) r = rgb.r / 12.92; else r = pow(((rgb.r + 0.055)/1.055), 2.4);
    if (rgb.g <= 0.04045) g = rgb.g / 12.92; else g = pow(((rgb.g + 0.055)/1.055), 2.4);
    if (rgb.b <= 0.04045) b = rgb.b / 12.92; else b = pow(((rgb.b + 0.055)/1.055), 2.4);
    float y = 0.212655 * r + 0.715158 * g + 0.072187 * b;

    if (y <= 0.0031308)
        return y * 12.92;
    else
        return 1.055 * pow(y, 1.0 / 2.4) - 0.055;
}

vec4 shift(vec4 p) {
  return vec4(p.rgb * 0.85 + vec3(0.15), p.a);
}

vec2 warp(vec2 pos)
{
    pos  = pos * 2.0 - 1.0;    
    pos *= vec2(1.0 + (pos.y * pos.y) * warpX, 1.0 + (pos.x * pos.x) * warpY);
    
    return pos*0.5 + 0.5;
}


float gauss[5] = float[](0.0545, 0.244, 0.403, 0.244, 0.0545);

void main()
{
    vec4 col = vec4(0.0);

    vec3 p = texture(Texture, warp(TEX0)).rgb;
    float kh = grayscale(p) * 0.25 + 1;

    for (int i = 0; i <= 4; i++)
    {
        float g = gauss[i];
        col += shift(texture(Texture, warp(TEX0 + vec2((i - 2) * BLUR_WIDTH * kh)), 0.0)) * g;
    }

    vec3 res = col.rgb;
    float a = col.a;

    float sine_comp = (0.5 - grayscale(res)) * LUM_K + SCANLINE_SINE;

    float scanline = sine_comp * sin(fract(warp(TEX0).y * TextureSize.y) * pi) + 1.0 - sine_comp;
    res *= scanline;
   
    FragColor = vec4(vec3(0.90, 0.976, 1.0) * grayscale(res.rgb), a);
} 
#endif
