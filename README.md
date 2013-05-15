
Extra Implementations in "moses_onlinelearning" branch. 

For computer assisted translation, we have implemented a online learning feature that learns from the corrections by the translator

1. This feature stores the phrase pairs which occurs in Oracle translation (the closest translation to a post edited sentence).
2. These phrase pairs get rewarded if they are seen to be translated again in the future.
3. These updates are similar to perceptron update where the margin is the difference between the scores of the oracle translation and the best translation (as decided by the decoder).
4. In this system we have additional two parameters, a weight for the online feature, and a learning rate for online algorithm. 
5. These parameters can be passed as

	1. "weight-ol" : is the initial weight of the online feature function
	
	2. "f_learningrate" : is the learning rate of the perceptron algorithm

	3. "w_learningrate" : is the learning rate for the weight of online feature

6. There are different online learning algorithms implemented to update the features and the feature weights

	1. features : MIRA, Perceptron 

	2. weights : MIRA

Input can be of two types.

	1. A source sentence one wants to translate. 
		source_segment

	2. If you want moses to learn from the post edited sentence, you have the option of passing the source and post edited sentence like this
		source_segment_#_postedited_segment

The decoder detects the delimiter "_#_" and automatically splits it based on the delimiter, and updates the models and weights.

