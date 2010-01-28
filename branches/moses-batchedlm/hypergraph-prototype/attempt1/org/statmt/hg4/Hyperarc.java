package org.statmt.hg4;
import java.util.List;

public class Hyperarc {

        List<Vertex> tail;

        public List<Vertex> getTail() {
                return tail;
        }

        public Hyperarc(List<Vertex> tail) {
                this.tail = tail;
        }
        
        public String toString() {
        	return super.toString() + "  tail=" + tail;
        }
}
              