/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 16bpp_simple.hpp Simple 16 bpp blitter. */

#ifndef BLITTER_16BPP_SIMPLE_HPP
#define BLITTER_16BPP_SIMPLE_HPP

#include "16bpp_base.hpp"
#include "factory.hpp"

/** The most trivial 32 bpp blitter (without palette animation). */
class Blitter_16bppSimple : public Blitter_16bppBase {
public:
	/* virtual */ void Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom);
	/* virtual */ void DrawColourMappingRect(void *dst, int width, int height, PaletteID pal);
	/* virtual */ Sprite *Encode(const SpriteLoader::Sprite *sprite, AllocatorProc *allocator);
	template <BlitterMode mode> void Draw(const Blitter::BlitterParams *bp, ZoomLevel zoom);

	/* virtual */ const char *GetName() { return "16bpp-simple"; }
};

/** Factory for the simple 16 bpp blitter. */
class FBlitter_16bppSimple : public BlitterFactory {
public:
	FBlitter_16bppSimple() : BlitterFactory("16bpp-simple", "16bpp Simple Blitter (no palette animation)") {}
	/* virtual */ Blitter *CreateInstance() { return new Blitter_16bppSimple(); }
};

#endif /* BLITTER_16BPP_SIMPLE_HPP */
