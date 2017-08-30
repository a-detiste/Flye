//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#pragma once

#include "repeat_graph.h"

struct EdgeAlignment
{
	OverlapRange overlap;
	GraphEdge* edge;
	SequenceSegment segment;
};
typedef std::vector<EdgeAlignment> GraphAlignment;

class RepeatResolver
{
public:
	RepeatResolver(RepeatGraph& graph, const SequenceContainer& asmSeqs,
				   const SequenceContainer& readSeqs): 
		_graph(graph), _asmSeqs(asmSeqs), _readSeqs(readSeqs) {}

	void alignReads();
	void findRepeats(int uniqueCovThreshold);
	void resolveRepeats(int meanCoverage);

	const std::vector<GraphAlignment>& getReadsAlignment() const
	{
		return _readAlignments;
	}

private:
	struct Connection
	{
		GraphPath path;
		SequenceSegment readSequence;
	};

	void clearResolvedRepeats();
	void removeUnsupportedEdges(int meanCoverage);
	std::vector<Connection> getConnections();
	int  resolveConnections(const std::vector<Connection>& conns);
	void separatePath(const GraphPath& path, SequenceSegment segment,
					  FastaRecord::Id startId);
	GraphAlignment chainReadAlignments(const SequenceContainer& edgeSeqs,
									   const std::vector<EdgeAlignment>& ovlps) const;
	int updateAlignments();

	std::vector<GraphAlignment> _readAlignments;

	RepeatGraph& _graph;
	const SequenceContainer& _asmSeqs;
	const SequenceContainer& _readSeqs;
};
