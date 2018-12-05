#version 330 core

uniform vec2 inOverOut0;
uniform float outWidth;
uniform float outHeight;
uniform float oneOverInX;

uniform vec3 offset;
uniform vec3 coeff1;
uniform vec3 coeff2;
uniform vec3 coeff3;
uniform sampler2D Ytex;

in vec2 v_texcoord;
layout (location = 0) out vec4 fragColor;

vec3 yuv_to_rgb (vec3 val, vec3 offset, vec3 ycoeff, vec3 ucoeff, vec3 vcoeff) {
	vec3 rgb;
	val += offset;
	rgb.r = dot(val, ycoeff);
	rgb.g = dot(val, ucoeff);
	rgb.b = dot(val, vcoeff);
	return rgb;
}


void main (void) {
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
	rgba.rgb = yuv_to_rgb (yuv, offset, coeff1, coeff2, coeff3);
	rgba.a = 1.0;
	fragColor = vec4(rgba.r,rgba.g,rgba.b,rgba.a);
}

