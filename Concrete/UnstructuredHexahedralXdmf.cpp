/***********************************************************************
UnstructuredHexahedralXdmf - Class reading unstructured hexahedral data
sets from files in Xdmf format, with mass data stored in HDF5 format.
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

#include <Concrete/UnstructuredHexahedralXdmf.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/StdError.h>
#include <Misc/HashTable.h>
#include <Plugins/FactoryManager.h>

#include <Concrete/HDF5Support.h>

namespace Misc {

/****************************
Hash functions for 3D points:
****************************/

template <>
class StandardHashFunction<Geometry::Point<Misc::Float32,3> >
	{
	/* Methods: */
	public:
	static size_t hash(const Geometry::Point<Misc::Float32,3>& source,size_t tableSize)
		{
		/* Re-interpret floating-point coordinates as unsigned integers: */
		const Misc::UInt32* comps=reinterpret_cast<const Misc::UInt32*>(source.getComponents());
		size_t result=size_t(comps[0])*71+size_t(comps[1])*37+size_t(comps[2])*19;
		return result%tableSize;
		}
	};

template <>
class StandardHashFunction<Geometry::Point<Misc::Float64,3> >
	{
	/* Methods: */
	public:
	static size_t hash(const Geometry::Point<Misc::Float64,3>& source,size_t tableSize)
		{
		/* Re-interpret floating-point coordinates as unsigned integers: */
		const Misc::UInt64* comps=reinterpret_cast<const Misc::UInt64*>(source.getComponents());
		size_t result=size_t(comps[0])*71+size_t(comps[1])*37+size_t(comps[2])*19;
		return result%tableSize;
		}
	};

}

