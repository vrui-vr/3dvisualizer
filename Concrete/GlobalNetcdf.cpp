/***********************************************************************
GlobalNetcdf - Class to visualize global 3D data on a latitude/longitude
grid stored in a NetCDF file.
Copyright (c) 2013 Oliver Kreylos

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

#include <Concrete/GlobalNetcdf.h>

#include <netcdfcpp.h>
#include <Misc/ThrowStdErr.h>

#include <Concrete/SphericalCoordinateTransformer.h>
#include <Concrete/EarthDataSet.h>

namespace Visualization {

namespace Concrete {

/*****************************
Methods of class GlobalNetcdf:
*****************************/

GlobalNetcdf::GlobalNetcdf(void)
	:BaseModule("GlobalNetcdf")
	{
	}

Visualization::Abstract::DataSet* GlobalNetcdf::load(const std::vector<std::string>& args,Cluster::MulticastPipe* pipe) const
	{
	/* Parse the module command line: */
	const char* fileName=0;
	for(unsigned int i=0;i<args.size();++i)
		{
		if(args[i][0]=='-')
			{
			}
		else if(fileName==0)
			fileName=args[i].c_str();
		}
	if(fileName==0)
		throw std::runtime_error("GlobalNetcdf::load: No NetCDF file name provided");
	
	/* Open the NetCDF file: */
	NcFile file(fileName);
	if(!file.isValid())
		Misc::throwStdErr("GlobalNetCDF::load: Error loading NetCDF file %s",fileName);
	
	}

Visualization::Abstract::DataSetRenderer* GlobalNetcdf::getRenderer(const Visualization::Abstract::DataSet* dataSet) const
	{
	return new EarthDataSetRenderer<DataSet,DataSetRenderer>(dataSet);
	}

}

}

/***************************
Plug-in interface functions:
***************************/

extern "C" Visualization::Abstract::Module* createFactory(Plugins::FactoryManager<Visualization::Abstract::Module>& manager)
	{
	/* Create module object and insert it into class hierarchy: */
	Visualization::Concrete::GlobalNetcdf* module=new Visualization::Concrete::GlobalNetcdf();
	
	/* Return module object: */
	return module;
	}

extern "C" void destroyFactory(Visualization::Abstract::Module* module)
	{
	delete module;
	}
