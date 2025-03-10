/***********************************************************************
VTKFileReader - Class to read VTK files in XML format.
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

#ifndef VISUALIZATION_CONCRETE_VTKFILEREADER_INCLUDED
#define VISUALIZATION_CONCRETE_VTKFILEREADER_INCLUDED

#include <string>
#include <vector>
#include <Misc/Endianness.h>
#include <IO/Directory.h>
#include <IO/XMLSource.h>

#include <Concrete/VTKFile.h>

/* Forward declarations: */
namespace IO {
class File;
}

namespace Visualization {

namespace Concrete {

class VTKFileReader // Class to read the contents of a VTK file into a "raw" representation
	{
	/* Embedded classes: */
	public:
	typedef VTKFile::Scalar Scalar;
	typedef VTKFile::VScalar VScalar;
	typedef VTKFile::Index Index;
	typedef VTKFile::CellType CellType;
	private:
	struct ComponentType; // Structure describing the file type of a data array's components
	struct DataArray; // Structure describing a data array after parsing a DataArray element's attributes
	
	public:
	struct Property // Structure for a vertex or cell property
		{
		/* Elements: */
		public:
		std::string name; // The property's name
		Index numComponents; // The property's number of components (1: scalar, 3: vector, ...)
		std::vector<VScalar> components; // The property's list of interleaved components
		};
	
	typedef std::vector<Property*> PropertyList; // Type for lists of properties
	
	/* Elements: */
	private:
	IO::DirectoryPtr baseDirectory; // Directory containing the VTK file
	IO::XMLSource vtk; // Low-level XML parser for the VTK file
	std::string gridType; // Type of grid
	bool compressed; // Flag if binary data arrays are zlib-compressed
	Misc::Endianness endianness; // Byte order for binary data arrays
	size_t headerIntSize; // Size of integers used in binary data array headers (4 or 8 bytes)
	std::vector<Scalar> vertexComponents; // List of interleaved vertex components
	std::vector<Property*> vertexProperties; // List of vertex properties
	std::vector<CellType> cellTypes; // List of cell types for all cells
	std::vector<Index> cellVertexIndices; // List of vertex indices for all cells
	std::vector<Property*> cellProperties; // List of cell properties
	
	/* Private methods: */
	void enterElement(const char* methodName,const char* elementName); // Enters an element's body, skipping leftover attributes; throws exception if body is empty
	void leaveElement(const char* methodName,const char* elementName); // Leaves an element's body; throws exception if closing tag is missing/mismatched
	static ComponentType parseComponentType(const std::string& typeName); // Parses the given component type name; throws exeption if type name is invalid
	DataArray parseDataArrayHeader(void);
	template <class FileScalarParam,class ArrayScalarParam>
	void readBinaryDataArray(const DataArray& dataArray,IO::File& data,std::vector<ArrayScalarParam>& array);
	template <class ArrayScalarParam>
	void readDataArray(const DataArray& dataArray,std::vector<ArrayScalarParam>& array); // Appends the contents of the current DataArray element to the given data array
	void processPoints(Index numVertices);
	void processCells(Index numCells);
	void processPointOrCellData(PropertyList& properties,Index numValues,const char* methodName,const char* elementName);
	void processUnstructuredGrid(void);
	void processParallelPointOrCellData(PropertyList& properties,const char* methodName,const char* elementName);
	void processParallelUnstructuredGrid(void);
	
	/* Constructors and destructors: */
	public:
	VTKFileReader(IO::DirectoryPtr directory,const char* vtkFileName); // Creates a reader for the VTK file of the given name relative to the given directory
	~VTKFileReader(void); // Destroys the VTK file reader
	
	/* Methods: */
	void addVertexProperty(const std::string& name,Index numComponents); // Adds an empty vertex property of the given name and number of components
	void addCellProperty(const std::string& name,Index numComponents); // Adds an empty cell property of the given name and number of components
	void read(void); // Reads the VTK file's contents
	const std::vector<Scalar>& getVertexComponents(void) const // Returns the list of read vertex components
		{
		return vertexComponents;
		}
	const PropertyList& getVertexProperties(void) const // Returns the list of per-vertex properties
		{
		return vertexProperties;
		}
	const std::vector<CellType>& getCellTypes(void) const // Returns the list of read cell types
		{
		return cellTypes;
		}
	const std::vector<Index>& getCellVertexIndices(void) const // Returns the list of read cell vertex indices
		{
		return cellVertexIndices;
		}
	const PropertyList& getCellProperties(void) const // Returns the list of per-cell properties
		{
		return cellProperties;
		}
	};

}

}

#endif
