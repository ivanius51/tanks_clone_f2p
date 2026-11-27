#pragma once

namespace game::view::cocos
{
/**
 * GLSL of the GPU wall merge: the same look as model::TileMergeBaker, painted per screen pixel from two small field
 * textures (the 7 material values of every field cell). GLSL 1.10 / ES 2 only (no integer operations), so the hash is
 * a float hash and the noise differs from the CPU baker's; shapes, edges, faces and shadows follow the same formulas.
 * The text is split in parts because some compilers limit the length of one string literal.
 */
inline constexpr const char* TileMergeGlslHeader = R"GLSL(
#ifdef GL_ES
precision highp float;
#endif
varying vec2 v_texCoord;
uniform sampler2D u_field0;
uniform sampler2D u_field1;
uniform vec2 u_cells;
uniform vec4 u_params0;
uniform vec4 u_params1;
uniform vec4 u_params2;
uniform vec3 u_overlayColor;
uniform vec2 u_seedOffset;
uniform vec4 u_hardA;
uniform vec4 u_hardB;
uniform vec4 u_sharpA;
uniform vec4 u_sharpB;
uniform vec4 u_shadow;

const float TU = 64.0;

float hash(vec2 p)
{
	vec3 q = fract(vec3(p.x + u_seedOffset.x, p.y + u_seedOffset.y, p.x + u_seedOffset.y) * vec3(0.1031, 0.1030, 0.0973));
	q += dot(q, q.yzx + 33.33);
	return fract((q.x + q.y) * q.z);
}

float noise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = p - i;
	f = f * f * (3.0 - 2.0 * f);
	float a = hash(i);
	float b = hash(i + vec2(1.0, 0.0));
	float c = hash(i + vec2(0.0, 1.0));
	float d = hash(i + vec2(1.0, 1.0));
	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void cellValues(float c, float r, out vec4 a, out vec4 b)
{
	if ( c < 0.0 || r < 0.0 || c >= u_cells.x || r >= u_cells.y )
	{
		a = vec4(0.0);
		b = vec4(0.0);
		return;
	}
	vec2 uv = (vec2(c, r) + 0.5) / u_cells;
	a = texture2D(u_field0, uv);
	b = texture2D(u_field1, uv);
}

float sumAt(float c, float r)
{
	vec4 a;
	vec4 b;
	cellValues(c, r, a, b);
	return min(a.x + a.y + a.z + a.w + b.x + b.y + b.z, 1.0);
}

float rounded(float t, float r)
{
	return t + (t * t * (3.0 - 2.0 * t) - t) * r;
}

float coverage(vec2 p, float roundness)
{
	vec2 uv = p - 0.5;
	vec2 c = floor(uv);
	vec2 f = uv - c;
	float fu = rounded(f.x, roundness);
	float fv = rounded(f.y, roundness);
	float a = sumAt(c.x, c.y);
	float b = sumAt(c.x + 1.0, c.y);
	float d = sumAt(c.x, c.y + 1.0);
	float e = sumAt(c.x + 1.0, c.y + 1.0);
	return (a * (1.0 - fu) + b * fu) * (1.0 - fv) + (d * (1.0 - fu) + e * fu) * fv;
}

float squareCoverage(vec2 p)
{
	vec2 uv = p - 0.5;
	vec2 c = floor(uv);
	vec2 f = uv - c;
	float nearRow = f.y < 0.5 ? c.y : c.y + 1.0;
	float nearColumn = f.x < 0.5 ? c.x : c.x + 1.0;
	float horizontal = sumAt(c.x, nearRow) * (1.0 - f.x) + sumAt(c.x + 1.0, nearRow) * f.x;
	float vertical = sumAt(nearColumn, c.y) * (1.0 - f.y) + sumAt(nearColumn, c.y + 1.0) * f.y;
	return min(horizontal, vertical);
}

float comp(vec4 a, vec4 b, int m)
{
	if ( m == 0 ) { return a.x; }
	if ( m == 1 ) { return a.y; }
	if ( m == 2 ) { return a.z; }
	if ( m == 3 ) { return a.w; }
	if ( m == 4 ) { return b.x; }
	if ( m == 5 ) { return b.y; }
	return b.z;
}

float hardOf(int m)
{
	return comp(u_hardA, u_hardB, m);
}

float sharpOf(int m)
{
	return comp(u_sharpA, u_sharpB, m);
}

bool isFloor(int m)
{
	return m >= 3;
}
)GLSL";

