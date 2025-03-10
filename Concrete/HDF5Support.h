/***********************************************************************
HDF5Support - Helper classes to simplify reading data from HDF5 files.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef VISUALIZATION_CONCRETE_HDF5SUPPORT_INCLUDED
#define VISUALIZATION_CONCRETE_HDF5SUPPORT_INCLUDED

#include <hdf5.h>
#include <vector>

namespace HDF5 {

class File // Class for HDF5 files
	{
	/* Elements: */
	private:
	hid_t id; // ID representing the file
	
	/* Constructors and destructors: */
	public:
	File(const char* fileName); // Opens an HDF5 file for reading
	~File(void); // Closes the file
	
	/* Methods: */
	hid_t getId(void) // Returns the file's ID
		{
		return id;
		}
	};

class DataSet // Class for HDF5 data sets
	{
	/* Elements: */
	private:
	hid_t id; // ID representing the data set
	
	/* Constructors and destructors: */
	public:
	DataSet(File& file,const char* dataSetName); // Opens the data set of the given name inside the given file
	~DataSet(void); // Releases the data set
	
	/* Methods: */
	hid_t getId(void) // Returns the data set's ID
		{
		return id;
		}
	template <class DataParam>
	size_t read(size_t& numDimensions,size_t*& dimensions,DataParam*& data); // Reads a data set's contents into a memory buffer, returning the data set's memory layout and total number of elements
	};

class DataSpace // Class for HDF5 data spaces
	{
	/* Elements: */
	private:
	hid_t id; // ID representing the data space
	
	/* Constructors and destructors: */
	public:
	DataSpace(DataSet& dataSet); // Accesses the data space of the given data set
	~DataSpace(void); // Releases the data space
	
	/* Methods: */
	hid_t getId(void) // Returns the data space's ID
		{
		return id;
		}
	bool isSimple(void) const; // Returns true if the data space is simple
	size_t getNumDimensions(void) const; // Returns the number of dimensions (data axes) of the data space
	void getDimensions(hsize_t dimensions[]) const; // Reads the sizes of the data space's dimensions into the provided array, which is assumed to be of sufficient size
	std::vector<size_t> getDimensions(void) const; // Returns the sizes of the data space's dimensions as a vector
	};

class DataType // Class for HDF5 data types
	{
	/* Elements: */
	private:
	hid_t id; // ID representing the data type
	bool defaultType; // Flag if this data type is a default type and does not have to be closed
	
	/* Constructors and destructors: */
	public:
	DataType(hid_t sId) // Creates a wrapper around a default data type
		:id(sId),defaultType(true)
		{
		}
	DataType(DataSet& dataSet); // Accesses the data type of the given data set
	~DataType(void); // Releases the data type
	
	/* Methods: */
	hid_t getId(void) // Returns the data type's ID
		{
		return id;
		}
	size_t getSize(void) const; // Returns the data type's total size in bytes
	H5T_class_t getClass(void) const; // Returns the data type's class
	bool isInteger(void) const // Returns true if the data type is of integer class
		{
		/* Check if the class is integer: */
		return getClass()==H5T_INTEGER;
		}
	bool isSigned(void) const; // Returns true if an integer-class type is signed
	H5T_order_t getByteOrder(void) const; // Returns the data type's byte order
	};

}

#endif
