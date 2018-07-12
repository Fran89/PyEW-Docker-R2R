FROM ubuntu:16.04
RUN rm /bin/sh && ln -s /bin/bash /bin/sh
RUN dpkg --add-architecture i386
RUN apt-get update
RUN apt-get install -y gcc-multilib g++-multilib gfortran-multilib build-essential  \
python-dev:i386 python:i386 python-minimal:i386 python2.7-dev:i386 python2.7-minimal:i386 \
python-pip
ADD ./ew /opt/earthworm
RUN source /opt/earthworm/7.9/environment/ew_linux.bash && cd /opt/earthworm/7.9/src && make unix
EXPOSE 16005
EXPOSE 16010
RUN pip install --upgrade pip
RUN pip install cython numpy
RUN ln -s /usr/bin/gcc /usr/bin/i686-linux-gnu-gcc
RUN source /opt/earthworm/7.9/environment/ew_linux.bash && cd /opt/earthworm/python/src && python setup.py build_ext -i 
RUN mv /opt/earthworm/python/src/PyEW.so /opt/earthworm/python/

CMD source /opt/earthworm/7.9/environment/ew_linux.bash && startstop
