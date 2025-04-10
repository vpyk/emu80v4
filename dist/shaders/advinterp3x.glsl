#version 120

// This file has been ported to Emu80 from DOSBox GLSL shader by Victor Pykhonin

/*
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 *  Copyright (C) 2020-2024  The DOSBox Staging Team
 *  Copyright (C) 2006-2020  The DOSBox Team
 *
 * Contributors:
 *   - 2006, Sjoerd van der Berg <harekiet@users.sourceforge.net>: authored
 *           https://svn.code.sf.net/p/dosbox/code-0/dosbox/trunk@2469
 *
 *   - 2020, jmarsh <jmarsh@vogons.org>: converted to OpenGL fragment shader
 *           https://svn.code.sf.net/p/dosbox/code-0/dosbox/trunk@4319
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#pragma force_single_scan
#pragma force_no_pixel_doubling

varying vec2 v_texCoord;
uniform sampler2D Texture;
uniform vec2 InputSize;
uniform vec2 OutputSize;
uniform vec2 TextureSize;

#if defined(VERTEX)
attribute vec4 VertexCoord;

void main()
{
    gl_Position = VertexCoord;
    v_texCoord = vec2(VertexCoord.x + 1.0, 1.0 - VertexCoord.y) / 2.0 * InputSize * 3.0;
}

#elif defined(FRAGMENT)

vec3 getadvinterp3xtexel(vec2 coord)
{
    vec2 base = floor(coord / vec2(3.0)) + vec2(0.5);
    vec3 c0 = texture2D(Texture, (base - vec2(1.0, 1.0)) / TextureSize).xyz;
    vec3 c1 = texture2D(Texture, (base - vec2(0.0, 1.0)) / TextureSize).xyz;
    vec3 c2 = texture2D(Texture, (base - vec2(-1.0, 1.0)) / TextureSize).xyz;
    vec3 c3 = texture2D(Texture, (base - vec2(1.0, 0.0)) / TextureSize).xyz;
    vec3 c4 = texture2D(Texture, base / TextureSize).xyz;
    vec3 c5 = texture2D(Texture, (base + vec2(1.0, 0.0)) / TextureSize).xyz;
    vec3 c6 = texture2D(Texture, (base + vec2(-1.0, 1.0)) / TextureSize).xyz;
    vec3 c7 = texture2D(Texture, (base + vec2(0.0, 1.0)) / TextureSize).xyz;
    vec3 c8 = texture2D(Texture, (base + vec2(1.0, 1.0)) / TextureSize).xyz;

    bool outer = c1 != c7 && c3 != c5;

    vec3 l00 = mix(c3, c4, (outer && c3 == c1) ? 3.0 / 8.0 : 1.0);
    vec3 l01 = (outer && ((c3 == c1 && c4 != c2) || (c5 == c1 && c4 != c0))) ? c1 : c4;
    vec3 l02 = mix(c5, c4, (outer && c5 == c1) ? 3.0 / 8.0 : 1.0);
    vec3 l10 = (outer && ((c3 == c1 && c4 != c6) || (c3 == c7 && c4 != c0))) ? c3 : c4;
    vec3 l11 = c4;
    vec3 l12 = (outer && ((c5 == c1 && c4 != c8) || (c5 == c7 && c4 != c2))) ? c5 : c4;
    vec3 l20 = mix(c3, c4, (outer && c3 == c7) ? 3.0 / 8.0 : 1.0);
    vec3 l21 = (outer && ((c3 == c7 && c4 != c8) || (c5 == c7 && c4 != c6))) ? c7 : c4;
    vec3 l22 = mix(c5, c4, (outer && c5 == c7) ? 3.0 / 8.0 : 1.0);

    coord = mod(coord, 3.0);
    bvec2 l = lessThan(coord, vec2(1.0));
    bvec2 h = greaterThanEqual(coord, vec2(2.0));

    if (h.x) {
        l01 = l02;
        l11 = l12;
        l21 = l22;
    }
    if (h.y) {
        l10 = l20;
        l11 = l21;
    }
    if (l.x) {
        l01 = l00;
        l11 = l10;
    }
    return l.y ? l01 : l11;
}

void main()
{
    vec2 coord = v_texCoord;
    coord -= 0.5;
    vec3 c0 = getadvinterp3xtexel(coord);
    vec3 c1 = getadvinterp3xtexel(coord + vec2(1.0, 0.0));
    vec3 c2 = getadvinterp3xtexel(coord + vec2(0.0, 1.0));
    vec3 c3 = getadvinterp3xtexel(coord + vec2(1.0));

    coord = fract(max(coord, 0.0));
    gl_FragColor = vec4(mix(mix(c0, c1, coord.x), mix(c2, c3, coord.x), coord.y), 1.0);
}
#endif
