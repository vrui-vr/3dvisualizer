/***********************************************************************
VTKFile - Class to represent an XML-format VTK file containing
volumetric data.
Copyright (c) 2019-2020 Oliver Kreylos

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

#ifndef VISUALIZATION_CONCRETE_VTKFILE_INCLUDED
#define VISUALIZATION_CONCRETE_VTKFILE_INCLUDED

#include <string>
#include <vector>
#include <Misc/SizedTypes.h>
#include <IO/Directory.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

namespace Visualization {

namespace Concrete {

class VTKFile
	{
	/* Embedded classes: */
	public:
	typedef Misc::Float32 Scalar; // Type for point components
	typedef Geometry::Point<Scalar,3> Point; // Type for points
	typedef Misc::Float32 VScalar; // Type for scalar data values and vector data value components
	typedef Geometry::Vector<VScalar,3> VVector; // Type for vector data values
	typedef Misc::UInt32 Index; // Type for indices
	typedef Misc::UInt8 CellType; // Type for cell types
	
	struct Property // Structure representing a vertex or cell property
		{
		/* Elements: */
		public:
		std::string name; // Name of the property
		Index numComponents; // Number of components of the property's values (1: scalar, 3: vector)
		Index firstSliceIndex; // Index of the data value slice containing the property's first component
		};
	
	/* Elements: */
	private:
	std::string gridType; // Type of the data grid (structured, unstructured, etc.)
	Index numVertices; // Number of distinct vertices read from the VTK file
	Point* vertices; // Array of distinct vertices read from the VTK file
	Index numVertexProperties; // Number of properties associated with vertices
	Property* vertexProperties; // Array of vertex property descriptors
	VScalar* vertexPropertySlices; // Array of consecutively-stored vertex property value slices
	Index numCells; // Number of cells read from the VTK file
	CellType* cellTypes; // Array of cell types read from the VTK file
	Index* cellVertexIndices; // Array of cell vertex indices read from the VTK file
	Index numCellProperties; // Number of properties associated with cells
	Property* cellProperties; // Array of cell property descriptors
	VScalar* cellPropertySlices; // Array of consecutively-stored cell property value slices
	
	/* Constructors and destructors: */
	public:
	VTKFile(void); // Creates an "empty" VTK file representation
	private:
	VTKFile(const VTKFile& source); // Prohibit copy constructor
	VTKFile& operator=(const VTKFile& source); // Prohibit assignment operator
	public:
	~VTKFile(void); // Destroys the VTK file representation
	
	/* Methods: */
	void read(const char* vtkFileName); // Reads the contents of a VTK file of the given name
	void read(IO::DirectoryPtr directory,const char* vtkFileName); // Reads the contents of a VTK file of the given name relative to the given directory
	Index getNumVertices(void) const // Returns the number of vertices
		{
		return numVertices;
		}
	const Point* getVertices(void) const // Returns the array of vertices
		{
		return vertices;
		}
	Index getNumVertexProperties(void) const // Returns the number of vertex properties
		{
		return numVertexProperties;
		}
	const Property& getVertexProperty(Index vertexPropertyIndex) const // Returns the vertex property of the given index
		{
		return vertexProperties[vertexPropertyIndex];
		}
	const VScalar* getVertexPropertySlice(Index vertexPropertyIndex,Index componentIndex) const // Returns the vertex property slice for the given property and given component index
		{
		return vertexPropertySlices+(vertexProperties[vertexPropertyIndex].firstSliceIndex+componentIndex)*numVertices;
		}
	Index getNumCells(void) const // Returns the number of cells
		{
		return numCells;
		}
	const CellType* getCellTypes(void) const // Returns the array of cell types
		{
		return cellTypes;
		}
	const Index* getCellVertexIndices(void) const // Returns the array of cell vertex indices
		{
		return cellVertexIndices;
		}
	Index getNumCellProperties(void) const // Returns the number of cell properties
		{
		return numCellProperties;
		}
	const Property& getCellProperty(Index cellPropertyIndex) const // Returns the cell property of the given index
		{
		return cellProperties[cellPropertyIndex];
		}
	const VScalar* getCellPropertySlice(Index cellPropertyIndex,Index componentIndex) const // Returns the cell property slice for the given property and given component index
		{
		return cellPropertySlices+(cellProperties[cellPropertyIndex].firstSliceIndex+componentIndex)*numCells;
		}
	};

}

}

#endif
