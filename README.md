
Extra Implementations in "moses_onlinelearning" branch. 

For computer assisted translation, we have implemented a online learning feature that learns from the corrections by the translator.
This feature stores the phrase pairs which occurs in Oracle translation (the closest translation to a post edited sentence).
These phrase pairs get rewarded if they are seen to be translated again in the future.
The feature also stores the phrase pairs in non-oracles and penalize them if they are used in the future translations.
This system has additional parameters, a weight for the online feature, and a learning rate for online algorithm. 
These parameters can be passed as

	1. "weight-ol" : is the initial weight of the online feature function
	
	2. "f_learningrate" : is the learning rate for online algorithm to update the feature

	3. "w_learningrate" : is the learning rate for online algorithm to update the weight of online feature

There are different online learning algorithms implemented to update the features and the feature weights

	1. features : MIRA, Perceptron 

	2. weights : MIRA

Input can be of two types.

	1. A source sentence one wants to translate. 
		source_segment

	2. If you want moses to learn from the post edited sentence, you have the option of 
	passing the source and post edited sentence like this
		source_segment_#_postedited_segment

The decoder detects the delimiter "_#_" and automatically splits it based on the delimiter, and updates the models and weights.

