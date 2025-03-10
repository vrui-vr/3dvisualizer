/***********************************************************************
SCTFile - Class to represent scalar-valued Cartesian data sets stored
as stacks of greyscale images in the format used by the tomographic
reconstruction code developed at Lawrence Livermore National Laboratory.
Copyright (c) 2012-2024 Oliver Kreylos

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

#include <Concrete/SCTFile.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StdError.h>
#include <Misc/FileNameExtensions.h>
#include <Plugins/FactoryManager.h>
#include <IO/ValueSource.h>
#include <Cluster/MulticastPipe.h>

namespace Visualization {

namespace Concrete {

namespace {

/****************
Helper functions:
****************/

inline const char* writeUInt(unsigned int value)
	{
	static char buffer[20];
	char* bufPtr=buffer+sizeof(buffer);
	*(--bufPtr)='\0';
	do
		{
		*(--bufPtr)=char(value%10U+'0');
		value/=10U;
		}
	while(bufPtr>buffer&&value!=0);
	
	return bufPtr;
	}

template <class ValueParam>
inline
void
readSliceFile(
	IO::FilePtr file,
	int sizeX,
	int sizeY,
	DS::Value* vPtr)
	{
	/* Read and convert all pixels: */
	for(int y=0;y<sizeY;++y)
		for(int x=0;x<sizeX;++x,++vPtr)
			*vPtr=DS::Value(file->read<Value>());
	}

}

/************************
Methods of class SCTFile:
************************/

SCTFile::SCTFile(void)
	:BaseModule("SCTFile")
	{
	}

Visualization::Abstract::DataSet* SCTFile::load(const std::vector<std::string>& args,Cluster::MulticastPipe* pipe) const
	{
	bool master=pipe==0||pipe->isMaster();
	
	/* Open the stack descriptor file: */
	IO::ValueSource stackDescriptor(openFile(args[0]));
	stackDescriptor.setPunctuation("\n");
	stackDescriptor.skipWs();
	
	/* Parse the volume data layout: */
	DS::Index numVertices(0,0,0);
	DS::Size cellSize(0,0,0);
	while(!stackDescriptor.eof())
		{
		/* Read the next tag: */
		std::string tag=stackDescriptor.readString();
		
		/* Process the tag: */
		if(tag=="-rxelements")
			numVertices[2]=stackDescriptor.readInteger();
		else if(tag=="-ryelements")
			numVertices[1]=stackDescriptor.readInteger();
		else if(tag=="-rzelements")
			numVertices[0]=stackDescriptor.readInteger();
		else if(tag=="-rxsize")
			cellSize[2]=DS::Scalar(stackDescriptor.readNumber());
		else if(tag=="-rysize")
			cellSize[1]=DS::Scalar(stackDescriptor.readNumber());
		else if(tag=="-rzsize")
			cellSize[0]=DS::Scalar(stackDescriptor.readNumber());
		
		/* Go to the next line: */
		stackDescriptor.skipLine();
		stackDescriptor.skipWs();
		}
	
	/* Create the data set: */
	Misc::SelfDestructPointer<DataSet> result(new DataSet);
	result->getDs().setData(numVertices,cellSize);
	
	/* Load all stack images: */
	DataSet::DS::Array& vertices=result->getDs().getVertices();
	DS::Value* vertexPtr=vertices.getArray();
	
	if(master)
		std::cout<<"Reading stack slices...   0%"<<std::flush;
	
	for(int sliceIndex=0;sliceIndex<numVertices[0];++sliceIndex)
		{
		/* Generate the slice file base name: */
		std::string sliceFileName=std::string(args[0].c_str(),Misc::getExtension(args[0].c_str()));
		sliceFileName.push_back('_');
		sliceFileName.append(writeUInt(sliceIndex));
		
		/* Generate the slice file header name: */
		std::string sliceHeaderName=sliceFileName;
		sliceHeaderName.append(".spr");
		
		/* Read the slice file header to check for data type and consistency with the stack descriptor: */
		IO::ValueSource sliceDescriptor(openFile(sliceHeaderName));
		sliceDescriptor.skipWs();
		int dimension=sliceDescriptor.readInteger();
		int sizeX=sliceDescriptor.readInteger();
		// DS::Scalar offsetX=DS::Scalar(sliceDescriptor.readNumber());
		sliceDescriptor.readNumber(); // Skip X offset
		DS::Scalar cellSizeX=DS::Scalar(sliceDescriptor.readNumber());
		int sizeY=sliceDescriptor.readInteger();
		// DS::Scalar offsetY=DS::Scalar(sliceDescriptor.readNumber());
		sliceDescriptor.readNumber(); // Skip Y offset
		DS::Scalar cellSizeY=DS::Scalar(sliceDescriptor.readNumber());
		int dataType=sliceDescriptor.readInteger();
		
		/* Check the slice descriptor against the stack descriptor: */
		if(dimension!=2||sizeX!=numVertices[2]||cellSizeX!=cellSize[2]||sizeY!=numVertices[1]||cellSizeY!=cellSize[1])
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Slice %d does not match stack descriptor",sliceIndex);
		if(dataType<0||dataType>3)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Slice %d has unsupported pixel format",sliceIndex);
		
		/* Generate the slice file name: */
		std::string sliceName=sliceFileName;
		sliceName.append(".sdt");
		
		/* Read the slice file: */
		switch(dataType)
			{
			case 0:
				readSliceFile<Misc::UInt8>(openFile(sliceName),sizeX,sizeY,vertexPtr);
				break;
			
			case 1:
				readSliceFile<Misc::UInt16>(openFile(sliceName),sizeX,sizeY,vertexPtr);
				break;
			
			case 2:
				readSliceFile<Misc::UInt32>(openFile(sliceName),sizeX,sizeY,vertexPtr);
				break;
			
			case 3:
				readSliceFile<Misc::Float32>(openFile(sliceName),sizeX,sizeY,vertexPtr);
				break;
			}
		
		/* Go to the next slice: */
		vertexPtr+=sizeY*sizeX;
		
		if(master)
			{
			/* Update the progress counter: */
			std::cout<<"\b\b\b\b"<<std::setw(3)<<((sliceIndex+1)*100)/numVertices[0]<<"%"<<std::flush;
			}
		}
	if(master)
		std::cout<<"\b\b\b\bdone"<<std::endl;
	
	return result.releaseTarget();
	}

}

}

/***************************
Plug-in interface functions:
***************************/

extern "C" Visualization::Abstract::Module* createFactory(Plugins::FactoryManager<Visualization::Abstract::Module>& manager)
	{
	/* Create module object and insert it into class hierarchy: */
	Visualization::Concrete::SCTFile* module=new Visualization::Concrete::SCTFile();
	
	/* Return module object: */
	return module;
	}

extern "C" void destroyFactory(Visualization::Abstract::Module* module)
	{
	delete module;
	}
