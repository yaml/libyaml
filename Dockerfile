FROM ubuntu:16.04
RUN apt-get update && \
    apt-get install -y \
        autoconf \
        build-essential \
        git \
        gist \
        libtool \
        tig \
        vim

WORKDIR /libyaml

CMD ["bash"]
