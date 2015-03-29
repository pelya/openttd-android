/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 16bpp_anim.hpp A 16 bpp blitter with animation support. */

#ifndef BLITTER_16BPP_ANIM_HPP
#define BLITTER_16BPP_ANIM_HPP

#include "16bpp_simple.hpp"

class Blitter_16bppOptimized: public Blitter_16bppSimple {
	// TODO: implement that
};

/** The optimised 16 bpp blitter with palette animation. */
class Blitter_16bppAnim : public Blitter_16bppOptimized {
protected:
	// PALETTE_ANIM_SIZE is less than 32, so we'll use 5 bits for color index, and 3 bits for brightness, losing 1 bit compared to struct Pixel
	struct Anim {
		unsigned m : 5 __attribute__((packed));  ///< Color index channel, packed 5 bits, 0 = no animation, 1 = PALETTE_ANIM_START
		unsigned v : 3 __attribute__((packed));  ///< Brightness-channel, packed 3 bits
	};

	Anim *anim_buf;        ///< In this buffer we keep track of the 8bpp indexes so we can do palette animation
	int anim_buf_width;    ///< The width of the animation buffer.
	int anim_buf_height;   ///< The height of the animation buffer.
	Colour16 palette[256]; ///< The current palette.

public:
	Blitter_16bppAnim() :
		anim_buf(NULL),
		anim_buf_width(0),
		anim_buf_height(0)
	{}

	/* virtual */ void Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom);
	/* virtual */ void DrawColourMappingRect(void *dst, int width, int height, PaletteID pal);
	/* virtual */ void SetPixel(void *video, int x, int y, uint8 colour);
	/* virtual */ void DrawRect(void *video, int width, int height, uint8 colour);
	/* virtual */ void CopyFromBuffer(void *video, const void *src, int width, int height);
	/* virtual */ void CopyToBuffer(const void *video, void *dst, int width, int height);
	/* virtual */ void ScrollBuffer(void *video, int &left_ref, int &top_ref, int &width_ref, int &height_ref, int scroll_x, int scroll_y);
	/* virtual */ int BufferSize(int width, int height);
	/* virtual */ void PaletteAnimate(const Palette &palette);
	/* virtual */ Blitter::PaletteAnimation UsePaletteAnimation();
	/* virtual */ int GetBytesPerPixel() { return 3; }

	/* virtual */ const char *GetName() { return "16bpp-anim"; }
	/* virtual */ void PostResize();

	/**
	 * Look up the colour in the current palette.
	 */
	inline Colour16 LookupColourInPalette(uint8 index)
	{
		return this->palette[index];
	}

	template <BlitterMode mode> void Draw(const Blitter::BlitterParams *bp, ZoomLevel zoom);
};

/** Factory for the 16bpp blitter with animation. */
class FBlitter_16bppAnim : public BlitterFactory {
public:
	FBlitter_16bppAnim() : BlitterFactory("16bpp-anim-broken", "16bpp Animation Blitter, currently broken (palette animation)") {}
	/* virtual */ Blitter *CreateInstance() { return new Blitter_16bppAnim(); }
};

#endif /* BLITTER_16BPP_ANIM_HPP */
