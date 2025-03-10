/***********************************************************************
VTKFileReader - Class to read VTK files in XML format.
Copyright (c) 2019-2024 Oliver Kreylos

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

#include <Concrete/VTKFileReader.h>

#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <Threads/Thread.h>
#include <IO/File.h>
#include <IO/GzipFilter.h>

#include <Concrete/VTKCDataParser.h>
#include <Concrete/VertexClusterer.h>

namespace Visualization {

namespace Concrete {

/***************************************
Embedded classes of class VTKFileReader:
***************************************/

struct VTKFileReader::ComponentType
	{
	/* Elements: */
	public:
	bool integerType; // Flag if the type is an integer type
	bool unsignedType; // Flag if the type is an unsigned integer type
	Index typeSize; // Storage size of the type in bytes
	};

struct VTKFileReader::DataArray
	{
	/* Elements: */
	public:
	std::string name; // Name of the data array
	ComponentType componentType; // Data type of the data array's components in the VTK file
	Index numValues; // Number of data values in the data array; 0 if left unspecified
	Index numComponents; // Number of components of each data value
	std::string format; // Format of data array's storage: ascii, binary, or appended
	};

/******************************
Methods of class VTKFileReader:
******************************/

void VTKFileReader::enterElement(const char* methodName,const char* elementName)
	{
	/* Skip the element's remaining attributes: */
	while(vtk.isAttributeName())
		{
		/* Skip the attribute name and value: */
		vtk.skip();
		vtk.skip();
		}
	
	/* Bail out if the element is empty, i.e., the opening tag was a self-closing tag: */
	if(vtk.wasSelfClosingTag())
		throw Misc::makeStdErr(methodName,"Empty %s element",elementName);
	}

void VTKFileReader::leaveElement(const char* methodName,const char* elementName)
	{
	if(vtk.eof())
		throw Misc::makeStdErr(methodName,"Unterminated %s element",elementName);
	std::string tagName;
	if(vtk.readUTF8(tagName)!=elementName)
		throw Misc::makeStdErr(methodName,"Mismatching closing tag %s in %s element",tagName.c_str(),elementName);
	}

VTKFileReader::ComponentType VTKFileReader::parseComponentType(const std::string& typeName)
	{
	ComponentType result;
	result.integerType=false;
	result.unsignedType=false;
	
	/* Extract the type name prefix: */
	std::string::const_iterator tnIt=typeName.begin();
	while(tnIt!=typeName.end()&&((*tnIt>='A'&&*tnIt<='Z')||(*tnIt>='a'&&*tnIt<='z')))
		++tnIt;
	size_t prefixLen=tnIt-typeName.begin();
	
	/* Extract the type name size: */
	Index size(0);
	while(tnIt!=typeName.end()&&*tnIt>='0'&&*tnIt<='9')
		{
		size=size*10+(*tnIt-'0');
		++tnIt;
		}
	if(size!=8&&size!=16&&size!=32&&size!=64)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid type size %u",size);
	
	/* Parse the type name: */
	result.typeSize=size/8;
	if(prefixLen==3&&typeName.compare(0,3,"Int")==0)
		{
		result.integerType=true;
		}
	else if(prefixLen==4&&typeName.compare(0,4,"UInt")==0)
		{
		result.integerType=true;
		result.unsignedType=true;
		}
	else if(prefixLen==5&&typeName.compare(0,5,"Float")==0)
		{
		result.integerType=false;
		if(size<32)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid floating-point type size %u",size);
		}
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid type name %s",typeName.c_str());
	
	return result;
	}

VTKFileReader::DataArray VTKFileReader::parseDataArrayHeader(void)
	{
	/* Process the current DataArray element's attributes: */
	DataArray result;
	result.numValues=0;
	result.numComponents=1;
	while(vtk.isAttributeName())
		{
		/* Read the attribute name: */
		std::string attributeName=vtk.readUTF8();
		
		/* Parse the attribute value: */
		if(attributeName=="Name")
			vtk.readUTF8(result.name);
		else if(attributeName=="type")
			result.componentType=parseComponentType(vtk.readUTF8());
		else if(attributeName=="NumberOfTuples")
			result.numValues=Index(strtoul(vtk.readUTF8().c_str(),0,10));
		else if(attributeName=="NumberOfComponents")
			result.numComponents=Index(strtoul(vtk.readUTF8().c_str(),0,10));
		else if(attributeName=="format")
			vtk.readUTF8(result.format);
		else
			vtk.skip();
		}
	
	return result;
	}

