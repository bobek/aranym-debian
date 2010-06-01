	{"OSMesaCreateContext",
	"OSMesaContext OSMesaCreateContext( GLenum format, OSMesaContext sharelist )",
	OSMesaCreateContext},
	{"OSMesaCreateContextExt",
	"OSMesaContext OSMesaCreateContextExt( GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist)",
	OSMesaCreateContextExt},
	{"OSMesaDestroyContext",
	"void OSMesaDestroyContext( OSMesaContext ctx )",
	OSMesaDestroyContext},
	{"OSMesaMakeCurrent",
	"GLboolean OSMesaMakeCurrent( OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height )",
	OSMesaMakeCurrent},
	{"OSMesaGetCurrentContext",
	"OSMesaContext OSMesaGetCurrentContext( void )",
	OSMesaGetCurrentContext},
	{"OSMesaPixelStore",
	"void OSMesaPixelStore( GLint pname, GLint value )",
	OSMesaPixelStore},
	{"OSMesaGetIntegerv",
	"void OSMesaGetIntegerv( GLint pname, GLint *value )",
	OSMesaGetIntegerv},
	{"OSMesaGetDepthBuffer",
	"GLboolean OSMesaGetDepthBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *bytesPerValue, void **buffer )",
	OSMesaGetDepthBuffer},
	{"OSMesaGetColorBuffer",
	"GLboolean OSMesaGetColorBuffer( OSMesaContext c, GLint *width, GLint *height, GLint *format, void **buffer )",
	OSMesaGetColorBuffer},
	{"OSMesaGetProcAddress",
	"void * OSMesaGetProcAddress( const char *funcName )",
	OSMesaGetProcAddress},