namespace Visualization {

namespace Concrete {

/*******************************************
Methods of class UnstructuredHexahedralXdmf:
*******************************************/

UnstructuredHexahedralXdmf::UnstructuredHexahedralXdmf(void)
	:BaseModule("UnstructuredHexahedralXdmf")
	{
	}

Visualization::Abstract::DataSet* UnstructuredHexahedralXdmf::load(const std::vector<std::string>& args,Cluster::MulticastPipe* pipe) const
	{
	bool master=pipe==0||pipe->isMaster();
	
	/* Create the result data set: */
	Misc::SelfDestructPointer<DataSet> result(new DataSet);
	DS& dataSet=result->getDs();
	
	/* Get the input file name's directory: */
	std::string::const_iterator fnIt=args[0].begin();
	for(std::string::const_iterator pIt=args[0].begin();pIt!=args[0].end();++pIt)
		if(*pIt=='/')
			fnIt=pIt+1;
	std::string baseDir(args[0].begin(),fnIt);
	
	/* This is where we would parse the main Xdmf file. For now, hard-code stuff: */
	std::string meshFileName=baseDir+"solution/mesh-00000.h5";
	std::string nodesDsName="nodes";
	std::string cellsDsName="cells";
	std::string solutionFileName=baseDir+"solution/solution-00000.h5";
	size_t numScalarVariables=5;
	std::string scalarVariableDsNames[]=
		{
		"C_1","T","p","strain_rate","viscosity"
		};
	size_t numVectorVariables=1;
	std::string vectorVariableDsNames[]=
		{
		"velocity"
		};
	
	/* Create an array to map vertex indices from the file to non-duplicate vertex indices in the data set: */
	size_t numVertexIndices=0;
	Misc::SelfDestructArray<DS::VertexIndex> vertexIndices;
	
	// DEBUGGING
	Misc::SelfDestructArray<size_t> svi;
	
	{
	/* Open the mesh file: */
	HDF5::File meshFile(meshFileName.c_str());
	
	/* Read the mesh vertex data: */
	size_t nodeNumDims;
	size_t* nodeDims;
	Scalar* nodeData;
	HDF5::DataSet nodes(meshFile,nodesDsName.c_str());
	nodes.read(nodeNumDims,nodeDims,nodeData);
	
	/* Check for data consistency: */
	if(nodeNumDims!=2||nodeDims[1]!=3)
		{
		delete[] nodeDims;
		delete[] nodeData;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mesh vertices in input file %s have wrong layout",args[0].c_str());
		}
	
	/* Print suspicious points: */
	std::cout<<nodeData[33021*3+0]<<", "<<nodeData[33021*3+1]<<", "<<nodeData[33021*3+2]<<std::endl;
	std::cout<<nodeData[31724*3+0]<<", "<<nodeData[31724*3+1]<<", "<<nodeData[31724*3+2]<<std::endl;
	
	/* Use a hash table of point coordinates to find duplicate points and assign them the same indices: */
	{
	numVertexIndices=nodeDims[0];
	vertexIndices.setTarget(new DS::VertexIndex[numVertexIndices]);
	Misc::HashTable<DS::Point,DS::VertexIndex> vertexHash(numVertexIndices);
	
	// DEBUGGING
	svi.setTarget(new size_t[numVertexIndices]);
	Misc::HashTable<DS::Point,size_t> svim(numVertexIndices);
	
	/* Copy mesh vertex data into the result data set: */
	Scalar* ndPtr=nodeData;
	for(size_t vertexIndex=0;vertexIndex<nodeDims[0];++vertexIndex,ndPtr+=3)
		{
		/* Create the vertex: */
		DS::Point vertex(ndPtr);
		
		/* Look for an existing vertex with the same position in the hash table: */
		Misc::HashTable<DS::Point,DS::VertexIndex>::Iterator vhIt=vertexHash.findEntry(vertex);
		if(vhIt.isFinished())
			{
			/* Add the vertex to the data set and enter it into the hash table: */
			DS::VertexIndex vi=dataSet.addVertex(vertex).getIndex();
			vertexIndices[vertexIndex]=vi;
			vertexHash[vertex]=vi;
			}
		else
			{
			/* Map the index of the existing vertex to the new one: */
			vertexIndices[vertexIndex]=vhIt->getDest();
			}
		
		// DEBUGGING
		Misc::HashTable<DS::Point,size_t>::Iterator svimIt=svim.findEntry(vertex);
		if(svimIt.isFinished())
			{
			/* Enter the vertex into the hash table: */
			svi[vertexIndex]=vertexIndex;
			svim[vertex]=vertexIndex;
			}
		else
			{
			/* Map the index of the existing vertex to the new one: */
			svi[vertexIndex]=svimIt->getDest();
			}
		}
	}
	
	/* Clean up mesh vertex data: */
	delete[] nodeDims;
	delete[] nodeData;
	
	/* Read mesh topology data: */
	size_t cellNumDims;
	size_t* cellDims;
	unsigned int* cellData;
	HDF5::DataSet cells(meshFile,cellsDsName.c_str());
	cells.read(cellNumDims,cellDims,cellData);
	
	/* Check for data consistency: */
	if(cellNumDims!=2||cellDims[1]!=8)
		{
		delete[] cellDims;
		delete[] cellData;
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Mesh cell indices in input file %s have wrong layout",args[0].c_str());
		}
	
	/* Create the result data set's cell topology: */
	dataSet.reserveCells(cellDims[0]);
	static const int vertexOrder[8]={0,1,3,2,4,5,7,6}; // Xdmf's cube vertex counting order
	unsigned int* cdPtr=cellData;
	for(size_t cellIndex=0;cellIndex<cellDims[0];++cellIndex,cdPtr+=8)
		{
		/* Unswizzle the cell's vertex indices and assign non-duplicate vertex IDs: */
		DS::VertexID cellVertices[8];
		for(int i=0;i<8;++i)
			cellVertices[vertexOrder[i]]=vertexIndices[cdPtr[i]];
		
		/* Add the cell to the data set: */
		dataSet.addCell(cellVertices);
		}
	
	/* Finalize the grid structure: */
	if(master)
		std::cout<<"Finalizing grid structure..."<<std::flush;
	dataSet.finalizeGrid();
	if(master)
		std::cout<<" done"<<std::endl;
	}
	
	{
	/* Open the solution file: */
	HDF5::File solutionFile(getFullPath(solutionFileName).c_str());
	
	/* Initialize the result data set's data value: */
	DataValue& dataValue=result->getDataValue();
	dataValue.initialize(&dataSet,0);
	
	/* Read all scalar variables: */
	for(size_t variableIndex=0;variableIndex<numScalarVariables;++variableIndex)
		{
		std::cout<<"Reading scalar variable "<<scalarVariableDsNames[variableIndex]<<"..."<<std::flush;
		
		/* Add another scalar variable to the data value: */
		dataValue.addScalarVariable(scalarVariableDsNames[variableIndex].c_str());
		
		/* Add another slice to the data set: */
		int sliceIndex=dataSet.getNumSlices();
		dataSet.addSlice();
		
		/* Read the scalar variable's data: */
		size_t varNumDims;
		size_t* varDims;
		VScalar* varData;
		HDF5::DataSet vars(solutionFile,scalarVariableDsNames[variableIndex].c_str());
		vars.read(varNumDims,varDims,varData);
		
		/* Check for data consistency: */
		if(varNumDims!=2||varDims[1]!=1||varDims[0]!=numVertexIndices)
			{
			delete[] varDims;
			delete[] varData;
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Scalar variable %s in input file %s has wrong layout",scalarVariableDsNames[variableIndex].c_str(),args[0].c_str());
			}
		
		/* Copy scalar variable data into the data set using non-duplicate vertex IDs: */
		VScalar maxMismatch(0);
		size_t mmi0,mmi1;
		VScalar* vdPtr=varData;
		VScalar* slice=dataSet.getSliceArray(sliceIndex);
		for(size_t vertexIndex=0;vertexIndex<varDims[0];++vertexIndex,++vdPtr)
			{
			/* Get the vertex's non-duplicate ID: */
			DS::VertexIndex vi=vertexIndices[vertexIndex];
			
			// DEBUGGING
			// if(varData[vertexIndex]!=varData[svi[vertexIndex]])
				{
				VScalar mismatch=Math::abs(varData[vertexIndex]-varData[svi[vertexIndex]]);
				if(maxMismatch<mismatch)
					{
					maxMismatch=mismatch;
					mmi0=vertexIndex;
					mmi1=svi[vertexIndex];
					}
				}
			
			/* Assign the scalar value: */
			slice[vi]=*vdPtr;
			}
		
		std::cout<<" done"<<std::endl;
		
		if(maxMismatch>VScalar(0))
			{
			std::cout<<"\tMaximum shared node mismatch: "<<maxMismatch<<" between nodes "<<mmi0<<" and "<<mmi1<<std::endl;
			std::cout<<"\t"<<varData[mmi0]<<", "<<varData[mmi1]<<std::endl;
			}
		}
	
	/* Read all vector variables: */
	for(size_t variableIndex=0;variableIndex<numVectorVariables;++variableIndex)
		{
		std::cout<<"Reading vector variable "<<vectorVariableDsNames[variableIndex]<<"..."<<std::flush;
		
		/* Add another vector variable to the data value: */
		int vectorVariableIndex=dataValue.addVectorVariable(vectorVariableDsNames[variableIndex].c_str());
		
		/* Add four new slices to the data set (three components plus magnitude): */
		int sliceIndex=dataSet.getNumSlices();
		for(int i=0;i<4;++i)
			{
			dataSet.addSlice();
			dataValue.addScalarVariable(makeVectorSliceName(vectorVariableDsNames[variableIndex],i).c_str());
			if(i<3)
				dataValue.setVectorVariableScalarIndex(vectorVariableIndex,i,sliceIndex+i);
			}
		
		/* Read the vector variable's data: */
		size_t varNumDims;
		size_t* varDims;
		VScalar* varData;
		HDF5::DataSet vars(solutionFile,vectorVariableDsNames[variableIndex].c_str());
		vars.read(varNumDims,varDims,varData);
		
		/* Check for data consistency: */
		if(varNumDims!=2||varDims[1]!=3||varDims[0]!=numVertexIndices)
			{
			delete[] varDims;
			delete[] varData;
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Vector variable %s in input file %s has wrong layout",vectorVariableDsNames[variableIndex].c_str(),args[0].c_str());
			}
		
		/* Copy vector variable data into the data set using non-duplicate vertex IDs: */
		VScalar* vdPtr=varData;
		VScalar* slices[4];
		for(int i=0;i<4;++i)
			slices[i]=dataSet.getSliceArray(sliceIndex+i);
		for(size_t vertexIndex=0;vertexIndex<varDims[0];++vertexIndex,vdPtr+=3)
			{
			/* Get the vertex's non-duplicate ID: */
			DS::VertexIndex vi=vertexIndices[vertexIndex];
			
			/* Store the vector's components and magnitude: */
			bool mismatch=false;
			VScalar len2(0);
			for(int i=0;i<3;++i)
				{
				/* Check for mismatching vector component values: */
				mismatch=mismatch||slices[i][vi]!=vdPtr[i];
				
				/* Assign the vector component value: */
				slices[i][vi]=vdPtr[i];
				
				len2+=Math::sqr(vdPtr[i]);
				}
			
			/* Store the vector magnitude: */
			slices[3][vi]=Math::sqrt(len2);
			
			#if 0
			if(mismatch)
				std::cout<<'X'<<std::flush;
			#endif
			}
		
		std::cout<<" done"<<std::endl;
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
	Visualization::Concrete::UnstructuredHexahedralXdmf* module=new Visualization::Concrete::UnstructuredHexahedralXdmf();
	
	/* Return module object: */
	return module;
	}

extern "C" void destroyFactory(Visualization::Abstract::Module* module)
	{
	delete module;
	}
