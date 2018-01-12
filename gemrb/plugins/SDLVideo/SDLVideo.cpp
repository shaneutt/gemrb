/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2006 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "SDLVideo.h"

#include "AnimationFactory.h"
#include "Game.h" // for GetGlobalTint
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"

#include "GUI/Button.h"
#include "GUI/Console.h"
#include "GUI/Window.h"

#if defined(__sgi)
#  include <math.h>
#  ifdef __cplusplus
extern "C" double round(double);
#  endif
#else
#  include <cmath>
#endif

using namespace GemRB;

SDLVideoDriver::SDLVideoDriver(void)
{
	lastTime = 0;
}

SDLVideoDriver::~SDLVideoDriver(void)
{
	SDL_Quit();
}

int SDLVideoDriver::Init(void)
{
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1) {
		return GEM_ERROR;
	}
	SDL_ShowCursor(SDL_DISABLE);
	return GEM_OK;
}

int SDLVideoDriver::PollEvents()
{
	int ret = GEM_OK;
	SDL_Event currentEvent;

	while (ret != GEM_ERROR && SDL_PollEvent(&currentEvent)) {
		ret = ProcessEvent(currentEvent);
	}

	return ret;
}

int SDLVideoDriver::ProcessEvent(const SDL_Event & event)
{
	if (!EvntManager)
		return GEM_ERROR;

	// FIXME: technically event.key.keysym.mod should be the mod,
	// but for the mod keys themselves this is 0 and therefore not what GemRB expects
	// int modstate = GetModState(event.key.keysym.mod);
	int modstate = GetModState(SDL_GetModState());
	SDLKey sym = event.key.keysym.sym;
	SDL_Keycode key = sym;
	Event e;

	/* Loop until there are no events left on the queue */
	switch (event.type) {
			/* Process the appropriate event type */
		case SDL_QUIT:
			/* Quit event originated from outside GemRB so ask the user if we should exit */
			core->AskAndExit();
			return GEM_OK;
			break;
		case SDL_KEYUP:
			switch(sym) {
				case SDLK_LALT:
				case SDLK_RALT:
					key = GEM_ALT;
					break;
				case SDLK_SCROLLOCK:
					key = GEM_GRAB;
					break;
				default:
					if (sym < 256) {
						key = sym;
					}
					break;
			}
			if (key != 0) {
				Event e = EvntManager->CreateKeyEvent(key, false, modstate);
				EvntManager->DispatchEvent(e);
			}
			break;
		case SDL_KEYDOWN:
#if SDL_VERSION_ATLEAST(1,3,0)
			key = SDL_GetKeyFromScancode(event.key.keysym.scancode);
#endif
			// reenable special numpad keys unless numlock is off
			if (SDL_GetModState() & KMOD_NUM) {
				switch (sym) {
					case SDLK_KP1: sym = SDLK_1; break;
					case SDLK_KP2: sym = SDLK_2; break;
					case SDLK_KP3: sym = SDLK_3; break;
					case SDLK_KP4: sym = SDLK_4; break;
					// 5 is not special
					case SDLK_KP6: sym = SDLK_6; break;
					case SDLK_KP7: sym = SDLK_7; break;
					case SDLK_KP8: sym = SDLK_8; break;
					case SDLK_KP9: sym = SDLK_9; break;
					default: break;
				}
			}
			switch (sym) {
				case SDLK_ESCAPE:
					key = GEM_ESCAPE;
					break;
				case SDLK_END:
				case SDLK_KP1:
					key = GEM_END;
					break;
				case SDLK_HOME:
				case SDLK_KP7:
					key = GEM_HOME;
					break;
				case SDLK_UP:
				case SDLK_KP8:
					key = GEM_UP;
					break;
				case SDLK_DOWN:
				case SDLK_KP2:
					key = GEM_DOWN;
					break;
				case SDLK_LEFT:
				case SDLK_KP4:
					key = GEM_LEFT;
					break;
				case SDLK_RIGHT:
				case SDLK_KP6:
					key = GEM_RIGHT;
					break;
				case SDLK_DELETE:
#if TARGET_OS_IPHONE < 1
					//iOS currently doesnt have a backspace so we use delete.
					//This change should be future proof in the event apple changes the delete key to a backspace.
					key = GEM_DELETE;
					break;
#endif
				case SDLK_BACKSPACE:
					key = GEM_BACKSP;
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					key = GEM_RETURN;
					break;
				case SDLK_LALT:
				case SDLK_RALT:
					key = GEM_ALT;
					break;
				case SDLK_TAB:
					key = GEM_TAB;
					break;
				case SDLK_PAGEUP:
				case SDLK_KP9:
					key = GEM_PGUP;
					break;
				case SDLK_PAGEDOWN:
				case SDLK_KP3:
					key = GEM_PGDOWN;
					break;
				case SDLK_SCROLLOCK:
					key = GEM_GRAB;
					break;
				case SDLK_F1:
				case SDLK_F2:
				case SDLK_F3:
				case SDLK_F4:
				case SDLK_F5:
				case SDLK_F6:
				case SDLK_F7:
				case SDLK_F8:
				case SDLK_F9:
				case SDLK_F10:
				case SDLK_F11:
				case SDLK_F12:
					//assuming they come sequentially,
					//also, there is no need to ever produce more than 12
					key = GEM_FUNCTIONX(1) + sym-SDLK_F1;
					break;
				default: break;
			}
			e = EvntManager->CreateKeyEvent(key, true, modstate);
			if (e.keyboard.character) {
#if SDL_VERSION_ATLEAST(1,3,0)
				e.keyboard.character = SDL_GetKeyFromScancode(event.key.keysym.scancode);
#else
				e.keyboard.character = event.key.keysym.unicode;
#endif
			}
			EvntManager->DispatchEvent(e);
			break;
		case SDL_MOUSEMOTION:
			e = EvntManager->CreateMouseMotionEvent(Point(event.motion.x, event.motion.y), modstate);
			EvntManager->DispatchEvent(e);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			bool down = (event.type == SDL_MOUSEBUTTONDOWN) ? true : false;
			Point p(event.button.x, event.button.y);
			EventButton btn = SDL_BUTTON(event.button.button);
			e = EvntManager->CreateMouseBtnEvent(p, btn, down, modstate);
			EvntManager->DispatchEvent(e);
			break;
	}
	return GEM_OK;
}