inline constexpr const char* TileMergeGlslMaterials = R"GLSL(
vec3 brick(float u, float v, float x, float y, float edge)
{
	float row = floor(v / 16.0);
	float inRow = v - row * 16.0;
	float offset = mod(row, 2.0) * 16.0;
	float column = floor((u + offset) / 32.0);
	float inColumn = u + offset - column * 32.0;
	float pu = floor(u);
	float pv = floor(v);
	vec3 color;
	if ( inRow < 2.0 || inColumn < 2.0 )
	{
		float grain = (hash(vec2(pu, pv)) - 0.5) * 14.0;
		color = vec3(178.0, 170.0, 154.0) + grain;
	}
	else
	{
		float tone = 0.82 + hash(vec2(column, row)) * 0.34;
		float grain = (hash(vec2(pu, pv)) - 0.5) * 18.0 + (noise(vec2(u, v) * 0.15) - 0.5) * 16.0;
		float light = inRow < 4.0 ? 1.12 : 1.0;
		color = vec3((150.0 * tone + grain) * light, (72.0 * tone + grain * 0.5) * light, (50.0 * tone + grain * 0.4) * light);
	}
	float plaster = u_params2.y;
	float patchValue = noise(vec2(x * 1.5 + 5.0, y * 1.5 + 2.0)) * 0.62 + edge * 0.5 + (noise(vec2(x, y) * 8.0) - 0.5) * 0.12;
	float limit = 1.0 - plaster * 0.55;
	if ( patchValue <= limit )
	{
		return color;
	}
	float speck = (noise(vec2(x, y) * 14.0) - 0.5) * 26.0;
	float rim = patchValue - limit < 0.025 ? 0.72 : 1.0;
	return vec3(206.0 + speck, 190.0 + speck, 168.0 + speck) * rim;
}

vec3 stone(float u, float v, float x, float y)
{
	float row = floor(v / 32.0);
	float offset = mod(row, 2.0) * 32.0;
	float column = floor((u + offset) / 64.0);
	float inColumn = u + offset - column * 64.0;
	float inRow = v - row * 32.0;
	if ( inRow < 1.6 || inColumn < 1.6 )
	{
		return vec3(78.0, 82.0, 92.0);
	}
	float tone = 0.9 + hash(vec2(column, row)) * 0.2;
	float grain = (noise(vec2(u, v) * 0.04) - 0.5) * 30.0 + (hash(vec2(floor(u), floor(v))) - 0.5) * 10.0;
	float light = inRow < 4.0 ? 1.08 : 1.0;
	vec3 color = vec3((118.0 * tone + grain) * light, (124.0 * tone + grain) * light, (136.0 * tone + grain) * light);
	float cracks = u_params2.z;
	float line = abs(noise(vec2(u * 0.05 + hash(vec2(column, row)) * 40.0, v * 0.05 + hash(vec2(row, column)) * 40.0)) - 0.5);
	float width = 0.012 * cracks * 2.0;
	if ( line < width )
	{
		color = vec3(58.0, 62.0, 72.0);
	}
	else if ( line < width * 1.7 )
	{
		color = color * vec3(0.8, 0.8, 0.82);
	}
	if ( line < width * 3.0 && noise(vec2(x, y) * TU * 0.28) > 1.0 - 0.22 * cracks )
	{
		color = vec3(164.0, 88.0, 44.0);
	}
	return color;
}

