/***********************************************************************
Module - Abstract base class to represent modules of visualization data
types and algorithms. A module corresponds to a dynamically-linkable
unit of functionality in a 3D visualization application.
Part of the abstract interface to the templatized visualization
components.
Copyright (c) 2005-2024 Oliver Kreylos

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

#include <Abstract/Module.h>

#include <string.h>
#include <Misc/StdError.h>

namespace Visualization {

namespace Abstract {

/***********************
Methods of class Module:
***********************/

std::string Module::makeVectorSliceName(const std::string& vectorName,int sliceIndex)
	{
	std::string result=vectorName;
	
	if(sliceIndex<3)
		{
		result.push_back(' ');
		result.push_back('X'+sliceIndex);
		}
	else
		result.append(" Magnitude");
	
	return result;
	}

Module::Module(const char* sClassName)
	:Plugins::Factory(sClassName),
	 baseDirectory(IO::Directory::getCurrent())
	{
	}

Module::~Module(void)
	{
	}

void Module::setBaseDirectory(IO::DirectoryPtr newBaseDirectory)
	{
	/* Set the base directory: */
	baseDirectory=newBaseDirectory;
	}

int Module::getNumScalarAlgorithms(void) const
	{
	return 0;
	}

const char* Module::getScalarAlgorithmName(int scalarAlgorithmIndex) const
	{
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid algorithm index %d",scalarAlgorithmIndex);
	return 0;
	}

#if VISUALIZATION_CONFIG_USE_COLLABORATION

Collab::DataType::TypeID Module::createScalarAlgorithmParametersType(int scalarAlgorithmIndex,Collab::DataType& dataType) const
	{
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid algorithm index %d",scalarAlgorithmIndex);
	return 0;
	}

#endif

Algorithm* Module::getScalarAlgorithm(int scalarAlgorithmIndex,VariableManager* variableManager,Cluster::MulticastPipe* pipe) const
	{
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid algorithm index %d",scalarAlgorithmIndex);
	return 0;
	}

int Module::getNumVectorAlgorithms(void) const
	{
	return 0;
	}

const char* Module::getVectorAlgorithmName(int vectorAlgorithmIndex) const
	{
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid algorithm index %d",vectorAlgorithmIndex);
	return 0;
	}

#if VISUALIZATION_CONFIG_USE_COLLABORATION

Collab::DataType::TypeID Module::createVectorAlgorithmParametersType(int vectorAlgorithmIndex,Collab::DataType& dataType) const
	{
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid algorithm index %d",vectorAlgorithmIndex);
	return 0;
	}

#endif

Algorithm* Module::getVectorAlgorithm(int vectorAlgorithmIndex,VariableManager* variableManager,Cluster::MulticastPipe* pipe) const
	{
	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid algorithm index %d",vectorAlgorithmIndex);
	return 0;
	}

Algorithm* Module::getAlgorithm(const char* algorithmName,VariableManager* variableManager,Cluster::MulticastPipe* pipe) const
	{
	/* Try all scalar algorithms: */
	int numScalarAlgorithms=getNumScalarAlgorithms();
	for(int i=0;i<numScalarAlgorithms;++i)
		if(strcmp(algorithmName,getScalarAlgorithmName(i))==0)
			return getScalarAlgorithm(i,variableManager,pipe);
	
	/* Try all vector algorithms: */
	int numVectorAlgorithms=getNumVectorAlgorithms();
	for(int i=0;i<numVectorAlgorithms;++i)
		if(strcmp(algorithmName,getVectorAlgorithmName(i))==0)
			return getVectorAlgorithm(i,variableManager,pipe);
	
	/* Return an invalid algorithm: */
	return 0;
	}

}

}