template <class FileScalarParam,class ArrayScalarParam>
void VTKFileReader::readBinaryDataArray(const VTKFileReader::DataArray& dataArray,IO::File& data,std::vector<ArrayScalarParam>& array)
	{
	/* Read all data array elements: */
	FileScalarParam components[9]; // Can be at most 9 for a tensor, not that we're using them
	while(!data.eof())
		{
		/* Read the element's components: */
		data.read(components,dataArray.numComponents);
		for(Index j=0;j<dataArray.numComponents;++j)
			array.push_back(ArrayScalarParam(components[j]));
		}
	}

template <class ArrayScalarParam>
void VTKFileReader::readDataArray(const VTKFileReader::DataArray& dataArray,std::vector<ArrayScalarParam>& array)
	{
	/* Check that there is character data inside the DataArray element: */
	if(vtk.wasSelfClosingTag()||!vtk.isCharacterData())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Empty DataArray element");
		
	/* Read the data array: */
	if(dataArray.format=="ascii")
		{
		/* Create an ASCII parser for the VTK file's current character data: */
		VTKCDataParser cdParser(vtk);
		
		/* Read the point data: */
		if(dataArray.componentType.integerType)
			{
			if(dataArray.componentType.unsignedType)
				{
				/* Read all elements as signed integers: */
				while(!cdParser.eocd())
					for(Index j=0;j<dataArray.numComponents;++j)
						array.push_back(ArrayScalarParam(cdParser.readInteger()));
				}
			else
				{
				/* Read all elements as unsigned integers: */
				while(!cdParser.eocd())
					for(Index j=0;j<dataArray.numComponents;++j)
						array.push_back(ArrayScalarParam(cdParser.readUnsignedInteger()));
				}
			}
		else
			{
			/* Read all elements as floating-point numbers: */
			while(!cdParser.eocd())
					for(Index j=0;j<dataArray.numComponents;++j)
					array.push_back(ArrayScalarParam(cdParser.readFloat()));
			}
		
		/* Skip the rest of the character data: */
		cdParser.finish();
		}
	else if(dataArray.format=="binary")
		{
		/* Skip initial whitespace in the character data: */
		vtk.skipWhitespace();
		
		/* Access the binary data block header through an IO::File interface: */
		IO::FilePtr data=vtk.readBase64();
		data->setEndianness(endianness);
		Index numBlocks;
		Index blockSizes;
		Index lastBlockSize;
		std::vector<Index> compressedBlockSizes;
		if(compressed)
			{
			/* Read a compressed block header: */
			numBlocks=headerIntSize==8?Index(data->read<Misc::UInt64>()):Index(data->read<Misc::UInt32>());
			blockSizes=headerIntSize==8?Index(data->read<Misc::UInt64>()):Index(data->read<Misc::UInt32>());
			lastBlockSize=headerIntSize==8?Index(data->read<Misc::UInt64>()):Index(data->read<Misc::UInt32>());
			compressedBlockSizes.reserve(numBlocks);
			for(Index i=0;i<numBlocks;++i)
				compressedBlockSizes.push_back(headerIntSize==8?Index(data->read<Misc::UInt64>()):Index(data->read<Misc::UInt32>()));
			}
		else
			{
			/* Read an uncompressed block header: */
			numBlocks=1;
			lastBlockSize=blockSizes=headerIntSize==8?Index(data->read<Misc::UInt64>()):Index(data->read<Misc::UInt32>());
			compressedBlockSizes.reserve(1);
			compressedBlockSizes.push_back(blockSizes);
			}
		
		/* Check for the block header separator: */
		if(vtk.readCharacterData()!='='||vtk.readCharacterData()!='=')
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid binary data block");
		
		/* Check if the block format is supported: */
		if(numBlocks!=1||lastBlockSize!=blockSizes)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Multi-block binary data not supported");
		
		/* Access the binary data block through an IO::File interface: */
		data=vtk.readBase64();
		if(compressed)
			data=new IO::GzipFilter(data);
		data->setEndianness(endianness);
		
		/* Read the binary data depending on its file type: */
		if(dataArray.componentType.integerType)
			{
			if(dataArray.componentType.unsignedType)
				{
				if(dataArray.componentType.typeSize==1)
					readBinaryDataArray<Misc::UInt8>(dataArray,*data,array);
				else if(dataArray.componentType.typeSize==2)
					readBinaryDataArray<Misc::UInt16>(dataArray,*data,array);
				else if(dataArray.componentType.typeSize==4)
					readBinaryDataArray<Misc::UInt32>(dataArray,*data,array);
				else
					readBinaryDataArray<Misc::UInt64>(dataArray,*data,array);
				}
			else
				{
				if(dataArray.componentType.typeSize==1)
					readBinaryDataArray<Misc::SInt8>(dataArray,*data,array);
				else if(dataArray.componentType.typeSize==2)
					readBinaryDataArray<Misc::SInt16>(dataArray,*data,array);
				else if(dataArray.componentType.typeSize==4)
					readBinaryDataArray<Misc::SInt32>(dataArray,*data,array);
				else
					readBinaryDataArray<Misc::SInt64>(dataArray,*data,array);
				}
			}
		else
			{
			if(dataArray.componentType.typeSize==4)
				readBinaryDataArray<Misc::Float32>(dataArray,*data,array);
			else
				readBinaryDataArray<Misc::Float64>(dataArray,*data,array);
			}
		
		/* Skip the rest of the character data: */
		vtk.skip();
		}
	else if(dataArray.format=="appended")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"\"appended\" data array format not supported");
	else
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid data array format %s",dataArray.format.c_str());
	
	/* Check that there is a closing tag for the DataArray element: */
	if(!vtk.isTagName()||vtk.isOpeningTag()||vtk.readUTF8()!="DataArray")
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unterminated DataArray element");
	}