vec3 wood(float u, float v)
{
	float row = floor(v / 16.0);
	float inRow = v - row * 16.0;
	float offset = hash(vec2(row, 3.0)) * 56.0;
	float index = floor((u + offset) / 56.0);
	float inPlank = u + offset - index * 56.0;
	float gapStart = 1.0 + hash(vec2(index, row)) * 3.0;
	float gapEnd = 1.0 + hash(vec2(row, index + 9.0)) * 3.0;
	float centerX = clamp(inPlank, gapStart + 5.0, 56.0 - gapEnd - 5.0);
	float centerY = clamp(inRow, 6.0, 10.0);
	if ( length(vec2(inPlank - centerX, inRow - centerY)) > 5.0 )
	{
		return vec3(82.0, 48.0, 24.0);
	}
	float tone = 0.8 + hash(vec2(index, row + 50.0)) * 0.4;
	float grain = sin(inRow * 0.95 + noise(vec2(u * 0.03 + row * 7.0, v * 0.2)) * 7.0) * 0.5 + 0.5;
	float shade = tone * (0.9 + 0.12 * grain);
	if ( inRow < 3.5 )
	{
		shade *= 1.14;
	}
	if ( inRow > 12.5 )
	{
		shade *= 0.86;
	}
	float knotChance = hash(vec2(index, row + 90.0));
	float knotDistance = length(vec2((inPlank - (20.0 + hash(vec2(index, row + 7.0)) * 20.0)) * 0.5, inRow - 8.0));
	if ( knotChance > 0.7 && knotDistance < 3.0 )
	{
		shade *= 0.62;
	}
	else if ( knotChance > 0.7 && knotDistance < 4.6 )
	{
		shade *= 0.8;
	}
	return vec3(172.0, 104.0, 50.0) * shade;
}

vec3 water(float x, float y)
{
	float depth = noise(vec2(x, y) * 2.2);
	float warpX = (noise(vec2(x * 6.0 + 3.0, y * 6.0)) - 0.5) * 0.5;
	float warpY = (noise(vec2(x * 6.0, y * 6.0 + 7.0)) - 0.5) * 0.5;
	float ridgeA = max(1.0 - abs(noise(vec2(x * 9.0 + warpX, y * 9.0 + warpY)) * 2.0 - 1.0), 0.0);
	float ridgeB = max(1.0 - abs(noise(vec2(x * 15.0 - warpY, y * 15.0 + warpX)) * 2.0 - 1.0), 0.0);
	float glint = pow(ridgeA, 9.0) * 0.9 + pow(ridgeB, 12.0) * 0.5;
	float base = 0.75 + depth * 0.5;
	return vec3(22.0 * base + glint * 200.0, 98.0 * base + glint * 190.0, 168.0 * base + glint * 70.0);
}

vec3 forest(float x, float y)
{
	vec2 point = vec2(x, y) * 5.0;
	vec2 cell = floor(point);
	float nearest = 9.0;
	float second = 9.0;
	float tone = 0.0;
	for ( int oy = -1; oy <= 1; ++oy )
	{
		for ( int ox = -1; ox <= 1; ++ox )
		{
			vec2 n = cell + vec2(float(ox), float(oy));
			vec2 center = n + vec2(0.2 + hash(n + vec2(0.0, 31.0)) * 0.6, 0.2 + hash(n + vec2(57.0, 0.0)) * 0.6);
			float dist = length(point - center);
			if ( dist < nearest )
			{
				second = nearest;
				nearest = dist;
				tone = hash(n + vec2(11.0, 5.0));
			}
			else if ( dist < second )
			{
				second = dist;
			}
		}
	}
	float dome = clamp(1.0 - nearest * 1.15, 0.0, 1.0);
	float gap = clamp((second - nearest) * 2.5, 0.0, 1.0);
	float light = (0.55 + 0.7 * dome) * (0.45 + 0.55 * gap) * (0.8 + 0.4 * tone);
	float leaf = (noise(vec2(x, y) * 60.0) - 0.5) * 0.18;
	return vec3(40.0 + 40.0 * tone, 112.0 + 50.0 * tone, 38.0 + 22.0 * tone) * (light + leaf);
}