Sprite2D* SDLVideoDriver::CreateSprite(int w, int h, int bpp, ieDword rMask,
	ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK, int index)
{
	sprite_t* spr = new sprite_t(w, h, bpp, pixels, rMask, gMask, bMask, aMask);

	if (cK) {
		spr->SetColorKey(index);
	}
	/*
	 there is at least one place (BlitGameSprite) that requires 8 or 32bpp sprites
	 untill we support 16bpp fully we cannot do this

	// make sure colorkey is set prior to conversion
	SDL_PixelFormat* fmt = backBuf->format;
	spr->ConvertFormatTo(fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
	*/
	return spr;
}

Sprite2D* SDLVideoDriver::CreateSprite8(int w, int h, void* pixels,
										Palette* palette, bool cK, int index)
{
	if (palette) {
		return CreatePalettedSprite(w, h, 8, pixels, palette->col, cK, index);
	} else {
		// an alpha only sprite. used by SpriteCover or as a mask passed to BlitTile
		return new sprite_t(w, h, 8, pixels, 0, 0, 0, 0xff);
	}
}

Sprite2D* SDLVideoDriver::CreatePalettedSprite(int w, int h, int bpp, void* pixels,
											   Color* palette, bool cK, int index)
{
	sprite_t* spr = new sprite_t(w, h, bpp, pixels, 0, 0, 0, 0);

	spr->SetPalette(palette);
	if (cK) {
		spr->SetColorKey(index);
	}
	return spr;
}

void SDLVideoDriver::BlitTile(const Sprite2D* spr, const Sprite2D* mask, int x, int y, const Region* clip, unsigned int flags)
{
	assert(spr->BAM == false);

	Region fClip = ClippedDrawingRect(Region(x, y, 64, 64), clip);
	Region srect(64 - fClip.w, 64 - fClip.h, fClip.w, fClip.h);

	const Color* tintcol = NULL;

	if (core->GetGame()) {
		tintcol = core->GetGame()->GetGlobalTint();
	}

	BlitSpriteClipped(spr, mask, srect, fClip, flags, tintcol);
}