void VTKFileReader::processPoints(VTKFileReader::Index numVertices)
	{
	enterElement("VTKFileReader::processPoints","Points");
	
	/* Process all elements contained in the Points element: */
	bool havePoints=false;
	while(vtk.skipToTag()&&vtk.isOpeningTag())
		{
		/* Read the tag name: */
		std::string tagName=vtk.readUTF8();
		if(!havePoints&&tagName=="DataArray")
			{
			/* Parse the data array's header: */
			DataArray dataArray=parseDataArrayHeader();
			if(dataArray.numComponents!=3)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid number of components %u in vertex positions",dataArray.numComponents);
			
			/* Read the vertex positions: */
			size_t vertexComponentsSize=vertexComponents.size();
			readDataArray(dataArray,vertexComponents);
			if(vertexComponents.size()-vertexComponentsSize!=numVertices*3)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Wrong number of vertices in DataArray element");
			havePoints=true;
			}
		else
			vtk.skipElement(tagName);
		}
	
	/* Check for errors: */
	if(!havePoints)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No DataArray element in Points element");
	leaveElement("VTKFileReader::processPoints","Points");
	}

void VTKFileReader::processCells(VTKFileReader::Index numCells)
	{
	enterElement("VTKFileReader::processCells","Cells");
	
	/* Process all elements contained in the Cells element: */
	bool haveConnectivity=false;
	bool haveTypes=false;
	while(vtk.skipToTag()&&vtk.isOpeningTag())
		{
		/* Read the tag name: */
		std::string tagName=vtk.readUTF8();
		if(tagName=="DataArray")
			{
			/* Parse the data array's header: */
			DataArray dataArray=parseDataArrayHeader();
			
			/* Determine which cell data array this is: */
			if(!haveConnectivity&&dataArray.name=="connectivity")
				{
				/* Read the cell vertex indices array: */
				#if 0
				size_t cellVertexIndicesSize=cellVertexIndices.size();
				#endif
				readDataArray(dataArray,cellVertexIndices);
				#if 0
				if(cellVertexIndices.size()-cellVertexIndicesSize!=numCells*8)
					{
					double connPerCell=double(cellVertexIndices.size()-cellVertexIndicesSize)/double(numCells);
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Wrong number %f of cell vertex indices in \"connectivity\" DataArray element",connPerCell);
					}
				#endif
				
				haveConnectivity=true;
				}
			else if(!haveTypes&&dataArray.name=="types")
				{
				/* Read the cell types array: */
				size_t cellTypesSize=cellTypes.size();
				readDataArray(dataArray,cellTypes);
				if(cellTypes.size()-cellTypesSize!=numCells)
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Wrong number of cells in \"types\" DataArray element");
				
				/* Check if all cells are linear or tri-quadratic hexahedra: */
				for(std::vector<CellType>::iterator tIt=cellTypes.begin()+cellTypesSize;tIt!=cellTypes.end();++tIt)
					if(*tIt!=12U&&*tIt!=72)
						throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Non-hexahedral cell type %u in grid",(unsigned int)*tIt);
				
				haveTypes=true;
				}
			else
				vtk.skipElement(tagName);
			}
		else
			vtk.skipElement(tagName);
		}
	
	/* Check for errors: */
	if(!haveConnectivity||!haveTypes)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing DataArray element(s) in Cells element");
	leaveElement("VTKFileReader::processCells","Cells");
	}

