# Python interface to Moses

The idea is to have some of Moses' internals exposed to Python (inspired by pycdec).

---
## What's been interfaced?

* Binary phrase table:

        Moses::PhraseDictionaryTree.h

---

## Building
1.  Compile the cython code

        cython --cplus binpt/binpt.pyx

2.  Build the python extension

        python setup.py build_ext -i

3.  Check the example code

        echo '! " and "' | python example.py bin-ptable-stem 5 1
        echo "casa" | python example.py bin-ptable-stem 5
