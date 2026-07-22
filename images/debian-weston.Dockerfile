ARG DEBIAN_VERSION=trixie

FROM debian:${DEBIAN_VERSION}

ARG WESTON_URL
ARG XWAYLAND_URL
ARG MESA_URL

RUN set -eux; \
    export DEBIAN_FRONTEND=noninteractive; \
    : "${WESTON_URL:?WESTON_URL must be provided}"; \
    : "${XWAYLAND_URL:?XWAYLAND_URL must be provided}"; \
    : "${MESA_URL:?MESA_URL must be provided}"; \
    apt update; \
    apt install -y --no-install-recommends \
        apt-transport-https \
        bash-completion \
        ca-certificates \
        curl \
        dbus-x11 \
        libegl-mesa0 \
        libgbm1 \
        libgl1-mesa-dri \
        libglx-mesa0 \
        locales \
        mesa-libgallium \
        mesa-vulkan-drivers \
        pipewire-audio \
        procps \
        sudo \
        unzip \
        weston \
        xwayland; \
    curl --fail --location --show-error --output /tmp/xwayland.deb "${XWAYLAND_URL}"; \
    apt reinstall -y --allow-downgrades /tmp/xwayland.deb; \
    curl --fail --location --show-error --output /tmp/weston.zip "${WESTON_URL}"; \
    mkdir -p /tmp/weston-debs-install; \
    unzip -q /tmp/weston.zip -d /tmp/weston-debs-install; \
    test -n "$(find /tmp/weston-debs-install -type f -name '*.deb' -print -quit)"; \
    find /tmp/weston-debs-install -type f -name '*.deb' \
        -exec apt reinstall -y --allow-downgrades {} +; \
    curl --fail --location --show-error --output /tmp/mesa.tar.gz "${MESA_URL}"; \
    tar -zxvf /tmp/mesa.tar.gz -C /; \
    apt-mark hold \
        xwayland \
        weston \
        libweston-14-0 \
        libegl-mesa0 \
        libgbm1 \
        libgl1-mesa-dri \
        libglx-mesa0 \
        mesa-libgallium \
        mesa-vulkan-drivers; \
    apt clean; \
    rm -rf \
        /tmp/xwayland.deb \
        /tmp/weston.zip \
        /tmp/weston-debs-install \
        /tmp/mesa.tar.gz \
        /var/lib/apt/lists/*

RUN install -d -m 0755 /opt/lfdevs/anland

COPY --chmod=0755 scripts/startweston-anland.sh /opt/lfdevs/anland/startweston-anland

RUN printf '%s\n' 'export PATH="/opt/lfdevs/anland:${PATH}"' \
    > /etc/profile.d/anland-path.sh \
    && chmod 0644 /etc/profile.d/anland-path.sh

ENV PATH="/opt/lfdevs/anland:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
