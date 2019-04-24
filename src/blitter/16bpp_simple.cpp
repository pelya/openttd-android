/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_simple.cpp Implementation of the simple 16 bpp blitter. */

#include "../stdafx.h"
#include "../zoom_func.h"
#include "16bpp_simple.hpp"

#include "../table/sprites.h"

/** Instantiation of the simple 16bpp blitter factory. */
static FBlitter_16bppSimple iFBlitter_16bppSimple;

template <BlitterMode mode>
void Blitter_16bppSimple::Draw(const Blitter::BlitterParams *bp, ZoomLevel zoom)
{
	const Pixel *src, *src_line;
	Colour16 *dst, *dst_line;

	/* Find where to start reading in the source sprite */
	src_line = (const Pixel *)bp->sprite + (bp->skip_top * bp->sprite_width + bp->skip_left) * ScaleByZoom(1, zoom);
	dst_line = (Colour16 *)bp->dst + bp->top * bp->pitch + bp->left;

	for (int y = 0; y < bp->height; y++) {
		dst = dst_line;
		dst_line += bp->pitch;

		src = src_line;
		src_line += bp->sprite_width * ScaleByZoom(1, zoom);

		for (int x = 0; x < bp->width; x++) {
			switch (mode) {
				case BM_COLOUR_REMAP:
					/* In case the m-channel is zero, do not remap this pixel in any way */
					if (src->m == 0) {
						if (src->a != 0) *dst = ComposeColourPA(src->c, src->a, *dst);
					} else {
						if (bp->remap[src->m] != 0) *dst = ComposeColourPA(AdjustBrightness(LookupColourInPalette(bp->remap[src->m]), src->v), src->a, *dst);
					}
					break;

				case BM_CRASH_REMAP:
					if (src->m == 0) {
						if (src->a != 0) {
							uint8 g = MakeDark(src->c);
							*dst = ComposeColourRGBA(g, g, g, src->a, *dst);
						}
					} else {
						if (bp->remap[src->m] != 0) *dst = ComposeColourPA(AdjustBrightness(LookupColourInPalette(bp->remap[src->m]), src->v), src->a, *dst);
					}
					break;

				case BM_TRANSPARENT:
					/* TODO -- We make an assumption here that the remap in fact is transparency, not some colour.
					 *  This is never a problem with the code we produce, but newgrfs can make it fail... or at least:
					 *  we produce a result the newgrf maker didn't expect ;) */

					/* Make the current colour a bit more black, so it looks like this image is transparent */
					if (src->a != 0) *dst = MakeTransparent(*dst, 192);
					break;

				case BM_BLACK_REMAP:
					if (src->a != 0) {
						*dst = Colour16(0, 0, 0);
					}
					break;

				default:
					if (src->a != 0) *dst = ComposeColourPA(src->c, src->a, *dst);
					break;
			}
			dst++;
			src += ScaleByZoom(1, zoom);
		}
	}
}

void Blitter_16bppSimple::Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom)
{
	switch (mode) {
		default: NOT_REACHED();
		case BM_NORMAL:       Draw<BM_NORMAL>      (bp, zoom); return;
		case BM_COLOUR_REMAP: Draw<BM_COLOUR_REMAP>(bp, zoom); return;
		case BM_TRANSPARENT:  Draw<BM_TRANSPARENT> (bp, zoom); return;
		case BM_CRASH_REMAP:  Draw<BM_CRASH_REMAP> (bp, zoom); return;
		case BM_BLACK_REMAP:  Draw<BM_BLACK_REMAP> (bp, zoom); return;
	}
}

void Blitter_16bppSimple::DrawColourMappingRect(void *dst, int width, int height, PaletteID pal)
{
	Colour16 *udst = (Colour16 *)dst;

	if (pal == PALETTE_TO_TRANSPARENT) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeTransparent(*udst, 154);
				udst++;
			}
			udst = udst - width + _screen.pitch;
		} while (--height);
		return;
	}
	if (pal == PALETTE_NEWSPAPER) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeGrey(*udst);
				udst++;
			}
			udst = udst - width + _screen.pitch;
		} while (--height);
		return;
	}

	DEBUG(misc, 0, "16bpp blitter doesn't know how to draw this colour table ('%d')", pal);
}

Sprite *Blitter_16bppSimple::Encode(const SpriteLoader::Sprite *sprite, AllocatorProc *allocator)
{
	Pixel *dst;
	Sprite *dest_sprite = (Sprite *)allocator(sizeof(*dest_sprite) + (size_t)sprite->height * (size_t)sprite->width * sizeof(Pixel));

	dest_sprite->height = sprite->height;
	dest_sprite->width  = sprite->width;
	dest_sprite->x_offs = sprite->x_offs;
	dest_sprite->y_offs = sprite->y_offs;

	dst = (Pixel *)dest_sprite->data;
	SpriteLoader::CommonPixel *src = (SpriteLoader::CommonPixel *)sprite->data;

	for (int i = 0; i < sprite->height * sprite->width; i++) {
		if (src->m == 0) {
			dst[i].c = To16(src->r, src->g, src->b);
			dst[i].a = src->a / 16;
			dst[i].m = 0;
			dst[i].v = 0;
		} else {
			/* Get brightest value */
			uint8 rgb_max = max(src->r, max(src->g, src->b));
#if 0
			/* Pre-convert the mapping channel to a RGB value,
			   use 32bpp AdjustBrightness() variant for better colors,
			   because this function is not called each frame */
			if (rgb_max == 0) rgb_max = Blitter_32bppBase::DEFAULT_BRIGHTNESS;
			dst[i].c = To16(Blitter_32bppBase::AdjustBrightness(LookupColourInPalette32(src->m), rgb_max));
			dst[i].v = rgb_max / 16;
#endif
			rgb_max /= 16;

			/* Black pixel (8bpp or old 32bpp image), so use default value */
			if (rgb_max == 0) rgb_max = DEFAULT_BRIGHTNESS;

			/* Pre-convert the mapping channel to a RGB value,
			   use 32bpp AdjustBrightness() variant for better colors,
			   because this function is not called each frame */
			dst[i].c = AdjustBrightness(LookupColourInPalette(src->m), rgb_max);
			dst[i].v = rgb_max;

			dst[i].a = src->a / 16;
			dst[i].m = src->m;
		}
		src++;
	}

	return dest_sprite;
}
