// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <getopt.h>

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

static int g_width = 1920;
static int g_height = 1080;
static int g_devNr = 0;
static eYUV_Fmt g_fmt = FMT_YUYV;
static char g_dev_name[30];
static int g_masked = -1;
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
#define MASK_SIZE          2
static float g_masks[MASK_SIZE][5] = {
	//blue
	{ 0.6667, 0.7, 1.0, 0.0, 0.0 },
	//green
	{ 0.3333, 0.7, 1.0, 0.0, 0.0 }
};


#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <pthread.h>
#include <list>

baseGL g_baseGL;

//v4l2
#define MM printf("[%s:%d]\n", __FILE__, __LINE__);

static char *dev_name;
static v4l2_base *v4l2base;
#define V4L2_BUF_COUNT       6
#define CAP_FORMAT         V4L2_PIX_FMT_YUYV
//#define CAP_FORMAT         V4L2_PIX_FMT_NV12

static struct buffer buffers[V4L2_BUF_COUNT];
static pthread_t v4l2_th;
static int g_stop = 0;
pthread_mutex_t g_freeLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_useLock = PTHREAD_MUTEX_INITIALIZER;
//index
std::list<int>       g_free;
std::list<int>       g_use;


static void checkParm(int argc, char *argv[])
{
	int opt;

	while( (opt = getopt(argc, argv, "hW:H:f:m:n:")) != -1) {
		switch(opt) {
		case 'W':
			g_width = atoi(optarg);
			break;
		case 'H':
			g_height = atoi(optarg);
			break;
		case 'f':
			g_fmt = (eYUV_Fmt)atoi(optarg);
			break;
		case 'n':
			g_devNr = atoi(optarg);
			break;
		case 'm':
			g_masked = atoi(optarg);
#if 1 //mark to test 1-9
			if( g_masked>=MASK_SIZE ) {
				printf("invalid mask index\n");
				g_masked = -1;
			}
#endif
			break;
		case 'h':
			printf("Usage: tutorial14 [-W WIDTH] [-H HEIGHT] [-f FMT] [-n DEV_NR] [-m INDEX]\n");
			printf("\t -W WIDTH           source width, default=1920\n");
			printf("\t -H HEIGHT          source height, default=1080\n");
			printf("\t -n DEV_NR          device number, default=0 (/dev/video0)\n");
			printf("\t -f FMT             source format, default=YUYV\n");
			printf("\t          0   NV12\n");
			printf("\t          1   YUYV\n");
			printf("\t -m INDEX           index param for blue=0, green=1\n");
			abort();
			break;
		default:
			printf("unknown option");
			abort();
			break;
		}
	}
}

void *v4l2_proc(void *dummy);

