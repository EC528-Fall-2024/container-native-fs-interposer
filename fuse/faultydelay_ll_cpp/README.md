How to Use:

To build docker image:

>>docker build -t my-fuse-app .

Create, name, and run a container then enter a bash shell for container(fuse needs priveledged):

>>docker run -it --privileged -v /dev/fuse:/dev/fuse --name fuse-container  my-fuse-app bash

-this essentially expands into:
docker create --privileged -v /dev/fuse:/dev/fuse --name fuse-container my-fuse-app
docker start fuse-container
docker exec -it fuse-container bash

Once in container shell, to mount fs:
>>./faulty_ll mountpoint

I created testmount as an empty directory to test IO, this is where errorlog.txt is created:
>>cd mountpoint/usr/src/myapp/testmount

-To exit shell: ctrl+d

If exited container and want to re enter shell:
docker start fuse-container
docker exec -it fuse-container bash

To remove container:
docker stop fuse-container && docker rm fuse-container
OR
docker rm fuse-container (if container was already stopped like after exiting shell)

List images: docker images

List running containers: docker ps

List all containers: docker ps --all

Compile File System in container: g++ -Wall -D_FILE_OFFSET_BITS=64 faulty_ll.cpp -I/usr/local/include -L/usr/local/lib -lopentelemetry_trace -lopentelemetry_resources -lopentelemetry_exporter_ostream_span -lopentelemetry_common `pkg-config fuse3 --cflags --libs` -o faulty_ll

Faulty Filesystem Configurables:

FAILRATE: likelihood of failure = 1/failrate
CONFIGSEED: 0 = use random seed; 1 = use default or user-defined seed
SEEDNUM: if CONFIGSEED = 1, use SEEDNUM as seed
DELAYTIME: Seconds for simulated delay