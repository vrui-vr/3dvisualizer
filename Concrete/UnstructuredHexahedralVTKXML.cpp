/***********************************************************************
UnstructuredHexahedralVTKXML - Class reading unstructured hexahedral data
sets from files in VTK XML format.
Copyright (c) 2019 Oliver Kreylos

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

#include <Concrete/UnstructuredHexahedralVTKXML.h>

#include <iostream>
#include <fstream>
#include <Misc/SelfDestructPointer.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <Plugins/FactoryManager.h>
#include <Math/Math.h>

#include <Concrete/VTKFile.h>

namespace Visualization {

namespace Concrete {

/*********************************************
Methods of class UnstructuredHexahedralVTKXML:
*********************************************/

UnstructuredHexahedralVTKXML::UnstructuredHexahedralVTKXML(void)
	:BaseModule("UnstructuredHexahedralVTKXML")
	{
	}

Visualization::Abstract::DataSet* UnstructuredHexahedralVTKXML::load(const std::vector<std::string>& args,Cluster::MulticastPipe* pipe) const
	{
	bool master=pipe==0||pipe->isMaster();
	
	/* Parse the command line: */
	std::vector<std::string>::const_iterator fileName=args.end();
	Misc::HashTable<std::string,void> logScalars(17);
	bool savePoints=false;
	std::string savePointsScalar;
	int savePointsSliceIndex=-1;
	VScalar savePointsMin(0);
	std::string savePointsFileName;
	for(std::vector<std::string>::const_iterator argIt=args.begin();argIt!=args.end();++argIt)
		{
		if((*argIt)[0]=='-')
			{
			if(*argIt=="-log")
				{
				if(argIt+1!=args.end())
					{
					/* Add the name of a log-scale scalar variable: */
					++argIt;
					logScalars.setEntry(Misc::HashTable<std::string,void>::Entry(*argIt));
					}
				else
					std::cout<<"Warning: Ignoring dangling -log command line parameter"<<std::endl;
				}
			else if(*argIt=="-savePoints")
				{
				if(argIt+1!=args.end()&&argIt+2!=args.end()&&argIt+3!=args.end())
					{
					savePoints=true;
					++argIt;
					savePointsScalar=*argIt;
					++argIt;
					savePointsMin=VScalar(atof(argIt->c_str()));
					++argIt;
					savePointsFileName=*argIt;
					}
				else
					std::cout<<"Warning: Ignoring dangling -savePoints command line parameter"<<std::endl;
				}
			}
		else if(fileName==args.end())
			fileName=argIt;
		else
			std::cout<<"Warning: Ignoring command line argument "<<*argIt<<std::endl;
		}
	if(fileName==args.end())
		throw std::runtime_error("No VTK input file name provided");
	
	/* Read the source VTK file: */
	if(master)
		std::cout<<"Reading VTK file "<<args[0]<<"..."<<std::flush;
	VTKFile vtkFile;
	vtkFile.read(args[0].c_str());
	if(master)
		std::cout<<" done"<<std::endl;
	
	/* Print a warning if the VTK file defines per-cell properties: */
	if(master&&vtkFile.getNumCellProperties()!=0)
		std::cout<<"Warning: VTK file "<<args[0]<<" defines per-cell properties, which will be ignored"<<std::endl;
	
	/* Create the result data set: */
	Misc::SelfDestructPointer<DataSet> result(new DataSet);
	DS& dataSet=result->getDs();
	
	/* Copy all read vertices into the data set: */
	VTKFile::Index numVertices=vtkFile.getNumVertices();
	if(master)
		std::cout<<"Adding "<<numVertices<<" vertices..."<<std::flush;
	dataSet.reserveVertices(numVertices);
	const VTKFile::Point* vPtr=vtkFile.getVertices();
	for(VTKFile::Index vi=0;vi<numVertices;++vi,++vPtr)
		dataSet.addVertex(DS::Point(*vPtr));
	if(master)
		std::cout<<" done"<<std::endl;
	
	/* Copy all read cells into the data set: */
	VTKFile::Index numCells=vtkFile.getNumCells();
	if(master)
		std::cout<<"Adding "<<numCells<<" cells..."<<std::flush;
	dataSet.reserveCells(numCells);
	static const int vertexOrder[8]={0,1,3,2,4,5,7,6}; // VTK's cube vertex counting order
	const VTKFile::CellType* ctPtr=vtkFile.getCellTypes();
	const VTKFile::Index* cviPtr=vtkFile.getCellVertexIndices();
	for(VTKFile::Index ci=0;ci<numCells;++ci,++ctPtr)
		{
		/* Unswizzle the cell's vertex indices: */
		DS::VertexID cellVertices[8];
		for(int i=0;i<8;++i)
			cellVertices[vertexOrder[i]]=cviPtr[i];
		dataSet.addCell(cellVertices);
		
		/* Go to the next cell: */
		if(*ctPtr==12U) // Linear hexahedron
			cviPtr+=8;
		else // Tri-quadratic hexahedron
			cviPtr+=27;
		}
	if(master)
		std::cout<<" done"<<std::endl;
	
	/* Finalize the grid structure: */
	if(master)
		std::cout<<"Finalizing grid structure..."<<std::flush;
	dataSet.finalizeGrid();
	if(master)
		std::cout<<" done"<<std::endl;
	
	/* Initialize the result data set's data value: */
	DataValue& dataValue=result->getDataValue();
	dataValue.initialize(&dataSet,0);
	
	/* Copy all read vertex properties: */
	VTKFile::Index numVertexProperties=vtkFile.getNumVertexProperties();
	for(VTKFile::Index vpIndex=0;vpIndex<numVertexProperties;++vpIndex)
		{
		/* Access the vertex property: */
		const VTKFile::Property& vp=vtkFile.getVertexProperty(vpIndex);
		
		/* Determine if the property is a scalar or a vector property: */
		if(vp.numComponents==1)
			{
			/* Check if the scalar property is log-scaled: */
			bool log=logScalars.isEntry(vp.name);
			if(master)
				{
				if(log)
					std::cout<<"Adding logarithmic scalar variable "<<vp.name<<"..."<<std::flush;
				else
					std::cout<<"Adding scalar variable "<<vp.name<<"..."<<std::flush;
				}
			
			/* Add another scalar variable to the data value: */
			if(log)
				dataValue.addScalarVariable((std::string("log(")+vp.name+std::string(")")).c_str());
			else
				dataValue.addScalarVariable(vp.name.c_str());
			
			/* Add another slice to the data set: */
			int sliceIndex=dataSet.getNumSlices();
			dataSet.addSlice();
			if(savePoints&&vp.name==savePointsScalar)
				savePointsSliceIndex=sliceIndex;
			
			/* Copy the scalar vertex property's values: */
			VScalar* dSlicePtr=dataSet.getSliceArray(sliceIndex);
			const VTKFile::VScalar* sSlicePtr=vtkFile.getVertexPropertySlice(vpIndex,0);
			if(log)
				{
				for(VTKFile::Index vi=0;vi<numVertices;++vi,++dSlicePtr,++sSlicePtr)
					*dSlicePtr=Math::log10(VScalar(*sSlicePtr));
				}
			else
				{
				for(VTKFile::Index vi=0;vi<numVertices;++vi,++dSlicePtr,++sSlicePtr)
					*dSlicePtr=VScalar(*sSlicePtr);
				}
			
			if(master)
				std::cout<<" done"<<std::endl;
			}
		else if(vp.numComponents==3)
			{
			if(master)
				std::cout<<"Adding vector variable "<<vp.name<<"..."<<std::flush;
			
			/* Add another vector variable to the data value: */
			int vectorVariableIndex=dataValue.addVectorVariable(vp.name.c_str());
			
			/* Add four new slices to the data set (three components plus magnitude): */
			int sliceIndex=dataSet.getNumSlices();
			for(int i=0;i<4;++i)
				{
				dataSet.addSlice();
				dataValue.addScalarVariable(makeVectorSliceName(vp.name,i).c_str());
				if(i<3)
					dataValue.setVectorVariableScalarIndex(vectorVariableIndex,i,sliceIndex+i);
				}
			
			/* Copy the vector vertex property's values: */
			const VTKFile::VScalar* sSlicePtr[3];
			for(int i=0;i<3;++i)
				sSlicePtr[i]=vtkFile.getVertexPropertySlice(vpIndex,i);
			VScalar* dSlicePtr[4];
			for(int i=0;i<4;++i)
				dSlicePtr[i]=dataSet.getSliceArray(sliceIndex+i);
			for(VTKFile::Index vi=0;vi<numVertices;++vi)
				{
				/* Copy the vector property value's components and calculate its magnitude: */
				VScalar mag2(0);
				for(int i=0;i<3;++i)
					{
					*(dSlicePtr[i])=*(sSlicePtr[i]);
					mag2+=Math::sqr(*(dSlicePtr[i]));
					++dSlicePtr[i];
					++sSlicePtr[i];
					}
				
				/* Store the vector property value's magnitude: */
				*(dSlicePtr[3])=Math::sqrt(mag2);
				++dSlicePtr[3];
				}
			
			if(master)
				std::cout<<" done"<<std::endl;
			}
		else
			{
			/* Print a warning: */
			if(master)
				std::cout<<"Warning: Ignoring "<<vp.numComponents<<"-component variable "<<vp.name<<std::endl;
			}
		}
	
	/* Save large-value points to a file if requested: */
	if(savePoints)
		{
		/* Create the output file: */
		std::ofstream pointsFile(savePointsFileName);
		
		/* Save the 3D positions of all vertices that have a data value larger than the requested minimum: */
		for(size_t vi=0;vi<dataSet.getTotalNumVertices();++vi)
			if(dataSet.getVertexValue(savePointsSliceIndex,vi)>=savePointsMin)
				{
				const DS::Point& p=dataSet.getVertexPosition(vi);
				pointsFile<<p[0]<<", "<<p[1]<<", "<<p[2]<<std::endl;
				}
		}
	
	/* Return the result data set: */
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
	Visualization::Concrete::UnstructuredHexahedralVTKXML* module=new Visualization::Concrete::UnstructuredHexahedralVTKXML();
	
	/* Return module object: */
	return module;
	}

extern "C" void destroyFactory(Visualization::Abstract::Module* module)
	{
	delete module;
	}
