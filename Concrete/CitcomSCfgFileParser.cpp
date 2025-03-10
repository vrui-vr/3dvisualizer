/***********************************************************************
CitcomSCfgFileParser - Helper function to parse configuration files
describing the results of a CitcomS simulation run.
Copyright (c) 2008-2024 Oliver Kreylos

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

#include <Concrete/CitcomSCfgFileParser.h>

#include <Misc/StdError.h>
#include <IO/ValueSource.h>

namespace Visualization {

namespace Concrete {

void
parseCitcomSCfgFile(
	const std::string& cfgFileName,
	IO::FilePtr cfgFile,
	std::string& dataDir,
	std::string& dataFileName,
	int& numSurfaces,
	Misc::ArrayIndex<3>& numCpus,
	Misc::ArrayIndex<3>& numVertices)
	{
	/* Read the run's configuration file: */
	IO::ValueSource cfgSource(cfgFile);
	cfgSource.setPunctuation("#;[]=");
	cfgSource.skipWs();
	bool inSolverSection=false;
	bool inSolverMesherSection=false;
	while(!cfgSource.eof())
		{
		/* Read the next tag: */
		std::string tag=cfgSource.readString();
		if(tag=="#"||tag==";")
			{
			/* Skip the rest of the line: */
			cfgSource.skipLine();
			cfgSource.skipWs();
			}
		else if(tag=="[")
			{
			/* Read the section name: */
			std::string section=cfgSource.readString();
			if(!cfgSource.isLiteral(']'))
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Malformed section header in configuration file %s",cfgFileName.c_str());
			
			/* Check for known sections: */
			inSolverSection=section=="CitcomS.solver";
			inSolverMesherSection=section=="CitcomS.solver.mesher";
			}
		else if(inSolverSection)
			{
			/* Check for equal sign: */
			if(!cfgSource.isLiteral('='))
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing \"=\" in tag %s in configuration file %s",tag.c_str(),cfgFileName.c_str());
			
			if(tag=="datadir")
				{
				/* Read the data directory: */
				dataDir=cfgSource.readString();
				}
			else if(tag=="datafile")
				{
				/* Read the data file name: */
				dataFileName=cfgSource.readString();
				}
			else
				{
				/* Skip the rest of the line: */
				cfgSource.skipLine();
				cfgSource.skipWs();
				}
			}
		else if(inSolverMesherSection)
			{
			/* Check for equal sign: */
			if(!cfgSource.isLiteral('='))
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing \"=\" in tag %s in configuration file %s",tag.c_str(),cfgFileName.c_str());
			
			if(tag=="nproc_surf")
				numSurfaces=cfgSource.readInteger();
			else if(tag=="nprocx")
				numCpus[0]=cfgSource.readInteger();
			else if(tag=="nprocy")
				numCpus[1]=cfgSource.readInteger();
			else if(tag=="nprocz")
				numCpus[2]=cfgSource.readInteger();
			else if(tag=="nodex")
				numVertices[0]=cfgSource.readInteger();
			else if(tag=="nodey")
				numVertices[1]=cfgSource.readInteger();
			else if(tag=="nodez")
				numVertices[2]=cfgSource.readInteger();
			else
				{
				/* Skip the rest of the line: */
				cfgSource.skipLine();
				cfgSource.skipWs();
				}
			}
		else
			{
			/* Skip the rest of the line: */
			cfgSource.skipLine();
			cfgSource.skipWs();
			}
		}
	}

}

}
