# This Dockerfile has the following stages
# - init: from `buildpack-deps:noble` which has a `bouncer` user/group and required software packages
# - core: from `init` which sets up environment variables and libraries from tests suite
# - build: from `core` which performs the compilation and packaging for release
# - deploy-base-core: from `init` which copies binaries from `build` for `3drepobouncer` and `bouncer_worker`
# - deploy: from `deploy-base-core` which installs required dependencies, configuration, and start script

FROM buildpack-deps:24.04 AS init
ARG DEBIAN_FRONTEND=noninteractive

ARG NODE_USERNAME=bouncer
ENV NODE_USERNAME ${NODE_USERNAME}
ARG NODE_GROUP=bouncer
ENV NODE_GROUP ${NODE_GROUP}
ARG NODE_UID=1101
ARG NODE_GID=1102       
RUN if [ ${NODE_USERNAME} != "root" ] \
    && [ ${NODE_GROUP} != "root" ] \
    && [ ${NODE_UID} -ne 0 ] \
    && [ ${NODE_GID} -ne 0 ]; then \
      groupadd ${NODE_GROUP} -g ${NODE_GID} && \
      useradd -g ${NODE_GID} -u ${NODE_UID} ${NODE_USERNAME} ; \
    fi

#ensure ipv6 is disabled as the cryptolens timecheck doesn't support it correctly.
RUN echo >> /etc/sysctl.conf ; \
        echo net.ipv6.conf.all.disable_ipv6 = 1 >> /etc/sysctl.conf ;\
        echo net.ipv6.conf.default.disable_ipv6 = 1 >> /etc/sysctl.conf

