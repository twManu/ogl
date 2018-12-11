/*
 *  1:1 color convert
 */
#ifndef __YUYV_TO_RGBA_H__
#define __YUYV_TO_RGBA_H__

#include "Shader.h"

/* 0.29, 0.29 for GSM 
   0.455, 0.396 for Green side
   Blue side
    if( distance(val.yz, vec2(0.698, 0.337))<0.2 ) \n\
 */

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

static const char *yuv_declare_fsh = "\
#version 330 core                              \n\
uniform float inWidth;                         \n\
uniform float maskOutOff;                      \n\
uniform int color709;                          \n\
uniform sampler2D Ytex;                        \n\
uniform sampler2D Utex;                        \n\
                                               \n\
in vec2 v_texcoord;                            \n\
layout (location = 0) out vec4 fragColor;      \n\
";


static const char *yuv_2_rgb_fsh = "\
vec3 yuv_to_rgb (vec3 val, mat3 coff_yuv) {    \n\
    vec3 rgb;                                  \n\
    val += vec3(-0.0625, -0.5, -0.5);          \n\
    rgb.r = dot(val, coff_yuv[0]);             \n\
    rgb.g = dot(val, coff_yuv[1]);             \n\
    rgb.b = dot(val, coff_yuv[2]);             \n\
    return rgb;                                \n\
}";


/*
 * 1st-var = u value
 * 2nd-var = v value
 * 3rd-var = uv-distance
 * 4th-var = y value
 * 5th-var = y-distance
 * 6th-var = r-replace
 * 7th-var = g-replace
 * 8th-var = b-replace
 */
static const char *yuv_2_rgb_mask_fsh = "\
vec3 yuv_to_rgb (vec3 val, mat3 coff_yuv) {    \n\
    vec3 rgb;                                  \n\
    if( distance(val.gb,vec2(%f,%f))<%f &&     \n\
        distance(val.r,%f)<%f )                \n\
        return vec3(%f,%f,%f);                 \n\
    val += vec3(-0.0625, -0.5, -0.5);          \n\
    rgb.r = dot(val, coff_yuv[0]);             \n\
    rgb.g = dot(val, coff_yuv[1]);             \n\
    rgb.b = dot(val, coff_yuv[2]);             \n\
    return rgb;                                \n\
}";

static const char *yuyv_main_fsh = "\
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
    float dx1 = -1/inWidth;                    \n\
    float dx2 = 0.0;                           \n\
    if( maskOutOff<=distance(vec2(0), vec2(0.5)) &&\n\
        distance(v_texcoord, vec2(0.5))        \n\
        >maskOutOff ) fragColor=vec4(0,0,0,1); \n\
    else {                                     \n\
        //Yn                                   \n\
        yuv.x = texture(Ytex, v_texcoord).r;   \n\
        float inorder =                        \n\
            mod (v_texcoord.x * inWidth, 2.0); \n\
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
}                                                  \n\
";

static const char *yuyv_fsh[] = {
	  yuv_declare_fsh
	, yuv_2_rgb_fsh
	, yuyv_main_fsh
};


static const char *nv12_main_fsh = "\
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
    vec4 rgba;                                 \n\
    vec3 yuv;                                  \n\
    if( maskOutOff<=distance(vec2(0), vec2(0.5)) &&\n\
        distance(v_texcoord, vec2(0.5))        \n\
        >maskOutOff ) fragColor=vec4(0,0,0,1); \n\
    else {                                     \n\
        //Yn                                       \n\
        yuv.x = texture(Ytex, v_texcoord).r;       \n\
        yuv.yz = texture(Utex, v_texcoord).rg;     \n\
        if( color709!=0 )                          \n\
            rgba.rgb = yuv_to_rgb(yuv, m_709);     \n\
        else                                       \n\
            rgba.rgb = yuv_to_rgb(yuv, m_601);     \n\
        rgba.a = 1.0;                              \n\
        fragColor = vec4(                          \n\
            rgba.r,rgba.g,rgba.b,rgba.a);          \n\
    }                                              \n\
}                                                  \n\
";

static const char *nv12_fsh[] = {
	  yuv_declare_fsh
	, yuv_2_rgb_fsh
	, nv12_main_fsh
};


