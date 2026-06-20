# syntax=docker/dockerfile:1
# ============================================================
# vote-backend Containerfile
# Multi-stage build for a rootless Podman container
# ============================================================

# ----------------------------------------------------------
# Stage 1 – Build
# ----------------------------------------------------------
FROM docker.io/silkeh/clang:21-trixie AS builder

RUN <<EOF cat > /etc/apt/sources.list.d/debian-backports.sources
Types: deb deb-src
URIs: http://deb.debian.org/debian
Suites: trixie-backports
Components: main
Enabled: yes
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg
EOF

# Install build tools and runtime dependencies
RUN apt-get update && apt-get install -t trixie-backports -y --no-install-recommends \
    git \
    curl \
    zip \
    unzip \
    tar \
    cmake \
    ninja-build \
    pkg-config \
    libssl-dev \
    libpq-dev \
    zlib1g-dev \
    lzip \
    bison \
    flex \
    && rm -rf /var/lib/apt/lists/*


WORKDIR /src

# Install vcpkg
ARG VCPKG_ROOT=/src/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh

# Copy dependency manifest first (layer caching)
COPY vcpkg.json CMakeLists.txt CMakePresets.json ./
COPY sql/ ./sql/
COPY src/ ./src/
COPY include/ ./include/
COPY ports/ ./ports/
COPY tests/ ./tests/

# Build release binary with clang
ENV CXX=clang++
ENV VCPKG_DEFAULT_BINARY_CACHE=/vcpkg-bincache

RUN --mount=type=cache,target=/src/vcpkg/downloads \
    --mount=type=cache,target=/vcpkg-bincache \
    cmake --preset ninja-multi-vcpkg \
    && cmake --build --preset ninja-vcpkg-release

# ----------------------------------------------------------
# Stage 2 – Runtime
# ----------------------------------------------------------
FROM docker.io/library/debian:trixie-slim AS runtime

LABEL org.opencontainers.image.title="vote-backend" \
      org.opencontainers.image.description="RESTful voting backend built with Drogon" \
      org.opencontainers.image.source="https://github.com/peschke/vote-backend"

# Install only runtime libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    libssl3 \
    zlib1g \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for the container
RUN groupadd --gid 1001 appuser \
    && useradd --uid 1001 --gid appuser --shell /bin/false --create-home appuser

# Configuration directory
ARG CONF_DIR=/etc/vote
RUN mkdir -p ${CONF_DIR} && chown appuser:appuser ${CONF_DIR}

ARG STATIC_DIR=/static
RUN mkdir -p ${STATIC_DIR} && chown appuser:appuser ${STATIC_DIR}

# Copy release binary from builder
COPY --from=builder /src/builds/ninja-multi-vcpkg/Release/main /usr/local/bin/vote-backend

# Copy default configuration
COPY config.json ${CONF_DIR}/config.json

# Drop privileges
USER appuser

# Expose the application port
EXPOSE 8848

# Health check
HEALTHCHECK --interval=30s --timeout=5s --retries=3 \
    CMD curl -f http://localhost:8848/health || exit 1

# Allow the config path to be overridden at runtime
ENV VOTE_BACKEND_CONFPATH=${CONF_DIR}

ENTRYPOINT ["vote-backend"]
