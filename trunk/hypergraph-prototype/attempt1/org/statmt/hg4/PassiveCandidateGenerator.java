package org.statmt.hg4;

import java.util.Collection;
import java.util.Map;

public abstract class PassiveCandidateGenerator {
	public abstract Collection<Vertex> retrieveCominableNodesForActiveVertex(Vertex activeVertex,
			Map<String, Object> vertexState, Phrase sentence); 

}
