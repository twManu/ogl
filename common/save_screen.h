#ifndef __SAVE_SCREEN__
#define __SAVE_SCREEN__

void save_screen(const char *spath)
{
	GLint vp[4];
	glGetIntegerv( GL_VIEWPORT, vp );
  
	int &x = vp[0], &y = vp[1], &w = vp[2] , &h = vp[3];
	int stride = 3*w, j;

	unsigned char *bottomup_pixel = (unsigned char *) malloc( stride*h*sizeof(unsigned char) );


	//Byte alignment (that is, no alignment)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, bottomup_pixel);
	FILE *f0 = fopen( spath, "wb" );
	if( f0==NULL ) {
		printf( "[Error] : SaveScreen(), Cannot open %s for writting.\n", spath );
		return;
	}
	fprintf( f0, "P6\n%d %d\n255\n", w, h);
	//reverse write
	for( j=0; j<h; ++j ) {
		fwrite(&bottomup_pixel[stride*(h-j-1)], sizeof(unsigned char), stride, f0);
	}
	fclose( f0 );

	free(bottomup_pixel);
}
#endif	//__SAVE_SCREEN__
