/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 16bpp_base.hpp Base for all 16 bits blitters. */

#ifndef BLITTER_16BPP_BASE_HPP
#define BLITTER_16BPP_BASE_HPP

#include "base.hpp"
#include "../core/bitmath_func.hpp"
#include "../core/math_func.hpp"
#include "../gfx_func.h"

/** Base for all 16bpp blitters. */
class Blitter_16bppBase : public Blitter {
public:

	// TODO: GCC-specific attributes
	struct Colour16 {
		unsigned b : 5 __attribute__((packed));  ///< Blue-channel, packed 5 bits
		unsigned g : 6 __attribute__((packed));  ///< Green-channel, packed 6 bits
		unsigned r : 5 __attribute__((packed));  ///< Red-channel, packed 5 bits
		Colour16(uint8 r = 0, uint8 g = 0, uint8 b = 0):
			b(b), g(g), r(r)
		{
		}
	};

	struct Pixel {
		Colour16 c;
		unsigned a : 4 __attribute__((packed));  ///< Alpha-channel, packed 4 bits
		unsigned v : 4 __attribute__((packed));  ///< Brightness-channel, packed 4 bits
		unsigned m : 8 __attribute__((packed));  ///< Remap-channel, cannot pack it, because it's palette lookup index, so it must be in range 0-255
	};

	/* virtual */ uint8 GetScreenDepth() { return 16; }
	/* virtual */ void *MoveTo(void *video, int x, int y);
	/* virtual */ void SetPixel(void *video, int x, int y, uint8 colour);
	/* virtual */ void DrawRect(void *video, int width, int height, uint8 colour);
	/* virtual */ void CopyFromBuffer(void *video, const void *src, int width, int height);
	/* virtual */ void CopyToBuffer(const void *video, void *dst, int width, int height);
	/* virtual */ void CopyImageToBuffer(const void *video, void *dst, int width, int height, int dst_pitch);
	/* virtual */ void ScrollBuffer(void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y);
	/* virtual */ int BufferSize(int width, int height);
	/* virtual */ void PaletteAnimate(const Palette &palette);
	/* virtual */ Blitter::PaletteAnimation UsePaletteAnimation();
	/* virtual */ int GetBytesPerPixel() { return 2; }


	/**
	 * Convert from rgb values to screen native 16bpp colour
	 */
	static inline Colour16 To16(uint8 r, uint8 g, uint8 b)
	{
		return Colour16(r >> 3, g >> 2, b >> 3);
	}

	/**
	 * Convert from 32bpp colour to screen native 16bpp colour
	 */
	static inline Colour16 To16(Colour c)
	{
		return To16(c.r, c.g, c.b);
	}

	/**
	 * Look up the colour in the current palette.
	 */
	static inline Colour LookupColourInPalette32(uint index)
	{
		return _cur_palette.palette[index];
	}

	/**
	 * Look up the colour in the current palette.
	 */
	static inline Colour16 LookupColourInPalette(uint index)
	{
		return To16(LookupColourInPalette32(index));
	}

	/**
	 * Compose a colour based on RGBA values and the current pixel value.
	 * @param r range is from 0 to 31.
	 * @param g range is from 0 to 63.
	 * @param b range is from 0 to 31.
	 * @param a range is from 0 to 15.
	 */
	static inline Colour16 ComposeColourRGBANoCheck(uint8 r, uint8 g, uint8 b, uint8 a, Colour16 current)
	{
		/* The 16 is wrong, it should be 15, but 16 is much faster... */
		return Colour16 (	((int)(r - current.r) * a) / 16 + current.r,
							((int)(g - current.g) * a) / 16 + current.g,
							((int)(b - current.b) * a) / 16 + current.b );
	}

	/**
	 * Compose a colour based on RGBA values and the current pixel value.
	 * Handles fully transparent and solid pixels in a special (faster) way.
	 * @param r range is from 0 to 31.
	 * @param g range is from 0 to 63.
	 * @param b range is from 0 to 31.
	 * @param a range is from 0 to 15.
	 */
	static inline Colour16 ComposeColourRGBA(uint8 r, uint8 g, uint8 b, uint8 a, Colour16 current)
	{
		if (a == 0) return current;
		if (a >= 15) return Colour16(r, g, b);

		return ComposeColourRGBANoCheck(r, g, b, a, current);
	}

