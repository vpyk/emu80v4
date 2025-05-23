/*
    CRT-interlaced

    Copyright (C) 2010-2012 cgwg, Themaister and DOLLS

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    (cgwg gave their consent to have the original version of this shader
    distributed under the GPL in this message:

        http://board.byuu.org/viewtopic.php?p=26075#p26075

        "Feel free to distribute my shaders under the GPL. After all, the
        barrel distortion code was taken from the Curvature shader, which is
        under the GPL."
    )
    This shader variant is pre-configured with screen curvature
*/

#pragma parameter CRTgamma "CRTGeom Target Gamma" 2.4 0.1 5.0 0.1
#pragma parameter INV "Inverse Gamma/CRT-Geom Gamma out" 1.0 0.0 1.0 1.0
#pragma parameter monitorgamma "CRTGeom Monitor Gamma" 2.2 0.1 5.0 0.1
#pragma parameter d "CRTGeom Distance" 1.6 0.1 3.0 0.1
#pragma parameter CURVATURE "CRTGeom Curvature Toggle" 1.0 0.0 1.0 1.0
#pragma parameter R "CRTGeom Curvature Radius" 2.0 0.1 10.0 0.1
#pragma parameter cornersize "CRTGeom Corner Size" 0.03 0.001 1.0 0.005
#pragma parameter cornersmooth "CRTGeom Corner Smoothness" 1000.0 80.0 2000.0 100.0
#pragma parameter x_tilt "CRTGeom Horizontal Tilt" 0.0 -0.5 0.5 0.05
#pragma parameter y_tilt "CRTGeom Vertical Tilt" 0.0 -0.5 0.5 0.05
#pragma parameter overscan_x "CRTGeom Horiz. Overscan %" 100.0 -125.0 125.0 1.0
#pragma parameter overscan_y "CRTGeom Vert. Overscan %" 100.0 -125.0 125.0 1.0
#pragma parameter DOTMASK "CRTGeom Dot Mask Strength" 0.3 0.0 1.0 0.1
#pragma parameter SHARPER "CRTGeom Sharpness" 1.0 1.0 3.0 1.0
#pragma parameter scanline_weight "CRTGeom Scanline Weight" 0.3 0.1 0.5 0.05
#pragma parameter lum "CRTGeom Luminance" 0.0 0.0 1.0 0.01
#pragma parameter interlace_detect "CRTGeom Interlacing Simulation" 1.0 0.0 1.0 1.0
#pragma parameter SATURATION "CRTGeom Saturation" 1.0 0.0 2.0 0.05

#ifndef PARAMETER_UNIFORM
#define CRTgamma 2.4
#define monitorgamma 2.2
#define d 1.6
#define CURVATURE 1.0
#define R 2.0
#define cornersize 0.03
#define cornersmooth 1000.0
#define x_tilt 0.0
#define y_tilt 0.0
#define overscan_x 100.0
#define overscan_y 100.0
#define DOTMASK 0.3
#define SHARPER 1.0
#define scanline_weight 0.3
#define lum 0.0
#define interlace_detect 1.0
#define SATURATION 1.0
#define INV 1.0
#endif

#if defined(VERTEX)

#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying 
#define COMPAT_ATTRIBUTE attribute 
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 COLOR;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 COL0;
COMPAT_VARYING vec4 TEX0;

vec4 _oPosition1; 
uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;

COMPAT_VARYING vec2 overscan;
COMPAT_VARYING vec2 aspect;
COMPAT_VARYING vec3 stretch;
COMPAT_VARYING vec2 sinangle;
COMPAT_VARYING vec2 cosangle;
COMPAT_VARYING vec2 one;
COMPAT_VARYING float mod_factor;
COMPAT_VARYING vec2 ilfac;

