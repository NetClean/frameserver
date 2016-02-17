FROM ubuntu:trusty
RUN apt-get -y update && apt-get -y install git cmake build-essential mingw-w64 wget nasm pkg-config

ADD deps /root/deps
RUN cd /root/deps && ./deps.sh all 32bit

ADD build.sh /bin/build
RUN chmod +x /bin/build
CMD build
