FROM ubuntu:latest

MAINTAINER Momo <mo@mo.com>
LABEL description="Moses docker container for 'Faster and Lighter Phrase-based Machine Translation Baseline' (aka vanilla-moses)"

# Update Ubuntu.
RUN apt-get update
RUN apt-get install -y apt-utils debconf-utils
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get update && apt-get -y upgrade

# Install some necessary tools.
RUN apt-get install -y sudo nano perl python-dev python3-dev python-pip python3-pip curl wget tar dtrx

# Install Moses dependencies.
RUN apt-get install -y libboost-all-dev
RUN apt-get install -y build-essential git-core pkg-config automake libtool wget zlib1g-dev python-dev libbz2-dev cmake

# Clone the repos we need.
RUN git clone https://github.com/moses-smt/mosesdecoder.git
RUN git clone https://github.com/moses-smt/mgiza.git
RUN git clone https://github.com/jonsafari/clustercat.git

# Install Moses.
WORKDIR /mosesdecoder
RUN make -f /mosesdecoder/contrib/Makefiles/install-dependencies.gmake
RUN /mosesdecoder/compile.sh  --max-kenlm-order=20 --max-factors=1000
WORKDIR /

# Install MGIZA++.
WORKDIR /mgiza/mgizapp
RUN cmake . && make && make install
RUN cp /mgiza/mgizapp/scripts/merge_alignment.py /mgiza/mgizapp/bin/
WORKDIR /

# Install clustercat.
WORKDIR /clustercat
RUN make -j 4
WORKDIR /

# Clean up the container.
RUN mkdir moses-training-tools
RUN cp /mgiza/mgizapp/bin/* /moses-training-tools/
RUN cp /clustercat/bin/clustercat /moses-training-tools/
RUN cp /clustercat/bin/mkcls /moses-training-tools/mkcls-clustercat
RUN mv /moses-training-tools/mkcls /moses-training-tools/mkcls-original
RUN cp /moses-training-tools/mkcls-clustercat /moses-training-tools/mkcls
