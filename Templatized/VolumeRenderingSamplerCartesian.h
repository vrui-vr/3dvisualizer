/***********************************************************************
VolumeRenderingSamplerCartesian - Specialized volume rendering sampler
class for Cartesian data sets.
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

#ifndef VISUALIZATION_TEMPLATIZED_VOLUMERENDERINGSAMPLERCARTESIAN_INCLUDED
#define VISUALIZATION_TEMPLATIZED_VOLUMERENDERINGSAMPLERCARTESIAN_INCLUDED

#include <Misc/Size.h>

#include <Templatized/VolumeRenderingSampler.h>

/* Forward declarations: */
namespace Cluster {
class MulticastPipe;
}
namespace Visualization {
namespace Abstract {
class Algorithm;
}
namespace Templatized {
template <class ScalarParam,int dimensionParam,class ValueParam>
class Cartesian;
template <class ScalarParam,int dimensionParam,class ValueScalarParam>
class SlicedCartesian;
}
}

namespace Visualization {

namespace Templatized {

template <class ScalarParam,class ValueParam>
class VolumeRenderingSampler<Cartesian<ScalarParam,3,ValueParam> >
	{
	/* Embedded classes: */
	public:
	typedef Misc::Size<3> Size3; // Type for volume sizes
	typedef Cartesian<ScalarParam,3,ValueParam> DataSet; // Type of data sets on which the sampler works
	typedef typename DataSet::Scalar Scalar; // Scalar type of domain
	typedef typename DataSet::Point Point; // Type for domain points
	typedef typename DataSet::Box Box; // Type for domain boxes
	typedef typename Box::Size Size; // Type for domain sizes
	
	/* Elements: */
	private:
	const DataSet& dataSet; // The data set from which the sampler samples
	Size3 samplerSize; // Size of the Cartesian volume
	
	/* Constructors and destructors: */
	public:
	VolumeRenderingSampler(const DataSet& sDataSet); // Creates a sampler for the given data set
	
	/* Methods: */
	const Size3& getSamplerSize(void) const // Returns the size of the Cartesian volume
		{
		return samplerSize;
		}
	template <class ScalarExtractorParam,class VoxelParam>
	void sample(const ScalarExtractorParam& scalarExtractor,typename ScalarExtractorParam::Scalar minValue,typename ScalarExtractorParam::Scalar maxValue,typename ScalarExtractorParam::Scalar outOfDomainValue,VoxelParam* voxels,const ptrdiff_t voxelStrides[3],Cluster::MulticastPipe* pipe,float percentageScale,float percentageOffset,Visualization::Abstract::Algorithm* algorithm) const; // Samples scalar values from the given scalar extractor into the given voxel block
	};

template <class ScalarParam,class ValueScalarParam>
class VolumeRenderingSampler<SlicedCartesian<ScalarParam,3,ValueScalarParam> >
	{
	/* Embedded classes: */
	public:
	typedef Misc::Size<3> Size3; // Type for volume sizes
	typedef SlicedCartesian<ScalarParam,3,ValueScalarParam> DataSet; // Type of data sets on which the sampler works
	typedef typename DataSet::Scalar Scalar; // Scalar type of domain
	typedef typename DataSet::Point Point; // Type for domain points
	typedef typename DataSet::Box Box; // Type for domain boxes
	typedef typename Box::Size Size; // Type for domain sizes
	
	/* Elements: */
	private:
	const DataSet& dataSet; // The data set from which the sampler samples
	Size3 samplerSize; // Size of the Cartesian volume
	
	/* Constructors and destructors: */
	public:
	VolumeRenderingSampler(const DataSet& sDataSet); // Creates a sampler for the given data set
	
	/* Methods: */
	const Size3& getSamplerSize(void) const // Returns the size of the Cartesian volume
		{
		return samplerSize;
		}
	template <class ScalarExtractorParam,class VoxelParam>
	void sample(const ScalarExtractorParam& scalarExtractor,typename ScalarExtractorParam::Scalar minValue,typename ScalarExtractorParam::Scalar maxValue,typename ScalarExtractorParam::Scalar outOfDomainValue,VoxelParam* voxels,const ptrdiff_t voxelStrides[3],Cluster::MulticastPipe* pipe,float percentageScale,float percentageOffset,Visualization::Abstract::Algorithm* algorithm) const; // Samples scalar values from the given scalar extractor into the given voxel block
	};

}

}

#ifndef VISUALIZATION_TEMPLATIZED_VOLUMERENDERINGSAMPLERCARTESIAN_IMPLEMENTATION
#include <Templatized/VolumeRenderingSamplerCartesian.icpp>
#endif

#endif
