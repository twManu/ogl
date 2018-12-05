#ifndef __FBO_h__
#define __FBO_h__

#include <GL/gl.h>
//#include <OpenGLES/ES2/gl.h>
#include <iostream>
#include <vector>
#include <string.h>

#ifndef DBG
#define DBG(level, fmt, arg...)                     \
	do {                                        \
		if( m_dbgLevel>=level )             \
			fprintf(stderr, fmt, ##arg);\
	} while( 0 )
#endif //DBG


class FBO
{
public:
	FBO(): m_dbgLevel(0)
		, m_fboName(0)
		, m_renderTexture(0)
		, m_width(0) 
		, m_height(0) {
		DBG(1, "FBO\n");
	}

	~FBO() {
		destroy();
		DBG(1, "~FBO\n");
	}
	void destroy() {
		if( m_fboName ) {
			DBG(1, "destroy\n");
			glDeleteFramebuffers(1, &m_fboName);
			m_fboName = 0;
		}
		if( m_renderTexture ) {
        		glDeleteTextures(1, &m_renderTexture);
			m_renderTexture = 0;
		}
		//glDeleteRenderbuffers(1, &depthrenderbuffer);
	}
	// Ret : true, if successful
	//       false, otherwise     
	int init(int width, int height/*, filter, clamp*/) {
		// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
		glGenFramebuffers(1, &m_fboName);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fboName);

		// The texture we're going to render to
		glGenTextures(1, &m_renderTexture);

		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, m_renderTexture);
		// Give an empty image to OpenGL ( the last "0" means "empty" )
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		m_width = width;
		m_height = height;
		// Poor filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
/*
		// The depth buffer
		GLuint depthrenderbuffer;
		glGenRenderbuffers(1, &depthrenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, windowWidth, windowHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

		//// Alternative : Depth texture. Slower, but you can sample it later in your shader
		//GLuint depthTexture;
		//glGenTextures(1, &depthTexture);
		//glBindTexture(GL_TEXTURE_2D, depthTexture);
		//glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT24, 1024, 768, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
 */
		// Set "renderedTexture" as our colour attachement #0
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_renderTexture, 0);
/*
		//// Depth texture alternative : 
		//glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
 */
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers); //"1" is the size of DrawBuffers
		// Always check that our framebuffer is ok
		return (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	}
	void bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, m_fboName);
	}
	//Ret : texture rendered
	GLuint unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return m_renderTexture;
	}
private:
	int                  m_dbgLevel;
	GLuint               m_fboName;
	GLuint               m_renderTexture;
	int                  m_width;
	int                  m_height;
};

#endif	//__FBO_h__