# Add Node.js 22 and Yarn
        RUN cd /tmp && \
        curl -fsSL https://deb.nodesource.com/setup_22.x | bash - && \
        curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | apt-key add - && \
        echo "deb https://dl.yarnpkg.com/debian/ stable main" | tee /etc/apt/sources.list.d/yarn.list && \
        apt-get update && \
        apt-get install -y nodejs yarn software-properties-common && \
        rm -rf /var/lib/apt/lists/*


RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
        echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections && \
        apt-get update && \
        apt-get install -y \
        apt-transport-https \
        gosu \
        less \
        cmake \
        tk-dev \
        tcl-dev \
        libxi-dev \
        libgtk-3-0 \
        libssl-dev \
        libxft-dev \
        libxmu-dev \
        zlib1g-dev \
        libxcursor1 \
        libboost-all-dev \
        libcurl4-openssl-dev \
        ttf-mscorefonts-installer \
        # libraries for IFCOPENSHELL install
        tk-dev \
        tcl-dev \
        libxmu-dev \
        libxi-dev \
        libboost-all-dev \
        libcgal-dev \
        libocct-data-exchange-dev \
        libocct-draw-dev \
        libocct-foundation-dev \
        libocct-modeling-algorithms-dev \
        libocct-modeling-data-dev \
        libocct-ocaf-dev \
        libocct-visualization-dev \
        libgmp-dev \
        libmpfr-dev \
        libxml2-dev \
        libeigen3-dev && \
        rm -rf /var/lib/apt/lists/*

FROM init AS core
ARG CORE_REBUILD=unknown
ARG GITHUB_TOKEN
ARG GITHUB_USER
ARG DEBIAN_FRONTEND=noninteractive

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV gcc_version=8.3
ENV MONGO_ROOT=/home/bouncer/mongo-cxx-driver
ENV ASSIMP_ROOT=/home/bouncer/assimp
ENV IFCOPENSHELL_ROOT=/home/bouncer/IfcOpenShell
ENV ODA_ROOT=/home/bouncer/oda/teighaLinuxGCC${gcc_version}
ENV ODA_LIB_DIR=/home/bouncer/oda/teighaLinuxGCC${gcc_version}/lib/lnxX64_${gcc_version}dll
ENV ODA_CSV_LOCATION=/home/bouncer/oda/teighaLinuxGCC${gcc_version}/bin/lnxX64_${gcc_version}dll
ENV THRIFT_ROOT=/home/bouncer/thrift-0.12.0/
ENV SYNCHRO_READER_ROOT=/home/bouncer/3drepoSynchroReader
ENV REPOBOUNCER_ROOT=/home/bouncer/bouncer_install
ENV REPO_RVIT_TEXTURES=/home/bouncer/revit_materials_edited
ENV LD_LIBRARY_PATH=$SYNCHRO_READER_ROOT/lib:$THRIFT_ROOT/lib/:$IFCOPENSHELL_ROOT/lib:$MONGO_ROOT/lib/:$ASSIMP_ROOT/lib:/usr/local/lib:$ODA_ROOT/bin/lnxX64_${gcc_version}dll/:/home/bouncer/bouncer_install/lib
ENV SYNCHRO_PLUGIN_LOCATION=/home/bouncer/3drepobouncer/bin/plugins
ENV CRYPTOLENS_ROOT=/home/bouncer/cryptoLens

ARG LICENSE_PRODUCT_ID
ENV LICENSE_PRODUCT_ID=${LICENSE_PRODUCT_ID}

ARG LICENSE_AUTH_TOKEN
ENV LICENSE_AUTH_TOKEN=${LICENSE_AUTH_TOKEN}

ARG LICENSE_RSA_PUB_KEY_MOD  
ENV LICENSE_RSA_PUB_KEY_MOD ${LICENSE_RSA_PUB_KEY_MOD}

ARG LICENSE_RSA_PUB_KEY_EXP=AQAB
ENV LICENSE_RSA_PUB_KEY_EXP ${LICENSE_RSA_PUB_KEY_EXP}

ENV DATE_ROOT=/home/bouncer/date

# Install all the libraries from tests suite.
WORKDIR /home/bouncer/
RUN git clone --filter=blob:none --sparse https://${GITHUB_USER}:${GITHUB_TOKEN}@github.com/3drepo/tests.git tests && \
	cd tests && \
	git sparse-checkout set cplusplus/bouncer/ext_libs/focal cplusplus/bouncer/ext_libs/noble && \
	cd .. && \
	mv tests/cplusplus/bouncer/ext_libs/focal . && \
	mv tests/cplusplus/bouncer/ext_libs/noble . && \
        tar xzvf noble/3drepoSynchroReader.tgz && \
        tar xzvf noble/assimp-5.4.3.tgz && \
        tar xvzf noble/cryptoLens.tgz && \
        tar xzvf focal/date.tgz && \
        tar xzvf noble/IfcOpenShell-0.8.0.tgz && \
        tar xzvf noble/mongo-cxx-driver.tgz && \
        tar xzvf noble/thrift-0.12.0.tgz && \
        chown -R bouncer:bouncer /home/bouncer && \
        mkdir -p /home/bouncer/oda && \
        chown -R bouncer:bouncer /home/bouncer/oda && \
        cat focal/teigha/2026.9/teighaLinuxGCC8.3.tgz* | tar -C /home/bouncer/oda -xzvf - && \
        rm -rf tests && rm -rf focal && rm -rf noble

USER root
CMD ["/bin/bash"]

FROM core AS build

ARG REBUILD_DATE=unknown
ARG LICENSE_CHECK
ENV LICENSE_CHECK=${LICENSE_CHECK}
ARG app_3drepobouncer_version
ARG DSYNCHRO_SUPPORT=on
ENV DSYNCHRO_SUPPORT ${DSYNCHRO_SUPPORT:-on}

#compile 3drepobouncer libraries and util fetch version from workflow variable
RUN echo -e "machine github.com\n login $GITHUB_USER\n password $GITHUB_TOKEN" >~/.netrc && \
        git clone --recurse-submodules -b ${app_3drepobouncer_version} https://3drepoinstaller:${GITHUB_TOKEN}@github.com/3drepo/3drepobouncer.git /home/bouncer/3drepobouncer && \
        mkdir -p /home/bouncer/3drepobouncer/log && \
        chown bouncer:bouncer /home/bouncer/3drepobouncer && \
        mkdir -p /home/bouncer/3drepobouncer/build && \
        cd /home/bouncer/3drepobouncer/build && \
        if [ "$LICENSE_CHECK" -gt 0 ]; then FLAG=ON; else FLAG=OFF; fi && \
        cmake \
              -DCMAKE_CXX_COMPILER=$CXX \
              -DCMAKE_CXX_FLAGS=-Wno-deprecated-declarations -Wno-dev \
              -DODA_SUPPORT=ON \
              -DLICENSE_CHECK=${FLAG} \
              -DSYNCHRO_SUPPORT=${DSYNCHRO_SUPPORT} \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX=/home/bouncer/bouncer_install ../ && \
        make -j8 install && \
        chown -R bouncer:bouncer /home/bouncer/3drepobouncer && \
        chmod -R 555 /home/bouncer/3drepobouncer

# package for release
WORKDIR /home/bouncer/
RUN     cd /home/bouncer/3drepobouncer/tools/release && \
        node package_release.js /home/bouncer/bouncer_package 

USER root
CMD ["/bin/bash"]

FROM init AS deploy-base-core
COPY --from=build /home/bouncer/bouncer_package /home/bouncer/3drepobouncer/bin/
COPY --from=build /home/bouncer/3drepobouncer/tools/bouncer_worker /home/bouncer/3drepobouncer/tools/bouncer_worker

FROM deploy-base-core AS deploy
ARG LICENSE_CHECK=0
ENV LICENSE_CHECK ${LICENSE_CHECK}
ENV LD_LIBRARY_PATH=/home/bouncer/3drepobouncer/bin/
ENV ODA_CSV_LOCATION=/home/bouncer/3drepobouncer/bin/
ENV SYNCHRO_PLUGIN_LOCATION=/home/bouncer/3drepobouncer/bin/plugins

ARG RSAKEY
ENV RSAKEY ${RSAKEY}

# Replace Inkscape with pdf2svg
RUN apt-get update && \
    apt-get install -y \
    pdf2svg \
    openssh-server \
    xterm \
    xvfb && \
    rm -rf /var/lib/apt/lists/*

#REVIT MATERIALS
WORKDIR /tmp
ARG GITHUB_TOKEN=xxx
ARG GITHUB_USER=xxx
RUN git clone --filter=blob:none --sparse https://${GITHUB_USER}:${GITHUB_TOKEN}@github.com/3drepo/tests.git tests && \
        cd tests && \
        git sparse-checkout set cplusplus/bouncer/ext_libs/focal && \
        mkdir -p /home/bouncer/revit_materials_edited && \
        chown bouncer:bouncer /home/bouncer/revit_materials_edited  && \
        unzip /tmp/tests/cplusplus/bouncer/ext_libs/focal/revit_materials_edited.zip -d /home/bouncer  && \
        chown -R bouncer:bouncer /home/bouncer/revit_materials_edited && \
        rm -rf /tmp/tests/

#SET PERMISSIONS
WORKDIR /tmp
RUN chown bouncer:bouncer /home/bouncer/3drepobouncer && \
        chown bouncer:bouncer /home/bouncer && \
        chown -R bouncer:bouncer /home/bouncer/3drepobouncer/tools

WORKDIR /home/bouncer/3drepobouncer/tools/bouncer_worker
RUN yarn install && yarn cache clean

#swap back to root to mount NFS
USER root
RUN wget https://github.com/kelseyhightower/confd/releases/download/v0.16.0/confd-0.16.0-linux-amd64 && \
        mv confd-0.16.0-linux-amd64 /usr/local/bin/confd && \
        chmod +x /usr/local/bin/confd 

COPY ./config /etc/confd

COPY ./src/init.sh /usr/local/bin/start 

USER root
WORKDIR /home/bouncer
ENTRYPOINT ["start"]
CMD ["bouncer"]
