#version 330 core

uniform vec2 inOverOut0;
uniform float outWidth;
uniform float outHeight;
uniform float oneOverInX;

uniform int color709;
uniform sampler2D Ytex;

in vec2 v_texcoord;
layout (location = 0) out vec4 fragColor;

vec3 yuv_to_rgb (vec3 val, mat3 coff_yuv) {
	vec3 rgb;
	val += vec3(-0.0625, -0.5, -0.5);
	rgb.r = dot(val, coff_yuv[0]);
	rgb.g = dot(val, coff_yuv[1]);
	rgb.b = dot(val, coff_yuv[2]);
	return rgb;
} 


void main (void) {
	mat3 m_709 = mat3(
		1.164,  0.000,  1.787,  // first column 
		1.164, -0.213, -0.531,  // second column
		1.164,  2.112,  0.000   // third column
	);
	mat3 m_601 = mat3(
		1.164,  0.000,  1.596,  // first column 
		1.164, -0.391, -0.813,  // second column
		1.164,  2.018,  0.000   // third column
	);
	vec2 texcoord;
	texcoord = v_texcoord;
	vec4 rgba, uv_texel;
	vec3 yuv;
	float dx1 = -oneOverInX;
	float dx2 = 0.0;
	//Yn
	yuv.x = texture(Ytex, texcoord * inOverOut0).r;
	float inorder = mod (v_texcoord.x * outWidth, 2.0);
	if (inorder < 1.0) {
		dx2 = -dx1;
		dx1 = 0.0;
	}
	//Y0U0, Y2U2
	uv_texel.rg = texture(Ytex, texcoord * inOverOut0 + vec2(dx1, 0.0)).rg;
	//Y1V0, Y3V2
	uv_texel.ba = texture(Ytex, texcoord * inOverOut0 + vec2(dx2, 0.0)).rg;
	//U0V0, U2V2
	yuv.yz = uv_texel.ga;
	//rgba.rgb = yuv_to_rgb(yuv, offset, coeff1, coeff2, coeff3);
	if( color709!=0 ) rgba.rgb = yuv_to_rgb(yuv, m_709);
	else rgba.rgb = yuv_to_rgb(yuv, m_601);
	rgba.a = 1.0;
	fragColor = vec4(rgba.r,rgba.g,rgba.b,rgba.a);
}