void VTKFileReader::processPointOrCellData(VTKFileReader::PropertyList& properties,VTKFileReader::Index numValues,const char* methodName,const char* elementName)
	{
	enterElement(methodName,elementName);
	
	/* Check if the properties list is already populated: */
	if(properties.empty())
		{
		/* Process all elements contained in the *Data element and create a new property for each one: */
		while(vtk.skipToTag()&&vtk.isOpeningTag())
			{
			/* Read the tag name: */
			std::string tagName=vtk.readUTF8();
			if(tagName=="DataArray")
				{
				/* Parse the data array's header: */
				DataArray dataArray=parseDataArrayHeader();
				
				/* Create a new property: */
				Misc::SelfDestructPointer<Property> newProperty(new Property);
				newProperty->name=dataArray.name;
				newProperty->numComponents=dataArray.numComponents;
				
				/* Read the data array's contents into the new property: */
				newProperty->components.reserve(numValues*dataArray.numComponents);
				readDataArray(dataArray,newProperty->components);
				if(newProperty->components.size()!=numValues*dataArray.numComponents)
					throw Misc::makeStdErr(methodName,"Wrong number of values in DataArray element for property %s",dataArray.name.c_str());
				
				/* Add the new property to the vertex/cell properties list: */
				properties.push_back(newProperty.releaseTarget());
				}
			else
				vtk.skipElement(tagName);
			}
		}
	else
		{
		/* Create a list of flags to track which properties have been read: */
		std::vector<bool> readProperties;
		for(PropertyList::iterator pIt=properties.begin();pIt!=properties.end();++pIt)
			readProperties.push_back(false);
		
		/* Process all elements contained in the *Data element and append data to an existing property for each one: */
		while(vtk.skipToTag()&&vtk.isOpeningTag())
			{
			/* Read the tag name: */
			std::string tagName=vtk.readUTF8();
			if(tagName=="DataArray")
				{
				/* Parse the data array's header: */
				DataArray dataArray=parseDataArrayHeader();
				
				/* Find a property of the data array's name in the vertex/cell properties list: */
				Property* property=0;
				std::vector<bool>::iterator rpIt=readProperties.begin();
				for(PropertyList::iterator pIt=properties.begin();pIt!=properties.end();++pIt,++rpIt)
					if(dataArray.name==(*pIt)->name)
						{
						property=*pIt;
						break;
						}
				if(property==0)
					throw Misc::makeStdErr(methodName,"Property %s not found in property list",dataArray.name.c_str());
				if(*rpIt)
					throw Misc::makeStdErr(methodName,"Multiple data arrays for property %s",dataArray.name.c_str());
				if(dataArray.numComponents!=property->numComponents)
					throw Misc::makeStdErr(methodName,"Property %s and data array have mismatching numbers of components",dataArray.name.c_str());
				
				/* Append the data array's contents to the found property: */
				size_t propertyComponentsSize=property->components.size();
				property->components.reserve(propertyComponentsSize+numValues*dataArray.numComponents);
				readDataArray(dataArray,property->components);
				if(property->components.size()-propertyComponentsSize!=numValues*dataArray.numComponents)
					throw Misc::makeStdErr(methodName,"Wrong number of values in DataArray element for property %s",dataArray.name.c_str());
				
				/* Mark the property as read: */
				*rpIt=true;
				}
			else
				vtk.skipElement(tagName);
			}
		
		/* Check if all properties have been read: */
		for(std::vector<bool>::iterator rpIt=readProperties.begin();rpIt!=readProperties.end();++rpIt)
			if(!*rpIt)
				throw Misc::makeStdErr(methodName,"Missing DataArray element(s) in %s element",elementName);
		}
	
	leaveElement(methodName,elementName);
	}

