# Image that will be used. Publish as dawidblom/admin-system:v5.3.0.
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

# turso_sqlite3 static library. This is Turso's sqlite3-compatible C API,
# not the libSQL SDK. The commit matches the tree used for the vendored header.
# The Rust toolchain is removed after the archive is installed.
RUN set -o pipefail \
 && for attempt in 1 2 3 4 5; do \
      curl --proto '=https' --tlsv1.2 -fsSL https://sh.rustup.rs -o /tmp/rustup-init.sh && break; \
      echo "rustup download failed, attempt ${attempt}" >&2; \
      sleep 5; \
    done \
 && test -s /tmp/rustup-init.sh \
 && sh /tmp/rustup-init.sh -y --default-toolchain 1.97.1 --profile minimal \
 && . /root/.cargo/env \
 && git init /tmp/turso-src \
 && git -C /tmp/turso-src remote add origin https://github.com/tursodatabase/turso.git \
 && git -C /tmp/turso-src fetch --depth 1 origin b5d373732bc1025eb0ad21c9ef32c18c5cdb406d \
 && git -C /tmp/turso-src checkout --detach FETCH_HEAD \
 && cargo build --manifest-path /tmp/turso-src/Cargo.toml -p turso_sqlite3 --release \
 && mkdir -p /opt/turso \
 && cp /tmp/turso-src/target/release/libturso_sqlite3.a /opt/turso/libturso_sqlite3.a \
 && chmod 644 /opt/turso/libturso_sqlite3.a \
 && rm -rf /tmp/turso-src /root/.cargo /root/.rustup

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
