package org.statmt.hg4;

public class DumbPBSignature extends VertexSignatureCreator {

	@Override
	public VertexSignature signature(VertexState vs) {
		VertexSignature res = new VertexSignature();
		res.put("COVERAGE", vs.get("COVERAGE"));
		res.put("ACTIVE?", vs.get("ACTIVE?"));
		return res;
	}

}