void VTKFileReader::processUnstructuredGrid(void)
	{
	enterElement("VTKFileReader::processUnstructuredGrid","UnstructuredGrid");
	
	/* Process all grid pieces: */
	while(vtk.skipToTag()&&vtk.isOpeningTag())
		{
		std::string tagName=vtk.readUTF8();
		if(tagName=="Piece")
			{
			/* Process the Piece element's attributes: */
			Index numVertices=0;
			Index numCells=0;
			while(vtk.isAttributeName())
				{
				/* Read the attribute name: */
				std::string attributeName=vtk.readUTF8();
				
				/* Parse the attribute: */
				if(attributeName=="NumberOfPoints")
					numVertices=Index(strtoul(vtk.readUTF8().c_str(),0,10));
				else if(attributeName=="NumberOfCells")
					numCells=Index(strtoul(vtk.readUTF8().c_str(),0,10));
				else
					vtk.skip();
				}
			
			enterElement("VTKFileReader::processUnstructuredGrid","Piece");
			
			/* Process all elements contained in the Piece element: */
			Index vertexBaseIndex=vertexComponents.size()/3;
			vertexComponents.reserve(vertexComponents.size()+numVertices*3);
			cellTypes.reserve(cellTypes.size()+numCells);
			Index cellBaseIndex=cellVertexIndices.size();
			cellVertexIndices.reserve(cellVertexIndices.size()+numCells*8); // Assuming that all cells are linear hexahedra for now
			bool havePoints=false;
			bool havePointData=false;
			bool haveCells=false;
			bool haveCellData=false;
			while(vtk.skipToTag()&&vtk.isOpeningTag())
				{
				/* Read the tag name: */
				std::string tagName=vtk.readUTF8();
				if(!havePoints&&tagName=="Points")
					{
					/* Process the Points element: */
					processPoints(numVertices);
					havePoints=true;
					}
				else if(!havePointData&&tagName=="PointData")
					{
					/* Process the PointData element: */
					processPointOrCellData(vertexProperties,numVertices,"VTKFileReader::processPointData","PointData");
					havePointData=true;
					}
				else if(!haveCells&&tagName=="Cells")
					{
					/* Process the Cells element: */
					processCells(numCells);
					haveCells=true;
					}
				else if(!haveCellData&&tagName=="CellData")
					{
					/* Process the CellData element: */
					processPointOrCellData(cellProperties,numCells,"VTKFileReader::processCellData","CellData");
					haveCellData=true;
					}
				else
					vtk.skipElement(tagName);
				}
			
			/* Check for error: */
			if(!havePoints||!haveCells)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No Points or Cells element in Piece element");
			if(!havePointData&&!haveCellData)
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No PointData or CellData elements in Piece element");
			leaveElement("VTKFileReader::processUnstructuredGrid","Piece");
			
			/* Offset the just-read cell vertex indices: */
			for(std::vector<Index>::iterator cviIt=cellVertexIndices.begin()+cellBaseIndex;cviIt!=cellVertexIndices.end();++cviIt)
				*cviIt+=vertexBaseIndex;
			}
		else
			vtk.skipElement(tagName);
		}
	
	leaveElement("VTKFileReader::processUnstructuredGrid","UnstructuredGrid");
	}

void VTKFileReader::processParallelPointOrCellData(VTKFileReader::PropertyList& properties,const char* methodName,const char* elementName)
	{
	enterElement(methodName,elementName);
	
	/* Process all elements contained in the P*Data element: */
	while(vtk.skipToTag()&&vtk.isOpeningTag())
		{
		/* Read the tag name: */
		std::string tagName=vtk.readUTF8();
		if(tagName=="PDataArray")
			{
			/* Parse the PDataArray element's header, which fortunately has the same format as a DataArray element's: */
			DataArray dataArray=parseDataArrayHeader();
			
			/* Add a vertex/cell property from the parsed data array: */
			Property* newProperty=new Property;
			newProperty->name=dataArray.name;
			newProperty->numComponents=dataArray.numComponents;
			properties.push_back(newProperty);
			
			/* Skip the rest of the PDataArray element: */
			vtk.skipElement(tagName);
			}
		else
			vtk.skipElement(tagName);
		}
	
	leaveElement(methodName,elementName);
	}

