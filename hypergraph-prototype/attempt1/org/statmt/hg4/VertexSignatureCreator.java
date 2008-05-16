package org.statmt.hg4;

import java.util.Map;

public abstract class VertexSignatureCreator {

	public abstract Map<String, Object>
	    signature(VertexState vs);

}
