/***********************************************************************
MultiChannelImageStack - Class to represent multivariate scalaar-valued
Cartesian data sets stored as multiple matching stacks of color or
greyscale images.
Copyright (c) 2009-2024 Oliver Kreylos

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

#include <Concrete/MultiChannelImageStack.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <Misc/SelfDestructPointer.h>
#include <Misc/PrintfTemplateTests.h>
#include <Misc/StdError.h>
#include <Misc/Timer.h>
#include <Plugins/FactoryManager.h>
#include <Math/Math.h>
#include <Images/BaseImage.h>
#include <Images/ReadImageFile.h>

namespace Visualization {

namespace Concrete {

namespace {

/**************
Helper classes:
**************/

struct StackDescriptor
	{
	/* Elements: */
	public:
	DS& dataSet;
	DS::Index numVertices;
	DS::Size cellSize;
	int dsPartsMask;
	bool haveDs;
	IO::DirectoryPtr imageDirectory;
	int imageIndexStart;
	int imageIndexStep;
	int regionOrigin[2];
	bool master;
	
	/* Constructors and destructors: */
	StackDescriptor(DS& sDataSet,IO::DirectoryPtr sImageDirectory,bool sMaster)
		:dataSet(sDataSet),
		 numVertices(0,0,0),cellSize(0,0,0),dsPartsMask(0x0),haveDs(false),
		 imageDirectory(sImageDirectory),
		 imageIndexStart(0),imageIndexStep(1),
		 master(sMaster)
		{
		regionOrigin[0]=regionOrigin[1]=0;
		}
	
	/* Methods: */
	void update(int dsPart)
		{
		dsPartsMask|=dsPart;
		if(dsPartsMask==0x7&&!haveDs)
			{
			dataSet.setData(numVertices,cellSize,0);
			haveDs=true;
			}
		}
	};

/****************
Helper functions:
****************/

template <class SourceScalarParam>
Value convertPixel(SourceScalarParam pixel) // Generic function to convert pixel values
	{
	return Value(pixel);
	}

template <>
Value convertPixel(signed char pixel) // Shifts signed 8-bit pixel
	{
	return Value(int(pixel)+128);
	}

template <>
Value convertPixel(short pixel) // Shifts signed 16-bit pixel
	{
	return Value(int(pixel)+32768);
	}

template <>
Value convertPixel(unsigned int pixel) // Squishes 32-bit pixel into 16-bit pixel
	{
	return Value(pixel>>16);
	}

template <>
Value convertPixel(int pixel) // Shifts and squishes signed 32-bit pixel into 16-bit pixel
	{
	return Value((long(pixel)+0x80000000L)>>16);
	}

template <class ImageScalarParam>
inline
void
copyGreyscalePixels(
	Value* slicePtr,
	const int regionOrigin[2],
	const DS::Index& numVertices,
	const Images::BaseImage& image)
	{
	ptrdiff_t rowStride=numVertices.calcIncrement(1);
	ptrdiff_t colStride=numVertices.calcIncrement(0);
	
	/* Copy image pixels row-by-row: */
	Value* sliceRowPtr=slicePtr;
	for(int y=0;y<numVertices[1];++y,sliceRowPtr+=rowStride)
		{
		/* Access the image's pixel row: */
		const ImageScalarParam* imagePtr=static_cast<const ImageScalarParam*>(image.getPixelRow(regionOrigin[1]+y))+regionOrigin[0];
		Value* sliceColPtr=sliceRowPtr;
		for(int x=0;x<numVertices[0];++x,++imagePtr,sliceColPtr+=colStride)
			*sliceColPtr=convertPixel(*imagePtr);
		}
	}

