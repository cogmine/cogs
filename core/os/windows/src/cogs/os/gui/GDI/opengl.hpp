////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: Placeholder
//
//#ifndef COGS_OS_OPENGL
//#define COGS_OS_OPENGL
//
//#pragma comment(lib, "opengl32.lib") 
//#pragma comment(lib, "glu32.lib") 
//
//#include "cogs/os.hpp"
//#include <gl/gl.h>
//#include <GL/glu.h>
////#include <GL/glx.h>
////#include <GL/glext.h>
////#include <GL/wglext.h>
//
//
//#include "cogs/env.hpp"
//#include "cogs/hwnd.hpp"
//
//
//namespace cogs {
//namespace gui {
//namespace os {
//
//
//template <unsigned int majorVersion, unsigned int minorVersion>
//class opengl : public hwnd_pane
//{
//private:
//	HGLRC	m_hGLRC;
//	HDC		m_hDC;
//
//	HGLRC	hOldRC;
//	HDC		hOldDC;
//
//public:
//	opengl()
//		:	hwnd_pane(composite_string(), WS_EX_NOPARENTNOTIFY, uiSubsystem)
//	{ }
//
//	~opengl()
//	{
//		wglDeleteContext(m_hGLRC);
//	}
//
//	//virtual color get_background_color() const	{ return color::green; }
//
//	virtual void installing()
//	{
//		PIXELFORMATDESCRIPTOR pfd; 
//		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR)); 
//		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); 
//		pfd.nVersion = 1; 
//		pfd.dwFlags =	PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW; 
//		pfd.iPixelType = PFD_TYPE_RGBA; 
//		pfd.cColorBits = 32;
//		//pfd.cDepthBits = 32;
//		pfd.iLayerType = PFD_MAIN_PLANE; 
//		int nPixelFormat = ChoosePixelFormat(get_HDC(), &pfd);
//		DWORD err = GetLastError();
//		COGS_ASSERT(nPixelFormat != 0);
//		BOOL bResult = SetPixelFormat (get_HDC(), nPixelFormat, &pfd); 
//		COGS_ASSERT(bResult);
//		m_hDC = get_HDC();
//		m_hGLRC = wglCreateContext(get_HDC());
//		COGS_ASSERT(m_hGLRC);
//	//	wglMakeCurrent(get_HDC(), m_hGLRC); 
//
////		int attribs[] = 
////		{
////			WGL_CONTEXT_MAJOR_VERSION_ARB, majorVersion, 
////			WGL_CONTEXT_MINOR_VERSION_ARB, minorVersion, 
////			WGL_CONTEXT_FLAGS_ARB, 
////			WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, 
////			0 
////		};
////		m_hrc = wglCreateContextAttribsARB(get_HDC(),0, attribs); 
//	//	wglMakeCurrent(NULL,NULL); 
////		wglDeleteContext(tempContext); 
//
//		// Set up 2D projection
//		
//		HDC hOldDC = wglGetCurrentDC();
//		HGLRC hOldRC = wglGetCurrentContext();
//		wglMakeCurrent(get_HDC(), m_hGLRC);
//
//		//glDisable(GL_DEPTH_TEST);
//		glEnable (GL_STENCIL_TEST);
//		glClear(GL_STENCIL_BUFFER_BIT);
//
//		// Dark blue background
//	//	glClearColor(0.0f, 0.0f, 0.3f, 0.0f);
//	//	glClear(GL_COLOR_BUFFER_BIT);
//
//		wglMakeCurrent(hOldDC, hOldRC);
//
//		hwnd_pane::installing();
//	}
//
//	virtual void invalidate(const canvas::bounds& r)
//	{
//		// Add bounds to stencil
//	}
//
//	virtual void pre_draw()
//	{
//		hOldDC = wglGetCurrentDC();
//		hOldRC = wglGetCurrentContext();
//		wglMakeCurrent(m_hDC, m_hGLRC);
//	}
//
//	virtual void post_draw()
//	{
//		SwapBuffers(m_hDC);
//		wglMakeCurrent(hOldDC, hOldRC);
//	}
//
//	virtual void reshape(const bounds& r, const point& oldOrigin = point(0, 0))
//	{
//		hwnd_pane::reshape(r, oldOrigin);
//
//		HDC hOldDC = wglGetCurrentDC();
//		HGLRC hOldRC = wglGetCurrentContext();
//		wglMakeCurrent(m_hDC, m_hGLRC);
//
//		glMatrixMode(GL_PROJECTION);
//		glLoadIdentity();
//		gluOrtho2D(0, r.get_width(), r.get_height(), 0);
//		//glOrtho(0, r.get_width(), r.get_height(), 0, 0, 1);
//		glViewport(0, 0, r.get_width(), r.get_height());                    
//		glMatrixMode(GL_MODELVIEW);
//
//		wglMakeCurrent(hOldDC, hOldRC);
//	}
//
//	// 2D
//	virtual void fill(const bounds& r, const color& c = color::black, bool blendAlpha = true)
//	{
//		point topRight = r.calc_top_right();
//		point bottomLeft = r.calc_bottom_left();
//
//		//glClear(GL_COLOR_BUFFER_BIT);
//
//		glBegin(GL_QUADS);
//		glColor4f((GLfloat)c.get_red() / 255, (GLfloat)c.get_green() / 255, (GLfloat)c.get_blue() / 255, (GLfloat)c.get_alpha() / 255);
//		glVertex2f((GLfloat)(topRight.get_x()), (GLfloat)(topRight.get_y()));
//		glVertex2f((GLfloat)(bottomLeft.get_x()), (GLfloat)(topRight.get_y()));
//		glVertex2f((GLfloat)(bottomLeft.get_x()), (GLfloat)(bottomLeft.get_y()));
//		glVertex2f((GLfloat)(topRight.get_x()), (GLfloat)(bottomLeft.get_y()));
////		glVertex2i(topRight.get_x(), topRight.get_y());
////		glVertex2i(bottomLeft.get_x(), topRight.get_y());
////		glVertex2i(bottomLeft.get_x(), bottomLeft.get_y());
////		glVertex2i(topRight.get_x(), bottomLeft.get_y());
//		glEnd();
//	}
//
//	virtual void invert(const bounds& r)
//	{
//	}
//
//	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true)
//	{
//		glBegin(GL_LINES);
//		glColor4f((GLfloat)c.get_red() / 255, (GLfloat)c.get_green() / 255, (GLfloat)c.get_blue() / 255, (GLfloat)c.get_alpha() / 255);
//		glVertex2f((GLfloat)(startPt.get_x()), (GLfloat)(startPt.get_y()));
//		glVertex2f((GLfloat)(endPt.get_x()), (GLfloat)(endPt.get_y()));
////		glVertex2i(startPt.get_x(), startPt.get_y());
////		glVertex2i(endPt.get_x(), endPt.get_y());
//		glEnd();
//	}
//
//	virtual void scroll(const bounds& r, const point& pt = point(0,0))
//	{
//		if (pt == r.get_position())
//			return;
//	}
//
//	virtual void draw_text(const composite_string& s, const bounds& r, const color& c = color::black, bool blendAlpha = true)
//	{
//		;
//	}
//
//	virtual void composite_scaled_pixel_image(const gfx::pixel_image& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true )
//	{
//	}
//
//	virtual void composite_pixel_mask(const gfx::pixel_mask& src, const bounds& srcBounds, const point& dstPt = point(0,0), const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true )
//	{
//	}
//
//	virtual rcptr<gfx::pixel_image_canvas> create_pixel_image_canvas(const size& sz, bool isOpaque = true, double dpi = canvas::dip_dpi)
//	{
//		return rcptr<gfx::pixel_image_canvas>();
//	}
//
//	virtual rcptr<gfx::pixel_image> load_pixel_image(const composite_string& location, double dpi = canvas::dip_dpi)
//	{
//		return rcptr<gfx::pixel_image>();
//	}
//
//	virtual rcptr<gfx::pixel_mask> load_pixel_mask(const composite_string& location, double dpi = canvas::dip_dpi)
//	{
//		return rcptr<gfx::pixel_mask>();
//	}
//
//	virtual rcptr<volatile gui::subsystem> get_subsystem()
//	{
//		return this_rcref;
//	}
//
//	virtual rcptr<console> get_default_console()
//	{
//		return rcptr<console>();	// TBD
//	}
//
//	virtual rcptr<console> create_console()
//	{
//		return rcptr<console>();	// TBD
//	}
//
//	virtual void message(const composite_string& msg)
//	{
//	}
//
//	virtual void open(const composite_string& title, const rcref<pane>& f, const rcptr<frame>& f = 0)
//	{
//	}
//
//	// planar 
//	virtual rcref<button_interface> create_button() volatile
//	{
//		rcptr<view> a;
//		return a.dereference();
//	}
//
//	virtual rcref<check_box_interface> create_check_box() volatile
//	{
//		rcptr<view> a;
//		return a.dereference();
//	}
//
//	virtual rcref<text_editor_interface> create_text_editor() volatile
//	{
//		rcptr<view> a;
//		return a.dereference();
//	}
//
//	virtual rcref<scroll_bar_interface> create_scroll_bar() volatile
//	{
//		rcptr<view> a;
//		return a.dereference();
//	}
//
//	// TBD - Create frame buffer objects for nested canvas's
////	virtual rcptr<view> create_canvas	(const rcref<canvas_panel>& cf) volatile	{ return rcptr<view>(); }
////	virtual rcptr<view> create_canvas	(const rcref<canvas_frame>& cf) volatile	{ return rcptr<view>(); }
//
////	virtual rcptr<view> create_canvas3D	(const rcref<canvas3D_frame>& cf) volatile	{ return rcptr<view>(); }
//};
//
//}
//}
//}
//
//#endif