void SDLVideoDriver::BlitSprite(const Sprite2D* spr, const Region& src, Region dst)
{
	dst.x -= spr->XPos;
	dst.y -= spr->YPos;
	BlitSpriteClipped(spr, NULL, src, dst);
}

void SDLVideoDriver::BlitGameSprite(const Sprite2D* spr, int x, int y,
		unsigned int flags, Color tint,
		SpriteCover* cover, const Region* clip)
{
	Region srect(0, 0, spr->Width, spr->Height);
	Region drect = (clip) ? *clip : Region(x - spr->XPos, y - spr->YPos, spr->Width, spr->Height);
	const Sprite2D* mask = (cover) ? cover->GetMask() : NULL;
	BlitSpriteClipped(spr, mask, srect, drect, flags, &tint);
}

// SetPixel is in screen coordinates
#if SDL_VERSION_ATLEAST(1,3,0)
#define SetPixel(buffer, _x, _y) { \
Region r = buffer->Rect(); \
Point p(_x - r.x, _y - r.x); \
if (r.PointInside(p)) { SDL_Point p2 = {p.x,p.y}; points.push_back(p2); } }
#else
#define SetPixel(buffer, _x, _y) { \
Region r = buffer->Rect(); \
Point p(_x - r.x, _y - r.x); \
if (r.PointInside(p)) { points.push_back(p); } }
#endif

/** This functions Draws a Circle */
void SDLVideoDriver::DrawCircle(const Point& c, unsigned short r, const Color& color)
{
	//Uses the Breshenham's Circle Algorithm
	long x, y, xc, yc, re;

	x = r;
	y = 0;
	xc = 1 - ( 2 * r );
	yc = 1;
	re = 0;

	std::vector<SDL_Point> points;

	while (x >= y) {
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		SetPixel( drawingBuffer, c.x + ( short ) y, c.y + ( short ) x );
		SetPixel( drawingBuffer, c.x - ( short ) y, c.y + ( short ) x );
		SetPixel( drawingBuffer, c.x - ( short ) y, c.y - ( short ) x );
		SetPixel( drawingBuffer, c.x + ( short ) y, c.y - ( short ) x );

		y++;
		re += yc;
		yc += 2;

		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color));
}

static double ellipseradius(unsigned short xr, unsigned short yr, double angle) {
	double one = (xr * sin(angle));
	double two = (yr * cos(angle));
	return sqrt(xr*xr*yr*yr / (one*one + two*two));
}