int nInitV4l2()
{
    struct v4l2_setting v4l2setting;
    int nbuf = V4L2_BUF_COUNT;
    int fcnt = 0;
    int i;
    
    sprintf(g_dev_name, "/dev/video%d", g_devNr);
    v4l2base = v4l2_open(g_dev_name);
    if (!v4l2base) {
        return -1;
    }
    
    memset(&v4l2setting, 0, sizeof(v4l2setting));
    memset(&buffers, 0, sizeof(buffers));
    v4l2setting.width = g_width;
    v4l2setting.height = g_height;
    v4l2setting.format = (FMT_YUYV==g_fmt) ? V4L2_PIX_FMT_YUYV : V4L2_PIX_FMT_NV12;
    v4l2setting.io = IO_MEMORY_USERPTR;
    if (v4l2_init(v4l2base, &v4l2setting) < 0)
        return -1;
    
    //alloc buffer
    for (i = 0; i < nbuf; ++i) {
        buffers[i].index = i;
	//YUYV
        buffers[i].length = v4l2setting.width*v4l2setting.height*2;
        buffers[i].start = malloc(buffers[i].length);
	if( !buffers[i].start ) {
		printf("fail to allocate buffer\n");
		return -1;
	}
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
	int index;

	if (v4l2_start_streaming(v4l2base) < 0) {
		printf("fail to start streaming\n");
		return NULL;
	}
	while( !g_stop ) {
		buf = v4l2_dqbuf(v4l2base);
		if (buf) {
			pthread_mutex_lock(&g_useLock);
			g_use.push_back(buf->index);
			pthread_mutex_unlock(&g_useLock);
		}
		if( g_free.size() ) {
			pthread_mutex_lock(&g_freeLock);
			while( g_free.size() ) {
				index = *(g_free.begin());
				g_free.pop_front();
				v4l2_qbuf(v4l2base, buffers+index);
			}
			pthread_mutex_unlock(&g_freeLock);
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

static void RGB2HSV(float r, float g, float b,
                    float &h, float &s, float &v)
{
    float rgb_max = std::max(r, std::max(g, b));
    float rgb_min = std::min(r, std::min(g, b));
    float delta = rgb_max - rgb_min;
    s = delta / (rgb_max + 1e-20f);
    v = rgb_max;

    float hue;
    if (r == rgb_max)
        hue = (g - b) / (delta + 1e-20f);
    else if (g == rgb_max)
        hue = 2 + (b - r) / (delta + 1e-20f);
    else
        hue = 4 + (r - g) / (delta + 1e-20f);
    if (hue < 0)
        hue += 6.f;
    h = hue * (1.f / 6.f);
}

int main(int argc, char *argv[] )
{
	checkParm(argc, argv);
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
	window = glfwCreateWindow( g_width, g_height, "Tutorial 14 - Render To Texture", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	g_baseGL.getInfo();
    // We would expect width and height to be 1024 and 768
    int windowWidth = g_width;
    int windowHeight = g_height;

    // But on MacOS X with a retina screen it'll be 1024*2 and 768*2, so we get the actual framebuffer size:
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    cYUYV2RGBA yuv2rgb(g_width, g_height, g_fmt);

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
	bool success;
#if 0
	float tmp[4] = { 0.0, 1.0, 0.0, 0.0 };
	if( g_masked>=0 ) {
		tmp[0] = 1.0/g_masked;
		success = yuv2rgb.Init(1, tmp);
	}
#else
	if( g_masked>=0 ) success = yuv2rgb.Init(1, g_masks[g_masked]);
#endif
	else success = yuv2rgb.Init();
	if( !success ) {
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
    glfwSetCursorPos(window, g_width/2, g_height/2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


	// Get a handle for our "LightPosition" uniform


	// ---------------------------------------------
	// Render to Texture - specific code begins here
	// ---------------------------------------------

#if 0
	// The fullscreen quad's FBO
	static const GLfloat g_quad_vertex_buffer_data[] = {
               -1.0f, -1.0f, 0.0f,
                1.0f, -1.0f, 0.0f,
               -1.0f,  1.0f, 0.0f,
               -1.0f,  1.0f, 0.0f,
                1.0f, -1.0f, 0.0f,
                1.0f,  1.0f, 0.0f
	};

	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
#endif
	// Create and compile our GLSL program from the shaders
	int nbFrames = 0;
	double lastTime = glfwGetTime();
	double msgPeriodS = 3.0;
#if 1
	cSaveScrn save_screen(0, g_width, g_height, (char *)"screen%03d.ppm", 100);
#else
	cSaveScrn save_screen;
#endif
	
	unsigned char *curBuf=NULL;
	do{
		int index;
		// Render to our framebuffer
		glViewport(0,0,windowWidth,windowHeight); // Render on the whole framebuffer, complete from the lower left corner to the upper right

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// Use our shader

		// Compute the MVP matrix from keyboard and mouse input

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform

/*
		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			a_positionID,       // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);


		// 2nd text : vertices
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			a_texcoordID,       // attribute
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
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

*/


		// Render to the screen
		index = -1;

		pthread_mutex_lock(&g_useLock);
		if( g_use.size() ) {
			index = *(g_use.begin());
			g_use.pop_front();
			curBuf=(unsigned char *)buffers[index].start;
		}
		pthread_mutex_unlock(&g_useLock);
#if 0
		float ratio = ((float) (nbFrames % 60))/60;
#else
		float ratio = 1;
#endif
		//update when ever got
		if( curBuf ) {
			yuv2rgb.Apply(curBuf, (FMT_NV12==g_fmt) ? (curBuf+g_width*g_height): NULL, ratio);
#if 0
			if( 0==(nbFrames&63) ) {
				unsigned char y=*curBuf;
				unsigned char u=*(curBuf+g_width*g_height);
				unsigned char v=*(curBuf+g_width*g_height+1);
				printf("yuv = 0x%02x (%.03f), 0x%02x (%.03f), 0x%02x (%.03f)\n",
					y, (float)y/255, u, (float)u/255, v, (float)v/255);
				float y0 = (float)y-0.0625;
				float u0 = (float)u-0.5;
				float v0 = (float)v-0.5;
				float r = y0*1.164+v0*1.787;
				float g = y0*1.164-u0*0.213-v0*0.531;
				float b = y0*1.164+u0*2.112;
				RGB2HSV(r, g, b, y0, u0, v0);
				printf("hsb = (%.03f, %.03f, %.03f)\n", y0, u0, v0);
			}
#endif
		}
		if( index>=0 ) {
			pthread_mutex_lock(&g_freeLock);
			g_free.push_back(index);
			pthread_mutex_unlock(&g_freeLock);
		}
        // Render on the whole framebuffer, complete from the lower left corner to the upper right
/*
		glViewport(0,0,windowWidth,windowHeight);

		// Clear the screen
		glClear( GL_COLOR_BUFFER_BIT);
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

*/
		// Swap buffers
		glfwSwapBuffers(window);
//		save_screen.nSave();
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
	//glDeleteBuffers(1, &quad_vertexbuffer);
	//glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

