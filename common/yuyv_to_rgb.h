#ifndef __YUYV_TO_RGBA_H__
#define __YUYV_TO_RGBA_H__

#include "Shader.h"

static const char *yuyv_to_rgb_vshader = "       \
#version 330 core                              \n\
                                               \n\
layout(location = 0) in vec3 a_position;       \n\
layout(location = 1) in vec2 a_texcoord;       \n\
out vec2 v_texcoord;                           \n\
                                               \n\
void main()                                    \n\
{                                              \n\
        gl_Position.xyz = a_position;          \n\
        gl_Position.w = 1;                     \n\
        v_texcoord = a_texcoord;               \n\
}                                              \n\
";


static const char *yuyv_to_rgb_fshader = "       \
#version 330 core                              \n\
                                               \n\
uniform float outWidth;                        \n\
uniform float oneOverInX;                      \n\
                                               \n\
uniform int color709;                          \n\
uniform sampler2D Ytex;                        \n\
                                               \n\
in vec2 v_texcoord;                            \n\
layout (location = 0) out vec4 fragColor;      \n\
                                               \n\
vec3 yuv_to_rgb (vec3 val, mat3 coff_yuv) {    \n\
    vec3 rgb;                                  \n\
    val += vec3(-0.0625, -0.5, -0.5);          \n\
    rgb.r = dot(val, coff_yuv[0]);             \n\
    rgb.g = dot(val, coff_yuv[1]);             \n\
    rgb.b = dot(val, coff_yuv[2]);             \n\
    return rgb;                                \n\
}                                              \n\
                                               \n\
void main (void) {                             \n\
    mat3 m_709 = mat3(                         \n\
        // first column                        \n\
        1.164,  0.000,  1.787,                 \n\
        // second column                       \n\
        1.164, -0.213, -0.531,                 \n\
        // third column                        \n\
        1.164,  2.112,  0.000                  \n\
    );                                         \n\
    mat3 m_601 = mat3(                         \n\
        // first column                        \n\
        1.164,  0.000,  1.596,                 \n\
        // second column                       \n\
        1.164, -0.391, -0.813,                 \n\
        // third column                        \n\
        1.164,  2.018,  0.000                  \n\
    );                                         \n\
    vec4 rgba, uv_texel;                       \n\
    vec3 yuv;                                  \n\
    float dx1 = -oneOverInX;                   \n\
    float dx2 = 0.0;                           \n\
    //Yn                                       \n\
    yuv.x = texture(Ytex, v_texcoord).r;       \n\
    float inorder =                            \n\
        mod (v_texcoord.x * outWidth, 2.0);    \n\
    if (inorder < 1.0) {                       \n\
        dx2 = -dx1;                            \n\
        dx1 = 0.0;                             \n\
    }                                          \n\
    //Y0U0, Y2U2                               \n\
    uv_texel.rg = texture(                     \n\
        Ytex, v_texcoord + vec2(dx1, 0.0)).rg; \n\
    //Y1V0, Y3V2                               \n\
    uv_texel.ba = texture(                     \n\
        Ytex, v_texcoord + vec2(dx2, 0.0)).rg; \n\
    //U0V0, U2V2                               \n\
    yuv.yz = uv_texel.ga;                      \n\
    if( color709!=0 )                          \n\
        rgba.rgb = yuv_to_rgb(yuv, m_709);     \n\
    else                                       \n\
        rgba.rgb = yuv_to_rgb(yuv, m_601);     \n\
    rgba.a = 1.0;                              \n\
    fragColor = vec4(                          \n\
        rgba.r,rgba.g,rgba.b,rgba.a);          \n\
}                                              \n\
";

#if 0
//output vertex is y vertex, and also coordinate
in vec4 a_position; 
in vec2 a_texcoord;
out vec2 v_texcoord;
void main()
{
  gl_Position = a_position;
  v_texcoord = a_texcoord;
}

//scale0 1:1, scale1 1:2
uniform vec2 tex_scale0;
uniform vec2 tex_scale1;
uniform vec2 tex_scale2;
uniform float width;
uniform float height;
uniform float poffset_x;
uniform float poffset_y;
uniform vec3 offset;
uniform vec3 coeff1;
uniform vec3 coeff2;
uniform vec3 coeff3;
uniform sampler2D Ytex, UVtex;

out vec4 fragColor;

vec3 yuv_to_rgb (vec3 val, vec3 offset, vec3 ycoeff, vec3 ucoeff, vec3 vcoeff) {
  vec3 rgb;
  val += offset;
  rgb.r = dot(val, ycoeff);
  rgb.g = dot(val, ucoeff);
  rgb.b = dot(val, vcoeff);
  return rgb;
}


in vec2 v_texcoord;
void main (void) {
vec2 texcoord;
texcoord = v_texcoord;
vec4 rgba;
vec3 yuv;
yuv.x=texture(Ytex, texcoord * tex_scale0).r;
yuv.yz=texture(UVtex, texcoord * tex_scale1).rg;
rgba.rgb = yuv_to_rgb (yuv, offset, coeff1, coeff2, coeff3);
rgba.a = 1.0;
fragColor=vec4(rgba.r,rgba.g,rgba.b,rgba.a);

}

#endif

static const char *g_vars[] = {
	  "outWidth"
	, "oneOverInX"
	, "color709"
	, "Ytex"
};