	/**
	 * Compose a colour based on Pixel value, alpha value, and the current pixel value.
	 * @param a range is from 0 to 16.
	 */
	static inline Colour16 ComposeColourPANoCheck(Colour16 colour, uint8 a, Colour16 current)
	{
		return ComposeColourRGBANoCheck(colour.r, colour.g, colour.b, a, current);
	}

	/**
	 * Compose a colour based on Pixel value, alpha value, and the current pixel value.
	 * Handles fully transparent and solid pixels in a special (faster) way.
	 * @param a range is from 0 to 15.
	 */
	static inline Colour16 ComposeColourPA(Colour16 colour, uint8 a, Colour16 current)
	{
		if (a == 0) return current;
		if (a >= 15) return colour;

		return ComposeColourPANoCheck(colour, a, current);
	}

	/**
	 * Make a pixel looks like it is transparent.
	 * @param colour the colour already on the screen.
	 * @param nom the amount of transparency, nominator, makes colour lighter.
	 * @param denom denominator, makes colour darker.
	 * @return the new colour for the screen.
	 */
	static inline Colour16 MakeTransparent(Colour16 colour, uint nom, uint denom = 256)
	{
		uint r = colour.r;
		uint g = colour.g;
		uint b = colour.b;

		return Colour16(	r * nom / denom,
							g * nom / denom,
							b * nom / denom );
	}

	/**
	 * Make a colour grey - based.
	 * @param colour the colour to make grey.
	 * @return the new colour, now grey.
	 */
	static inline Colour16 MakeGrey(Colour16 colour)
	{
		uint8 r = colour.r;
		uint8 g = colour.g;
		uint8 b = colour.b;

		/* To avoid doubles and stuff, multiple it with a total of 65536 (16bits), then
		 *  divide by it to normalize the value to a byte again. See heightmap.cpp for
		 *  information about the formula. */
		uint grey = (((r << 3) * 19595) + ((g << 2) * 38470) + ((b << 3) * 7471)) / 65536;

		return To16(grey, grey, grey);
	}

	/**
	 * Make a colour dark grey, for specialized 32bpp remapping.
	 * @param r red component
	 * @param g green component
	 * @param b blue component
	 * @return the brightness value of the new colour, now dark grey.
	 */
	static inline uint8 MakeDark(Colour16 colour)
	{
		uint8 r = colour.r;
		uint8 g = colour.g;
		uint8 b = colour.b;

		/* Magic-numbers are ~66% of those used in MakeGrey() */
		return (((r << 3) * 13063) + ((g << 2) * 25647) + ((b << 3) * 4981)) / 65536;
	}

	enum { DEFAULT_BRIGHTNESS = 8 };

	/**
	 * @param brightness range is from 0 to 15.
	 */
	static inline Colour16 AdjustBrightness(Colour16 colour, uint8 brightness)
	{
		/* Shortcut for normal brightness */
		if (brightness == DEFAULT_BRIGHTNESS) return colour;

		uint16 ob = 0;
		uint16 r = colour.r * brightness / DEFAULT_BRIGHTNESS;
		uint16 g = colour.g * brightness / DEFAULT_BRIGHTNESS;
		uint16 b = colour.b * brightness / DEFAULT_BRIGHTNESS;

		/* Sum overbright */
		if (r > 31) ob += r - 31;
		if (g > 63) ob += g - 63;
		if (b > 31) ob += b - 31;

		if (ob == 0) return Colour16(r, g, b);

		/* Reduce overbright strength */
		ob /= 2;
		return Colour16(	r >= 31 ? 31 : min(r + ob * (31 - r) / 32, 31),
							g >= 63 ? 63 : min(g + ob * (63 - g) / 64, 63),
							b >= 31 ? 31 : min(b + ob * (31 - b) / 32, 31) );
	}
};

#endif /* BLITTER_16BPP_BASE_HPP */
