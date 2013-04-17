promix - for training translation model interpolation weights using PRO

Author: Barry Haddow <bhaddow [AT] inf.ed.ac.uk>

ABOUT
-----

The code here provides the "inner loop" for a batch tuning algorithm (like MERT) which 
optimises phrase table interpolation weights at the same time as the standard linear
model weights. Interpolation of the phrase tables uses the "naive" method of tmcombine.

Currently it only works on interpolations of two phrase tables.


REQUIREMENTS
------------
The scripts require the Moses Python interface (in contrib/python)
They also require scipy and numpy. They have been tested with the following versions:
  Python 2.7
  Scipy 0.11.0
  Numpy 1.6.2


USAGE
-----


REFERENCES
----------