/** This functions Draws an Ellipse Segment */
void SDLVideoDriver::DrawEllipseSegment(const Point& c, unsigned short xr,
	unsigned short yr, const Color& color, double anglefrom, double angleto, bool drawlines)
{
	/* beware, dragons and clockwise angles be here! */
	double radiusfrom = ellipseradius(xr, yr, anglefrom);
	double radiusto = ellipseradius(xr, yr, angleto);
	long xfrom = (long)round(radiusfrom * cos(anglefrom));
	long yfrom = (long)round(radiusfrom * sin(anglefrom));
	long xto = (long)round(radiusto * cos(angleto));
	long yto = (long)round(radiusto * sin(angleto));

	if (drawlines) {
		DrawLine(c, Point(c.x + xfrom, c.y + yfrom), color);
		DrawLine(c, Point(c.x + xto, c.y + yto), color);
	}

	// *Attempt* to calculate the correct x/y boundaries.
	// TODO: this doesn't work very well - you can't actually bound many
	// arcs this way (imagine a segment with a small piece cut out).
	if (xfrom > xto) {
		long tmp = xfrom; xfrom = xto; xto = tmp;
	}
	if (yfrom > yto) {
		long tmp = yfrom; yfrom = yto; yto = tmp;
	}
	if (xfrom >= 0 && yto >= 0) xto = xr;
	if (xto <= 0 && yto >= 0) xfrom = -xr;
	if (yfrom >= 0 && xto >= 0) yto = yr;
	if (yto <= 0 && xto >= 0) yfrom = -yr;

	//Uses Bresenham's Ellipse Algorithm
	long x, y, xc, yc, ee, tas, tbs, sx, sy;

	tas = 2 * xr * xr;
	tbs = 2 * yr * yr;
	x = xr;
	y = 0;
	xc = yr * yr * ( 1 - ( 2 * xr ) );
	yc = xr * xr;
	ee = 0;
	sx = tbs * xr;
	sy = 0;

	std::vector<SDL_Point> points;

	while (sx >= sy) {
		if (x >= xfrom && x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		if (-x >= xfrom && -x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		if (-x >= xfrom && -x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		if (x >= xfrom && x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		y++;
		sy += tas;
		ee += yc;
		yc += tas;
		if (( 2 * ee + xc ) > 0) {
			x--;
			sx -= tbs;
			ee += xc;
			xc += tbs;
		}
	}

	x = 0;
	y = yr;
	xc = yr * yr;
	yc = xr * xr * ( 1 - ( 2 * yr ) );
	ee = 0;
	sx = 0;
	sy = tas * yr;

	while (sx <= sy) {
		if (x >= xfrom && x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		if (-x >= xfrom && -x <= xto && y >= yfrom && y <= yto)
			SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		if (-x >= xfrom && -x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		if (x >= xfrom && x <= xto && -y >= yfrom && -y <= yto)
			SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		x++;
		sx += tbs;
		ee += xc;
		xc += tbs;
		if (( 2 * ee + yc ) > 0) {
			y--;
			sy -= tas;
			ee += yc;
			yc += tas;
		}
	}

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color));
}


/** This functions Draws an Ellipse */
void SDLVideoDriver::DrawEllipse(const Point& c, unsigned short xr,
								 unsigned short yr, const Color& color)
{
	//Uses Bresenham's Ellipse Algorithm
	long x, y, xc, yc, ee, tas, tbs, sx, sy;

	tas = 2 * xr * xr;
	tbs = 2 * yr * yr;
	x = xr;
	y = 0;
	xc = yr * yr * ( 1 - ( 2 * xr ) );
	yc = xr * xr;
	ee = 0;
	sx = tbs * xr;
	sy = 0;

	std::vector<SDL_Point> points;

	while (sx >= sy) {
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		y++;
		sy += tas;
		ee += yc;
		yc += tas;
		if (( 2 * ee + xc ) > 0) {
			x--;
			sx -= tbs;
			ee += xc;
			xc += tbs;
		}
	}

	x = 0;
	y = yr;
	xc = yr * yr;
	yc = xr * xr * ( 1 - ( 2 * yr ) );
	ee = 0;
	sx = 0;
	sy = tas * yr;

	while (sx <= sy) {
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y + ( short ) y );
		SetPixel( drawingBuffer, c.x - ( short ) x, c.y - ( short ) y );
		SetPixel( drawingBuffer, c.x + ( short ) x, c.y - ( short ) y );
		x++;
		sx += tbs;
		ee += xc;
		xc += tbs;
		if (( 2 * ee + yc ) > 0) {
			y--;
			sy -= tas;
			ee += yc;
			yc += tas;
		}
	}

	DrawPoints(points, reinterpret_cast<const SDL_Color&>(color));
}

void SDLVideoDriver::DrawPolygon(Gem_Polygon* poly, const Point& origin, const Color& color, bool fill)
{
	Region bbox = poly->BBox;
	bbox.x -= origin.x;
	bbox.y -= origin.y;

	if (!poly->count || !bbox.IntersectsRegion(screenClip)) {
		return;
	}

	std::vector<SDL_Point> points;

	if (fill) {
		std::list<Trapezoid>::iterator iter;
		for (iter = poly->trapezoids.begin(); iter != poly->trapezoids.end();
			 ++iter)
		{
			int y_top = iter->y1 - origin.y; // inclusive
			int y_bot = iter->y2 - origin.y; // exclusive

			if (y_top < 0) y_top = 0;
			if (y_bot > screenSize.h) y_bot = screenSize.h;
			if (y_top >= y_bot) continue; // clipped

			int ledge = iter->left_edge;
			int redge = iter->right_edge;
			const Point& a = poly->points[ledge];
			const Point& b = poly->points[(ledge+1)%(poly->count)];
			const Point& c = poly->points[redge];
			const Point& d = poly->points[(redge+1)%(poly->count)];

			for (int y = y_top; y < y_bot; ++y) {
				int py = y + origin.y;

				int lt = (b.x * (py - a.y) + a.x * (b.y - py))/(b.y - a.y);
				int rt = (d.x * (py - c.y) + c.x * (d.y - py))/(d.y - c.y) + 1;

				lt -= origin.x;
				rt -= origin.x;

				if (lt < 0) lt = 0;
				if (rt > screenSize.w) rt = screenSize.w;
				if (lt >= rt) { continue; } // clipped

				// Draw a line from (y,lt) to (y,rt)
				SetPixel(drawingBuffer, lt, y);
				SetPixel(drawingBuffer, rt, y);
			}
		}
	} else {
		SetPixel(drawingBuffer, poly->points[0].x - origin.x, poly->points[0].y - origin.y);

		for (unsigned int i = 1; i < poly->count; i++) {
			SetPixel(drawingBuffer, poly->points[i].x - origin.x, poly->points[i].y - origin.y);
			SetPixel(drawingBuffer, poly->points[i].x - origin.x, poly->points[i].y - origin.y);
		}
		// reconnect with start point
		SetPixel(drawingBuffer, poly->points[0].x - origin.x, poly->points[0].y - origin.y);
	}

	// TODO: if our points were compatible with SDL_Point we could just pass poly->points
	DrawLines(points, reinterpret_cast<const SDL_Color&>(color));
}

#undef SetPixel

void SDLVideoDriver::SetFadeColor(int r, int g, int b)
{
	if (r>255) r=255;
	else if(r<0) r=0;
	fadeColor.r=r;
	if (g>255) g=255;
	else if(g<0) g=0;
	fadeColor.g=g;
	if (b>255) b=255;
	else if(b<0) b=0;
	fadeColor.b=b;
	//long val = SDL_MapRGBA( extra->format, fadeColor.r, fadeColor.g, fadeColor.b, fadeColor.a );
	//SDL_FillRect( extra, NULL, val );
}

void SDLVideoDriver::SetFadePercent(int percent)
{
	if (percent>100) percent = 100;
	else if (percent<0) percent = 0;
	fadeColor.a = (255 * percent ) / 100;
}

void SDLVideoDriver::BlitSpriteClipped(const Sprite2D* spr, const Sprite2D* mask, Region src, const Region& dst, unsigned int flags, const Color* tint)
{
	// FIXME?: srect isn't verified
	Region dclipped = ClippedDrawingRect(dst);
	int trim = dst.h - dclipped.h;
	if (trim) {
		src.h -= trim;
		if (dclipped.y > dst.y) { // top clipped
			src.y += trim;
		} // already have appropriate y for bottom clip
	}
	trim = dst.w - dclipped.w;
	if (trim) {
		src.w -= trim;
		if (dclipped.x > dst.x) { // left clipped
			src.x += trim;
		}
	} // already have appropriate y for right clip

	if (dclipped.Dimensions().IsEmpty() || src.Dimensions().IsEmpty()) {
		return;
	}

	if (spr->BAM) {
		BlitSpriteBAMClipped(spr, mask, src, dclipped, flags, tint);
	} else {
		SDL_Rect srect = RectFromRegion(src);
		SDL_Rect drect = RectFromRegion(dclipped);
		BlitSpriteNativeClipped(spr, mask, srect, drect, flags, reinterpret_cast<const SDL_Color*>(tint));
	}
}

// static class methods

int SDLVideoDriver::SetSurfacePalette(SDL_Surface* surf, const SDL_Color* pal, int numcolors)
{
	if (pal) {
#if SDL_VERSION_ATLEAST(1,3,0)
		return SDL_SetPaletteColors( surf->format->palette, pal, 0, numcolors );
#else
		// const_cast because SDL doesnt alter this and we want our interface to be const correct
		return SDL_SetPalette( surf, SDL_LOGPAL | SDL_RLEACCEL, const_cast<SDL_Color*>(pal), 0, numcolors );
#endif
	}
	return -1;
}

template <typename T>
void BlendPixel(const Color& srcc, SDL_PixelFormat* fmt, Uint8* pixel)
{
	T dst = *(T*)pixel;
	Color dstc;
	SDL_GetRGB( dst, fmt, &dstc.r, &dstc.g, &dstc.b );

	// dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA))
	dstc.r = (srcc.r * srcc.a/255) + (dstc.r * (1 - srcc.a/255));
	dstc.g = (srcc.g * srcc.a/255) + (dstc.g * (1 - srcc.a/255));
	dstc.b = (srcc.b * srcc.a/255) + (dstc.b * (1 - srcc.a/255));

	*(T*)pixel = SDL_MapRGB(fmt, dstc.r, dstc.g, dstc.b);
}

void SDLVideoDriver::SetSurfacePixels(SDL_Surface* surface, const std::vector<SDL_Point>& points, const Color& srcc)
{
	SDL_PixelFormat* fmt = surface->format;
	SDL_LockSurface( surface );

	std::vector<SDL_Point>::const_iterator it;
	it = points.begin();
	for (; it != points.end(); ++it) {
		SDL_Point p = *it;

		unsigned char* start = static_cast<unsigned char*>(surface->pixels);
		unsigned char* pixel = start + ((p.y * surface->pitch) + (p.x * fmt->BytesPerPixel));
		// NOTICE: we assume the points were clipped by the caller for performance reasons
		assert(pixel <= start + (surface->pitch * surface->h));

		switch (fmt->BytesPerPixel) {
			case 1:
				BlendPixel<Uint8>(srcc, surface->format, pixel);
				break;
			case 2:
				BlendPixel<Uint16>(srcc, surface->format, pixel);
				break;
			case 3:
			{
				// FIXME: implement alpha blending for this... or nix it
				// is this even used?
				Uint32 val = SDL_MapRGB(surface->format, srcc.r, srcc.g, srcc.b);
	#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				pixel[0] = val & 0xff;
				pixel[1] = (val >> 8) & 0xff;
				pixel[2] = (val >> 16) & 0xff;
	#else
				pixel[2] = val & 0xff;
				pixel[1] = (val >> 8) & 0xff;
				pixel[0] = (val >> 16) & 0xff;
	#endif
			}
				break;
			case 4:
				BlendPixel<Uint32>(srcc, surface->format, pixel);
				break;
			default:
				Log(ERROR, "sprite_t", "Working with unknown pixel format: %s", SDL_GetError());
				break;
		}
	}

	SDL_UnlockSurface( surface );
}

Color SDLVideoDriver::GetSurfacePixel(SDL_Surface* surface, short x, short y)
{
	Color c;
	SDL_LockSurface( surface );
	Uint8 Bpp = surface->format->BytesPerPixel;
	unsigned char * pixels = ( ( unsigned char * ) surface->pixels ) +
	( ( y * surface->pitch + (x*Bpp)) );
	Uint32 val = 0;

	if (Bpp == 1) {
		val = *pixels;
	} else if (Bpp == 2) {
		val = *(Uint16 *)pixels;
	} else if (Bpp == 3) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		val = pixels[0] + ((unsigned int)pixels[1] << 8) + ((unsigned int)pixels[2] << 16);
#else
		val = pixels[2] + ((unsigned int)pixels[1] << 8) + ((unsigned int)pixels[0] << 16);
#endif
	} else if (Bpp == 4) {
		val = *(Uint32 *)pixels;
	}
	
	SDL_UnlockSurface( surface );
	SDL_GetRGBA( val, surface->format, (Uint8 *) &c.r, (Uint8 *) &c.g, (Uint8 *) &c.b, (Uint8 *) &c.a );
	
	// check color key... SDL_GetRGBA wont get this
#if SDL_VERSION_ATLEAST(1,3,0)
	Uint32 ck;
	if (SDL_GetColorKey(surface, &ck) != -1 && ck == val) c.a = SDL_ALPHA_TRANSPARENT;
#else
	if (surface->format->colorkey == val) c.a = SDL_ALPHA_TRANSPARENT;
#endif
	return c;
}