vec3 dirt(float x, float y)
{
	float large = noise(vec2(x, y) * 3.0);
	float fine = noise(vec2(x, y) * 18.0);
	float grit = hash(floor(vec2(x, y) * TU));
	float streak = noise(vec2(x * 5.0 + noise(vec2(x, y) * 11.0) * 2.0, y * 22.0));
	float shade = 0.72 + large * 0.4 + (fine - 0.5) * 0.28 + (grit - 0.5) * 0.12;
	vec3 color = vec3(150.0, 86.0, 62.0) * shade;
	if ( streak > 0.7 )
	{
		color *= vec3(1.18, 1.14, 1.1);
	}
	float puddle = noise(vec2(x * 4.0 + 40.0, y * 4.0 + 40.0));
	if ( puddle > 0.68 )
	{
		float wet = clamp((puddle - 0.68) * 6.0, 0.0, 1.0) * 0.35;
		color += (vec3(176.0, 118.0, 104.0) - color) * wet;
	}
	float blade = noise(vec2(x * 30.0 + 90.0, y * 30.0));
	if ( blade > 0.84 && large < 0.42 )
	{
		color = vec3(110.0, 160.0, 50.0);
	}
	return color;
}

vec3 road(float x, float y, float edge)
{
	float grain = (hash(floor(vec2(x, y) * TU)) - 0.5) * 26.0;
	float patchValue = (noise(vec2(x, y) * 4.0) - 0.5) * 16.0;
	float base = 62.0 + grain + patchValue;
	if ( edge > 0.8 && edge < 0.9 && noise(vec2(x, y) * 25.0) > 0.12 )
	{
		return vec3(214.0, 214.0, 208.0) + grain;
	}
	return vec3(base, base + 1.0, base + 2.0);
}

vec3 fillMaterial(int m, float x, float y, float edge)
{
	float u = x * TU;
	float v = y * TU;
	if ( m == 0 ) { return brick(u, v, x, y, edge); }
	if ( m == 1 ) { return stone(u, v, x, y); }
	if ( m == 2 ) { return wood(u, v); }
	if ( m == 3 ) { return water(x, y); }
	if ( m == 4 ) { return forest(x, y); }
	if ( m == 5 ) { return dirt(x, y); }
	return road(x, y, edge);
}
)GLSL";

inline constexpr const char* TileMergeGlslColorMain = R"GLSL(
float edgeLightShade(vec2 p)
{
	float light = u_params1.w;
	if ( light <= 0.0 )
	{
		return 1.0;
	}
	float stepSize = 2.0 / TU * u_params0.y;
	bool topOpen = coverage(vec2(p.x, p.y - stepSize), u_params1.x) < u_params0.z;
	bool leftOpen = coverage(vec2(p.x - stepSize, p.y), u_params1.x) < u_params0.z;
	return topOpen || leftOpen ? 1.0 + light : 1.0;
}

float faceShade(vec2 p)
{
	if ( u_params2.x <= 0.0 )
	{
		return 1.0;
	}
	float roundness = u_params1.x;
	float threshold = u_params0.z;
	float face = u_params2.x / TU * u_params0.y;
	if ( coverage(vec2(p.x, p.y + face), roundness) >= threshold )
	{
		return 1.0;
	}
	float nearEnd = 0.0;
	float farEnd = face;
	for ( int s = 0; s < 5; ++s )
	{
		float middle = (nearEnd + farEnd) * 0.5;
		if ( coverage(vec2(p.x, p.y + middle), roundness) >= threshold )
		{
			nearEnd = middle;
		}
		else
		{
			farEnd = middle;
		}
	}
	float position = clamp(1.0 - (nearEnd + farEnd) * 0.5 / face, 0.0, 1.0);
	if ( position < 0.03 )
	{
		return 1.25;
	}
	if ( position < 0.07 )
	{
		return 0.4;
	}
	if ( position < 0.77 )
	{
		return 0.85 + (0.35 - 0.85) * (position - 0.07) / (0.77 - 0.07);
	}
	return 0.2 + (0.16 - 0.2) * (position - 0.77) / (1.0 - 0.77);
}

