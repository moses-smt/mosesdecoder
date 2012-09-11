# Moses interface for python

1.  Compile the cython code

    cython --cplus binpt/binpt.pyx

2.  Build the python extension

    python setup.py build_ext -i

3.  Check the example code

    echo '! " and "' | python example.py /media/Data/data/smt/sample/bin/sample.en-es 5 1

    echo "casa" | python example.py /media/Data/data/smt/fapesp/bin/fapesp.br-en 5
