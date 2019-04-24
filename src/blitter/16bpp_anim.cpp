/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 16bpp_anim.cpp Implementation of the optimized 16 bpp blitter with animation support, currently broken. */

#include "../stdafx.h"
#include "../video/video_driver.hpp"
#include "../zoom_func.h"
#include "16bpp_anim.hpp"

#include "../table/sprites.h"

/** Instantiation of the 16bpp with animation blitter factory. */
static FBlitter_16bppAnim iFBlitter_16bppAnim;

template <BlitterMode mode>
void Blitter_16bppAnim::Draw(const Blitter::BlitterParams *bp, ZoomLevel zoom)
{
	const Pixel *src, *src_line;
	Colour16 *dst, *dst_line;
	Anim *anim, *anim_line;

	/* Find where to start reading in the source sprite */
	src_line = (const Pixel *)bp->sprite + (bp->skip_top * bp->sprite_width + bp->skip_left) * ScaleByZoom(1, zoom);
	dst_line = (Colour16 *)bp->dst + bp->top * bp->pitch + bp->left;
	anim_line = this->anim_buf + ((Colour16 *)bp->dst - (Colour16 *)_screen.dst_ptr) + bp->top * this->anim_buf_width + bp->left;

	for (int y = 0; y < bp->height; y++) {
		dst = dst_line;
		dst_line += bp->pitch;

		src = src_line;
		src_line += bp->sprite_width * ScaleByZoom(1, zoom);

		anim = anim_line;
		anim_line += this->anim_buf_width;

		for (int x = 0; x < bp->width; x++) {
			switch (mode) {
				case BM_COLOUR_REMAP:
					/* In case the m-channel is zero, do not remap this pixel in any way */
					anim->m = 0;
					anim->v = 0;
					if (src->m == 0) {
						if (src->a != 0) *dst = ComposeColourPA(src->c, src->a, *dst);
					} else {
						uint8 r = bp->remap[src->m];
						if (r != 0) {
							*dst = ComposeColourPA(AdjustBrightness(LookupColourInPalette(r), src->v), src->a, *dst);
							if (src->a == 15 && r >= PALETTE_ANIM_START) {
								anim->m = r - PALETTE_ANIM_START + 1;
								anim->v = src->v >> 1;
							}
						}
					}
					break;

				case BM_TRANSPARENT:
					/* TODO -- We make an assumption here that the remap in fact is transparency, not some colour.
					 *  This is never a problem with the code we produce, but newgrfs can make it fail... or at least:
					 *  we produce a result the newgrf maker didn't expect ;) */

					/* Make the current colour a bit more black, so it looks like this image is transparent */
					if (src->a != 0) *dst = MakeTransparent(*dst, 192);
					anim->m = 0;
					anim->v = 0;
					break;

				default:
					if (src->a == 15 && src->m >= PALETTE_ANIM_START) {
						*dst = AdjustBrightness(LookupColourInPalette(src->m), src->v);
						anim->m = src->m - PALETTE_ANIM_START + 1;
						anim->v = src->v >> 1;
					} else {
						if (src->a != 0) {
							if (src->m >= PALETTE_ANIM_START) {
								*dst = ComposeColourPANoCheck(AdjustBrightness(LookupColourInPalette(src->m), src->v), src->a, *dst);
							} else {
								*dst = ComposeColourPA(src->c, src->a, *dst);
							}
						}
						anim->m = 0;
						anim->v = 0;
					}
					break;
			}
			dst++;
			src += ScaleByZoom(1, zoom);
		}
	}
}

void Blitter_16bppAnim::Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom)
{
	if (_screen_disable_anim) {
		/* This means our output is not to the screen, so we can't be doing any animation stuff, so use our parent Draw() */
		Blitter_16bppOptimized::Draw(bp, mode, zoom);
		return;
	}

	switch (mode) {
		default: NOT_REACHED();
		case BM_NORMAL:       Draw<BM_NORMAL>      (bp, zoom); return;
		case BM_COLOUR_REMAP: Draw<BM_COLOUR_REMAP>(bp, zoom); return;
		case BM_TRANSPARENT:  Draw<BM_TRANSPARENT> (bp, zoom); return;
	}
}

