package org.statmt.ghgd;

import java.util.ArrayList;
import java.util.Properties;

public abstract class Rule {

	public abstract String recoverString(ArrayList<Or> children, Properties p);
	
}
