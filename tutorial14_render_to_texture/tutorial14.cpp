// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
using namespace glm;
#include <glm/gtc/matrix_transform.hpp>
#include <common/Shader.h>
#include <common/FBO.h>
#include <common/save_screen.h>
#include <common/yuyv_to_rgb.h>
#include <common/baseGL.h>
#include "v4l2_base.h"
#include "v4l2_base.cc"

#define  WIDTH  1920
#define  HEIGHT 1080

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <pthread.h>

baseGL g_baseGL;

//v4l2
#define MM printf("[%s:%d]\n", __FILE__, __LINE__);

static char *dev_name;
static v4l2_base *v4l2base;
#define V4L2_BUF_COUNT       6
static struct buffer buffers[V4L2_BUF_COUNT];
static pthread_t v4l2_th;
static int g_curWrite;
static int g_curRead=-1;     //start cond
static int g_stop = 0;

void *v4l2_proc(void *dummy);

int nInitV4l2()
{
    struct v4l2_setting v4l2setting;
    int nbuf = V4L2_BUF_COUNT;
    int fcnt = 0;
    int i;
    
    dev_name = (char *)"/dev/video0";

    v4l2base = v4l2_open(dev_name);
    if (!v4l2base) {
        return -1;
    }
    
    memset(&v4l2setting, 0, sizeof(v4l2setting));
    memset(&buffers, 0, sizeof(buffers));
    v4l2setting.width = WIDTH;
    v4l2setting.height = HEIGHT;
    v4l2setting.format = V4L2_PIX_FMT_YUYV;
    v4l2setting.io = IO_MEMORY_USERPTR;
    if (v4l2_init(v4l2base, &v4l2setting) < 0)
        return -1;
    
    //alloc buffer
    for (i = 0; i < nbuf; ++i) {
        buffers[i].index = i;
        buffers[i].length = v4l2setting.width*v4l2setting.height*2;
	//YUYV
        buffers[i].start = malloc(v4l2setting.width*v4l2setting.height*2);
    }
    
    if (v4l2_req_buf(v4l2base, &nbuf, buffers) < 0)
        return -1;
    
    printf("req count: %d\n", nbuf);
    
	if (pthread_create(&v4l2_th, NULL, v4l2_proc, NULL)) {
		printf("Failed creating v4l2 thread\n");
		v4l2_th = (pthread_t) NULL;
		return -1;
	}
    return 0;
}


void *v4l2_proc(void *dummy)
{
	struct buffer *buf;
	if (v4l2_start_streaming(v4l2base) < 0) {
		printf("fail to start streaming\n");
		return NULL;
	}
	while( !g_stop ) {
		buf = v4l2_dqbuf(v4l2base);
		if (buf) {
			if( g_curWrite==g_curRead ) {
				printf("v4l2 buffer overflow\n"); //skip
			} else {
				//adv wptr
				if( ++g_curWrite>=V4L2_BUF_COUNT )
					g_curWrite = 0;
			}
			v4l2_qbuf(v4l2base, buf);
        	}
	}
	return NULL;
}
 
void finiV4l2()
{
	int i;
	void *result = NULL;
	if( v4l2_th ) {
		g_stop = 1;
		pthread_join(v4l2_th, &result);
	}
    if (v4l2_stop_streaming(v4l2base) < 0)
        return ;
    
    if (v4l2_uninit(v4l2base) < 0)
        return ;
    
    v4l2_close(v4l2base);
    
	//free buffer
	for (i = 0; i < V4L2_BUF_COUNT; ++i) {
		if( buffers[i].start ) {
			free(buffers[i].start);
			buffers[i].start=NULL;
		}
	}
    
}
//v4l2

