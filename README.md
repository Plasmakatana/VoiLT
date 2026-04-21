# VoiLT
HTTP server written purely in C language. Lightning fast, reliable and portable.
## Features
- Cross platform: Linux, Windows, MacOS, Android
- Highly concurrent: A mobile device is enough to serve thousands
- Load Balancing for multiple servers using RoundRobin algorithm
## Getting started
### LINUX
Install dependencies
```bash
#Arch
pacman -S zlib cmake git base-devel rust
#Debian/Ubuntu
apt install build-essential cmake libz-dev rust
```
Similar for other distros
- Clone the repository
```bash
git clone https://github.com/Plasmakatana/VoiLT.git
```
Build
```bash
cd server
mkdir build && cd build
cmake ..
make $(nproc)
```

### Windows
- Navigate to the provided dockerfile inside ```server/``` folder to run a container (WSL or HyperV) of the webserver
- Type the following commands to build and run container:
```bash
docker build -t voilt-server .
docker run --rm -p 8080:8080 voilt-server
```

### MacOS
- Dependencies:
```bash
brew install cmake rust
```
- Follow same build process as Linux
### Android
- Install termux
- Follow the same steps as linux build-procedure
NOTE:For Android(Termux) server, set affinity for specific cores allowed by android like shown in the source-code and then build
## Usage
- Navigate to directory containing VoiLT executable binary
- Type:
```bash
./VoiLT ../server.conf
```
add ```-d``` for dev mode
- If u are on the load-balancing server, navigate to ```reverse-proxy/``` folder
- After configuring upstream IP hosts in ```reverse-proxy/src/main.rs```, run load-balancer+rate-limiter service using:
```bash
cargo run
```
## Note
The servers, by default run on port ```8080``` and the load-balancer, by default runs on port ```10000```
You can change these by configuring ```server/server.conf``` and ```reverse-proxy/src/main.rs```
## Usage (Releases)
- Download latest release for your preferred platform
- Extract the release to a folder
- if ```run.sh``` is not executable, type 
  ```bash
  sudo chmod +x run.sh
  ```
  similar for all other ```*.sh``` files
- Configure ```reverseproxy/upstreams.conf``` to include all services running on your local network
- Execute 
```bash
./run.sh
```
- This will automatically start your local server on port 8080, load balancer on port 10000 and will create a cloudflare tunnel to enable TLS based encryption on a publically available domain provided by cloudflare
- To use your own domain use Stunnel, NGINX or other similar services
- You now have a TLS-complete website running!
- If you want to run only the webserver locally, navigate to ```/server/``` and then execute
```bash
run_server.sh
```
- If you want to run only load-balancer locally, navigate to ```/loadbalancer/``` and then execute
```bash
run_proxy.sh
```
after configuring the upstreams.conf file for your own network's running webservers
- Modify and tweak ```run.sh```, ```run_server.sh```& ```run_proxy.sh``` to run any combination of the 3 services (http, load-balancer, tunnel)
  
## Future plans:
- Support for services other than HTTP
- WASM version
