package org.statmt.hg4;

import java.util.Map;

public abstract class FundamentalRuleFunctor {

	public abstract VertexState apply(
			Hyperarc ha,
			Map<Vertex, VertexState> v2s);

}