class cYUYV2RGBA : public Shader {
protected:
	GLuint               m_inTexture;
	int                  m_inWidth;
	int                  m_inHeight;
	int                  m_outWidth;
	int                  m_outHeight;
	//shader var
	GLuint               m_varId[10];
	GLuint               m_vao;
	enum {
		  ATTR_0_VERTEX
		, ATTR_1_TEXCOORD
		, ATTR_END
	};
	GLuint               m_vbo[ATTR_END];

public:
	//index of shader var
	enum {
		  OUT_WIDTH
		, ONE_OVER_INx
		, COLOR709
		, Y_TEX
	};
	cYUYV2RGBA(int inW, int inH, int outW, int outH)
		: Shader()
		, m_inTexture(0)
		, m_inWidth(inW)
		, m_inHeight(inH)
		, m_outWidth(outW)
		, m_outHeight(outH)
		{
		memset(m_varId, 0, sizeof(m_varId));
	}
	~cYUYV2RGBA() {
		if( m_inTexture ) glDeleteTextures(1, &m_inTexture);
	}
	int Init() {
		if( !m_inWidth || !m_inHeight ) {
			DBG(0, "input dimension error\n");
			return 0;
		}
		if( !m_outWidth || !m_outHeight ) {
			DBG(0, "output dimension error\n");
			return 0;
		}
		load( (   char *)"yuyv_to_rgba"
			, yuyv_to_rgb_vshader
			, yuyv_to_rgb_fshader
		);
		const GLfloat vertex[6][3] = {
			{ -1.0f, -1.0f, 0.0f }, /* BL */
			{  1.0f, -1.0f, 0.0f }, /* BR */
			{ -1.0f,  1.0f, 0.0f }, /* TL */
			{ -1.0f,  1.0f, 0.0f }, /* TL */
			{  1.0f, -1.0f, 0.0f }, /* BR */
			{  1.0,   1.0f, 0.0f }  /* TR */
		};

		//texture from UVC is TL first
		const GLfloat texcoord[6][2] = {
			{ 0.0f, 1.0f }, /* TL */
			{ 1.0f, 1.0f }, /* TR */
			{ 0.0f, 0.0f }, /* BL */
			{ 0.0f, 0.0f }, /* BL */
			{ 1.0f, 1.0f }, /* TR */
			{ 1.0f, 0.0f }  /* BR */
#if 0
			{ 0.0f, 0.0f }, /* BL */
			{ 1.0f, 0.0f }, /* BR */
			{ 0.0f, 1.0f }, /* TL */
			{ 0.0f, 1.0f }, /* TL */
			{ 1.0f, 0.0f }, /* BR */
			{ 1.0f, 1.0f }  /* TR */
#endif
		};
		 /* Allocate and assign a Vertex Array Object to our handle */
		glGenVertexArrays(1, &m_vao);
		/* Bind our Vertex Array Object as the current used object */
		glBindVertexArray(m_vao);
		/* Allocate and assign two Vertex Buffer Objects to our handle */
		glGenBuffers(ATTR_END, m_vbo);
		/* Bind our first VBO as being the active buffer and storing vertex attributes (coordinates) */
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		/* Copy the vertex data from diamond to our buffer */
		/* 8 * sizeof(GLfloat) is the size of the diamond array, since it contains 8 GLfloat values */
		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), vertex, GL_STATIC_DRAW);

		/* Specify that our coordinate data is going into attribute index 0, and contains 3 floats per vertex */
		glVertexAttribPointer(ATTR_0_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);

		/* Enable attribute index 0 as being used */
		glEnableVertexAttribArray(0);

		/* Bind our second VBO as being the active buffer and storing vertex attributes (colors) */
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		/* Copy the color data from colors to our buffer */
		/* 12 * sizeof(GLfloat) is the size of the colors array, since it contains 12 GLfloat values */
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), texcoord, GL_STATIC_DRAW);

		/* Specify that our color data is going into attribute index 1, and contains 2 floats per vertex */
		glVertexAttribPointer(ATTR_1_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);

		/* Enable attribute index 1 as being used */
		glEnableVertexAttribArray(1);
#if 0
		/* Bind attribute index 0 (coordinates) to in_Position and attribute index 1 (color) to in_Color */
		/* Attribute locations must be setup before calling glLinkProgram. */
		glBindAttribLocation(m_program, ATTR_0_VERTEX, "a_position");
		glBindAttribLocation(m_program, ATTR_1_TEXCOORD, "a_texcoord");/* Bind attribute index 0 (coordinates) to in_Position and attribute index 1 (color) to in_Color */
#endif
		if( false==link() ) {
			DBG(0, "fail to link shader\n");
			return 0;
		}
		use();
		GLuint prog = getProg();
		for( int i=0; i<sizeof(g_vars)/sizeof(g_vars[0]); ++i )
			m_varId[i] = glGetUniformLocation(prog, g_vars[i]);
		//texture prepare
		glGenTextures(1, &m_inTexture);
		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, m_inTexture);
		// Give an empty image to OpenGL ( the last "0" means "empty" )
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, m_inWidth, m_inHeight, 0, GL_RG, GL_UNSIGNED_BYTE, 0);
		// Poor filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		return 1;
	}
	int Apply(const GLvoid *buf=NULL, int is709=1) {
		use();
		glUniform1f(m_varId[OUT_WIDTH], (GLfloat) m_outWidth);
                glUniform1f(m_varId[ONE_OVER_INx], 1.0/m_inWidth);
		//color
		glUniform1i(m_varId[COLOR709], is709);
		//texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_inTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, m_inWidth, m_inHeight, 0, GL_RG, GL_UNSIGNED_BYTE, buf);
		glUniform1i(m_varId[Y_TEX], 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		return 1;
	}
};

#endif	//__YUYV_TO_RGBA_H__
