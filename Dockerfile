# Image that will be used
FROM fedora:40

# Shell that will be used
SHELL ["/bin/bash", "-c"]

# Installing all development dependencies
RUN dnf -y install \
    python3 lcov cloc cppcheck git texinfo help2man \
    make cmake dh-autoreconf autoconf automake \
    binutils bison valgrind check gcovr gcc gcc-c++ \
    glibc-devel curl gtk4 gtk4-devel gtkmm4.0-devel \
    cairomm cairomm-devel libcurl libcurl-devel \ 
    poppler-cpp-devel xorg-x11-server-Xvfb dbus-daemon \
    dbus-x11 mesa-libGLES
    

RUN dnf swap libcurl-minimal libcurl

ENV USER=admin-system
ENV HOME_DIR=/home/${USER}

# Setting up the user environment
RUN groupadd --gid=1002 ${USER} && \
    useradd --uid=1001 --gid=${USER} --create-home ${USER} && \
    echo "${USER} ALL=(ALL) NOPASSWD : ALL" >> /etc/sudoers && \
    sudo -u ${USER} mkdir /home/${USER}/bin && \
    sudo -u ${USER} mkdir -p /home/${USER}/.local/bin

# Setting up the GUI environment
RUN mkdir -p /tmp/.X11-unix && \
    chown root:root /tmp/.X11-unix && \
    chmod 1777 /tmp/.X11-unix && \
    dbus-uuidgen > /etc/machine-id

# Setup CppUTest
WORKDIR $HOME_DIR
RUN git clone https://github.com/cpputest/cpputest.git

WORKDIR $HOME_DIR/cpputest
RUN autoreconf --install
RUN ./configure
RUN make tdd
ENV CPPUTEST_HOME=$HOME_DIR/cpputest

# Providing system information like adding things to the path and setting up the language to be used
ENV PATH=/home/${USER}/bin:$PATH \
    PATH=/home/${USER}/.local/bin:$PATH \
    PATH=/usr/include:$PATH \
    PATH=/usr/share:$PATH \
    LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=C.UTF-8 \
    LIBGL_ALWAYS_SOFTWARE=1


# Finnishing off by setting the user and changing into the project directory
WORKDIR $HOME_DIR
RUN sudo chown -R ${USER}:${USER} /home/${USER}/
USER ${USER}
