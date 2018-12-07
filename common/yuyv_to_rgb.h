#ifndef __YUYV_TO_RGBA_H__
#define __YUYV_TO_RGBA_H__

#include "Shader.h"

static const char *g_vars[] = {
	  "inOverOut0"
	, "outWidth"
	, "outHeight"
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
		  IN_OVER_OUT0
		, OUT_WIDTH
		, OUT_HEIGHT
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
		, m_outHeight(outH){
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
		load((char *)"/opt/.gh/ogl/common/yuyv_to_rgba");
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
		glUniform2f(m_varId[IN_OVER_OUT0], (GLfloat) m_inWidth/m_outWidth,
			(GLfloat) m_inHeight/m_outHeight);
		glUniform1f(m_varId[OUT_WIDTH], (GLfloat) m_outWidth);
		glUniform1f(m_varId[OUT_HEIGHT], (GLfloat) m_outHeight);
                glUniform1f(m_varId[ONE_OVER_INx], 1.0/m_inWidth);
		//color
		glUniform1i(m_varId[COLOR709], is709);
		//texture
		glActiveTexture(GL_TEXTURE0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, m_inWidth, m_inHeight, 0, GL_RG, GL_UNSIGNED_BYTE, buf);
		glBindTexture(GL_TEXTURE_2D, m_inTexture);
		glUniform1i(m_varId[Y_TEX], 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		return 1;
	}
};

#endif	//__YUYV_TO_RGBA_H__
