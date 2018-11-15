#ifndef __Shader_h__
#define __Shader_h__

#include <GL/gl.h>
//#include <OpenGLES/ES2/gl.h>
#include <iostream>
#include <vector>
#include <string.h>


#define DBG(level, fmt, arg...)                     \
	do {                                        \
		if( level>=m_dbgLevel )             \
			fprintf(stderr, fmt, ##arg);\
	} while( 0 )


class Shader
{
public:
	Shader(): m_dbgLevel(0)
		, m_program(0) {
		DBG(1, "Shader\n");
	}
	~Shader() {
		DBG(1, "~Shader\n");
		delProg();
		glDeleteProgram(m_program);
	}

	bool addProg(GLenum type, const GLchar *src) {
		DBG(1, "addProg %d\n", type);
		if( !m_program ) {
			m_program = glCreateProgram();
			if( !m_program ) {
				DBG(0, "Shader program not created.\n");
				return false;
			}
		}
		GLuint shader = glCreateShader(type);
		if( !shader ){
			DBG(0, "glCreateShader ERROR\n");
			return false;
		}
		glShaderSource(shader, 1, &src, NULL);
		glCompileShader(shader);
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			GLchar *log = (GLchar *)malloc(logLength);
			glGetShaderInfoLog(shader, logLength, &logLength, log);
			DBG(0, "Shader compile log: %s\n", log);
			free(log);
		}
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			glDeleteShader(shader);
			DBG(0, "Shader compilation error.\n");
			return false;
		}
		m_shaders.push_back(shader);
		glAttachShader(m_program, shader);
		GLenum glerr = glGetError();
		if (glerr == GL_INVALID_VALUE) 
			DBG(0, "glAttachShader GL_INVALID_VALUE\n");
		
		if (glerr == GL_INVALID_OPERATION)
			DBG(0, "glAttachShader GL_INVALID_OPERATION\n");
		return true;
	}
	bool link() {
		GLint status;
		DBG(1, "link\n");
		if( !m_program ) {
			DBG(0, "Shader program not created.\n");
			return false;
		}
		glLinkProgram(m_program);
		GLint logLength;
		glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			GLchar *log = (GLchar *)malloc(logLength);
			glGetProgramInfoLog(m_program, logLength, &logLength, log);
			DBG(0, "Program link log: %s\n", log);
			free(log);
		}
		glGetProgramiv(m_program, GL_LINK_STATUS, &status);
		if (status == 0) {
			return false;
		}
		return true;
	}
	void use() {
		GLenum glerr;
		DBG(1, "use\n");
		glUseProgram(m_program);
		glerr = glGetError();
		if (glerr == GL_INVALID_VALUE) {
			std::cout <<"glUseProgram GL_INVALID_VALUE"<<std::endl;
		}
		if (glerr == GL_INVALID_OPERATION) {
			std::cout <<"glUseProgram GL_INVALID_OPERATION"<<std::endl;
		}
	}
	void setDebug(int level) { m_dbgLevel = level; }
	void load(char *fileName) {
		const char* fileExt[] = {".fsh",".vsh"};
		static unsigned int shaderType[] = {GL_FRAGMENT_SHADER, GL_VERTEX_SHADER};
		FILE *f;
		DBG(1, "load %s\n", fileName);
		for (int i=0; i<2; i++){
			std::string shaderName = std::string(fileName) + fileExt[i];
			f = fopen(shaderName.c_str(), "rb");
			if (f) {
				fseek(f, 0, SEEK_END);
				long fileSize = ftell(f);
				fseek(f, 0, SEEK_SET);
				if (fileSize>0 ) {
					char *src = (char*)malloc(fileSize+1);
					memset(src,0,fileSize+1);
					long bytesRead = fread(src, 1, fileSize, f);
					if (bytesRead == fileSize) {
						addProg(shaderType[i], src);
					} else {
						std::cout <<"File IO error: "<<stderr<<std::endl;
					}
					free(src);
					fclose(f);
				}
			} else {
				std::cout <<"File missing: "<< shaderName <<std::endl;
			}
		}
	}
	GLuint getProg() {
		return m_program;
	}
private:
	GLuint               m_program;
	std::vector<GLuint>  m_shaders;
	int                  m_dbgLevel;
	void delProg() {
		DBG(1, "delProg\n");
		for (std::vector<GLuint>::iterator i = m_shaders.begin() ;
			i != m_shaders.end();
			++i) {
			glDeleteShader(*i);
		}
		m_shaders.clear();
	}
	void detach() {
		DBG(1, "detach\n");
		for (std::vector<GLuint>::iterator i = m_shaders.begin() ;
			i != m_shaders.end(); ++i) {
			glDetachShader(m_program, *i);
		}
	}
};

#endif	//__Shader_h__
