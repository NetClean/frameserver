FROM ubuntu:bionic
RUN rm -rf /var/lib/apt/lists/*
RUN sed -i -e 's/archive\./se\.archive\./g' /etc/apt/sources.list
RUN apt-get -y clean
RUN apt-get -y update && apt-get -y install git cmake build-essential mingw-w64 wget nasm pkg-config sudo

ADD deps /root/deps
RUN cd /root/deps && bash -x ./deps.sh all 32bit bogus-argument-to-force-docker-rebuild-0001
RUN cd /root/deps && rm -rf libs/ out/
RUN cd /root/deps && ./deps.sh all 64bit bogus-argument-to-force-docker-rebuild-0001

ADD build.sh /bin/build
RUN chmod +x /bin/build
CMD build
