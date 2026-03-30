# VoiLT
HTTP server written purely in C language. Lightning fast, reliable and portable.
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