#ifdef PARAMETER_UNIFORM
uniform COMPAT_PRECISION float CRTgamma;
uniform COMPAT_PRECISION float monitorgamma;
uniform COMPAT_PRECISION float d;
uniform COMPAT_PRECISION float CURVATURE;
uniform COMPAT_PRECISION float R;
uniform COMPAT_PRECISION float cornersize;
uniform COMPAT_PRECISION float cornersmooth;
uniform COMPAT_PRECISION float x_tilt;
uniform COMPAT_PRECISION float y_tilt;
uniform COMPAT_PRECISION float overscan_x;
uniform COMPAT_PRECISION float overscan_y;
uniform COMPAT_PRECISION float DOTMASK;
uniform COMPAT_PRECISION float SHARPER;
uniform COMPAT_PRECISION float scanline_weight;
uniform COMPAT_PRECISION float lum;
uniform COMPAT_PRECISION float interlace_detect;
uniform COMPAT_PRECISION float SATURATION;
#endif

#define FIX(c) max(abs(c), 1e-5);

float intersect(vec2 xy)
        {
    float A = dot(xy,xy)+d*d;
    float B = 2.0*(R*(dot(xy,sinangle)-d*cosangle.x*cosangle.y)-d*d);
    float C = d*d + 2.0*R*d*cosangle.x*cosangle.y;
    return (-B-sqrt(B*B-4.0*A*C))/(2.0*A);
        }

vec2 bkwtrans(vec2 xy)
        {
    float c = intersect(xy);
    vec2 point = vec2(c)*xy;
    point -= vec2(-R)*sinangle;
    point /= vec2(R);
    vec2 tang = sinangle/cosangle;
    vec2 poc = point/cosangle;
    float A = dot(tang,tang)+1.0;
    float B = -2.0*dot(poc,tang);
    float C = dot(poc,poc)-1.0;
    float a = (-B+sqrt(B*B-4.0*A*C))/(2.0*A);
    vec2 uv = (point-a*sinangle)/cosangle;
    float r = R*acos(a);
    return uv*r/sin(r/R);
        }

vec2 fwtrans(vec2 uv)
        {
    float r = FIX(sqrt(dot(uv,uv)));
    uv *= sin(r/R)/r;
    float x = 1.0-cos(r/R);
    float D = d/R + x*cosangle.x*cosangle.y+dot(uv,sinangle);
    return d*(uv*cosangle-x*sinangle)/D;
        }

vec3 maxscale()
        {
    vec2 c = bkwtrans(-R * sinangle / (1.0 + R/d*cosangle.x*cosangle.y));
    vec2 a = vec2(0.5,0.5)*aspect;
    vec2 lo = vec2(fwtrans(vec2(-a.x,c.y)).x, fwtrans(vec2(c.x,-a.y)).y)/aspect;
    vec2 hi = vec2(fwtrans(vec2(+a.x,c.y)).x, fwtrans(vec2(c.x,+a.y)).y)/aspect;
    return vec3((hi+lo)*aspect*0.5,max(hi.x-lo.x,hi.y-lo.y));
        }

void main()
{
// START of parameters

// gamma of simulated CRT
//  CRTgamma = 1.8;
// gamma of display monitor (typically 2.2 is correct)
//  monitorgamma = 2.2;
// overscan (e.g. 1.02 for 2% overscan)
    overscan = vec2(1.00,1.00);
// aspect ratio
    aspect = vec2(1.0, 0.75);
// lengths are measured in units of (approximately) the width
// of the monitor simulated distance from viewer to monitor
//  d = 2.0;
// radius of curvature
//  R = 1.5;
// tilt angle in radians
// (behavior might be a bit wrong if both components are
// nonzero)
    const vec2 angle = vec2(0.0,0.0);
// size of curved corners
//  cornersize = 0.03;
// border smoothness parameter
// decrease if borders are too aliased
//  cornersmooth = 1000.0;

// END of parameters

    vec4 _oColor;
    vec2 _otexCoord;
    gl_Position = VertexCoord.x * MVPMatrix[0] + VertexCoord.y * MVPMatrix[1] + VertexCoord.z * MVPMatrix[2] + VertexCoord.w * MVPMatrix[3];
    _oPosition1 = gl_Position;
    _oColor = COLOR;
    _otexCoord = TexCoord.xy*1.0001;
    COL0 = COLOR;
    TEX0.xy = TexCoord.xy*1.0001;

// Precalculate a bunch of useful values we'll need in the fragment
// shader.
    sinangle = sin(vec2(x_tilt, y_tilt)) + vec2(0.001);//sin(vec2(max(abs(x_tilt), 1e-3), max(abs(y_tilt), 1e-3)));
    cosangle = cos(vec2(x_tilt, y_tilt)) + vec2(0.001);//cos(vec2(max(abs(x_tilt), 1e-3), max(abs(y_tilt), 1e-3)));
    stretch = maxscale();

    ilfac = vec2(1.0,clamp(floor(InputSize.y/200.0), 1.0, 2.0));

// The size of one texel, in texture-coordinates.
    vec2 sharpTextureSize = vec2(SHARPER * TextureSize.x, TextureSize.y);
    one = ilfac / sharpTextureSize;

// Resulting X pixel-coordinate of the pixel we're drawing.
    mod_factor = TexCoord.x * TextureSize.x * OutputSize.x / InputSize.x;

}