void main()
{
	vec2 p = v_texCoord * u_cells;
	float roundness = u_params1.x;
	float thresholdSample = u_params0.w;
	vec2 uv = p - 0.5;
	vec2 c = floor(uv);
	vec2 f = uv - c;
	float smoothCover = coverage(p, roundness);
	float squareCover = squareCoverage(p);
	if ( max(smoothCover, squareCover) < thresholdSample - 0.12 )
	{
		gl_FragColor = vec4(0.0);
		return;
	}
	vec2 jitter = vec2(noise(p * 3.1) - 0.5, noise(p * 3.1 + vec2(17.0, 5.0)) - 0.5) * 0.9;
	vec4 a00; vec4 b00; vec4 a10; vec4 b10; vec4 a01; vec4 b01; vec4 a11; vec4 b11;
	cellValues(c.x, c.y, a00, b00);
	cellValues(c.x + 1.0, c.y, a10, b10);
	cellValues(c.x, c.y + 1.0, a01, b01);
	cellValues(c.x + 1.0, c.y + 1.0, a11, b11);
	float best = -1.0;
	int material = 0;
	for ( int m = 0; m < 7; ++m )
	{
		float hard = hardOf(m);
		float reach = min(hard * 3.0, 1.0);
		float occupancy = (comp(a00, b00, m) * (1.0 - f.x) + comp(a10, b10, m) * f.x) * (1.0 - f.y) + (comp(a01, b01, m) * (1.0 - f.x) + comp(a11, b11, m) * f.x) * f.y;
		float mixed = occupancy * (1.0 - hard);
		if ( hard > 0.0 )
		{
			vec4 sa;
			vec4 sb;
			vec2 stepCell = floor(p + jitter * reach);
			cellValues(stepCell.x, stepCell.y, sa, sb);
			mixed += comp(sa, sb, m) * hard;
		}
		if ( mixed > best )
		{
			best = mixed;
			material = m;
		}
	}
	float sharp = sharpOf(material);
	float cover = smoothCover * (1.0 - sharp) + squareCover * sharp;
	if ( cover < thresholdSample - 0.03 )
	{
		gl_FragColor = vec4(0.0);
		return;
	}
	float depth = cover - u_params0.z;
	float edge = clamp(1.0 - depth / 0.35, 0.0, 1.0);
	vec2 tile = p / u_params0.y;
	vec3 color = fillMaterial(material, tile.x, tile.y, edge);
	if ( !isFloor(material) )
	{
		color *= faceShade(p) * edgeLightShade(p);
	}
	color *= 1.0 - 0.18 + 0.18 * clamp(depth / 0.1, 0.0, 1.0);
	float amount = u_params2.w;
	if ( amount > 0.0 )
	{
		vec2 pixel = floor(tile * TU);
		float cluster = noise(tile * 1.3 + vec2(3.0, 9.0));
		float chance = amount * (0.35 + 1.3 * cluster) + edge * 0.05;
		if ( hash(floor(pixel * 0.5)) < chance )
		{
			float strength = 0.3 + 0.25 * hash(pixel);
			color += (u_overlayColor * 255.0 - color) * strength;
		}
	}
	float alpha = clamp((depth + 0.03) / 0.03, 0.0, 1.0);
	gl_FragColor = vec4(clamp(color, 0.0, 255.0) / 255.0, alpha);
}
)GLSL";

inline constexpr const char* TileMergeGlslShadowMain = R"GLSL(
void main()
{
	vec2 p = v_texCoord * u_cells;
	float roundness = u_params1.x;
	float threshold = u_params0.z;
	float soft = u_shadow.z;
	float footprint = clamp((coverage(p, roundness) - (threshold - soft)) / (2.0 * soft), 0.0, 1.0);
	float strongest = 0.0;
	for ( int tap = 0; tap <= 6; ++tap )
	{
		float along = float(tap) / 6.0;
		float cover = coverage(p - u_shadow.xy * along, roundness);
		strongest = max(strongest, clamp((cover - (threshold - soft)) / (2.0 * soft), 0.0, 1.0));
	}
	gl_FragColor = vec4(0.0, 0.0, 0.0, u_shadow.w * strongest * (1.0 - footprint));
}
)GLSL";
} // namespace game::view::cocos
