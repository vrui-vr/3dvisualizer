/***********************************************************************
VertexClusterer - Class to cluster close-by vertices read from a VTK
data file to enable cell connectivity reconstruction.
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

#ifndef VISUALIZATION_CONCRETE_VERTEXCLUSTERER_INCLUDED
#define VISUALIZATION_CONCRETE_VERTEXCLUSTERER_INCLUDED

#include <vector>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/ValuedPoint.h>
#include <Geometry/Box.h>
#include <Geometry/ArrayKdTree.h>

#include <Concrete/VTKFile.h>

namespace Visualization {

namespace Concrete {

class VertexClusterer // Helper class to cluster cell vertices for index sharing
	{
	/* Embedded classes: */
	public:
	typedef VTKFile::Scalar Scalar; // Scalar type for points
	typedef VTKFile::Point Point; // Type for points
	typedef VTKFile::Index Index; // Type for indices
	typedef Geometry::Box<Scalar,3> Box; // Type for axis-aligned boxes
	private:
	typedef Math::Constants<Scalar>::PrecisionScalar PScalar; // Scalar type for higher-precision calculations
	typedef Geometry::ValuedPoint<Point,Index> IPoint; // Type for points with associated indices
	typedef Geometry::ArrayKdTree<IPoint> VertexTree; // Type for kd-trees of indexed points
	
	struct Cluster // Structure to hold cluster state for subset-merge algorithm
		{
		/* Elements: */
		public:
		Cluster* root; // Pointer to this cluster's root cluster
		PScalar centroidAcc[3]; // Accumulated centroid position
		PScalar centroidWeight; // Total weight of points that have been accumulated into this cluster's centroid
		Index vertexIndex; // Vertex index assigned to a root cluster
		};
	
	/* Elements: */
	private:
	VertexTree vertices; // Kd-tree containing all cell vertices for nearest-neighbor look-ups
	Box bbox; // Bounding box of all cell vertices
	Cluster* clusters; // Array of one cluster for each cell vertex for subset-merge algorithm
	Index numClusters; // Number of remaining separate clusters after merging step
	Cluster** rootClusters; // Array of pointers to root clusters, in order of increasing merged vertex index
	
	/* Additional state for kd-tree traversals: */
	Point currentCenter; // Center position for current kd-tree traversal
	Scalar currentMaxDist2; // Squared maximum search distance for current kd-tree traversal
	Cluster* currentCluster; // The cluster currently being merged with near-by clusters
	
	/* Constructors and destructors: */
	public:
	VertexClusterer(const std::vector<Scalar>& vertexCoords) // Creates a vertex clusterer for the given point set
		:clusters(0),rootClusters(0)
		{
		/* Create a kd-tree and initial cluster array holding all vertices: */
		bbox=Box::empty;
		Index numVertices=Index(vertexCoords.size()/3);
		IPoint* vPtr=vertices.createTree(numVertices);
		clusters=new Cluster[numVertices];
		Cluster* cPtr=clusters;
		std::vector<Scalar>::const_iterator vcIt=vertexCoords.begin();
		for(Index i=0;i<numVertices;++i,++vPtr,++cPtr)
			{
			/* Add the vertex to the kd-tree: */
			for(int j=0;j<3;++j,++vcIt)
				(*vPtr)[j]=*vcIt;
			vPtr->value=i;
			
			/* Add the vertex to the bounding box: */
			bbox.addPoint(*vPtr);
			
			/* Singleton clusters are their own roots: */
			cPtr->root=cPtr;
			
			/* Singleton clusters have their original vertices as centroids: */
			for(int j=0;j<3;++j)
				cPtr->centroidAcc[j]=PScalar((*vPtr)[j]);
			cPtr->centroidWeight=PScalar(1);
			}
		vertices.releasePoints();
		}
	~VertexClusterer(void) // Destroys the vertex clusterer
		{
		/* Destroy the cluster array: */
		delete[] clusters;
		delete[] rootClusters;
		}
	
	/* Methods: */
	const Box& getBoundingBox(void) const // Returns the vertices' bounding box
		{
		return bbox;
		}
	Index createClusters(Scalar maxDist) // Creates clusters by joining points no more than maxDist apart; returns number of remaining separate clusters
		{
		/* Merge near-by clusters by traversing the vertex kd-tree for each vertex: */
		Index numVertices=Index(vertices.getNumNodes());
		currentMaxDist2=Math::sqr(maxDist);
		Cluster* cPtr=clusters;
		IPoint* vPtr=vertices.accessPoints();
		for(Index i=0;i<numVertices;++i,++cPtr,++vPtr)
			{
			currentCenter=*vPtr;
			currentCluster=clusters+vPtr->value;
			vertices.traverseTreeDirected(*this);
			}
		
		/* Assign vertex indices to all root clusters, and short-circuit all non-root clusters: */
		numClusters=Index(0);
		Cluster* cEnd=clusters+numVertices;
		for(Cluster* cPtr=clusters;cPtr!=cEnd;++cPtr)
			{
			/* Check if the cluster is a root cluster: */
			if(cPtr==cPtr->root)
				{
				/* Assign a new vertex index to this cluster: */
				cPtr->vertexIndex=numClusters;
				++numClusters;
				}
			else
				{
				/* Short-circuit the cluster: */
				while(cPtr->root!=cPtr->root->root)
					cPtr->root=cPtr->root->root;
				}
			}
		
		/* Collect all root clusters into an array: */
		rootClusters=new Cluster*[numClusters];
		Cluster** rcPtr=rootClusters;
		for(Cluster* cPtr=clusters;cPtr!=cEnd;++cPtr)
			if(cPtr==cPtr->root)
				*(rcPtr++)=cPtr;
		
		return numClusters;
		}
	Point* retrieveMergedVertices(void) const // Returns a new-allocated array containing the positions of all merged vertices
		{
		/* Allocate the result array: */
		Point* result=new Point[numClusters];
		
		/* Create vertices from root clusters: */
		Point* rPtr=result;
		Cluster** rcEnd=rootClusters+numClusters;
		for(Cluster** rcPtr=rootClusters;rcPtr!=rcEnd;++rcPtr,++rPtr)
			{
			/* Create a vertex: */
			for(int i=0;i<3;++i)
				(*rPtr)[i]=Scalar((*rcPtr)->centroidAcc[i]/(*rcPtr)->centroidWeight);
			}
		
		return result;
		}
	void retrieveMergedVertices(std::vector<Scalar>& vertexComponents) const // Ditto; appends merged vertex components to the given list
		{
		/* Create vertices from root clusters: */
		Cluster** rcEnd=rootClusters+numClusters;
		for(Cluster** rcPtr=rootClusters;rcPtr!=rcEnd;++rcPtr)
			{
			/* Create a vertex: */
			for(int i=0;i<3;++i)
				vertexComponents.push_back(Scalar((*rcPtr)->centroidAcc[i]/(*rcPtr)->centroidWeight));
			}
		}
	Index getMergedVertexIndex(Index originalVertexIndex) const // Returns the merged vertex index for the vertex of the given original index
		{
		/* Merged vertex indices are at most one root pointer hop away, as all clusters have been short-circuited: */
		return clusters[originalVertexIndex].root->vertexIndex;
		}
	Index getOriginalVertexIndex(Index mergedVertexIndex) const // Returns one original vertex index of the merged vertex of the given index
		{
		return Index(rootClusters[mergedVertexIndex]-clusters);
		}
	std::vector<Index> getOriginalVertexIndices(Index mergedVertexIndex) const // Returns list of the indices of all original vertices that were merged into the merged vertex of the given index
		{
		std::vector<Index> result;
		
		/* Get a pointer to the cluster corresponding to the merged vertex of the given index: */
		const Cluster* mergedCluster=rootClusters[mergedVertexIndex];
		
		/* Collect the indices of all clusters that belong to the merged cluster: */
		const Cluster* cPtr=clusters;
		Index numVertices=Index(vertices.getNumNodes());
		for(Index i=0;i<numVertices;++i,++cPtr)
			if(cPtr->root==mergedCluster)
				result.push_back(i);
		
		return result;
		}
	
	/* Methods for ArrayKdTree::traverseTreeDirected: */
	const Point& getQueryPosition(void) const
		{
		return currentCenter;
		}
	bool operator()(const IPoint& node,int splitDimension)
		{
		/* Check if the node is close enough: */
		if(Geometry::sqrDist(node,currentCenter)<=currentMaxDist2)
			{
			/* Find the current cluster's root: */
			Cluster* root=currentCluster;
			while(root!=root->root)
				root=root->root;
			
			/* Retrieve the other point's cluster: */
			Cluster* otherCluster=clusters+node.value;
			
			/* Find the other cluster's root: */
			Cluster* otherRoot=otherCluster;
			while(otherRoot!=otherRoot->root)
				otherRoot=otherRoot->root;
			
			/* Check if the two clusters are still separate: */
			if(otherRoot!=root)
				{
				/* Merge the two clusters: */
				for(int i=0;i<3;++i)
					root->centroidAcc[i]+=otherRoot->centroidAcc[i];
				root->centroidWeight+=otherRoot->centroidWeight;
				otherRoot->root=root;
				}
			
			/* Short-circuit the two clusters: */
			currentCluster->root=root;
			otherCluster->root=root;
			}
		
		/* Cull the other side of the node if the node is too far away: */
		return Math::sqr(currentCenter[splitDimension]-node[splitDimension])<=currentMaxDist2;
		}
	};

}

}

#endif
