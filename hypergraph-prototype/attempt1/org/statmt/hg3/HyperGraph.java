package org.statmt.hg3;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.Queue;
import java.util.Set;

public class HyperGraph {
	
	Set<Vertex> sinkVertices; //
	Set<Vertex> sourceVertices; //no children
	
	/**
	 * active and passive edges
	 * 
	 * passive edge (rule,phrase pair, introduced at start)
	 * active edge (hypothesis)
	 * parse: 2 adgenda/lists
	 * finishing agenda
	 * put all rules in passive, one empty passive edge
	 * @param vertex
	 */
	
	public void addNode(Vertex vertex) {
		if (!vertex.hasChildren()) sourceVertices.add(vertex);
		sinkVertices.add(vertex);
	}
	
	/**
	 * Assumes vertex and edge's children are already in graph
	 * @param vertex
	 * @param edge
	 */
	public void addLink(Vertex vertex, HyperArc arc) {
		vertex.addArc(arc);
		for(Vertex child : arc.getTail()) {
			sinkVertices.remove(child);
		}
		sourceVertices.remove(vertex);
	}
	
	
	
	
	public static void main(String[] args) {
		
		Queue<HyperArc> explorationAgenda = new LinkedList<HyperArc>();
		Queue<Vertex> finishingAgenda = new LinkedList<Vertex>();
		
		while(!finishingAgenda.isEmpty()) {
			Vertex fv = finishingAgenda.poll();
			while(!explorationAgenda.isEmpty()) {
				HyperArc ha = explorationAgenda.remove();
				/**
				 get state of ha+tail(always 2 for pb)
				 	1. phrase pair
				 	2. coverage vector
				 if equivalent state exists in finishing agenda
				 	merge(ha, equivalent state) //this is compare for viterbi
				 else
				 	new Node(state,ha)
				 	finishingAgenda.push(node);
				*/
			}
			/*
			 * hypergraph.addNode(fv);
			 * if(fv.active())
			 * 	foreach hypergraph.sinkNodes() 
			 * 		if can_combine(fv,active)
			 * 			new hyper(fv,active)
			 * 			explorationAgenda.add(hyper);
			 * 
			 * 
			 * 
			 */
			
		}
		
	}
	
	
}