namespace {

/**************
Helper classes:
**************/

struct ThreadArgument
	{
	/* Elements: */
	public:
	VTKFileReader* reader; // A reader for a piece file
	VertexClusterer* clusterer; // A vertex clusterer to merge vertices read from the piece file
	VTKFileReader::Index numVertices; // Number of merged vertices
	Threads::Thread thread; // A thread running the reader function
	bool ok; // A flag if everything was A-OK
	};

void* readerThreadFunction(ThreadArgument* ta)
	{
	try
		{
		/* Read the piece file: */
		ta->reader->read();
		
		/* Merge close-by vertices to remove redundancy from the saved output and enable cell face matching: */
		ta->clusterer=new VertexClusterer(ta->reader->getVertexComponents());
		
		/* Calculate a default maximum distance for cluster merging based on domain size and the selected scalar data type's machine epsilon: */
		const VertexClusterer::Box& bbox=ta->clusterer->getBoundingBox();
		VertexClusterer::Scalar maxDim(0);
		for(int i=0;i<3;++i)
			maxDim=Math::max(maxDim,Math::max(Math::abs(bbox.min[i]),Math::abs(bbox.max[i])));
		VertexClusterer::Scalar maxDist=maxDim*Math::Constants<VertexClusterer::Scalar>::epsilon;
		
		/* Create clusters: */
		ta->numVertices=ta->clusterer->createClusters(maxDist);
		
		/* Signal success: */
		ta->ok=true;
		}
	catch(const std::runtime_error& err)
		{
		Misc::formattedUserError("VTKFileReader::readerThread: Caught exception %s",err.what());
		}
	
	return 0;
	}

}

