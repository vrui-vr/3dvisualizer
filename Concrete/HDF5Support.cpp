/***********************************************************************
HDF5Support - Helper classes to simplify reading data from HDF5 files.
Copyright (c) 2018-2024 Oliver Kreylos

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

#include <Concrete/HDF5Support.h>

#include <stdexcept>
#include <Misc/StdError.h>

namespace HDF5 {

/*********************
Methods of class File:
*********************/

File::File(const char* fileName)
	{
	/* Open the file and check for errors: */
	id=H5Fopen(fileName,H5F_ACC_RDONLY,H5P_DEFAULT);
	if(id<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot open %s",fileName);
	}

File::~File(void)
	{
	H5Fclose(id);
	}

/************************
Methods of class DataSet:
************************/

DataSet::DataSet(File& file,const char* dataSetName)
	{
	/* Open the data set and check for errors: */
	id=H5Dopen2(file.getId(),dataSetName,H5P_DEFAULT);
	if(id<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot access data set %s",dataSetName);
	}

DataSet::~DataSet(void)
	{
	H5Dclose(id);
	}

namespace {

/**************************************************************************
Helper functions to communicate compile-time types to the run-time library:
**************************************************************************/

template <class DataParam>
hid_t getNativeType(void)
	{
	return -1;
	}

template <>
hid_t getNativeType<int>(void)
	{
	return H5T_NATIVE_INT;
	}

template <>
hid_t getNativeType<unsigned int>(void)
	{
	return H5T_NATIVE_UINT;
	}

template <>
hid_t getNativeType<float>(void)
	{
	return H5T_NATIVE_FLOAT;
	}

template <>
hid_t getNativeType<double>(void)
	{
	return H5T_NATIVE_DOUBLE;
	}

}

template <class DataParam>
inline
size_t
DataSet::read(
	size_t& numDimensions,
	size_t*& dimensions,
	DataParam*& data)
	{
	/* Initialize output values: */
	numDimensions=0;
	dimensions=0;
	data=0;
	
	/* Access the data set's data space and data type: */
	HDF5::DataSpace dataSpace(*this);
	HDF5::DataType dataType(*this);
	
	/* Create a data type representing the given memory buffer: */
	HDF5::DataType memDataType(getNativeType<DataParam>());
	
	/* Check that the data type and the given memory buffer are compatible: */
	if(memDataType.getClass()!=dataType.getClass()||(memDataType.isInteger()&&memDataType.isSigned()!=dataType.isSigned()))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Data set's type is incompatible with requested memory type");
	
	/* Check that the data space is simple: */
	if(!dataSpace.isSimple())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Data set has non-simple data space");
	
	/* Retrieve the data space's dimensions and calculate total number of values: */
	std::vector<size_t> dims=dataSpace.getDimensions();
	numDimensions=dims.size();
	dimensions=new size_t[numDimensions];
	size_t dataSize=1;
	for(size_t i=0;i<numDimensions;++i)
		{
		dimensions[i]=dims[i];
		dataSize*=dims[i];
		}
	
	/* Allocate the result buffer: */
	data=new DataParam[dataSize];
	
	/* Read the data set: */
	if(H5Dread(id,memDataType.getId(),H5S_ALL,H5S_ALL,H5P_DEFAULT,data)<0)
		{
		/* Clear the result and signal an error: */
		numDimensions=0;
		delete[] dimensions;
		dimensions=0;
		delete[] data;
		data=0;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot read data set's data into memory buffer");
		}
	
	return dataSize;
	}

/**************************
Methods of class DataSpace:
**************************/

DataSpace::DataSpace(DataSet& dataSet)
	{
	/* Get the data set's data space and check for errors: */
	id=H5Dget_space(dataSet.getId());
	if(id<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot access data set's data space");
	}

DataSpace::~DataSpace(void)
	{
	H5Sclose(id);
	}

bool DataSpace::isSimple(void) const
	{
	/* Get the data space's "simplicity" and check for errors: */
	htri_t result=H5Sis_simple(id);
	if(result<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot determine simplicity of data space");
	
	return result>0;
	}

size_t DataSpace::getNumDimensions(void) const
	{
	/* Get the number of dimensions and check for errors: */
	int result=H5Sget_simple_extent_ndims(id);
	if(result<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve number of data space's dimensions");
	
	return size_t(result);
	}

void DataSpace::getDimensions(hsize_t dimensions[]) const
	{
	/* Get the dimensions and check for errors: */
	if(H5Sget_simple_extent_dims(id,dimensions,0)<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve sizes of data space's dimensions");
	}

std::vector<size_t> DataSpace::getDimensions(void) const
	{
	/* Get the number of dimensions: */
	size_t numDimensions=getNumDimensions();
	
	/* Retrieve the dimensions: */
	hsize_t* dimensions=new hsize_t[numDimensions];
	if(H5Sget_simple_extent_dims(id,dimensions,0)<0)
		{
		delete[] dimensions;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve sizes of data space's dimensions");
		}
	
	/* Convert the result to a vector: */
	std::vector<size_t> result;
	result.reserve(numDimensions);
	for(size_t i=0;i<numDimensions;++i)
		result.push_back(size_t(dimensions[i]));
	
	/* Clean up and return the result: */
	delete[] dimensions;
	return result;
	}

/*************************
Methods of class DataType:
*************************/

DataType::DataType(DataSet& dataSet)
	:defaultType(false)
	{
	/* Get the data set's data type and check for errors: */
	id=H5Dget_type(dataSet.getId());
	if(id<0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot access data set's data type");
	}

DataType::~DataType(void)
	{
	if(!defaultType)
		H5Tclose(id);
	}

size_t DataType::getSize(void) const
	{
	/* Get the size and check for errors: */
	size_t result=H5Tget_size(id);
	if(result==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve data type's size in bytes");
	
	return result;
	}

H5T_class_t DataType::getClass(void) const
	{
	/* Get the class and check for errors: */
	H5T_class_t result=H5Tget_class(id);
	if(result==H5T_NO_CLASS)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve data type's class");
	
	return result;
	}

bool DataType::isSigned(void) const
	{
	/* Get the sign and check for errors: */
	H5T_sign_t result=H5Tget_sign(id);
	if(result==H5T_SGN_ERROR)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve data type's signedness");
	
	return result==H5T_SGN_2;
	}

H5T_order_t DataType::getByteOrder(void) const
	{
	/* Get the byte order and check for errors: */
	H5T_order_t result=H5Tget_order(id);
	if(result==H5T_ORDER_ERROR)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Cannot retrieve data type's byte order");
	
	return result;
	}

/*************************************************
Force instantiations of standard template methods:
*************************************************/

template size_t DataSet::read(size_t& numDimensions,size_t*& dimensions,int*& data);
template size_t DataSet::read(size_t& numDimensions,size_t*& dimensions,unsigned int*& data);
template size_t DataSet::read(size_t& numDimensions,size_t*& dimensions,float*& data);
template size_t DataSet::read(size_t& numDimensions,size_t*& dimensions,double*& data);

}
