#ifndef __Shader_h__
#define __Shader_h__

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


class Shader
{
#define FNAME_SIZE         1024
public:
	Shader(): m_dbgLevel(0)
		, m_program(0) {
		DBG(1, "Shader\n");
	}
	~Shader() {
		destroy();
		DBG(1, "~Shader\n");
	}

	void destroy() {
		delProg();
		if( m_program ) {
			DBG(1, "destroy\n");
			glDeleteProgram(m_program);
			m_program = 0;
		}
	}
	bool addProg(GLenum type, const GLchar *src) {
		DBG(1, "%s addProg %s\n", m_progName.c_str(),
			GL_FRAGMENT_SHADER==type?"FRAGMENT"\
			 : GL_VERTEX_SHADER==type?"VERTEX":"unknown");
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
		DBG(1, "link %s\n", m_progName.c_str());
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
		if (GL_FALSE==status) {
			DBG(0, "%s link fail\n", m_progName.c_str());
			return false;
		}
		return true;
	}
	void use() {
		GLenum glerr;
		DBG(2, "use %s\n", m_progName.c_str());
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
	void load(char *fileName, const char *vsh, const char *fsh) {
		m_progName = fileName;
		addProg(GL_VERTEX_SHADER, (char *)vsh);
		addProg(GL_FRAGMENT_SHADER, (char *)fsh);
	}
	void load(char *fileName) {
		const char* fileExt[] = {".fsh",".vsh"};
		static unsigned int shaderType[] = {GL_FRAGMENT_SHADER, GL_VERTEX_SHADER};
		FILE *f;

		m_progName = fileName;
		for (int i=0; i<2; i++){
			std::string shaderName = m_progName + fileExt[i];
			f = fopen(shaderName.c_str(), "rb");
			if (f) {
				fseek(f, 0, SEEK_END);
				long fileSize = ftell(f);
				fseek(f, 0, SEEK_SET);
				if (fileSize>0 ) {
					char *src = (char*)malloc(fileSize+1);
					memset(src,0,fileSize+1);
					long bytesRead = fread(src, 1, fileSize, f);
					DBG(1, "load %s, %ld B\n", shaderName.c_str(), bytesRead);
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
protected:
	GLuint               m_program;
	std::vector<GLuint>  m_shaders;
	int                  m_dbgLevel;
	std::string          m_progName;
	void delProg() {
		if( !m_shaders.size() ) return;
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
