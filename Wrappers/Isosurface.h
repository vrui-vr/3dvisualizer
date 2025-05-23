/***********************************************************************
Isosurface - Wrapper class for isosurfaces as visualization elements.
Part of the wrapper layer of the templatized visualization
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

#ifndef VISUALIZATION_WRAPPERS_ISOSURFACE_INCLUDED
#define VISUALIZATION_WRAPPERS_ISOSURFACE_INCLUDED

#include <GL/GLVertex.icpp>

#include <Config.h>
#include <Abstract/Element.h>
#include <Templatized/IndexedTriangleSet.h>

/* Forward declarations: */
#if VISUALIZATION_CONFIG_USE_SHADERS
class TwoSidedSurfaceShader;
#endif

namespace Visualization {

namespace Wrappers {

template <class DataSetWrapperParam>
class Isosurface:public Visualization::Abstract::Element
	{
	/* Embedded classes: */
	public:
	typedef Visualization::Abstract::Element Base; // Base class
	typedef DataSetWrapperParam DataSetWrapper; // Compatible data set type
	typedef typename DataSetWrapper::DS DS; // Type of templatized data set
	typedef typename DS::Scalar Scalar; // Scalar type of data set's domain
	static const int dimension=DS::dimension; // Dimension of data set's domain
	typedef typename DataSetWrapper::VScalar VScalar; // Scalar type of scalar extractor
	typedef GLVertex<void,0,void,0,Scalar,Scalar,dimension> Vertex; // Data type for triangle vertices
	typedef Visualization::Templatized::IndexedTriangleSet<Vertex> Surface; // Data structure to represent surfaces
	
	/* Elements: */
	private:
	int scalarVariableIndex; // Index of the scalar variable visualized by the isosurface
	VScalar isovalue; // Isosurface's isovalue
	#if VISUALIZATION_CONFIG_USE_SHADERS
	TwoSidedSurfaceShader* shader; // Shader for the isosurface
	#endif
	Surface surface; // Representation of the isosurface
	
	/* Constructors and destructors: */
	public:
	Isosurface(Visualization::Abstract::VariableManager* sVariableManager,Visualization::Abstract::Parameters* sParameters,int sScalarVariableIndex,VScalar sIsovalue,Cluster::MulticastPipe* pipe); // Creates an empty isosurface for the given parameters
	private:
	Isosurface(const Isosurface& source); // Prohibit copy constructor
	Isosurface& operator=(const Isosurface& source); // Prohibit assignment operator
	public:
	virtual ~Isosurface(void);
	
	/* Methods from class SceneGraph::Node: */
	virtual const char* getClassName(void) const;
	
	/* Methods from class SceneGraph::GraphNode: */
	virtual void glRenderAction(SceneGraph::GLRenderState& renderState) const;
	
	/* Methods from class Visualization::Abstract::Element: */
	virtual std::string getName(void) const;
	virtual size_t getSize(void) const;
	
	/* New methods: */
	Surface& getSurface(void) // Returns the surface representation
		{
		return surface;
		}
	size_t getElementSize(void) const // Returns the number of triangles in the surface representation
		{
		return surface.getNumTriangles();
		}
	};

}

}

#ifndef VISUALIZATION_WRAPPERS_ISOSURFACE_IMPLEMENTATION
#include <Wrappers/Isosurface.icpp>
#endif

#endif
