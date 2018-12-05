#ifndef __BASEGL_H__
#define	__BASEGL_H__

#define GET_GL_INT(NAME)                            \
do {                                                \
	glGetIntegerv(NAME, &value);                \
	fprintf(stderr, "%s = %d\n", #NAME, value); \
} while( 0 )


#define GET_GL_STRING(NAME)                               \
do {                                                      \
	str = (char *)glGetString(NAME);                  \
	if( str )                                         \
		fprintf(stderr, "%s = %s\n", #NAME, str); \
	else                                              \
		fprintf(stderr, "%s = NULL\n", #NAME);    \
} while( 0 )


class baseGL {
public:
	baseGL() {}
	~baseGL() {}
	void getInfo() {
		char* str = 0;
		GLint value; 
		// get vendor string
		GET_GL_STRING(GL_VENDOR);
		// get renderer string
		GET_GL_STRING(GL_RENDERER);

		// get version string
		GET_GL_STRING(GL_VERSION);
		// get all extensions as a string
		GET_GL_STRING(GL_EXTENSIONS);

		// get number of color bits
		GET_GL_INT(GL_RED_BITS);
		GET_GL_INT(GL_GREEN_BITS);
		GET_GL_INT(GL_BLUE_BITS);
		GET_GL_INT(GL_ALPHA_BITS);

		// get depth bits
		GET_GL_INT(GL_DEPTH_BITS);

		// get stecil bits
		GET_GL_INT(GL_STENCIL_BITS);

		// get max number of lights allowed
		GET_GL_INT(GL_MAX_LIGHTS);
		// get max texture resolution
		GET_GL_INT(GL_MAX_TEXTURE_SIZE);

		// get max number of clipping planes
		GET_GL_INT(GL_MAX_CLIP_PLANES);

		// get max modelview and projection matrix stacks
		GET_GL_INT(GL_MAX_MODELVIEW_STACK_DEPTH);
		GET_GL_INT(GL_MAX_PROJECTION_STACK_DEPTH);
		GET_GL_INT(GL_MAX_ATTRIB_STACK_DEPTH);

		// get max texture stacks
		GET_GL_INT(GL_MAX_TEXTURE_STACK_DEPTH);
	}
	
};
#endif	//__BASEGL_H__