void VTKFileReader::processParallelUnstructuredGrid(void)
	{
	enterElement("VTKFileReader::processParallelUnstructuredGrid","PUnstructuredGrid");
	
	/* Process all elements contained in the PUnstructuredGrid element: */
	bool havePPointData=false;
	bool havePCellData=false;
	std::vector<std::string> pieceUrls; // List of names of piece files
	while(vtk.skipToTag()&&vtk.isOpeningTag())
		{
		/* Read the tag name: */
		std::string tagName=vtk.readUTF8();
		if(tagName=="PPoints")
			{
			/* Skip it for now: */
			vtk.skipElement(tagName);
			}
		else if(!havePPointData&&tagName=="PPointData")
			{
			/* Parse the list of vertex properties expected in all piece files: */
			processParallelPointOrCellData(vertexProperties,"VTKFileReader::processParallelPointData","PPointData");
			havePPointData=true;
			}
		else if(!havePCellData&&tagName=="PCellData")
			{
			/* Parse the list of cell properties expected in all piece files: */
			processParallelPointOrCellData(cellProperties,"VTKFileReader::processParallelCellData","PCellData");
			havePCellData=true;
			}
		else if(tagName=="Piece")
			{
			/* Read the piece element's attributes: */
			while(vtk.isAttributeName())
				{
				std::string attributeName=vtk.readUTF8();
				if(attributeName=="Source")
					{
					/* Store the URL of the piece file for later: */
					pieceUrls.push_back(vtk.readUTF8());
					}
				else
					vtk.skip();
				}
			
			/* Skip the rest of the Piece element: */
			vtk.skipElement(tagName);
			}
		else
			vtk.skipElement(tagName);
		}
	
	leaveElement("VTKFileReader::processParallelUnstructuredGrid","PUnstructuredGrid");
	
	/* Prepare work assignments for the reader threads: */
	ThreadArgument* threadArgs=new ThreadArgument[pieceUrls.size()];
	ThreadArgument* taPtr=threadArgs;
	for(std::vector<std::string>::iterator puIt=pieceUrls.begin();puIt!=pieceUrls.end();++puIt,++taPtr)
		{
		/* Add a new reader: */
		taPtr->reader=new VTKFileReader(baseDirectory,puIt->c_str());
		
		/* Set the new reader's vertex and cell properties: */
		for(PropertyList::iterator vpIt=vertexProperties.begin();vpIt!=vertexProperties.end();++vpIt)
			taPtr->reader->addVertexProperty((*vpIt)->name,(*vpIt)->numComponents);
		for(PropertyList::iterator cpIt=cellProperties.begin();cpIt!=cellProperties.end();++cpIt)
			taPtr->reader->addCellProperty((*cpIt)->name,(*cpIt)->numComponents);
		
		taPtr->clusterer=0;
		taPtr->ok=false;
		}
	
	/* Read all piece files in parallel: */
	for(Index i=0;i<pieceUrls.size();++i)
		threadArgs[i].thread.start(readerThreadFunction,threadArgs+i);
	
	/* Wait for all reader threads to finish: */
	for(Index i=0;i<pieceUrls.size();++i)
		threadArgs[i].thread.join();
	
	/* Check if all threads did OK: */
	bool ok=true;
	for(Index i=0;i<pieceUrls.size();++i)
		ok=ok&&threadArgs[i].ok;
	if(ok)
		{
		/* Combine the merged vertices and cell vertex indices from all piece files into a single list: */
		Index totalNumVertices=0;
		Index totalNumCells=0;
		Index totalNumCellVertexIndices=0;
		for(Index i=0;i<pieceUrls.size();++i)
			{
			totalNumVertices+=threadArgs[i].numVertices;
			totalNumCells+=threadArgs[i].reader->getCellTypes().size();
			totalNumCellVertexIndices+=threadArgs[i].reader->getCellVertexIndices().size();
			}
		vertexComponents.reserve(totalNumVertices*3);
		cellTypes.reserve(totalNumCells);
		cellVertexIndices.reserve(totalNumCellVertexIndices);
		Index baseIndex=0;
		for(Index i=0;i<pieceUrls.size();++i)
			{
			/* Collect the piece's number of vertices and cells: */
			Index numVertices=threadArgs[i].numVertices;
			Index numCells=threadArgs[i].reader->getCellTypes().size();
			
			/* Collect merged vertices from the thread's clusterer: */
			threadArgs[i].clusterer->retrieveMergedVertices(vertexComponents);
			
			/* Collect all per-vertex data values for all merged vertices: */
			PropertyList::const_iterator ovpIt=threadArgs[i].reader->getVertexProperties().begin();
			for(PropertyList::iterator vpIt=vertexProperties.begin();vpIt!=vertexProperties.end();++vpIt,++ovpIt)
				{
				/* Retrieve the number of components of this vertex property: */
				Index numComponents=(*vpIt)->numComponents;
				
				/* Retrieve the vertex property's original and merged component lists: */
				std::vector<VScalar>& components=(*vpIt)->components;
				const std::vector<VScalar>& originalComponents=(*ovpIt)->components;
				
				/* Copy vertex properties for all merged vertices: */
				for(Index vi=0;vi<numVertices;++vi)
					{
					/* Get the index of one of the original vertices that were merged into this merged vertex: */
					Index vertexIndex=threadArgs[i].clusterer->getOriginalVertexIndex(vi);
					
					/* Get an iterator to the first vertex property component of the original vertex: */
					std::vector<VScalar>::const_iterator ocIt=originalComponents.begin()+(vertexIndex*numComponents);
					
					/* Copy this vertex's vertex property components: */
					for(Index i=0;i<numComponents;++i,++ocIt)
						components.push_back(*ocIt);
					}
				}
			
			/* Copy the piece's cell types: */
			cellTypes.insert(cellTypes.end(),threadArgs[i].reader->getCellTypes().begin(),threadArgs[i].reader->getCellTypes().end());
			
			/* Convert cell vertex indices into shared vertex space: */
			const std::vector<Index>& cvis=threadArgs[i].reader->getCellVertexIndices();
			for(std::vector<Index>::const_iterator cviIt=cvis.begin();cviIt!=cvis.end();++cviIt)
				cellVertexIndices.push_back(baseIndex+threadArgs[i].clusterer->getMergedVertexIndex(*cviIt));
			
			/* Collect all per-cell data values for all cells: */
			PropertyList::const_iterator ocpIt=threadArgs[i].reader->getCellProperties().begin();
			for(PropertyList::iterator cpIt=cellProperties.begin();cpIt!=cellProperties.end();++cpIt,++ocpIt)
				{
				/* Retrieve the number of components of this cell property: */
				Index numComponents=(*cpIt)->numComponents;
				
				/* Retrieve the cell property's original and merged component lists: */
				std::vector<VScalar>& components=(*cpIt)->components;
				std::vector<VScalar>& originalComponents=(*ocpIt)->components;
				
				/* Copy cell properties for all cells: */
				std::vector<VScalar>::const_iterator ocIt=originalComponents.begin();
				for(Index ci=0;ci<numCells;++ci)
					{
					/* Copy this vertex's vertex property components: */
					for(Index i=0;i<numComponents;++i,++ocIt)
						components.push_back(*ocIt);
					}
				}
			
			baseIndex+=threadArgs[i].numVertices;
			}
		}
		
	/* Delete the thread work assignments: */
	for(Index i=0;i<pieceUrls.size();++i)
		{
		delete threadArgs[i].reader;
		delete threadArgs[i].clusterer;
		}
	
	/* Check if there was an error reading any of the piece files: */
	if(!ok)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Error while reading piece files");
	}

