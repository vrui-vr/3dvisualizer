/***********************************************************************
DataSetRenderer - Abstract base class to render the structure of data
sets using OpenGL.
Part of the abstract interface to the templatized visualization
components.
Copyright (c) 2005-2023 Oliver Kreylos

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

#ifndef VISUALIZATION_ABSTRACT_DATASETRENDERER_INCLUDED
#define VISUALIZATION_ABSTRACT_DATASETRENDERER_INCLUDED

#include <SceneGraph/GraphNode.h>

#include <Abstract/DataSet.h>

namespace Visualization {

namespace Abstract {

class DataSetRenderer:public SceneGraph::GraphNode
	{
	/* Elements: */
	protected:
	float gridLineWidth; // Pixel width to render grid lines
	float gridOpacity; // Opacity to render grids
	
	/* Constructors and destructors: */
	public:
	DataSetRenderer(void) // Default constructor
		:gridLineWidth(1.0f),gridOpacity(0.25f)
		{
		}
	private:
	DataSetRenderer(const DataSetRenderer& source); // Prohibit copy constructor
	DataSetRenderer& operator=(const DataSetRenderer& source); // Prohibit assignment operator
	public:
	
	/* Methods from class SceneGraph::Node: */
	virtual const char* getClassName(void) const;
	
	/* New methods: */
	float getGridLineWidth(void) const // Returns the grid line width
		{
		return gridLineWidth;
		}
	float getGridOpacity(void) const // Returns the grid opacity
		{
		return gridOpacity;
		}
	void setGridLineWidth(float newGridLineWidth); // Sets the grid line width
	void setGridOpacity(float newGridOpacity); // Sets the grid opacity
	virtual int getNumRenderingModes(void) const =0; // Returns the number of rendering modes supported by the renderer
	virtual const char* getRenderingModeName(int renderingModeIndex) const; // Returns the name of a supported rendering mode
	virtual int getRenderingMode(void) const =0; // Returns the current rendering mode
	virtual void setRenderingMode(int renderingModeIndex); // Sets the given rendering mode for future rendering
	virtual void highlightLocator(const DataSet::Locator* locator,SceneGraph::GLRenderState& renderState) const; // Highlights the given data set locator
	};

}

}

#endif