void Blitter_16bppAnim::DrawColourMappingRect(void *dst, int width, int height, PaletteID pal)
{
	if (_screen_disable_anim) {
		/* This means our output is not to the screen, so we can't be doing any animation stuff, so use our parent DrawColourMappingRect() */
		Blitter_16bppOptimized::DrawColourMappingRect(dst, width, height, pal);
		return;
	}

	Colour16 *udst = (Colour16 *)dst;
	Anim *anim;

	anim = this->anim_buf + ((Colour16 *)dst - (Colour16 *)_screen.dst_ptr);

	if (pal == PALETTE_TO_TRANSPARENT) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeTransparent(*udst, 154);
				anim->m = 0;
				anim->v = 0;
				udst++;
				anim++;
			}
			udst = udst - width + _screen.pitch;
			anim = anim - width + this->anim_buf_width;
		} while (--height);
		return;
	}
	if (pal == PALETTE_NEWSPAPER) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeGrey(*udst);
				anim->m = 0;
				anim->v = 0;
				udst++;
				anim++;
			}
			udst = udst - width + _screen.pitch;
			anim = anim - width + this->anim_buf_width;
		} while (--height);
		return;
	}

	DEBUG(misc, 0, "16bpp blitter doesn't know how to draw this colour table ('%d')", pal);
}

void Blitter_16bppAnim::SetPixel(void *video, int x, int y, uint8 colour)
{
	*((Colour16 *)video + x + y * _screen.pitch) = LookupColourInPalette(colour);

	/* Set the colour in the anim-buffer too, if we are rendering to the screen */
	if (_screen_disable_anim) return;
	Anim *anim = this->anim_buf + ((Colour16 *)video - (Colour16 *)_screen.dst_ptr) + x + y * this->anim_buf_width;
	if (colour >= PALETTE_ANIM_START) {
		anim->m = colour - PALETTE_ANIM_START + 1;
		anim->v = DEFAULT_BRIGHTNESS >> 1;
	} else {
		anim->m = 0;
		anim->v = 0;
	}
}

void Blitter_16bppAnim::DrawRect(void *video, int width, int height, uint8 colour)
{
	if (_screen_disable_anim) {
		/* This means our output is not to the screen, so we can't be doing any animation stuff, so use our parent DrawRect() */
		Blitter_16bppOptimized::DrawRect(video, width, height, colour);
		return;
	}

	Colour16 colour16 = LookupColourInPalette(colour);
	Anim *anim_line = this->anim_buf + ((Colour16 *)video - (Colour16 *)_screen.dst_ptr);

	do {
		Colour16 *dst = (Colour16 *)video;
		Anim *anim = anim_line;

		for (int i = width; i > 0; i--) {
			*dst = colour16;
			/* Set the colour in the anim-buffer too */
			if (colour >= PALETTE_ANIM_START) {
				anim->m = colour - PALETTE_ANIM_START + 1;
				anim->v = DEFAULT_BRIGHTNESS >> 1;
			} else {
				anim->m = 0;
				anim->v = 0;
			}
			dst++;
			anim++;
		}
		video = (Colour16 *)video + _screen.pitch;
		anim_line += this->anim_buf_width;
	} while (--height);
}

void Blitter_16bppAnim::CopyFromBuffer(void *video, const void *src, int width, int height)
{
	assert(!_screen_disable_anim);
	assert(video >= _screen.dst_ptr && video <= (Colour16 *)_screen.dst_ptr + _screen.width + _screen.height * _screen.pitch);
	Colour16 *dst = (Colour16 *)video;
	const uint8 *usrc = (const uint8 *)src;
	Anim *anim_line = this->anim_buf + ((Colour16 *)video - (Colour16 *)_screen.dst_ptr);

	for (; height > 0; height--) {
		/* We need to keep those for palette animation. */
		Colour16 *dst_pal = dst;
		Anim *anim_pal = anim_line;

		memcpy(dst, usrc, width * sizeof(Colour16));
		usrc += width * sizeof(Colour16);
		dst += _screen.pitch;
		/* Copy back the anim-buffer */
		memcpy(anim_line, usrc, width * sizeof(Anim));
		usrc += width * sizeof(Anim);
		anim_line += this->anim_buf_width;

		/* Okay, it is *very* likely that the image we stored is using
		 * the wrong palette animated colours. There are two things we
		 * can do to fix this. The first is simply reviewing the whole
		 * screen after we copied the buffer, i.e. run PaletteAnimate,
		 * however that forces a full screen redraw which is expensive
		 * for just the cursor. This just copies the implementation of
		 * palette animation, much cheaper though slightly nastier. */
		for (int i = 0; i < width; i++) {
			uint8 colour = anim_pal->m;
			if (colour) {
				/* Update this pixel */
				*dst_pal = AdjustBrightness(LookupColourInPalette(colour + PALETTE_ANIM_START - 1), anim_pal->v << 1);
			}
			dst_pal++;
			anim_pal++;
		}
	}
}

