#ifndef __SAVE_SCREEN__
#define __SAVE_SCREEN__

#include   <stdio.h>
#include   <stdlib.h>

#ifndef DBG
	#define DBG(level, fmt, arg...)                     \
	        do {                                        \
	                if( level>=m_dbgLevel )             \
	                        fprintf(stderr, fmt, ##arg);\
	        } while( 0 )
#endif //DBG

class cSaveScrn {
public:
	cSaveScrn(int debug=0, int bufW=0, int bufH=0, const char *templ=NULL, int frameSave=0)
		  : m_dbgLevel(debug)
		  , m_nameTemp(templ)
       		  , m_index(0)
		  , m_frameSave(frameSave)
       {
		//get window size
		GLint vp[4];
		glGetIntegerv( GL_VIEWPORT, vp );
		m_x = vp[0];
		m_y = vp[1];
		m_w = vp[2];
		m_h = vp[3];
		m_stride = 3*m_w; //rgb
		if( !bufW || !bufH ) {
			//use window size of invalid
			bufW = m_w;
			bufH = m_h;
		}
		if( !m_nameTemp ) sprintf(m_fName, "/dev/null");
		m_pixelBuf = new unsigned char [bufW*bufH*3];
		if( !m_pixelBuf ) {
			DBG(0, "fail to new pixel buffer\n");
			return;
		}
	}
	~cSaveScrn() {
		if( m_pixelBuf ) delete [] m_pixelBuf;
	}
	int nSave() {
		if( !m_pixelBuf ) return 0;
		if( m_frameSave && m_frameSave<m_index )
			return 0;
		//Byte alignment (that is, no alignment)
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(m_x, m_y, m_w, m_h, GL_RGB, GL_UNSIGNED_BYTE, m_pixelBuf);
		if( m_nameTemp ) {
			sprintf(m_fName, (const char *)m_nameTemp, m_index);
			++m_index;
			FILE *f0 = fopen((const char *)m_fName, "wb");
			if( NULL==f0 ) {
				DBG(0,  "Cannot open %s for writting.\n", m_fName);
				return 0;
			}
		
			fprintf(f0, "P6\n%d %d\n255\n", m_w, m_h);
			//reverse write
			for( int j=0; j<m_h; ++j ) {
				fwrite(&m_pixelBuf[m_stride*(m_h-j-1)]
					, sizeof(unsigned char)
					, m_stride
					, f0);
			}
			fclose(f0);
		}
		return 1;
	}
private:
	int m_dbgLevel;
	//name to save
	const char *m_nameTemp;
	char m_fName[128];
	unsigned char *m_pixelBuf;
	//(x,y) -- to --> (+w,+h)
	int m_w;
	int m_h;
	int m_x;
	int m_y;
	int m_stride;
	int m_index;    //frame index
	int m_frameSave;
};

#endif	//__SAVE_SCREEN__
