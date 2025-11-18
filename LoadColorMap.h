/***********************************************************************
LoadColorMap - Helper function to load color maps from text files.
Copyright (c) 2025 Oliver Kreylos

This file is part of the 3D Data Visualizer (Visualizer).

The 3D Data Visualizer is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The 3D Data Visualizer is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the 3D Data Visualizer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef LOADCOLORMAP_INCLUDED
#define LOADCOLORMAP_INCLUDED

#include <utility>
#include <Misc/RGBA.h>
#include <Misc/ColorMap.h>

Misc::ColorMap<Misc::RGBA<float> >* createDefaultColorMap(const std::pair<double,double>& valueRange); // Creates a default grayscale color map covering the given value range
Misc::ColorMap<Misc::RGBA<float> >* loadColorMap(const char* fileName,const std::pair<double,double>& valueRange); // Returns a new color map from reading the given text file and adjusts the color map's value range to the given value range

#endif