static const char *g_vars[] = {
	  "inWidth"
	, "maskOutOff"
	, "color709"
	, "Ytex"
	, "Utex"            //NV12 use while YUYV not
};


typedef enum {
	  FMT_NV12
	, FMT_YUYV
} eYUV_Fmt;


class cYUYV2RGBA : public Shader {
protected:
	int                  m_inWidth;
	int                  m_inHeight;
	//shader var
	GLuint               m_varId[10];
	GLuint               m_vao;
	enum {
		  ATTR_0_VERTEX
		, ATTR_1_TEXCOORD
		, ATTR_END
	};
	typedef struct fmtDesc {
		GLuint           texIndex;
		int              width;
		int              height;
		GLuint           fmt;
		GLuint           size;
		int              uniformIndex;
	} fmtDesc;
	fmtDesc              m_fmtDesc[3];
	GLuint               m_inTexture[3];
	GLuint               m_vbo[ATTR_END];
	eYUV_Fmt             m_pixfmt;
	int                  m_texCount;

public:
	//index of shader var
	enum {
		  IN_WIDTH
		, MASK_OUT_OFF
		, COLOR709
		, Y_TEX
		, U_TEX
	};
	cYUYV2RGBA(int inW, int inH, eYUV_Fmt fmt=FMT_YUYV)
		: Shader()
		, m_inWidth(inW)
		, m_inHeight(inH)
		, m_pixfmt(fmt)
		, m_texCount(0) {
		memset(m_inTexture, 0, sizeof(m_inTexture));
		memset(m_varId, 0, sizeof(m_varId));
	}
	~cYUYV2RGBA() {
		if( m_inTexture )
			glDeleteTextures(m_texCount, m_inTexture);
	}
	/*
	 * replace 2nd as yuv_2_rgb_mask_fsh 
	 * mask_param needs 8 floats
	 */
	char *composeShader(const char *progs[], int count, float *mask_param=NULL) {
		int i, len;
		char *buf;
		char *maskBuf=NULL;
		for( i=len=0; i<count; ++i ) {
			if( 1==i && mask_param ) {  //todo hardcoding
				int maskLen=strlen(yuv_2_rgb_mask_fsh) + 8*6 + 2;
				maskBuf = (char*) malloc(maskLen);
				if( !maskBuf ) {
					DBG(0, "fail to allocate mask buffer\n");
					return NULL;
				}
				sprintf(maskBuf, yuv_2_rgb_mask_fsh\
					,mask_param[0]
					,mask_param[1]
					,mask_param[2]
					,mask_param[3]
					,mask_param[4]
					,mask_param[5]
					,mask_param[6]
					,mask_param[7]);
				len += maskLen;
			} else len += strlen(progs[i])+2;
		}
		buf = (char *)malloc(len);
		if( !buf ) {
			if( maskBuf ) free(maskBuf);
			return buf;    //error
		}
		for( buf[0]=0, i=0; i<count; ++i ) {
			if( NULL==progs[i] ) continue;
			if( 1==i && maskBuf ) {
				strcat(buf, maskBuf);
				free(maskBuf);
				maskBuf = NULL;
			} else strcat(buf, progs[i]);
			//skip last
			if( i!=(count-1) )
				strcat(buf, "\n");
		}
		printf("%s\n", buf);
		return buf;
	}
	//masks are 8 parameters
	int Init(bool linear=1, float *masks=NULL) {
		if( !m_inWidth || !m_inHeight ) {
			DBG(0, "input dimension error\n");
			return 0;
		}
		char *tmp_fsh = NULL;
		switch ( m_pixfmt ) {
		case FMT_NV12:
			m_texCount = 2;
			m_fmtDesc[0].texIndex = GL_TEXTURE0;
			m_fmtDesc[0].fmt = GL_RED;
			m_fmtDesc[0].width = m_inWidth;
			m_fmtDesc[0].height = m_inHeight;
			m_fmtDesc[0].size = GL_UNSIGNED_BYTE;
			m_fmtDesc[0].uniformIndex = Y_TEX;

			m_fmtDesc[1].texIndex = GL_TEXTURE1;
			m_fmtDesc[1].fmt = GL_RG;
			m_fmtDesc[1].width = m_inWidth/2;
			m_fmtDesc[1].height = m_inHeight/2;
			m_fmtDesc[1].size = GL_UNSIGNED_BYTE;
			m_fmtDesc[1].uniformIndex = U_TEX;
			tmp_fsh = composeShader(nv12_fsh, sizeof(nv12_fsh)/sizeof(nv12_fsh[0]), masks);
			if( !tmp_fsh ) {
				DBG(0, "fail to allocate shader\n");
				return 0;
			}
			load( (char *)"nv12_to_rgba"
				, yuyv_to_rgb_vshader
				, tmp_fsh
			);
			break;

		case FMT_YUYV:
			m_texCount = 1;
			m_fmtDesc[0].texIndex = GL_TEXTURE0;
			m_fmtDesc[0].fmt = GL_RG;
			m_fmtDesc[0].width = m_inWidth;
			m_fmtDesc[0].height = m_inHeight;
			m_fmtDesc[0].size = GL_UNSIGNED_BYTE;
			m_fmtDesc[0].uniformIndex = Y_TEX;
			tmp_fsh = composeShader(yuyv_fsh, sizeof(yuyv_fsh)/sizeof(yuyv_fsh[0]), masks);
			if( !tmp_fsh ) {
				DBG(0, "fail to allocate shader\n");
				return 0;
			}
			load( (char *)"yuyv_to_rgba"
				, yuyv_to_rgb_vshader
				, tmp_fsh
			);
			break;
		default:
			DBG(0, "unknow yuv format\n");
			return 0;
		}
		
		if( tmp_fsh ) free(tmp_fsh);
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
			{ 0.0f, 1.0f }, /* BL */
			{ 1.0f, 1.0f }, /* BR */
			{ 0.0f, 0.0f }, /* TL */
			{ 0.0f, 0.0f }, /* TL */
			{ 1.0f, 1.0f }, /* BR */
			{ 1.0f, 0.0f }  /* TR */
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
		#if 0
		glEnableVertexAttribArray(0);
		#endif

		/* Bind our second VBO as being the active buffer and storing vertex attributes (colors) */
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		/* Copy the color data from colors to our buffer */
		/* 12 * sizeof(GLfloat) is the size of the colors array, since it contains 12 GLfloat values */
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), texcoord, GL_STATIC_DRAW);

		/* Specify that our color data is going into attribute index 1, and contains 2 floats per vertex */
		glVertexAttribPointer(ATTR_1_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);
		#if 0
		/* Enable attribute index 1 as being used */
		glEnableVertexAttribArray(1);
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
		glGenTextures(m_texCount, m_inTexture);
		for( int i=0; i<m_texCount; ++i ) {
			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, m_inTexture[i]);
			// Give an empty image to OpenGL ( the last "0" means "empty" )
			glTexImage2D(GL_TEXTURE_2D
					,0
					, m_fmtDesc[i].fmt
					, m_fmtDesc[i].width
					, m_fmtDesc[i].height
					, 0
					, m_fmtDesc[i].fmt
					, m_fmtDesc[i].size
					, 0);
			// Poor filtering
			if( linear ) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			} else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		return 1;
	}
	int Apply(const GLvoid *buf=NULL, const GLvoid *buf2=NULL, float maskOut=1.0, int is709=1) {
		use();
		glUniform1f(m_varId[IN_WIDTH], (GLfloat) m_inWidth);
		glUniform1f(m_varId[MASK_OUT_OFF], maskOut);
		//color
		glUniform1i(m_varId[COLOR709], is709);
		//vertex
		glBindVertexArray(m_vao);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glVertexAttribPointer(ATTR_0_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glVertexAttribPointer(ATTR_1_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);

		//texture
		for( int i=0; i<m_texCount; ++i ) {
			glActiveTexture(m_fmtDesc[i].texIndex);
			glBindTexture(GL_TEXTURE_2D, m_inTexture[i]);
			glTexImage2D(GL_TEXTURE_2D
					,0
					, m_fmtDesc[i].fmt
					, m_fmtDesc[i].width
					, m_fmtDesc[i].height
					, 0
					, m_fmtDesc[i].fmt
					, m_fmtDesc[i].size
					, 0==i?buf:buf2);
			glUniform1i(m_varId[m_fmtDesc[i].uniformIndex], i);
		}

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		return 1;
	}
};

#endif	//__YUYV_TO_RGBA_H__
