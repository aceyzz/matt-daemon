FROM debian:stable-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
	build-essential \
	make \
	vim \
	valgrind \
	openssl \
	curl \
	ca-certificates \
	git \
	netcat-traditional \
	sudo \
	&& apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN bash -c "$(curl -fsSL https://raw.githubusercontent.com/ohmybash/oh-my-bash/master/tools/install.sh)" --unattended

RUN mkdir -p /work

RUN useradd -ms /bin/bash cedmulle
RUN echo "cedmulle ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

USER cedmulle
RUN bash -c "$(curl -fsSL https://raw.githubusercontent.com/ohmybash/oh-my-bash/master/tools/install.sh)" --unattended

WORKDIR /work