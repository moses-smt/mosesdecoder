package org.statmt.hg4;

/**
 * For a vertex, returns the features that identify its equivalence class for
 * purely monotonic scoring.  For example, in CKY parsing, this is everything but the
 * LM score/state (or any other context-dependent features).
 * 
 * This is similar to the "SuperItem" in the JosHUa decoder.
 * 
 * @author redpony
 *
 */
public abstract class VertexSignatureCreator {

	public abstract VertexSignature
	    signature(VertexState vs);

}
