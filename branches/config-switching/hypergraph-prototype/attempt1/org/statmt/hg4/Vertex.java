package org.statmt.hg4;

import java.util.ArrayList;
import java.util.List;

public class Vertex {

        List<Hyperarc> incomingArcs = null;

        public void addArc(Hyperarc arc) {
                if (incomingArcs == null) incomingArcs = new ArrayList<Hyperarc>();
                incomingArcs.add(arc);
        }

        public boolean hasChildren() {
                return (incomingArcs != null && !incomingArcs.isEmpty());
        }

        /**
         * Constructor for non-source Vertex
         * @param incomingArcs
         */
        public Vertex(List<Hyperarc> incomingArcs) {
                this.incomingArcs = incomingArcs;
        }


        public String toString() {
        	return super.toString() + " inarcs=" + incomingArcs;
        }

}
