** **

# Container Native FS Interposer

**Team Members**: Julia Hua, Jiawei Xiang, Hilario Gonzalez, Shamir Legaspi, Juncheng Cao  
**Mentor**: Vasily Tarasov, Alex Merenstein  

## Project Description
The goal of this project is to integrate the FUSE library, which provides a mechanism and API for implementing file systems in userspace, natively on containers deployed in Kubernetes, which are often used to deploy cloud-based applications. One application of the FUSE library is to create utility file systems that provide testing applications. 

Our project consists of the following components:
1) Create a CSI plugin for Kubernetes that allows mounting a stackable FUSE-based file system over another file system
2) Implement utility FUSE file systems
3) Run experiments with several data-intensive applications using the CSI plugin and utility FUSE file systems

## 1.   Vision and Goals Of The Project:

Our project's vision is to allow users to mount file systems with various testing utilities in a Kubernetes application. These utilities include workload tracing, workload metric collection, faulty I/O, throttle I/O, and fake I/O.

## 2. Users/Personas Of The Project:

### Cluster administrator at MOC - Steven

- Background and Role: Steven is the cluster admin of the MOC Openshift responsible for the performance and reliability of the share file systems.

- Needs and Goals: Steven wants to make sure all the applications running on the cluster still works fine during these monthly maintenances, since there can be degredation in latency/bandwidth during these events. And want to minimize the impact on these applications by choosing the best maintenance windows. 

- Challenges: Steven don't have enough visibility into the filesystem access patterns of the applications.

### Researcher at BU - Nancy

- Background and Role: Nancy is a researcher at BU ECE, her research is on AI.

- Needs and Goals: During the recent training sessions the GPU server she manages is no longer of enough capacity, she is thinking of outsourcing the training to the cloud, but she is worried about the training performance since the storage latency is usually worse on the cloud. She want to confirm the performance numbers before actually signing the contract.

- Challenges: Nancy needs a emulated cloud environment with arbitrarily worse storage performance to test things out.

### Developer at a large business - John

- Background and Role: John is the CTO of a grocery store called missing the target, the company has a website for online ordering.

- Needs and Goals: John is worried about the stability of the website in case of filesystem failures because he has been cutting corners and buying second handed hard drives, he want the website to be still accessible during failures, maybe in a degraded mode.

- Challenges: He wants to employ the idea of chaos enginnering, and test fault tolerance of the system.

** **

## 3.   Scope and Features Of The Project:

### CSI
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
- passthru
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

### Global Architectural Structure Of the Project

FUSE library provides a mechanism and an API for implementing full-fledged file systems in user space, which makes developing a new file system significantly easier and safer compared to kernel based file systems. This also enables quick development of various “utility” file systems that layer on top of other file systems to add new functionalities.  For example, a file system that injects random errors to evaluate applications’ error handling, a file system that logs activity for later analysis or playback, or a file system that throttles I/O operations for quality of service (QoS) purposes or reducing the load on backend.

Applications are now often deployed in container native environments, such as Kubernetes.  In Kubernetes, storage is provided to workloads (“pods”) via volumes (“persistent volumes, PVs”) that are usually formatted with a file system such as ext4 or xfs.  To use a FUSE-based stackable utility file system with these volumes and workloads, some integration with Kubernetes is required, e.g. a special CSI plugin.

### TODO: insert diagram

### Design Implications and Discussion

1. FUSE itself may introduce too much noise/overhead.
2. CSI filesystems stacking would be a nice addition.

## 5. Acceptance criteria

1. Create a new CSI plugin for Kubernetes that allows users to mount a stackable FUSE-based file system over another file system. Many open-source CSI plugins exist and can be used as a reference. CSI plugins are usually implemented using Go language.

2. Implement one or more (depending on the student group size) utility FUSE file systems (basing them on existing passthrough) that do: workload tracing, workload metric collection, faulty I/O, throttle I/O, fake IO. The details of each file system will be discussed with the mentors.

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
- Mount the passthrough FUSE filesystem.

3. Sprint 3
Implement new utility FUSE filesystems for throttling/fault injection/etc., integrate the FUSE filesystems with CSI.

4. Sprint 4
- Implement tracing and metrics with OTEL
- Implement a dashboard for visualizing these signals

5. Sprint 5
- Run experiments with data-intensive application

** **



