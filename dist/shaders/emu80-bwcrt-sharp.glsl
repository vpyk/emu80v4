// (c) Copyright Viktor Pykhonin, 2025
// This Emu80 shader based on Libretro's shaders scanline.glsl and hauss_horiz.glsl

#version 130

#define SCANLINE_SINE 0.5


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


void main()
{
   vec3 res = texture(Texture, TEX0).rgb;
   float a = texture(Texture, TEX0).a;

   float scanline = SCANLINE_SINE * sin(fract(TEX0.y * TextureSize.y) * pi) + 1.0 - SCANLINE_SINE;
   res *= scanline;
   
   FragColor = vec4(vec3(0.90, 0.976, 1.0) * grayscale(res.rgb), a);
} 
#endif
