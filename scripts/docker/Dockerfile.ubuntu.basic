FROM ubuntu:latest

MAINTAINER Momo <mo@mo.com>
LABEL description="Basic Moses docker container for Ubuntu"

# Update Ubuntu.
RUN apt-get update
RUN apt-get install -y apt-utils debconf-utils
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get update && apt-get -y upgrade

# Install some necessary tools.
RUN apt-get install -y nano perl

# Install Moses dependencies.
RUN apt-get install -y libboost-all-dev
RUN apt-get install -y build-essential git-core pkg-config automake libtool wget zlib1g-dev python-dev libbz2-dev cmake

# Clone the repos we need.
RUN git clone https://github.com/moses-smt/mosesdecoder.git

# Install Moses.
WORKDIR /mosesdecoder
RUN make -f /mosesdecoder/contrib/Makefiles/install-dependencies.gmake
RUN /mosesdecoder/compile.sh  --max-kenlm-order=20 --max-factors=1000
WORKDIR /