int main( void )
{

	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( WIDTH, HEIGHT, "Tutorial 14 - Render To Texture", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	g_baseGL.getInfo();
    // We would expect width and height to be 1024 and 768
    int windowWidth = WIDTH;
    int windowHeight = HEIGHT;
    Shader StandardShadingRTT;
    cYUYV2RGBA yuv2rgb(1920, 1080, 1920, 1080);
    StandardShadingRTT.setDebug(1);

    // But on MacOS X with a retina screen it'll be 1024*2 and 768*2, so we get the actual framebuffer size:
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	} else {
		fprintf(stderr, "FramebufferSize = %d x %d\n", windowWidth, windowHeight);
	}

	if( nInitV4l2() ) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	yuv2rgb.setDebug(1);
	if( !yuv2rgb.Init() ) {
		fprintf(stderr, "Failed to initialize yuv shader\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	StandardShadingRTT.load((char *)"StandardShadingRTT");
	StandardShadingRTT.link();
	GLuint programID = StandardShadingRTT.getProg();

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	GLuint Texture = loadDDS("uvmap.DDS");
	
	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ("suzanne.obj", vertices, uvs, normals);

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	// Load it into a VBO

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

	// Get a handle for our "LightPosition" uniform
	StandardShadingRTT.use();
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");


	// ---------------------------------------------
	// Render to Texture - specific code begins here
	// ---------------------------------------------

	FBO fbo;

	if( !fbo.init(windowWidth, windowHeight) )
		return false;

	// The fullscreen quad's FBO
	static const GLfloat g_quad_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
	};

	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	// Create and compile our GLSL program from the shaders
	GLuint quad_programID = LoadShaders( "Passthrough.vertexshader", "WobblyTexture.fragmentshader" );
	GLuint texID = glGetUniformLocation(quad_programID, "renderedTexture");
	GLuint timeID = glGetUniformLocation(quad_programID, "time");
	GLuint tWidthID = glGetUniformLocation(quad_programID, "tWidth");
	GLuint tHeightID = glGetUniformLocation(quad_programID, "tHeight");
	int nbFrames = 0;
	double lastTime = glfwGetTime();
	double msgPeriodS = 3.0;
#if 0
	cSaveScrn save_screen(0, windowWidth, windowHeight, (char *)"screen%03d.ppm", 100);
#else
	cSaveScrn save_screen;
#endif
	
	do{
		// Render to our framebuffer
		fbo.bind();
		glViewport(0,0,windowWidth,windowHeight); // Render on the whole framebuffer, complete from the lower left corner to the upper right

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
/*		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix(); */
		glm::mat4 ProjectionMatrix = glm::mat4(1.0);
		glm::mat4 ViewMatrix = glm::mat4(1.0); 
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		glm::vec3 lightPos = glm::vec3(4,4,4);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);



		// Render to the screen

		GLuint renderedTexture = fbo.unbind();
		if( g_curRead<0 ) {
			//1st read point
			if( g_curWrite ) g_curRead = g_curWrite - 1;
			else g_curRead = V4L2_BUF_COUNT - 1;
			//printf("read start at %d\n", g_curRead);
		} else {
			if( ++g_curRead>=V4L2_BUF_COUNT )
				g_curRead = 0;
			//printf("read = %d\n", g_curRead);
		}
		yuv2rgb.Apply(buffers[g_curRead].start);
        // Render on the whole framebuffer, complete from the lower left corner to the upper right
		glViewport(0,0,windowWidth,windowHeight);

		// Clear the screen
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(quad_programID);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderedTexture);
		// Set our "renderedTexture" sampler to use Texture Unit 0
		glUniform1i(texID, 0);

		glUniform1f(timeID, (float)(glfwGetTime()*10.0f) );
		glUniform1i(tWidthID, windowWidth);
		glUniform1i(tHeightID, windowHeight);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the triangles !
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles

		glDisableVertexAttribArray(0);

		// Swap buffers
		glfwSwapBuffers(window);
		save_screen.nSave();
		nbFrames++;
		if ( glfwGetTime() - lastTime >= msgPeriodS ){ // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f fps\n", double(nbFrames)/msgPeriodS);
			nbFrames = 0;
			lastTime += msgPeriodS;
		}
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	finiV4l2();
	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteTextures(1, &Texture);
	fbo.destroy();
	StandardShadingRTT.destroy();

	glDeleteBuffers(1, &quad_vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	//todo wait thread

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