void Blitter_16bppAnim::CopyToBuffer(const void *video, void *dst, int width, int height)
{
	assert(!_screen_disable_anim);
	assert(video >= _screen.dst_ptr && video <= (Colour16 *)_screen.dst_ptr + _screen.width + _screen.height * _screen.pitch);
	uint8 *udst = (uint8 *)dst;
	const Colour16 *src = (const Colour16 *)video;
	const Anim *anim_line = this->anim_buf + ((const Colour16 *)video - (Colour16 *)_screen.dst_ptr);

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(Colour16));
		src += _screen.pitch;
		udst += width * sizeof(Colour16);
		/* Copy the anim-buffer */
		memcpy(udst, anim_line, width * sizeof(Anim));
		udst += width * sizeof(Anim);
		anim_line += this->anim_buf_width;
	}
}

void Blitter_16bppAnim::ScrollBuffer(void *video, int &left_ref, int &top_ref, int &width_ref, int &height_ref, int scroll_x, int scroll_y)
{
	assert(!_screen_disable_anim);
	assert(video >= _screen.dst_ptr && video <= (Colour16 *)_screen.dst_ptr + _screen.width + _screen.height * _screen.pitch);
	const Anim *src;
	Anim *dst;
	int left = left_ref, top = top_ref, width = width_ref, height = height_ref;

	/* We need to scroll the anim-buffer too */
	if (scroll_y > 0) {
		/* Calculate pointers */
		dst = this->anim_buf + left + (top + height - 1) * this->anim_buf_width;
		src = dst - scroll_y * this->anim_buf_width;

		/* Decrease height and increase top */
		top += scroll_y;
		height -= scroll_y;
		assert(height > 0);

		/* Adjust left & width */
		if (scroll_x >= 0) {
			dst += scroll_x;
			left += scroll_x;
			width -= scroll_x;
		} else {
			src -= scroll_x;
			width += scroll_x;
		}

		for (int h = height; h > 0; h--) {
			memcpy(dst, src, width * sizeof(Anim));
			src -= this->anim_buf_width;
			dst -= this->anim_buf_width;
		}
	} else {
		/* Calculate pointers */
		dst = this->anim_buf + left + top * this->anim_buf_width;
		src = dst - scroll_y * this->anim_buf_width;

		/* Decrease height. (scroll_y is <=0). */
		height += scroll_y;
		assert(height > 0);

		/* Adjust left & width */
		if (scroll_x >= 0) {
			dst += scroll_x;
			left += scroll_x;
			width -= scroll_x;
		} else {
			src -= scroll_x;
			width += scroll_x;
		}

		/* the y-displacement may be 0 therefore we have to use memmove,
		 * because source and destination may overlap */
		for (int h = height; h > 0; h--) {
			memmove(dst, src, width * sizeof(Anim));
			src += _screen.pitch;
			dst += _screen.pitch;
		}
	}

	Blitter_16bppOptimized::ScrollBuffer(video, left_ref, top_ref, width_ref, height_ref, scroll_x, scroll_y);
}

int Blitter_16bppAnim::BufferSize(int width, int height)
{
	return width * height * (sizeof(Anim) + sizeof(Colour16));
}

void Blitter_16bppAnim::PaletteAnimate(const Palette &palette)
{
	assert(!_screen_disable_anim);

	/* If first_dirty is 0, it is for 8bpp indication to send the new
	 *  palette. However, only the animation colours might possibly change.
	 *  Especially when going between toyland and non-toyland. */
	assert(palette.first_dirty == PALETTE_ANIM_START || palette.first_dirty == 0);

	for (int i = 0; i < 256; i++) {
		this->palette[i] = To16(palette.palette[i]);
	}

	const Anim *anim = this->anim_buf;
	Colour16 *dst = (Colour16 *)_screen.dst_ptr;

	/* Let's walk the anim buffer and try to find the pixels */
	for (int y = this->anim_buf_height; y != 0 ; y--) {
		for (int x = this->anim_buf_width; x != 0 ; x--) {
			uint8 colour = anim->m;
			if (colour) {
				/* Update this pixel */
				*dst = AdjustBrightness(LookupColourInPalette(colour + PALETTE_ANIM_START - 1), anim->v << 1);
			}
			dst++;
			anim++;
		}
		dst += _screen.pitch - this->anim_buf_width;
	}

	/* Make sure the backend redraws the whole screen */
	VideoDriver::GetInstance()->MakeDirty(0, 0, _screen.width, _screen.height);
}

Blitter::PaletteAnimation Blitter_16bppAnim::UsePaletteAnimation()
{
	return Blitter::PALETTE_ANIMATION_BLITTER;
}

void Blitter_16bppAnim::PostResize()
{
	if (_screen.width != this->anim_buf_width || _screen.height != this->anim_buf_height) {
		/* The size of the screen changed; we can assume we can wipe all data from our buffer */
		free(this->anim_buf);
		this->anim_buf = CallocT<Anim>(_screen.width * _screen.height);
		this->anim_buf_width = _screen.width;
		this->anim_buf_height = _screen.height;
	}
}
