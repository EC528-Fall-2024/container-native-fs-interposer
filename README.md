** **
## Final Presentation [Here](https://drive.google.com/file/d/1pZEh2ZCTQqHuE2QEdGVAztjpCRspdbd6/view?usp=sharing)
## Sprint 5 Presentation [Here](https://drive.google.com/file/d/1orbwBaVa9WKAZwWxqHgIzPNDnG8HWYuJ/view?usp=sharing)
## Sprint 4 Presentation [Here](https://drive.google.com/file/d/16IoPRz-5V4tFpbQp9j9vrLVQzbS8WNiE/view?usp=sharing)
## Sprint 3 Presentation [Here](https://drive.google.com/file/d/1H5nYOQyf8Jh2MvwgTy0MzZMQaYOPkJl2/view?usp=sharing)
## Sprint 2 Presentation [Here](https://drive.google.com/file/d/1uCAigyRg6A2l4iw9vFUE9lcXuRVmmxaq/view?usp=sharing)
## Sprint 1 Presentation [Here](https://drive.google.com/file/d/181ri_21EBSxvOOHjBrGhNJPuYTLsopJ7/view?usp=sharing)


# Container Native FS Interposer

**Team Members**: Julia Hua, Jiawei Xiang, Hilario Gonzalez, Shamir Legaspi, Juncheng Cao  
**Mentor**: Vasily Tarasov, Alex Merenstein  

## Installation

### Prerequisites

#### 1. Kubernetes Cluster
A kubernetes cluster which you have admin acccess to is required. It's recommended to create a test cluster with [kind](https://github.com/kubernetes-sigs/kind) if you don't have one readily available.

```shell
kind create cluster
```

#### 2. Nix (optional)
Nix is the build system we use for building the FUSE fileystem and the CSI plugin and bundling everything into a container image. It can be installed on any linux distribution with the following command:

```shell
sh <(curl -L https://nixos.org/nix/install) --daemon
```

If you choose not to install nix, you can pull the prebuilt image from ghcr.io.

```shell
docker login ghcr.io # login using your github username and personal access token
docker pull ghcr.io/ec528-fall-2024/container-native-fs-interposer/csi-plugin:v0.0.5
docker tag ghcr.io/ec528-fall-2024/container-native-fs-interposer/csi-plugin:v0.0.5 docker.io/library/csi-node:latest
```

