# VoiLT
HTTP server written purely in C language. Lightning fast, reliable and portable.
## Features
- Cross platform: Linux, Windows, MacOS, Android
- Highly concurrent: A mobile device is enough to serve thousands
## Getting started
Clone the repository
```bash
git clone https://github.com/Plasmakatana/VoiLT.git
```
Build
```bash
mkdir build && cd build
cmake ..
make $(nproc)
```
NOTE:For android(Termux) server, set affinity for specific cores allowed by android like shown in the source-code and then build
## Usage
- Navigate to directory containing VoiLT executable binary
- Type:
```bash
./VoiLT ../server.conf
```
add ```-d``` for dev mode
## Future plans:
- Support for services other than HTTP
- SSL/TLS
- WASM version
