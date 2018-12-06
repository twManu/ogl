#ifndef __YUYV_TO_RGBA_H__
#define __YUYV_TO_RGBA_H__

#include "Shader.h"

static const char *g_vars[] = {
	  "inOverOut0"
	, "outWidth"
	, "outHeight"
	, "oneOverInX"
	, "offset"
	, "coeff1"
	, "coeff2"
	, "coeff3"
	, "Ytex"
};

/* BT. 601 standard with the following ranges:
 * Y = [16..235] (of 255)
 * Cb/Cr = [16..240] (of 255)
 */
static const GLfloat from_yuv_bt601_offset[] = {-0.0625f, -0.5f, -0.5f};
static const GLfloat from_yuv_bt601_rcoeff[] = {1.164f, 0.000f, 1.596f};
static const GLfloat from_yuv_bt601_gcoeff[] = {1.164f,-0.391f,-0.813f};
static const GLfloat from_yuv_bt601_bcoeff[] = {1.164f, 2.018f, 0.000f};

/* BT. 709 standard with the following ranges:
 * Y = [16..235] (of 255)
 * Cb/Cr = [16..240] (of 255)
 */
static const GLfloat from_yuv_bt709_offset[] = {-0.0625f, -0.5f, -0.5f};
static const GLfloat from_yuv_bt709_rcoeff[] = {1.164f, 0.000f, 1.787f};
static const GLfloat from_yuv_bt709_gcoeff[] = {1.164f,-0.213f,-0.531f};
static const GLfloat from_yuv_bt709_bcoeff[] = {1.164f,2.112f, 0.000f};

class cYUYV2RGBA : public Shader {
protected:
	GLuint               m_inTexture;
	int                  m_inWidth;
	int                  m_inHeight;
	int                  m_outWidth;
	int                  m_outHeight;
	//shader var
	GLuint               m_varId[10];

public:
	//index of shader var
	enum {
		  IN_OVER_OUT0
		, OUT_WIDTH
		, OUT_HEIGHT
		, ONE_OVER_INx
		, OFFSET
		, COEFF1
		, COEFF2
		, COEFF3
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
		if( is709 ) {
			glUniform3fv(m_varId[OFFSET], 1, from_yuv_bt709_offset);
			glUniform3fv(m_varId[COEFF1], 1, from_yuv_bt709_rcoeff);
			glUniform3fv(m_varId[COEFF2], 1, from_yuv_bt709_gcoeff);
			glUniform3fv(m_varId[COEFF3], 1, from_yuv_bt709_bcoeff);
		} else {
			glUniform3fv(m_varId[OFFSET], 1, from_yuv_bt601_offset);
			glUniform3fv(m_varId[COEFF1], 1, from_yuv_bt601_rcoeff);
			glUniform3fv(m_varId[COEFF2], 1, from_yuv_bt601_gcoeff);
			glUniform3fv(m_varId[COEFF3], 1, from_yuv_bt601_bcoeff);
		}
		//texture
		glActiveTexture(GL_TEXTURE0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, m_inWidth, m_inHeight, 0, GL_RG, GL_UNSIGNED_BYTE, buf);
		glBindTexture(GL_TEXTURE_2D, m_inTexture);
		glUniform1i(m_varId[Y_TEX], 0);
		return 1;
	}
};

#endif	//__YUYV_TO_RGBA_H__
