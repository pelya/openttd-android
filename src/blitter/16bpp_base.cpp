/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 16bpp_base.cpp Implementation of base for 32 bpp blitters. */

#include "../stdafx.h"
#include "16bpp_base.hpp"

void *Blitter_16bppBase::MoveTo(void *video, int x, int y)
{
	return (uint16 *)video + x + y * _screen.pitch;
}

void Blitter_16bppBase::SetPixel(void *video, int x, int y, uint8 colour)
{
	*((Colour16 *)video + x + y * _screen.pitch) = LookupColourInPalette(colour);
}

void Blitter_16bppBase::DrawRect(void *video, int width, int height, uint8 colour)
{
	Colour16 target = LookupColourInPalette(colour);

	do {
		Colour16 *dst = (Colour16 *)video;
		for (int i = width; i > 0; i--) {
			*dst = target;
			dst++;
		}
		video = (uint16 *)video + _screen.pitch;
	} while (--height);
}

void Blitter_16bppBase::CopyFromBuffer(void *video, const void *src, int width, int height)
{
	uint16 *dst = (uint16 *)video;
	const uint16 *usrc = (const uint16 *)src;

	for (; height > 0; height--) {
		memcpy(dst, usrc, width * sizeof(uint16));
		usrc += width;
		dst += _screen.pitch;
	}
}

void Blitter_16bppBase::CopyToBuffer(const void *video, void *dst, int width, int height)
{
	uint16 *udst = (uint16 *)dst;
	const uint16 *src = (const uint16 *)video;

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint16));
		src += _screen.pitch;
		udst += width;
	}
}

void Blitter_16bppBase::CopyImageToBuffer(const void *video, void *dst, int width, int height, int dst_pitch)
{
	uint16 *udst = (uint16 *)dst;
	const uint16 *src = (const uint16 *)video;

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint16));
		src += _screen.pitch;
		udst += dst_pitch;
	}
}

void Blitter_16bppBase::ScrollBuffer(void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y)
{
	const Colour16 *src;
	Colour16 *dst;

	if (scroll_y > 0) {
		/* Calculate pointers */
		dst = (Colour16 *)video + left + (top + height - 1) * _screen.pitch;
		src = dst - scroll_y * _screen.pitch;

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
			memcpy(dst, src, width * sizeof(Colour16));
			src -= _screen.pitch;
			dst -= _screen.pitch;
		}
	} else {
		/* Calculate pointers */
		dst = (Colour16 *)video + left + top * _screen.pitch;
		src = dst - scroll_y * _screen.pitch;

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
			memmove(dst, src, width * sizeof(Colour16));
			src += _screen.pitch;
			dst += _screen.pitch;
		}
	}
}

int Blitter_16bppBase::BufferSize(int width, int height)
{
	return width * height * sizeof(Colour16);
}

void Blitter_16bppBase::PaletteAnimate(const Palette &palette)
{
	/* By default, 16bpp doesn't have palette animation */
}

Blitter::PaletteAnimation Blitter_16bppBase::UsePaletteAnimation()
{
	return Blitter::PALETTE_ANIMATION_NONE;
}
