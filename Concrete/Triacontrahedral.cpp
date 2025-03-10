/***********************************************************************
Triacontrahedral - Class for spherical grids subdivided based on a
rhombic triacontrahedron.
Copyright (c) 2012 Oliver Kreylos

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

#include <Concrete/Triacontrahedral.h>

#include <Misc/SelfDestructPointer.h>
#include <Misc/ThrowStdErr.h>
#include <IO/File.h>

#include <Concrete/SphericalCoordinateTransformer.h>
#include <Concrete/EarthDataSet.h>

namespace Visualization {

namespace Concrete {

/*********************************
Methods of class Triacontrahedral:
*********************************/

Triacontrahedral::Triacontrahedral(void)
	:BaseModule("Triacontrahedral")
	{
	}

Visualization::Abstract::DataSet* Triacontrahedral::load(const std::vector<std::string>& args,Cluster::MulticastPipe* pipe) const
	{
	bool master=pipe==0||pipe->isMaster();
	
	/* Create the result data set: */
	Misc::SelfDestructPointer<DataSet> result(new DataSet);
	DS& dataSet=result->getDs();
	
	/* Open the input file: */
	IO::FilePtr file(openFile(args[0],pipe));
	file->setEndianness(Misc::LittleEndian);
	
	/* Read the number of grid vertices per tile: */
	DS::Index tileNumVertices;
	file->read<int>(tileNumVertices.getComponents(),2);
	
	/* Read the number of radial slices: */
	tileNumVertices[2]=file->read<int>();
	
	/* Read the radii of all slices: */
	double* sliceRadii=new double[tileNumVertices[2]];
	file->read<double>(sliceRadii,tileNumVertices[2]);
	
	/* Initialize the data set's grid structure: */
	dataSet.setNumGrids(30);
	for(int tile=0;tile<30;++tile)
		{
		/* Allocate the tile's grid: */
		dataSet.setGrid(tile,tileNumVertices);
		}
	
	delete[] sliceRadii;
	
	/* Initialize the result data set's data value: */
	DataValue& dataValue=result->getDataValue();
	dataValue.initialize(&dataSet,0);
	

	}

}

/***************************
Plug-in interface functions:
***************************/

extern "C" Visualization::Abstract::Module* createFactory(Plugins::FactoryManager<Visualization::Abstract::Module>& manager)
	{
	/* Create module object and insert it into class hierarchy: */
	Visualization::Concrete::Triacontrahedral* module=new Visualization::Concrete::Triacontrahedral();
	
	/* Return module object: */
	return module;
	}

extern "C" void destroyFactory(Visualization::Abstract::Module* module)
	{
	delete module;
	}

}
