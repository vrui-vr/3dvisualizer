/***********************************************************************
DataSetRenderer - Abstract base class to render the structure of data
sets using OpenGL.
Part of the abstract interface to the templatized visualization
components.
Copyright (c) 2005-2024 Oliver Kreylos

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

#include <Misc/StdError.h>

#include <Abstract/DataSetRenderer.h>

namespace Visualization {

namespace Abstract {

/********************************
Methods of class DataSetRenderer:
********************************/

const char* DataSetRenderer::getClassName(void) const
	{
	return "3DVisualizer::DataSetRenderer";
	}

void DataSetRenderer::setGridLineWidth(float newGridLineWidth)
	{
	gridLineWidth=newGridLineWidth;
	}

void DataSetRenderer::setGridOpacity(float newGridOpacity)
	{
	gridOpacity=newGridOpacity;
	}

const char* DataSetRenderer::getRenderingModeName(int renderingModeIndex) const
	{
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid rendering mode index %d",renderingModeIndex);
	return 0;
	}

void DataSetRenderer::setRenderingMode(int renderingModeIndex)
	{
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid rendering mode index %d",renderingModeIndex);
	}

void DataSetRenderer::highlightLocator(const DataSet::Locator* locator,SceneGraph::GLRenderState& renderState) const
	{
	}

}

}
