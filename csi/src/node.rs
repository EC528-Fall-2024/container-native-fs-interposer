use crate::csi::v1::node_server::Node;
use crate::csi::v1::*;
use k8s_openapi::{
    api::core::v1::{
        Affinity, Container, HostPathVolumeSource, Pod, PodAffinity, PodAffinityTerm, PodSpec,
        SecurityContext, Volume, VolumeMount,
    },
    apimachinery::pkg::apis::meta::v1::LabelSelector,
};
use kube::{
    api::{ObjectMeta, PartialObjectMetaExt, Patch, PatchParams, PostParams},
    Api, Client,
};
use nix::mount::MntFlags;
use std::{collections::BTreeMap, io::ErrorKind, path::Path, process::Command};
use tonic::{Request, Response, Status};
use uuid::Uuid;

pub struct NodeService {
    client: Client,
    node_id: String,
}

impl NodeService {
    pub async fn new(node_id: &str) -> Self {
        let client = Client::try_default().await.unwrap();
        Self {
            client,
            node_id: node_id.to_string(),
        }
    }
}

#[tonic::async_trait]
impl Node for NodeService {
    async fn node_stage_volume(
        &self,
        _: Request<NodeStageVolumeRequest>,
    ) -> Result<Response<NodeStageVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_unstage_volume(
        &self,
        _: Request<NodeUnstageVolumeRequest>,
    ) -> Result<Response<NodeUnstageVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_publish_volume(
        &self,
        request: Request<NodePublishVolumeRequest>,
    ) -> Result<Response<NodePublishVolumeResponse>, Status> {
        let request = request.into_inner();

        let pod_namespace = request
            .volume_context
            .get("csi.storage.k8s.io/pod.namespace")
            .unwrap();
        let pod_name = request
            .volume_context
            .get("csi.storage.k8s.io/pod.name")
            .unwrap();

        let pods: Api<Pod> = Api::namespaced(self.client.clone(), pod_namespace);
        let labels: BTreeMap<String, String> = [(
            "interposer.csi.example.com/affinity".to_string(),
            Uuid::new_v4().to_string(),
        )]
        .into();
        pods.patch_metadata(
            pod_name,
            &PatchParams::default(),
            &Patch::Merge(
                &ObjectMeta {
                    labels: Some(labels.clone()),
                    ..Default::default()
                }
                .into_request_partial::<Pod>(),
            ),
        )
        .await
        .unwrap();
        pods.create(
            &PostParams::default(),
            &Pod {
                metadata: ObjectMeta {
                    name: Some(format!("{}-interposer", pod_name)),
                    namespace: Some(pod_namespace.to_string()),
                    ..Default::default()
                },
                spec: Some(PodSpec {
                    affinity: Some(Affinity {
                        pod_affinity: Some(PodAffinity {
                            required_during_scheduling_ignored_during_execution: Some(vec![
                                PodAffinityTerm {
                                    label_selector: Some(LabelSelector {
                                        match_labels: Some(labels),
                                        ..Default::default()
                                    }),
                                    namespaces: Some(vec![pod_namespace.to_string()]),
                                    topology_key: "kubernetes.io/hostname".to_string(),
                                    ..Default::default()
                                },
                            ]),
                            ..Default::default()
                        }),
                        ..Default::default()
                    }),
                    containers: vec![Container {
                        command: Some(vec!["sleep".to_string(), "inf".to_string()]),
                        image: Some("busybox:latest".to_string()),
                        name: "interposer".to_string(),
                        security_context: Some(SecurityContext {
                            privileged: Some(true),
                            ..Default::default()
                        }),
                        volume_mounts: Some(vec![
                            VolumeMount {
                                mount_path: "/var/lib/kubelet/pods".to_string(),
                                mount_propagation: Some("Bidirectional".to_string()),
                                name: "mountpoint-dir".to_string(),
                                ..Default::default()
                            },
                            VolumeMount {
                                mount_path: "/dev".to_string(),
                                name: "dev-dir".to_string(),
                                ..Default::default()
                            },
                        ]),
                        ..Default::default()
                    }],
                    volumes: Some(vec![
                        Volume {
                            host_path: Some(HostPathVolumeSource {
                                path: "/var/lib/kubelet/pods".to_string(),
                                type_: Some("DirectoryOrCreate".to_string()),
                            }),
                            name: "mountpoint-dir".to_string(),
                            ..Default::default()
                        },
                        Volume {
                            host_path: Some(HostPathVolumeSource {
                                path: "/dev".to_string(),
                                type_: Some("Directory".to_string()),
                            }),
                            name: "dev-dir".to_string(),
                            ..Default::default()
                        },
                    ]),
                    ..Default::default()
                }),
                ..Default::default()
            },
        )
        .await
        .unwrap();

        match std::fs::create_dir(&request.target_path) {
            Err(err) if err.kind() == ErrorKind::AlreadyExists => (),
            result => result?,
        };

        // FIXME: check if a filesystem is already mounted
        Command::new("basic_passthrough")
            .args([&request.target_path])
            .spawn()?;

        Ok(Response::new(NodePublishVolumeResponse {}))
    }
    async fn node_unpublish_volume(
        &self,
        request: Request<NodeUnpublishVolumeRequest>,
    ) -> Result<Response<NodeUnpublishVolumeResponse>, Status> {
        let request = request.into_inner();

        // FIXME: cleanup the fuse process
        nix::mount::umount2(Path::new(&request.target_path), MntFlags::empty())
            .map_err(|err| Status::internal(err.to_string()))?;

        std::fs::remove_dir_all(request.target_path)?;
        Ok(Response::new(NodeUnpublishVolumeResponse {}))
    }
    async fn node_get_volume_stats(
        &self,
        _: Request<NodeGetVolumeStatsRequest>,
    ) -> Result<Response<NodeGetVolumeStatsResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_expand_volume(
        &self,
        _: Request<NodeExpandVolumeRequest>,
    ) -> Result<Response<NodeExpandVolumeResponse>, Status> {
        Err(Status::unimplemented("method not supported"))
    }
    async fn node_get_capabilities(
        &self,
        _: Request<NodeGetCapabilitiesRequest>,
    ) -> Result<Response<NodeGetCapabilitiesResponse>, Status> {
        Ok(Response::new(NodeGetCapabilitiesResponse {
            capabilities: vec![],
        }))
    }
    async fn node_get_info(
        &self,
        _: Request<NodeGetInfoRequest>,
    ) -> Result<Response<NodeGetInfoResponse>, Status> {
        Ok(Response::new(NodeGetInfoResponse {
            node_id: self.node_id.clone(),
            max_volumes_per_node: 0,
            accessible_topology: None,
        }))
    }
}
