** **

## Project Description Template

The purpose of this Project Description is to present the ideas proposed and decisions made during the preliminary envisioning and inception phase of the project. The goal is to analyze an initial concept proposal at a strategic level of detail and attain/compose an agreement between the project team members and the project customer (mentors and instructors) on the desired solution and overall project direction.

This template proposal contains a number of sections, which you can edit/modify/add/delete/organize as you like.  Some key sections we’d like to have in the proposal are:

- Vision: An executive summary of the vision, goals, users, and general scope of the intended project.

- Solution Concept: the approach the project team will take to meet the business needs. This section also provides an overview of the architectural and technical designs made for implementing the project.

- Scope: the boundary of the solution defined by itemizing the intended features and functions in detail, determining what is out of scope, a release strategy and possibly the criteria by which the solution will be accepted by users and operations.

Project Proposal can be used during the follow-up analysis and design meetings to give context to efforts of more detailed technical specifications and plans. It provides a clear direction for the project team; outlines project goals, priorities, and constraints; and sets expectations.

** **

## 1.   Vision and Goals Of The Project:

The vision section describes the final desired state of the project once the project is complete. It also specifies the key goals of the project. This section provides a context for decision-making. A shared vision among all team members can help ensuring that the solution meets the intended goals. A solid vision clarifies perspective and facilitates decision-making.

The vision statement should be specific enough that you can look at a proposed solution and say either "yes, this meets the vision and goals", or "no, it does not".

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

The Scope places a boundary around the solution by detailing the range of features and functions of the project. This section helps to clarify the solution scope and can explicitly state what will not be delivered as well.

It should be specific enough that you can determine that e.g. feature A is in-scope, while feature B is out-of-scope.

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

1. Create a new CSI plugin for Kubernetes that allows to mount a stackable FUSE-based file system over another file system. Many open-source CSI plugins exist and can be used as a reference. CSI plugins are usually implemented using Go language.

2. Implement one or more (depending on the student group size) utility FUSE file systems (basing them on existing passthrough) that do: workload tracing, workload metric collection, faulty I/O, throttle I/O, fake IO. The details of each file system will be discussed with the mentors.

3. Run experiments with several data-intensive applications using the 2 technologies above. Perform descriptive analysis of applications’ behavior when a utility file system is used.

## 6.  Release Planning:

Release planning section describes how the project will deliver incremental sets of features and functions in a series of releases to completion. Identification of user stories associated with iterations that will ease/guide sprint planning sessions is encouraged. Higher level details for the first iteration is expected.

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
Create scaffold for the CSI plugin, mounting the passthru FUSE filesystem.

3. Sprint 3
Implement new FUSE filesystems for throttling/fault injection/etc., integrate the FUSE filesystems with CSI.

4. Sprint 4
Implement tracing and metrics with OTEL, implement a dashboard for visualizing these signals.

5. Sprint 5
Run experiments with data-intensive application.

** **

## General comments

Remember that you can always add features at the end of the semester, but you can't go back in time and gain back time you spent on features that you couldn't complete.

** **

For more help on markdown, see
https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet

In particular, you can add images like this (clone the repository to see details):

![alt text](https://github.com/BU-NU-CLOUD-SP18/sample-project/raw/master/cloud.png "Hover text")