void loadGreyscaleImageStack(StackDescriptor& sd,int newSliceIndex,const char* imageFileNameTemplate)
	{
	if(sd.master)
		std::cout<<"Reading greyscale image stack "<<imageFileNameTemplate<<"...   0%"<<std::flush;
	Misc::Timer loadTimer;
	
	/* Load all stack images into the dataset's 3D array: */
	Value* slicePtr=sd.dataSet.getSliceArray(newSliceIndex);
	for(int imageIndex=0;imageIndex<sd.numVertices[2];++imageIndex,++slicePtr)
		{
		/* Generate the image file name: */
		char imageFileName[1024];
		snprintf(imageFileName,sizeof(imageFileName),imageFileNameTemplate,imageIndex*sd.imageIndexStep+sd.imageIndexStart);
		
		/* Load the image: */
		Images::BaseImage image=Images::readGenericImageFile(*sd.imageDirectory,imageFileName);
		
		/* Check if the image conforms: */
		if(image.getSize(0)<(unsigned int)(sd.regionOrigin[0]+sd.numVertices[0])||image.getSize(1)<(unsigned int)(sd.regionOrigin[1]+sd.numVertices[1]))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Size of image file \"%s\" does not match image stack size",imageFileName);
		
		/* Convert the image to greyscale: */
		image=image.toGrey().dropAlpha();
		
		/* Copy the image's pixels into the data set: */
		switch(image.getScalarType())
			{
			case GL_BYTE:
				copyGreyscalePixels<signed char>(slicePtr,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_UNSIGNED_BYTE:
				copyGreyscalePixels<unsigned char>(slicePtr,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_SHORT:
				copyGreyscalePixels<short>(slicePtr,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_UNSIGNED_SHORT:
				copyGreyscalePixels<unsigned short>(slicePtr,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_INT:
				copyGreyscalePixels<int>(slicePtr,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_UNSIGNED_INT:
				copyGreyscalePixels<unsigned int>(slicePtr,sd.regionOrigin,sd.numVertices,image);
				break;
			
			default:
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image file \"%s\" has unsupported pixel format",imageFileName);
			}
		
		if(sd.master)
			std::cout<<"\b\b\b\b"<<std::setw(3)<<((imageIndex+1)*100)/sd.numVertices[2]<<"%"<<std::flush;
		}
	
	loadTimer.elapse();
	if(sd.master)
		std::cout<<"\b\b\b\bdone in "<<loadTimer.getTime()*1000.0<<" ms"<<std::endl;
	}

template <class ImageScalarParam>
inline
void
copyRGBPixels(
	Value* slices[3],
	ptrdiff_t sliceIndex,
	const int regionOrigin[2],
	const DS::Index& numVertices,
	const Images::BaseImage& image)
	{
	ptrdiff_t rowStride=numVertices.calcIncrement(1);
	ptrdiff_t colStride=numVertices.calcIncrement(0);
	
	/* Copy image pixels row-by-row: */
	ptrdiff_t sliceRowIndex=sliceIndex;
	for(int y=0;y<numVertices[1];++y,sliceRowIndex+=rowStride)
		{
		/* Access the image's pixel row: */
		const ImageScalarParam* imagePtr=static_cast<const ImageScalarParam*>(image.getPixelRow(regionOrigin[1]+y))+regionOrigin[0]*3;
		ptrdiff_t sliceColIndex=sliceRowIndex;
		for(int x=0;x<numVertices[0];++x,sliceColIndex+=colStride)
			for(int i=0;i<3;++i,++imagePtr)
				slices[i][sliceColIndex]=convertPixel(*imagePtr);
		}
	}

void loadColorImageStack(StackDescriptor& sd,const int newSliceIndices[3],const char* imageFileNameTemplate)
	{
	if(sd.master)
		std::cout<<"Reading color image stack "<<imageFileNameTemplate<<"...   0%"<<std::flush;
	Misc::Timer loadTimer;
	
	/* Load all stack images into the dataset's 3D array: */
	Value* slices[3];
	for(int i=0;i<3;++i)
		slices[i]=sd.dataSet.getSliceArray(newSliceIndices[i]);
	ptrdiff_t sliceIndex=0;
	for(int imageIndex=0;imageIndex<sd.numVertices[2];++imageIndex,++sliceIndex)
		{
		/* Generate the image file name: */
		char imageFileName[1024];
		snprintf(imageFileName,sizeof(imageFileName),imageFileNameTemplate,imageIndex*sd.imageIndexStep+sd.imageIndexStart);
		
		/* Load the image: */
		Images::BaseImage image=Images::readGenericImageFile(*sd.imageDirectory,imageFileName);
		
		/* Drop the image's alpha channel: */
		image=image.dropAlpha();
		
		/* Check if the image has three channels: */
		if(image.getNumChannels()!=3)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image file \"%s\" is not an RGB color image",imageFileName);
		
		/* Check if the image conforms: */
		if(image.getSize(0)<(unsigned int)(sd.regionOrigin[0]+sd.numVertices[0])||image.getSize(1)<(unsigned int)(sd.regionOrigin[1]+sd.numVertices[1]))
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Size of image file \"%s\" does not match image stack size",imageFileName);
		
		/* Copy the image's pixels into the data set: */
		switch(image.getScalarType())
			{
			case GL_BYTE:
				copyRGBPixels<signed char>(slices,sliceIndex,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_UNSIGNED_BYTE:
				copyRGBPixels<unsigned char>(slices,sliceIndex,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_SHORT:
				copyRGBPixels<short>(slices,sliceIndex,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_UNSIGNED_SHORT:
				copyRGBPixels<unsigned short>(slices,sliceIndex,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_INT:
				copyRGBPixels<int>(slices,sliceIndex,sd.regionOrigin,sd.numVertices,image);
				break;
			
			case GL_UNSIGNED_INT:
				copyRGBPixels<unsigned int>(slices,sliceIndex,sd.regionOrigin,sd.numVertices,image);
				break;
			
			default:
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Image file \"%s\" has unsupported pixel format",imageFileName);
			}
		if(sd.master)
			std::cout<<"\b\b\b\b"<<std::setw(3)<<((imageIndex+1)*100)/sd.numVertices[2]<<"%"<<std::flush;
		}
	loadTimer.elapse();
	if(sd.master)
		std::cout<<"\b\b\b\bdone in "<<loadTimer.getTime()*1000.0<<" ms"<<std::endl;
	}

void filterImageStack(StackDescriptor& sd,int sliceIndex,bool medianFilter,bool lowpassFilter)
	{
	/* Get a pointer to the slice: */
	Value* slicePtr=sd.dataSet.getSliceArray(sliceIndex);
	if(sd.master)
		std::cout<<"Filtering image stack...   0%"<<std::flush;
	Misc::Timer filterTimer;
	
	/* Create a buffer for a single voxel pile: */
	Value* pileBuffer=new Value[sd.numVertices[2]];
	
	/* Filter all pixels through all images: */
	Value* columnPtr=slicePtr;
	for(int x=0;x<sd.numVertices[0];++x,columnPtr+=sd.dataSet.getVertexStride(0))
		{
		Value* pilePtr=columnPtr;
		for(int y=0;y<sd.numVertices[1];++y,pilePtr+=sd.dataSet.getVertexStride(1))
			{
			Value* vPtr=pilePtr;
			int vInc=sd.dataSet.getVertexStride(2);
			Value* pPtr=pileBuffer;
			
			if(medianFilter)
				{
				/* Run a median filter over the pixel pile: */
				*pPtr=*vPtr;
				vPtr+=vInc;
				++pPtr;
				for(int z=2;z<sd.numVertices[2];++z,vPtr+=vInc,++pPtr)
					{
					if(vPtr[-vInc]<vPtr[0])
						{
						if(vPtr[0]<vPtr[vInc])
							*pPtr=vPtr[0];
						else
							*pPtr=vPtr[-vInc]<vPtr[vInc]?vPtr[vInc]:vPtr[-vInc];
						}
					else
						{
						if(vPtr[-vInc]<vPtr[vInc])
							*pPtr=vPtr[-vInc];
						else
							*pPtr=vPtr[0]<vPtr[vInc]?vPtr[vInc]:vPtr[0];
						}
					}
				*pPtr=*vPtr;
				}
			else
				{
				/* Copy the voxel pile into the pile buffer for the lowpass filter: */
				for(int z=0;z<sd.numVertices[2];++z,vPtr+=vInc,++pPtr)
					*pPtr=*vPtr;
				}
			
			vPtr=pilePtr;
			pPtr=pileBuffer;
			
			if(lowpassFilter)
				{
				/* Run a lowpass filter over the pixel pile: */
				*vPtr=Value((int(pPtr[0])*3+int(pPtr[1])*2+int(pPtr[2])+3)/6);
				vPtr+=vInc;
				++pPtr;
				*vPtr=Value((int(pPtr[-1])*2+int(pPtr[0])*3+int(pPtr[1])*2+int(pPtr[2])+4)/8);
				vPtr+=vInc;
				++pPtr;
				for(int z=4;z<sd.numVertices[2];++z,vPtr+=vInc,++pPtr)
					*vPtr=Value((int(pPtr[-2])+int(pPtr[-1])*2+int(pPtr[0])*3+int(pPtr[1])*2+int(pPtr[2])+4)/9);
				*vPtr=Value((int(pPtr[-2])+int(pPtr[-1])*2+int(pPtr[0])*3+int(pPtr[1])*2+4)/8);
				vPtr+=vInc;
				++pPtr;
				*vPtr=Value((int(pPtr[-2])+int(pPtr[-1])*2+int(pPtr[0])*3+3)/6);
				}
			else
				{
				/* Copy the pile buffer back into the voxel pile: */
				for(int z=0;z<sd.numVertices[2];++z,vPtr+=vInc,++pPtr)
					*vPtr=*pPtr;
				}
			}
		if(sd.master)
			std::cout<<"\b\b\b\b"<<std::setw(3)<<((x+1)*100)/sd.numVertices[0]<<"%"<<std::flush;
		}
	
	delete[] pileBuffer;
	
	filterTimer.elapse();
	if(sd.master)
		std::cout<<"\b\b\b\bdone in "<<filterTimer.getTime()*1000.0<<" ms"<<std::endl;
	}

}

/***************************************
Methods of class MultiChannelImageStack:
***************************************/

MultiChannelImageStack::MultiChannelImageStack(void)
	:BaseModule("MultiChannelImageStack")
	{
	}

Visualization::Abstract::DataSet* MultiChannelImageStack::load(const std::vector<std::string>& args,Cluster::MulticastPipe* pipe) const
	{
	bool master=pipe==0||pipe->isMaster();
	
	/* Create the result data set: */
	Misc::SelfDestructPointer<DataSet> result(new DataSet);
	DS& dataSet=result->getDs();
	
	/* Initialize the result data set's data value: */
	DataValue& dataValue=result->getDataValue();
	dataValue.initialize(&dataSet,0);
	
	/* Parse the module arguments: */
	StackDescriptor sd(dataSet,getBaseDirectory(),master);
	bool medianFilter=false;
	bool lowpassFilter=false;
	for(size_t i=0;i<args.size();++i)
		{
		bool argComplete=true;
		if(strcasecmp(args[i].c_str(),"-imageSize")==0)
			{
			if((argComplete=(i+2<args.size())))
				{
				for(int j=0;j<2;++j)
					sd.numVertices[j]=atoi(args[++i].c_str());
				sd.update(0x1);
				}
			}
		else if(strcasecmp(args[i].c_str(),"-numImages")==0)
			{
			if((argComplete=(i+1<args.size())))
				{
				sd.numVertices[2]=atoi(args[++i].c_str());
				sd.update(0x2);
				}
			}
		else if(strcasecmp(args[i].c_str(),"-sampleSpacing")==0)
			{
			if((argComplete=(i+3<args.size())))
				{
				for(int j=0;j<3;++j)
					sd.cellSize[j]=DS::Scalar(atof(args[++i].c_str()));
				sd.update(0x4);
				}
			}
		else if(strcasecmp(args[i].c_str(),"-regionOrigin")==0)
			{
			if((argComplete=(i+2<args.size())))
				{
				for(int j=0;j<2;++j)
					sd.regionOrigin[j]=atoi(args[++i].c_str());
				}
			}
		else if(strcasecmp(args[i].c_str(),"-imageDirectory")==0)
			{
			if((argComplete=(i+1<args.size())))
				sd.imageDirectory=getBaseDirectory()->openDirectory(args[++i].c_str());
			}
		else if(strcasecmp(args[i].c_str(),"-imageIndexStart")==0)
			{
			if((argComplete=(i+1<args.size())))
				sd.imageIndexStart=atoi(args[++i].c_str());
			}
		else if(strcasecmp(args[i].c_str(),"-imageIndexStep")==0)
			{
			if((argComplete=(i+1<args.size())))
				sd.imageIndexStep=atoi(args[++i].c_str());
			}
		else if(strcasecmp(args[i].c_str(),"-median")==0)
			medianFilter=true;
		else if(strcasecmp(args[i].c_str(),"-lowpass")==0)
			lowpassFilter=true;
		else if(strcasecmp(args[i].c_str(),"-greyscale")==0)
			{
			if((argComplete=(i+2<args.size())))
				{
				/* Check if the data set is completely defined: */
				if(!sd.haveDs)
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"-greyscale argument before dataset definition");
				
				/* Check if the slice file name template is valid: */
				if(!Misc::isValidTemplate(args[i+2],'d',1024))
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid slice file name template \"%s\"",args[i+2].c_str());
				
				/* Add a new slice to the data set: */
				int newSliceIndex=dataSet.addSlice();
				
				/* Add a new scalar variable to the data value: */
				dataValue.addScalarVariable(args[++i].c_str());
				
				/* Load the greyscale image stack: */
				loadGreyscaleImageStack(sd,newSliceIndex,args[++i].c_str());
				
				/* Filter the image stack if requested: */
				if(medianFilter||lowpassFilter)
					filterImageStack(sd,newSliceIndex,medianFilter,lowpassFilter);
				medianFilter=false;
				lowpassFilter=false;
				}
			}
		else if(strcasecmp(args[i].c_str(),"-color")==0)
			{
			if((argComplete=(i+4<args.size())))
				{
				/* Check if the data set is completely defined: */
				if(!sd.haveDs)
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"-color argument before dataset definition");
				
				/* Check if the slice file name template is valid: */
				if(!Misc::isValidTemplate(args[i+4],'d',1024))
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid slice file name template \"%s\"",args[i+4].c_str());
				
				/* Add three new slices to the data set: */
				int newSliceIndices[3];
				for(int j=0;j<3;++j)
					newSliceIndices[j]=dataSet.addSlice();
				
				/* Add three new scalar variables to the data value: */
				for(int j=0;j<3;++j)
					dataValue.addScalarVariable(args[++i].c_str());
				
				/* Load the color image stack: */
				loadColorImageStack(sd,newSliceIndices,args[++i].c_str());
				
				/* Filter the image stack if requested: */
				if(medianFilter||lowpassFilter)
					for(int j=0;j<3;++j)
						filterImageStack(sd,newSliceIndices[j],medianFilter,lowpassFilter);
				medianFilter=false;
				lowpassFilter=false;
				}
			}
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unrecognized argument %s",args[i]);
		
		if(!argComplete)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Incomplete argument %s",args[i]);
		}
	
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
	Visualization::Concrete::MultiChannelImageStack* module=new Visualization::Concrete::MultiChannelImageStack();
	
	/* Return module object: */
	return module;
	}

extern "C" void destroyFactory(Visualization::Abstract::Module* module)
	{
	delete module;
	}
