#ifndef __Shader_h__
#define __Shader_h__

#include <GL/gl.h>
//#include <OpenGLES/ES2/gl.h>
#include <iostream>
#include <vector>
#include <string.h>

class Shader
{
public:
	Shader(void) {
		m_program = glCreateProgram();
		if( !m_program ) {
			std::cout<<"glCreateProgram ERROR"<<std::endl;
		} else {
			std::cout<<"Shader program id "<<m_program<<std::endl;
		}
	}
	~Shader(void) {
		delProg();
		glDeleteProgram(m_program);
	}

	bool addProg(GLenum type, const GLchar *src) {
		if( !m_program ) {
			std::cout << "Shader program not created." << std::endl;
			return false;
		}
		GLuint shader = glCreateShader(type);
		if( !shader ){
			std::cout<<"glCreateShader ERROR"<<std::endl;
		}
		glShaderSource(shader, 1, &src, NULL);
		glCompileShader(shader);
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			GLchar *log = (GLchar *)malloc(logLength);
			glGetShaderInfoLog(shader, logLength, &logLength, log);
			std::cout << "Shader compile log: "<< std::endl << log << std::endl;
			free(log);
		}
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			glDeleteShader(shader);
			std::cout << "Shader compilation error." << std::endl;
			return false;
		}
		m_shaders.push_back(shader);
		glAttachShader(m_program, shader);
		GLenum glerr = glGetError();
		if (glerr == GL_INVALID_VALUE) {
			std::cout <<"glAttachShader GL_INVALID_VALUE"<<std::endl;
		}
		if (glerr == GL_INVALID_OPERATION){
			std::cout <<"glAttachShader GL_INVALID_OPERATION"<<std::endl;
		}
		return true;
	}
	bool link() {
		GLint status;
		if( !m_program ) {
			std::cout << "Shader program not created." << std::endl;
			return false;
		}
		glLinkProgram(m_program);
		GLint logLength;
		glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			GLchar *log = (GLchar *)malloc(logLength);
			glGetProgramInfoLog(m_program, logLength, &logLength, log);
			std::cout << "Program link log:"<< std::endl << log << std::endl;
			free(log);
		}
		glGetProgramiv(m_program, GL_LINK_STATUS, &status);
		if (status == 0) {
			return false;
		}
		detach();
		return true;
	}
	void use() {
		GLenum glerr = glGetError();
		glUseProgram(m_program);
		glerr = glGetError();
		if (glerr == GL_INVALID_VALUE) {
			std::cout <<"glUseProgram GL_INVALID_VALUE"<<std::endl;
		}
		if (glerr == GL_INVALID_OPERATION) {
			std::cout <<"glUseProgram GL_INVALID_OPERATION"<<std::endl;
		}
	}
	void load(char *fileName) {
		const char* fileExt[] = {".fsh",".vsh"};
		static unsigned int shaderType[] = {GL_FRAGMENT_SHADER, GL_VERTEX_SHADER};
		FILE *f;
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
	void delProg() {
		for (std::vector<GLuint>::iterator i = m_shaders.begin() ;
			i != m_shaders.end();
			++i) {
			glDeleteShader(*i);
		}
		m_shaders.clear();
	}
	void detach() {
		for (std::vector<GLuint>::iterator i = m_shaders.begin() ;
			i != m_shaders.end(); ++i) {
			glDetachShader(m_program, *i);
		}
	}
};

#endif	//__Shader_h__
