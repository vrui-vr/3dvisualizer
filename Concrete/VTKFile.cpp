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

#include <Concrete/VTKFile.h>

#include <Math/Math.h>
#include <Math/Constants.h>

#include <Concrete/VTKFileReader.h>
#include <Concrete/VertexClusterer.h>

namespace Visualization {

namespace Concrete {

/************************
Methods of class VTKFile:
************************/

VTKFile::VTKFile(void)
	:numVertices(0),vertices(0),
	 numVertexProperties(0),vertexProperties(0),vertexPropertySlices(0),
	 numCells(0),cellTypes(0),cellVertexIndices(0),
	 numCellProperties(0),cellProperties(0),cellPropertySlices(0)
	{
	}

VTKFile::~VTKFile(void)
	{
	/* Delete all allocated arrays: */
	delete[] vertices;
	delete[] vertexProperties;
	delete[] vertexPropertySlices;
	delete[] cellTypes;
	delete[] cellVertexIndices;
	delete[] cellProperties;
	delete[] cellPropertySlices;
	}

void VTKFile::read(const char* vtkFileName)
	{
	/* Call the other method with the current directory: */
	read(IO::Directory::getCurrent(),vtkFileName);
	}

void VTKFile::read(IO::DirectoryPtr directory,const char* vtkFileName)
	{
	/* Create a VTK file reader: */
	VTKFileReader reader(directory,vtkFileName);
	
	/* Read the VTK file: */
	reader.read();
	
	/* Merge close-by vertices to remove redundancy from the saved output and enable cell face matching: */
	VertexClusterer clusterer(reader.getVertexComponents());
	
	/* Calculate a default maximum distance for cluster merging based on domain size and the selected scalar data type's machine epsilon: */
	const VertexClusterer::Box& bbox=clusterer.getBoundingBox();
	Scalar maxDim(0);
	for(int i=0;i<3;++i)
		maxDim=Math::max(maxDim,Math::max(Math::abs(bbox.min[i]),Math::abs(bbox.max[i])));
	Scalar maxDist=maxDim*Math::Constants<Scalar>::epsilon;
	
	/* Create clusters and retrieve the merged vertices: */
	numVertices=clusterer.createClusters(maxDist);
	vertices=clusterer.retrieveMergedVertices();
	
	/* Copy the read file's per-vertex properties: */
	numVertexProperties=reader.getVertexProperties().size();
	if(numVertexProperties!=0)
		{
		vertexProperties=new Property[numVertexProperties];
		Property* vpPtr=vertexProperties;
		Index nextSliceIndex(0);
		for(VTKFileReader::PropertyList::const_iterator vpIt=reader.getVertexProperties().begin();vpIt!=reader.getVertexProperties().end();++vpIt,++vpPtr)
			{
			/* Copy the property descriptor: */
			vpPtr->name=(*vpIt)->name;
			vpPtr->numComponents=(*vpIt)->numComponents;
			vpPtr->firstSliceIndex=nextSliceIndex;
			
			nextSliceIndex+=vpPtr->numComponents;
			}
	
		/* Allocate the array holding all vertex property values: */
		vertexPropertySlices=new VScalar[nextSliceIndex*numVertices];
		
		/* Copy the read file's vertex property values: */
		VScalar* vpsPtr=vertexPropertySlices;
		VTKFileReader::PropertyList::const_iterator ovpIt=reader.getVertexProperties().begin();
		for(Index vpIndex=0;vpIndex<numVertexProperties;++vpIndex,++ovpIt)
			{
			/* Retrieve the number of components of this vertex property: */
			Index numComponents=(*ovpIt)->numComponents;
			
			/* Retrieve the vertex property's original component lists: */
			const std::vector<VScalar>& originalComponents=(*ovpIt)->components;
			
			/* Copy all vertex property components into separate value slices: */
			for(Index componentIndex=0;componentIndex<numComponents;++componentIndex)
				for(Index vi=0;vi<numVertices;++vi,++vpsPtr)
					{
					/* Get the index of one of the original vertices that were merged into this merged vertex: */
					Index vertexIndex=clusterer.getOriginalVertexIndex(vi);
					
					/* Copy this vertex's vertex property component: */
					*vpsPtr=originalComponents[vertexIndex*numComponents+componentIndex];
					}
			}
		}
	
	/* Create the array of merged cell vertex indices: */
	numCells=Index(reader.getCellTypes().size());
	cellVertexIndices=new Index[reader.getCellVertexIndices().size()];
	Index* cviPtr=cellVertexIndices;
	for(std::vector<Index>::const_iterator cviIt=reader.getCellVertexIndices().begin();cviIt!=reader.getCellVertexIndices().end();++cviIt,++cviPtr)
		*cviPtr=clusterer.getMergedVertexIndex(*cviIt);
	
	/* Copy the read cell types: */
	cellTypes=new CellType[numCells];
	CellType* ctPtr=cellTypes;
	for(std::vector<CellType>::const_iterator ctIt=reader.getCellTypes().begin();ctIt!=reader.getCellTypes().end();++ctIt,++ctPtr)
		*ctPtr=*ctIt;
	
	/* Copy the read file's per-cell properties: */
	numCellProperties=reader.getCellProperties().size();
	if(numCellProperties!=0)
		{
		cellProperties=new Property[numCellProperties];
		Property* cpPtr=cellProperties;
		Index nextSliceIndex(0);
		for(VTKFileReader::PropertyList::const_iterator cpIt=reader.getCellProperties().begin();cpIt!=reader.getCellProperties().end();++cpIt,++cpPtr)
			{
			/* Copy the property descriptor: */
			cpPtr->name=(*cpIt)->name;
			cpPtr->numComponents=(*cpIt)->numComponents;
			cpPtr->firstSliceIndex=nextSliceIndex;
			
			nextSliceIndex+=cpPtr->numComponents;
			}
		
		/* Allocate the array holding all cell property values: */
		cellPropertySlices=new VScalar[nextSliceIndex*numCells];
		
		/* Copy the read file's cell property values: */
		VScalar* cpsPtr=cellPropertySlices;
		VTKFileReader::PropertyList::const_iterator ocpIt=reader.getCellProperties().begin();
		for(Index cpIndex=0;cpIndex<numCellProperties;++cpIndex,++ocpIt)
			{
			/* Retrieve the number of components of this cell property: */
			Index numComponents=(*ocpIt)->numComponents;
			
			/* Copy all cell property components into separate value slices: */
			for(Index componentIndex=0;componentIndex<numComponents;++componentIndex)
				{
				/* Retrieve the cell property's original component lists: */
				std::vector<VScalar>::const_iterator ocIt=(*ocpIt)->components.begin()+componentIndex;
				
				/* Copy all cell property values for this slice: */
				for(Index ci=0;ci<numCells;++ci,++cpsPtr,ocIt+=numComponents)
					*cpsPtr=*ocIt;
				}
			}
		}
	}

}

}