#elif defined(FRAGMENT)

#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

struct output_dummy {
    vec4 _color;
};

uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;

// Comment the next line to disable interpolation in linear gamma (and
// gain speed).
    #define LINEAR_PROCESSING

// Enable screen curvature.
//        #define CURVATURE

// Enable 3x oversampling of the beam profile
        #define OVERSAMPLE

// Use the older, purely gaussian beam profile
        //#define USEGAUSSIAN

// Macros.
#define FIX(c) max(abs(c), 1e-5);
#define PI 3.141592653589

#ifdef LINEAR_PROCESSING
#       define TEX2D(c) pow(COMPAT_TEXTURE(Texture, (c)), vec4(CRTgamma))
#else
#       define TEX2D(c) COMPAT_TEXTURE(Texture, (c))
#endif

COMPAT_VARYING vec2 one;
COMPAT_VARYING float mod_factor;
COMPAT_VARYING vec2 ilfac;
COMPAT_VARYING vec2 overscan;
COMPAT_VARYING vec2 aspect;
COMPAT_VARYING vec3 stretch;
COMPAT_VARYING vec2 sinangle;
COMPAT_VARYING vec2 cosangle;

#ifdef PARAMETER_UNIFORM
uniform COMPAT_PRECISION float CRTgamma;
uniform COMPAT_PRECISION float monitorgamma;
uniform COMPAT_PRECISION float d;
uniform COMPAT_PRECISION float CURVATURE;
uniform COMPAT_PRECISION float R;
uniform COMPAT_PRECISION float cornersize;
uniform COMPAT_PRECISION float cornersmooth;
uniform COMPAT_PRECISION float x_tilt;
uniform COMPAT_PRECISION float y_tilt;
uniform COMPAT_PRECISION float overscan_x;
uniform COMPAT_PRECISION float overscan_y;
uniform COMPAT_PRECISION float DOTMASK;
uniform COMPAT_PRECISION float SHARPER;
uniform COMPAT_PRECISION float scanline_weight;
uniform COMPAT_PRECISION float lum;
uniform COMPAT_PRECISION float interlace_detect;
uniform COMPAT_PRECISION float SATURATION;
uniform COMPAT_PRECISION float INV;
#endif

float intersect(vec2 xy)
        {
    float A = dot(xy,xy)+d*d;
    float B = 2.0*(R*(dot(xy,sinangle)-d*cosangle.x*cosangle.y)-d*d);
    float C = d*d + 2.0*R*d*cosangle.x*cosangle.y;
    return (-B-sqrt(B*B-4.0*A*C))/(2.0*A);
        }

vec2 bkwtrans(vec2 xy)
        {
    float c = intersect(xy);
    vec2 point = vec2(c)*xy;
    point -= vec2(-R)*sinangle;
    point /= vec2(R);
    vec2 tang = sinangle/cosangle;
    vec2 poc = point/cosangle;
    float A = dot(tang,tang)+1.0;
    float B = -2.0*dot(poc,tang);
    float C = dot(poc,poc)-1.0;
    float a = (-B+sqrt(B*B-4.0*A*C))/(2.0*A);
    vec2 uv = (point-a*sinangle)/cosangle;
    float r = FIX(R*acos(a));
    return uv*r/sin(r/R);
        }