#### 3. helm and helmfile
We use [helm](https://helm.sh/) and [helmfile](https://helmfile.readthedocs.io/en/latest/) for deploying our project as well as its depenencies into kubernetes, please follow their respective installation guides to install helm and helmfile: [Installing Helm](https://helm.sh/docs/intro/install/), [Helmfile Installation](https://helmfile.readthedocs.io/en/latest/#installation). Since we already have nix installed, an alternative way is to use the following command:

```shell
nix profile install nixpkgs#helm nixpkgs#helmfile
```

### Build Container Image
The container image can be built with the following commands, after which the image tarball would be placed at `/tmp/csi-node.tar`, if you've choosen to pull the prebuilt image, please skip this step:

```shell
nix build .#csi-node
./result > /tmp/csi-node.tar
```

After building the image, it has to be loaded into the kubernetes cluster, if you are using kind, the following command does it for you:

```shell
kind load image-archive /tmp/csi-node.tar
# or below for prebuilt image
kind load docker-image docker.io/library/csi-node:latest
```

### Deploy
The project together with its dependencies can be deployed into the cluster with a single command:

```shell
helmfile apply
```

The command would take a while.


### Run Example Workload
Example workloads are provided in the `examples` folder, they can be run with kubectl:

```shell
# run individual examples
kubectl apply -f examples/minio.yaml
# run all of them
kubectl apply -k examples/
```

Progress of the job can be checked with:

```shell
kubectl get jobs.batch
# or watch their progress
kubectl get jobs.batch --watch
```

And the following commands can be used to forward the jaeger and grafana dashboard to a local port:

```shell
# grafana
kubectl -n csi-interposer port-forward services/grafana-service 3000:3000
# jaeger
kubectl -n csi-interposer port-forward services/jaeger-default-query 16686:16686
# prometheus
kubectl -n prometheus port-forward services/prometheus-server 8080:80
```

## Project Description

## 1.   Vision and Goals Of The Project:

The goal of our project is to integrate FUSE file systems within the container-native environment Kubernetes, which is commonly used for deploying cloud-based applications. By leveraging the FUSE library, which provides the mechanisms and API for implementing file systems in user space, we aim to develop file systems with various testing utilities for Kubernetes applications. These utilities include workload tracing, workload metric collection, faulty I/O, and throttle I/O.

## 2. Users/Personas Of The Project:

The users of this project is anyone who is developing in or managing a cloud environment and need a file system that evaluates and tests the workload and its performance. 

### Cluster administrator at MOC - Steven

- Background and Role: Steven is the cluster admin of the MOC Openshift responsible for the performance and reliability of the share file systems.

- Needs and Goals: Steven wants to make sure all the applications running on the cluster still works fine during these monthly maintenances, since there can be degredation in latency/bandwidth during these events. He wants to minimize the impact on these applications by choosing the best maintenance windows. 

- Challenges: Steven doesn't have enough visibility into the filesystem access patterns of the applications.

### Researcher at BU - Nancy

- Background and Role: Nancy is a researcher at BU ECE, and her research is on AI.

- Needs and Goals: During the recent training sessions, the GPU server she manages is no longer of enough capacity. She is thinking of outsourcing the training to the cloud but is worried about the training performance since the storage latency is usually worse on the cloud. She wants to confirm the performance numbers before actually signing the contract.

- Challenges: Nancy needs an emulated cloud environment with arbitrarily worse storage performance to test things out.

### Developer at a large business - John

- Background and Role: John is the CTO of a grocery store called "Missing the Target". The company has a website for online ordering.

- Needs and Goals: John is worried about the stability of the website in case of filesystem failures because he has been cutting corners and buying second handed hard drives. He wants the website to still be accessible during failures, maybe in a degraded mode.

- Challenges: He wants to employ the idea of chaos enginnering and test fault tolerance of the system.

** **

## 3.   Scope and Features Of The Project:

### CSI Plugin
#### in-scope
- Node Controller
- Ephemeral Volumes
- ReadWriteOncePod
#### out-of-scope
- Cluster Controller
- Persistent Volumes
- ReadWriteOnce/ReadOnlyMany/ReadWriteMany

### FUSE
#### in-scope
- passthrough
- random fault
- throttle bandwidth
- random/fixed delay
- tracing with filtering
- metrics/IO accouting
- in memory fs
- /dev/null
#### out-of-scope
- persistent storage 
- metadata randomization
- quota

### Tracing/Metrics
#### in-scope
- latency
- thoughput
- failure rate
- dashboard
#### out-of-scope
- analysis

** **

## 4. Solution Concept

Applications are now often deployed in container-native environments, such as Kubernetes. Often, developers would like to observe and test the application through its underlying file system. This project entails wrapping that underlying file system with utility layers implemented with FUSE. The utilities provide observability, fault injection, and throttling the I/O.

In Kubernetes, storage is provided to workloads (“pods”) via volumes (“persistent volumes, PVs”) that are usually formatted with a file system such as ext4 or xfs.  To use a FUSE-based stackable utility file system with these volumes and workloads, some integration with Kubernetes is required. This integration will be implemented by a CSI node plugin.

The FUSE library provides a mechanism and an API for implementing full-fledged file systems in user space, which makes developing a new file system significantly easier and safer compared to kernel based file systems. This also enables quick development of various “utility” file systems that layer on top of other file systems to add new functionalities.  For example, a file system that injects random errors to evaluate applications’ error handling, a file system that logs activity for later analysis or playback, or a file system that throttles I/O operations for quality of service (QoS) purposes or reducing the load on backend. A user request (i.e., system call) would travel from the userspace to the virtual file system (VFS) and subsequently the FUSE kernel module. FUSE then forwards these I/O requests to the handler, which is a utility FUSE implementation.

### Global Architectural Structure of the Project

The following diagram shows the general architecture of our project. The application, deployed in Kubernetes, would send I/O requests to the interposer binary, our FUSE file system implementation, which then forwards that request to the backing volume. The interposer binary takes in a configuration file that allows the user to enable or disable four utilities: workload tracing, metric collection, faulty I/O, and throttle I/O.
The binary is instrumented with OpenTelemetry's Metrics and Traces API. The metrics and traces, if enabled in the configuration file, are exported to and collected by a Promeetheus and Jaeger backend, respectively. Finally, the data is displayed in a Grafana dashboard. 

<p align="center">
<img src="./images/architecture.png" width="50%">
</p>
<p align="center">
Diagram 1: General Architecture of Project
</p>

#### Architecture of the CSI plugin

CSI, or Container Storage Interface, defines a set of interfaces that allows storage systems to expose their services to containerized environments. A CSI plugin is an implementation of the CSI that enable containers to provision and mount volumes. This plugin allows the application deployed in Kubernetes to interface with our FUSE utility file system.

##### Generic CSI Architecture
![](./images/csi-arch-rev1.png)

##### CSI sidecar Architecture to allow volume stacking
![](./images/csi-sidecar.png)

### Design Discussion

1. FUSE itself may introduce too much noise/overhead. This will be evaluated by running expriments with data-intensive applications.
2. CSI filesystems stacking would be a nice addition, if time allows.

## 5. Acceptance criteria

1. Create a new CSI plugin for Kubernetes that allows users to mount a stackable FUSE-based file system over another file system.

2. Implement utility FUSE file systems that provide workload tracing, workload metric collection, faulty I/O, and throttle I/O.

3. Run experiments with several data-intensive applications using the 2 technologies above. Perform descriptive analysis of applications’ behavior when a utility file system is used.

## 6.  Release Planning:

1. Sprint 1
Research existing projects, learn k8s, grafana, prometheus, Go, C and FUSE.

Links for reference:
- https://kubernetes.io/docs/concepts/storage/storage-classes/
- https://kubernetes.io/docs/concepts/storage/ephemeral-volumes/
- https://kubernetes-csi.github.io/docs/introduction.html
- https://chaos-mesh.org/docs/simulate-io-chaos-on-kubernetes/
- https://github.com/chaos-mesh/toda
- https://grafana.com/
- https://opentelemetry.io/
- https://prometheus.io/

Artifacts:
- Diagrams for the system architecture
- A test kubernetes cluster with example CSI plugins/monitoring stack installed

2. Sprint 2
- Create scaffold for the CSI plugin
- Mount the passthrough FUSE filesystem

3. Sprint 3
- Implement new utility FUSE filesystems for throttling/fault injection/etc.
- Integrate the FUSE filesystems with CSI

4. Sprint 4
- Implement tracing and metrics with OTEL
- Implement a dashboard for visualizing these signals

5. Sprint 5
- Run experiments with data-intensive application

** **



