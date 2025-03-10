/***********************************************************************
AnalyzeFile - Class to encapsulate operations on scalar-valued data sets
stored in Analyze 7.5 format.
Copyright (c) 2006-2024 Oliver Kreylos

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

#include <string>
#include <Misc/StdError.h>
#include <IO/File.h>
#include <IO/SeekableFile.h>
#include <IO/OpenFile.h>
#include <Plugins/FactoryManager.h>

#include <Concrete/AnalyzeFile.h>

namespace Visualization {

namespace Concrete {

namespace {

/*****************
Helper structures:
*****************/

struct HeaderKey
	{
	/* Elements: */
	public:
	int headerSize;
	char dataType[10];
	char dataName[18];
	int extents;
	short int sessionError;
	char regular;
	char hkeyUn0;
	
	/* Methods: */
	Misc::Endianness read(IO::File& file,const std::string& headerFileName)
		{
		/* Treat the file as little endian first: */
		Misc::Endianness fileEndianness=Misc::LittleEndian;
		file.setEndianness(fileEndianness);
		
		/* Read the header size: */
		file.read(headerSize);
		
		/* Check if the header size was read correctly: */
		if(headerSize!=348)
			{
			/* Flip the header size's endianness and check again: */
			Misc::swapEndianness(headerSize);
			if(headerSize!=348)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Illegal header size in input file %s",headerFileName.c_str());
			
			/* Switch to big endianness: */
			Misc::Endianness fileEndianness=Misc::BigEndian;
			file.setEndianness(fileEndianness);
			}
		
		file.read(dataType,10);
		file.read(dataName,18);
		file.read(extents);
		file.read(sessionError);
		file.read(regular);
		file.read(hkeyUn0);
		
		return fileEndianness;
		}
	};

struct ImageDimension
	{
	/* Elements: */
	public:
	short int dim[8];
	short int unused[7];
	short int dataType;
	short int bitPix;
	short int dimUn0;
	float pixDim[8];
	float voxOffset;
	float fUnused[3];
	float calMax;
	float calMin;
	float compressed;
	float verified;
	int glMax;
	int glMin;
	
	/* Methods: */
	void read(IO::File& file)
		{
		file.read(dim,8);
		file.read(unused,7);
		file.read(dataType);
		file.read(bitPix);
		file.read(dimUn0);
		file.read(pixDim,8);
		file.read(voxOffset);
		file.read(fUnused,3);
		file.read(calMax);
		file.read(calMin);
		file.read(compressed);
		file.read(verified);
		file.read(glMax);
		file.read(glMin);
		}
	};

/****************
Helper functions:
****************/

template <class ScalarParam>
void readArray(IO::File& file,Misc::Array<float,3>& array)
	{
	/* Create a temporary array to read a slice of source data: */
	size_t sliceSize=array.getSize(1)*array.getSize(2);
	ScalarParam* slice=new ScalarParam[sliceSize];
	
	/* Read the data by slice in negative order to flip data orientation: */
	float* slicePtr=array.getArray()+(array.getSize(0)-1)*sliceSize;
	for(int z=array.getSize(0)-1;z>=0;--z,slicePtr-=sliceSize)
		{
		/* Read the data slice: */
		file.read(slice,sliceSize);
		
		/* Copy and convert the data: */
		float* dPtr=slicePtr;
		float* dEnd=dPtr+sliceSize;
		const ScalarParam* sPtr=slice;
		for(;dPtr!=dEnd;++dPtr,++sPtr)
			*dPtr=float(*sPtr);
		}
	
	delete[] slice;
	}

}

/****************************
Methods of class AnalyzeFile:
****************************/

AnalyzeFile::AnalyzeFile(void)
	:BaseModule("AnalyzeFile")
	{
	}

Visualization::Abstract::DataSet* AnalyzeFile::load(const std::vector<std::string>& args,Cluster::MulticastPipe* pipe) const
	{
	/* Open the Analyze 7.5 header file: */
	std::string headerFileName=args[0];
	headerFileName.append(".hdr");
	IO::FilePtr headerFile(openFile(headerFileName));
	
	/* Read the header key and determine the file's endianness: */
	HeaderKey hk;
	Misc::Endianness endianness=hk.read(*headerFile,headerFileName);
	
	/* Read the image dimensions: */
	ImageDimension imageDim;
	imageDim.read(*headerFile);
	
	/* Create the data set: */
	DS::Index numVertices;
	DS::Size cellSize;
	for(int i=0;i<3;++i)
		{
		numVertices[i]=imageDim.dim[3-i];
		cellSize[i]=imageDim.pixDim[3-i];
		}
	DataSet* result=new DataSet;
	result->getDs().setData(numVertices,cellSize);
	
	/* Open the image file: */
	std::string imageFileName=args[0];
	imageFileName.append(".img");
	IO::FilePtr imageFile(openFile(imageFileName));
	imageFile->setEndianness(endianness);
	
	/* Read the vertex values from file: */
	switch(imageDim.dataType)
		{
		case 2: // unsigned char
			readArray<unsigned char>(*imageFile,result->getDs().getVertices());
			break;
		
		case 4: // signed short
			readArray<signed short int>(*imageFile,result->getDs().getVertices());
			break;
		
		case 8: // signed int
			readArray<signed int>(*imageFile,result->getDs().getVertices());
			break;
		
		case 16: // float
			readArray<float>(*imageFile,result->getDs().getVertices());
			break;
		
		case 64: // double
			readArray<double>(*imageFile,result->getDs().getVertices());
			break;
		
		default:
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported data type %d in input file %s",imageDim.dataType,imageFileName.c_str());
		}
	
	return result;
	}

}

}

/***************************
Plug-in interface functions:
***************************/

extern "C" Visualization::Abstract::Module* createFactory(Plugins::FactoryManager<Visualization::Abstract::Module>& manager)
	{
	/* Create module object and insert it into class hierarchy: */
	Visualization::Concrete::AnalyzeFile* module=new Visualization::Concrete::AnalyzeFile();
	
	/* Return module object: */
	return module;
	}

extern "C" void destroyFactory(Visualization::Abstract::Module* module)
	{
	delete module;
	}