VTKFileReader::VTKFileReader(IO::DirectoryPtr directory,const char* vtkFileName)
	:baseDirectory(directory->openFileDirectory(vtkFileName)),
	 vtk(directory->openFile(vtkFileName)),
	 compressed(false),endianness(Misc::HostEndianness),headerIntSize(4)
	{
	}

VTKFileReader::~VTKFileReader(void)
	{
	/* Delete all vertex and cell properties: */
	for(PropertyList::iterator vpIt=vertexProperties.begin();vpIt!=vertexProperties.end();++vpIt)
		delete *vpIt;
	for(PropertyList::iterator cpIt=cellProperties.begin();cpIt!=cellProperties.end();++cpIt)
		delete *cpIt;
	}

void VTKFileReader::addVertexProperty(const std::string& name,VTKFileReader::Index numComponents)
	{
	/* Add a new property to the vertex properties: */
	Property* newProperty=new Property;
	newProperty->name=name;
	newProperty->numComponents=numComponents;
	vertexProperties.push_back(newProperty);
	}

void VTKFileReader::addCellProperty(const std::string& name,VTKFileReader::Index numComponents)
	{
	/* Add a new property to the cell properties: */
	Property* newProperty=new Property;
	newProperty->name=name;
	newProperty->numComponents=numComponents;
	cellProperties.push_back(newProperty);
	}

void VTKFileReader::read(void)
	{
	/* Find the root VTKFile element: */
	if(!vtk.skipToElement("VTKFile"))
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No VTKFile element found");
	
	/* Read the root element's attributes: */
	while(vtk.isAttributeName())
		{
		/* Read the attribute name: */
		std::string attributeName=vtk.readUTF8();
		
		/* Parse the attribute: */
		if(attributeName=="type")
			gridType=vtk.readUTF8();
		else if(attributeName=="version")
			{
			std::string version=vtk.readUTF8();
			if(version!="0.1")
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported VTK file version %s",version.c_str());
			}
		else if(attributeName=="compressor")
			{
			std::string compressor=vtk.readUTF8();
			if(compressor=="vtkZLibDataCompressor")
				compressed=true;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported binary data compressor %s",compressor.c_str());
			}
		else if(attributeName=="byte_order")
			{
			std::string byteOrder=vtk.readUTF8();
			if(byteOrder=="LittleEndian")
				endianness=Misc::LittleEndian;
			else if(byteOrder=="BigEndian")
				endianness=Misc::BigEndian;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported binary data byte order %s",byteOrder.c_str());
			}
		else if(attributeName=="header-type")
			{
			std::string headerType=vtk.readUTF8();
			if(headerType=="UInt32")
				headerIntSize=4;
			else if(headerType=="UInt64")
				headerIntSize=8;
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported header type %s",headerType.c_str());
			}
		else
			vtk.skip();
		}
	
	enterElement("VTKFileReader::read","VTKFile");
	
	/* Process all elements contained in the VTKFile element: */
	bool haveGrid=false;
	while(vtk.skipToTag()&&vtk.isOpeningTag())
		{
		/* Read the tag name: */
		std::string tagName=vtk.readUTF8();
		if(!haveGrid&&tagName==gridType)
			{
			/* Process the grid element: */
			if(gridType=="UnstructuredGrid")
				processUnstructuredGrid();
			else if(gridType=="PUnstructuredGrid")
				processParallelUnstructuredGrid();
			else
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Unsupported grid type %s",gridType.c_str());
			haveGrid=true;
			}
		else
			vtk.skipElement(tagName);
		}
	
	/* Check for errors: */
	if(!haveGrid)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No %s element found",gridType.c_str());
	leaveElement("VTKFileReader::read","VTKFile");
	}

}

}