vec2 transform(vec2 coord)
        {
    coord *= TextureSize / InputSize;
    coord = (coord-vec2(0.5))*aspect*stretch.z+stretch.xy;
    return (bkwtrans(coord)/vec2(overscan_x / 100.0, overscan_y / 100.0)/aspect+vec2(0.5)) * InputSize / TextureSize;
        }

float corner(vec2 coord)
        {
    coord *= TextureSize / InputSize;
    coord = (coord - vec2(0.5)) * vec2(overscan_x / 100.0, overscan_y / 100.0) + vec2(0.5);
    coord = min(coord, vec2(1.0)-coord) * aspect;
    vec2 cdist = vec2(cornersize);
    coord = (cdist - min(coord,cdist));
    float dist = sqrt(dot(coord,coord));
    return clamp((cdist.x-dist)*cornersmooth,0.0, 1.0)*1.0001;
        }

// Calculate the influence of a scanline on the current pixel.
//
// 'distance' is the distance in texture coordinates from the current
// pixel to the scanline in question.
// 'color' is the colour of the scanline at the horizontal location of
// the current pixel.
vec4 scanlineWeights(float distance, vec4 color)
        {
    // "wid" controls the width of the scanline beam, for each RGB
    // channel The "weights" lines basically specify the formula
    // that gives you the profile of the beam, i.e. the intensity as
    // a function of distance from the vertical center of the
    // scanline. In this case, it is gaussian if width=2, and
    // becomes nongaussian for larger widths. Ideally this should
    // be normalized so that the integral across the beam is
    // independent of its width. That is, for a narrower beam
    // "weights" should have a higher peak at the center of the
    // scanline than for a wider beam.
#ifdef USEGAUSSIAN
    vec4 wid = 0.3 + 0.1 * pow(color, vec4(3.0));
    vec4 weights = vec4(distance / wid);
    return (lum + 0.4) * exp(-weights * weights) / wid;
#else
    vec4 wid = 2.0 + 2.0 * pow(color, vec4(4.0));
    vec4 weights = vec4(distance / scanline_weight);
    return (lum + 1.4) * exp(-pow(weights * inversesqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid);
#endif
        }

vec3 saturation (vec3 textureColor)
{
    float _lum=length(textureColor)*0.5775;

    vec3 luminanceWeighting = vec3(0.3,0.6,0.1);
    if (_lum<0.5) luminanceWeighting.rgb=(luminanceWeighting.rgb*luminanceWeighting.rgb)+(luminanceWeighting.rgb*luminanceWeighting.rgb);

    float luminance = dot(textureColor, luminanceWeighting);
    vec3 greyScaleColor = vec3(luminance);

    vec3 res = vec3(mix(greyScaleColor, textureColor, SATURATION));
    return res;
}

#define pwr vec3(1.0/((-0.7*(1.0-scanline_weight)+1.0)*(-0.5*DOTMASK+1.0))-1.25)


// Returns gamma corrected output, compensated for scanline+mask embedded gamma
vec3 inv_gamma(vec3 col, vec3 power)
{
    vec3 cir  = col-1.0;
         cir *= cir;
         col  = mix(sqrt(col),sqrt(1.0-cir),power);
    return col;
}

void main()
{
// Here's a helpful diagram to keep in mind while trying to
// understand the code:
//
//  |      |      |      |      |
// -------------------------------
//  |      |      |      |      |
//  |  01  |  11  |  21  |  31  | <-- current scanline
//  |      | @    |      |      |
// -------------------------------
//  |      |      |      |      |
//  |  02  |  12  |  22  |  32  | <-- next scanline
//  |      |      |      |      |
// -------------------------------
//  |      |      |      |      |
//
// Each character-cell represents a pixel on the output
// surface, "@" represents the current pixel (always somewhere
// in the bottom half of the current scan-line, or the top-half
// of the next scanline). The grid of lines represents the
// edges of the texels of the underlying texture.

// Texture coordinates of the texel containing the active pixel.
    vec2 xy = (CURVATURE > 0.5) ? transform(TEX0.xy) : TEX0.xy;

    float cval = corner(xy);

// Of all the pixels that are mapped onto the texel we are
// currently rendering, which pixel are we currently rendering?
    vec2 ilvec = vec2(0.0,ilfac.y * interlace_detect > 1.5 ? mod(float(FrameCount),2.0) : 0.0);
    vec2 ratio_scale = (xy * TextureSize - vec2(0.5) + ilvec)/ilfac;
#ifdef OVERSAMPLE
    float filter_ = InputSize.y/OutputSize.y;//fwidth(ratio_scale.y);
#endif
    vec2 uv_ratio = fract(ratio_scale);

// Snap to the center of the underlying texel.
    xy = (floor(ratio_scale)*ilfac + vec2(0.5) - ilvec) / TextureSize;

// Calculate Lanczos scaling coefficients describing the effect
// of various neighbour texels in a scanline on the current
// pixel.
    vec4 coeffs = PI * vec4(1.0 + uv_ratio.x, uv_ratio.x, 1.0 - uv_ratio.x, 2.0 - uv_ratio.x);

// Prevent division by zero.
    coeffs = FIX(coeffs);

// Lanczos2 kernel.
    coeffs = 2.0 * sin(coeffs) * sin(coeffs / 2.0) / (coeffs * coeffs);

// Normalize.
    coeffs /= dot(coeffs, vec4(1.0));

// Calculate the effective colour of the current and next
// scanlines at the horizontal location of the current pixel,
// using the Lanczos coefficients above.
    vec4 col  = clamp(mat4(
                        TEX2D(xy + vec2(-one.x, 0.0)),
                        TEX2D(xy),
                        TEX2D(xy + vec2(one.x, 0.0)),
                        TEX2D(xy + vec2(2.0 * one.x, 0.0))) * coeffs,
                        0.0, 1.0);
        vec4 col2 = clamp(mat4(
                        TEX2D(xy + vec2(-one.x, one.y)),
                        TEX2D(xy + vec2(0.0, one.y)),
                        TEX2D(xy + one),
                        TEX2D(xy + vec2(2.0 * one.x, one.y))) * coeffs,
                        0.0, 1.0);

#ifndef LINEAR_PROCESSING
    col  = pow(col , vec4(CRTgamma));
    col2 = pow(col2, vec4(CRTgamma));
#endif

// Calculate the influence of the current and next scanlines on
// the current pixel.
    vec4 weights  = scanlineWeights(uv_ratio.y, col);
    vec4 weights2 = scanlineWeights(1.0 - uv_ratio.y, col2);
#ifdef OVERSAMPLE
    uv_ratio.y =uv_ratio.y+1.0/3.0*filter_;
    weights = (weights+scanlineWeights(uv_ratio.y, col))/3.0;
    weights2=(weights2+scanlineWeights(abs(1.0-uv_ratio.y), col2))/3.0;
    uv_ratio.y =uv_ratio.y-2.0/3.0*filter_;
    weights=weights+scanlineWeights(abs(uv_ratio.y), col)/3.0;
    weights2=weights2+scanlineWeights(abs(1.0-uv_ratio.y), col2)/3.0;
#endif

    vec3 mul_res  = (col * weights + col2 * weights2).rgb * vec3(cval);

// dot-mask emulation:
// Output pixels are alternately tinted green and magenta.
vec3 dotMaskWeights = mix(
    vec3(1.0, 1.0 - DOTMASK, 1.0),
    vec3(1.0 - DOTMASK, 1.0, 1.0 - DOTMASK),
    floor(mod(mod_factor, 2.0))
        );

    mul_res *= dotMaskWeights;

// Convert the image gamma for display on our output device.
if (INV == 1.0){ mul_res = inv_gamma(mul_res,pwr);} 
    else mul_res = pow(mul_res, vec3(1.0/monitorgamma));
        
        mul_res = saturation(mul_res);



// Color the texel.
    output_dummy _OUT;
    _OUT._color = vec4(mul_res, 1.0);
    FragColor = _OUT._color;
    return;
} 
#endif
