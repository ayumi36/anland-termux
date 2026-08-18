ARG DEBIAN_VERSION=trixie

FROM debian:${DEBIAN_VERSION}

ARG MUTTER_URL
ARG XWAYLAND_URL
ARG MESA_URL

RUN set -eux; \
    export DEBIAN_FRONTEND=noninteractive; \
    : "${MUTTER_URL:?MUTTER_URL must be provided}"; \
    : "${XWAYLAND_URL:?XWAYLAND_URL must be provided}"; \
    : "${MESA_URL:?MESA_URL must be provided}"; \
    apt update; \
    apt install -y --no-install-recommends \
        apt-transport-https \
        bash-completion \
        ca-certificates \
        curl \
        dbus-x11 \
        gnome-core \
        gnome-shell \
        gnome-shell-extension-manager \
        gnome-tweaks \
        libegl-mesa0 \
        libgbm1 \
        libgl1-mesa-dri \
        libglx-mesa0 \
        locales \
        mutter \
        network-manager-gnome \
        pipewire-audio \
        pipewire-libcamera \
        procps \
        sudo \
        unzip \
        xfonts-base \
        xwayland \
        yaru-theme-gnome-shell \
        yaru-theme-gtk \
        yaru-theme-icon \
        yaru-theme-sound; \
    curl --fail --location --show-error --output /tmp/xwayland.deb "${XWAYLAND_URL}"; \
    apt reinstall -y --allow-downgrades /tmp/xwayland.deb; \
    curl --fail --location --show-error --output /tmp/mutter.zip "${MUTTER_URL}"; \
    mkdir -p /tmp/mutter-debs-install; \
    unzip -q /tmp/mutter.zip -d /tmp/mutter-debs-install; \
    test -n "$(find /tmp/mutter-debs-install -type f -name '*.deb' -print -quit)"; \
    find /tmp/mutter-debs-install -type f -name '*.deb' \
        -exec apt reinstall -y --allow-downgrades {} +; \
    curl --fail --location --show-error --output /tmp/mesa.tar.gz "${MESA_URL}"; \
    tar -zxvf /tmp/mesa.tar.gz -C /; \
    printf '%s\n' \
        'Package: xwayland libegl-mesa0 libgbm1 libgl1-mesa-dri libglx-mesa0 mesa-libgallium mesa-vulkan-drivers gir1.2-mutter-* libmutter-* mutter mutter-common mutter-common-bin' \
        'Pin: release *' \
        'Pin-Priority: -1' \
        > /etc/apt/preferences.d/hold-anland-package; \
    apt clean; \
    rm -rf \
        /tmp/xwayland.deb \
        /tmp/mutter.zip \
        /tmp/mutter-debs-install \
        /tmp/mesa.tar.gz \
        /var/lib/apt/lists/*

RUN install -d -m 0755 /opt/lfdevs/anland

COPY --chmod=0755 scripts/startgnome-anland.sh /opt/lfdevs/anland/startgnome-anland

RUN printf '%s\n' 'export PATH="/opt/lfdevs/anland:${PATH}"' \
    > /etc/profile.d/anland-path.sh \
    && chmod 0644 /etc/profile.d/anland-path.sh

ENV PATH="/opt/lfdevs/anland:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
