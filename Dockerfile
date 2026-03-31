# ---------- BUILD STAGE ----------
FROM archlinux:base AS builder
# Avoid interactive pacman
ENV TERM=xterm-256color
ENV LANG=C.UTF-8
# Install build requirements: git + build tools + cmake
RUN pacman -Sy --noconfirm \
    && pacman -S --noconfirm git base-devel cmake zlib ninja pkgconf \
    && pacman -Scc --noconfirm
# Create and switch to app directory
WORKDIR /VoiLT
# Clone your GitHub repository (replace URL with your own)
RUN git clone https://github.com/Plasmakatana/VoiLT.git .
# Create and move into build directory
RUN mkdir -p build
WORKDIR /VoiLT/build
# Run CMake & build
RUN cmake .. \
    && make -j$(nproc)

# ---------- RUNTIME STAGE ----------
FROM archlinux:base AS runtime
# Avoid interactive pacman
ENV TERM=xterm-256color
ENV LANG=C.UTF-8
# We only need minimal runtime dependencies
RUN pacman -Sy --noconfirm \
    && pacman -S --noconfirm libstdc++ \
    && pacman -Scc --noconfirm
# Create app directory
WORKDIR /VoiLT
# Copy built binary from builder
COPY --from=builder /VoiLT/static/* /VoiLT/static/
# Copy config if needed
COPY --from=builder /VoiLT/server.conf .
RUN chmod 664 ./static/*
COPY --from=builder /VoiLT/build/VoiLT /VoiLT/static/
# Expose port if needed (optional, e.g., 8080)
EXPOSE 8080
WORKDIR /VoiLT/static
# Run service
CMD ["./VoiLT","../server.conf","-d"]
