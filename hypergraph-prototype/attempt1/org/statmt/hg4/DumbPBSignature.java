package org.statmt.hg4;

import java.util.Map;
import java.util.HashMap;

public class DumbPBSignature extends VertexSignatureCreator {

	@Override
	public Map<String, Object> signature(VertexState vs) {
		HashMap<String, Object> res = new HashMap<String, Object>();
		res.put("COVERAGE", vs.get("COVERAGE"));
		return res;
	}

}
